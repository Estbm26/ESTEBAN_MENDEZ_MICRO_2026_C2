#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stdbool.h>

void encoder_init(void);
void encoder_enable(bool enable);
bool encoder_is_enabled(void);
int32_t encoder_get_pulses(void);
void encoder_reset(void);
int32_t encoder_get_speed(void);

#endif /* ENCODER_H */
