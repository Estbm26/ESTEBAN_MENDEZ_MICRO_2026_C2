#include "state_machine.h"
#include "esp_log.h"

static const char *TAG = "STATE_MACHINE";

void sm_init(state_machine_t *sm)
{
    sm->current_state = STATE_INIT;
    sm->state_time_ms = 0;
    sm->position_pulses = 0;
    sm->target_pulses = 0;
    sm->ftc_was_blocked = false;
    sm->last_dir = DIR_NONE;
    ESP_LOGI(TAG, "State machine initialized to INIT");
}

void sm_process(state_machine_t *sm, porton_event_t event)
{
    porton_state_t next = sm->current_state;

    switch (sm->current_state) {

        case STATE_INIT:
            if (event == EV_INIT_COMPLETE) {
                /* Inicio normal -> IDLE_CLOSED; la calibración inicial se maneja aparte */
                next = STATE_IDLE_CLOSED;
                ESP_LOGI(TAG, "INIT -> IDLE_CLOSED");
            }
            break;

        case STATE_IDLE_CLOSED:
            if (event == EV_CMD_OPEN || event == EV_CMD_TOGGLE) {
                next = STATE_OPENING;
                sm->last_dir = DIR_OPENING;
                ESP_LOGI(TAG, "IDLE_CLOSED -> OPENING");
            } else if (event == EV_CMD_CALIBRATE) {
                next = STATE_CALIBRATION;
                ESP_LOGI(TAG, "IDLE_CLOSED -> CALIBRATION");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "IDLE_CLOSED -> ERROR (both LS pressed)");
            }
            break;

        case STATE_IDLE_OPEN:
            if (event == EV_CMD_CLOSE || event == EV_CMD_TOGGLE) {
                next = STATE_CLOSING;
                sm->last_dir = DIR_CLOSING;
                ESP_LOGI(TAG, "IDLE_OPEN -> CLOSING");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "IDLE_OPEN -> ERROR (both LS pressed)");
            }
            break;

        case STATE_IDLE_PARTIAL:
            if (event == EV_CMD_OPEN) {
                next = STATE_OPENING;
                sm->last_dir = DIR_OPENING;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> OPENING");
            } else if (event == EV_CMD_CLOSE) {
                next = STATE_CLOSING;
                sm->last_dir = DIR_CLOSING;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> CLOSING");
            } else if (event == EV_CMD_TOGGLE) {
                /* Toggle desde parcial: prefiere ir a CLOSING (sentido más seguro) */
                next = STATE_CLOSING;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> CLOSING (toggle)");
            } else if (event == EV_LS_CLOSED_PRESSED) {
                next = STATE_IDLE_CLOSED;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> IDLE_CLOSED (LS closed)");
            } else if (event == EV_LS_OPEN_PRESSED) {
                next = STATE_IDLE_OPEN;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> IDLE_OPEN (LS open)");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "IDLE_PARTIAL -> ERROR (both LS pressed)");
            }
            break;

        case STATE_OPENING:
            if (event == EV_CMD_STOP) {
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "OPENING -> STOPPED");
            } else if (event == EV_CMD_CLOSE || event == EV_CMD_TOGGLE) {
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "OPENING -> STOPPED (cmd close/toggle)");
            } else if (event == EV_LS_OPEN_PRESSED) {
                next = STATE_IDLE_OPEN;
                ESP_LOGI(TAG, "OPENING -> IDLE_OPEN (LS open)");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "OPENING -> ERROR (both LS)");
            } else if (event == EV_FTC_TRIGGERED) {
                next = STATE_FTC_BLOCKED;
                sm->ftc_was_blocked = true;
                ESP_LOGI(TAG, "OPENING -> FTC_BLOCKED");
            } else if (event == EV_MOTOR_TIMEOUT) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "OPENING -> ERROR (timeout)");
            }
            break;

        case STATE_CLOSING:
            if (event == EV_CMD_STOP) {
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "CLOSING -> STOPPED");
            } else if (event == EV_CMD_OPEN || event == EV_CMD_TOGGLE) {
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "CLOSING -> STOPPED (cmd open/toggle)");
            } else if (event == EV_LS_CLOSED_PRESSED) {
                next = STATE_IDLE_CLOSED;
                ESP_LOGI(TAG, "CLOSING -> IDLE_CLOSED (LS closed)");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "CLOSING -> ERROR (both LS)");
            } else if (event == EV_FTC_TRIGGERED) {
                next = STATE_FTC_BLOCKED;
                sm->ftc_was_blocked = true;
                ESP_LOGI(TAG, "CLOSING -> FTC_BLOCKED");
            } else if (event == EV_MOTOR_TIMEOUT) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "CLOSING -> ERROR (timeout)");
            }
            break;

        case STATE_STOPPED:
            if (event == EV_CMD_OPEN) {
                next = STATE_OPENING;
                sm->last_dir = DIR_OPENING;
                ESP_LOGI(TAG, "STOPPED -> OPENING");
            } else if (event == EV_CMD_CLOSE) {
                next = STATE_CLOSING;
                sm->last_dir = DIR_CLOSING;
                ESP_LOGI(TAG, "STOPPED -> CLOSING");
            } else if (event == EV_CMD_TOGGLE) {
                next = STATE_OPENING;
                sm->last_dir = DIR_OPENING;
                ESP_LOGI(TAG, "STOPPED -> OPENING (toggle)");
            } else if (event == EV_LS_CLOSED_PRESSED) {
                next = STATE_IDLE_CLOSED;
                ESP_LOGI(TAG, "STOPPED -> IDLE_CLOSED");
            } else if (event == EV_LS_OPEN_PRESSED) {
                next = STATE_IDLE_OPEN;
                ESP_LOGI(TAG, "STOPPED -> IDLE_OPEN");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "STOPPED -> ERROR (both LS)");
            }
            break;

        case STATE_FTC_BLOCKED:
            if (event == EV_FTC_CLEARED) {
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "FTC_BLOCKED -> STOPPED (FTC cleared)");
            } else if (event == EV_CMD_STOP) {
                sm->ftc_was_blocked = false;
                next = STATE_STOPPED;
                ESP_LOGI(TAG, "FTC_BLOCKED -> STOPPED (cmd stop)");
            } else if (event == EV_CMD_OPEN) {
                sm->ftc_was_blocked = false;
                sm->last_dir = DIR_OPENING;
                next = STATE_OPENING;
                ESP_LOGI(TAG, "FTC_BLOCKED -> OPENING");
            } else if (event == EV_CMD_CLOSE) {
                sm->ftc_was_blocked = false;
                sm->last_dir = DIR_CLOSING;
                next = STATE_CLOSING;
                ESP_LOGI(TAG, "FTC_BLOCKED -> CLOSING");
            } else if (event == EV_LS_BOTH_PRESSED) {
                next = STATE_ERROR;
                ESP_LOGI(TAG, "FTC_BLOCKED -> ERROR (both LS)");
            }
            break;

        case STATE_ERROR:
            if (event == EV_ERROR_CLEAR) {
                next = STATE_IDLE_CLOSED;
                sm->position_pulses = 0;
                sm->target_pulses = 0;
                ESP_LOGI(TAG, "ERROR -> IDLE_CLOSED (error cleared)");
            }
            break;

        case STATE_CALIBRATION:
            if (event == EV_CMD_STOP || event == EV_ERROR_CLEAR) {
                next = STATE_IDLE_CLOSED;
                ESP_LOGI(TAG, "CALIBRATION -> IDLE_CLOSED");
            }
            break;
    }

    if (next != sm->current_state) {
        sm->state_time_ms = 0;
        sm->current_state = next;
    }
}

