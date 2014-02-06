/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
\****************************************************************************/
#include <xmk.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "wall_lcd.h"
#include "i2c.h"
#include "main.h"
#include "my_debug.h"
#include "messages.h"

extern message_qs thread_qs;

// Put 7 characters in each array, one array for each mode
//  - Construct easily at jondoman.net/char-generator
//  - Use in string by using escaped hex index
//  - e.g. "\x7Z\x1" will write the 8th custom char, followed by a Z and the 2st character
#ifdef MILESTONE_4
uint8_t ms4_chars[][8] = {
{0,0,0,0,0,0,0,0},
{0x0,0x4,0x6,0x1f,0x1f,0x6,0x4,0x0},       // right arrow
{0x1f,0x11,0x11,0x11,0x11,0x11,0x11,0x1f}, // empty box
{0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f}, // filled box
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};
#endif

uint8_t train_chars[][8] = {
{0,0,0,0,0,0,0,0},
{0x18,0x1c,0xe,0x7,0x7,0xe,0x1c,0x18},     // right chevron
{0x18,0x14,0xa,0x5,0x5,0xa,0x14,0x18},     // right empty chevron
{0x3,0x7,0xe,0x1c,0x1c,0xe,0x7,0x3},       // left chevron
{0x3,0x5,0xa,0x14,0x14,0xa,0x5,0x3},       // left empty chevron
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};
uint8_t switch_chars[][8] = {
{0,0,0,0,0,0,0,0},
{0x1f,0x0,0x0,0x0,0x0,0x0,0x0,0x1f},   // untied tracks
{0x0,0x0,0x0,0x0,0x0,0x0,0x00,0x1f},    // lower topless track
{0x1f,0x0,0x0,0x0,0x0,0x0,0x0,0x0},    // upper bottomless track
{0x0,0x0,0x0,0x1,0x3,0x6,0xc,0x18},    // bottom angled
{0x3,0x6,0xc,0x18,0x10,0x0,0x0,0x0},  // top angled
{0x15,0x0,0x0,0x0,0x0,0x0,0x0,0x15},  // unused tracks
{0,0,0,0,0,0,0,0}
};
uint8_t demo_chars[][8] = {
{0,0,0,0,0,0,0,0},
{0x0,0x4,0x6,0x1f,0x1f,0x6,0x4,0x0},  // right arrow
{0x0,0x4,0xc,0x1f,0x1f,0xc,0x4,0x0},   // left arrow
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};
uint8_t help_chars[][8] = {
{0,0,0,0,0,0,0,0},
{0x0,0x4,0x6,0x1f,0x1f,0x6,0x4,0x0},  // right arrow
{0x0,0x4,0xc,0x1f,0x1f,0xc,0x4,0x0},   // left arrow
{0x0,0xe,0x1b,0x11,0x11,0x1b,0xe,0x0}, // stop
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};

#define NUM_HELP_LINES 20
char* help_text[] = {
   "HELP MODE",
   "\x2: Page Up",
   " ",
   "\x1: Page Down",
   "TRAIN 1/2 MODE",
   "\x2: Reverse Speed",
   "\x3: Stop",
   "\x1: Forward Speed",
   "SWITCH 1/2 MODE",
   "\x2: Throw switch",
   " ",
   "\x1: Close switch",
   "DEMO MODE",
   "\x2: Start Demo",
   " ",
   "\x1: Stop Demo",
   "ALL MODES",
   "Press all 4 buttons:",
   "Emergency Stop All",
   "MODE: Change Modes"
};

void lcd_init_screen( enum interface_mode_type mode )
{
   LCD_CLEAR();

   switch( mode )
   {
#ifdef MILESTONE_4
   case SENSOR_MODE:
      load_custom_chars( ms4_chars );
      // This is a lot of data to send at once, so let the i2c thread
      // handle it by sleeping every now and then
      lcd_write( 0, "MS 4! I <3 Embedded!" );
      lcd_write( 0x40+1, "Buttons: \x2\x2\x2\x2" );
      lcd_write( 0x14+1, "Error: " );
      lcd_write( 0x54+1, "No New Data..." );
      break;
#endif

   case TRAIN_MODE:
      load_custom_chars( train_chars );
      sleep( 10 );
      lcd_write( 0x06, "Train #" );
      break;
   case SWITCH_MODE:
      load_custom_chars( switch_chars );
      sleep( 10 );
      lcd_write( 0x06, "Switch #" );
      lcd_write( 0x4C, "\x1\x1\x1\x1\x1\x1" );
      lcd_write( 0x56, "\x1\x1\x1\x1\x1\x1" );
      break;
   case HELP_MODE:
      load_custom_chars( help_chars );
      lcd_scroll_help( -10000 );
      break;
   case DEMO_MODE:
      load_custom_chars( demo_chars );
      lcd_write( 0x08, "DEMO" );
      lcd_write( 0x40, "Not Running" );
      lcd_write( 0x14, "Press \x2 to start" );
      break;
   default:
      break;
   }
}

