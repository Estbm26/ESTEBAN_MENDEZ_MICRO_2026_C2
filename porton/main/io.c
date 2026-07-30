#include "io.h"
#include "app_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static const char *TAG = "IO";

/* Debounce states */
static uint32_t ls_closed_last_change = 0;
static uint32_t ls_open_last_change = 0;
static uint32_t ftc_last_change = 0;
static bool ls_closed_stable = true;
static bool ls_open_stable = true;
static bool ftc_stable = true;
static int ls_closed_count = 0;
static int ls_open_count = 0;
static int ftc_count = 0;

/* Buzzer state */
static bool buzzer_state = false;

void io_init(void)
{
    /* Limit switches como input con pull-up */
    gpio_set_direction(LS_CLOSED_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(LS_CLOSED_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction(LS_OPEN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(LS_OPEN_GPIO, GPIO_PULLUP_ONLY);

    /* FTC como input con pull-up */
    gpio_set_direction(FTC_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(FTC_GPIO, GPIO_PULLUP_ONLY);

    /* RGB LED */
    gpio_set_direction(RGB_R_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_G_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_B_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RGB_R_GPIO, 0);
    gpio_set_level(RGB_G_GPIO, 0);
    gpio_set_level(RGB_B_GPIO, 0);

    /* Buzzer PWM */
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BUZZER_PWM_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = BUZZER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .channel = BUZZER_PWM_CHANNEL,
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = BUZZER_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch);

    /* Estado inicial de entradas */
    ls_closed_stable = !gpio_get_level(LS_CLOSED_GPIO);
    ls_open_stable = !gpio_get_level(LS_OPEN_GPIO);
    ftc_stable = !gpio_get_level(FTC_GPIO);

    ESP_LOGI(TAG, "IO initialized: LS_CLOSED=%d LS_OPEN=%d FTC=%d",
             ls_closed_stable, ls_open_stable, ftc_stable);
}

void io_set_rgb(rgb_state_t state, uint32_t tick_ms)
{
    static bool blink_on = false;
    static uint32_t last_blink = 0;

    switch (state) {
        case RGB_CLOSED:
            gpio_set_level(RGB_R_GPIO, 1);
            gpio_set_level(RGB_G_GPIO, 0);
            gpio_set_level(RGB_B_GPIO, 0);
            break;
        case RGB_OPEN:
            gpio_set_level(RGB_R_GPIO, 0);
            gpio_set_level(RGB_G_GPIO, 1);
            gpio_set_level(RGB_B_GPIO, 0);
            break;
        case RGB_MOVING:
            if (tick_ms - last_blink >= 500) {
                last_blink = tick_ms;
                blink_on = !blink_on;
                gpio_set_level(RGB_R_GPIO, blink_on);
                gpio_set_level(RGB_G_GPIO, blink_on);
                gpio_set_level(RGB_B_GPIO, 0);
            }
            break;
        case RGB_ERROR:
            gpio_set_level(RGB_R_GPIO, 1);
            gpio_set_level(RGB_G_GPIO, 1);
            gpio_set_level(RGB_B_GPIO, 0);
            break;
        case RGB_OFF:
            gpio_set_level(RGB_R_GPIO, 0);
            gpio_set_level(RGB_G_GPIO, 0);
            gpio_set_level(RGB_B_GPIO, 0);
            break;
    }
}

void io_buzzer_on(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CHANNEL, 128);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CHANNEL);
    buzzer_state = true;
}

void io_buzzer_off(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CHANNEL);
    buzzer_state = false;
}

bool io_buzzer_is_on(void)
{
    return buzzer_state;
}

void io_tick(uint32_t tick_ms)
{
}

bool io_get_ls_closed(void)
{
    return ls_closed_stable;
}

bool io_get_ls_open(void)
{
    return ls_open_stable;
}

bool io_get_ftc(void)
{
    return ftc_stable;
}

porton_event_t io_scan_inputs(uint32_t tick_ms)
{
    porton_event_t event = EV_NONE;

    /* Leer raw values (0 = pressed por pull-up) */
    bool ls_closed_raw = (gpio_get_level(LS_CLOSED_GPIO) == 0);
    bool ls_open_raw   = (gpio_get_level(LS_OPEN_GPIO) == 0);
    bool ftc_raw       = (gpio_get_level(FTC_GPIO) == 0);

    /* Debounce limit switches */
    if (ls_closed_raw != ls_closed_stable) {
        if (tick_ms - ls_closed_last_change >= LS_DEBOUNCE_MS) {
            ls_closed_stable = ls_closed_raw;
            ls_closed_last_change = tick_ms;
        }
    } else {
        ls_closed_last_change = tick_ms;
    }

    if (ls_open_raw != ls_open_stable) {
        if (tick_ms - ls_open_last_change >= LS_DEBOUNCE_MS) {
            ls_open_stable = ls_open_raw;
            ls_open_last_change = tick_ms;
        }
    } else {
        ls_open_last_change = tick_ms;
    }

    /* Debounce FTC */
    if (ftc_raw != ftc_stable) {
        if (tick_ms - ftc_last_change >= FTC_DEBOUNCE_MS) {
            ftc_stable = ftc_raw;
            ftc_last_change = tick_ms;
            if (ftc_stable) {
                event = EV_FTC_TRIGGERED;
                ESP_LOGI(TAG, "FTC triggered");
            } else {
                event = EV_FTC_CLEARED;
                ESP_LOGI(TAG, "FTC cleared");
            }
        }
    } else {
        ftc_last_change = tick_ms;
    }

    /* Detectar ambos LS presionados (error) */
    if (ls_closed_stable && ls_open_stable) {
        event = EV_LS_BOTH_PRESSED;
    }

    return event;
}
