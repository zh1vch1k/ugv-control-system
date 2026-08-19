#include "stm32f4xx_hal.h"

// Системный обработчик прерывания SysTick (нужен для работы HAL_Delay)
extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

int main(void) {
    // 1. Инициализация HAL: настраивает Flash prefetch, SysTick и приоритеты
    HAL_Init();

    // 2. Включаем тактирование порта C
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // 3. Конфигурируем встроенный светодиод (PC13)
    GPIO_InitTypeDef gpio_init = {};
    gpio_init.Pin = GPIO_PIN_13;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    // 4. Главный цикл мигалки
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500); // Если HAL_Init и SysTick работают, пауза ровно 500 мс
    }

    return 0;
}
