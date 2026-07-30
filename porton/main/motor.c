#include "motor.h"
#include "app_config.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

static const char *TAG = "MOTOR";

static motor_direction_t current_dir = MOTOR_STOP;
static uint8_t current_speed = 0;

void motor_init(void)
{
    gpio_set_direction(MOTOR_DIR1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_DIR2_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_DIR1_GPIO, 0);
    gpio_set_level(MOTOR_DIR2_GPIO, 0);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = MOTOR_PWM_TIMER,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .channel = MOTOR_PWM_CHANNEL,
        .gpio_num = MOTOR_PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch);

    ESP_LOGI(TAG, "Motor initialized");
}

void motor_set(motor_direction_t dir, uint8_t speed)
{
    current_dir = dir;
    current_speed = speed;

    switch (dir) {
        case MOTOR_OPEN:
            gpio_set_level(MOTOR_DIR1_GPIO, 1);
            gpio_set_level(MOTOR_DIR2_GPIO, 0);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, speed);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            ESP_LOGD(TAG, "Motor OPEN speed=%u", speed);
            break;
        case MOTOR_CLOSE:
            gpio_set_level(MOTOR_DIR1_GPIO, 0);
            gpio_set_level(MOTOR_DIR2_GPIO, 1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, speed);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            ESP_LOGD(TAG, "Motor CLOSE speed=%u", speed);
            break;
        case MOTOR_STOP:
        default:
            motor_stop();
            break;
    }
}

void motor_stop(void)
{
    gpio_set_level(MOTOR_DIR1_GPIO, 0);
    gpio_set_level(MOTOR_DIR2_GPIO, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
    current_dir = MOTOR_STOP;
    current_speed = 0;
    ESP_LOGD(TAG, "Motor STOP");
}

motor_direction_t motor_get_direction(void)
{
    return current_dir;
}

uint8_t motor_get_speed(void)
{
    return current_speed;
}
