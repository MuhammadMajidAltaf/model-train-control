/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * Modifications:
 * - Added timer based debouncing of buttons and switches
 *
\****************************************************************************/
#include <xmk.h>
#include <xgpio.h>
#include <sys/intr.h>

#include "buttons.h"
#include "timer.h"
#include "main.h"
#include "my_debug.h"

extern message_qs thread_qs;
extern XTmrCtr timer_3;

// Note: The code does not currently have any debouncing of buttons.  
// Oh, wait. Yeah, it does.
// This code seems like it should work but I could only test it with
// a board that had faulty buttons. Switches seemed to work fine.
int button_change;
int prev_buttons;

void button_handler( void *dptr )
{
   XGpio* button_instance = (XGpio*)dptr;

   int data = XGpio_DiscreteRead( button_instance, 1 );

   // Find the differences and store the value
   button_change = prev_buttons ^ data;
   prev_buttons = data;

   // I only care if the changed values are now 1
   if( (button_change & data) == 0 )
   {
      XGpio_InterruptClear( button_instance, XGPIO_IR_CH1_MASK );
      return;
   }

   // Don't let the bounces interrupt again
   XGpio_InterruptDisable( button_instance, XGPIO_IR_CH1_MASK );
   XTmrCtr_Start( &timer_3, DEBOUNCE_TIMER );
}

void debounce_handler( void* dptr )
{
   XGpio* button_instance = (XGpio*)dptr;

   int data = XGpio_DiscreteRead( button_instance, 1 );

   // If the changed buttons are still 1
   if( button_change & data )
   {
      msgsnd_int( thread_qs.button_thread, &data, sizeof(data),
                  "debounce_handler send failed" );
   }

   // Store the value again
   prev_buttons = data;

   // clear and re-enable the interrupt
   XGpio_InterruptEnable( button_instance, XGPIO_IR_CH1_MASK );
   XGpio_InterruptClear( button_instance, XGPIO_IR_CH1_MASK );
}

///////////////////////////////////////////
// initialize the button device and interrupt handler
///////////////////////////////////////////
XStatus init_buttons( XGpio* instance )
{
   XStatus status;

   // initialize the driver
   status = XGpio_Initialize( instance, XPAR_BUTTONS_KNOB_SWITCHES_DEVICE_ID );
   if( status != XST_SUCCESS )
      return status;

   // set channel 1 to read
   XGpio_SetDataDirection( instance, DEBOUNCE_TIMER, ~0 );

   // connect the interrupt handler
   status = register_int_handler( XPAR_XPS_INTC_0_BUTTONS_KNOB_SWITCHES_IP2INTC_IRPT_INTR,
                                  button_handler,
                                  instance );
   if (status != XST_SUCCESS)
      return status;

   init_counter( &timer_3, DEBOUNCE_TIMER, DEBOUNCE_INTERVAL );

   // Tell the RTOS to allow this interrupt
   enable_interrupt( XPAR_XPS_INTC_0_BUTTONS_KNOB_SWITCHES_IP2INTC_IRPT_INTR );

   // tell the device to allow this interrupt on this channel
   XGpio_InterruptEnable( instance, XGPIO_IR_CH1_MASK );
   XGpio_InterruptGlobalEnable( instance );

   return status;
}
