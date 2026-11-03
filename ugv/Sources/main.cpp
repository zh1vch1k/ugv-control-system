#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <optional>
#include <cstdio>
#include <atomic>
#include <array>

#include "msp.hpp"
#include "it.hpp"
#include "Modules/Led/led_config.hpp"
#include "packet.hpp"
#include "fsm.hpp"
#include "key.hpp"
#include "speck.hpp"

inline constexpr size_t   FRAME_SIZE      = 8;
inline constexpr uint8_t  ACK_CMD         = 0xA5;
inline constexpr uint32_t STM32_ACK_MAGIC = 0xA4A3A2A1;

static constexpr uint32_t GPS_BAUDRATES[] = { 115200, 9600, 38400 };
static size_t current_baud_idx = 0;
static bool baud_locked = false;

static uint8_t packet_buffer[FRAME_SIZE] = {};
static uint32_t session_id = 0;
static RadioFSM state = RadioFSM::IDLE;

static SpeckContext_t crypto_ctx = {};
static SpeckSessionCounter_t rx_cnt = {};
static uint32_t active_timeout_count = 0;
static uint32_t corrupted_packet_count = 0;

namespace DriveConfig {
    constexpr int16_t ThrottleDeadband = 6;
    constexpr int16_t SteeringDeadband = 6;
    constexpr uint32_t MinStartingPwm  = 200; 
    constexpr uint32_t MaxArr          = msp::config::PWM_PERIOD;
}

void command_task(uint8_t speed_fwd, uint8_t turn_speed) noexcept {
    const int16_t throttle = static_cast<int16_t>(speed_fwd) - 64;
    const int16_t steering = static_cast<int16_t>(turn_speed) - 64;

    const int16_t th = (std::abs(throttle) < DriveConfig::ThrottleDeadband) ? 0 : throttle;
    const int16_t st = (std::abs(steering) < DriveConfig::SteeringDeadband) ? 0 : steering;

    if (th == 0 && st == 0) {
        msp::stop_all_motors();
        return;
    }

    const float norm_th = static_cast<float>(th) / 64.0f;
    const float norm_st = static_cast<float>(st) / 64.0f;

    const float expo_th = 0.6f * (norm_th * norm_th * norm_th) + 0.4f * norm_th;
    const float expo_st = 0.6f * (norm_st * norm_st * norm_st) + 0.4f * norm_st;

    float left  = expo_th - expo_st;
    float right = expo_th + expo_st;

    const float max_mag = std::max(std::abs(left), std::abs(right));
    if (max_mag > 1.0f) {
        left  /= max_mag;
        right /= max_mag;
    }

    auto calc_pwm = [](float power) -> uint32_t {
        const float abs_p = std::abs(power);
        if (abs_p < 0.02f) return 0;
        return DriveConfig::MinStartingPwm + 
               static_cast<uint32_t>(abs_p * (DriveConfig::MaxArr - DriveConfig::MinStartingPwm));
    };

    const uint32_t pwm_l = calc_pwm(left);
    const uint32_t pwm_r = calc_pwm(right);

    if (left > 0.0f) {
        msp::set_left_motors_pwm(pwm_l, 0);
    } else if (left < 0.0f) {
        msp::set_left_motors_pwm(0, pwm_l);
    } else {
        msp::set_left_motors_pwm(0, 0);
    }

    if (right > 0.0f) {
        msp::set_right_motors_pwm(pwm_r, 0);
    } else if (right < 0.0f) {
        msp::set_right_motors_pwm(0, pwm_r);
    } else {
        msp::set_right_motors_pwm(0, 0);
    }
}

