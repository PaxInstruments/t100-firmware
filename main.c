/*-----------------------------------------------------------------------------
/ Todo: Fill this header ...
/----------------------------------------------------------------------------*/
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h> 
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
/*---------------------------------------------------------------------------*/
#include "usbdrv.h"
#include "string.h"
#include "digital.h"
#include "usbRelated.h"
#include "bitbang_i2c.h"
/*---------------------------------------------------------------------------*/
#define VERSION 0x02
/*---------------------------------------------------------------------------*/
void timer_init();
void initSerialNumber();
/*---------------------------------------------------------------------------*/
uint8_t thermocoupleReadout[4]; 
uint8_t coldJunctionReadout[4];
uint8_t* EE_addr = (uint8_t*)32;
volatile uint16_t timer0_counter;
/*---------------------------------------------------------------------------*/
int usbDescriptorStringSerialNumber[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_SERIAL_NUMBER_LEN),
    USB_CFG_SERIAL_NUMBER
};
/*---------------------------------------------------------------------------*/
void handleMessage()
{
    switch(msgbuf[1])
    {
        case 0: /* Version reading */
        {                        
            msgbuf[1] = 0xAA;            
            msgbuf[2] = 0x55;
            msgbuf[3] = VERSION;            
            break;
        }
        case 1: /* ADC reading */
        {            
            msgbuf[1] = thermocoupleReadout[0];
            msgbuf[2] = thermocoupleReadout[1];
            msgbuf[3] = thermocoupleReadout[2];
            msgbuf[4] = thermocoupleReadout[3];  
            msgbuf[5] = coldJunctionReadout[0];
            msgbuf[6] = coldJunctionReadout[1];
            break;
        }
        case 2: /* GPIO control */
        {
            if(msgbuf[2] == 0)
            {
                digitalWrite(B,1,LOW);
            }
            else
            {
                digitalWrite(B,1,HIGH);
            }
            break;
        } 
        case 3: /* Change serial number */
        {
            eeprom_write_byte((EE_addr+0),msgbuf[2]);
            eeprom_write_byte((EE_addr+1),msgbuf[3]);
            eeprom_write_byte((EE_addr+2),msgbuf[4]);
        }
        default:
        {
            
            break;
        }
    }    
}
/*---------------------------------------------------------------------------*/
int main(void)
{    
    uint8_t tmpReadout[4];

    initSerialNumber();
    usb_init();  
    I2C_Init();  
    timer_init();

    pinMode(B,1,OUTPUT);

    /* Set MCP9800 to 12 bit resolution */
    I2C_Start();
    I2C_Write(write_address(0x4D));
    I2C_Write(0x01);
    I2C_Write((1<<7)|(1<<6)|(1<<5));
    I2C_Stop();

    /* Set MCP9800 Register Pointer to Ambient Temperature */
    I2C_Start();
    I2C_Write(write_address(0x4D));
    I2C_Write(0x00);
    I2C_Stop();
    
    while(1)
    {        
        /*-------------------------------------------------------------------*/
        /* MCP9800: Cold junction channel */
        /*-------------------------------------------------------------------*/
        usbPoll();
        I2C_Start();
        I2C_Write(read_address(0x4D));
        tmpReadout[0] = I2C_Read(ACK);
        tmpReadout[1] = I2C_Read(NO_ACK);
        I2C_Stop();

        usbPoll();
        cli();
            coldJunctionReadout[0] = tmpReadout[0];            
            coldJunctionReadout[1] = tmpReadout[1];        
        sei();

        /*-------------------------------------------------------------------*/
        /* Small delay ...
        /*-------------------------------------------------------------------*/
        timer0_counter = 250;
        while(timer0_counter)
        {
            usbPoll();            
        }
    }

    return 0;
}
/*---------------------------------------------------------------------------*/
void timer_init()
{       
    /* Set prescale to 64 */
    TCCR0B = (1<<CS01)|(1<<CS00);

    /* enable overflow interrupt */
    TIMSK = (1<<TOV0);
}
/*---------------------------------------------------------------------------*/
ISR(TIMER0_OVF_vect,ISR_NOBLOCK)
{
    /* Resolution is almost 1ms */
    if(timer0_counter > 0)
    {
        timer0_counter--;
    }
}
/*---------------------------------------------------------------------------*/
void initSerialNumber()
{
    uint8_t eepromProblem = 0;  

    char val1 = eeprom_read_byte(EE_addr+0);
    char val2 = eeprom_read_byte(EE_addr+1);
    char val3 = eeprom_read_byte(EE_addr+2);

    // ascii 48 -> '0' , 57 -> '9'

    if((val1 < 48) || (val1 > 57))
    {
        eepromProblem = 1;
    }
    else if((val2 < 48) || (val2 > 57))
    {
        eepromProblem = 1;
    }
    else if((val3 < 48) || (val3 > 57))
    {
        eepromProblem = 1;
    }

    /* default serial number ... */
    if(eepromProblem != 0)
    {
        eeprom_write_byte((EE_addr+0),'5');
        eeprom_write_byte((EE_addr+1),'1');
        eeprom_write_byte((EE_addr+2),'2');
    }

    usbDescriptorStringSerialNumber[1] = eeprom_read_byte(EE_addr+0);
    usbDescriptorStringSerialNumber[2] = eeprom_read_byte(EE_addr+1);
    usbDescriptorStringSerialNumber[3] = eeprom_read_byte(EE_addr+2);
}
/*---------------------------------------------------------------------------*/