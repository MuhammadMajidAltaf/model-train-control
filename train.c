/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * train.c contains:
 * - The I2C message receiver train_thread
 * - Several functions for handling data received over I2C and
 *   running state machines.
 *
\****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "my_debug.h"
#include "train.h"
#include "i2c.h"
#include "wall_lcd.h"
#include "messages.h"
#include "main.h"

extern message_qs thread_qs;

static uint8_t loconet_speed[] = {0,14,28,42,56,69,83,97,111,125};

enum interface_mode_type train_state = TRAIN_MODE;

#define NUM_TRAINS 2
train_data* active_train;
train_data trains[NUM_TRAINS];

#define NUM_SWITCHES 2
switch_data* active_switch;
switch_data switches[NUM_SWITCHES];

// Thread for most major processing of sensor data
// and train control
//  - dptr is unused
void* train_thread( void* dptr )
{
   init_trains();
   init_switches();

   wall_lcd_msg out_msg;
   out_msg.type = LCD_DISPLAY_TRAIN;
   out_msg.train = *active_train;
   msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );

   train_msg in_msg;
	while( 1 )
	{
		msgrcv_em( thread_qs.train_thread, &in_msg, sizeof(in_msg),
                 "train_thread recv fail" );
      DBG_LOG_ENTRY( "train_thread start", 0 );

		switch( in_msg.type )
		{
      case SENSOR_READ:
         DBG_LOG_ENTRY( "sensor read msg", 0 );
         process_sensor_read( in_msg.sensor_msg );
         break;

      case LOCONET_DISCONNECTED:
      case LCD_DISCONNECTED:
      case SENSOR_DISCONNECTED:
         trains_stop_all();
         break;

      case LOCONET_RECONNECTED:
      case LCD_RECONNECTED:
      case SENSOR_RECONNECTED:
      {
         train_state = TRAIN_MODE;
         active_train = &trains[0];
         resend_train_data();
         wall_lcd_msg out_msg;
         out_msg.type = LCD_CHANGE_STATE;
         out_msg.new_state = train_state;
         msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
         out_msg.type = LCD_DISPLAY_TRAIN;
         out_msg.train = *active_train;
         msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
         break;
      }

      default:
         DBG_LOG_ENTRY( "Bad msg type", in_msg.type );
         break;
		}
      DBG_LOG_ENTRY( "train_thread end", 0 );
	}

   return dptr;
}

void process_sensor_read( sensor_read_msg sensor_msg )
{
#ifdef MILESTONE_4
   wall_lcd_msg out_msg;
#endif

   // Tell the LocoNet PIC what to do
   if( sensor_msg.type == SENSOR_DATA )
   {
      process_sensor_data( sensor_msg.data );
      return;
   }

   // TODO: Handle sensor error properly
#ifdef MILESTONE_4
   out_msg.type = sensor_msg.type;
   out_msg.sensor_data = sensor_msg.data;
   switch( sensor_msg.type )
   {
   case SENSOR_DATA:
      out_msg.sensor_data = sensor_msg.data;
      break;
   case SENSOR_ERROR:
      out_msg.sensor_error = sensor_msg.error;
      break;
   case SENSOR_NO_DATA:
      break;
   default:
      DBG_LOG_ENTRY( "bad sensor data type", sensor_msg.type );
      break;
   }

   msgsnd_em( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg),
              "process_sensor_read send" );
#endif
}

void init_trains()
{
   active_train = &trains[0];

   int i;
   for( i = 0; i < NUM_TRAINS; ++i )
   {
      trains[i].id = i;
      trains[i].dir = TRAIN_FORWARD;
      trains[i].speed = MIN_SPEED;
   }
}

void init_switches()
{
   active_switch = &switches[0];

   int i;
   for( i = 0; i < NUM_SWITCHES; ++i )
   {
      switches[i].id = i;
      switches[i].state = SWITCH_CLOSED;
   }
}

