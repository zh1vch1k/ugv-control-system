#pragma once
#include "utils/controller.hpp"
#include "driver/uart.h"
#include "esp_log.h"

#define UART_PORT           UART_NUM_0  
#define UART_BAUD_RATE      115200          
#define RX_BUF_SIZE         1024

bool readUART(Controller &pad);

void initUart(void);
