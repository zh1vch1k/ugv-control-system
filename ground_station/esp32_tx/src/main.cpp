#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <atomic>
#include <cmath>
#include <stdio.h>
#include <string.h>

#include "fsm.hpp"
#include "packet.hpp"
#include "esp_random.h"
#include "esp_log.h"
#include "drivers/oled.hpp"
#include "drivers/LoRa.hpp"
#include "drivers/uart_controller.hpp"
#include "utils/packet_bridge.hpp"
#include "utils/controller.hpp"

// Флаги синхронизации Event Group
#define BIT_RS485_OK       (1 << 0)
#define BIT_HANDSHAKE_DONE (1 << 1)

static EventGroupHandle_t state_event_group = NULL;
static QueueHandle_t controller_queue_handle = NULL;
static QueueHandle_t ui_msg_queue = NULL;

static std::atomic<uint32_t> lora_count{0};

// Структура сообщений для экрана
struct UiMessage_t {
    uint8_t line;
    char text[32];
};

static void send_ui_status(uint8_t line, const char* str) {
    if (!ui_msg_queue) return;
    UiMessage_t msg;
    msg.line = line;
    snprintf(msg.text, sizeof(msg.text), "%s", str);
    xQueueOverwrite(ui_msg_queue, &msg);
}

void draw_controller_screen(void* pv) {
    Controller pad;
    UiMessage_t status_msg;
    char buffer[32];
    int display_cnt = 0;

    oled_clear();
    oled_print(0, 0, "STATUS: INIT...");
    vTaskDelay(pdMS_TO_TICKS(500));
    oled_clear();

    while (1) {
        if (xQueueReceive(ui_msg_queue, &status_msg, 0) == pdTRUE) {
            oled_print(status_msg.line, 0, status_msg.text);
        }

        EventBits_t bits = xEventGroupGetBits(state_event_group);

        if (!(bits & BIT_RS485_OK)) {
            oled_print(0, 0, "RS-485: NO STICK!");
            oled_print(2, 0, "CHECK CABLE      ");
            oled_print(4, 0, "RADIO: PAUSED    ");
            oled_print(6, 0, "                 ");
        } else if (!(bits & BIT_HANDSHAKE_DONE)) {
            oled_print(0, 0, "RS-485: CONNECTED");
            oled_print(2, 0, "LORA: HANDSHAKE  ");
            oled_print(4, 0, "WAITING SHASSIS  ");
            oled_print(6, 0, "                 ");
        } else {
            if (xQueuePeek(controller_queue_handle, &pad, 0) == pdTRUE) {
                snprintf(buffer, sizeof(buffer), "TX:%-5lu | #%-4d",
                         lora_count.load(std::memory_order_relaxed), ++display_cnt);
                oled_print(0, 0, buffer);

                snprintf(buffer, sizeof(buffer), "LX:%-3u | LY:%-3u", pad.left_x, pad.left_y);
                oled_print(2, 0, buffer);

                snprintf(buffer, sizeof(buffer), "RX:%-3u | RY:%-3u", pad.right_x, pad.right_y);
                oled_print(3, 0, buffer);

                snprintf(buffer, sizeof(buffer), "LT:%-2u  | RT:%-2u", pad.l_trigger, pad.r_trigger);
                oled_print(4, 0, buffer);

                snprintf(buffer, sizeof(buffer), "BTN MASK: 0x%02X", pad.btn_mask);
                oled_print(6, 0, buffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(60));
    }
}

void uart_rx_task(void* pv) {
    Controller local_pad;
    uint32_t last_valid_rx_tick = 0;
    uint8_t stable_count = 0;

    while (1) {
        if (readUART(local_pad)) {
            last_valid_rx_tick = xTaskGetTickCount();
            xQueueOverwrite(controller_queue_handle, &local_pad);

            if (stable_count < 5) {
                stable_count++;
                if (stable_count >= 5) {
                    xEventGroupSetBits(state_event_group, BIT_RS485_OK);
                    send_ui_status(0, "RS-485: ONLINE   ");
                }
            }
        }

        if ((xTaskGetTickCount() - last_valid_rx_tick) > pdMS_TO_TICKS(350)) {
            if (xEventGroupGetBits(state_event_group) & BIT_RS485_OK) {
                stable_count = 0;
                xEventGroupClearBits(state_event_group, BIT_RS485_OK);
                send_ui_status(0, "RS-485: TIMEOUT  ");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void broadcast(void* pv) {
    static const char *TAG = "LORA_TEST";

    uint8_t rx_buffer[64];
    Controller pad;

    SpeckContext_t crypto_ctx = {};
    speck_init(&crypto_ctx, RFBP_SECRET_KEY);

    while (1) {
        // БАРЬЕР 1: Ждем устойчивый поток стиков по RS-485
        ESP_LOGI(TAG, "Waiting for RS-485 link to stabilize...");
        send_ui_status(2, "WAIT FOR STICK...");
        xEventGroupWaitBits(state_event_group, BIT_RS485_OK,
                            pdFALSE, pdTRUE, portMAX_DELAY);

        // БАРЬЕР 2: Подготовка и прохождение Handshake
        uint32_t session_id = esp_random();
        SpeckSessionCounter_t session_cnt = {session_id, 0};
        bool handshake_ok = false;

        while (!handshake_ok) {
            // Если джойстик отключили во время согласования
            if (!(xEventGroupGetBits(state_event_group) & BIT_RS485_OK)) {
                break;
            }

            ESP_LOGI(TAG, "Sending Handshake (id: 0x%08lX)...", (unsigned long)session_id);
            packetDataTransaction(0x00, 0x01, 0x17, TX_SESSION_SALT, &crypto_ctx, &session_cnt);

            uint8_t rx_len = receiveEbyteRadioFrame(rx_buffer, 300);
            if (rx_len > 0) {
                const uint8_t* payload_ptr = (rx_len > sizeof(HandshakeRx_t)) ? (rx_buffer + 4) : rx_buffer;
                const HandshakeRx_t* resp = reinterpret_cast<const HandshakeRx_t*>(payload_ptr);

                if (resp->rx_session_salt == TX_SESSION_SALT && resp->session_id == session_id) {
                    session_cnt.session_id = session_id;
                    session_cnt.cnt = 0;

                    handshake_ok = true;
                    xEventGroupSetBits(state_event_group, BIT_HANDSHAKE_DONE);
                    send_ui_status(2, "LINK: ACTIVE     ");
                    ESP_LOGI(TAG, "Handshake successful! Switched to ACTIVE stream");
                    break;
                } else {
                    ESP_LOGW(TAG, "Salt/ID mismatch: got salt=0x%08lX id=0x%08lX",
                             (unsigned long)resp->rx_session_salt, (unsigned long)resp->session_id);
                }
            } else {
                ESP_LOGW(TAG, "Handshake ACK timeout, retrying...");
            }

            vTaskDelay(pdMS_TO_TICKS(150));
        }

        if (!handshake_ok) {
            continue;
        }

        // БАРЬЕР 3: Активный цикл передачи (согласован со скоростью LoRa 2.4kbps)
        while (1) {
            // Failsafe: проверка физического подключения стиков
            if (!(xEventGroupGetBits(state_event_group) & BIT_RS485_OK)) {
                ESP_LOGW(TAG, "RS-485 lost during stream! Sending stop frame and aborting radio stream.");
                // Немедленно шлем нейтральный стоп-кадр в эфир перед разрывом
                Controller stop_pad = {0xAA, 64, 64, 64, 64, 0, 0, 0};
                LoraPayload_t stop_payload = convertControllerData(stop_pad);
                packetDataTransaction(0x00, 0x01, 0x17, stop_payload, &crypto_ctx, &session_cnt);
                xEventGroupClearBits(state_event_group, BIT_HANDSHAKE_DONE);
                break;
            }

            if (xQueuePeek(controller_queue_handle, &pad, 0) == pdTRUE) {
                LoraPayload_t payload = convertControllerData(pad);
                packetDataTransaction(0x00, 0x01, 0x17, payload, &crypto_ctx, &session_cnt);
                lora_count.fetch_add(1, std::memory_order_relaxed);
            }

            // Гарантированная пауза между кадрами для передачи кванта задаче IDLE1 и сброса Watchdog
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}

extern "C" void app_main(void) {
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("LORA_TEST", ESP_LOG_VERBOSE);
    esp_log_level_set("LORA_DRV", ESP_LOG_VERBOSE);

    controller_queue_handle = xQueueCreate(1, sizeof(Controller));
    ui_msg_queue            = xQueueCreate(1, sizeof(UiMessage_t));
    state_event_group       = xEventGroupCreate();

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

    initUart();

    oled_init();
    oled_clear();
    oled_print(0, 0, "ESP32 LoRa TEST");
    oled_print(1, 0, "SYSTEM BOOT OK");
    vTaskDelay(pdMS_TO_TICKS(1000));

    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx", 3072, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(broadcast, "lora_broadcast", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(draw_controller_screen, "oled_draw", 4096, NULL, 1, NULL, 0);

    vTaskDelete(NULL);
}