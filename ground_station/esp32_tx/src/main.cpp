#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <atomic>
#include <cmath>

#include "fsm.hpp"
#include "packet.hpp"
#include "esp_random.h"
#include "esp_log.h"
#include "drivers/oled.hpp"
#include "drivers/LoRa.hpp"
#include "utils/packet_bridge.hpp"
#include "utils/controller.hpp"



QueueHandle_t controller_queue_handle = NULL;
static std::atomic<uint32_t> lora_count{0};

static Controller generateTestController() {
    static uint32_t tick = 0;
    tick++;

    Controller pad{};
    pad.start_byte = 0xAA;

    pad.left_x  = static_cast<uint8_t>((sinf(tick * 0.05f) * 0.5f + 0.5f) * 127.0f);
    pad.left_y  = static_cast<uint8_t>((sinf(tick * 0.05f + 1.57f) * 0.5f + 0.5f) * 127.0f);
    pad.right_x = static_cast<uint8_t>((sinf(tick * 0.08f + 3.14f) * 0.5f + 0.5f) * 127.0f);
    pad.right_y = static_cast<uint8_t>((sinf(tick * 0.08f + 4.71f) * 0.5f + 0.5f) * 127.0f);

    pad.l_trigger = tick % 16;
    pad.r_trigger = 15 - (tick % 16);
    pad.btn_mask  = static_cast<uint8_t>((tick / 20) & 0x0F);

    return pad;
}