const char *sm_state_str(porton_state_t state)
{
    switch (state) {
        case STATE_INIT:          return "INIT";
        case STATE_IDLE_CLOSED:   return "IDLE_CLOSED";
        case STATE_IDLE_OPEN:     return "IDLE_OPEN";
        case STATE_IDLE_PARTIAL:  return "IDLE_PARTIAL";
        case STATE_OPENING:       return "OPENING";
        case STATE_CLOSING:       return "CLOSING";
        case STATE_STOPPED:       return "STOPPED";
        case STATE_FTC_BLOCKED:   return "FTC_BLOCKED";
        case STATE_ERROR:         return "ERROR";
        case STATE_CALIBRATION:   return "CALIBRATION";
        default:                  return "UNKNOWN";
    }
}

const char *sm_event_str(porton_event_t event)
{
    switch (event) {
        case EV_NONE:              return "NONE";
        case EV_CMD_OPEN:          return "CMD_OPEN";
        case EV_CMD_CLOSE:         return "CMD_CLOSE";
        case EV_CMD_STOP:          return "CMD_STOP";
        case EV_CMD_TOGGLE:        return "CMD_TOGGLE";
        case EV_CMD_CALIBRATE:     return "CMD_CALIBRATE";
        case EV_LS_CLOSED_PRESSED: return "LS_CLOSED";
        case EV_LS_OPEN_PRESSED:   return "LS_OPEN";
        case EV_LS_BOTH_PRESSED:   return "LS_BOTH";
        case EV_FTC_TRIGGERED:     return "FTC_TRIGGERED";
        case EV_FTC_CLEARED:       return "FTC_CLEARED";
        case EV_INIT_COMPLETE:     return "INIT_COMPLETE";
        case EV_ERROR_CLEAR:       return "ERROR_CLEAR";
        case EV_MOTOR_TIMEOUT:     return "MOTOR_TIMEOUT";
        default:                   return "UNKNOWN";
    }
}

bool sm_is_moving(porton_state_t state)
{
    return state == STATE_OPENING || state == STATE_CLOSING;
}

bool sm_is_idle(porton_state_t state)
{
    return state == STATE_IDLE_CLOSED || state == STATE_IDLE_OPEN || state == STATE_IDLE_PARTIAL;
}
