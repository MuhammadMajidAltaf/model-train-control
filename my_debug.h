#ifndef DEBUG_H
#define DEBUG_H

#include "main.h"
#include "timer.h"

// General behavior:
// The log system must be initialized (see specific functions below).  It
// uses one timer device; the parameters for this timer device are passed
// to the log system at initialization.  Application code makes subroutine
// calls to the log system to "log" events.  The logging code keeps these
// events in RAM.  The overhead associated with these calls is very low.
// The contents of the log are printed out when either:
//   (a) the application makes a subroutine call to do so, or
//   (b) the application fails to "checkin" within a given time period.
// When the log is printed, the application program is halted.  Printing is
// done to stdout (which will go back to an xmd terminal is you have configured
// xmd properly.
// The log system uses a timer to keep track of whether the application is
// still running properly.  When an application calls the "checkin" subroutine
// (see below), then the timer is reset.  If the application fails to call
// this checkin subroutine, then the log system assumes the application is
// no longer running properly.

// The following are compile-time settings that determine
//   the behavior of the debugging code

// (2) By setting DEBUG_TIMESTAMP > 0, each log entry will also contain
//     a timestamp indicating when it occurred.  The timestamp is the
//     number of kernel ticks since the kernel started (the function used
//     is xget_clock_ticks(), see the Xilinx documentation for details)
// Currently set to active; to inactivate, comment out the next line
#define DEBUG_TIMESTAMP 1

// (3) The number of entries retained in the log.  If the number of log
//     events exceeds this, the behavior is governed by setting (4)
#define LOG_SIZE 2000

// (4) This setting determines the behavior when the number of log events
//     exceeds LOG_SIZE.
//     if CIRCULAR is 1: the log "wraps" around and overwrites the oldest
//       event.  This has the effect of retaining the most recent LOG_SIZE
//       events.
//     if CIRCULAR is 0: the log stops recording events.  This has the effect
//       of retaining the first LOG_SIZE events.
#define CIRCULAR 0

// (5) The number of characters that are retained in the string associated
//     with each log entry.
#define NAME_SIZE 40

// The following are the macros that are used in application programs
// INIT_DEBUG -- must be called prior to any other debug calls
//   Arguments:
//     a: pointer to Tcomm_struct
//     b: timer device id
//     c: interrupt number for timer device
//     d: timer number (leave at 0), indicating which of two (possibly) timers
//        are used on the timer device
//     e: clock rate (presumed to be 50000000 unless otherwise specified)
//     f: number of seconds the log system will wait (since the last "checkin"
//        before it assumes the program is no longer functioning correctly
// DBG_LOG_ENTRY -- called to log an event, this version is not called in
//                  interrupt handlers
//   Arguments:
//     a: pointer to a character array (string)
//     b: unsigned integer
// DBG_LOG_ENTRY_INTR -- version of DBG_LOG_ENTRY() to be called in interrupt
//                       handlers
// PRINT_DBG_LOG -- called to print out the debug log, the code then stops
//   No Arguments:
// DBG_CHECK_IN -- called to "checkin" which resets the timer as described 
//                 above
//   No Arguments:

// The remainder of this file does not need to be examined or changed

#ifdef MY_DEBUG
#define INIT_DEBUG(a,b,c,d,e) init_debug(a,b,c,d,e);
#define DBG_LOG_ENTRY(a,b) dbg_log_entry(a,b);
#define DBG_LOG_ENTRY_INTR(a,b) dbg_log_entry_intr(a,b);
#define PRINT_DBG_LOG() print_dbg_log();
#define DBG_CHECK_IN() wd_check_in();
#else
#define INIT_DEBUG(a,b,c,d,e) 0
#define DBG_LOG_ENTRY(a,b)
#define DBG_LOG_ENTRY_INTR(a,b)
#define PRINT_DBG_LOG()
#define DBG_CHECK_IN()
#define DBG_COPY_LOG_ENTRY(a,b) 1
#endif

#ifdef MY_DEBUG
XStatus init_debug( XTmrCtr* instance,
                    int timer_device,
                    int timer_int,
                    int timer_num,
                    int num_seconds );
void print_dbg_log(void);
void dbg_log_entry_internal( const char *name, unsigned int val ) __attribute__ ((section(".high_access")));
void dbg_log_entry(const char *,unsigned int);
void dbg_log_entry_intr(const char *,unsigned int);
void wd_check_in(void);
void init_log();
#endif

#endif
