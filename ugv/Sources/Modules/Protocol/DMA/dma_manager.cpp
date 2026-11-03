#include "dma_manager.hpp"
#include "msp.hpp"

namespace bsp::dma {
	using msp::huart1;
	using msp::huart4;

	DMA_HandleTypeDef hdma_radar_rx = {};
	DMA_HandleTypeDef hdma_gps_rx   = {};

	static inline void enable_gpdma_clock() noexcept {
		if (!__HAL_RCC_GPDMA1_IS_CLK_ENABLED()) {
			__HAL_RCC_GPDMA1_CLK_ENABLE();
		}
	}

	[[nodiscard]] bool init_radar_rx() noexcept {
		enable_gpdma_clock();

		hdma_radar_rx.Instance                 = GPDMA1_Channel0;
		hdma_radar_rx.Init.Request             = GPDMA1_REQUEST_USART1_RX;
		hdma_radar_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		hdma_radar_rx.Init.SrcInc              = DMA_SINC_FIXED;
		hdma_radar_rx.Init.DestInc             = DMA_DINC_INCREMENTED;
		hdma_radar_rx.Init.SrcDataWidth        = DMA_SRC_DATAWIDTH_BYTE;
		hdma_radar_rx.Init.DestDataWidth       = DMA_DEST_DATAWIDTH_BYTE;
		hdma_radar_rx.Init.Priority            = DMA_LOW_PRIORITY_LOW_WEIGHT;
		hdma_radar_rx.Init.TransferEventMode   = DMA_TCEM_BLOCK_TRANSFER;
		hdma_radar_rx.Init.Mode                = DMA_NORMAL;

		if (HAL_DMA_Init(&hdma_radar_rx) != HAL_OK) {
			return false;
		}

		__HAL_LINKDMA(&huart1, hdmarx, hdma_radar_rx);

		// Пріоритет переривання під FreeRTOS (число має бути >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
		HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 6, 0);
		HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

		return true;
	}

	[[nodiscard]] bool init_gps_rx() noexcept {
		enable_gpdma_clock();

		hdma_gps_rx.Instance                   = GPDMA1_Channel1;
		hdma_gps_rx.Init.Request               = GPDMA1_REQUEST_UART4_RX;
		hdma_gps_rx.Init.Direction             = DMA_PERIPH_TO_MEMORY;
		hdma_gps_rx.Init.SrcInc                = DMA_SINC_FIXED;
		hdma_gps_rx.Init.DestInc               = DMA_DINC_INCREMENTED;
		hdma_gps_rx.Init.SrcDataWidth          = DMA_SRC_DATAWIDTH_BYTE;
		hdma_gps_rx.Init.DestDataWidth         = DMA_DEST_DATAWIDTH_BYTE;
		hdma_gps_rx.Init.Priority              = DMA_LOW_PRIORITY_LOW_WEIGHT;
		hdma_gps_rx.Init.TransferEventMode     = DMA_TCEM_BLOCK_TRANSFER;
		hdma_gps_rx.Init.Mode                  = DMA_NORMAL;

		if (HAL_DMA_Init(&hdma_gps_rx) != HAL_OK) {
			return false;
		}

		__HAL_LINKDMA(&huart4, hdmarx, hdma_gps_rx);

		HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 6, 0);
		HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

		return true;
	}

	void gps_uart_ClearErrors() noexcept {
	    uint32_t isr = huart4.Instance->ISR;
	    if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_PE | USART_ISR_FE)) {
	        __HAL_UART_CLEAR_FLAG(&huart4, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
	        __HAL_UART_SEND_REQ(&huart4, UART_RXDATA_FLUSH_REQUEST);
	    }
	    huart4.ErrorCode = HAL_UART_ERROR_NONE;
	    huart4.RxState = HAL_UART_STATE_READY;
	}

	void radar_uart_ClearErrors() noexcept {
		uint32_t isr = huart1.Instance->ISR;
	    if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_PE | USART_ISR_FE)) {
	        __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
	        __HAL_UART_SEND_REQ(&huart1, UART_RXDATA_FLUSH_REQUEST);
		}
	    huart1.ErrorCode = HAL_UART_ERROR_NONE;
	    huart1.RxState = HAL_UART_STATE_READY;
	}

	void deinit_radar_rx() noexcept {
		HAL_NVIC_DisableIRQ(GPDMA1_Channel0_IRQn);
		HAL_DMA_DeInit(&hdma_radar_rx);
	}

	void deinit_gps_rx() noexcept {
		HAL_NVIC_DisableIRQ(GPDMA1_Channel1_IRQn);
		HAL_DMA_DeInit(&hdma_gps_rx);
	}

} // namespace bsp::dma

