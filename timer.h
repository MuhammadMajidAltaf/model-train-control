#ifndef TIMER_H
#define TIMER_H

// Xilinx includes
#include <xtmrctr.h>
#include <xstatus.h>
#include <xparameters.h>

#define timer_start(A,B) XTmrCtr_Start(A,B)
#define timer_stop(A,B) XTmrCtr_Stop(A,B)

#define TIMER_2_DEVICE XPAR_XPS_TIMER_2_DEVICE_ID
#define TIMER_2_INT    XPAR_XPS_INTC_0_XPS_TIMER_2_INTERRUPT_INTR
#define TIMER_3_DEVICE XPAR_XPS_TIMER_3_DEVICE_ID
#define TIMER_3_INT    XPAR_XPS_INTC_0_XPS_TIMER_3_INTERRUPT_INTR
#define TIMER_4_DEVICE XPAR_XPS_TIMER_4_DEVICE_ID
#define TIMER_4_INT    XPAR_XPS_INTC_0_XPS_TIMER_4_INTERRUPT_INTR

// Timer API
XStatus init_timer( XTmrCtr* instance,
                    int device_id,
                    int int_id,
                    XTmrCtr_Handler func,
                    void* dptr );

void init_counter( XTmrCtr* instance,
                   int timer_num,
                   int ms );

#endif
