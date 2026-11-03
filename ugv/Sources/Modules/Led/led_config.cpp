#include "led_config.hpp"

//Board LED
namespace board_led {
	void init() noexcept {
		__HAL_RCC_GPIOB_CLK_ENABLE();
		GPIO_InitTypeDef gpio = {};
		gpio.Pin   = PIN;
	    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
	    gpio.Pull  = GPIO_NOPULL;
	    gpio.Speed = GPIO_SPEED_FREQ_LOW;
	    HAL_GPIO_Init(GPIOB, &gpio);
	}

	void deinit() noexcept {
		GPIO_InitTypeDef gpio = {};
		gpio.Pin   = PIN;
	    gpio.Mode  = GPIO_MODE_ANALOG;
	    gpio.Pull  = GPIO_NOPULL;
	    gpio.Speed = GPIO_SPEED_FREQ_LOW;
	    HAL_GPIO_Init(GPIOB, &gpio);
	}

	void on() noexcept { GPIOB->BSRR = PIN; }
	void off() noexcept { GPIOB->BSRR = static_cast<uint32_t>(PIN) << 16; }
	void toggle() noexcept { HAL_GPIO_TogglePin(GPIOB, PIN); }
}

//RGB_Init
void RGBLed::init() noexcept {
	__HAL_RCC_GPIOE_CLK_ENABLE();
	GPIO_InitTypeDef gpio = {};
	gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Pin   = PINS;

	HAL_GPIO_Init(GPIOE, &gpio);
    GPIOE->BSRR = (PINS) << 16;
}


void RGBLed::deinit() noexcept {
	GPIOE->BSRR = (PINS) << 16;

	GPIO_InitTypeDef gpio = {};
	gpio.Mode  = GPIO_MODE_ANALOG;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	gpio.Pull  = GPIO_NOPULL;
    gpio.Pin   = PINS;
    HAL_GPIO_Init(GPIOE, &gpio);
}


void RGBLed::setColor(RgbColor color) noexcept {
	static constexpr uint32_t PinMask = PINS;
	const uint32_t set_bits    = (static_cast<uint32_t>(color) & 0x07U) << 2;
	const uint32_t reset_bits  = (~set_bits & PinMask) << 16;
    GPIOE->BSRR = set_bits | reset_bits;
}








