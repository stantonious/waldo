#!/usr/bin/python3
import RPi.GPIO as GPIO
from smbus2 import SMBus
import time
import math
import threading
import argparse
import waldo_mag as wm


MAX_STEPS_PER_REV = 4096 # 360 degs
deg_per_step = 360./MAX_STEPS_PER_REV

parser = argparse.ArgumentParser()
parser.add_argument('--clockwise',action='store_true')
parser.add_argument('--xy',action='store_true')
parser.add_argument('--yz',action='store_true')
parser.add_argument('--xy_period',type=int,default=MAX_STEPS_PER_REV)
parser.add_argument('--yz_period',type=int,default=MAX_STEPS_PER_REV)
parser.add_argument('--xy_step_period',type=int,default=1)
parser.add_argument('--yz_step_period',type=int,default=1)
parser.add_argument('--steps',type=int,default=1)
args = parser.parse_args()

# GPIO
xy_motor_pins = [23,24,25,16]
yz_motor_pins = [17,18,27,22]


# careful lowering this, at some point you run into the mechanical limitation of how quick your motor can move
motor_step_sleep = 0.001

#yz_motor_dir = False # True for clockwise, False for counter-clockwise

# defining stepper motor sequence (found in documentation http://www.4tronix.co.uk/arduino/Stepper-Motors.php)
step_sequence = [[1,0,0,1],
                 [1,0,0,0],
                 [1,1,0,0],
                 [0,1,0,0],
                 [0,1,1,0],
                 [0,0,1,0],
                 [0,0,1,1],
                 [0,0,0,1]]

# setting up
GPIO.setmode( GPIO.BCM )
for _n in xy_motor_pins:
    GPIO.setup( _n, GPIO.OUT )
    GPIO.output( _n, GPIO.LOW )
for _n in yz_motor_pins:
    GPIO.setup( _n, GPIO.OUT )
    GPIO.output( _n, GPIO.LOW )

def cleanup():
    for _n in xy_motor_pins:
        GPIO.output( _n, GPIO.LOW )
    for _n in yz_motor_pins:
        GPIO.output( _n, GPIO.LOW )
    GPIO.cleanup()


def degs_to_rads(degs):
    return degs * math.pi/180.


def step_clockwise(step_idx,num=1):
    for i in range(num):
        for pin in range(0, len(yz_motor_pins)):
            time.sleep(.001)
            GPIO.output( yz_motor_pins[pin], step_sequence[step_idx][pin] )
        step_idx = (step_idx - 1) % len(step_sequence) 
    return step_idx

def step_counter_clockwise(step_idx,num=1):
    for i in range(num):
        for pin in range(0, len(yz_motor_pins)):
            time.sleep(.001)
            GPIO.output( yz_motor_pins[pin], step_sequence[step_idx][pin] )
        step_idx = (step_idx + 1) %  len(step_sequence)
    return step_idx



def auto_align():
    global current_r
    step_size = 40
    last_r = -1
    last_step = -1 # 0 no step, -1 counter, 1 clockwise
    step_idx = 0
    while step_size > 0:
        if current_r > last_r:
            last_r = current_r
            # redo
            if last_step == -1:
                step_idx = step_counter_clockwise(step_idx,num=step_size)
                last_step = -1
            else:
                step_idx = step_clockwise(step_idx,num=step_size)
                last_step = 1
        else:
            last_r = current_r
            step_size -= 1
            if last_step == -1:
                step_idx = step_clockwise(step_idx,num=step_size)
                last_step = 1
            else:
                step_idx = step_counter_clockwise(step_idx,num=step_size)
                last_step = -1
        time.sleep(.05)

current_r = 0 


def run_motors(
        total_steps,
        xy_run,
        yz_run,
        xy_period,
        yz_period,
        xy_step_period,
        yz_step_period,
        xy_pins,
        yz_pins,
        xy_direction,
        yz_direction,
        step_sleep):

    step_idx=0


    def signal(i,period,step_period,pins,direction,step_idx):
        if i % step_period != 0:
            return direction,step_idx,False
        if i % (period * step_period) == 0:
            direction = not direction
        if direction==True:
            step_idx = (step_idx - 1) % len(step_sequence) 
        else:
            step_idx = (step_idx + 1) %  len(step_sequence)
        for pin in range(0, len(pins)):
            GPIO.output( pins[pin], step_sequence[step_idx][pin] )
        return direction,step_idx,True

    # LIDAR
    bus = SMBus(1)
    LIDAR_ADDR=0x10
    LIDAR_DIST_L=0
    LIDAR_DIST_H=1


    xy_step_idx = 0
    yz_step_idx = 0
    xy_dir = xy_direction
    yz_dir = yz_direction

    xy_steps_in_dir = 0
    yz_steps_in_dir = 1024 #auto aligned up 
    xy_degs_in_dir = 0.
    yz_degs_in_dir = 0.
    xy_stepped = False
    yz_stepped = False

    for i in range(1,total_steps):
        if (xy_run): 
            xy_dir,xy_step_idx,xy_stepped = signal(i,xy_period,xy_step_period,xy_pins,xy_dir,xy_step_idx);
        if (yz_run): 
            yz_dir,yz_step_idx,yz_stepped = signal(i,yz_period,yz_step_period,yz_pins,yz_dir,yz_step_idx);
        time.sleep( step_sleep )

        h_dist = bus.read_byte_data(LIDAR_ADDR,LIDAR_DIST_H)
        l_dist = bus.read_byte_data(LIDAR_ADDR,LIDAR_DIST_L)
        t_dist = (h_dist * 0xFF) + l_dist #cm 


        if xy_stepped: xy_steps_in_dir += 1 if xy_dir == xy_direction else -1
        if yz_stepped: yz_steps_in_dir += 1 if yz_dir == yz_direction else -1
        xy_rads_in_dir = degs_to_rads(xy_steps_in_dir * deg_per_step)
        yz_rads_in_dir = degs_to_rads(yz_steps_in_dir * deg_per_step)

        # xy
        x = t_dist * math.cos(yz_rads_in_dir) * math.sin(xy_rads_in_dir)
        y = t_dist * math.cos(yz_rads_in_dir) * math.cos(xy_rads_in_dir)
        z = t_dist * math.sin(yz_rads_in_dir)

        if abs(y) < .001 and abs(x< .001):
            continue

        if yz_dir==yz_direction: print (' '.join([str(x),str(y),str(z)]))

try:
    #MAG 
    def set_r(r):
        global current_r
        current_r = r
    wm.init()
    wm.register_cbs(cb_r=set_r,)
                    #cb_xyz=lambda x,y,z:print(x,y,z),
                    #cb_ae=lambda a,e:print(a,e))
    wm.start()

    auto_align()

    m_t = threading.Thread(target=run_motors,args=(
        args.steps,
        args.xy,
        args.yz,
        args.xy_period,
        args.yz_period,
        args.xy_step_period,
        args.yz_step_period,
        xy_motor_pins,
        yz_motor_pins,
        args.clockwise,
        args.clockwise,
        motor_step_sleep))

    m_t.start()
    m_t.join()

except KeyboardInterrupt:
    cleanup()
    exit( 1 )

cleanup()
exit( 0 )

