#include "lsm6ds.hpp"
#include "stm32h5xx_hal.h"

LSM6DS::LSM6DS(I2CBus& bus, uint8_t ctrl1_val) noexcept
    : bus_(bus),
      ctrl1_val_(ctrl1_val) {}

std::optional<uint16_t> LSM6DS::get_active_sensor_addr() noexcept {
    constexpr uint16_t ADDR_A = 0x6A << 1;
    constexpr uint16_t ADDR_B = 0x6B << 1;

    if (bus_.is_device_ready(ADDR_A, 3)) {
        return ADDR_A;
    }
    if (bus_.is_device_ready(ADDR_B, 3)) {
        return ADDR_B;
    }
    return std::nullopt;
}

std::optional<uint8_t> LSM6DS::ack_chip_id() noexcept {
    auto addr = get_active_sensor_addr();
    if (!addr.has_value()) {
        return std::nullopt;
    }
    active_sensor_addr_ = *addr;

    uint8_t id = 0;
    if (!bus_.read_mem(active_sensor_addr_, REG_WHO_AM_I, &id, 1)) {
        return std::nullopt;
    }
    return id;
}

bool LSM6DS::init() noexcept {
    auto id = ack_chip_id();
    if (!id.has_value()) {
        initialized_ = false;
        return false;
    }
    chip_id_ = *id;

    if (!bus_.write_mem(active_sensor_addr_, REG_CTRL1_XL, &ctrl1_val_, 1)) {
        initialized_ = false;
        return false;
    }

    HAL_Delay(30);
    initialized_ = true;
    return true;
}

std::optional<LSM6DS::AxesRaw> LSM6DS::read_data() noexcept {
    if (!initialized_ || active_sensor_addr_ == 0) {
        return std::nullopt;
    }
    uint8_t raw[6] = {0};
    if (!bus_.read_mem(active_sensor_addr_, REG_OUTX_L_XL, raw, sizeof(raw))) {
        return std::nullopt;
    }

    return parse_raw_buffer(raw);
}

bool LSM6DS::start_read_async(uint8_t* raw_buf_6b) noexcept {
    if (!initialized_ || active_sensor_addr_ == 0 || raw_buf_6b == nullptr) {
        return false;
    }
    return bus_.read_mem_async(active_sensor_addr_, REG_OUTX_L_XL, raw_buf_6b, 6);
}

LSM6DS::AxesRaw LSM6DS::parse_raw_buffer(const uint8_t* buf) noexcept {
    if (buf == nullptr) return {};
    return AxesRaw{
        .x = static_cast<int16_t>(buf[0] | (static_cast<uint16_t>(buf[1]) << 8)),
        .y = static_cast<int16_t>(buf[2] | (static_cast<uint16_t>(buf[3]) << 8)),
        .z = static_cast<int16_t>(buf[4] | (static_cast<uint16_t>(buf[5]) << 8))
    };
}
