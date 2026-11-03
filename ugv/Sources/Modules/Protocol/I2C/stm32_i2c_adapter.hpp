#pragma once

#include "I2C_adapter.hpp"
#include "stm32h5xx_hal.h"

class STM32_I2C_Adapter final : public I2CBus {
public:
    explicit STM32_I2C_Adapter(I2C_HandleTypeDef* handle = nullptr, I2C_TypeDef* instance = I2C3) noexcept;
    ~STM32_I2C_Adapter() override = default;

    STM32_I2C_Adapter(const STM32_I2C_Adapter&) = delete;
    STM32_I2C_Adapter& operator=(const STM32_I2C_Adapter&) = delete;

    [[nodiscard]] bool init() noexcept;

    bool write_mem(uint16_t dev_addr, uint16_t mem_addr,
                   const uint8_t* data, size_t size) override;

    bool read_mem(uint16_t dev_addr, uint16_t mem_addr,
                  uint8_t* data, size_t size) override;

    bool is_device_ready(uint16_t dev_addr, uint32_t trials = 3) override;

    bool read_mem_async(uint16_t dev_addr, uint16_t mem_addr,
                        uint8_t* data_buffer, size_t size) override;

    [[nodiscard]] I2C_HandleTypeDef& get_handle() noexcept {
        return *hi2c_;
    }

private:
    I2C_HandleTypeDef owned_handle_{};
    I2C_HandleTypeDef* hi2c_{&owned_handle_};
};
