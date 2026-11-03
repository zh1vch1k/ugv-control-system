#pragma once
#include "stm32h5xx_hal.h"

/*
5. СВЕТОВАЯ ИНДИКАЦИЯ:
----------------------------------------------------------------------------------------
| Пин МК | Периферия       | Назначение                | Индикация                      |
|--------|-----------------|---------------------------|--------------------------------|
| PB2    | GPIO Output     | Heartbeat LED (Плата)     | Мигает при каждом пакете       |
| PE2    | GPIO Output     | RGB LED: RED              | Режим ошибки / Failsafe        |
| PE3    | GPIO Output     | RGB LED: GREEN            | Связь активна (ACTIVE)         |
| PE4    | GPIO Output     | RGB LED: BLUE             | Ожидание связи (IDLE)          |
========================================================================================
*/

//Board LED config
namespace board_led {

	static constexpr uint32_t PIN = GPIO_PIN_2;

	void init() noexcept;

	void deinit() noexcept;

	void on() noexcept;

	void off() noexcept;

	void toggle() noexcept;
}


//External RGB config
enum class RgbColor : uint8_t {
    Off     = 0b000,
    Red     = 0b001,
    Green   = 0b010,
    Blue    = 0b100,
    Yellow  = Red | Green,
    Cyan    = Green | Blue,
    Magenta = Red | Blue,
    White   = Red | Green | Blue
};


class RGBLed {
public:
	static constexpr uint32_t PINS = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;

	static void init() noexcept;

	static void deinit() noexcept;

	static void setColor(RgbColor color) noexcept;
};
