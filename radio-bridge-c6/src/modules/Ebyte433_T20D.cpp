#include "Ebyte433_T20D.hpp"

bool waitForAuxHigh(gpio_num_t auxPin, uint32_t timeout_ms) {
    uint32_t start_tick = esp_timer_get_time() / 1000;

    while (!gpio_get_level(auxPin)) {
        if ((esp_timer_get_time() / 1000) - start_tick >= timeout_ms) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return true;
}

std::optional<EbyteSettings_t> EbyteCfg(const EbyteConfig& cfg, uint32_t timeout_ms) {
    uint8_t read_cmd[3] = {0xC1, 0xC1, 0xC1};
    EbyteSettings_t settings = {};

    int status = uart_write_bytes(cfg.uart_port_num, read_cmd, sizeof(read_cmd));
    if (status == -1) {
        return std::nullopt;
    }

    int length = uart_read_bytes(cfg.uart_port_num,
        reinterpret_cast<uint8_t*>(&settings),
        sizeof(EbyteSettings_t),
        pdMS_TO_TICKS(timeout_ms)
    );

    if (length != sizeof(EbyteSettings_t)) {
        return std::nullopt;
    }

    return settings;
}

bool changeMode(EbyteMode mode, const EbyteConfig& cfg, uint32_t timeout_ms) {
    if (!waitForAuxHigh(cfg.auxPin, timeout_ms)) {
        return false;
    }

    const uint8_t uint_mode = static_cast<uint8_t>(mode);
    gpio_set_level(cfg.m0Pin, (uint_mode & 0x01) ? 1 : 0);
    gpio_set_level(cfg.m1Pin, (uint_mode & 0x02) ? 1 : 0);

    vTaskDelay(pdMS_TO_TICKS(5));

    return waitForAuxHigh(cfg.auxPin, timeout_ms);
}

void sendTelemetry() {}

