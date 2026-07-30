#ifndef DISPLAY_H
#define DISPLAY_H

#include "state_machine.h"

void display_init(void);
void display_update(state_machine_t *sm);
void display_show_message(const char *line1, const char *line2);
void display_clear(void);

#endif /* DISPLAY_H */
