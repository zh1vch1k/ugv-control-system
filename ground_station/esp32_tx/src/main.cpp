#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "packet.hpp"
#include "drivers/uart_controller.hpp"
#include "drivers/oled.hpp"




void draw_controller_screen(void* pv) {
    char buffer[64];
    int cnt = 0;
    Controller pad;

    oled_clear();
    oled_print(0, 0, "STATUS: UART WAIT");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();
    while(1) {
        if (readUART(pad)) {
            uint8_t btnMask = pad.btn_mask;

            snprintf(buffer, sizeof(buffer), "PACKET:%-4d" ,++cnt);
            oled_print(0, 0, buffer);
            oled_print(1, 0, "Sticks & Triggers");
            snprintf(buffer, sizeof(buffer), "LX:%-3u | LY:%-3u", pad.left_x, pad.left_y);
            oled_print(2, 4, buffer);

            snprintf(buffer, sizeof(buffer), "RX:%-3u | RY:%-3u", pad.right_x, pad.right_y);
            oled_print(3, 4, buffer);

            snprintf(buffer, sizeof(buffer), "LT: %d | RT:%-3u", pad.l_trigger, pad.r_trigger);
            oled_print(4, 4, buffer);

            oled_print(5, 0, "Buttons");
            snprintf(buffer, sizeof(buffer), "L3:%d | R3:%d", checkBtn(btnMask, 0), checkBtn(btnMask, 1));
            oled_print(6, 4, buffer);

            snprintf(buffer, sizeof(buffer), "L1:%d | R1:%d", checkBtn(btnMask, 2), checkBtn(btnMask, 3));
            oled_print(7, 4, buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
}



extern "C" void app_main(void) {
    TaskHandle_t drawHandler = NULL;

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_PORT;
    bus_config.scl_io_num = OLED_SCL;
    bus_config.sda_io_num = OLED_SDA;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.flags.enable_internal_pullup = true;

    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    i2c_master_bus_handle_t bus_handler;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handler));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = OLED_ADDRESS;
    dev_cfg.scl_speed_hz = 400000;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handler, &dev_cfg, &oled_dev));

    initUart();
    
    oled_init();
    oled_clear();

    oled_print(0, 0, "ESP32 LoRa");
    oled_print(1, 0, "Status: OK");
    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_set_level(GPIO_NUM_25, 1);
    xTaskCreatePinnedToCore(draw_controller_screen, "Draw", 4096, NULL, 1, &drawHandler, 1);

    vTaskDelete(NULL);
}