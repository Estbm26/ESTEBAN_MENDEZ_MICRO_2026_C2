#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "driver/ledc.h"

/* ============================================================
 *  PIN DEFINITIONS (Ajustar según cableado real)
 * ============================================================ */

/* MOTOR - 2 pines de dirección + 1 PWM */
#define MOTOR_DIR1_GPIO      GPIO_NUM_2
#define MOTOR_DIR2_GPIO      GPIO_NUM_4
#define MOTOR_PWM_GPIO       GPIO_NUM_5
#define MOTOR_PWM_TIMER      LEDC_TIMER_0
#define MOTOR_PWM_CHANNEL    LEDC_CHANNEL_0
#define MOTOR_PWM_FREQ_HZ    5000
#define MOTOR_PWM_RESOLUTION LEDC_TIMER_8_BIT
#define MOTOR_DEFAULT_SPEED  200   /* 0-255 */

/* ENCODER - 2 pines */
#define ENCODER_A_GPIO       GPIO_NUM_6
#define ENCODER_B_GPIO       GPIO_NUM_7

/* I2C para display OLED 0.96" SSD1306 */
#define I2C_MASTER_SCL       GPIO_NUM_8
#define I2C_MASTER_SDA       GPIO_NUM_9
#define I2C_MASTER_FREQ_HZ   400000
#define OLED_ADDR            0x3C
#define OLED_WIDTH           128
#define OLED_HEIGHT          64

/* LIMIT SWITCHES (NA - botones pull-up) */
#define LS_CLOSED_GPIO       GPIO_NUM_10
#define LS_OPEN_GPIO         GPIO_NUM_11

/* FTC - Fotocelda */
#define FTC_GPIO             GPIO_NUM_12

/* RGB LED */
#define RGB_R_GPIO           GPIO_NUM_13
#define RGB_G_GPIO           GPIO_NUM_14
#define RGB_B_GPIO           GPIO_NUM_15

/* BUZZER */
#define BUZZER_GPIO          GPIO_NUM_16
#define BUZZER_PWM_TIMER     LEDC_TIMER_1
#define BUZZER_PWM_CHANNEL   LEDC_CHANNEL_1
#define BUZZER_PWM_FREQ_HZ   2000

/* ============================================================
 *  TIMING CONSTANTS
 * ============================================================ */
#define MAIN_TIMER_MS        50         /* Timer principal cada 50ms */
#define FTC_DEBOUNCE_MS      100        /* Anti-rebote FTC */
#define LS_DEBOUNCE_MS       50         /* Anti-rebote limit switches */
#define TELEMETRY_INTERVAL_MS 5000      /* Publicar telemetría cada 5s */
#define DISPLAY_UPDATE_MS    250        /* Actualizar display cada 250ms */

/* ============================================================
 *  MQTT CONSTANTS
 * ============================================================ */
#define MQTT_BROKER_URI      "mqtt://192.168.1.100:1883"
#define MQTT_CLIENT_ID       "porton-esp32s3"

/* Tópicos MQTT */
#define TOPIC_CMD            "porton/cmd"
#define TOPIC_CONFIG_SET     "porton/config/set"
#define TOPIC_STATUS         "porton/status"
#define TOPIC_EVENT          "porton/event"
#define TOPIC_TELEMETRY      "porton/telemetry"
#define TOPIC_CONFIG         "porton/config"

/* ============================================================
 *  MISC CONSTANTS
 * ============================================================ */
#define FIRMWARE_VERSION     "1.0.0"

/* Modos de comportamiento FTC */
typedef enum {
    FTC_MODE_STOP = 0,
    FTC_MODE_RESUME,
    FTC_MODE_REVERSE
} ftc_mode_t;

#endif /* APP_CONFIG_H */
