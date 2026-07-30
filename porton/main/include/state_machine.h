#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>

/* Estados del portón */
typedef enum {
    STATE_INIT = 0,
    STATE_IDLE_CLOSED,
    STATE_IDLE_OPEN,
    STATE_IDLE_PARTIAL,
    STATE_OPENING,
    STATE_CLOSING,
    STATE_STOPPED,
    STATE_FTC_BLOCKED,
    STATE_ERROR,
    STATE_CALIBRATION,
} porton_state_t;

/* Eventos que disparan transiciones */
typedef enum {
    EV_NONE = 0,
    EV_CMD_OPEN,
    EV_CMD_CLOSE,
    EV_CMD_STOP,
    EV_CMD_TOGGLE,
    EV_CMD_CALIBRATE,
    EV_LS_CLOSED_PRESSED,
    EV_LS_OPEN_PRESSED,
    EV_LS_BOTH_PRESSED,
    EV_FTC_TRIGGERED,
    EV_FTC_CLEARED,
    EV_INIT_COMPLETE,
    EV_ERROR_CLEAR,
    EV_MOTOR_TIMEOUT,
} porton_event_t;

typedef enum {
    DIR_NONE = 0,
    DIR_OPENING,
    DIR_CLOSING
} last_dir_t;

typedef struct {
    porton_state_t current_state;
    uint32_t state_time_ms;
    uint32_t position_pulses;
    uint32_t target_pulses;
    bool ftc_was_blocked;
    last_dir_t last_dir;
} state_machine_t;

void sm_init(state_machine_t *sm);
void sm_process(state_machine_t *sm, porton_event_t event);
const char *sm_state_str(porton_state_t state);
const char *sm_event_str(porton_event_t event);
bool sm_is_moving(porton_state_t state);
bool sm_is_idle(porton_state_t state);

#endif /* STATE_MACHINE_H */
