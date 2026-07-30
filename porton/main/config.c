#include "config.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CONFIG";

static const char *NVS_NAMESPACE = "porton_cfg";

static const system_config_t DEFAULT_CONFIG = {
    .ftc_mode = FTC_MODE_STOP,
    .motor_speed = MOTOR_DEFAULT_SPEED,
    .encoder_enabled = true,
    .mqtt_broker_uri = MQTT_BROKER_URI,
    .calibration_max_pulses = 5000,
};

esp_err_t config_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase failed");
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t config_load(system_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        memcpy(cfg, &DEFAULT_CONFIG, sizeof(system_config_t));
        return ESP_OK;
    }

    uint8_t ftc_mode = DEFAULT_CONFIG.ftc_mode;
    nvs_get_u8(handle, "ftc_mode", &ftc_mode);
    cfg->ftc_mode = (ftc_mode > FTC_MODE_REVERSE) ? FTC_MODE_STOP : (ftc_mode_t)ftc_mode;

    uint8_t speed = DEFAULT_CONFIG.motor_speed;
    nvs_get_u8(handle, "motor_speed", &speed);
    cfg->motor_speed = speed;

    uint8_t enc_en = DEFAULT_CONFIG.encoder_enabled ? 1 : 0;
    nvs_get_u8(handle, "encoder_en", &enc_en);
    cfg->encoder_enabled = enc_en != 0;

    size_t len = sizeof(cfg->mqtt_broker_uri);
    if (nvs_get_str(handle, "mqtt_uri", cfg->mqtt_broker_uri, &len) != ESP_OK) {
        strncpy(cfg->mqtt_broker_uri, DEFAULT_CONFIG.mqtt_broker_uri, sizeof(cfg->mqtt_broker_uri) - 1);
    }

    int32_t cal_pulses = DEFAULT_CONFIG.calibration_max_pulses;
    nvs_get_i32(handle, "cal_pulses", &cal_pulses);
    cfg->calibration_max_pulses = cal_pulses;

    nvs_close(handle);
    return ESP_OK;
}

esp_err_t config_save(const system_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), TAG, "nvs open failed");

    nvs_set_u8(handle, "ftc_mode", (uint8_t)cfg->ftc_mode);
    nvs_set_u8(handle, "motor_speed", cfg->motor_speed);
    nvs_set_u8(handle, "encoder_en", cfg->encoder_enabled ? 1 : 0);
    nvs_set_str(handle, "mqtt_uri", cfg->mqtt_broker_uri);
    nvs_set_i32(handle, "cal_pulses", cfg->calibration_max_pulses);

    esp_err_t ret = nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

esp_err_t config_reset_defaults(void)
{
    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), TAG, "nvs open failed");
    esp_err_t ret = nvs_erase_all(handle);
    if (ret == ESP_OK) ret = nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

void config_print(const system_config_t *cfg)
{
    if (!cfg) return;
    ESP_LOGI(TAG, "FTC mode: %s",
             cfg->ftc_mode == FTC_MODE_STOP   ? "STOP" :
             cfg->ftc_mode == FTC_MODE_RESUME  ? "RESUME" :
             cfg->ftc_mode == FTC_MODE_REVERSE ? "REVERSE" : "UNKNOWN");
    ESP_LOGI(TAG, "Motor speed: %u", cfg->motor_speed);
    ESP_LOGI(TAG, "Encoder: %s", cfg->encoder_enabled ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "MQTT broker: %s", cfg->mqtt_broker_uri);
    ESP_LOGI(TAG, "Calibration max pulses: %ld", cfg->calibration_max_pulses);
}
