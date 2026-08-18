#include "oled.hpp"
#include "utils/font6_8.hpp"
#include <stdio.h>
#include <string.h>

i2c_master_dev_handle_t oled_dev = NULL;

void oled_send_cmd(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    i2c_master_transmit(oled_dev, buffer, sizeof(buffer), 100);
}

void oled_send_data(const uint8_t *data, size_t len) {
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

int checkBtn(uint8_t data, uint8_t bit) {
    return (data >> bit) & 1;
}