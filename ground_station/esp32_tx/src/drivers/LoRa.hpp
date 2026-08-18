#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "packet.hpp"
#include <cstring>
#include <array>
#include <iostream>
#include "freertos/task.h"


#define LORA_MOSI 27
#define LORA_MISO 19
#define LORA_CS   18
#define LORA_SCLK  5
#define LORA_DIO  26
#define LORA_RST  GPIO_NUM_23

namespace config::freq {
    typedef struct { 
        uint8_t reg;
        uint8_t data;
    } RegConfig;

    enum class Band {
        F_433,
        F_868
    };

    constexpr std::array<RegConfig, 10> config_868 = {{
    {0x01, 0x80}, // RegOpMode = Sleep + LoRa
    {0x01, 0x81}, // RegOpMode = Standby

    {0x06, 0xE4}, // RegFrfMsb
    {0x07, 0xC0}, // RegFrfMid
    {0x08, 0x00}, // RegFrfLsb

    {0x09, 0x8F}, // RegPaConfig (max power)
    {0x1D, 0x72}, // RegModemConfig1 (BW 125kHz, CR 4/5)
    {0x1E, 0x74}, // RegModemConfig2 (SF7, CRC On)

    {0x0E, 0x80}, // RegFifoTxBaseAddr
    {0x0F, 0x00}  // RegFifoRxBaseAddr
}};

    constexpr std::array<RegConfig, 10> config_433 = {{
    {0x01, 0x80}, // RegOpMode: Sleep + LoRa
    {0x01, 0x81}, // RegOpMode: Standby

    {0x06, 0x6C}, // RegFrfMsb
    {0x07, 0x40}, // RegFrfMid
    {0x08, 0x00}, // RegFrfLsb

    {0x09, 0x85}, // RegPaConfig 
    {0x1D, 0x72}, // RegModemConfig1 
    {0x1E, 0x94}, // RegModemConfig2 

    {0x0E, 0x80}, // RegFifoTxBaseAddr
    {0x0F, 0x00}  // RegFifoRxBaseAddr 
    }};

    constexpr const std::array<RegConfig, 10>& resolveConfig(Band band) {
        switch (band) {
            case Band::F_433: return config_433;
            case Band::F_868: return config_868;
            default:          return config_433;
        }
    }
}

esp_err_t lora_init(spi_host_device_t host);

void write_LoRa_register(uint8_t reg, uint8_t data);

uint8_t read_LoRa_register(uint8_t reg);

void packetDataTransaction(uint8_t addh, uint8_t addl, uint8_t channel, LoraPayload_t data);

void lora_setup(config::freq::Band band = config::freq::Band::F_433);