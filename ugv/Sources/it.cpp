#include "it.hpp"
#include "msp.hpp"

namespace isr_state {
    uint8_t gps_rx_line[UART_BUFFER_SIZE] = {0};
    uint16_t gps_rx_pos = 0;
    std::atomic<bool> gps_nmea_received{false};

    std::atomic<bool> dma_transfer_completed{false};
    std::atomic<bool> uart4_error_flag{false};
    volatile uint16_t uart4_rx_size = 0;

    uint8_t radar_rx_buffer[UART_BUFFER_SIZE] = {0};
    std::atomic<bool> radar_data_ready{false};
    std::atomic<bool> radar_error_flag{false};
    volatile uint16_t radar_rx_size = 0;

    std::atomic<uint32_t> left_ticks{0};
    std::atomic<uint32_t> right_ticks{0};
    std::atomic<bool> encoder_ready{false};

    std::atomic<bool> i2c_data_ready{false};
    std::array<int16_t, 3> accelerometer_data{0, 0, 0};
    alignas(4) uint8_t i2c_rx_raw[6] = {0};
}

extern "C" {

void NMI_Handler(void)
{
    while (1) {}
}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void DebugMon_Handler(void)
{
}

void TIM6_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&msp::htim6);
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&msp::hgpdma1);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&bsp::dma::hdma_gps_rx);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&msp::huart1);
}

void UART4_IRQHandler(void)
{
    uint32_t isr = msp::huart4.Instance->ISR;

    if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_PE | USART_ISR_FE)) {
        msp::huart4.Instance->ICR = USART_ICR_ORECF | USART_ICR_NECF | USART_ICR_PECF | USART_ICR_FECF;
        msp::huart4.ErrorCode = HAL_UART_ERROR_NONE;
    }

    while (msp::huart4.Instance->ISR & USART_ISR_RXNE_RXFNE) {
        uint8_t ch = static_cast<uint8_t>(msp::huart4.Instance->RDR);
        if (ch == '$') {
            isr_state::gps_rx_pos = 0;
        }
        if (isr_state::gps_rx_pos < isr_state::UART_BUFFER_SIZE - 1) {
            isr_state::gps_rx_line[isr_state::gps_rx_pos++] = ch;
        }
        if (ch == '\n' || ch == '\r') {
            if (isr_state::gps_rx_pos > 5) {
                isr_state::gps_rx_line[isr_state::gps_rx_pos] = '\0';
                isr_state::gps_nmea_received.store(true, std::memory_order_release);
            }
        }
    }
}

void EXTI14_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
}

void EXTI15_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

void I2C3_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&msp::hi2c3);
}

void I2C3_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&msp::hi2c3);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1) {
        isr_state::radar_rx_size = Size;
        isr_state::radar_data_ready.store(true, std::memory_order_release);
        isr_state::dma_transfer_completed.store(true, std::memory_order_release);
    } else if (huart->Instance == UART4) {
        isr_state::uart4_rx_size = Size;
        isr_state::dma_transfer_completed.store(true, std::memory_order_release);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        isr_state::radar_error_flag.store(true, std::memory_order_release);
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);
    } else if (huart->Instance == UART4) {
        isr_state::uart4_error_flag.store(true, std::memory_order_release);
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);
    }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    const uint32_t now = HAL_GetTick();
    if (msp::left_encoder.handle_exti(GPIO_Pin, now)) {
        if (msp::left_encoder.check_and_clear_rev()) {
            isr_state::encoder_ready.exchange(true, std::memory_order_relaxed);
        }
    }
    if (msp::right_encoder.handle_exti(GPIO_Pin, now)) {
        if (msp::right_encoder.check_and_clear_rev()) {
            isr_state::encoder_ready.exchange(true, std::memory_order_relaxed);
        }
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        auto parsed = LSM6DS::parse_raw_buffer(isr_state::i2c_rx_raw);
        isr_state::accelerometer_data[0] = parsed.x;
        isr_state::accelerometer_data[1] = parsed.y;
        isr_state::accelerometer_data[2] = parsed.z;
        isr_state::i2c_data_ready.store(true, std::memory_order_release);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        isr_state::i2c_data_ready.store(false, std::memory_order_relaxed);
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_AF | I2C_FLAG_OVR);
        hi2c->State = HAL_I2C_STATE_READY;
    }
}

} // extern "C"
