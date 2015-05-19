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
#define VERSION 0x04
/*---------------------------------------------------------------------------*/
void timer_init();
void initSerialNumber();
/*---------------------------------------------------------------------------*/
uint8_t debug[3];
volatile uint8_t gainSetting;
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
            msgbuf[2] = 0xBB;            
            msgbuf[3] = 0x66;
            msgbuf[4] = VERSION;            
            break;
        }
        case 1: /* ADC reading */
        {            
            msgbuf[2] = thermocoupleReadout[0];
            msgbuf[3] = thermocoupleReadout[1];
            msgbuf[4] = thermocoupleReadout[2];    
            msgbuf[5] = thermocoupleReadout[3];            
            msgbuf[6] = coldJunctionReadout[0];
            msgbuf[7] = coldJunctionReadout[1];
            msgbuf[8] = debug[0];
            msgbuf[9] = debug[1];
            msgbuf[10] = debug[2];
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
            eeprom_write_byte((EE_addr+3),msgbuf[5]);
            eeprom_write_byte((EE_addr+4),msgbuf[6]);
            eeprom_write_byte((EE_addr+5),msgbuf[7]);
            eeprom_write_byte((EE_addr+6),msgbuf[8]);
            eeprom_write_byte((EE_addr+7),msgbuf[9]);
            eeprom_write_byte((EE_addr+8),msgbuf[10]);
            eeprom_write_byte((EE_addr+9),msgbuf[11]);            
            break;
        }
        case 4: /* Change PGA setting for MCP3421 */
        {
            gainSetting = msgbuf[2];
            break;
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
    uint8_t rv;
    uint8_t i = 0;
    uint8_t mcp3421_addr;
    uint8_t mcp9800_addr;
    uint8_t tmpReadout[4];

    /* Variables for cold junction moving average filter */
    int16_t movAvg_read;
    int8_t movAvg_ind = 0;
    int32_t movAvg_sum = 0;
    uint8_t movAvg_stabil = 0;
    int16_t movAvg_mem[8] = {0,0,0,0,0,0,0,0};    

    gainSetting = 0;
    timer0_counter = 0;

    initSerialNumber();
    usb_init();  
    I2C_Init();  
    timer_init();

    pinMode(B,1,OUTPUT);

    /*-----------------------------------------------------------------------*/
    /* Search for viable MCP3421 address options */
    /*-----------------------------------------------------------------------*/
    for(i=0x68;i<0x70;i++)
    {
        I2C_Start();
        rv = I2C_Write(write_address(i));
        I2C_Stop();

        if(rv == 0)
        {
            mcp3421_addr = i;
        }
    }

    /*-----------------------------------------------------------------------*/
    /* Search for viable MCP9800 address options */
    /*-----------------------------------------------------------------------*/
    I2C_Start();
    rv = I2C_Write(write_address(0x48));
    I2C_Stop();

    if(rv == 0)
    {
        mcp9800_addr = 0x48;
    }
    else
    {
        mcp9800_addr = 0x4D;
    }

    /*-----------------------------------------------------------------------*/
    /* Set MCP9800 to 12 bit resolution */
    /*-----------------------------------------------------------------------*/
    I2C_Start();
    I2C_Write(write_address(mcp9800_addr));    
    I2C_Write(0x01);
    I2C_Write((1<<7)|(1<<6)|(1<<5));
    I2C_Stop();

    /*-----------------------------------------------------------------------*/
    /* Set MCP9800 Register Pointer to Ambient Temperature */
    /*-----------------------------------------------------------------------*/
    I2C_Start();
    I2C_Write(write_address(mcp9800_addr));
    I2C_Write(0x00);
    I2C_Stop();
    
    while(1)
    {                
        /*-------------------------------------------------------------------*/
        /* MCP9800: Cold junction channel */
        /*-------------------------------------------------------------------*/
        usbPoll();
        I2C_Start();
        debug[0] = I2C_Write(read_address(mcp9800_addr));
        tmpReadout[0] = I2C_Read(ACK);
        tmpReadout[1] = I2C_Read(NO_ACK);        
        I2C_Stop();

        movAvg_read = ((int16_t)tmpReadout[0] << 8) + ((int16_t)tmpReadout[1]);                
        movAvg_sum -= movAvg_mem[movAvg_ind];
        movAvg_sum += movAvg_read;        
        movAvg_mem[movAvg_ind] = movAvg_read;
        
        if(movAvg_ind == 7)
        {
            movAvg_ind = 0;
            movAvg_stabil = 1;
        }
        else
        {
            movAvg_ind++;
        }

        if(movAvg_stabil == 1)
        {
            movAvg_read = movAvg_sum >> 3;    
        }        

        usbPoll();
        cli();                                
            coldJunctionReadout[0] = movAvg_read >> 8;            
            coldJunctionReadout[1] = movAvg_read & 0xFF;          
        sei();

        /*-------------------------------------------------------------------*/
        /* MCP3421: 3.75 SPS + 18 Bits + Initiate new conversion
        /*-------------------------------------------------------------------*/
        usbPoll();
        I2C_Start();
        I2C_Write(write_address(mcp3421_addr));
        I2C_Write((1<<7)|(1<<3)|(1<<2)|gainSetting);
        I2C_Stop();

        /*-------------------------------------------------------------------*/
        /* Small delay ...
        /*-------------------------------------------------------------------*/
        timer0_counter = 250;
        while(timer0_counter)
        {
            usbPoll();            
        }

        /*-------------------------------------------------------------------*/
        /* MCP3421
        /*-------------------------------------------------------------------*/
        usbPoll();
        I2C_Start();
        I2C_Write(read_address(mcp3421_addr));
        tmpReadout[0] = I2C_Read(ACK);
        tmpReadout[1] = I2C_Read(ACK);
        tmpReadout[2] = I2C_Read(ACK);
        tmpReadout[3] = I2C_Read(NO_ACK);
        I2C_Stop();

        usbPoll();
        cli();
            thermocoupleReadout[0] = tmpReadout[0];
            thermocoupleReadout[1] = tmpReadout[1];
            thermocoupleReadout[2] = tmpReadout[2];
            thermocoupleReadout[3] = tmpReadout[3];
        sei();
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
    uint8_t val[10];
    uint8_t qw = 0;

    for(qw=0;qw<10;qw++)
    {
        val[qw] = eeprom_read_byte(EE_addr+qw);

        if((val[qw] < 48) || (val[qw] > 57))
        {
            eepromProblem = 1;
        }
    }

    /* default serial number ... */
    if(eepromProblem != 0)
    {        
        for(qw=0;qw<10;qw++)
        {
            eeprom_write_byte((EE_addr+qw),'9');
        }
    }

    for(qw=0;qw<10;qw++)
    {
        usbDescriptorStringSerialNumber[qw+1] = eeprom_read_byte(EE_addr+qw);
    }
    
}
/*---------------------------------------------------------------------------*/