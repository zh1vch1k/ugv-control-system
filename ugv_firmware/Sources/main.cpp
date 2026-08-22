#include "stm32f4xx_hal.h"
#include "Modules/Ebyte433_T20D.hpp"

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

int main(void) {
    HAL_Init();

    uint32_t resetCauseRaw = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
    uint8_t resetCauseByte = static_cast<uint8_t>(resetCauseRaw >> 24);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpioCfg = {};
    gpioCfg.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpioCfg.Mode = GPIO_MODE_AF_PP;
    gpioCfg.Pull = GPIO_PULLUP;
    gpioCfg.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioCfg.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpioCfg);

    GPIO_InitTypeDef auxCfg = {};
    auxCfg.Pin = GPIO_PIN_10;
    auxCfg.Mode = GPIO_MODE_INPUT;
    auxCfg.Pull = GPIO_PULLUP;
    auxCfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &auxCfg);

    GPIO_InitTypeDef m0m1_cfg = {};
    m0m1_cfg.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    m0m1_cfg.Mode = GPIO_MODE_OUTPUT_PP;
    m0m1_cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    m0m1_cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &m0m1_cfg);

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

    // === ДИАГНОСТИКА: шлём маркер + причину сброса ПЕРВЫМ делом,
    // ещё до того, как трогаем модуль E32 (M0/M1/AUX) ===
    // 0xEE — маркер "начало загрузки", такого байта больше нигде не бывает.
    // Второй байт — сама причина сброса (расшифровка ниже).
    uint8_t diag[2] = { 0xEE, resetCauseByte };
    HAL_UART_Transmit(&uartCfg, diag, sizeof(diag), 100);
    HAL_Delay(5);
    // === конец диагностики ===


    EbyteConfig ebyte_cfg = {};
    ebyte_cfg.huart = &uartCfg;
    ebyte_cfg.auxPin = GPIO_PIN_10;
    ebyte_cfg.m0Pin = GPIO_PIN_12;
    ebyte_cfg.m1Pin = GPIO_PIN_13;
    ebyte_cfg.auxPort = GPIOB;
    ebyte_cfg.m0Port = GPIOB;
    ebyte_cfg.m1Port = GPIOB;

    changeMode(EbyteMode::Config, ebyte_cfg);
    HAL_Delay(50);

    std::optional<EbyteSettings_t> settings = EbyteCfg(ebyte_cfg);
    HAL_Delay(50);

    uint8_t set_10dbm_cmd[6] = {
        0xC0,
        0x00,
        0x01,
        0x1A,
        0x17,
        0xC3
    };

    HAL_UART_Transmit(ebyte_cfg.huart, set_10dbm_cmd, sizeof(set_10dbm_cmd), 100);
    HAL_Delay(50);

    uint8_t buffer[6];
    HAL_UART_Receive(ebyte_cfg.huart, buffer, sizeof(buffer), 100);

    changeMode(EbyteMode::Normal, ebyte_cfg);
    HAL_Delay(50);

    while (HAL_GPIO_ReadPin(ebyte_cfg.auxPort, ebyte_cfg.auxPin) == GPIO_PIN_RESET) {
        HAL_Delay(1);
    }

    HAL_Delay(20);

    __HAL_UART_CLEAR_OREFLAG(&uartCfg);
    __HAL_UART_FLUSH_DRREGISTER(ebyte_cfg.huart);

    while (1) {
        std::optional<LoraRxFrame_t> packet = readLoRa(ebyte_cfg, 200);
        if (packet.has_value()) {
            const LoraRxFrame_t& rx = *packet;
            __NOP();
        }
    }
    return 0;
}