void lcd_init()
{
   lcd_send_cmd( LCD_CURSOR_HOME );
   load_custom_chars( train_chars );
   sleep( 10 );
}

void* wall_lcd_thread( void* dptr )
{
   wall_lcd_msg in_msg;
   static enum wall_lcd_msg_type prev_type = LCD_ACTION;

#ifdef MILESTONE_4
   unsigned char buttons = 0;
   unsigned char changed_buttons;
#endif

   lcd_init();
   lcd_init_screen( TRAIN_MODE );

	while( 1 )
	{
		msgrcv_em( thread_qs.wall_lcd_thread, &in_msg, sizeof(in_msg),
                 "wall_lcd_thread recv failed" );
      DBG_LOG_ENTRY( "wall_lcd_thread start", 0 );

		switch( in_msg.type )
		{
         case LCD_ACTION:
            break;

#ifdef MILESTONE_4
         case LCD_NO_SENSOR_DATA:
            if( prev_type != LCD_NO_SENSOR_DATA )
            {
               lcd_write( 0x40, " " );
               lcd_write( 0x14, " " );
               lcd_write( 0x54, "\x1" );
            }
            break;

         case LCD_UPDATE_BUTTON_STATUS:
            if( prev_type != LCD_UPDATE_BUTTON_STATUS )
            {
               lcd_write( 0x40, "\x1" );
               lcd_write( 0x14, " " );
               lcd_write( 0x54, " " );
            }

            if( buttons == in_msg.sensor_data )
               break;

            changed_buttons = buttons ^ in_msg.sensor_data;
            buttons = in_msg.sensor_data;

            if( changed_buttons & BUTTON_1)
               lcd_write( 0x40+10, (buttons&BUTTON_1)?"\x3":"\x2" );
            if( changed_buttons & BUTTON_2)
               lcd_write( 0x40+11, (buttons&BUTTON_2)?"\x3":"\x2" );
            if( changed_buttons & BUTTON_3)
               lcd_write( 0x40+12, (buttons&BUTTON_3)?"\x3":"\x2" );
            if( changed_buttons & BUTTON_4)
               lcd_write( 0x40+13, (buttons&BUTTON_4)?"\x3":"\x2" );

            break;

         case LCD_DISPLAY_SENSE_ERROR:
         {
            char error[100];
            if( prev_type != LCD_DISPLAY_SENSE_ERROR )
            {
               lcd_write( 0x40, " " );
               lcd_write( 0x14, "\x1" );
               lcd_write( 0x54, " " );
            }
            sprintf( error, "0x%2X", in_msg.sensor_error );
            lcd_write( 0x1C, error );
            break;
         }
#endif // MILESTONE_4

         case LCD_CHANGE_STATE:
            lcd_init_screen( in_msg.new_state );
            break;

         case LCD_DISPLAY_TRAIN:
         {
            int i;
            char id[2] = {'1' + in_msg.train.id, 0};
            char arrows[19];
            lcd_write( 0x0D, id );
            for( i = -9; i < MAX_SPEED; ++i )
            {
               if( i < 0 )
               {
                  if( i < in_msg.train.speed )
                     arrows[i+9] = 4;
                  else
                     arrows[i+9] = 3;
               }
               else
               {
                  if( i < in_msg.train.speed )
                     arrows[i+9] = 1;
                  else
                     arrows[i+9] = 2;
               }
            }
            arrows[18] = '\0';
            lcd_write( 0x41, arrows );
            break;
         }

         case LCD_DISPLAY_SWITCH:
         {
            char id[2] = {'1' + in_msg.t_switch.id, 0};
            lcd_write( 0x0E, id );

            if( in_msg.t_switch.state == SWITCH_THROWN )
            {
               lcd_write( 0x42, "\x6\x6\x6\x6\x6\x6\x6\x6\x4\x5" );
               lcd_write( 0x1C, "\x4\x5\x4\x5" );
               lcd_write( 0x5C, "\x4\x5\x6\x6\x6\x6\x6\x6\x6\x6" );
            }
            else
            {
               lcd_write( 0x42, "\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1" );
               lcd_write( 0x1C, "    " );
               lcd_write( 0x5C, "\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1" );
            }
            break;
         }

         case LCD_HELP_DOWN:
            lcd_scroll_help( 1 );
            break;
         case LCD_HELP_UP:
            lcd_scroll_help( -1 );
            break;

         case LCD_DISPLAY_ERROR:
            LCD_CLEAR();
            lcd_write( 0x14, in_msg.error );
            break;

         case LCD_DEMO_START:
            lcd_write( 0x40, "Running    " );
            lcd_write( 0x14, "Press \x1 to Stop " );
            break;

         case LCD_DEMO_STOP:
            lcd_init_screen( DEMO_MODE );
            break;

         default:
           break;
      }
      prev_type = in_msg.type;
      DBG_LOG_ENTRY( "wall_lcd_thread end", 0 );
	}

   return dptr;
}

