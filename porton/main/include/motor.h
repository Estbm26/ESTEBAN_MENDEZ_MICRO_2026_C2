#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_OPEN,
    MOTOR_CLOSE
} motor_direction_t;

void motor_init(void);
void motor_set(motor_direction_t dir, uint8_t speed);
void motor_stop(void);
motor_direction_t motor_get_direction(void);
uint8_t motor_get_speed(void);

#endif /* MOTOR_H */
