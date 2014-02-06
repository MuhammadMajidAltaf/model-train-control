#ifndef VTIO_H
#define VTIO_H

// must be called at the beginning of the program
void VTinit();

// Must be called before using LCD
// It is included in VTinit()
void VTinitLCD();

// Called to init ports
// It is included in VTinit()
void VTinitialize_ports();

// Print on the LCD
// Input is a char string (16 char or less)
// and the line to print to (1 or 2)
void VTprintLCD( const char* str, int line );

#endif
