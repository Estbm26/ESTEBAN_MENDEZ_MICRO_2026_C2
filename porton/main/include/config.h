#ifndef CONFIG_H
#define CONFIG_H

#include "app_config.h"
#include <stdbool.h>

typedef struct {
    ftc_mode_t ftc_mode;
    uint8_t motor_speed;
    bool encoder_enabled;
    char mqtt_broker_uri[128];
    int32_t calibration_max_pulses;
} system_config_t;

esp_err_t config_init(void);
esp_err_t config_load(system_config_t *cfg);
esp_err_t config_save(const system_config_t *cfg);
esp_err_t config_reset_defaults(void);
void config_print(const system_config_t *cfg);

#endif /* CONFIG_H */
