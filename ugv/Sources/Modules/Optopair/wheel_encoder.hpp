#pragma once

#include <atomic>
#include <cstdint>
#include "stm32h5xx_hal.h"

class WheelEncoder {
public:
    struct Config {
        GPIO_TypeDef* port;
        uint16_t pin;
        IRQn_Type irq;
        uint8_t preemption_priority{1};
        uint8_t sub_priority{0};
    };

    explicit WheelEncoder(const Config& config,
                         uint16_t ticks_per_rev = 2,
                         uint32_t debounce_ms = 10) noexcept;

    WheelEncoder(const WheelEncoder&) = delete;
    WheelEncoder& operator=(const WheelEncoder&) = delete;


    void init() noexcept;


    void deinit() noexcept;


    bool handle_exti(uint16_t triggered_pin, uint32_t now_ms) noexcept;

    [[nodiscard]] bool check_and_clear_rev() noexcept;

    [[nodiscard]] uint16_t get_ticks() const noexcept;

    void reset() noexcept;

private:
    const Config config_;
    const uint16_t ticks_per_rev_;
    const uint32_t debounce_ms_;

    uint32_t last_pulse_ms_{0};
    std::atomic<uint16_t> ticks_{0};
    std::atomic<bool> rev_completed_{false};
};
