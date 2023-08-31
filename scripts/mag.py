#!/usr/bin/python3
import RPi.GPIO as GPIO
from smbus2 import SMBus
import time
import math
import threading
import argparse

import signal
import sys
import RPi.GPIO as GPIO

MAG_GPIO = 26
parser = argparse.ArgumentParser()
args = parser.parse_args()

# LIDAR
bus = SMBus(1)
MAG_ADDR=0x35
MAG_CONF_ADDR=0x10
MAG_MOD1_ADDR=0x11


bus.write_byte_data(MAG_ADDR,MAG_CONF_ADDR,0b00000000)
bus.write_byte_data(MAG_ADDR,MAG_MOD1_ADDR,0b00001011)
#bus.write_byte(0x00,0x00)

# Trigger
#bus.write_byte(MAG_ADDR,0x20)

#block=bus.read_i2c_block_data(MAG_ADDR,MAG_MOD1_ADDR,2)
#block = bus.read_byte_data(MAG_ADDR,0x10)
def int_cb(chan):
    meas = bus.read_block_data(MAG_ADDR,0x00,7)
    print (meas)
    #trigger
    bus.write_byte(MAG_ADDR,0x20)
    print ('sig')

GPIO.setmode(GPIO.BCM)
GPIO.setup(MAG_GPIO, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(MAG_GPIO, GPIO.FALLING,
            callback=int_cb, bouncetime=100)


time.sleep(100)
exit( 0 )