void lcd_write( unsigned char address, const char* text )
{
   size_t length = 0;
   i2c_msg out_msg;

   lcd_send_cmd( LCD_SET_CURSOR, address );

   out_msg.type = I2C_SEND;
   out_msg.address = WALL_LCD_ADDR;
   length += offsetof( i2c_msg, lcd_text );

   strncpy( out_msg.lcd_text, text, sizeof(out_msg.lcd_text) );

   if( strlen(text) > sizeof(out_msg.lcd_text) )
      length += sizeof(out_msg.lcd_text);
   else
      length += strlen( text );

   msgsnd_em( thread_qs.i2c_thread, &out_msg, length,
              "lcd_write send failed" );

   sleep( 10 );
}

void lcd_send_cmd( enum wall_lcd_cmd cmd, ... )
{
   va_list vl;
   size_t length = 0;
   i2c_msg out_msg;
   wall_lcd_cmd_msg* lcd_cmd = &out_msg.lcd_cmd;

   out_msg.type = I2C_SEND;
   out_msg.address = WALL_LCD_ADDR;
   length += offsetof( i2c_msg, lcd_cmd );

   lcd_cmd->prefix = (uint8_t)LCD_CMD_PREFIX;
   lcd_cmd->cmd    = cmd;
   length += offsetof( wall_lcd_cmd_msg, params );

   va_start( vl, cmd );

   switch( cmd )
   {
      // These commands have 1 byte argument
      case LCD_SET_CURSOR:
      case LCD_SET_CONTRAST:
      case LCD_SET_BACKLIGHT:
      case LCD_SET_ADDRESS:
      {
         // For some reason, variadic arguments must be passed as ints
         lcd_cmd->params[0] = (uint8_t)va_arg( vl, int );
         length += 1;
         break;
      }
      // This command has 1 byte address, and pointer to array
      case LCD_LOAD_CHAR:
      {
         lcd_cmd->params[0] = (uint8_t)va_arg( vl, int );
         uint8_t* char_index = va_arg( vl, uint8_t* );
         memcpy( &lcd_cmd->params[1], char_index, 8 );
         length += 9;

         break;
      }
      // No arguments
      default:
         break;
   }
   va_end( vl );
   msgsnd_em( thread_qs.i2c_thread, &out_msg, length,
              "lcd_send_cmd send failed" );
}

void load_custom_chars( uint8_t array[][8] )
{
   unsigned int i;
   for( i = 0; i < 8; i++ )
   {
      lcd_send_cmd( LCD_LOAD_CHAR, i, array[i] );
      sleep( 1 );
   }
}

void lcd_scroll_help( int pages )
{
   static int base_line = 0;
   base_line += (pages*4);

   if( base_line < 0 )
      base_line = 0;
   else if( base_line > NUM_HELP_LINES-4 )
      base_line = NUM_HELP_LINES-4;

   LCD_CLEAR();

   lcd_write( 0x00, help_text[base_line] );

   if( (base_line+1) < NUM_HELP_LINES )
      lcd_write( 0x40, help_text[base_line+1] );
   else
      lcd_write( 0x40, "                    " );
      
   if( (base_line+2) < NUM_HELP_LINES )
      lcd_write( 0x14, help_text[base_line+2] );
   else
      lcd_write( 0x14, "                    " );

   if( (base_line+3) < NUM_HELP_LINES )
      lcd_write( 0x54, help_text[base_line+3] );
   else
      lcd_write( 0x40, "                    " );
}

void lcd_handle_i2c( enum i2c_status status )
{
   static enum i2c_status state = I2C_SEND_COMPLETE;
   if( status == state )
      return;

   train_msg out_msg;

   switch( status )
   {
   case I2C_SLAVE_NO_ACK:
      out_msg.type = LCD_DISCONNECTED;
      break;
   case I2C_SEND_COMPLETE:
      out_msg.type = LCD_RECONNECTED;
      break;
   default:
      break;
   }

   msgsnd_e( thread_qs.train_thread, &out_msg, sizeof(out_msg) );

   state = status;
}

void lcd_display_error( const char* message )
{
   wall_lcd_msg error_msg;
   error_msg.type = LCD_DISPLAY_ERROR;
   error_msg.error = message;
   msgsnd_e( thread_qs.wall_lcd_thread, &error_msg, sizeof(error_msg) );
}
