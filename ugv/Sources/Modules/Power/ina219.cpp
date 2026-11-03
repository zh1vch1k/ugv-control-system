#include "ina219.hpp"
#include <algorithm>
#include <cmath>

INA219::INA219(I2CBus& bus, uint8_t addr) noexcept
    : bus_(bus), dev_addr_(static_cast<uint16_t>(addr << 1)) {}

bool INA219::write_reg16(uint8_t reg, uint16_t val) noexcept {
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(val & 0xFF);
    return bus_.write_mem(dev_addr_, reg, buf, 2);
}

std::optional<uint16_t> INA219::read_reg16(uint8_t reg) noexcept {
    uint8_t buf[2] = {0, 0};
    if (!bus_.read_mem(dev_addr_, reg, buf, 2)) {
        return std::nullopt;
    }

    return static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
}

bool INA219::init(float shunt_resistor_ohms, float max_expected_current_a) noexcept {
    if (!bus_.is_device_ready(dev_addr_, 3)) {
        initialized_ = false;
        return false;
    }

    shunt_ohms_ = shunt_resistor_ohms;


    write_reg16(REG_CONFIG, 0x8000);

    if (shunt_ohms_ < 0.05f) {
        cal_val_ = 4096;
        current_lsb_ma_ = 1.0f;
    } else {
        cal_val_ = 4096;
        current_lsb_ma_ = 0.1f;
    }

    write_reg16(REG_CALIBRATION, cal_val_);

    if (!write_reg16(REG_CONFIG, 0x399F)) {
        initialized_ = false;
        return false;
    }

    initialized_ = true;
    return true;
}

std::optional<CompactPowerTelemetry_t> INA219::read_power_data() noexcept {
    if (!initialized_) {
        return std::nullopt;
    }

    auto raw_bus = read_reg16(REG_BUSVOLTAGE);
    if (!raw_bus.has_value()) {
        return std::nullopt;
    }

    uint16_t bus_voltage_mv = static_cast<uint16_t>((raw_bus.value() >> 3) * 4);

    int16_t current_ma = 0;
    auto raw_current = read_reg16(REG_CURRENT);
    if (raw_current.has_value() && raw_current.value() != 0) {
        current_ma = static_cast<int16_t>(static_cast<int16_t>(raw_current.value()) * current_lsb_ma_);
    } else {
        auto raw_shunt = read_reg16(REG_SHUNTVOLTAGE);
        if (raw_shunt.has_value()) {
            float shunt_mv = static_cast<int16_t>(raw_shunt.value()) * 0.01f;
            current_ma = static_cast<int16_t>(shunt_mv / shunt_ohms_);
        }
    }

    int32_t power_mw = (static_cast<int32_t>(bus_voltage_mv) * std::abs(static_cast<int32_t>(current_ma))) / 1000;
    uint8_t power_w = static_cast<uint8_t>(std::min(255L, power_mw / 1000));

    CompactPowerTelemetry_t telemetry = {};
    telemetry.bus_voltage_mv = bus_voltage_mv;
    telemetry.current_ma     = current_ma;
    telemetry.power_w        = power_w;

    return telemetry;
}
