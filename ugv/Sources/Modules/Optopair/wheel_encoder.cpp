#include "wheel_encoder.hpp"

WheelEncoder::WheelEncoder(const Config& config,
                           uint16_t ticks_per_rev,
                           uint32_t debounce_ms) noexcept
    : config_(config),
      ticks_per_rev_(ticks_per_rev),
      debounce_ms_(debounce_ms) {}

void WheelEncoder::init() noexcept {
    if (config_.port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (config_.port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (config_.port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (config_.port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (config_.port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin   = config_.pin;
    gpio.Mode  = GPIO_MODE_IT_RISING;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(config_.port, &gpio);

    HAL_NVIC_SetPriority(config_.irq, config_.preemption_priority, config_.sub_priority);
    HAL_NVIC_EnableIRQ(config_.irq);

    reset();
}

void WheelEncoder::deinit() noexcept {
    HAL_NVIC_DisableIRQ(config_.irq);

    GPIO_InitTypeDef gpio = {};
    gpio.Pin  = config_.pin;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(config_.port, &gpio);

    reset();
}

bool WheelEncoder::handle_exti(uint16_t triggered_pin, uint32_t now_ms) noexcept {
    if (triggered_pin != config_.pin) {
        return false;
    }

    if (now_ms - last_pulse_ms_ < debounce_ms_) {
        return true;
    }
    last_pulse_ms_ = now_ms;

    uint16_t current = ticks_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (current >= ticks_per_rev_) {
        ticks_.store(0, std::memory_order_relaxed);
        rev_completed_.store(true, std::memory_order_release);
    }

    return true;
}

bool WheelEncoder::check_and_clear_rev() noexcept {
    return rev_completed_.exchange(false, std::memory_order_acq_rel);
}

uint16_t WheelEncoder::get_ticks() const noexcept {
    return ticks_.load(std::memory_order_relaxed);
}

void WheelEncoder::reset() noexcept {
    ticks_.store(0, std::memory_order_relaxed);
    rev_completed_.store(false, std::memory_order_relaxed);
    last_pulse_ms_ = 0;
}
