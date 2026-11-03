#include "led.h"
#include "led_strip.h"
#include "esp_log.h"

#define LED_STRIP_GPIO_PIN    8
#define LED_STRIP_LED_NUMBERS 1

static const char *TAG = "LED";
static led_strip_handle_t s_led_strip = nullptr;

const uint8_t COLORS[][3] = {
    {50,   0,   0},  // 0: Red
    {0,   50,   0},  // 1: Green
    {0,    0,  50},  // 2: Blue
    {50,  50,   0},  // 3: Yellow
    {0,   50,  50},  // 4: Cyan
    {50,   0,  50},  // 5: Magenta
    {50,  20,   0},  // 6: Orange
    {0,   50,  20},  // 7: Mint
    {20,   0,  50},  // 8: Deep Violet
    {50,   0,  20},  // 9: Pink
    {30,  50,   0},  // 10: Lime
    {0,   20,  50},  // 11: Sky Blue
    {40,  15,  40},  // 12: Lavender
    {50,  35,  10},  // 13: Warm Amber
    {30,  30,  30}   // 14: Soft White
};

esp_err_t led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,
        .max_leds = LED_STRIP_LED_NUMBERS,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (err == ESP_OK) {
        led_clear();
        ESP_LOGI(TAG, "LED strip initialized on GPIO %d", LED_STRIP_GPIO_PIN);
    } else {
        ESP_LOGE(TAG, "Failed to init LED strip: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!s_led_strip) return ESP_ERR_INVALID_STATE;
    led_strip_set_pixel(s_led_strip, 0, r, g, b);
    return led_strip_refresh(s_led_strip);
}

esp_err_t led_set_status(LedStatus status) {
    uint8_t idx = static_cast<uint8_t>(status);
    if (idx >= sizeof(COLORS) / sizeof(COLORS[0])) {
        return ESP_ERR_INVALID_ARG;
    }
    return led_set_rgb(COLORS[idx][0], COLORS[idx][1], COLORS[idx][2]);
}

esp_err_t led_clear(void) {
    if (!s_led_strip) return ESP_ERR_INVALID_STATE;
    return led_strip_clear(s_led_strip);
}