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

    constexpr std::array<RegConfig, 13> config_868 = {{
        {0x01, 0x80}, // RegOpMode: Sleep + LoRa
        {0x01, 0x81}, // RegOpMode: Standby

        {0x06, 0xE4}, // RegFrfMsb (868.000 MHz)
        {0x07, 0xC0}, // RegFrfMid
        {0x08, 0x00}, // RegFrfLsb

        {0x09, 0x85}, // RegPaConfig: 7 dBm (PA_BOOST)
        {0x1D, 0x72}, // RegModemConfig1: BW 125kHz, CR 4/5, Explicit Header
        {0x1E, 0x74}, // RegModemConfig2: SF7, CRC On
        {0x26, 0x04}, // RegModemConfig3: LowDataRateOptimize Off, AGC Auto On

        {0x20, 0x00}, // RegPreambleMsb
        {0x21, 0x10}, // RegPreambleLsb: 16 symbols preamble for reliable EBYTE sync
        {0x39, 0x12}, // RegSyncWord: 0x12 (Default EBYTE E32 private network)

        {0x0E, 0x80}  // RegFifoTxBaseAddr
}};

constexpr std::array<RegConfig, 13> config_433 = {{
    {0x01, 0x80}, // RegOpMode: Sleep + LoRa
    {0x01, 0x81}, // RegOpMode: Standby

    {0x06, 0x6C}, // RegFrfMsb (433.000 MHz)
    {0x07, 0x40}, // RegFrfMid
    {0x08, 0x00}, // RegFrfLsb

    {0x09, 0x85}, // RegPaConfig: ~7 dBm (PA_BOOST) — в пределах лимита 10dBm
    {0x1D, 0x92}, // RegModemConfig1: BW 500kHz, CR 4/5, Explicit Header
    {0x1E, 0xB4}, // RegModemConfig2: SF11, CRC On
    {0x26, 0x0C}, // RegModemConfig3: LowDataRateOptimize ON + AGC Auto On

    {0x20, 0x00}, // RegPreambleMsb
    {0x21, 0x08}, // RegPreambleLsb: 8 символов
    {0x39, 0x12}, // RegSyncWord: 0x12 (E32 default)

    {0x0E, 0x80}  // RegFifoTxBaseAddr
}};

    constexpr const std::array<RegConfig, 13>& resolveConfig(Band band) {
        switch (band) {
            case Band::F_433: return config_433;
            case Band::F_868: return config_868;
            default:          return config_433;
        }
    }
}

esp_err_t lora_init(spi_host_device_t host);

void write_LoRa_fifo(const uint8_t* buffer, size_t size);

void write_LoRa_register(uint8_t reg, uint8_t data);

uint8_t read_LoRa_register(uint8_t reg);

void packetDataTransaction(uint8_t addh, uint8_t addl, uint8_t channel, LoraPayload_t data);

void lora_setup(config::freq::Band band = config::freq::Band::F_433);