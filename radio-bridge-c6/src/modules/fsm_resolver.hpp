#pragma once

#include "fsm.hpp"
#include "key.hpp"
#include "speck.hpp"
#include <cstring>
#include <optional>
#include "esp_timer.h"
#include "esp_log.h"
#include "modules/Ebyte433_T20D.hpp"

inline constexpr uint32_t STM32_ACK_MAGIC = 0xA4A3A2A1;
inline constexpr uint8_t  STM32_ACK_CMD   = 0xA5;

std::optional<uint32_t> readHandshake(const EbyteConfig& cfg, uint32_t salt, uint32_t timeout_ms = 100);

bool sendHandshake(const EbyteConfig& cfg, uint8_t addh, uint8_t addl, uint8_t channel,
                   uint32_t session_id, uint32_t salt, uint32_t timeout_ms = 200);

bool syncWithSTM32(uart_port_t uart_port, uint32_t session_id, uint32_t timeout_ms = 150);