#pragma once
#include <cstdint>

struct __attribute__((packed)) Gamepad {
    uint8_t start_byte;
    uint8_t left_x;
    uint8_t left_y;
    uint8_t right_x;
    uint8_t right_y;
    uint8_t l_trigger;
    uint8_t r_trigger;
    uint8_t btn_mask;
};

