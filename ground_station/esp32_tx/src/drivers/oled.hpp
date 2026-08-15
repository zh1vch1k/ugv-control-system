#pragma once
#include <cstdint>
#include "driver/i2c_master.h"
#include "utils/font6_8.hpp"

#define I2C_PORT            I2C_NUM_0
#define OLED_SDA            GPIO_NUM_21
#define OLED_SCL            GPIO_NUM_22
#define PIN_RESET           GPIO_NUM_16
#define OLED_ADDRESS        0x3C

extern i2c_master_dev_handle_t oled_dev;

void oled_send_cmd(uint8_t cmd);

void oled_send_data(const uint8_t *data, size_t len);

void oled_init(void);

void oled_clear(void);

void oled_print(uint8_t line, uint8_t col, const char *str);

int checkBtn(uint8_t data, uint8_t bit);