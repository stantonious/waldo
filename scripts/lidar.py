#!/usr/bin/python
from smbus2 import SMBus
import time

# Open i2c bus 1 and read one byte from address 80, offset 0
bus = SMBus(1)
LIDAR_ADDR=0x10

while 1:
    h_b = bus.read_byte_data(LIDAR_ADDR, 1)
    l_b = bus.read_byte_data(LIDAR_ADDR, 0)
    print(h_b,l_b)
    time.sleep(.001)

bus.close()

