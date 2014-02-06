/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * train.h contains enums and structs defining the i2c interface to the
 * LocoNet board
 *
\****************************************************************************/
#ifndef TRAIN_H
#define TRAIN_H

#include <stdint.h>

#include "main.h"
#include "sensor.h"
#include "i2c.h"

// Type of message passed into the train thread queue
enum train_msg_type
{
   SENSOR_READ = 0x02,
   SENSOR_DISCONNECTED = 0x10,
   SENSOR_RECONNECTED,
   LCD_DISCONNECTED,
   LCD_RECONNECTED,
   LOCONET_DISCONNECTED,
   LOCONET_RECONNECTED
};

// All messages sent to the train_thread must use this struct
typedef struct
{
   enum train_msg_type type;
   union {
      sensor_read_msg sensor_msg;
   };
} train_msg;

enum train_dir
{
   TRAIN_FORWARD  = 0x00,
   TRAIN_BACKWARD = 0x20
};

enum switch_state
{
   SWITCH_THROWN = 0x10,
   SWITCH_CLOSED = 0x30
};

typedef int8_t train_speed;
#define MAX_SPEED 9
#define MIN_SPEED 0

// Data structure holding state of a train
typedef struct
{
   uint8_t        id;
   train_speed    speed;
   enum train_dir dir;
} train_data;

typedef struct
{
   uint8_t           id;
   enum switch_state state;
} switch_data;


// Current state of the user interface
enum interface_mode_type
{
   TRAIN_MODE,
   SWITCH_MODE,
   HELP_MODE,
   DEMO_MODE,
   SENSOR_MODE
};

// Map each button to a function
enum train_mode_buttons
{
   TRAIN_SPEED_DEC = BUTTON_2,
   TRAIN_STOP      = BUTTON_3,
   TRAIN_SPEED_INC = BUTTON_4
};

enum switch_mode_buttons
{
   SWITCH_THROW = BUTTON_2,
   SWITCH_CLOSE = BUTTON_4
};

enum help_mode_buttons
{
   HELP_SCROLL_UP   = BUTTON_2,
   HELP_SCROLL_DOWN = BUTTON_4
};

enum demo_mode_buttons
{
   DEMO_START = BUTTON_2,
   DEMO_STOP  = BUTTON_4
};

// A LocoNet command to send out
enum train_cmd_type
{
   LOCONET_SET_SPEED     = 0xF1,
   LOCONET_STOP          = 0xF2,
   LOCONET_SWITCH        = 0xF3,
   LOCONET_SET_DIRECTION = 0xF7,
   LOCONET_DEMO          = 0x04
};

void* train_thread( void* dptr ) __attribute__ ((section(".high_access")));
void  process_sensor_read( sensor_read_msg in_msg ) __attribute__ ((section(".high_access")));
void  init_trains();
void  init_switches();
void  process_sensor_data( uint8_t buttons );

// One processing function for each interface mode
void  process_train_input( uint8_t buttons );
void  process_switch_input( uint8_t buttons );
void  process_help_input( uint8_t buttons );
void  process_demo_input( uint8_t buttons );

// Tell all the trains to stop (e.g. is UI becomes disconnected)
void trains_stop_all();
void resend_train_data();
void emergency_stop();
void loconet_handle_i2c( enum i2c_status status );

#if ENABLE_LOCONET_SEND
void  send_train_cmd( enum train_cmd_type type );
#else
#define send_train_cmd(A);
#endif

/************************************************************
 * An compile error about duplicate cases originating from
 * this function means that the corresponding assertion failed. 
 * 
 * The enums and structs must be properly sized in order to
 * work with the PICs over i2c.
 *
 * The code must be compiled using the mb-gcc switch -fshort-enums
 * in order for the enums size to be reduced to a byte
 *************************************************************/
extern inline void train_c_assertions()
{
   STATIC_ASSERT( sizeof(enum train_msg_type) == 1 );
   STATIC_ASSERT( sizeof(enum train_dir) == 1 );
   STATIC_ASSERT( sizeof(train_speed) == 1 );
   STATIC_ASSERT( sizeof(enum train_cmd_type) == 1 );
}

#endif // !TRAIN_H
