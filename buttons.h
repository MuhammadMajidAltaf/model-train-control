#ifndef BUTTONS_H
#define BUTTONS_H

#include <xgpio.h>

#define DEBOUNCE_INTERVAL 5

// GPIO button interrupt handler
void button_handler( void* dptr );

// timer_3-1 debounce interrupt handler
void debounce_handler( void* dptr );

// Xgpio initialization
XStatus init_buttons( XGpio* instance );

#endif // !BUTTONS_H
