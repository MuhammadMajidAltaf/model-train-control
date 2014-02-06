/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * main.c contains:
 * - button handler thread
 * - msg{snd,rcv} utility wrapper functions
 * - user_main
 * - a timer ISR
 *
 * I highly recommend the code in this project. It works.
 *
\****************************************************************************/
#include <xmk.h>
#include <pthread.h>
#include <sys/msg.h>
#include <errno.h>
#include <xparameters.h>
#include <stdio.h>

#include "buttons.h"
#include "timer.h"
#include "my_debug.h"
#include "VTio.h"
#include "wall_lcd.h"
#include "main.h"
#include "train.h"
#include "messages.h"

message_qs thread_qs;

XTmrCtr timer_3;
extern XTmrCtr debug_timer;

void sensor_handle_i2c( enum i2c_status status ) __attribute__ ((section(".high_access")));
//void sensor_handle_i2c( enum i2c_status status );
extern void wd_handler( void *dptr, Xuint8 timer_num );

// Entry point into the program
int main()
{
   xilkernel_main();
   return 0;
}

int msgsnd_int( int msqid, const void* msgp, size_t msgsz, const char* message )
{
   int result = msgsnd( msqid, msgp, msgsz, IPC_NOWAIT );

#ifdef MY_DEBUG
   if( result != -1 )
      return result;

   if( message == NULL )
      message = "send failed";

   if( errno == ENOSPC )
      DBG_LOG_ENTRY_INTR( message, 0 );
#endif

   return result;
}

ssize_t msgrcv_e( int msqid, void* msgp, size_t nbytes )
{
   return msgrcv_em( msqid, msgp, nbytes, "" );
}
ssize_t msgrcv_em( int msqid, void* msgp, size_t nbytes, const char* message )
{
   ssize_t length = msgrcv( msqid, msgp, nbytes, 0, 0 );

#ifdef MY_DEBUG
   if( length != -1 )
      return length;

   if( message == NULL )
      message = "";

   char buf[NAME_SIZE+1];
   const char log_fmt[] = "%.20s:%19s";

   // I hope errno is threadsafe on xilkernel
   switch( errno )
   {
   case EINVAL:
      sprintf( buf, log_fmt , message, "msgrcv EINVAL" );
      DBG_LOG_ENTRY( buf, errno );
      break;
   case EIDRM:
      sprintf( buf, log_fmt, message, "msgrcv EIDRM" );
      DBG_LOG_ENTRY( buf, errno );
      break;
   case ENOMSG:
      sprintf( buf, log_fmt, message, "msgrcv ENOMSG" );
      DBG_LOG_ENTRY( buf, errno );
      break;
   }
#endif // MY_DEBUG

   return length;
}


int msgsnd_e( int msqid, const void* msgp, size_t msgsz )
{
   return msgsnd_em( msqid, msgp, msgsz, "" );
}
int msgsnd_em( int msqid, const void* msgp, size_t msgsz, const char* message )
{
   int result = -1;
   int tries = 0;
   while( tries++ < 3 )
   {
      // Normally, it would make sense to wait. But a full queue
      // probably indicates there's a problem I'd like to know about
      result = msgsnd( msqid, msgp, msgsz, IPC_NOWAIT );

      if( result == -1 && errno == ENOSPC )
         sleep( 1 );
      else
         break;
   }

#ifdef MY_DEBUG
   char buf[NAME_SIZE+1];
   const char log_fmt[] = "%.26s:%13s";

   if( result == 0 )
      return result;

   if( message == NULL )
      message = "";

   switch( errno )
   {
   case EINVAL:
      sprintf( buf, log_fmt, message, "msgsnd EINVAL" );
      DBG_LOG_ENTRY( buf, msgsz );
      break;
   case ENOSPC:
      sprintf( buf, log_fmt, message, "msgsnd ENOSPC" );
      DBG_LOG_ENTRY( buf, msgsz );
      break;
   case EIDRM:
      sprintf( buf, log_fmt, message, "msgsnd EIDRM" );
      DBG_LOG_ENTRY( buf, msgsz );
      break;
   }
#endif

   return result;
}

