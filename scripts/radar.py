#!/usr/bin/python

import spidev
import time

spi = spidev.SpiDev() # create spi object
spi.open(0, 0) # open spi port 0, device (CS) 0
spi.max_speed_hz = 9600000
spi.mode = 0
STATUS_REG = 0x38 #register 56
ADC_STATUS_REG = 0x00 
HOLD_REG = 0x0a 
ALGO1_REG = 0x0d 
ALGO2_REG = 0x03 
ADC_START_REG = 0x22  #34
ADC_CONVERT_REG = 0x23 #35
ADC_STATUS_REG = 0x24 #36

ADC_RESULT_START = 51
ADC_RESULT_END = 53

#req = STATUS_REG << 1 & 0xFE
#req = HOLD_REG << 1 & 0xFE
#req = ALGO1_REG << 1 | 0x01
req = ADC_CONVERT_REG << 1 | 0x01
#req = ALGO1_REG << 1 & 0xFE 
d = 1 << 6 | 1 << 3 | 1 << 1
try:
  while True:
    #r = spi.writebytes([req,0x00,0x00]) # transfer one byte
    print('dev status',spi.xfer([STATUS_REG << 1 & 0xFE,0x00,0x00]))
    print('start band/clock',spi.xfer([ADC_START_REG << 1 | 1,0x00,0x03])) # clock and bandgap
    time.sleep(0.01) # sleep for 0.1 seconds
    adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
    while (adc_status[-1]!=1):
        adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
        print ('1 status',adc_status)
        time.sleep(0.1) # sleep for 0.1 seconds
    print('start adc',spi.xfer([ADC_START_REG << 1 | 1,0x00,0x07])) #adc
    adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
    while (adc_status[-1]!=3):
        print('start adc',spi.xfer([ADC_START_REG << 1 | 1,0x00,0x07])) #adc
        adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
        print ('2 status',adc_status)
        time.sleep(0.1) # sleep for 0.1 seconds
    print('convert',spi.xfer([ADC_CONVERT_REG << 1 | 1,0x00,0x00]))
    adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
    while (adc_status[0] != 4): #adc_result_ready
        adc_status = spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00])
        time.sleep(0.1) # sleep for 0.1 seconds

    print ('reading ADC results')
    for i in range(ADC_RESULT_START,ADC_RESULT_END +1):
        print(spi.xfer([i << 1 & 0xFE,0x00,0x00]))
        time.sleep(.001)


    print('adc status',spi.xfer([ADC_STATUS_REG << 1 & 0xFE,0x00,0x00]))
    print('start',spi.xfer([ADC_START_REG << 1 | 1,0x00,0x00]))

except KeyboardInterrupt: # Ctrl+C pressed, so…
  spi.close() # … close the port before exi
