#pragma once

#include "stm32h5xx_hal.h"

namespace bsp::dma {


extern DMA_HandleTypeDef hdma_radar_rx;
extern DMA_HandleTypeDef hdma_gps_rx;

[[nodiscard]] bool init_radar_rx() noexcept;
[[nodiscard]] bool init_gps_rx() noexcept;

void deinit_radar_rx() noexcept;
void deinit_gps_rx() noexcept;

void gps_uart_ClearErrors() noexcept;
void radar_uart_ClearErrors() noexcept;

} // namespace bsp::dma