void draw_controller_screen(void* pv) {
    char buffer[64];
    int cnt = 0;
    Controller pad;

    oled_clear();
    oled_print(0, 0, "STATUS: TEST MODE");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();

    while (1) {
        if (xQueueReceive(controller_queue_handle, &pad, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint8_t btnMask = pad.btn_mask;

            snprintf(buffer, sizeof(buffer), "SENT:%-5lu <-> %-4d",
                     lora_count.load(std::memory_order_relaxed), ++cnt);
            oled_print(0, 0, buffer);
            oled_print(1, 0, "Synthetic data");

            snprintf(buffer, sizeof(buffer), "LX:%-3u | LY:%-3u", pad.left_x, pad.left_y);
            oled_print(2, 4, buffer);

            snprintf(buffer, sizeof(buffer), "RX:%-3u | RY:%-3u", pad.right_x, pad.right_y);
            oled_print(3, 4, buffer);

            snprintf(buffer, sizeof(buffer), "LT: %d | RT:%-3u", pad.l_trigger, pad.r_trigger);
            oled_print(4, 4, buffer);

            oled_print(5, 0, "Buttons");
            snprintf(buffer, sizeof(buffer), "MASK: 0x%02X", btnMask);
            oled_print(6, 4, buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


void broadcast(void* pv) {
    static const char *TAG = "LORA_TEST";

    uint32_t last_poll = 0;
    const uint16_t session_timeout_ms = 1500;
    const uint16_t polling_timeout_ms = 150;
    uint8_t rx_buffer[64];

    Controller pad;
    uint32_t session_id = esp_random();

    SpeckContext_t crypto_ctx = {};
    speck_init(&crypto_ctx, RFBP_SECRET_KEY);
    static RadioFSM state = RadioFSM::IDLE;

    SpeckSessionCounter_t session_cnt = {session_id, 0};

    while (1) {
        switch (state) {
            case RadioFSM::IDLE: {
                packetDataTransaction(0x00, 0x01, 0x17, TX_SESSION_SALT, &crypto_ctx, &session_cnt);
                ESP_LOGI(TAG, "Handshake request sent, waiting for response...");

                uint8_t rx_len = receiveEbyteRadioFrame(rx_buffer, 1000);

                if (rx_len == 0) {
                    ESP_LOGW(TAG, "No response (timeout)");
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
                }

                if ((rx_len - 5) == sizeof(HandshakeRx_t)) {
                    HandshakeRx_t resp = *(reinterpret_cast<HandshakeRx_t*>(rx_buffer+4));

                    if (resp.rx_session_salt == TX_SESSION_SALT) {
                        session_id = resp.session_id;
                        state = RadioFSM::ACTIVE;
                        last_poll = xTaskGetTickCount();
                        ESP_LOGI(TAG, "Handshake successful! session_id: %lu", session_id);
                    } else {
                        ESP_LOGW(TAG, "Salt mismatch: got 0x%08lX, expected 0x%08lX",
                                 resp.rx_session_salt, (uint32_t)RX_SESSION_SALT);
                    }
                } else {
                    ESP_LOGW(TAG, "Unexpected frame length: %d (bytes: %02X %02X %02X %02X)",
                             rx_len, rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
                }
                break;
            }

            case RadioFSM::ACTIVE: {
                TickType_t current_tick = xTaskGetTickCount();

                if ((current_tick - last_poll) > pdMS_TO_TICKS(session_timeout_ms)) {
                    ESP_LOGW(TAG, "Session timeout, back to IDLE");
                    state = RadioFSM::IDLE;
                    last_poll = 0;
                    break;
                }

                if ((current_tick - last_poll) >= pdMS_TO_TICKS(polling_timeout_ms)) {
                    pad = generateTestController();
                    xQueueOverwrite(controller_queue_handle, &pad);

                    LoraPayload_t payload = convertControllerData(pad);
                    ESP_LOGI("DECODE", "LX=%u, LY=%u, RX=%u, RY=%u, LT=%u, RT=%u, Mask=0x%02X",
                        payload.st_l_ax_x,
                        payload.st_l_ax_y,
                        payload.st_r_ax_x,
                        payload.st_r_ax_y,
                        payload.trigger_l,
                        payload.trigger_r,
                        payload.reserved_buttons);
                    packetDataTransaction(0x00, 0x01, 0x17, payload, &crypto_ctx, &session_cnt);

                    lora_count.fetch_add(1, std::memory_order_relaxed);
                    session_cnt.cnt++;
                    last_poll = current_tick;

                    ESP_LOGI(TAG, "TX #%lu: LX=%u LY=%u RX=%u RY=%u LT=%u RT=%u MASK=0x%02X",
                             lora_count.load(std::memory_order_relaxed),
                             pad.left_x, pad.left_y, pad.right_x, pad.right_y,
                             pad.l_trigger, pad.r_trigger, pad.btn_mask);
                } else {
                    uint8_t rx_len = receiveEbyteRadioFrame(rx_buffer, 100);
                    if (rx_len > 0 && rx_len == sizeof(HandshakeRx_t)) {
                        HandshakeRx_t resp = *(reinterpret_cast<HandshakeRx_t*>(rx_buffer));
                        if (resp.rx_session_salt == RX_SESSION_SALT) {
                            ESP_LOGW(TAG, "RX salt reset request, back to IDLE");
                            state = RadioFSM::IDLE;
                            last_poll = 0;
                        }
                    }
                }
                break;
            }

            default:
                state = RadioFSM::IDLE;
        }
    }
}


extern "C" void app_main(void) {
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("LORA_TEST", ESP_LOG_VERBOSE);
    esp_log_level_set("LORA_DRV", ESP_LOG_VERBOSE);

    controller_queue_handle = xQueueCreate(1, sizeof(Controller));
    TaskHandle_t drawHandler = NULL;
    TaskHandle_t broadcastHandler = NULL;

    spi_host_device_t spi_port = SPI2_HOST;

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_PORT;
    bus_config.scl_io_num = OLED_SCL;
    bus_config.sda_io_num = OLED_SDA;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handler;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handler));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = OLED_ADDRESS;
    dev_cfg.scl_speed_hz = 400000;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handler, &dev_cfg, &oled_dev));

    // LoRa setup
    gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    spi_bus_config_t spi_cfg = {};
    spi_cfg.mosi_io_num = LORA_MOSI;
    spi_cfg.miso_io_num = LORA_MISO;
    spi_cfg.sclk_io_num = LORA_SCLK;
    spi_cfg.quadwp_io_num = -1;
    spi_cfg.quadhd_io_num = -1;
    spi_cfg.max_transfer_sz = 256;

    ESP_ERROR_CHECK(spi_bus_initialize(spi_port, &spi_cfg, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(lora_init(spi_port));
    lora_setup();


    oled_init();
    oled_clear();
    oled_print(0, 0, "ESP32 LoRa TEST");
    oled_print(1, 0, "Synthetic TX");
    vTaskDelay(pdMS_TO_TICKS(1000));

    xTaskCreatePinnedToCore(draw_controller_screen, "Draw", 4096, NULL, 1, &drawHandler, 1);
    xTaskCreatePinnedToCore(broadcast, "broadcast", 4096, NULL, 1, &broadcastHandler, 0);
    vTaskDelete(NULL);
}