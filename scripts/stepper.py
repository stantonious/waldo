#!/usr/bin/python3
import RPi.GPIO as GPIO
import time

in1 = 23
in2 = 24
in3 = 25
in4 = 16

in1_2 = 17
in2_2 = 18
in3_2 = 27
in4_2 = 22

# careful lowering this, at some point you run into the mechanical limitation of how quick your motor can move
step_sleep = 0.005

step_count = 4096 * 12 # 5.625*(1/64) per step, 4096 steps is 360°

direction_1 = False # True for clockwise, False for counter-clockwise
direction_2 = False # True for clockwise, False for counter-clockwise

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
GPIO.setup( in1, GPIO.OUT )
GPIO.setup( in2, GPIO.OUT )
GPIO.setup( in3, GPIO.OUT )
GPIO.setup( in4, GPIO.OUT )

GPIO.setup( in1_2, GPIO.OUT )
GPIO.setup( in2_2, GPIO.OUT )
GPIO.setup( in3_2, GPIO.OUT )
GPIO.setup( in4_2, GPIO.OUT )

# initializing
GPIO.output( in1, GPIO.LOW )
GPIO.output( in2, GPIO.LOW )
GPIO.output( in3, GPIO.LOW )
GPIO.output( in4, GPIO.LOW )
GPIO.output( in1_2, GPIO.LOW )
GPIO.output( in2_2, GPIO.LOW )
GPIO.output( in3_2, GPIO.LOW )
GPIO.output( in4_2, GPIO.LOW )

motor_pins = [in1,in2,in3,in4]
motor_pins_2 = [in1_2,in2_2,in3_2,in4_2]
motor_step_counter_1 = 0 ;
motor_step_counter_2 = 0 ;

def cleanup():
    GPIO.output( in1, GPIO.LOW )
    GPIO.output( in2, GPIO.LOW )
    GPIO.output( in3, GPIO.LOW )
    GPIO.output( in4, GPIO.LOW )
    GPIO.output( in1_2, GPIO.LOW )
    GPIO.output( in2_2, GPIO.LOW )
    GPIO.output( in3_2, GPIO.LOW )
    GPIO.output( in4_2, GPIO.LOW )
    GPIO.cleanup()

# the meat
try:
    i = 0
    for i in range(step_count):
        if i % 4096 == 0:
            direction_1 = not direction_1
        if i % 2048 == 0:
            direction_2 = not direction_2
        for pin in range(0, len(motor_pins)):
            GPIO.output( motor_pins[pin], step_sequence[motor_step_counter_1][pin] )
        for pin in range(0, len(motor_pins_2)):
            GPIO.output( motor_pins_2[pin], step_sequence[motor_step_counter_2][pin] )
        if direction_1==True:
            motor_step_counter_1 = (motor_step_counter_1 - 1) % len(step_sequence) 
        elif direction_1==False:
            motor_step_counter_1 = (motor_step_counter_1 + 1) %  len(step_sequence)
        if direction_2==True:
            motor_step_counter_2 = (motor_step_counter_2 - 1) % len(step_sequence) 
        elif direction_2==False:
            motor_step_counter_2 = (motor_step_counter_2 + 1) %  len(step_sequence)
        else: # defensive programming
            print( "uh oh... direction should *always* be either True or False" )
            cleanup()
            exit( 1 )
        time.sleep( step_sleep )

except KeyboardInterrupt:
    cleanup()
    exit( 1 )

cleanup()
exit( 0 )

