#pragma once

#include <cstdint>
#include <optional>
#include "I2C_adapter.hpp"

class LSM6DS {
public:
    struct AxesRaw {
        int16_t x{0};
        int16_t y{0};
        int16_t z{0};
    };

    static constexpr uint8_t REG_WHO_AM_I  = 0x0F;
    static constexpr uint8_t REG_CTRL1_XL  = 0x10;
    static constexpr uint8_t REG_OUTX_L_XL = 0x28;

    explicit LSM6DS(I2CBus& bus, uint8_t ctrl1_val = 0x40) noexcept;
    ~LSM6DS() = default;

    LSM6DS(const LSM6DS&) = delete;
    LSM6DS& operator=(const LSM6DS&) = delete;

    [[nodiscard]] bool init() noexcept;


    [[nodiscard]] std::optional<AxesRaw> read_data() noexcept;

    [[nodiscard]] bool start_read_async(uint8_t* raw_buf_6b) noexcept;

    [[nodiscard]] static AxesRaw parse_raw_buffer(const uint8_t* buf) noexcept;

    [[nodiscard]] uint8_t get_chip_id() const noexcept { return chip_id_; }
    [[nodiscard]] uint16_t get_active_addr() const noexcept { return active_sensor_addr_; }
    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
    [[nodiscard]] uint8_t get_ctrl1_val() const noexcept { return ctrl1_val_; }
    void set_ctrl1_val(uint8_t val) noexcept { ctrl1_val_ = val; }

private:
    [[nodiscard]] std::optional<uint16_t> get_active_sensor_addr() noexcept;
    [[nodiscard]] std::optional<uint8_t> ack_chip_id() noexcept;

    I2CBus& bus_;
    uint8_t ctrl1_val_;
    uint8_t chip_id_{0};
    uint16_t active_sensor_addr_{0};
    bool initialized_{false};
};
