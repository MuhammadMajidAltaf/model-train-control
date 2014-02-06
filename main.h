/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * main.h includes:
 * - Globally used enums and macros
 *
\****************************************************************************/
#ifndef MAIN_H
#define MAIN_H

#include <xparameters.h>
#include <sys/types.h>

#if 1
#define MY_DEBUG
#endif

#define ENABLE_SENSOR_TIMER 0
#define SENSOR_INTERVAL 500
#define SENSOR_ACK_INTERVAL 3000

#define ENABLE_BUTTON_INTR 1
#define ENABLE_LOCONET_SEND 0

#ifdef MY_DEBUG
#define ENABLE_DEBUG_TIMER 0
#define WATCHDOG_TIMEOUT 30   // s
#endif

#define ENABLE_TRAIN_THREAD 1
#define TRAIN_THREAD_PRIORITY 6

#define ENABLE_LCD_THREAD 1
#define LCD_THREAD_PRIORITY 6

#define ENABLE_BUTTON_THREAD 1
#define BUTTON_THREAD_PRIORITY 4

#define ENABLE_IIC_THREAD 1
#define IIC_THREAD_PRIORITY 5

// Place message queue ids in this global struct
typedef struct 
{
   int button_thread;
   int train_thread;
   int wall_lcd_thread;
   int i2c_thread;
   int i2c_out_status;
} message_qs;

// Spartan board push button masks
enum S3E_PUSH_BUTTONS
{
   WEST_BUTTON  = 0x200,
   EAST_BUTTON  = 0x100,
   NORTH_BUTTON = 0x080,
   SWITCH_3     = 0x008,
   SWITCH_2     = 0x004,
   SWITCH_1     = 0x002,
   SWITCH_0     = 0x001
};
#define BUTTONS_MASK   0x380
#define SWITCHES_MASK  0x00F
#define NO_ROTARY_MASK (BUTTONS_MASK|SWITCHES_MASK)

// Make new message queue key ids in here
enum message_queue_keys
{
   BUTTON_Q_1 = 0x51,
   LCD_Q_1,
   I2C_STATUS_Q,
   I2C_ISTATUS_Q,
   I2C_RECV_Q,
   I2C_INCOMING_Q,
   WALL_LCD_Q,
   TRAIN_Q        
};

// I2C address of the attached peripherals
enum i2c_address
{
   INVALID_ADDR     = 0x00,
   SELF_ADDR        = 0x4F,
   WALL_LCD_ADDR    = (0x50>>1), //0x28
   LOCONET_PIC_ADDR = 0x75, 
   SENSOR_PIC_ADDR  = 0x51
};
#define NUM_SLAVES 3

// Masks for each cap. sensor button
enum sensor_button_mask
{
   BUTTON_1 = 0x8,
   BUTTON_2 = 0x4,
   BUTTON_3 = 0x2,
   BUTTON_4 = 0x1
};

enum timer_3_num
{
   SENSOR_TIMER = 0,
   DEBOUNCE_TIMER = 1
};

// Use simple memory-mapped interface
#define LEDS     ((volatile unsigned char* const)(XPAR_LEDS_8BIT_BASEADDR+3))
#define SWITCHES ((volatile const unsigned int* const)(XPAR_BUTTONS_KNOB_SWITCHES_BASEADDR))

// Use this macro to check static compile time assertions
#define STATIC_ASSERT(pred) switch(0){case 0:case pred:;}

// Provides simpler usage and automatic debug error logging
ssize_t msgrcv_em( int msqid, void* msgp, size_t nbytes, const char* message );
ssize_t msgrcv_e( int msqid, void* msgp, size_t nbytes );
int     msgsnd_em( int msqid, const void* msgp, size_t msgsz, const char* message );
int     msgsnd_e( int msqid, const void* msgp, size_t msgsz );
int     msgsnd_int( int msqid, const void* msgp, size_t msgsz, const char* message );

#endif // !MAIN_H