void process_sensor_data( uint8_t buttons )
{
   // Only the lower nybble should be set
   if( buttons != (buttons&0x0F) )
   {
      DBG_LOG_ENTRY( "Bad Button Data", 0 );
      return;
   }

   if( buttons == 0x0F )
   {
      trains_stop_all();
      if( train_state == TRAIN_MODE )
      {
         wall_lcd_msg out_msg;
         out_msg.type = LCD_DISPLAY_TRAIN;
         out_msg.train = *active_train;
         msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
      }
      return;
   }
      
   // Only one bit should be set
   if( (buttons & (buttons-1)) != 0 || buttons == 0 )
      return;

   switch( train_state )
   {
   case TRAIN_MODE:
      if( buttons == BUTTON_1 )
      {
         if( active_train != &trains[NUM_TRAINS-1] )
            active_train += 1;
         else
         {
            active_train = &trains[0];
            ++train_state;
         }
      }
      else
         process_train_input( buttons );
      break;

   case SWITCH_MODE:
      if( buttons == BUTTON_1 )
      {
         if( active_switch != &switches[NUM_SWITCHES-1] )
            active_switch += 1;
         else
         {
            active_switch = &switches[0];
            ++train_state;
         }
      }
      else
         process_switch_input( buttons );
      break;

   case HELP_MODE:
      if( buttons == BUTTON_1 )
         ++train_state;
      else
         process_help_input( buttons );
      break;

   case DEMO_MODE:
      if( buttons == BUTTON_1 )
      {
         train_state = TRAIN_MODE;
         resend_train_data();
      }
      else
         process_demo_input( buttons );
      break;

   default:
      train_state = TRAIN_MODE;
      break;
   }

   if( buttons == BUTTON_1 ) 
   {
      if( active_train == &trains[0] && active_switch == &switches[0] )
      {
         wall_lcd_msg out_msg;
         out_msg.type = LCD_CHANGE_STATE;
         out_msg.new_state = train_state;
         msgsnd_em( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg),
                    "process_sensor_data lcd send failed" );
      }
      if( train_state == TRAIN_MODE )
      {
         wall_lcd_msg out_msg = { LCD_DISPLAY_TRAIN };
         out_msg.train = *active_train;
         msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
      }
      else if( train_state == SWITCH_MODE )
      {
         wall_lcd_msg out_msg = { LCD_DISPLAY_SWITCH };
         out_msg.t_switch = *active_switch;
         msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
      }
   }
}

void process_train_input( uint8_t buttons )
{
   switch( buttons )
   {
   case TRAIN_SPEED_DEC:
      if( active_train->speed == 0 )
      {
         active_train->dir = TRAIN_BACKWARD;
         send_train_cmd( LOCONET_SET_DIRECTION );
      }
      if( active_train->speed != -MAX_SPEED )
      {
         active_train->speed -= 1;
         send_train_cmd( LOCONET_SET_SPEED );
      }
      break;

   case TRAIN_STOP:
      active_train->speed = 0;
      send_train_cmd( LOCONET_SET_SPEED );
      break;

   case TRAIN_SPEED_INC:
      if( active_train->speed == 0 )
      {
         active_train->dir = TRAIN_FORWARD;
         send_train_cmd( LOCONET_SET_DIRECTION );
      }
      if( active_train->speed != MAX_SPEED )
      {
         active_train->speed += 1;
         send_train_cmd( LOCONET_SET_SPEED );
      }
      break;

   default:
      DBG_LOG_ENTRY( "Bad train button!", buttons );
      break;
   }

   wall_lcd_msg lcd_msg;
   lcd_msg.type = LCD_DISPLAY_TRAIN;
   lcd_msg.train = *active_train;
   msgsnd_e( thread_qs.wall_lcd_thread, &lcd_msg, sizeof(lcd_msg) );
}

