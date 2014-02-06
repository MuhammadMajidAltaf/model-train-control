// Xilinx include files
#include <xmk.h>
#include <stdio.h>
#include <xstatus.h>
#include <xparameters.h>
#include <xgpio.h>
#include <sys/intr.h>
#include <mb_interface.h>
#include <string.h>

// Other include files
#include "timer.h"
#include "my_debug.h"

XTmrCtr debug_timer;

#ifdef MY_DEBUG
// variables and declarations "global" to this file
unsigned int wd_flag = 0;
typedef struct 
{
   unsigned int  num_entries;
   char           names[LOG_SIZE][NAME_SIZE+1];
   unsigned int  vals[LOG_SIZE];
   unsigned char wrapped;
   #if DEBUG_TIMESTAMP
   unsigned int timestamp[LOG_SIZE];
   #endif
} log;
static log dbg_log;

void init_log(void)
{
   dbg_log.num_entries = 0;
   dbg_log.wrapped = 0;

   int i;
   for( i=0; i < LOG_SIZE; i++ )
   {
      dbg_log.names[i][0] = '\0';
   }
}

void dbg_log_entry_internal( const char *name, unsigned int val )
{
   if( dbg_log.num_entries < LOG_SIZE)
   {
      dbg_log.names[dbg_log.num_entries][NAME_SIZE] = '\0';
      strncpy( dbg_log.names[dbg_log.num_entries], name, NAME_SIZE );
      dbg_log.vals[dbg_log.num_entries] = val;
      #if DEBUG_TIMESTAMP
      dbg_log.timestamp[dbg_log.num_entries] = XTmrCtr_GetValue( &debug_timer, 1 );
      #endif
   }

   dbg_log.num_entries++;

   #if CIRCULAR
   if (dbg_log.num_entries == LOG_SIZE) 
   {
      dbg_log.wrapped++;

      // See rant below
      dbg_log.num_entries = 0;
   }

   #endif
}

void dbg_log_entry( const char *name, unsigned int val )
{
   microblaze_disable_interrupts();
   dbg_log_entry_internal(name,val);
   microblaze_enable_interrupts();
}

void dbg_log_entry_intr( const char *name, unsigned int val )
{
   dbg_log_entry_internal(name,val);
}

void print_dbg_log(void)
{
   int   i;

   #if !CIRCULAR
   if( dbg_log.num_entries > LOG_SIZE )
   {
      xil_printf("Full Log: Logged %d messages of %d messages\n",
                 LOG_SIZE, dbg_log.num_entries );
   } 
   else
   {
      xil_printf("Logged %d messages\n",dbg_log.num_entries);
   }

   for( i=0; (i < dbg_log.num_entries) && (i < LOG_SIZE); i++ )
   {
      #if DEBUG_TIMESTAMP
      char timestamp_buf[30];
      #endif

      xil_printf("LOG(%d) <%s> <%x>", i, dbg_log.names[i], dbg_log.vals[i] );
      #if DEBUG_TIMESTAMP
      sprintf( timestamp_buf, " <%u>\n", dbg_log.timestamp[i] );
      xil_printf( "%s", timestamp_buf );
      #else
      xil_printf( "\n" );
      #endif
   }
   #else
   if( dbg_log.wrapped )
   {
      int t_i;
      xil_printf( "Log wrapped %d times\n", dbg_log.wrapped );
      t_i = dbg_log.num_entries;
      for( i = 0; i < LOG_SIZE; i++ )
      {
         xil_printf( "LOG(%d) <%s> <%x>", i, dbg_log.names[t_i], dbg_log.vals[t_i] );
         #if DEBUG_TIMESTAMP
         xil_printf( " <%d>\n", dbg_log.timestamp[t_i] );
         #else
         xil_printf( "\n" );
         #endif
         t_i = (t_i + 1) % LOG_SIZE;
      }
   }
   else
   {
      xil_printf( "Logged %d messages\n", dbg_log.num_entries );
      for( i = 0; i < dbg_log.num_entries; i++ )
      {
         xil_printf( "LOG(%d) <%s> <%x>", i, dbg_log.names[i], dbg_log.vals[i] );
         #if DEBUG_TIMESTAMP
         xil_printf( " <%d>\n", dbg_log.timestamp[i] );
         #else
         xil_printf( "\n" );
         #endif
      }
   }
#endif
   xil_printf( "End of Log\n" );
}

void wd_check_in()
{
   wd_flag = 1;
}

void wd_handler( void *dptr, Xuint8 timer_num )
{
   if( timer_num == 1 )
      return;

   if( wd_flag == 1 ) 
   {
      wd_flag = 0;

      // reset and start the timer
      XTmrCtr_Start( (XTmrCtr*)dptr, timer_num );
   }
   else
   {
      *LEDS = 0xFF;

      xil_printf("Watchdog expired\n");

      print_dbg_log();

      xil_printf("Watchdog printed the log\n");
      xil_printf("Watchdog will now loop forever\n");

      while(1);
   }
}

// I will use an ordinary timer/counter for this rather than the WDT for now
XStatus init_wd_clock( XTmrCtr* instance, int timer_device, int timer_int,
                       int timer_num, int num_seconds)
{
   XStatus status;
   status = init_timer( &debug_timer, timer_device, timer_int, wd_handler, &debug_timer );
   init_counter( &debug_timer, timer_num, num_seconds*1000 );
   timer_start( &debug_timer, timer_num );

   return status;
}

XStatus init_debug( XTmrCtr* instance, int timer_device, int timer_int,
                    int timer_num, int num_seconds)
{
   init_log();
   return init_wd_clock( instance,
                         timer_device,
                         timer_int,
                         timer_num,
                         num_seconds);
}
#endif
