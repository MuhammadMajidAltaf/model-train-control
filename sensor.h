/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * sensor.h contains enums and structs defining the i2c interface to the 
 * capacitive touch sensor board
 *
\****************************************************************************/
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

#include "main.h"

// Actual data packet returned by sensor
typedef struct
{
   // Status returned by the board
   enum sensor_read_msg_type {
      SENSOR_ERROR   = -1,
      SENSOR_NO_DATA = 0,
      SENSOR_DATA    = 1
   } type;

   // Pick a payload
   union {
      // An error type
      enum sensor_error_type {
         SOME_ERROR
      } error;

      // Or a button data packet
      uint8_t data;
   };
} sensor_read_msg;

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
extern inline void sensor_h_assertions()
{
   STATIC_ASSERT( sizeof(sensor_read_msg) == 2 );
}

#endif // !SENSOR_H
