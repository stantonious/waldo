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

BUS = 1
MAG_ADDR=0x35
MAG_CONF_ADDR=0x10
MAG_MOD1_ADDR=0x11
MAG_TRIG_ADDR=0x20

trig_bytes = [0b00100000]


FULL_MULT = 1.0/7.7
TEMP_OFFSET = 1180
TEMP_MULT = .24
TEMP_25 = 25

pi = None
h = None
negate_field = True


def init():
    global pi
    pi = pigpio.pi()
    global h
    h = pi.i2c_open(BUS,MAG_ADDR)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(MAG_GPIO, GPIO.IN)


def start():
    if h:
        pi.i2c_write_byte_data(h,MAG_CONF_ADDR,0b00000000)
        pi.i2c_write_byte_data(h,MAG_MOD1_ADDR,0b00011001)
        time.sleep(2)
        pi.i2c_write_device(h,trig_bytes)
    else:
        #print ('error, no mag data!')
        pass

def to_rad(d):
    return d*math.pi/180.

def int_cb(chan,
        cb_t = lambda T:None,
        cb_xyz = lambda X,Y,Z:None,
        cb_ae = lambda A,E:None,
        cb_r = lambda R:None
        ):
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

    X = -X if negate_field else X
    Y = -Y if negate_field else Y
    Z = -Z if negate_field else Z
    R = math.sqrt(math.pow(X,2)+math.pow(Y,2)+math.pow(Z,2))

    T = (d[3] << 4) | (d[5] >> 4);

    _N = math.sqrt(math.pow(X,2) + math.pow(Y,2) + math.pow(Z,2)) * FULL_MULT
    _X = float(X)* FULL_MULT
    _Y = float(Y)* FULL_MULT
    _Z = float(Z)* FULL_MULT
    _T = float (T - TEMP_OFFSET) * TEMP_MULT + TEMP_25

    theta = math.acos(float(Z)/float(R))
    phi = math.atan2(float(Y),float(X))

    azimuth = math.atan2(float(Y),float(X))
    polar = math.atan2(float(Z),math.sqrt(math.pow(float(X),2) + math.pow(float(Y),2)))

    if cb_t:cb_t(_T)
    if cb_xyz:cb_xyz(X,Y,Z)
    if cb_ae:cb_ae(azimuth,polar)
    if cb_r:cb_r(R)
    time.sleep(.1)
    pi.i2c_write_device(h,trig_bytes)


def register_cbs(
    cb_t = None,
    cb_xyz = None,
    cb_ae =None,
    cb_r = None,):
    _cb = lambda c: int_cb(c,cb_t=cb_t,cb_xyz=cb_xyz,cb_ae=cb_ae,cb_r=cb_r)
    GPIO.add_event_detect(MAG_GPIO, GPIO.FALLING,
            callback=_cb )

def cleanup():
    pi.i2c_close(h)

