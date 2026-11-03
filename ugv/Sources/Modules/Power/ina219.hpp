#pragma once
#include <cstdint>
#include <optional>
#include "I2C_adapter.hpp"
#include "telemetry.hpp"
class INA219 {
public:
    static constexpr uint8_t REG_CONFIG       = 0x00;
    static constexpr uint8_t REG_SHUNTVOLTAGE = 0x01;
    static constexpr uint8_t REG_BUSVOLTAGE   = 0x02;
    static constexpr uint8_t REG_POWER        = 0x03;
    static constexpr uint8_t REG_CURRENT      = 0x04;
    static constexpr uint8_t REG_CALIBRATION  = 0x05;

    explicit INA219(I2CBus& bus, uint8_t addr = 0x40) noexcept;
    [[nodiscard]] bool init(float shunt_resistor_ohms = 0.1f, float max_expected_current_a = 3.2f) noexcept;
    [[nodiscard]] std::optional<CompactPowerTelemetry_t> read_power_data() noexcept;
    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
    [[nodiscard]] uint16_t get_dev_addr() const noexcept { return dev_addr_; }

private:
    [[nodiscard]] std::optional<uint16_t> read_reg16(uint8_t reg) noexcept;
    [[nodiscard]] bool write_reg16(uint8_t reg, uint16_t val) noexcept;
    I2CBus& bus_;
    uint16_t dev_addr_;
    float shunt_ohms_{0.1f};
    float current_lsb_ma_{1.0f};
    bool initialized_{false};
};
