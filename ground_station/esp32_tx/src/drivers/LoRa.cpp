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

void lora_setup(config::freq::Band band) {
    const auto& config = resolveConfig(band);
    
    for (const auto& it: config) {
        write_LoRa_register(it.reg, it.data);

        if (it.reg == 0x01) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void packetDataTransaction(uint8_t addh, uint8_t addl, uint8_t channel, LoraPayload_t data) {
    LoraTxFrame_t tx_frame;
    tx_frame.addh = addh;
    tx_frame.addl = addl;
    tx_frame.channel = channel;
    tx_frame.payload_len = sizeof(data);
    tx_frame.mode = 7;
    tx_frame.payload = data;
    tx_frame.crc8 = CRC8_calc(reinterpret_cast<const uint8_t*>(&data), sizeof(data));
    
    write_LoRa_register(0x0D, 0x80);
    const uint8_t* transactionData = reinterpret_cast<const uint8_t*>(&tx_frame);

    for (int i = 0; i < sizeof(tx_frame); i++) {
        write_LoRa_register(0x00, transactionData[i]);
    }

    write_LoRa_register(0x22, sizeof(tx_frame));

    write_LoRa_register(0x01, 0x83);

    while ((read_LoRa_register(0x12) & 0x08) == 0) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }   
    write_LoRa_register(0x12, 0x08);
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


