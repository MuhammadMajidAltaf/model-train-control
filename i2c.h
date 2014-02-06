#ifndef IIC_H
#define IIC_H

#include <xiic.h>

#include "main.h"

#define MAX_ATTEMPTS 15

#define MAX_IIC_MSGLEN 20
#define IIC_MSGLEN 12
typedef struct __I2C_Comm {
   XIic instance;
   // this is the queue that accepts messages to send
   int incoming_msg_queue_id;
   // this is the queue to which reports on the status of i2c sends
   // are sent
   int out_status_queue_id;
   // purely for the internal i2c use
   int internal_status_queue_id;
   // queue to which received i2c messages are sent
   int outgoing_msg_queue_id;
   Xuint8 iic_recv_msg[MAX_IIC_MSGLEN];
   int   int_id;
} I2C_Comm;

#define GIVE_UP_PERIOD 20
#define RETRY_PERIOD   1000
enum i2c_timer_num
{
   GIVE_UP_TIMER = 0,
   TRY_AGAIN_TIMER = 1
};

enum i2c_status
{
   I2C_BUS_NOT_BUSY,
   I2C_SLAVE_NO_ACK,
   I2C_ARB_LOST_EVENT,
   I2C_SEND_COMPLETE,
   I2C_RECV_COMPLETE,
   I2C_TIMEOUT
};

typedef struct
{
   enum i2c_status status;
   enum i2c_address slave_addr;
} i2c_status_msg;

// function definitions
XStatus init_i2c(I2C_Comm *,int,int,int,int);
void* i2c_thread( void* dptr );

#endif

