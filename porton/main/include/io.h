#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>
#include "state_machine.h"

/* Estados del RGB */
typedef enum {
    RGB_CLOSED,       /* Rojo fijo */
    RGB_OPEN,         /* Verde fijo */
    RGB_MOVING,       /* Amarillo parpadeando */
    RGB_ERROR,        /* Amarillo fijo */
    RGB_OFF
} rgb_state_t;

/* FTC modos */
typedef enum {
    FTC_STOP = 0,
    FTC_RESUME,
    FTC_REVERSE
} ftc_behavior_t;

void io_init(void);
void io_set_rgb(rgb_state_t state, uint32_t tick_ms);
void io_buzzer_on(void);
void io_buzzer_off(void);
bool io_buzzer_is_on(void);
bool io_get_ls_closed(void);
bool io_get_ls_open(void);
bool io_get_ftc(void);
porton_event_t io_scan_inputs(uint32_t tick_ms);
void io_tick(uint32_t tick_ms);

#endif /* IO_H */
