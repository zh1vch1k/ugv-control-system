#include "LoRa.hpp"
#include "esp_log.h"

using namespace config::freq;

static const char* TAG_LORA = "LORA_DRV";
static spi_device_handle_t lora_spi_handle = NULL;

esp_err_t lora_init(spi_host_device_t host) {
    spi_device_interface_config_t lora_cfg = {};
    lora_cfg.clock_speed_hz = 9000000;
    lora_cfg.queue_size = 7;
    lora_cfg.mode = 0;
    lora_cfg.spics_io_num = LORA_CS;

    esp_err_t err = spi_bus_add_device(host, &lora_cfg, &lora_spi_handle);
    ESP_LOGI(TAG_LORA, "spi_bus_add_device: %s", esp_err_to_name(err));
    return err;
}

void write_LoRa_register(uint8_t reg, uint8_t data) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 16;

    t.tx_data[0] = reg | 0x80;
    t.tx_data[1] = data;

    esp_err_t err = spi_device_polling_transmit(lora_spi_handle, &t);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_LORA, "SPI write reg=0x%02X failed: %s", reg, esp_err_to_name(err));
    }
}

uint8_t read_LoRa_register(uint8_t reg) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 16;

    t.tx_data[0] = reg & 0x7F;
    t.tx_data[1] = 0x00;

    esp_err_t err = spi_device_polling_transmit(lora_spi_handle, &t);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_LORA, "SPI read reg=0x%02X failed: %s", reg, esp_err_to_name(err));
    }

    return t.rx_data[1];
}

void write_LoRa_fifo(const uint8_t* buffer, size_t size) {
    uint8_t tx_buf[64];
    if (size + 1 > sizeof(tx_buf)) {
        ESP_LOGE(TAG_LORA, "write_LoRa_fifo: size %zu too large", size);
        return;
    }

    tx_buf[0] = 0x00 | 0x80;
    memcpy(&tx_buf[1], buffer, size);

    char hex[196] = {0};
    int off = 0;
    for (size_t i = 0; i < size; i++) {
        off += snprintf(hex + off, sizeof(hex) - off, "%02X ", buffer[i]);
    }
    ESP_LOGI(TAG_LORA, "FIFO WRITE (%zu bytes): %s", size, hex);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (size + 1) * 8;
    t.tx_buffer = tx_buf;

    spi_device_polling_transmit(lora_spi_handle, &t);
}

void read_LoRa_fifo(uint8_t* buffer, uint8_t size) {
    if (size > 63) {
        ESP_LOGE(TAG_LORA, "read_LoRa_fifo: size %u too large", size);
        return;
    }

    uint8_t tx_buf[64] = {0};
    uint8_t rx_buf[64] = {0};

    tx_buf[0] = 0x00;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (size + 1) * 8;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;

    spi_device_polling_transmit(lora_spi_handle, &t);

    memcpy(buffer, &rx_buf[1], size);

    char hex[196] = {0};
    int off = 0;
    for (uint8_t i = 0; i < size; i++) {
        off += snprintf(hex + off, sizeof(hex) - off, "%02X ", buffer[i]);
    }
    ESP_LOGI(TAG_LORA, "FIFO READ (%u bytes): %s", size, hex);
}

void lora_setup(config::freq::Band band) {
    const auto& config = resolveConfig(band);

    for (const auto& it : config) {
        write_LoRa_register(it.reg, it.data);

        if (it.reg == 0x01) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }


    ESP_LOGI(TAG_LORA, "=== Register verification after setup ===");
    uint8_t opmode   = read_LoRa_register(0x01);
    uint8_t frf_msb  = read_LoRa_register(0x06);
    uint8_t frf_mid  = read_LoRa_register(0x07);
    uint8_t frf_lsb  = read_LoRa_register(0x08);
    uint8_t modcfg1  = read_LoRa_register(0x1D);
    uint8_t modcfg2  = read_LoRa_register(0x1E);
    uint8_t modcfg3  = read_LoRa_register(0x26);
    uint8_t syncword = read_LoRa_register(0x39);
    uint8_t version  = read_LoRa_register(0x42);

    ESP_LOGI(TAG_LORA, "RegVersion (0x42) = 0x%02X (should be 0x12 for SX1276/78)", version);
    ESP_LOGI(TAG_LORA, "RegOpMode  (0x01) = 0x%02X", opmode);
    ESP_LOGI(TAG_LORA, "Frf MSB/MID/LSB   = 0x%02X 0x%02X 0x%02X", frf_msb, frf_mid, frf_lsb);
    ESP_LOGI(TAG_LORA, "ModemConfig1/2/3  = 0x%02X 0x%02X 0x%02X", modcfg1, modcfg2, modcfg3);
    ESP_LOGI(TAG_LORA, "SyncWord   (0x39) = 0x%02X", syncword);
    ESP_LOGI(TAG_LORA, "==========================================");
}

uint8_t ebyteChecksum(const uint8_t* buf, uint8_t len) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < len; ++i) sum += buf[i];
    return static_cast<uint8_t>(0x100 - (sum & 0xFF));
}

uint8_t receiveEbyteRadioFrame(uint8_t* rx_buffer, uint32_t timeout_ms) {
    write_LoRa_register(0x01, 0x85); // RX continuous

    uint32_t start_tick = xTaskGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    uint32_t poll_count = 0;

    uint8_t irq_flags = 0;
    while (!((irq_flags = read_LoRa_register(0x12)) & (0x40 | 0x20))) {
        poll_count++;
        if ((xTaskGetTickCount() - start_tick) > timeout_ticks) {
            uint8_t rssi = read_LoRa_register(0x1B); 
            ESP_LOGW(TAG_LORA, "RX TIMEOUT after %lu polls. Last IRQ=0x%02X, RSSI=%d dBm",
                     poll_count, irq_flags, (int8_t)(-157 + rssi));
            write_LoRa_register(0x01, 0x81);
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_LOGI(TAG_LORA, "Radio event! IRQ flags=0x%02X after %lu polls", irq_flags, poll_count);

    write_LoRa_register(0x12, 0xFF);

    if (irq_flags & 0x20) {
        ESP_LOGW(TAG_LORA, "PayloadCrcError - пакет долетів, але побився CRC!");
        write_LoRa_register(0x01, 0x81);
        return 0;
    }

    uint8_t rx_len = read_LoRa_register(0x13);
    uint8_t start_pos = read_LoRa_register(0x10);
    int8_t rssi_pkt = read_LoRa_register(0x1A) - 137;

    ESP_LOGI(TAG_LORA, "Packet received! len=%u, RSSI=%d dBm", rx_len, rssi_pkt);

    write_LoRa_register(0x0D, start_pos);
    read_LoRa_fifo(rx_buffer, rx_len);
    write_LoRa_register(0x01, 0x81);

    return rx_len;
}