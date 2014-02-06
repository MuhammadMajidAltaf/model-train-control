/****************************************************************************\
 *
 * Group 9
 * ECE 4534, Spring 2011
 * 
 * Modifications:
 * - i2c_thread accepts messages formatted using the i2c_msg struct in
 *   messages.h
 * - Can request an arbitrary number of bytes from a slave
 * - Properly handles status messages returned on the internal queue
 *
\****************************************************************************/
#include <xmk.h>
#include <sys/intr.h>
#include <errno.h>

#include "i2c.h"
#include "my_debug.h"
#include "messages.h"
#include "main.h"

extern message_qs thread_qs;

void i2c_status_handler( void *dptr, XStatus s_event )
{
   I2C_Comm* cptr = (I2C_Comm*)dptr;
   //XStatus status;
   enum i2c_status out_msg;

   //DBG_LOG_ENTRY_INTR("IICStatus:Event",s_event);

   // This will never happen if configured as master
   //if( s_event == XII_MASTER_WRITE_EVENT )
   //{
   //   status = XIic_SlaveRecv( &(cptr->instance), cptr->iic_recv_msg,
   //         sizeof(Xuint8)*IIC_MSGLEN);

   //   DBG_LOG_ENTRY_INTR("IICStatus:SlaRecv",status);

   //   if (status != XST_SUCCESS)
   //   {
   //      DBG_LOG_ENTRY_INTR("IICStatus:BadSRecv",status);
   //   }
   //}

   switch( s_event )
   {
   case XII_SLAVE_NO_ACK_EVENT:
      out_msg = I2C_SLAVE_NO_ACK;
      break;
   case XII_ARB_LOST_EVENT:
      out_msg = I2C_ARB_LOST_EVENT;
      break;
   case XII_BUS_NOT_BUSY_EVENT:
      out_msg = I2C_BUS_NOT_BUSY;
      break;
   default:
      DBG_LOG_ENTRY_INTR( "other event in status handler", s_event );
      out_msg = s_event;
      break;
   }

   msgsnd_int( cptr->internal_status_queue_id, &out_msg, sizeof(out_msg),
               "i2c_status_handler send failed" );
}

void i2c_send_handler( void *dptr, int bytecount )
{
   I2C_Comm *cptr = (I2C_Comm*)dptr;
   enum i2c_status out_msg;

   if( bytecount != 0 )
   {
      DBG_LOG_ENTRY_INTR( "bytecount != 0 in send_handler", bytecount );
      return;
   }

   // send a message that the send completed
   out_msg = I2C_SEND_COMPLETE;
   msgsnd_int( cptr->internal_status_queue_id, &out_msg, sizeof(out_msg),
               "i2c_send_handler send failed" );
}

void i2c_recv_handler( void *dptr, int bytecount )
{
   I2C_Comm *cptr = (I2C_Comm*)dptr;
   enum i2c_status out_msg;

   if( bytecount != 0 )
   {
      DBG_LOG_ENTRY_INTR( "bytecount != 0 in recv_handler", bytecount );
      return;
   }

   // send a message to a receiver
   msgsnd_int( cptr->outgoing_msg_queue_id, cptr->iic_recv_msg,
               sizeof(train_msg), "i2c_recv_handler outgoing failed" );

   // send a message that the recv completed
   out_msg = I2C_RECV_COMPLETE;
   msgsnd_int( cptr->internal_status_queue_id, &out_msg, sizeof(out_msg),
               "i2c_recv_handler internal failed" );
}

XStatus init_i2c(I2C_Comm *cptr,int device_id,int int_id,int i2c_addr,
        int master)
{
   XStatus status;

   // initialize the driver
   status = XIic_Initialize(&(cptr->instance),device_id);
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY( "INIT IIC Failed\n", 0 );
      return(status);
   }

   // connect the interrupt handler
   cptr->int_id = int_id;
   status = register_int_handler(int_id,XIic_InterruptHandler,
      &(cptr->instance));
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY( "INT_REG error\n", 0 );
      return(status);
   }

   // connect the recv handler
   XIic_SetRecvHandler(&(cptr->instance),cptr,i2c_recv_handler);
   XIic_SetSendHandler(&(cptr->instance),cptr,i2c_send_handler);
   XIic_SetStatusHandler(&(cptr->instance),cptr,i2c_status_handler);

   enable_interrupt(int_id);
   status = XIic_Start(&(cptr->instance));
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY( "IIC Start Error\n", 0 );
      return(status);
   }

   DBG_LOG_ENTRY( "IIC_Init: Master", master );

   return status;
}

void i2c_timer_handler( void *dptr, Xuint8 timer_num )
{
   I2C_Comm* cptr = (I2C_Comm*)dptr;

   switch( timer_num )
   {
   case GIVE_UP_TIMER:
   {
      enum i2c_status out_msg = I2C_TIMEOUT;

      msgsnd_int( cptr->internal_status_queue_id, &out_msg, sizeof(out_msg),
                  "i2c_timout handler send failed" );
      break;
   }
   case TRY_AGAIN_TIMER:
      break;
   }
}

