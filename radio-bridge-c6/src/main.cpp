#include <stdio.h>
#include <atomic>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "modules/led.h"
#include "modules/fsm_resolver.hpp"
#include "modules/Ebyte433_T20D.hpp"
#include "packet.hpp"

static const char *TAG = "RADIO_BRIDGE";

// =================== ПИНЫ СВЯЗИ СО STM32 ===================
#define STM32_TX_PIN   GPIO_NUM_21  // ESP32 TX -> STM32 RX (PA3)
#define STM32_RX_PIN   GPIO_NUM_20  // ESP32 RX <- STM32 TX (PA2)

// =================== ПИНЫ МОДУЛЯ EBYTE E32 ===================
#define M0_PIN         GPIO_NUM_4
#define M1_PIN         GPIO_NUM_5
#define LORA_TX_PIN    6            // ESP32 TX -> Ebyte RX
#define LORA_RX_PIN    7            // ESP32 RX <- Ebyte TX
#define AUX_PIN        GPIO_NUM_14

#define UART_BUF_SIZE  1024

const uart_port_t stm32_link_port = UART_NUM_0;
const uart_port_t lora_port       = UART_NUM_1;

static std::atomic<RadioFSM> state = RadioFSM::IDLE;

void init_stm32_link_uart(void) {
    if (uart_is_driver_installed(stm32_link_port)) {
        uart_driver_delete(stm32_link_port);
    }

    uart_config_t config = {};
    config.baud_rate  = 115200;
    config.data_bits  = UART_DATA_8_BITS;
    config.parity     = UART_PARITY_DISABLE;
    config.stop_bits  = UART_STOP_BITS_1;
    config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(stm32_link_port, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(stm32_link_port, &config));
    ESP_ERROR_CHECK(uart_set_pin(stm32_link_port, STM32_TX_PIN, STM32_RX_PIN, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "STM32 UART0 готов: TX=GPIO%d, RX=GPIO%d (115200 8N1)", STM32_TX_PIN, STM32_RX_PIN);
}

void init_lora_uart(void) {
    if (uart_is_driver_installed(lora_port)) {
        uart_driver_delete(lora_port);
    }

    uart_config_t config = {};
    config.baud_rate  = 9600;
    config.data_bits  = UART_DATA_8_BITS;
    config.parity     = UART_PARITY_DISABLE;
    config.stop_bits  = UART_STOP_BITS_1;
    config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(lora_port, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(lora_port, &config));
    ESP_ERROR_CHECK(uart_set_pin(lora_port, LORA_TX_PIN, LORA_RX_PIN, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "LoRa UART1 готов: TX=GPIO%d, RX=GPIO%d (9600 8N1)", LORA_TX_PIN, LORA_RX_PIN);
}

void init_aux_pin() {
    gpio_reset_pin(AUX_PIN);
    gpio_set_direction(AUX_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(AUX_PIN, GPIO_PULLUP_ONLY);
}

void led_control_task(void* pvParameters) { 
    while (1) {
        switch (state.load()) {
            case RadioFSM::IDLE:
                led_set_status(LedStatus::Idle); // Синий: ждем хэндшейк
                break;
            case RadioFSM::ACTIVE:
                led_set_status(LedStatus::SessionValid); // Мятный/зеленый: связь активна
                break;
            case RadioFSM::ERROR: 
                led_set_status(LedStatus::Error); // Красный: ошибка
                break;
            default:
                led_set_status(LedStatus::Heartbeat);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void bridge_task(void* pvParameters) {
    EbyteConfig* ebyte_cfg = reinterpret_cast<EbyteConfig*>(pvParameters);

    uint32_t current_session_id = 0;
    uint32_t last_packet_time = 0;

    while (1) {
        RadioFSM current_state = state.load();

        switch (current_state) {
            case RadioFSM::IDLE: {
                // Ожидаем хэндшейк от Ground Station (LilyGO LoRa32) через модуль Ebyte (UART1)
                auto maybe_session = readHandshake(*ebyte_cfg, TX_SESSION_SALT, 100);
                if (maybe_session.has_value()) {
                    current_session_id = maybe_session.value();
                    ESP_LOGI(TAG, "Получен Handshake от GS! session_id=0x%08lX", (unsigned long)current_session_id);
                    led_set_status(LedStatus::HandshakePending);

                    // Синхронизируем STM32H562 по UART0
                    bool stm_synced = syncWithSTM32(stm32_link_port, current_session_id, 200);
                    if (!stm_synced) {
                        ESP_LOGW(TAG, "STM32 не ответил на первый Handshake, повторяем попытку...");
                        vTaskDelay(pdMS_TO_TICKS(50));
                        stm_synced = syncWithSTM32(stm32_link_port, current_session_id, 200);
                    }

                    if (stm_synced) {
                        ESP_LOGI(TAG, "STM32 синхронизирован! Отправляем ACK на Ground Station...");
                        // Отправляем подтверждение Handshake на базовую станцию (ADDH=0, ADDL=0, CHAN=0x17)
                        sendHandshake(*ebyte_cfg, 0x00, 0x00, 0x17, current_session_id, TX_SESSION_SALT, 200);

                        last_packet_time = esp_timer_get_time() / 1000;
                        state.store(RadioFSM::ACTIVE);
                        ESP_LOGI(TAG, "Радиомост переведен в режим ACTIVE!");
                    } else {
                        ESP_LOGE(TAG, "Не удалось синхронизировать STM32. Возврат в IDLE.");
                        led_set_status(LedStatus::Error);
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
                break;
            }

            case RadioFSM::ACTIVE: {
                // Прием активных пакетов управления от Ebyte (UART1)
                uint8_t rx_buffer[sizeof(LoraRxFrame_t)] = {0};
                int bytes_read = uart_read_bytes(ebyte_cfg->uart_port_num, rx_buffer, sizeof(LoraRxFrame_t), pdMS_TO_TICKS(150));

                if (bytes_read == sizeof(LoraRxFrame_t)) {
                    // 1. Проверяем, не прислала ли базовая станция новый хэндшейк (перезагрузка GS)
                    const HandshakeRx_t* hs = reinterpret_cast<const HandshakeRx_t*>(rx_buffer);
                    if (hs->rx_session_salt == TX_SESSION_SALT) {
                        current_session_id = hs->session_id;
                        ESP_LOGI(TAG, "Получен повторный Handshake (GS reboot? id=0x%08lX)", (unsigned long)current_session_id);
                        syncWithSTM32(stm32_link_port, current_session_id, 150);
                        sendHandshake(*ebyte_cfg, 0x00, 0x00, 0x17, current_session_id, TX_SESSION_SALT, 200);
                        last_packet_time = esp_timer_get_time() / 1000;
                        break;
                    }

                    // 2. Валидация пакета управления перед пересылкой в STM32
                    const LoraRxFrame_t* frame = reinterpret_cast<const LoraRxFrame_t*>(rx_buffer);
                    if (frame->payload_len == sizeof(LoraPayload_t)) {
                        const uint8_t* raw_payload = reinterpret_cast<const uint8_t*>(&(frame->payload));
                        uint8_t crc = CRC8_calc(raw_payload, frame->payload_len);

                        if (crc == frame->crc8) {
                            // Пакет абсолютно валиден! Пересылаем его напрямую в STM32H562
                            uart_write_bytes(stm32_link_port, rx_buffer, sizeof(LoraRxFrame_t));
                            last_packet_time = esp_timer_get_time() / 1000;
                            led_set_status(LedStatus::LoraRx); // Короткая индикация приема
                        } else {
                            ESP_LOGW(TAG, "Битая контрольная сумма радиоканала: CRC exp=0x%02X got=0x%02X", frame->crc8, crc);
                        }
                    } else {
                        ESP_LOGW(TAG, "Некорректная длина payload %u, очистка буфера", frame->payload_len);
                        uart_flush_input(ebyte_cfg->uart_port_num);
                    }
                } else if (bytes_read > 0) {
                    // Обрывок или шум в радиоэфире — очищаем входной буфер
                    ESP_LOGW(TAG, "Фрагмент пакета (%d байт), сброс буфера Ebyte", bytes_read);
                    uart_flush_input(ebyte_cfg->uart_port_num);
                }

                // Сторожевой таймер связи (2.5 секунды без пакетов)
                uint32_t now = esp_timer_get_time() / 1000;
                if ((now - last_packet_time) > 2500) {
                    ESP_LOGW(TAG, "Таймаут радиоканала (>2500 мс). Возврат в IDLE");
                    state.store(RadioFSM::IDLE);
                }
                break;
            }

            case RadioFSM::ERROR:
            default: {
                vTaskDelay(pdMS_TO_TICKS(500));
                state.store(RadioFSM::IDLE);
                break;
            }
        }
    }
}

extern "C" void app_main(void) {
    if (led_init() != ESP_OK) {
        ESP_LOGE(TAG, "LED Init Error");
    }

    init_stm32_link_uart();
    init_lora_uart();
    init_aux_pin();

    static EbyteConfig ebyte_cfg = {};
    ebyte_cfg.uart_port_num = lora_port;
    ebyte_cfg.auxPin        = AUX_PIN;
    ebyte_cfg.m0Pin         = M0_PIN;
    ebyte_cfg.m1Pin         = M1_PIN;

    gpio_reset_pin(M0_PIN);
    gpio_set_direction(M0_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(M1_PIN);
    gpio_set_direction(M1_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "Конфигурация Ebyte модуля через M0/M1...");

    changeMode(EbyteMode::Config, ebyte_cfg);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t set_config_cmd[6] = {
       0xC0, // Запись в EEPROM с сохранением
       0x00, // ADDH
       0x01, // ADDL
       0x1A, // SPED: 9600 bps UART, 2.4 kbps Air
       0x17, // CHAN: 433 MHz (канал 23)
       0xC3  // OPTION: Fixed Transmission Mode, 10dBm
    };

    uart_write_bytes(ebyte_cfg.uart_port_num, set_config_cmd, sizeof(set_config_cmd));
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t buffer[6];
    int resp_len = uart_read_bytes(ebyte_cfg.uart_port_num, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
    if (resp_len == 6) {
        ESP_LOGI(TAG, "Ebyte ответ на конфиг: %02X %02X %02X %02X %02X %02X",
                 buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    } else {
        ESP_LOGW(TAG, "Ebyte не вернул подтверждение конфига (байт: %d)", resp_len);
    }

    // Возвращаем в Normal Mode
    changeMode(EbyteMode::Normal, ebyte_cfg);
    vTaskDelay(pdMS_TO_TICKS(50));

    waitForAuxHigh(ebyte_cfg.auxPin);
    vTaskDelay(pdMS_TO_TICKS(50));

    uart_flush_input(ebyte_cfg.uart_port_num);
    uart_flush_input(stm32_link_port);
    ESP_LOGI(TAG, "Ebyte запущен в Normal Mode. Запуск FSM задач...");

    xTaskCreate(&led_control_task, "LED_TASK", 2048, nullptr, 2, nullptr);
    xTaskCreate(&bridge_task, "BRIDGE_TASK", 4096, &ebyte_cfg, 4, nullptr);
}

