/****************************************************************************\
 *
 * Jonathan Doman
 * Group 9
 * ECE 4534, Spring 2011
 *
 * Modifications:
 * - Removed unused functions and unimplemented prototypes
 * - Changed LCD routines to use OS sleeps instead of inefficient and
 *   inaccurate busy wait sleeps
 * - Made thread safe
 *
\****************************************************************************/
#include <xmk.h>
#include <xparameters.h>
#include <stdio.h>
#include <pthread.h>
#include <xgpio.h>

#include "VTio.h"

pthread_mutex_t vt_lcd_mutex = PTHREAD_MUTEX_INITIALIZER;

// Values for the MODE parameter to lcd_write()
#define LCD_DATA   4
#define LCD_INST   0
#define LCD_4BITMODE 16

// Initialize board
// Set up LCD and any ports
void VTinit(void)
{
   VTinitialize_ports();
   VTinitLCD();
}

//
// Intialize the data ports
//
void VTinitialize_ports() {
   // In the LCD GPIO, the 2nd port contains the outputs 
   // MSB to LSB {LCD_E, LCD_RS, LCD_RW, 0}
   XGpio_mSetDataDirection(XPAR_LCD_BASEADDR, 2, 0x0);
   // The first contains the bidirectional 4-bit data bus
   XGpio_mSetDataDirection(XPAR_LCD_BASEADDR, 1, 0xffffffff);
}

//
// Write to LCD display
//   c: 8-bit character to write
// mode: 'odd'=data, 'even'=instruction.  If mode > 4, 4-bit write only.
//
void VTlcd_write( char c, char mode ) 
{
   pthread_mutex_lock( &vt_lcd_mutex );
   char chigh = c >> 4; 

   // Set LCD data bus to output
   XGpio_mSetDataDirection(XPAR_LCD_BASEADDR, 2, 0);

   XGpio_mWriteReg(XPAR_LCD_BASEADDR, 0, 0);
   // Transfer high nibble before low nibble
   XGpio_mWriteReg(XPAR_LCD_BASEADDR, 8, chigh);
   // Strobe E
   XGpio_mWriteReg(XPAR_LCD_BASEADDR, 0, 8|mode);
   XGpio_mWriteReg(XPAR_LCD_BASEADDR, 0, mode);
   sleep( 1 );
   if (mode < 16) {
      XGpio_mWriteReg(XPAR_LCD_BASEADDR, 8, c);
      // Strobe E
      XGpio_mWriteReg(XPAR_LCD_BASEADDR, 0, 8|mode); 
      XGpio_mWriteReg(XPAR_LCD_BASEADDR, 0, mode);
      sleep( 1 );
   }
   pthread_mutex_unlock( &vt_lcd_mutex );
}

//
// Initialize the LCD 
//
void VTinitLCD()
{
   sleep(40);
   // Initialize modes
   VTlcd_write( 0x30, LCD_4BITMODE | LCD_INST);
   sleep(20);
   VTlcd_write( 0x30, LCD_4BITMODE | LCD_INST);
   sleep(1);
   VTlcd_write( 0x30, LCD_4BITMODE | LCD_INST);
   sleep(1);
   VTlcd_write( 0x20, LCD_4BITMODE | LCD_INST);
   sleep(1);
   VTlcd_write( 0x28, LCD_INST); // Function set
   VTlcd_write( 0x6, LCD_INST);  // Display set
   VTlcd_write( 0xc, LCD_INST);    // Entry mode
   VTlcd_write( 0x1, LCD_INST);  // Clear display
   sleep(8);
}

//
// Print a string to the LCD display.  If line==1, print to the first 
// line.  If line==2, print to the second line.
//
void VTprintLCD( const char *str, int line ) 
{
   char eos = 0;
   int i;

   VTlcd_write( 0x80 | ((line-1)*40), LCD_INST );

   for (i=0; i<16; i++) {
      if ( (str[i] == 0) || (eos == 1)) {
         VTlcd_write( ' ', LCD_DATA);
         eos = 1;
      } else {
         VTlcd_write( str[i], LCD_DATA);
      }
   }
}


