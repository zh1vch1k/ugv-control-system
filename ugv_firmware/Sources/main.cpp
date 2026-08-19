#include "stm32f4xx_hal.h"

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

int main(void) {
    HAL_Init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpioCfg = {};
    gpioCfg.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpioCfg.Mode = GPIO_MODE_AF_PP;
    gpioCfg.Pull = GPIO_PULLUP;
    gpioCfg.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioCfg.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpioCfg);

    GPIO_InitTypeDef auxCfg = {};
    auxCfg.Pin = GPIO_PIN_2;
    auxCfg.Mode = GPIO_MODE_INPUT;
    auxCfg.Pull = GPIO_NOPULL;
    auxCfg.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &auxCfg);


    __HAL_RCC_USART2_CLK_ENABLE();

    UART_HandleTypeDef uartCfg = {};
    uartCfg.Instance = USART2;
    uartCfg.Init.BaudRate = 9600;
    uartCfg.Init.WordLength = UART_WORDLENGTH_8B;
    uartCfg.Init.StopBits = UART_STOPBITS_1;
    uartCfg.Init.Parity = UART_PARITY_NONE;
    uartCfg.Init.Mode = UART_MODE_TX_RX;
    uartCfg.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uartCfg.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&uartCfg);

    while(1) {

    }

    return 0;
}