///////////////////////////////////////////
// the button thread
///////////////////////////////////////////
void* button_thread( void* dptr )
{
   int data;

   while( 1 )
   {
      msgrcv_em( thread_qs.button_thread, &data, 
                 sizeof(data), "button_thread" );

      *LEDS = 0x01;

      // Mask off the rotary knob inputs
      data &= NO_ROTARY_MASK;

      if( (data & BUTTONS_MASK) != 0 )
      {
         switch( data & BUTTONS_MASK )
         {
         case NORTH_BUTTON:
            VTinitLCD();
            VTprintLCD( "                ", 1 );
            VTprintLCD( "                ", 2 );
            break;

         case EAST_BUTTON:
            PRINT_DBG_LOG();
            break;

         case WEST_BUTTON:
            data |= (*SWITCHES & SWITCHES_MASK);
            break;

         default: 
            break;
         }
      }
      if( (data & SWITCHES_MASK) != 0 )
      {
          train_msg out_msg;
          out_msg.type = SENSOR_READ;
          out_msg.sensor_msg.type = SENSOR_DATA;
          out_msg.sensor_msg.data = (data & SWITCHES_MASK);
          msgsnd_e( thread_qs.train_thread, &out_msg, sizeof(out_msg) );
      }
   }
}

void timer_3_handler( void *dptr, Xuint8 timer_num )
{
   switch( timer_num )
   {
   // Sensor timer
   case SENSOR_TIMER:
   {
      DBG_LOG_ENTRY_INTR( "sensor timer intr", 0 );
      i2c_msg out_msg;
      
      timer_start( &timer_3, timer_num );

      out_msg.type = I2C_RQST;
      out_msg.address = SENSOR_PIC_ADDR;
      out_msg.recv_length = 3;
      
      msgsnd_em( thread_qs.i2c_thread, &out_msg, sizeof(out_msg),
                 "sensor_timer" );
      DBG_LOG_ENTRY_INTR( "sensor timer intr done", 0 );
      break;
   }
   case DEBOUNCE_TIMER:
      debounce_handler( dptr );
      break;
   }
}

// Set the schedule priority of the pthread attribute
void init_thread_attr( pthread_attr_t* attr, int sched_priority )
{
   struct sched_param sp;

   pthread_attr_init( attr );
   pthread_attr_getschedparam( attr, &sp );
   sp.sched_priority = sched_priority;
   pthread_attr_setschedparam( attr, &sp );
}

////////////////////////////////////////
// This thread is "statically" started by the kernel on initialization
//   -- It takes no arguments (unlike other threads)
//   -- In this program, it is responsible for initializing devices and threads
////////////////////////////////////////
void* user_main()
{
   // Device-related structures
   XGpio    buttons;
   I2C_Comm   i2c_comm;

   // Thread-related structures
   // Use the same ones over for each thread
   pthread_t          thread;
   pthread_attr_t     thread_attr;
   struct sched_param thread_sp;
   int                policy;

   // Return code variable for calls to Xilinx routines
   XStatus status;

   init_timer( &timer_3, TIMER_3_DEVICE, TIMER_3_INT,
               timer_3_handler, &buttons );

   ////////////////////////////////////////
   // Initialize the debugging code
   // -- It will use the 2nd timer (the OS uses the first timer)
   ////////////////////////////////////////
#if ENABLE_DEBUG_TIMER
   status = INIT_DEBUG( NULL,
                        TIMER_2_DEVICE,
                        TIMER_2_INT,
                        0,
                        WATCHDOG_TIMEOUT);
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY( "Main:INIT_DEBUG failed", status );
   }
#else
#ifdef MY_DEBUG
   init_log();
   init_timer( &debug_timer, 
               TIMER_2_DEVICE, 
               TIMER_2_INT, 
               wd_handler, 
               &debug_timer );
   XTmrCtr_SetOptions( &debug_timer, 
                       1,
                       XTC_AUTO_RELOAD_OPTION );
   XTmrCtr_SetResetValue( &debug_timer, 1, 0 );
   XTmrCtr_Start( &debug_timer, 1 );
#endif
#endif


   ////////////////////////////////////////
   // Set up message queues
   ////////////////////////////////////////
   thread_qs.wall_lcd_thread = msgget( WALL_LCD_Q, IPC_CREAT );
   thread_qs.train_thread    = msgget( TRAIN_Q, IPC_CREAT );
   thread_qs.button_thread   = msgget( BUTTON_Q_1, IPC_CREAT );
   thread_qs.i2c_thread      = msgget( I2C_INCOMING_Q, IPC_CREAT );
   thread_qs.i2c_out_status  = msgget( I2C_STATUS_Q, IPC_CREAT );

   i2c_comm.incoming_msg_queue_id    = thread_qs.i2c_thread;
   i2c_comm.out_status_queue_id      = thread_qs.i2c_out_status;
   i2c_comm.internal_status_queue_id = msgget(I2C_ISTATUS_Q,IPC_CREAT);
   i2c_comm.outgoing_msg_queue_id    = thread_qs.train_thread;

   ////////////////////////////////////////
   // initialize the button device, including its message queue
   // to which it will send messages.
   ////////////////////////////////////////
#if ENABLE_BUTTON_INTR
   status = init_buttons( &buttons );
   if (status != XST_SUCCESS)
      DBG_LOG_ENTRY("Main:Err_init_button",status);
