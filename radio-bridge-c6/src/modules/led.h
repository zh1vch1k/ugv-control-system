#pragma once

#include <stdint.h>
#include "esp_err.h"

enum class LedStatus : uint8_t {
    Error = 0,         // Red
    Success,           // Green
    Idle,              // Blue
    Warning,           // Yellow
    LoraRx,            // Cyan
    LoraTx,            // Magenta
    HandshakePending,  // Orange
    SessionValid,      // Mint
    CryptoInit,        // Deep Violet
    Heartbeat,         // Pink
    SpeedLimit,        // Lime
    MeshForward,       // Sky Blue
    DebugTrace,        // Lavender
    LowBattery,        // Warm Amber
    Ready              // Soft White
};

esp_err_t led_init(void);
esp_err_t led_set_status(LedStatus status);
esp_err_t led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
esp_err_t led_clear(void);
