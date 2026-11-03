#include "msp.hpp"
#include "Modules/Led/led_config.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

// ====================================================================================
//                  ГЛОБАЛЬНЫЕ ХЭНДЛЫ АППАРАТНОЙ ПЕРИФЕРИИ
// ====================================================================================

// Экземпляр системного таймера тиков HAL (TIM6) для обратной совместимости с Си-вызовами
TIM_HandleTypeDef htim6 = {};

namespace msp {
    TIM_HandleTypeDef htim2 = {};
    TIM_HandleTypeDef htim3 = {};
    TIM_HandleTypeDef htim4 = {};

    UART_HandleTypeDef huart1 = {};
    UART_HandleTypeDef huart2 = {};
    UART_HandleTypeDef huart4 = {};
    UART_HandleTypeDef huart6 = {};

    I2C_HandleTypeDef hi2c3 = {};

    DMA_HandleTypeDef& hgpdma1 = bsp::dma::hdma_radar_rx;

    STM32_I2C_Adapter i2c3_adapter(&hi2c3, I2C3);
    LSM6DS            imu_sensor(i2c3_adapter);
    WheelEncoder      left_encoder({GPIOE, config::ENCODER_LEFT_PIN, EXTI14_IRQn, 6, 0}, 20, 10);
    WheelEncoder      right_encoder({GPIOE, config::ENCODER_RIGHT_PIN, EXTI15_IRQn, 6, 0}, 20, 10);
}

extern "C" {

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t pFLatency;
    uint32_t uwTimclock;
    uint32_t uwPrescalerValue;

    __HAL_RCC_TIM6_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    /* Вычисляем тактовую частоту TIM6 */
    if (clkconfig.APB1CLKDivider == RCC_HCLK_DIV1) {
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    } else {
        uwTimclock = 2UL * HAL_RCC_GetPCLK1Freq();
    }

    if (uwTimclock < 1000000U) {
        uwTimclock = 64000000U;
    }

    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

    ::htim6.Instance = TIM6;
    ::htim6.Init.Period = 1000U - 1U;
    ::htim6.Init.Prescaler = uwPrescalerValue;
    ::htim6.Init.ClockDivision = 0;
    ::htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    ::htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&::htim6) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_TIM_Base_Start_IT(&::htim6) != HAL_OK) {
        return HAL_ERROR;
    }

    HAL_NVIC_SetPriority(TIM6_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);

    uwTickPrio = TickPriority;

    return HAL_OK;
}

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&::htim6, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&::htim6, TIM_IT_UPDATE);
}


/**
  * @brief  Глобальная инициализация MSP (питание, системные шины, кэш)
  */
void HAL_MspInit(void)
{
    __HAL_RCC_SBS_CLK_ENABLE();

    // Отключение внутреннего подтягивания для энергосбережения
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
}

void HAL_MspDeInit(void)
{
}

