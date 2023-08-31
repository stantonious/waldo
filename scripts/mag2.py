#!/usr/bin/python3
import RPi.GPIO as GPIO
import time
import math
import threading
import argparse
import pigpio
import struct
import ctypes

import sys

MAG_GPIO = 26
parser = argparse.ArgumentParser()
args = parser.parse_args()

BUS = 1
MAG_ADDR=0x35
MAG_CONF_ADDR=0x10
MAG_MOD1_ADDR=0x11
MAG_TRIG_ADDR=0x20

trig_bytes = [0b00100000]

pi = pigpio.pi()

h = pi.i2c_open(BUS,MAG_ADDR)

FULL_MULT = 1.0/7.7
TEMP_OFFSET = 1180
TEMP_MULT = .24
TEMP_25 = 25


def to_rad(d):
    return d*math.pi/180.
def int_cb(chan):
    b,d = pi.i2c_read_device(h,7)

    d=d[:]

    X = ((d[0] << 8) | (d[4] & 0xF0)) >> 4;
    Y = ((d[1] << 8) | ((d[4] & 0x0F) << 4)) >> 4;
    Z = ((d[2] << 8) | ((d[5] & 0x0F) << 4)) >> 4;

    if X & 0x800:
        X = (X ^ 0xFFF) + 1
        X = -(X & 0x7FF)
    if Y & 0x800:
        Y = (Y ^ 0xFFF) + 1
        Y = -(Y & 0x7FF)
    if Z & 0x800:
        Z = (Z ^ 0xFFF) + 1
        Z = -(Z & 0x7FF)

    X = -X
    Y = -Y
    Z = -Z
    R = math.sqrt(math.pow(X,2)+math.pow(Y,2)+math.pow(Z,2))



    T = (d[3] << 4) | (d[5] >> 4);

    _N = math.sqrt(math.pow(X,2) + math.pow(Y,2) + math.pow(Z,2)) * FULL_MULT
    _X = float(X)* FULL_MULT
    _Y = float(Y)* FULL_MULT
    _Z = float(Z)* FULL_MULT
    _T = float (T - TEMP_OFFSET) * TEMP_MULT + TEMP_25

    theta = math.acos(float(Z)/float(R))
    phi = math.atan2(float(Y),float(X))
    print ('theta,phi',180./math.pi*theta,180./math.pi*phi)
    print ('R',R)

    #print (X,Y,Z)
    #print (_N)
    #print('temp   ',_T)
    #print (_X,_Y,_Z)

    azimuth = math.atan2(float(Y),float(X))
    polar = math.atan2(float(Z),math.sqrt(math.pow(float(X),2) + math.pow(float(Y),2)))


    #print (azimuth,polar)
    #print ('polar','az',polar,azimuth)
    #print (float(X),float(Y),float(Z),_T)
    time.sleep(.1)
    pi.i2c_write_device(h,trig_bytes)


GPIO.setmode(GPIO.BCM)
GPIO.setup(MAG_GPIO, GPIO.IN)
GPIO.add_event_detect(MAG_GPIO, GPIO.FALLING,
            callback=int_cb )
if h:
    print ('we going')
    pi.i2c_write_byte_data(h,MAG_CONF_ADDR,0b00000000)
    pi.i2c_write_byte_data(h,MAG_MOD1_ADDR,0b00011001)

    time.sleep(2)

    pi.i2c_write_device(h,trig_bytes)



time.sleep(150)
pi.i2c_close(h)
exit( 0 )

