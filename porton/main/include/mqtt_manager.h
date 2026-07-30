#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "state_machine.h"
#include "config.h"
#include <stdint.h>

typedef void (*mqtt_cmd_cb_t)(porton_event_t event);
typedef void (*mqtt_config_cb_t)(system_config_t *cfg);

esp_err_t mqtt_manager_init(mqtt_cmd_cb_t cmd_cb, mqtt_config_cb_t config_cb);
void mqtt_manager_start(void);
void mqtt_manager_publish_status(const char *state_str, uint32_t position);
void mqtt_manager_publish_event(const char *event_str);
void mqtt_manager_publish_telemetry(uint32_t position, int32_t speed);
void mqtt_manager_publish_config(const system_config_t *cfg);
bool mqtt_manager_is_connected(void);

#endif /* MQTT_MANAGER_H */
