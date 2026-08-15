#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "font6_8.hpp"
#include "Gamepad.hpp"

#define I2C_PORT            I2C_NUM_0
#define OLED_SDA            GPIO_NUM_21
#define OLED_SCL            GPIO_NUM_22
#define PIN_RESET           GPIO_NUM_16
#define OLED_ADDRESS        0x3C

#define UART_PORT           UART_NUM_0  
#define UART_BAUD_RATE      115200          
#define RX_BUF_SIZE         1024

static i2c_master_dev_handle_t oled_dev = NULL;

static void oled_send_cmd(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    i2c_master_transmit(oled_dev, buffer, sizeof(buffer), 100);
}

static void oled_send_data(const uint8_t *data, size_t len) {
    uint8_t buffer[len + 1];
    buffer[0] = 0x40;
    memcpy(&buffer[1], data, len);
    i2c_master_transmit(oled_dev, buffer, sizeof(buffer), 100);
}

void oled_init(void) {
    static const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, // Page Addressing Mode
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
        0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        oled_send_cmd(init_cmds[i]);
    }
}

void oled_clear(void) {
    uint8_t empty_page[128] = {0};
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page);
        oled_send_cmd(0x00);
        oled_send_cmd(0x10);
        oled_send_data(empty_page, sizeof(empty_page));
    }
}

void oled_print(uint8_t line, uint8_t col, const char *str) {
    if (line > 7 || col > 15) return;

    oled_send_cmd(0xB0 + line);
    oled_send_cmd(0x00 | ((col * 8) & 0x0F));
    oled_send_cmd(0x10 | (((col * 8) >> 4) & 0x0F));

    while (*str) {
        uint8_t ascii = (uint8_t)*str;
        if (ascii < 128) {
            oled_send_data(font::font6x8[ascii], 6);
        }
        str++;
    }
}

bool readUART(Gamepad &pad) {
    int len = uart_read_bytes(UART_NUM_0, (uint8_t*)&pad, sizeof(Gamepad), portMAX_DELAY);

    if (len == sizeof(Gamepad)) {
        if (pad.start_byte == 0xAA) {
            return true;
        }
    }   
    return false;
}

int checkBtn(uint8_t data, uint8_t bit) {
    return (data >> bit) & 1;
}

void draw_gamepad_screen(void* pv) {
    char buffer[64];
    int cnt = 0;
    Gamepad pad;

    oled_clear();
    oled_print(0, 0, "STATUS: UART WAIT");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();
    while(1) {
        if (readUART(pad)) {
            uint8_t btnMask = pad.btn_mask;

            snprintf(buffer, sizeof(buffer), "PACKET:%-4d" ,++cnt);
            oled_print(0, 0, buffer);
            oled_print(1, 0, "Sticks & Triggers");
            snprintf(buffer, sizeof(buffer), "LX:%-3u | LY:%-3u", pad.left_x, pad.left_y);
            oled_print(2, 4, buffer);

            snprintf(buffer, sizeof(buffer), "RX:%-3u | RY:%-3u", pad.right_x, pad.right_y);
            oled_print(3, 4, buffer);

            snprintf(buffer, sizeof(buffer), "LT: %d | RT:%-3u", pad.l_trigger, pad.r_trigger);
            oled_print(4, 4, buffer);

            oled_print(5, 0, "Buttons");
            snprintf(buffer, sizeof(buffer), "L3:%d | R3:%d", checkBtn(btnMask, 0), checkBtn(btnMask, 1));
            oled_print(6, 4, buffer);

            snprintf(buffer, sizeof(buffer), "L1:%d | R1:%d", checkBtn(btnMask, 2), checkBtn(btnMask, 3));
            oled_print(7, 4, buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
}

void initUart(void) {
    uart_config_t config = {};
    config.baud_rate = UART_BAUD_RATE;
    config.data_bits = UART_DATA_8_BITS;
    config.parity    = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));

    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT,
        RX_BUF_SIZE,
        0, 
        0, 
        NULL, 
        0
    ));
}

TaskHandle_t drawHandler;

extern "C" void app_main(void) {
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_PORT;
    bus_config.scl_io_num = OLED_SCL;
    bus_config.sda_io_num = OLED_SDA;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.flags.enable_internal_pullup = true;

    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    i2c_master_bus_handle_t bus_handler;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handler));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = OLED_ADDRESS;
    dev_cfg.scl_speed_hz = 400000;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handler, &dev_cfg, &oled_dev));

    initUart();
    
    oled_init();
    oled_clear();

    oled_print(0, 0, "ESP32 LoRa");
    oled_print(1, 0, "Status: OK");
    vTaskDelay(pdMS_TO_TICKS(1000));
    // int counter = 0;
    // char buffer[32];
    gpio_set_level(GPIO_NUM_25, 1);
    xTaskCreatePinnedToCore(draw_gamepad_screen, "Draw", 4096, NULL, 1, &drawHandler, 1);
    // while (1) {
        
    //     snprintf(buffer, sizeof(buffer), "Count: %d", counter++);
    //     oled_print(2, 0, buffer);
        
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
    vTaskDelete(NULL);
}