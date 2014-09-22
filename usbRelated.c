/*-----------------------------------------------------------------------------
/ Todo: Fill this header ...
/----------------------------------------------------------------------------*/
#include "usbRelated.h"
/*---------------------------------------------------------------------------*/
extern void handleMessage();
/*---------------------------------------------------------------------------*/
PROGMEM const char usbHidReportDescriptor[22] = { /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x20,                    //   REPORT_COUNT (32)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/*---------------------------------------------------------------------------*/
static uint8_t currentAddress;
static uint8_t bytesRemaining;
/*---------------------------------------------------------------------------*/
void usb_init()
{
	usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */    
    _delay_ms(255); /* fake USB disconnect for > 250 ms */            
    usbDeviceConnect();
    sei(); /* Enable interrupts */
}
/*---------------------------------------------------------------------------*/
uint8_t usbFunctionRead(uint8_t *data, uint8_t len)
{
    uint8_t q;

    if(len > bytesRemaining)
    {
        len = bytesRemaining;
    }
        
    /* Copy the data from our global msgbuf array to temp data array */
    for (q = 0; q < len; ++q)
    {        
        data[q] = msgbuf[q+currentAddress];     
    }

    currentAddress += len;
    bytesRemaining -= len;

    return len;
}
/*---------------------------------------------------------------------------*/
uint8_t usbFunctionWrite(uint8_t *data, uint8_t len)
{
    uint8_t q;

    /* end of transfer */
    if(bytesRemaining == 0)
    {
        handleMessage();
        return 1;
    }

    if(len > bytesRemaining)
    {
        len = bytesRemaining;
    }
        
    /* Copy the data from temp buffer to our global msgbuf array */
    for (q = 0; q < len; ++q)
    {
        msgbuf[q+currentAddress] = data[q];
    }
    
    currentAddress += len;
    bytesRemaining -= len;

    /* end of transfer */
    if(bytesRemaining == 0)
    {
        handleMessage();        
        return 1;
    }

    return bytesRemaining == 0; /* return 1 if this was the last chunk */
}
/*---------------------------------------------------------------------------*/
usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
    usbRequest_t *rq = (void *)data;

    /* HID class request */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    { 
        /* wValue: ReportType (highbyte), ReportID (lowbyte) */
        if(rq->bRequest == USBRQ_HID_GET_REPORT)  
        {
            /* since we have only one report type, we can ignore the report-ID */
            bytesRemaining = 32;
            currentAddress = 0;
            return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        }
        else if(rq->bRequest == USBRQ_HID_SET_REPORT)
        {
            /* since we have only one report type, we can ignore the report-ID */
            bytesRemaining = 32;
            currentAddress = 0;
            return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
        }
    }
    else
    {
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
static void calibrateOscillator(void)
{
    uint8_t step = 128;
    uint8_t trialValue = 0, optimumValue;
    int x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
        {
            trialValue += step;
        }
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++)
    {
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
        {
            x = -x;
        }
        if(x < optimumDev)
        {
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue; 
}
/*---------------------------------------------------------------------------*/
void usbEventResetReady(void)
{
    calibrateOscillator();
    eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}
/*---------------------------------------------------------------------------*/