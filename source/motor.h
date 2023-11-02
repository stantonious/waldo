
#ifndef MOTOR_H
#define MOTOR_H

#include "cybsp.h"

#define NUM_MOTOR_PINS 4
#define STEP_SEQ_LEN 8
#define XY_MOTOR_ID 0
#define YZ_MOTOR_ID 1

cy_rslt_t init_motors();

uint8_t step_clockwise(uint8_t step_idx, int motor_id);

uint8_t step_anticlockwise(uint8_t step_idx, int motor_id);

uint8_t step(uint8_t step_idx, int motor_id, bool clockwise, int num);

#endif