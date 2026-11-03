#include "uart_controller.hpp"

#define RS485_UART_PORT     UART_NUM_2
#define RS485_PIN_TX        14
#define RS485_PIN_RX        13
#define RS485_RE_DE_PIN  GPIO_NUM_15
#define RX_BUFFER_SIZE      256

bool readUART(Controller &pad) {
    uint8_t byte = 0;

    while (uart_read_bytes(RS485_UART_PORT, &byte, 1, pdMS_TO_TICKS(10)) > 0) {
        if (byte == 0xAA) {
            pad.start_byte = byte;

            constexpr size_t remaining = sizeof(Controller) - 1;
            uint8_t *payload_ptr = reinterpret_cast<uint8_t*>(&pad) + 1;
            size_t received = 0;

            while (received < remaining) {
                int res = uart_read_bytes(
                    RS485_UART_PORT, 
                    payload_ptr + received, 
                    remaining - received, 
                    pdMS_TO_TICKS(20)
                );
                if (res <= 0) {
                    return false;
                }
                received += res;
            }
            return true;
        }
    }
    return false;
}

void initUart(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << RS485_RE_DE_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Притягиваем DE/RE к нулю (жесткий режим постоянного приема)
    gpio_set_level(RS485_RE_DE_PIN, 0);

    uart_config_t config = {};
    config.baud_rate = UART_BAUD_RATE;
    config.data_bits = UART_DATA_8_BITS;
    config.parity    = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    //setting pins for rs485
    ESP_ERROR_CHECK(uart_set_pin(
        RS485_UART_PORT , 
        RS485_PIN_TX, 
        RS485_PIN_RX, 
        UART_PIN_NO_CHANGE, 
        UART_PIN_NO_CHANGE
    ));

    ESP_ERROR_CHECK(uart_param_config(RS485_UART_PORT, &config));

    ESP_ERROR_CHECK(uart_driver_install(
        RS485_UART_PORT,
        RX_BUF_SIZE,
        0, 
        0, 
        NULL, 
        0
    ));
}
