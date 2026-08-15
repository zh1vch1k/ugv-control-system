#include "uart_controller.hpp"

bool readUART(Controller &pad) {
    int len = uart_read_bytes(UART_NUM_0, (uint8_t*)&pad, sizeof(Controller), portMAX_DELAY);

    if (len == sizeof(Controller)) {
        if (pad.start_byte == 0xAA) {
            return true;
        }
    }   
    return false;
}

void initUart(void) {
    uart_config_t config = {};
    config.baud_rate = UART_BAUD_RATE;
    config.data_bits = UART_DATA_8_BITS;
    config.parity    = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));

    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT,
        RX_BUF_SIZE,
        0, 
        0, 
        NULL, 
        0
    ));
}
