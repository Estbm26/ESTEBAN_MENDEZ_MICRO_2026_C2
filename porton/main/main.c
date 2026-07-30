#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "config.h"
#include "state_machine.h"
#include "motor.h"
#include "encoder.h"
#include "display.h"
#include "io.h"
#include "mqtt_manager.h"

static const char *TAG = "MAIN";

static state_machine_t sm;
static system_config_t system_cfg;
static uint32_t main_tick_ms = 0;
static uint32_t last_telemetry_ms = 0;
static uint32_t last_display_ms = 0;

static void on_mqtt_cmd(porton_event_t event);
static void on_mqtt_config(system_config_t *cfg);
static void handle_ftc_behavior(void);

/* Callback cuando se recibe un comando por MQTT */
static void on_mqtt_cmd(porton_event_t event)
{
    ESP_LOGI(TAG, "MQTT command received: %s", sm_event_str(event));
    sm_process(&sm, event);
}

/* Callback cuando se recibe una configuracion por MQTT */
static void on_mqtt_config(system_config_t *new_cfg)
{
    ESP_LOGI(TAG, "MQTT config received, reloading");
    memcpy(&system_cfg, new_cfg, sizeof(system_config_t));
    mqtt_manager_publish_config(&system_cfg);
}

/* Manejo del comportamiento FTC segun config */
static void handle_ftc_behavior(void)
{
    if (sm.current_state != STATE_FTC_BLOCKED) return;

    switch (system_cfg.ftc_mode) {
        case FTC_MODE_STOP:
            sm_process(&sm, EV_CMD_STOP);
            break;
        case FTC_MODE_RESUME:
            break;
        case FTC_MODE_REVERSE:
            if (sm.ftc_was_blocked) {
                porton_event_t rev_event = (sm.last_dir == DIR_OPENING) ? EV_CMD_CLOSE : EV_CMD_OPEN;
                sm_process(&sm, rev_event);
                sm.ftc_was_blocked = false;
            }
            break;
    }
}



/* Timer periodico principal - corazon del sistema */
static void main_timer_callback(void *arg)
{
    main_tick_ms += MAIN_TIMER_MS;

    /* 1. Leer entradas con debounce */
    porton_event_t input_event = io_scan_inputs(main_tick_ms);

    /* 2. Procesar eventos de entrada en maquina de estados */
    if (input_event != EV_NONE) {
        /* FTC_CLEARED en modo RESUME: reanudar directamente segun ultima direccion */
        if (input_event == EV_FTC_CLEARED &&
            system_cfg.ftc_mode == FTC_MODE_RESUME &&
            sm.current_state == STATE_FTC_BLOCKED) {
            porton_event_t resume_ev = (sm.last_dir == DIR_OPENING) ? EV_CMD_OPEN : EV_CMD_CLOSE;
            sm_process(&sm, resume_ev);
        } else {
            sm_process(&sm, input_event);
        }
    }

    /* 3. Manejo de FTC segun config */
    handle_ftc_behavior();

    /* 4. Control del motor segun estado */
    if (sm.current_state == STATE_OPENING) {
        motor_set(MOTOR_OPEN, system_cfg.motor_speed);
    } else if (sm.current_state == STATE_CLOSING) {
        motor_set(MOTOR_CLOSE, system_cfg.motor_speed);
    } else {
        motor_stop();
    }

    /* 5. Actualizar posicion si hay encoder activo y motor moviendo */
    if (system_cfg.encoder_enabled && sm_is_moving(sm.current_state)) {
        sm.position_pulses = encoder_get_pulses();
    }

    /* 6. Manejo de RGB segun estado */
    rgb_state_t rgb;
    if (sm.current_state == STATE_IDLE_CLOSED) {
        rgb = RGB_CLOSED;
    } else if (sm.current_state == STATE_IDLE_OPEN) {
        rgb = RGB_OPEN;
    } else if (sm.current_state == STATE_OPENING || sm.current_state == STATE_CLOSING) {
        rgb = RGB_MOVING;
    } else if (sm.current_state == STATE_ERROR || sm.current_state == STATE_CALIBRATION) {
        rgb = RGB_ERROR;
    } else if (sm.current_state == STATE_FTC_BLOCKED) {
        rgb = RGB_ERROR;
    } else {
        rgb = RGB_OFF;
    }
    io_set_rgb(rgb, main_tick_ms);

    /* 7. Buzzer patterns */
    if (sm.current_state == STATE_ERROR) {
        if ((main_tick_ms / 1000) % 2 == 0) io_buzzer_on();
        else io_buzzer_off();
    } else if (sm.current_state == STATE_FTC_BLOCKED) {
        if ((main_tick_ms / 500) % 2 == 0) io_buzzer_on();
        else io_buzzer_off();
    } else if (!sm_is_moving(sm.current_state)) {
        io_buzzer_off();
    }
    io_tick(main_tick_ms);

    /* 8. Publicar telemetria periodica */
    if (main_tick_ms - last_telemetry_ms >= TELEMETRY_INTERVAL_MS) {
        last_telemetry_ms = main_tick_ms;
        int32_t speed = 0;
        if (system_cfg.encoder_enabled) {
            speed = encoder_get_speed();
        }
        mqtt_manager_publish_telemetry(sm.position_pulses, speed);
        mqtt_manager_publish_status(sm_state_str(sm.current_state), sm.position_pulses);
    }

    /* 9. Actualizar display periodico */
    if (main_tick_ms - last_display_ms >= DISPLAY_UPDATE_MS) {
        last_display_ms = main_tick_ms;
        display_update(&sm);
    }

    /* 10. Incrementar tiempo en estado */
    sm.state_time_ms += MAIN_TIMER_MS;
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  PORTON - Garage Door Controller v%s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "========================================");

    /* Inicializar NVS y cargar config */
    config_init();
    config_load(&system_cfg);
    config_print(&system_cfg);

    /* Inicializar modulos de hardware */
    io_init();
    motor_init();
    if (system_cfg.encoder_enabled) {
        encoder_init();
    }
    display_init();
    display_show_message("Porton v" FIRMWARE_VERSION, "Iniciando...");

    /* Inicializar maquina de estados */
    sm_init(&sm);

    /* Inicializar MQTT */
    mqtt_manager_init(on_mqtt_cmd, on_mqtt_config);
    mqtt_manager_start();

    /* Indicar INIT completo */
    sm_process(&sm, EV_INIT_COMPLETE);

    /* Publicar estado inicial */
    mqtt_manager_publish_status(sm_state_str(sm.current_state), 0);
    mqtt_manager_publish_config(&system_cfg);

    /* Crear timer periodico */
    const esp_timer_create_args_t timer_args = {
        .callback = main_timer_callback,
        .name = "main_timer"
    };
    esp_timer_handle_t main_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &main_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(main_timer, MAIN_TIMER_MS * 1000));

    ESP_LOGI(TAG, "System ready, state: %s", sm_state_str(sm.current_state));

    /* Loop principal solo para logs (el timer hace el trabajo pesado) */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGD(TAG, "State: %s, Position: %lu, Tick: %lu",
                 sm_state_str(sm.current_state),
                 (unsigned long)sm.position_pulses,
                 (unsigned long)main_tick_ms);
    }
}
