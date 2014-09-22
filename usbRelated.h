/*-----------------------------------------------------------------------------
/ Todo: Fill this header ...
/----------------------------------------------------------------------------*/
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h> 
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "usbdrv.h"

uint8_t msgbuf[64];

void usb_init();
