#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <atomic>

#include "packet.hpp"
#include "drivers/uart_controller.hpp"
#include "drivers/oled.hpp"
#include "drivers/LoRa.hpp"
#include "utils/packet_bridge.hpp"
QueueHandle_t controller_queue_handle = NULL;
static std::atomic<uint32_t> lora_count {0};

void draw_controller_screen(void* pv) {
    char buffer[64];
    int cnt = 0;
    Controller pad;

    oled_clear();
    oled_print(0, 0, "STATUS: UART WAIT");
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_clear();
    while(1) {
        if (xQueueReceive(controller_queue_handle, &pad, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint8_t btnMask = pad.btn_mask;

            snprintf(buffer, sizeof(buffer), "PACKET:%-5lu <-> %-4d" ,lora_count.load(std::memory_order_relaxed), ++cnt);
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

void broadcast(void* pv) {
    Controller pad;
    
    while(1) {
        if (readUART(pad)) {
            xQueueOverwrite(controller_queue_handle, &pad);
            LoraPayload_t payload =convertControllerData(pad);
            
            packetDataTransaction(0x00, 0x00, 0x17, payload);
            lora_count.fetch_add(1, std::memory_order_relaxed);
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

extern "C" void app_main(void) {
    controller_queue_handle = xQueueCreate(1, sizeof(Controller));
    TaskHandle_t drawHandler = NULL;
    TaskHandle_t broadcastHandler = NULL;

    spi_host_device_t spi_port = SPI2_HOST;

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_PORT;
    bus_config.scl_io_num = OLED_SCL;
    bus_config.sda_io_num = OLED_SDA;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handler;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handler));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = OLED_ADDRESS;
    dev_cfg.scl_speed_hz = 400000;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handler, &dev_cfg, &oled_dev));

    //LoRa setup
    gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    spi_bus_config_t spi_cfg = {};
    spi_cfg.mosi_io_num = LORA_MOSI;
    spi_cfg.miso_io_num = LORA_MISO;
    spi_cfg.sclk_io_num = LORA_SCLK;
    spi_cfg.quadwp_io_num = -1;
    spi_cfg.quadhd_io_num = -1;
    spi_cfg.max_transfer_sz = 256;

    ESP_ERROR_CHECK(spi_bus_initialize(spi_port, &spi_cfg, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(lora_init(spi_port));
    lora_setup();

    initUart();
    
    oled_init();
    oled_clear();

    oled_print(0, 0, "ESP32 LoRa");
    oled_print(1, 0, "Status: OK");
    vTaskDelay(pdMS_TO_TICKS(1000));

    xTaskCreatePinnedToCore(draw_controller_screen, "Draw", 4096, NULL, 1, &drawHandler, 1);
    xTaskCreatePinnedToCore(broadcast, "broadcast", 4096, NULL, 1, &broadcastHandler, 0);
    vTaskDelete(NULL);
}