#pragma once
#include <cstdint>
#include <cstddef>

class I2CBus {
public:
    virtual ~I2CBus() = default;

    virtual bool write_mem(uint16_t dev_addr, uint16_t mem_addr,
                           const uint8_t* data, size_t size) = 0;

    virtual bool read_mem(uint16_t dev_addr, uint16_t mem_addr,
                          uint8_t* data, size_t size) = 0;

    virtual bool is_device_ready(uint16_t dev_addr, uint32_t trials = 3) = 0;

    virtual bool read_mem_async(uint16_t dev_addr, uint16_t mem_addr,
                                uint8_t* data_buffer, size_t size) = 0;
};
