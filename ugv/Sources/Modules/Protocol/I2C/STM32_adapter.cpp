#include "stm32_i2c_adapter.hpp"

STM32_I2C_Adapter::STM32_I2C_Adapter(I2C_HandleTypeDef* handle, I2C_TypeDef* instance) noexcept {
    if (handle != nullptr) {
        hi2c_ = handle;
    } else {
        hi2c_ = &owned_handle_;
    }
    if (hi2c_->Instance == nullptr) {
        hi2c_->Instance = instance;
    }
}

bool STM32_I2C_Adapter::init() noexcept {
    if (hi2c_ == nullptr) return false;

    hi2c_->Init.Timing          = 0x10B0DCFB;
    hi2c_->Init.OwnAddress1     = 0;
    hi2c_->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c_->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c_->Init.OwnAddress2     = 0;
    hi2c_->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c_->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    return HAL_I2C_Init(hi2c_) == HAL_OK;
}

bool STM32_I2C_Adapter::write_mem(uint16_t dev_addr, uint16_t mem_addr,
                                 const uint8_t* data, size_t size) {
    if (hi2c_ == nullptr || hi2c_->Instance == nullptr) return false;
    return HAL_I2C_Mem_Write(hi2c_, dev_addr, mem_addr,
                             I2C_MEMADD_SIZE_8BIT,
                             const_cast<uint8_t*>(data),
                             static_cast<uint16_t>(size), 100) == HAL_OK;
}

bool STM32_I2C_Adapter::read_mem(uint16_t dev_addr, uint16_t mem_addr,
                                uint8_t* data, size_t size) {
    if (hi2c_ == nullptr || hi2c_->Instance == nullptr) return false;
    return HAL_I2C_Mem_Read(hi2c_, dev_addr, mem_addr,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            static_cast<uint16_t>(size), 100) == HAL_OK;
}

bool STM32_I2C_Adapter::is_device_ready(uint16_t dev_addr, uint32_t trials) {
    if (hi2c_ == nullptr || hi2c_->Instance == nullptr) return false;
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c_, dev_addr, trials, 50);
    if (status != HAL_OK) {
        __HAL_I2C_CLEAR_FLAG(hi2c_, I2C_FLAG_AF | I2C_FLAG_BERR | I2C_FLAG_OVR);
    }
    return status == HAL_OK;
}

bool STM32_I2C_Adapter::read_mem_async(uint16_t dev_addr, uint16_t mem_addr,
                                       uint8_t* data_buffer, size_t size) {
    if (hi2c_ == nullptr || hi2c_->Instance == nullptr || hi2c_->State != HAL_I2C_STATE_READY) {
        return false;
    }
    return HAL_I2C_Mem_Read_IT(hi2c_, dev_addr, mem_addr,
                               I2C_MEMADD_SIZE_8BIT,
                               data_buffer,
                               static_cast<uint16_t>(size)) == HAL_OK;
}
