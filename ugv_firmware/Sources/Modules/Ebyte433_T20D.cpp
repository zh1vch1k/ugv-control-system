#include "Ebyte433_T20D.hpp"


bool waitForAuxHigh(GPIO_TypeDef* auxPort, uint16_t auxPin, uint32_t timeout_ms) {
	uint32_t start_tick = HAL_GetTick();

	while((HAL_GPIO_ReadPin(auxPort, auxPin) == GPIO_PIN_RESET)) {
		if (HAL_GetTick() - start_tick >= timeout_ms) {
			return false;
		}
	}
	return true;
}


std::optional<LoraRxFrame_t> readLoRa(const EbyteConfig& cfg, uint32_t timeout_ms) {
	if(__HAL_UART_GET_FLAG(cfg.huart, UART_FLAG_RXNE) == RESET) {
		return std::nullopt;
	}

	LoraRxFrame_t frame = {};


	HAL_StatusTypeDef status = HAL_UART_Receive(
	    cfg.huart,
		reinterpret_cast<uint8_t*>(&frame),
	    sizeof(LoraRxFrame_t),
		timeout_ms
	);

	if (status != HAL_OK) {
		__HAL_UART_CLEAR_OREFLAG(cfg.huart);
		return std::nullopt;
	}

	const uint8_t* payloadBytes = reinterpret_cast<const uint8_t*>(&frame.payload);
	uint8_t calculatedCrc = CRC8_calc(payloadBytes, sizeof(frame.payload));

	if (calculatedCrc == frame.crc8) {
		return frame;
	}

	return std::nullopt;
}


std::optional<EbyteSettings_t> EbyteCfg(const EbyteConfig& cfg, uint32_t timeout_ms) {
	uint8_t read_cmd[3] = {0xC1, 0xC1, 0xC1};
	EbyteSettings_t settings = {};
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_UART_Transmit(
		cfg.huart,
		read_cmd,
		sizeof(read_cmd),
		timeout_ms
	);

	if (status != HAL_OK) {
		return std::nullopt;
	}

	status = HAL_UART_Receive(
		cfg.huart,
		reinterpret_cast<uint8_t*>(&settings),
		sizeof(EbyteSettings_t),
		timeout_ms
	);

	if (status != HAL_OK) {
		return std::nullopt;
	}

	return settings;
}


bool changeMode(EbyteMode mode, const EbyteConfig& cfg, uint32_t timeout_ms) {
	const uint8_t uint_mode = static_cast<uint8_t>(mode);

	if (waitForAuxHigh(cfg.auxPort, cfg.auxPin, timeout_ms)) {
		GPIO_PinState m0_state = (uint_mode & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
		GPIO_PinState m1_state = (uint_mode & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET;

		HAL_GPIO_WritePin(cfg.m0Port, cfg.m0Pin, m0_state);
		HAL_GPIO_WritePin(cfg.m1Port, cfg.m1Pin, m1_state);

	}
	else {
		return false;
	}

	return waitForAuxHigh(cfg.auxPort, cfg.auxPin, timeout_ms);
}


void sendTelemetry() {}
