#include "LoRa.hpp"

using namespace config::freq;

static spi_device_handle_t lora_spi_handle = NULL;

esp_err_t lora_init(spi_host_device_t host) {
    spi_device_interface_config_t lora_cfg= {};
    lora_cfg.clock_speed_hz = 9000000;
    lora_cfg.queue_size = 7;
    lora_cfg.mode = 0;
    lora_cfg.spics_io_num = LORA_CS;

    return spi_bus_add_device(host, &lora_cfg, &lora_spi_handle);
}


void write_LoRa_register(uint8_t reg, uint8_t data) { 
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 16;

    t.tx_data[0] =  reg | 0x80;
    t.tx_data[1] = data;

    spi_device_polling_transmit(lora_spi_handle, &t);
}


void write_LoRa_fifo(const uint8_t* buffer, size_t size) {
    uint8_t tx_buf[64];
    if (size + 1 > sizeof(tx_buf)) return;

    tx_buf[0] = 0x00 | 0x80;
    memcpy(&tx_buf[1], buffer, size);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (size + 1) * 8;
    t.tx_buffer = tx_buf;

    spi_device_polling_transmit(lora_spi_handle, &t);
}


void lora_setup(config::freq::Band band) {
    const auto& config = resolveConfig(band);
    
    for (const auto& it: config) {
        write_LoRa_register(it.reg, it.data);

        if (it.reg == 0x01) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}


static uint8_t ebyteChecksum(const uint8_t* buf, uint8_t len) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < len; ++i) sum += buf[i];
    return static_cast<uint8_t>(0x100 - (sum & 0xFF));
}


void packetDataTransaction(uint8_t addh, uint8_t addl, uint8_t channel, LoraPayload_t data) {
    LoraRxFrame_t inner{};
    inner.payload_len = sizeof(data);
    inner.mode = 7;
    inner.payload = data;
    inner.crc8 = CRC8_calc(reinterpret_cast<const uint8_t*>(&data), sizeof(data));

    uint8_t frame[64];
    const uint8_t innerLen = sizeof(inner);

    frame[0] = 0x40 + innerLen;       
    frame[1] = channel;               
    frame[2] = addh;                  
    frame[3] = addl;

    const uint8_t xorKey = 0xCA - channel;
    const uint8_t* innerBytes = reinterpret_cast<const uint8_t*>(&inner);
    for (uint8_t i = 0; i < innerLen; ++i) {
        frame[4 + i] = innerBytes[i] ^ xorKey;
    }

    uint8_t totalLen = 4 + innerLen;
    frame[totalLen] = ebyteChecksum(frame, totalLen);
    totalLen += 1;

    write_LoRa_register(0x0D, 0x80);
    write_LoRa_fifo(frame, totalLen);

    write_LoRa_register(0x22, totalLen);
    write_LoRa_register(0x01, 0x83);
    while ((read_LoRa_register(0x12) & 0x08) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    write_LoRa_register(0x12, 0xFF);
    write_LoRa_register(0x01, 0x81);
}


uint8_t read_LoRa_register(uint8_t reg) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 16;

    t.tx_data[0] = reg & 0x7F;
    t.tx_data[1] = 0x00;

    spi_device_polling_transmit(lora_spi_handle, &t);

    return t.rx_data[1];
}


