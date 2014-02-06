/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * Modifications:
 * - Split functions up to allow for discrete use of both counters within
 *   each timer device.
 * - Modified to have simpler usage
 * - Made thread safe
 *
\****************************************************************************/
#include <xmk.h>
#include <os_config.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/intr.h>

#include "timer.h"
#include "main.h"

pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_counter( XTmrCtr* instance, int timer_num, int ms )
{
   XTmrCtr_SetResetValue( instance, timer_num, (SYSTMR_CLK_FREQ/1000)*ms );
   XTmrCtr_SetOptions( instance, 
                       timer_num,
                       XTC_DOWN_COUNT_OPTION
                       | XTC_INT_MODE_OPTION
                       | XTC_AUTO_RELOAD_OPTION );
}

// Initialize a timer without starting it
XStatus init_timer( XTmrCtr *instance, int device_id, int int_id, 
                    XTmrCtr_Handler func, void* dptr )
{
   pthread_mutex_lock( &timer_mutex );
   XStatus status;

   // initialize the timer driver 
   status = XTmrCtr_Initialize( instance, device_id );
   if (status != XST_SUCCESS)
   {
      pthread_mutex_unlock( &timer_mutex );
      return status;
   }

   // connect the interrupt handler to the driver
   XTmrCtr_SetHandler( instance, func, dptr );

   // enable the interrupt and connect the driver to the interrupt device
   status = register_int_handler( int_id, XTmrCtr_InterruptHandler, instance );

   enable_interrupt(int_id);

   pthread_mutex_unlock( &timer_mutex );

   return status;
}
