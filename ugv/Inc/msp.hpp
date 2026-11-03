#pragma once

#include "stm32h5xx_hal.h"
#include <cstdint>
#include <cstddef>
#include <array>

#include "Modules/Led/led_config.hpp"
#include "Modules/Optopair/wheel_encoder.hpp"
#include "Modules/Protocol/I2C/stm32_i2c_adapter.hpp"
#include "Modules/LSM6DS/lsm6ds.hpp"
#include "Modules/Protocol/DMA/dma_manager.hpp"

namespace msp {

namespace config {
    inline constexpr uint32_t PWM_PRESCALER = 15;
    inline constexpr uint32_t PWM_PERIOD    = 999;
    inline constexpr uint32_t PWM_CHANNELS[4] = {
        TIM_CHANNEL_1,
        TIM_CHANNEL_2,
        TIM_CHANNEL_3,
        TIM_CHANNEL_4
    };

    inline constexpr uint32_t USART1_BAUDRATE = 115200;
    inline constexpr uint32_t USART2_BAUDRATE = 115200;
    inline constexpr uint32_t UART4_BAUDRATE  = 115200;
    inline constexpr uint32_t USART6_BAUDRATE = 115200;

    inline constexpr uint32_t I2C3_TIMING = 0x10B0DCFB;

    inline constexpr uint16_t LSM6DS_ADDR_PRIMARY   = (0x6A << 1);
    inline constexpr uint16_t LSM6DS_ADDR_SECONDARY = (0x6B << 1);

    inline constexpr uint8_t REG_LSM6_WHO_AM_I  = 0x0F;
    inline constexpr uint8_t REG_LSM6_CTRL1_XL  = 0x10;
    inline constexpr uint8_t REG_LSM6_CTRL3_C   = 0x12;
    inline constexpr uint8_t REG_LSM6_OUTX_L_XL = 0x28;

    inline constexpr uint16_t ENCODER_LEFT_PIN  = GPIO_PIN_14;
    inline constexpr uint16_t ENCODER_RIGHT_PIN = GPIO_PIN_15;
}

struct CheckResult {
    bool clock_ok{false};
    bool tick_timer_ok{false};
    bool leds_ok{false};
    bool tim3_motors_ok{false};
    bool tim4_motors_ok{false};
    bool tim2_motors_ok{false};
    bool usart1_ok{false};
    bool usart2_ok{false};
    bool uart4_ok{false};
    bool usart6_ok{false};
    bool i2c3_ok{false};
    bool imu_sensor_ok{false};
    uint8_t imu_chip_id{0};
    uint16_t imu_active_addr{0};
    int16_t accel_raw[3]{0, 0, 0};
    bool optopair_left_ok{false};
    bool optopair_right_ok{false};
    bool dma_ok{false};
    bool all_ok{false};
};

} // namespace msp

extern TIM_HandleTypeDef htim6;

namespace msp {
    using ::htim6;

    extern TIM_HandleTypeDef  htim2;
    extern TIM_HandleTypeDef  htim3;
    extern TIM_HandleTypeDef  htim4;
    extern UART_HandleTypeDef huart1;
    extern UART_HandleTypeDef huart2;
    extern UART_HandleTypeDef huart4;
    extern UART_HandleTypeDef huart6;
    extern I2C_HandleTypeDef  hi2c3;
    extern DMA_HandleTypeDef& hgpdma1;

    extern STM32_I2C_Adapter  i2c3_adapter;
    extern LSM6DS             imu_sensor;
    extern WheelEncoder       left_encoder;
    extern WheelEncoder       right_encoder;

    void init_system_clock() noexcept;
    bool init_tick_timer(uint32_t priority = 0) noexcept;
    void init_leds() noexcept;
    bool init_tim3_motors() noexcept;
    bool init_tim4_motors() noexcept;
    bool init_tim2_motors() noexcept;
    bool init_usart1(uint32_t baudrate = config::USART1_BAUDRATE) noexcept;
    bool init_usart2(uint32_t baudrate = config::USART2_BAUDRATE) noexcept;
    bool init_uart4(uint32_t baudrate = config::UART4_BAUDRATE) noexcept;
    bool init_usart6(uint32_t baudrate = config::USART6_BAUDRATE) noexcept;
    bool init_i2c3() noexcept;
    void init_optopairs() noexcept;
    bool init_dma() noexcept;

    bool init_all() noexcept;

    void stop_all_motors() noexcept;
    void set_left_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept;
    void set_right_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept;
    void set_axis3_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept;

    void uart6_log(const char* str) noexcept;
    void clear_uart_errors(UART_HandleTypeDef* huart) noexcept;
    void reset_i2c3_bus() noexcept;

    bool probe_and_init_lsm6ds(uint16_t* active_addr, uint8_t* chip_id) noexcept;
    bool read_lsm6ds_raw_accel(uint16_t dev_addr, int16_t raw[3]) noexcept;

    CheckResult check_peripherals(bool log_to_uart6 = true) noexcept;
}

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);

void HAL_MspInit(void);
void HAL_MspDeInit(void);

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim);

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim);
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim);

void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart);

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif
