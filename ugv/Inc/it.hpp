#pragma once

#include "stm32h5xx_hal.h"
#include <cstdint>
#include <atomic>
#include <array>

namespace isr_state {
    inline constexpr uint32_t UART_BUFFER_SIZE = 256;
    inline constexpr int8_t   TICKS_PER_REV    = 20;

    extern uint8_t gps_rx_line[UART_BUFFER_SIZE];
    extern uint16_t gps_rx_pos;
    extern std::atomic<bool> gps_nmea_received;

    extern std::atomic<bool> dma_transfer_completed;
    extern std::atomic<bool> uart4_error_flag;
    extern volatile uint16_t uart4_rx_size;

    extern uint8_t radar_rx_buffer[UART_BUFFER_SIZE];
    extern std::atomic<bool> radar_data_ready;
    extern std::atomic<bool> radar_error_flag;
    extern volatile uint16_t radar_rx_size;

    extern std::atomic<uint32_t> left_ticks;
    extern std::atomic<uint32_t> right_ticks;
    extern std::atomic<bool> encoder_ready;

    extern std::atomic<bool> i2c_data_ready;
    extern std::array<int16_t, 3> accelerometer_data;
    extern uint8_t i2c_rx_raw[6];
}

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);

void TIM6_IRQHandler(void);
void GPDMA1_Channel0_IRQHandler(void);
void GPDMA1_Channel1_IRQHandler(void);
void USART1_IRQHandler(void);
void UART4_IRQHandler(void);
void EXTI14_IRQHandler(void);
void EXTI15_IRQHandler(void);
void I2C3_EV_IRQHandler(void);
void I2C3_ER_IRQHandler(void);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif
