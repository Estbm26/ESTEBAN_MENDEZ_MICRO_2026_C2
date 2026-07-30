#include "encoder.h"
#include "app_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "ENCODER";

static volatile int32_t pulse_count = 0;
static int32_t last_a = 0;
static int32_t last_b = 0;
static bool enabled = false;

static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    int a = gpio_get_level(ENCODER_A_GPIO);
    int b = gpio_get_level(ENCODER_B_GPIO);

    if (a != last_a) {
        if (a == b) {
            pulse_count++;
        } else {
            pulse_count--;
        }
        last_a = a;
    }
    if (b != last_b) {
        last_b = b;
    }
}

void encoder_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_A_GPIO) | (1ULL << ENCODER_B_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    last_a = gpio_get_level(ENCODER_A_GPIO);
    last_b = gpio_get_level(ENCODER_B_GPIO);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_A_GPIO, encoder_isr_handler, NULL);
    gpio_isr_handler_add(ENCODER_B_GPIO, encoder_isr_handler, NULL);

    enabled = true;
    ESP_LOGI(TAG, "Encoder initialized, A=%d B=%d", last_a, last_b);
}

void encoder_enable(bool enable)
{
    enabled = enable;
    if (enable) {
        gpio_isr_handler_add(ENCODER_A_GPIO, encoder_isr_handler, NULL);
        gpio_isr_handler_add(ENCODER_B_GPIO, encoder_isr_handler, NULL);
    } else {
        gpio_isr_handler_remove(ENCODER_A_GPIO);
        gpio_isr_handler_remove(ENCODER_B_GPIO);
    }
    ESP_LOGI(TAG, "Encoder %s", enable ? "enabled" : "disabled");
}

bool encoder_is_enabled(void)
{
    return enabled;
}

int32_t encoder_get_pulses(void)
{
    int32_t val;
    val = pulse_count;
    return val;
}

void encoder_reset(void)
{
    pulse_count = 0;
    last_a = gpio_get_level(ENCODER_A_GPIO);
    last_b = gpio_get_level(ENCODER_B_GPIO);
    ESP_LOGD(TAG, "Encoder reset");
}

int32_t encoder_get_speed(void)
{
    static int32_t last_pulses = 0;
    static int32_t speed = 0;
    int32_t current = pulse_count;
    speed = current - last_pulses;
    last_pulses = current;
    return speed;
}
