/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
\****************************************************************************/
#ifndef WALL_LCD_H
#define WALL_LCD_H

#include <stdarg.h>
#include "train.h"

// Tell the thread what to do, how to interpret the message
enum wall_lcd_msg_type 
{
   LCD_DISPLAY_SENSE_ERROR = SENSOR_ERROR,
   LCD_NO_SENSOR_DATA = SENSOR_NO_DATA,
   LCD_UPDATE_BUTTON_STATUS = SENSOR_DATA,
   LCD_ACTION,
   LCD_CHANGE_STATE,
   LCD_DISPLAY_TRAIN,
   LCD_DISPLAY_SWITCH,
   LCD_HELP_DOWN,
   LCD_HELP_UP,
   LCD_DISPLAY_ERROR,
   LCD_DEMO_START,
   LCD_DEMO_STOP
};

// All messages sent to the wall_lcd thread must use this struct
typedef struct
{
   enum wall_lcd_msg_type type;
   union {
      uint8_t sensor_data;
      enum sensor_error_type sensor_error;
      enum interface_mode_type new_state;
      train_data train;
      switch_data t_switch;
      const char* error;
   };
} wall_lcd_msg;

// LCD Addresses
//           Column 1    Column 20
// Line 1    0x00        0x13
// Line 2    0x40        0x53
// Line 3    0x14        0x27
// Line 4    0x54        0x67

// A command to be sent to the LCD
enum wall_lcd_cmd
{                             // No parameters unless specified
   LCD_DISPLAY_ON    = 0x41,
   LCD_DISPLAY_OFF   = 0x42,
   LCD_SET_CURSOR    = 0x45,  // address (0x00 to 0x67)
   LCD_CURSOR_HOME   = 0x46,
   LCD_UNDERLINE_ON  = 0x47,
   LCD_UNDERLINE_OFF = 0x48,
   LCD_CURSOR_LEFT   = 0x49,
   LCD_CURSOR_RIGHT  = 0x4A,
   LCD_BLINK_ON      = 0x4B,
   LCD_BLINK_OFF     = 0x4C,
   LCD_BACKSPACE     = 0x4E,
   LCD_CLEAR_SCREEN  = 0x51,
   LCD_SET_CONTRAST  = 0x52,  // value (1 to 50)
   LCD_SET_BACKLIGHT = 0x53,  // value (1 to 8)
   LCD_LOAD_CHAR     = 0x54,  // address(0-7), character index
   LCD_DISPLAY_LEFT  = 0x55,
   LCD_DISPLAY_RIGHT = 0x56,
   LCD_SET_ADDRESS   = 0x62,  // 1 byte (LSB must be 0)
   LCD_SHOW_VERSION  = 0x70,
   LCD_SHOW_ADDRESS  = 0x72
};

#define LCD_CMD_PREFIX 0xFE
#define LCD_CLEAR() lcd_send_cmd( LCD_CLEAR_SCREEN )

void* wall_lcd_thread( void* dptr );

// Initialize the LCD with different defaults
void lcd_init();
void lcd_init_screen( enum interface_mode_type mode );

// Write the given string at the given LCD address (see above)
void lcd_write( unsigned char address, const char* text );

// Load the first 8 characters onto the LCD
void load_custom_chars( uint8_t array[][8] );

// Send a generic command with optional arguments
void lcd_send_cmd( enum wall_lcd_cmd cmd, ... );

// Scroll help screen text by given number of lines
void lcd_scroll_help( int lines );

// I2C status handler
void lcd_handle_i2c( enum i2c_status status );

// Easy message interface for displaying errors
void lcd_display_error( const char* message );

#endif // !WALL_LCD_H