void test_tim3_motors_ladder() noexcept {
    msp::uart6_log("\r\n=======================================================\r\n");
    msp::uart6_log("[TEST] STARTING TIM3 (LEFT MOTORS: PA6, PA7, PB0, PB1)\r\n");
    msp::uart6_log("=======================================================\r\n");

    msp::stop_all_motors();
    RGBLed::setColor(RgbColor::Yellow);
    board_led::off();
    HAL_Delay(1000);

    const uint32_t pwm_steps[] = { 250, 500, 750, 999 };

    msp::uart6_log("[TEST] Phase 1: FORWARD Ladder on PA6 (CH1) and PB0 (CH3)\r\n");
    RGBLed::setColor(RgbColor::Green);

    for (uint32_t pwm : pwm_steps) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "  -> Forward PWM = %lu / 999 (%lu%%)\r\n", pwm, (pwm * 100) / 999);
        msp::uart6_log(msg);

        msp::set_left_motors_pwm(pwm, 0);
        board_led::toggle();
        HAL_Delay(1200);
    }

    msp::stop_all_motors();
    RGBLed::setColor(RgbColor::Yellow);
    msp::uart6_log("[TEST] Pause between phases: STOP (1 sec)...\r\n");
    HAL_Delay(1000);

    msp::uart6_log("[TEST] Phase 2: REVERSE Ladder on PA7 (CH2) and PB1 (CH4)\r\n");
    RGBLed::setColor(RgbColor::Cyan);

    for (uint32_t pwm : pwm_steps) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "  -> Reverse PWM = %lu / 999 (%lu%%)\r\n", pwm, (pwm * 100) / 999);
        msp::uart6_log(msg);

        msp::set_left_motors_pwm(0, pwm);
        board_led::toggle();
        HAL_Delay(1200);
    }

    msp::stop_all_motors();
    RGBLed::setColor(RgbColor::Off);
}

void UART4_SetBaudRate(uint32_t baud) noexcept {
    msp::huart4.Init.BaudRate = baud;
    if (HAL_UART_Init(&msp::huart4) == HAL_OK) {
        msp::clear_uart_errors(&msp::huart4);
        ATOMIC_SET_BIT(msp::huart4.Instance->CR1, USART_CR1_RXNEIE_RXFNEIE);
        char log_buf[64];
        std::snprintf(log_buf, sizeof(log_buf), "[GPS] UART4 baud changed to %lu\r\n", baud);
        msp::uart6_log(log_buf);
    }
}