#if ENABLE_LOCONET_SEND
void send_train_cmd( enum train_cmd_type type )
{
   i2c_msg out_msg;
   size_t length = offsetof(i2c_msg,train_cmd) + sizeof(train_cmd_msg);

   out_msg.type = I2C_SEND;
   out_msg.address = LOCONET_PIC_ADDR;
   out_msg.train_cmd.type = type;

   if( type == LOCONET_SWITCH )
      out_msg.train_cmd.id = active_switch->id;
   else
      out_msg.train_cmd.id = active_train->id;

   switch( type )
   {
   case LOCONET_STOP:
      active_train->speed = 0;
   case LOCONET_SET_SPEED:
      out_msg.train_cmd.speed = loconet_speed[abs(active_train->speed)];
      break;
   case LOCONET_SWITCH:
      out_msg.train_cmd.thrown = active_switch->state;
      break;
   case LOCONET_SET_DIRECTION:
      out_msg.train_cmd.dir = active_train->dir;
      break;
   case LOCONET_DEMO:
      out_msg.train_cmd.speed = 0;
   }

   msgsnd_em( thread_qs.i2c_thread, &out_msg, length,
              "send_train_cmd send failed" );
   DBG_LOG_ENTRY( "sent train msg", 0 );
}
#endif

void process_switch_input( uint8_t buttons )
{
   int changed = 0;
   switch( buttons )
   {
   case SWITCH_THROW:
      if( active_switch->state != SWITCH_THROWN )
      {
         active_switch->state = SWITCH_THROWN;
         send_train_cmd( LOCONET_SWITCH );
         changed = 1;
      }
      break;
   case SWITCH_CLOSE:
      if( active_switch->state != SWITCH_CLOSED )
      {
         active_switch->state = SWITCH_CLOSED;
         send_train_cmd( LOCONET_SWITCH );
         changed = 1;
      }
      break;
   }

   if( changed )
   {
      wall_lcd_msg out_msg;
      out_msg.type = LCD_DISPLAY_SWITCH;
      out_msg.t_switch = *active_switch;
      msgsnd_em( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg),
                 "process_switch_input send failed" );
   }
}

void process_help_input( uint8_t buttons )
{
   wall_lcd_msg out_msg;

   switch( buttons )
   {
   case HELP_SCROLL_UP:
      out_msg.type = LCD_HELP_UP;
      break;
   case HELP_SCROLL_DOWN:
      out_msg.type = LCD_HELP_DOWN;
      break;
   }
   msgsnd_e( thread_qs.wall_lcd_thread, &out_msg, sizeof(out_msg) );
}

void process_demo_input( uint8_t buttons )
{
   wall_lcd_msg demo_msg;
   switch( buttons )
   {
   case DEMO_START:
      send_train_cmd( LOCONET_DEMO );
      demo_msg.type = LCD_DEMO_START;
      break;
   case DEMO_STOP:
   {
      resend_train_data();

      int i;
      switch_data* s = active_switch;
      for( i = 0; i < NUM_SWITCHES; ++i )
      {
         active_switch = &switches[i];
         send_train_cmd( LOCONET_SWITCH );
      }
      active_switch = s;

      demo_msg.type = LCD_DEMO_STOP;
      break;
   }

   default:
      break;
   }
   msgsnd_e( thread_qs.wall_lcd_thread, &demo_msg, sizeof(demo_msg) );
}

void trains_stop_all()
{
   train_data* temp = active_train;

   int i;
   for( i = 0; i < NUM_TRAINS; ++i )
   {
      active_train = &trains[i];
      send_train_cmd( LOCONET_STOP );
   }
   
   active_train = temp;
}

void resend_train_data()
{
   train_data* temp = active_train;
   int i;
   for( i = 0; i < NUM_TRAINS; ++i )
   {
      active_train = &trains[i];
      send_train_cmd( LOCONET_SET_DIRECTION );
      send_train_cmd( LOCONET_SET_SPEED );
   }
   active_train = temp;
}

void loconet_handle_i2c( enum i2c_status status )
{
   static enum i2c_status state = I2C_SEND_COMPLETE;
   if( status == state )
      return;

   train_msg out_msg;

   switch( status )
   {
   case I2C_SLAVE_NO_ACK:
      out_msg.type = LOCONET_DISCONNECTED;
      lcd_display_error( "LocoNet Disconnected" );
      break;
   case I2C_SEND_COMPLETE:
      out_msg.type = LOCONET_RECONNECTED;
      break;
   default:
      break;
   }

   msgsnd_e( thread_qs.train_thread, &out_msg, sizeof(out_msg) );

   state = status;
}