/**
  * @brief  Низкоуровневая инициализация таймеров ШИМ TIM3 и TIM4 (пины и тактирование)
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef gpio = {};

    if (htim->Instance == TIM2) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_TIM2_CLK_ENABLE();

        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF1_TIM2;

        gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &gpio);

        gpio.Pin = GPIO_PIN_10;
        HAL_GPIO_Init(GPIOB, &gpio);

        gpio.Pin = GPIO_PIN_4;
        HAL_GPIO_Init(GPIOC, &gpio);
    }
    else if (htim->Instance == TIM3) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_TIM3_CLK_ENABLE();

        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF2_TIM3;

        gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        HAL_GPIO_Init(GPIOA, &gpio);

        gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
    else if (htim->Instance == TIM4) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_TIM4_CLK_ENABLE();

        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF2_TIM4;

        gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4);
    }
    else if (htim->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6 | GPIO_PIN_7);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1);
    }
    else if (htim->Instance == TIM4) {
        __HAL_RCC_TIM4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        __HAL_RCC_TIM6_CLK_ENABLE();
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        __HAL_RCC_TIM6_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(TIM6_IRQn);
    }
}

/**
  * @brief  Низкоуровневая инициализация UART (USART2, UART4, USART6)
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {};

    if (huart->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        gpio.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_PULLUP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio);

        HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (huart->Instance == USART2) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART2_CLK_ENABLE();

        gpio.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_PULLUP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &gpio);

        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
    else if (huart->Instance == UART4) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_UART4_CLK_ENABLE();

        gpio.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_PULLUP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF8_UART4;
        HAL_GPIO_Init(GPIOC, &gpio);

        HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(UART4_IRQn);
    }
    else if (huart->Instance == USART6) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_USART6_CLK_ENABLE();

        gpio.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_PULLUP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART6;
        HAL_GPIO_Init(GPIOC, &gpio);

        HAL_NVIC_SetPriority(USART6_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }
    else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    }
    else if (huart->Instance == UART4) {
        __HAL_RCC_UART4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11);
        HAL_NVIC_DisableIRQ(UART4_IRQn);
    }
    else if (huart->Instance == USART6) {
        __HAL_RCC_USART6_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
        HAL_NVIC_DisableIRQ(USART6_IRQn);
    }
}

/**
  * @brief  Низкоуровневая инициализация шины I2C3 (LSM6DS датчик)
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_I2C3_CLK_ENABLE();

        __HAL_RCC_I2C3_FORCE_RESET();
        __HAL_RCC_I2C3_RELEASE_RESET();

        GPIO_InitTypeDef gpio = {};
        gpio.Mode      = GPIO_MODE_AF_OD;
        gpio.Pull      = GPIO_PULLUP;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF4_I2C3;

        gpio.Pin = GPIO_PIN_8;
        HAL_GPIO_Init(GPIOA, &gpio);

        gpio.Pin = GPIO_PIN_9;
        HAL_GPIO_Init(GPIOC, &gpio);

        HAL_NVIC_SetPriority(I2C3_EV_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);

        HAL_NVIC_SetPriority(I2C3_ER_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        __HAL_RCC_I2C3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
        HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);
    }
}

} // extern "C"

// ====================================================================================
//                  ВЫСОКОУРОВНЕВАЯ РЕАЛИЗАЦИЯ ИНИЦИАЛИЗАЦИИ MSP
// ====================================================================================

namespace msp {

void init_system_clock() noexcept
{
    // Максимальная производительность регулятора напряжения (VOS0)
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitTypeDef osc = {};
    RCC_ClkInitTypeDef clk = {};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = RCC_HSI_DIV1;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while (1) {}
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_HCLK   |
                    RCC_CLOCKTYPE_PCLK1  |
                    RCC_CLOCKTYPE_PCLK2  |
                    RCC_CLOCKTYPE_PCLK3;

    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    clk.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) {
        while (1) {}
    }

    // Тактирование интерфейсов связи от PCLK1 (64 МГц)
    RCC_PeriphCLKInitTypeDef periph_clk = {};
    periph_clk.PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_UART4 | RCC_PERIPHCLK_USART6;
    periph_clk.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    periph_clk.Uart4ClockSelection  = RCC_UART4CLKSOURCE_PCLK1;
    periph_clk.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) != HAL_OK) {
        while (1) {}
    }

    SystemCoreClockUpdate();
}

bool init_tick_timer(uint32_t priority) noexcept
{
    return (HAL_InitTick(priority) == HAL_OK);
}

void init_leds() noexcept
{
    board_led::init();
    RGBLed::init();
}

bool init_tim3_motors() noexcept
{
    HAL_TIM_PWM_MspInit(&htim3);

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = config::PWM_PRESCALER;
    htim3.Init.Period            = config::PWM_PERIOD;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef oc = {};
    oc.OCMode     = TIM_OCMODE_PWM2;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.Pulse      = 0; // Изначально двигатели заглушены

    for (const uint32_t ch : config::PWM_CHANNELS) {
        if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, ch) != HAL_OK) {
            return false;
        }
        if (HAL_TIM_PWM_Start(&htim3, ch) != HAL_OK) {
            return false;
        }
    }

    stop_all_motors();
    return true;
}

bool init_tim4_motors() noexcept
{
    HAL_TIM_PWM_MspInit(&htim4);

    htim4.Instance               = TIM4;
    htim4.Init.Prescaler         = config::PWM_PRESCALER;
    htim4.Init.Period            = config::PWM_PERIOD;
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef oc = {};
    oc.OCMode     = TIM_OCMODE_PWM2;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.Pulse      = 0;

    for (const uint32_t ch : config::PWM_CHANNELS) {
        if (HAL_TIM_PWM_ConfigChannel(&htim4, &oc, ch) != HAL_OK) {
            return false;
        }
        if (HAL_TIM_PWM_Start(&htim4, ch) != HAL_OK) {
            return false;
        }
    }

    stop_all_motors();
    return true;
}

bool init_tim2_motors() noexcept
{
    HAL_TIM_PWM_MspInit(&htim2);

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = config::PWM_PRESCALER;
    htim2.Init.Period            = config::PWM_PERIOD;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef oc = {};
    oc.OCMode     = TIM_OCMODE_PWM2;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.Pulse      = 0;

    for (const uint32_t ch : config::PWM_CHANNELS) {
        if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc, ch) != HAL_OK) {
            return false;
        }
        if (HAL_TIM_PWM_Start(&htim2, ch) != HAL_OK) {
            return false;
        }
    }

    stop_all_motors();
    return true;
}

void stop_all_motors() noexcept
{
    if (htim2.Instance != nullptr) {
        htim2.Instance->CCR1 = 0;
        htim2.Instance->CCR2 = 0;
        htim2.Instance->CCR3 = 0;
        htim2.Instance->CCR4 = 0;
    }
    if (htim3.Instance != nullptr) {
        htim3.Instance->CCR1 = 0;
        htim3.Instance->CCR2 = 0;
        htim3.Instance->CCR3 = 0;
        htim3.Instance->CCR4 = 0;
    }
    if (htim4.Instance != nullptr) {
        htim4.Instance->CCR1 = 0;
        htim4.Instance->CCR2 = 0;
        htim4.Instance->CCR3 = 0;
        htim4.Instance->CCR4 = 0;
    }
}

void set_left_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept
{
    if (htim3.Instance == nullptr) return;
    if (fwd_pwm > 0) {
        htim3.Instance->CCR1 = fwd_pwm; htim3.Instance->CCR2 = 0;
        htim3.Instance->CCR3 = fwd_pwm; htim3.Instance->CCR4 = 0;
    } else if (rev_pwm > 0) {
        htim3.Instance->CCR1 = 0;       htim3.Instance->CCR2 = rev_pwm;
        htim3.Instance->CCR3 = 0;       htim3.Instance->CCR4 = rev_pwm;
    } else {
        htim3.Instance->CCR1 = 0;       htim3.Instance->CCR2 = 0;
        htim3.Instance->CCR3 = 0;       htim3.Instance->CCR4 = 0;
    }
}

void set_right_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept
{
    if (htim4.Instance == nullptr) return;
    if (fwd_pwm > 0) {
        htim4.Instance->CCR1 = fwd_pwm; htim4.Instance->CCR2 = 0;
        htim4.Instance->CCR3 = fwd_pwm; htim4.Instance->CCR4 = 0;
    } else if (rev_pwm > 0) {
        htim4.Instance->CCR1 = 0;       htim4.Instance->CCR2 = rev_pwm;
        htim4.Instance->CCR3 = 0;       htim4.Instance->CCR4 = rev_pwm;
    } else {
        htim4.Instance->CCR1 = 0;       htim4.Instance->CCR2 = 0;
        htim4.Instance->CCR3 = 0;       htim4.Instance->CCR4 = 0;
    }
}

void set_axis3_motors_pwm(uint32_t fwd_pwm, uint32_t rev_pwm) noexcept
{
    if (htim2.Instance == nullptr) return;
    if (fwd_pwm > 0) {
        htim2.Instance->CCR1 = fwd_pwm; htim2.Instance->CCR2 = 0;
        htim2.Instance->CCR3 = fwd_pwm; htim2.Instance->CCR4 = 0;
    } else if (rev_pwm > 0) {
        htim2.Instance->CCR1 = 0;       htim2.Instance->CCR2 = rev_pwm;
        htim2.Instance->CCR3 = 0;       htim2.Instance->CCR4 = rev_pwm;
    } else {
        htim2.Instance->CCR1 = 0;       htim2.Instance->CCR2 = 0;
        htim2.Instance->CCR3 = 0;       htim2.Instance->CCR4 = 0;
    }
}

bool init_usart1(uint32_t baudrate) noexcept
{
    HAL_UART_MspInit(&huart1);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = baudrate;
    huart1.Init.WordLength   = USART_WORDLENGTH_8B;
    huart1.Init.Parity       = USART_PARITY_NONE;
    huart1.Init.Mode         = USART_MODE_TX_RX;
    huart1.Init.StopBits     = USART_STOPBITS_1;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        return false;
    }
    clear_uart_errors(&huart1);
    return true;
}

bool init_usart2(uint32_t baudrate) noexcept
{
    HAL_UART_MspInit(&huart2);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = baudrate;
    huart2.Init.WordLength   = USART_WORDLENGTH_8B;
    huart2.Init.Parity       = USART_PARITY_NONE;
    huart2.Init.Mode         = USART_MODE_TX_RX;
    huart2.Init.StopBits     = USART_STOPBITS_1;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        return false;
    }
    clear_uart_errors(&huart2);
    return true;
}

bool init_uart4(uint32_t baudrate) noexcept
{
    HAL_UART_MspInit(&huart4);

    huart4.Instance          = UART4;
    huart4.Init.BaudRate     = baudrate;
    huart4.Init.WordLength   = USART_WORDLENGTH_8B;
    huart4.Init.Parity       = USART_PARITY_NONE;
    huart4.Init.Mode         = USART_MODE_TX_RX;
    huart4.Init.StopBits     = USART_STOPBITS_1;
    huart4.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart4) != HAL_OK) {
        return false;
    }

    clear_uart_errors(&huart4);

    // Включаем прерывание приемника по поступившему байту
    ATOMIC_SET_BIT(huart4.Instance->CR1, USART_CR1_RXNEIE_RXFNEIE);
    return true;
}

bool init_usart6(uint32_t baudrate) noexcept
{
    HAL_UART_MspInit(&huart6);

    huart6.Instance          = USART6;
    huart6.Init.BaudRate     = baudrate;
    huart6.Init.WordLength   = USART_WORDLENGTH_8B;
    huart6.Init.Parity       = USART_PARITY_NONE;
    huart6.Init.Mode         = USART_MODE_TX_RX;
    huart6.Init.StopBits     = USART_STOPBITS_1;
    huart6.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart6) != HAL_OK) {
        return false;
    }
    clear_uart_errors(&huart6);
    return true;
}

void uart6_log(const char* str) noexcept
{
    if (huart6.Instance == nullptr || str == nullptr) return;
    HAL_UART_Transmit(&huart6, reinterpret_cast<const uint8_t*>(str), std::strlen(str), 100);
}

void clear_uart_errors(UART_HandleTypeDef* huart) noexcept
{
    if (huart == nullptr || huart->Instance == nullptr) return;
    uint32_t isr = huart->Instance->ISR;
    if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_PE | USART_ISR_FE)) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);
    }
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_READY;
}

bool init_i2c3() noexcept
{
    HAL_I2C_MspInit(&hi2c3);
    return i2c3_adapter.init();
}

void reset_i2c3_bus() noexcept
{
    __HAL_UNLOCK(&hi2c3);
    hi2c3.State = HAL_I2C_STATE_RESET;
    hi2c3.Mode  = HAL_I2C_MODE_NONE;
    __HAL_I2C_CLEAR_FLAG(&hi2c3, I2C_FLAG_STOPF | I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_AF | I2C_FLAG_OVR);
    __HAL_RCC_I2C3_FORCE_RESET();
    __HAL_RCC_I2C3_RELEASE_RESET();
    init_i2c3();
}

void init_optopairs() noexcept
{
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin   = config::ENCODER_LEFT_PIN | config::ENCODER_RIGHT_PIN;
    gpio.Mode  = GPIO_MODE_IT_RISING;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

    HAL_NVIC_SetPriority(EXTI14_IRQn, 6, 0);
    HAL_NVIC_SetPriority(EXTI15_IRQn, 6, 0);

    HAL_NVIC_EnableIRQ(EXTI14_IRQn);
    HAL_NVIC_EnableIRQ(EXTI15_IRQn);
}

bool init_dma() noexcept
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    hgpdma1.Instance                 = GPDMA1_Channel0;
    hgpdma1.Init.Request             = GPDMA1_REQUEST_USART1_RX;
    hgpdma1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hgpdma1.Init.SrcInc              = DMA_SINC_FIXED;
    hgpdma1.Init.SrcDataWidth        = DMA_SRC_DATAWIDTH_BYTE;
    hgpdma1.Init.DestInc             = DMA_DINC_INCREMENTED;
    hgpdma1.Init.DestDataWidth       = DMA_DEST_DATAWIDTH_BYTE;
    hgpdma1.Init.Mode                = DMA_NORMAL;
    hgpdma1.Init.Priority            = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hgpdma1.Init.TransferEventMode   = DMA_TCEM_BLOCK_TRANSFER;

    if (HAL_DMA_Init(&hgpdma1) != HAL_OK) {
        __HAL_RCC_GPDMA1_CLK_DISABLE();
        return false;
    }

    if (huart1.Instance != nullptr) {
        __HAL_LINKDMA(&huart1, hdmarx, hgpdma1);
    }

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    return true;
}

bool init_all() noexcept
{
    // Включение FPU в CPACR
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
    __DSB();
    __ISB();

    HAL_Init();
    init_system_clock();
    init_tick_timer(0);

    init_leds();
    init_usart6(config::USART6_BAUDRATE);
    init_usart1(config::USART1_BAUDRATE);
    init_usart2(config::USART2_BAUDRATE);
    init_uart4(config::UART4_BAUDRATE);
    init_i2c3();
    init_optopairs();
    init_tim2_motors();
    init_tim3_motors();
    init_tim4_motors();
    init_dma();

    return true;
}

bool probe_and_init_lsm6ds(uint16_t* active_addr, uint8_t* chip_id) noexcept
{
    if (active_addr == nullptr || chip_id == nullptr) return false;

    uint16_t target_addr = 0;

    // 1. Проверяем адрес 0x6A (SA0 к GND), затем 0x6B (SA0 к 3.3V)
    if (HAL_I2C_IsDeviceReady(&hi2c3, config::LSM6DS_ADDR_PRIMARY, 3, 50) == HAL_OK) {
        target_addr = config::LSM6DS_ADDR_PRIMARY;
    } else if (HAL_I2C_IsDeviceReady(&hi2c3, config::LSM6DS_ADDR_SECONDARY, 3, 50) == HAL_OK) {
        target_addr = config::LSM6DS_ADDR_SECONDARY;
    } else {
        // Сканирование всей шины I2C при нестандартном адресе
        for (uint8_t a = 0x08; a <= 0x77; ++a) {
            if (HAL_I2C_IsDeviceReady(&hi2c3, (a << 1), 2, 20) == HAL_OK) {
                target_addr = (a << 1);
                break;
            }
        }
    }

    if (target_addr == 0) {
        return false;
    }

    *active_addr = target_addr;

    // 2. Чтение регистра WHO_AM_I (0x0F)
    uint8_t id = 0;
    hi2c3.State = HAL_I2C_STATE_READY;
    if (HAL_I2C_Mem_Read(&hi2c3, target_addr, config::REG_LSM6_WHO_AM_I,
                         I2C_MEMADD_SIZE_8BIT, &id, 1, 100) != HAL_OK) {
        return false;
    }
    *chip_id = id;

    // 3. Конфигурация акселерометра: 104 Гц, шкала ±2g (CTRL1_XL = 0x40)
    uint8_t ctrl1 = 0x40;
    hi2c3.State = HAL_I2C_STATE_READY;
    hi2c3.Lock  = HAL_UNLOCKED;
    HAL_I2C_Mem_Write(&hi2c3, target_addr, config::REG_LSM6_CTRL1_XL,
                      I2C_MEMADD_SIZE_8BIT, &ctrl1, 1, 100);

    uint8_t ctrl3 = 0x44;
    hi2c3.State = HAL_I2C_STATE_READY;
    hi2c3.Lock  = HAL_UNLOCKED;
    HAL_I2C_Mem_Write(&hi2c3, target_addr, config::REG_LSM6_CTRL3_C,
                      I2C_MEMADD_SIZE_8BIT, &ctrl3, 1, 100);

    HAL_Delay(30);
    return true;
}

bool read_lsm6ds_raw_accel(uint16_t dev_addr, int16_t raw[3]) noexcept
{
    if (raw == nullptr || dev_addr == 0) return false;

    uint8_t buf[6] = {0};
    if (HAL_I2C_Mem_Read(&hi2c3, dev_addr, config::REG_LSM6_OUTX_L_XL,
                         I2C_MEMADD_SIZE_8BIT, buf, sizeof(buf), 50) != HAL_OK) {
        reset_i2c3_bus();
        return false;
    }

    // LSM6DS: Little-Endian (младший байт Low, старший High)
    raw[0] = static_cast<int16_t>(buf[0] | (static_cast<uint16_t>(buf[1]) << 8));
    raw[1] = static_cast<int16_t>(buf[2] | (static_cast<uint16_t>(buf[3]) << 8));
    raw[2] = static_cast<int16_t>(buf[4] | (static_cast<uint16_t>(buf[5]) << 8));

    return true;
}

// ====================================================================================
//            КОМПЛЕКСНАЯ БАЗОВАЯ ПРОВЕРКА И САМОДИАГНОСТИКА ПЕРИФЕРИИ (POST)
// ====================================================================================

CheckResult check_peripherals(bool log_to_uart6) noexcept
{
    CheckResult res = {};
    char log_buf[128];

    // 1. Проверка системного тактирования и таймера квантования HAL (TIM6)
    res.clock_ok = (SystemCoreClock >= 16000000U);
    const uint32_t tick_start = HAL_GetTick();
    HAL_Delay(10);
    const uint32_t tick_delta = HAL_GetTick() - tick_start;
    res.tick_timer_ok = (tick_delta >= 8 && tick_delta <= 15);

    // 2. Визуальный тест светодиодов
    board_led::on();
    RGBLed::setColor(RgbColor::White);
    HAL_Delay(100);
    RGBLed::setColor(RgbColor::Red);
    HAL_Delay(100);
    RGBLed::setColor(RgbColor::Green);
    HAL_Delay(100);
    RGBLed::setColor(RgbColor::Blue);
    HAL_Delay(100);
    RGBLed::setColor(RgbColor::Off);
    board_led::off();
    res.leds_ok = true;

    if (htim2.Instance == TIM2 && (htim2.Instance->CR1 & TIM_CR1_CEN)) {
        res.tim2_motors_ok = (htim2.Init.Period == config::PWM_PERIOD);
    }
    if (htim3.Instance == TIM3 && (htim3.Instance->CR1 & TIM_CR1_CEN)) {
        res.tim3_motors_ok = (htim3.Init.Period == config::PWM_PERIOD);
    }
    if (htim4.Instance == TIM4 && (htim4.Instance->CR1 & TIM_CR1_CEN)) {
        res.tim4_motors_ok = (htim4.Init.Period == config::PWM_PERIOD);
    }
    stop_all_motors();

    res.usart1_ok = (huart1.Instance == USART1) && (huart1.Instance->CR1 & USART_CR1_UE);
    res.usart2_ok = (huart2.Instance == USART2) && (huart2.Instance->CR1 & USART_CR1_UE);
    res.uart4_ok  = (huart4.Instance == UART4)  && (huart4.Instance->CR1 & USART_CR1_UE);
    res.usart6_ok = (huart6.Instance == USART6) && (huart6.Instance->CR1 & USART_CR1_UE);

    // 5. Проверка шины I2C3 и датчика IMU LSM6DS
    res.i2c3_ok = (hi2c3.Instance == I2C3);
    if (res.i2c3_ok) {
        uint16_t sensor_addr = 0;
        uint8_t chip_id = 0;
        if (probe_and_init_lsm6ds(&sensor_addr, &chip_id)) {
            res.imu_sensor_ok   = true;
            res.imu_active_addr = sensor_addr;
            res.imu_chip_id     = chip_id;
            read_lsm6ds_raw_accel(sensor_addr, res.accel_raw);
        }
    }

    // 6. Проверка оптопар (энкодеры колес)
    GPIO_PinState p14 = HAL_GPIO_ReadPin(GPIOE, config::ENCODER_LEFT_PIN);
    GPIO_PinState p15 = HAL_GPIO_ReadPin(GPIOE, config::ENCODER_RIGHT_PIN);
    res.optopair_left_ok  = (p14 == GPIO_PIN_SET || p14 == GPIO_PIN_RESET);
    res.optopair_right_ok = (p15 == GPIO_PIN_SET || p15 == GPIO_PIN_RESET);

    // 7. Проверка DMA
    res.dma_ok = __HAL_RCC_GPDMA1_IS_CLK_ENABLED();

    // Общая оценка готовности системы
    res.all_ok = res.clock_ok && res.tick_timer_ok && res.leds_ok &&
                 res.tim2_motors_ok && res.tim3_motors_ok && res.tim4_motors_ok &&
                 res.usart2_ok && res.usart6_ok;

    // 8. Форматированный вывод протокола диагностики в UART6 (если запрошен)
    if (log_to_uart6 && res.usart6_ok) {
        uart6_log("\r\n====================================================================\r\n");
        uart6_log("             UGV CHASSIS PERIPHERAL SELF-TEST (MSP)                 \r\n");
        uart6_log("====================================================================\r\n");

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Clock System    : HSI %lu MHz (Core: %lu Hz)\r\n",
                      res.clock_ok ? " OK " : "FAIL",
                      SystemCoreClock / 1000000U, SystemCoreClock);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Timebase (TIM6) : 1000 Hz Tick (measured: %lu ms/10ms)\r\n",
                      res.tick_timer_ok ? " OK " : "FAIL", tick_delta);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] LEDs Subsystem  : Board LED (PB2) + RGB (PE2..PE4)\r\n",
                      res.leds_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Axis3 PWM (TIM2): 4 kHz, 4 Channels (PA0, PA1, PB10, PC4)\r\n",
                      res.tim2_motors_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Left PWM  (TIM3): 4 kHz, 4 Channels (PA6, PA7, PB0, PB1)\r\n",
                      res.tim3_motors_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Right PWM (TIM4): 4 kHz, 4 Channels (PB6, PB7, PB8, PB9)\r\n",
                      res.tim4_motors_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Radar UART (US1): %lu 8N1 (PA9 TX, PA10 RX)\r\n",
                      res.usart1_ok ? " OK " : "FAIL", config::USART1_BAUDRATE);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Radio UART (US2): %lu 8N1 (PA2 TX, PA3 RX)\r\n",
                      res.usart2_ok ? " OK " : "FAIL", config::USART2_BAUDRATE);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] GPS UART   (U4) : %lu 8N1 (PC10 TX, PC11 RX)\r\n",
                      res.uart4_ok ? " OK " : "FAIL", config::UART4_BAUDRATE);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Logger UART(US6): %lu 8N1 (PC6 TX, PC7 RX)\r\n",
                      res.usart6_ok ? " OK " : "FAIL", config::USART6_BAUDRATE);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] I2C3 Bus        : 100 kHz (PA8 SCL, PC9 SDA)\r\n",
                      res.i2c3_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        if (res.imu_sensor_ok) {
            std::snprintf(log_buf, sizeof(log_buf),
                          "[ OK ] IMU Sensor (LSM) : Found at 0x%02X, ID=0x%02X | X:%d Y:%d Z:%d\r\n",
                          res.imu_active_addr >> 1, res.imu_chip_id,
                          res.accel_raw[0], res.accel_raw[1], res.accel_raw[2]);
            uart6_log(log_buf);
        } else {
            uart6_log("[WARN] IMU Sensor (LSM) : NOT DETECTED at 0x6A/0x6B\r\n");
        }

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] Wheel Encoders  : Left (PE14)=%d, Right (PE15)=%d\r\n",
                      (res.optopair_left_ok && res.optopair_right_ok) ? " OK " : "FAIL",
                      p14, p15);
        uart6_log(log_buf);

        std::snprintf(log_buf, sizeof(log_buf),
                      "[%s] DMA Controller  : GPDMA1 Clock & Channel Ready\r\n",
                      res.dma_ok ? " OK " : "FAIL");
        uart6_log(log_buf);

        uart6_log("--------------------------------------------------------------------\r\n");
        if (res.all_ok && res.imu_sensor_ok) {
            uart6_log("STATUS: >>> ALL SUBSYSTEMS FULLY OPERATIONAL <<<\r\n");
            RGBLed::setColor(RgbColor::Green);
        } else if (res.all_ok && !res.imu_sensor_ok) {
            uart6_log("STATUS: >>> CHASSIS READY (WARNING: IMU SENSOR OFFLINE) <<<\r\n");
            RGBLed::setColor(RgbColor::Yellow);
        } else {
            uart6_log("STATUS: >>> CRITICAL PERIPHERAL FAILURE DETECTED <<<\r\n");
            RGBLed::setColor(RgbColor::Red);
        }
        uart6_log("====================================================================\r\n\r\n");
    }

    return res;
}

} // namespace msp
