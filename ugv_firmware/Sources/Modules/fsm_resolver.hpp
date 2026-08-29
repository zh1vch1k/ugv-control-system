#pragma once

#include "fsm.hpp"
#include "key.hpp"
#include "speck.hpp"
#include <cstring>

#include "Modules/Ebyte433_T20D.hpp"

std::optional<uint32_t> readHandshake(const EbyteConfig& cfg, uint32_t salt, uint32_t timeout_ms = 100);

std::optional<HandshakeTx_t> sendHandshake(const EbyteConfig& cfg, uint8_t addh, uint8_t addl, uint8_t channel,
		   	   	   uint32_t session_id, uint32_t salt, uint32_t timeout_ms);


std::optional<HandshakeRx_t> sendHandshakeReply(const EbyteConfig& cfg, uint32_t session_id, uint32_t salt, uint32_t timeout_ms);
