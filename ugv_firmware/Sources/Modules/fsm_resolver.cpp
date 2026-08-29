#include "fsm_resolver.hpp"


void flushUartBuffer(UART_HandleTypeDef* huart) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);

    while ((huart->Instance->SR & USART_SR_RXNE) != 0) {
        volatile uint8_t dummy = huart->Instance->DR;
        (void)dummy;
    }
}

std::optional<uint32_t> readHandshake(const EbyteConfig& cfg, uint32_t salt, uint32_t timeout_ms) {
	flushUartBuffer(cfg.huart);

//	if (__HAL_UART_GET_FLAG(cfg.huart, UART_FLAG_RXNE) == RESET) {
//	   return std::nullopt;
//	}

	HandshakeRx_t handshake = {};

	HAL_StatusTypeDef status = HAL_UART_Receive(
            cfg.huart,
			reinterpret_cast<uint8_t*>(&handshake),
	        sizeof(HandshakeRx_t),
	        timeout_ms
    );

	if (status != HAL_OK) {
		flushUartBuffer(cfg.huart);
		return std::nullopt;
	}

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

	if (handshake.rx_session_salt == salt) {
		return handshake.session_id;
	}

	return std::nullopt;
}

//std::optional<HandshakeTx_t> sendHandshake(const EbyteConfig& cfg, uint8_t addh, uint8_t addl, uint8_t channel,
//				   uint32_t session_id, uint32_t salt, uint32_t timeout_ms) {
//	HandshakeTx_t response = {};
//
//	response.addh = addh;
//	response.addl = addl;
//	response.channel = channel;
//	response.tx_session_salt = salt;
//	response.session_id = session_id;
//
//	HAL_StatusTypeDef status = HAL_UART_Transmit(
//		cfg.huart,
//		reinterpret_cast<uint8_t*>(&response),
//		sizeof(HandshakeTx_t),
//		timeout_ms);
//
//	if (status != HAL_OK) {
//		return std::nullopt;
//	}
//
//	return response;
//}

std::optional<HandshakeTx_t> sendHandshake(const EbyteConfig& cfg, uint8_t addh, uint8_t addl, uint8_t channel,
                                   uint32_t session_id, uint32_t salt, uint32_t timeout_ms) {
    // 1. Формуємо структуру відповіді
    HandshakeTx_t response = {};
    response.addh = addh;
    response.addl = addl;
    response.channel = channel;
    response.tx_session_salt = salt;
    response.session_id = session_id;

    // 2. Якщо модуль EBYTE на STM32 налаштований у напівпрозорому режимі (Transparent transmission),
    // він очікує першими байтами ADDH, ADDL, CHANNEL, а далі корисне навантаження.
    // Але якщо на ESP32 обробник очікує XOR-маскування (як у sendEbyteRadioFrame),
    // нам потрібно застосувати такий самий XOR-ключ перед відправкою у UART,
    // щоб модуль EBYTE випнув в ефір байти, які ESP32 зможе розшифрувати!

    // Виділяємо корисне навантаження (все, що після addh, addl, channel -> це 3 байти)
    const uint8_t* innerBytes = reinterpret_cast<const uint8_t*>(&response) + 3;
    const size_t innerLen = sizeof(HandshakeTx_t) - 3;

    // Створюємо буфер для відправки з урахуванням логіки пакету
    uint8_t tx_buffer[sizeof(HandshakeTx_t)];

    // Копіюємо заголовки (addh, addl, channel) без змін
    std::memcpy(tx_buffer, &response, 3);

    // Обчислюємо той самий XOR-ключ, який використовує ESP32 у sendEbyteRadioFrame:
    // const uint8_t xorKey = 0xCA - txFrame.channel;
    const uint8_t xorKey = 0xCA - channel;

    // Накладаємо XOR на корисне навантаження (щоб воно збіглося з тим, що чекає ESP32)
    for (size_t i = 0; i < innerLen; ++i) {
        tx_buffer[3 + i] = innerBytes[i] ^ xorKey;
    }

    // 3. Перед передачею перевіряємо, чи вільний UART і чи не заблокований пін AUX модуля EBYTE
    uint32_t start_tick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(cfg.auxPort, cfg.auxPin) == GPIO_PIN_RESET) {
        if (HAL_GetTick() - start_tick >= timeout_ms) {
            return std::nullopt; // Модуль зайнятий передачею попереднього пакету
        }
    }

    // 4. Здійснюємо безпечну передачу через UART
    HAL_StatusTypeDef status = HAL_UART_Transmit(
        cfg.huart,
        tx_buffer,
        sizeof(tx_buffer),
        timeout_ms
    );

    if (status != HAL_OK) {
        return std::nullopt;
    }

    return response;
}



std::optional<HandshakeRx_t> sendHandshakeReply(const EbyteConfig& cfg, uint32_t session_id, uint32_t salt, uint32_t timeout_ms) {
    HandshakeRx_t reply{};
    reply.rx_session_salt = salt;      // RX_SESSION_SALT — без XOR, как ждёт ESP
    reply.session_id      = session_id;

    HAL_StatusTypeDef status = HAL_UART_Transmit(
        cfg.huart,
        reinterpret_cast<uint8_t*>(&reply),
        sizeof(HandshakeRx_t),         // 8 байт, без заголовка и checksum
        timeout_ms
    );

    return reply;
}