#endif

   ////////////////////////////////////////
   // Initialize the master i2c device on J2
   ////////////////////////////////////////
#if ENABLE_IIC_THREAD
   status = init_i2c( &i2c_comm,
                      XPAR_XPS_IIC_J2_DEVICE_ID,
                      XPAR_XPS_INTC_0_XPS_IIC_J2_IIC2INTC_IRPT_INTR,
                      SELF_ADDR,
                      1 );
   if( status != XST_SUCCESS ) {
      DBG_LOG_ENTRY( "Init I2C Failed", status );
   }
#endif
   
   ////////////////////////////////////////
   // Code for examining the priority of this thread
   // -- This isn't "necessary", but it does tell us the priority of
   //    this thread
   ////////////////////////////////////////
   // get schedule information on the static thread (this thread)
   thread = pthread_self();
   pthread_getschedparam( thread, &policy, &thread_sp );
   DBG_LOG_ENTRY("Main:self_priority", thread_sp.sched_priority );

#if ENABLE_TRAIN_THREAD
   //Create Train Thread
   init_thread_attr( &thread_attr, TRAIN_THREAD_PRIORITY );
   status = pthread_create( &thread, &thread_attr,   train_thread, NULL );
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY("Main:Err_train_thread",status);
   }
#endif

#if ENABLE_LCD_THREAD
   //Create Wall LCD Thread
   init_thread_attr( &thread_attr, LCD_THREAD_PRIORITY );
   status = pthread_create( &thread, &thread_attr, wall_lcd_thread, NULL );
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY("Main:Err_wall_lcd_thread",status);
   }
#endif

#if ENABLE_BUTTON_THREAD
   init_thread_attr( &thread_attr, BUTTON_THREAD_PRIORITY );
   status = pthread_create( &thread, &thread_attr, button_thread, NULL );
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY("Main:Err_button_thread",status);
   }
#endif

#if ENABLE_IIC_THREAD
   ////////////////////////////////////////
   // Code for setting up the I2C Thread
   ////////////////////////////////////////
   init_thread_attr( &thread_attr, IIC_THREAD_PRIORITY );
   status = pthread_create( &thread, &thread_attr, i2c_thread, &i2c_comm );
   if (status != XST_SUCCESS) {
      DBG_LOG_ENTRY("Main:Err_i2c_thread",status);
   }
#endif

   ////////////////////////////////////////
   // On LCD indicate if master or slave
   ////////////////////////////////////////
   VTprintLCD("I am the Master ",1);

#if ENABLE_SENSOR_TIMER
   init_counter( &timer_3, SENSOR_TIMER, SENSOR_INTERVAL );
   timer_start( &timer_3, SENSOR_TIMER );
#endif

   ////////////////////////////////////////
   // Wait on status messages from the i2c
   // For the master, we note when msgs are
   // send, on the slave, we recieve the msgs
   // sent by the internal status queue
   ////////////////////////////////////////

   char str[20];
   int  msgs_recieved = 0;

   while( 1 )
   {
      enum i2c_status old_status = 0xFF; // invalid status
      i2c_status_msg in_msg;
      msgs_recieved++;

      msgrcv_em( thread_qs.i2c_out_status, &in_msg, sizeof(in_msg),
                 "main recv failed" );

      DBG_CHECK_IN();

      switch( in_msg.slave_addr )
      {
      case WALL_LCD_ADDR:
         lcd_handle_i2c( in_msg.status );
         break;

      case LOCONET_PIC_ADDR:
         loconet_handle_i2c( in_msg.status );
         break;

      case SENSOR_PIC_ADDR:
         sensor_handle_i2c( in_msg.status );
         break;

      default:
         break;
      }

      //sprintf( str, "Master   %4d ", msgs_recieved );
      //VTprintLCD( str, 1 );
      if( in_msg.status != old_status )
      {
         sprintf( str, "I2C status %3x ", in_msg.status );
         VTprintLCD( str, 2 );
      }
      old_status = in_msg.status;
   }
}

void sensor_handle_i2c( enum i2c_status status )
{
   static enum i2c_status state = I2C_SEND_COMPLETE;
   if( state == status )
      return;

   train_msg out_msg;

   switch( status )
   {
   case I2C_SLAVE_NO_ACK:
      out_msg.type = SENSOR_DISCONNECTED;
      lcd_display_error( "Sensor Disconnected" );
      break;
   case I2C_RECV_COMPLETE:
      out_msg.type = SENSOR_RECONNECTED;
      break;
   default:
      break;
   }

   msgsnd_e( thread_qs.train_thread, &out_msg, sizeof(out_msg) );

   state = status;
}