// This thread receives messages from other threads/handlers.
void* i2c_thread( void* dptr )
{
   I2C_Comm* cptr = (I2C_Comm*)dptr;
   XTmrCtr i2c_timer;
   i2c_msg in_msg;   
   i2c_status_msg out_msg;

   enum i2c_status status_msg;
   size_t length = 0;
   int retry = 0;
   XStatus status;

   // Initialize the bus error recovery timer
   init_timer( &i2c_timer, TIMER_4_DEVICE, TIMER_4_INT,
               i2c_timer_handler, dptr );
   init_counter( &i2c_timer, GIVE_UP_TIMER, GIVE_UP_PERIOD );

   while( 1 )
   {
      if( retry == 0 )
      {
         length = msgrcv_em( thread_qs.i2c_thread, &in_msg, sizeof(in_msg),
                             "i2c_thread receive" );

         // Set the address for both read and write
         XIic_SetAddress( &(cptr->instance), XII_ADDR_TO_SEND_TYPE, in_msg.address );
      }
      else
         retry = 0;

      // start a message send
      switch( in_msg.type )
      {
      case I2C_RQST:
         if( in_msg.recv_length > sizeof(cptr->iic_recv_msg) )
            in_msg.recv_length = sizeof(cptr->iic_recv_msg);

         status = XIic_MasterRecv( &(cptr->instance), cptr->iic_recv_msg, in_msg.recv_length );
         break;

      case I2C_SEND:
         status = XIic_MasterSend( &(cptr->instance), &in_msg.d, length-offsetof(i2c_msg,d) );
         break;
      
      default:
         status = XST_SUCCESS;
         DBG_LOG_ENTRY( "Bad I2C Type", in_msg.type );
         break;
      }

      out_msg.status = status;
      out_msg.slave_addr = in_msg.address;

      // Handle all possible return values of XIic_Master{Recv,Send}
      switch( status )
      {
      // Indicates the transaction started successfully
      case XST_SUCCESS:
      {
         // Wait for it to finish
         XTmrCtr_Start( &i2c_timer, GIVE_UP_TIMER );
         msgrcv_em( cptr->internal_status_queue_id, &status_msg,
                    sizeof(status_msg), "xst_success wait fail" );
         XTmrCtr_Stop( &i2c_timer, GIVE_UP_TIMER );

         out_msg.status = status_msg;
         switch( status_msg )
         {
         // This can only happen in a multi-master configuration
         case I2C_ARB_LOST_EVENT:
            break;

         // In this context, a timeout means the slave didn't ACK
         case I2C_TIMEOUT:
            out_msg.status = I2C_SLAVE_NO_ACK;
            // fallthrough

         case I2C_SLAVE_NO_ACK:
            DBG_LOG_ENTRY( "slave no ack", in_msg.address );
            break;

         case I2C_RECV_COMPLETE:
            break;

         case I2C_SEND_COMPLETE:
            break;

         default:
            DBG_LOG_ENTRY( "bad xst_success status msg", status_msg );
            break;
         }
         // send a status message to the outgoing status queue
         msgsnd_em( cptr->out_status_queue_id, &out_msg, sizeof(out_msg),
                    "i2c out_status send fail" );
         break;
      }

      // If the bus is busy in a single-master configuration,
      // a device is erroneously holding a line low and the
      // only way to fix it is probably a hardware reset of all devices
      case XST_IIC_BUS_BUSY:
      {
         msgsnd_e( thread_qs.i2c_out_status, &out_msg, sizeof(out_msg) );

         DBG_LOG_ENTRY( "bus busy", in_msg.address );
         while( 1 )
         {
            XTmrCtr_Start( &i2c_timer, GIVE_UP_TIMER );
            msgrcv_em( cptr->internal_status_queue_id, &status_msg,
                       sizeof(status_msg), "iic_bus_busy recv fail" );
            XTmrCtr_Stop( &i2c_timer, GIVE_UP_TIMER );
            
            out_msg.status = status_msg;
            msgsnd_e( thread_qs.i2c_out_status, &out_msg, sizeof(out_msg) );
            if( status_msg == I2C_BUS_NOT_BUSY )
            {
               DBG_LOG_ENTRY( "bus no longer busy", 0 );
               break;
            }
            else
            {
               DBG_LOG_ENTRY( "not BNB message", status_msg );
            }
         }
         retry = 1;
         break;
      }

      // We do not make use of the general call address,
      // so consider it an error
      case XST_IIC_GENERAL_CALL_ADDRESS:
         DBG_LOG_ENTRY( "general call address error", status );
         break;
      }
   }
}
