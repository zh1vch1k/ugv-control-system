#include "fsm_resolver.hpp"

static const char* TAG_HS = "FSM_RES";

std::optional<uint32_t> readHandshake(const EbyteConfig& cfg, uint32_t salt, uint32_t timeout_ms) {
    HandshakeRx_t handshake = {};

    int length = uart_read_bytes(
            cfg.uart_port_num,
            reinterpret_cast<uint8_t*>(&handshake),
            sizeof(HandshakeRx_t),
            pdMS_TO_TICKS(timeout_ms)
    );

    if (length <= 0) {
        return std::nullopt;
    }

    if (length != sizeof(HandshakeRx_t)) {
        ESP_LOGW(TAG_HS, "Length mismatch in Handshake! Expected %d, got %d", (int)sizeof(HandshakeRx_t), length);
        uart_flush_input(cfg.uart_port_num);
        return std::nullopt;
    }

    if (handshake.rx_session_salt == salt) {
        ESP_LOGI(TAG_HS, "Handshake validated! session_id=0x%08lX", (unsigned long)handshake.session_id);
        return handshake.session_id;
    }

    ESP_LOGW(TAG_HS, "Salt mismatch! Got 0x%08lX != Expected 0x%08lX", 
             (unsigned long)handshake.rx_session_salt, (unsigned long)salt);
    uart_flush_input(cfg.uart_port_num);
    return std::nullopt;
}

bool sendHandshake(const EbyteConfig& cfg, uint8_t addh, uint8_t addl, uint8_t channel,
                   uint32_t session_id, uint32_t salt, uint32_t timeout_ms) {
    HandshakeTx_t response = {};
    response.addh = addh;
    response.addl = addl;
    response.channel = channel;
    response.tx_session_salt = salt;
    response.session_id = session_id;

    const uint8_t* innerBytes = reinterpret_cast<const uint8_t*>(&response) + 3;
    const size_t innerLen = sizeof(HandshakeTx_t) - 3;

    uint8_t tx_buffer[sizeof(HandshakeTx_t)];
    std::memcpy(tx_buffer, &response, 3);

    const uint8_t xorKey = 0xCA - channel;
    for (size_t i = 0; i < innerLen; ++i) {
        tx_buffer[3 + i] = innerBytes[i] ^ xorKey;
    }

    if (!waitForAuxHigh(cfg.auxPin, timeout_ms)) {
        ESP_LOGW(TAG_HS, "Ebyte AUX not ready before TX");
        return false;
    }

    int written = uart_write_bytes(cfg.uart_port_num, tx_buffer, sizeof(tx_buffer));
    if (written != sizeof(tx_buffer)) {
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
    return waitForAuxHigh(cfg.auxPin, timeout_ms);
}

bool syncWithSTM32(uart_port_t uart_port, uint32_t session_id, uint32_t timeout_ms) {
    uart_flush_input(uart_port);

    uint8_t sync_frame[8] = {0};
    std::memcpy(&sync_frame[0], &session_id, sizeof(uint32_t));
    sync_frame[4] = STM32_ACK_CMD; // 0xA5
    sync_frame[5] = 0x00;
    sync_frame[6] = 0x00;
    sync_frame[7] = 0x00;

    int written = uart_write_bytes(uart_port, sync_frame, sizeof(sync_frame));
    if (written != sizeof(sync_frame)) {
        return false;
    }

    uint32_t stm_resp = 0;
    int len = uart_read_bytes(uart_port, reinterpret_cast<uint8_t*>(&stm_resp), sizeof(uint32_t), pdMS_TO_TICKS(timeout_ms));
    if (len == sizeof(uint32_t) && stm_resp == STM32_ACK_MAGIC) {
        ESP_LOGI(TAG_HS, "STM32 Handshake ACK verified (0x%08lX)", (unsigned long)stm_resp);
        return true;
    }

    ESP_LOGW(TAG_HS, "STM32 sync failed: len=%d resp=0x%08lX (expected 0x%08lX)", len, (unsigned long)stm_resp, (unsigned long)STM32_ACK_MAGIC);
    return false;
}