int main(void) {
    msp::init_all();
    msp::stop_all_motors();

    speck_init(&crypto_ctx, RFBP_SECRET_KEY);

    msp::CheckResult post = msp::check_peripherals(true);

    uint32_t last_i2c_req = 0;
    uint32_t last_accel_log = 0;
    uint32_t last_baud_check = 0;

    while (1) {
        switch (state) {
            case RadioFSM::IDLE: {
                msp::clear_uart_errors(&msp::huart2);
                std::memset(packet_buffer, 0, FRAME_SIZE);

                if (HAL_UART_Receive(&msp::huart2, packet_buffer, FRAME_SIZE, 10) == HAL_OK) {
                    int ack_idx = -1;
                    for (int i = 4; i < static_cast<int>(FRAME_SIZE); ++i) {
                        if (packet_buffer[i] == ACK_CMD) {
                            ack_idx = i;
                            break;
                        }
                    }

                    if (ack_idx >= 4) {
                        std::memcpy(&session_id, &packet_buffer[ack_idx - 4], sizeof(uint32_t));
                        rx_cnt.session_id = session_id;
                        rx_cnt.cnt        = 0;

                        const uint32_t ack_resp = STM32_ACK_MAGIC;
                        HAL_UART_Transmit(&msp::huart2, reinterpret_cast<const uint8_t*>(&ack_resp), sizeof(uint32_t), 50);

                        board_led::on();
                        RGBLed::setColor(RgbColor::Green);

                        active_timeout_count = 0;
                        corrupted_packet_count = 0;
                        state = RadioFSM::ACTIVE;
                    } else {
                        RGBLed::setColor(RgbColor::Yellow);
                    }
                }
                break;
            }

            case RadioFSM::ACTIVE: {
                msp::clear_uart_errors(&msp::huart2);

                if (HAL_UART_Receive(&msp::huart2, packet_buffer, FRAME_SIZE, 10) == HAL_OK) {
                    if (packet_buffer[4] == ACK_CMD && packet_buffer[5] == 0x00 &&
                        packet_buffer[6] == 0x00 && packet_buffer[7] == 0x00) {

                        std::memcpy(&session_id, &packet_buffer[0], sizeof(uint32_t));
                        rx_cnt.session_id = session_id;
                        rx_cnt.cnt        = 0;

                        const uint32_t ack_resp = STM32_ACK_MAGIC;
                        HAL_UART_Transmit(&msp::huart2, reinterpret_cast<const uint8_t*>(&ack_resp), sizeof(uint32_t), 50);
                        RGBLed::setColor(RgbColor::Cyan);
                        active_timeout_count = 0;
                        break;
                    }

                    auto frame = decodePacket(packet_buffer, FRAME_SIZE, &crypto_ctx, &rx_cnt);
                    if (frame.has_value()) {
                        active_timeout_count = 0;
                        corrupted_packet_count = 0;
                        board_led::toggle();

                        HAL_UART_Transmit(&msp::huart6, reinterpret_cast<const uint8_t*>(&frame.value()), FRAME_SIZE, 50);
                        const uint8_t speed_fwd  = frame->payload.st_r_ax_y;
                        const uint8_t turn_speed = frame->payload.st_r_ax_x;
                        const uint8_t mode       = frame->mode;

                        RGBLed::setColor(static_cast<RgbColor>(mode & 0x07U));
                        command_task(speed_fwd, turn_speed);
                    } else {
                        RGBLed::setColor(RgbColor::Red);
                        corrupted_packet_count++;
                        if (corrupted_packet_count >= 3) {
                            msp::stop_all_motors();
                        }
                    }
                } else {
                    msp::stop_all_motors();
                    __HAL_UART_SEND_REQ(&msp::huart2, UART_RXDATA_FLUSH_REQUEST);
                    active_timeout_count++;
                    if (active_timeout_count >= 100) {
                        RGBLed::setColor(RgbColor::Blue);
                        board_led::off();
                        session_id = 0;
                        state = RadioFSM::IDLE;
                    }
                }
                break;
            }

            case RadioFSM::ERROR:
            default: {
                msp::stop_all_motors();
                RGBLed::setColor(RgbColor::Red);
                board_led::off();
                session_id = 0;
                state = RadioFSM::IDLE;
                break;
            }
        }

        if (post.imu_sensor_ok && (HAL_GetTick() - last_i2c_req >= 40)) {
            last_i2c_req = HAL_GetTick();

            auto accel = msp::imu_sensor.read_data();
            if (accel.has_value()) {
                uint8_t res = 0;
                if (std::abs(accel->x) > 2500) res |= static_cast<uint8_t>(RgbColor::Red);
                if (std::abs(accel->y) > 2500) res |= static_cast<uint8_t>(RgbColor::Green);
                if (std::abs(accel->z) > 2500) res |= static_cast<uint8_t>(RgbColor::Blue);

                if (state == RadioFSM::IDLE && res != 0) {
                    RGBLed::setColor(static_cast<RgbColor>(res));
                }
            }
        }

        if (post.imu_sensor_ok && (HAL_GetTick() - last_accel_log >= 1000)) {
            last_accel_log = HAL_GetTick();
            auto accel = msp::imu_sensor.read_data();
            if (accel.has_value()) {
                char dbg[80];
                std::snprintf(dbg, sizeof(dbg), "[ACCEL] X:%6d  Y:%6d  Z:%6d\r\n",
                              accel->x, accel->y, accel->z);
                msp::uart6_log(dbg);
            }
        }

        if (msp::left_encoder.check_and_clear_rev() || msp::right_encoder.check_and_clear_rev() ||
            isr_state::encoder_ready.exchange(false, std::memory_order_relaxed)) {
            board_led::toggle();
        }

        if (isr_state::gps_nmea_received.exchange(false, std::memory_order_acq_rel)) {
            baud_locked = true;
            board_led::toggle();
            msp::uart6_log(reinterpret_cast<const char*>(isr_state::gps_rx_line));
            msp::uart6_log("\r\n");
        }

        if (!baud_locked && (HAL_GetTick() - last_baud_check >= 3000)) {
            last_baud_check = HAL_GetTick();
            current_baud_idx = (current_baud_idx + 1) % (sizeof(GPS_BAUDRATES) / sizeof(GPS_BAUDRATES[0]));
            UART4_SetBaudRate(GPS_BAUDRATES[current_baud_idx]);
        }
    }
}
