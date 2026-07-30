#include "mqtt_manager.h"
#include "app_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static mqtt_cmd_cb_t user_cmd_cb = NULL;
static mqtt_config_cb_t user_config_cb = NULL;
static bool connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            connected = true;
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_CMD, 1);
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_CONFIG_SET, 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            connected = false;
            break;

        case MQTT_EVENT_DATA: {
            char *topic = strndup(event->topic, event->topic_len);
            char *data = strndup(event->data, event->data_len);
            ESP_LOGI(TAG, "MQTT data: topic=%s data=%s", topic, data);

            if (strcmp(topic, TOPIC_CMD) == 0) {
                if (user_cmd_cb) {
                    if (strcmp(data, "OPEN") == 0)      user_cmd_cb(EV_CMD_OPEN);
                    else if (strcmp(data, "CLOSE") == 0) user_cmd_cb(EV_CMD_CLOSE);
                    else if (strcmp(data, "STOP") == 0)  user_cmd_cb(EV_CMD_STOP);
                    else if (strcmp(data, "TOGGLE") == 0) user_cmd_cb(EV_CMD_TOGGLE);
                    else if (strcmp(data, "CALIBRATE") == 0) user_cmd_cb(EV_CMD_CALIBRATE);
                    else if (strcmp(data, "RESET") == 0) user_cmd_cb(EV_ERROR_CLEAR);
                }
            } else if (strcmp(topic, TOPIC_CONFIG_SET) == 0) {
                if (user_config_cb) {
                    cJSON *root = cJSON_Parse(data);
                    if (root) {
                        system_config_t cfg;
                        config_load(&cfg);

                        cJSON *item;
                        item = cJSON_GetObjectItem(root, "ftc_mode");
                        if (item && cJSON_IsString(item)) {
                            if (strcmp(item->valuestring, "stop") == 0)
                                cfg.ftc_mode = FTC_MODE_STOP;
                            else if (strcmp(item->valuestring, "resume") == 0)
                                cfg.ftc_mode = FTC_MODE_RESUME;
                            else if (strcmp(item->valuestring, "reverse") == 0)
                                cfg.ftc_mode = FTC_MODE_REVERSE;
                        }
                        item = cJSON_GetObjectItem(root, "motor_speed");
                        if (item && cJSON_IsNumber(item))
                            cfg.motor_speed = (uint8_t)item->valueint;
                        item = cJSON_GetObjectItem(root, "encoder_enabled");
                        if (item && cJSON_IsBool(item))
                            cfg.encoder_enabled = cJSON_IsTrue(item);
                        item = cJSON_GetObjectItem(root, "mqtt_broker");
                        if (item && cJSON_IsString(item))
                            strncpy(cfg.mqtt_broker_uri, item->valuestring, sizeof(cfg.mqtt_broker_uri) - 1);

                        config_save(&cfg);
                        if (user_config_cb) user_config_cb(&cfg);
                        cJSON_Delete(root);
                    }
                }
            }

            free(topic);
            free(data);
            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

esp_err_t mqtt_manager_init(mqtt_cmd_cb_t cmd_cb, mqtt_config_cb_t config_cb)
{
    user_cmd_cb = cmd_cb;
    user_config_cb = config_cb;
    return ESP_OK;
}

void mqtt_manager_start(void)
{
    system_config_t cfg;
    config_load(&cfg);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = cfg.mqtt_broker_uri,
        .credentials.client_id = MQTT_CLIENT_ID,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT manager started (MQTT 5.0)");
}

static void publish_json(const char *topic, const char *json)
{
    if (mqtt_client && connected) {
        esp_mqtt_client_publish(mqtt_client, topic, json, 0, 0, 0);
    }
}

void mqtt_manager_publish_status(const char *state_str, uint32_t position)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "state", state_str);
    cJSON_AddNumberToObject(root, "position", position);
    char *json = cJSON_PrintUnformatted(root);
    publish_json(TOPIC_STATUS, json);
    free(json);
    cJSON_Delete(root);
}

void mqtt_manager_publish_event(const char *event_str)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", event_str);
    cJSON_AddNumberToObject(root, "ts", esp_timer_get_time() / 1000);
    char *json = cJSON_PrintUnformatted(root);
    publish_json(TOPIC_EVENT, json);
    free(json);
    cJSON_Delete(root);
}

void mqtt_manager_publish_telemetry(uint32_t position, int32_t speed)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "position", position);
    cJSON_AddNumberToObject(root, "speed", speed);
    char *json = cJSON_PrintUnformatted(root);
    publish_json(TOPIC_TELEMETRY, json);
    free(json);
    cJSON_Delete(root);
}

void mqtt_manager_publish_config(const system_config_t *cfg)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ftc_mode",
        cfg->ftc_mode == FTC_MODE_STOP    ? "stop" :
        cfg->ftc_mode == FTC_MODE_RESUME   ? "resume" : "reverse");
    cJSON_AddNumberToObject(root, "motor_speed", cfg->motor_speed);
    cJSON_AddBoolToObject(root, "encoder_enabled", cfg->encoder_enabled);
    cJSON_AddStringToObject(root, "mqtt_broker", cfg->mqtt_broker_uri);
    cJSON_AddNumberToObject(root, "calibration_max_pulses", cfg->calibration_max_pulses);
    char *json = cJSON_PrintUnformatted(root);
    publish_json(TOPIC_CONFIG, json);
    free(json);
    cJSON_Delete(root);
}

bool mqtt_manager_is_connected(void)
{
    return connected;
}
