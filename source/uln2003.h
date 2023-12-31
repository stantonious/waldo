
#ifndef STEPPER_MOTOR_ULN2003_H
#define STEPPER_MOTOR_ULN2003_H

#include "cybsp.h"

#define NUM_MOTOR_PINS 4
#define STEP_SEQ_LEN 8
#define XY_MOTOR_ID 0
#define YZ_MOTOR_ID 1

/*
* @brief Initialization of the Stepper Motors
*/
cy_rslt_t init_motors();

/*
* @brief Step the motor 1 time clockwise
* 
* @param step_idx Step index
* @param motor_id The id of the motor
*
* @return The next step index
*/
uint8_t step_clockwise(uint8_t step_idx, int motor_id);

/*
* @brief Step the motor 1 time anti-clockwise
* 
* @param step_idx Step index
* @param motor_id The id of the motor
*
* @return The next step index
*/
uint8_t step_anticlockwise(uint8_t step_idx, int motor_id);

/*
* @brief Step the motor many times
* 
* @param step_idx Step index
* @param motor_id The id of the motor
* @param clockwise The motor direction
* @param num The
*
* @return The next step index
*/
uint8_t step(uint8_t step_idx, int motor_id, bool clockwise, int num);

#endif