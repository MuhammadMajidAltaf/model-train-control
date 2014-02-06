/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 * 
 * messages.h contains the structs used to contain all i2c messages 
 *
\****************************************************************************/
#ifndef MESSAGES_H
#define MESSAGES_H

#include "train.h"
#include "wall_lcd.h"
#include "i2c.h"

typedef char wall_lcd_text_msg[MAX_IIC_MSGLEN];

// A command struct to send to the LCD
typedef struct
{
   // prefix = LCD_CMD_PREFIX;
   uint8_t      prefix;

   // Type of command being sent
   enum wall_lcd_cmd cmd;

   // Array for extra parameters (usually empty, max is 9 bytes);
   uint8_t      params[9];
} wall_lcd_cmd_msg;

// A command struct to send to the LocoNet PIC
typedef struct
{
   // Type of command
   enum train_cmd_type type;

   // LocoNet id of the desired train or switch
   uint8_t        id;

   // Pick one of these data types
   union {
      enum train_dir    dir;
      train_speed       speed;
      enum switch_state thrown;
   };
} train_cmd_msg;

// All messages sent to i2c_thread must use this struct
typedef struct
{
   // Receive or Send?
   enum {
      I2C_RQST,
      I2C_SEND
   } type;

   // I2C Address of slave
   uint8_t address;

   union {
      // Specify the number of bytes to read from the slave, or...
      size_t recv_length;

      // Pick a command struct to send to a slave
      // These are the actual data bytes that get sent
      wall_lcd_text_msg lcd_text;
      wall_lcd_cmd_msg  lcd_cmd;
      train_cmd_msg     train_cmd;

      // For internal use by i2c_thread
      uint8_t d;
   };
} i2c_msg;

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
extern inline void messages_h_assertions()
{
   STATIC_ASSERT( offsetof(wall_lcd_cmd_msg,params) == 2 );
   STATIC_ASSERT( sizeof(train_cmd_msg) == 3 );
   STATIC_ASSERT( offsetof(i2c_msg,recv_length)
                  == offsetof(i2c_msg,d) );
}

#endif // !MESSAGES_H
