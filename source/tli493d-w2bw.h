
#ifndef TLI493_W2BW_H
#define TLI493_W2BW_H

#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "semphr.h"


/*
* @brief Initialize the magentic sensor 
* 
* @param i2c The i2c handle
* @param cb The callback for new measurements
*
* @return the result
*/
cy_rslt_t init_sensor(cyhal_i2c_t *i2c, cyhal_gpio_event_callback_t cb);

/*
* @brief Update the mag values
* 
* @param i2c The i2c handle
*/
void update_mag_vals(cyhal_i2c_t *i2c);

/*
* @brief Get X magnitude
* 
* @return X magnitude
*/
float get_X();
/*
* @brief Get X magnitude
* 
* @return X magnitude
*/
float get_Y();
/*
* @brief Get Y magnitude
* 
* @return Y magnitude
*/
float get_Z();
/*
* @brief Get Z magnitude
* 
* @return Z magnitude
*/
float get_R();
/*
* @brief Get R magnitude
* 
* @return R magnitude
*/
float get_T();
/*
* @brief Get T temperature
* 
* @return T 
*/
float get_N();
/*
* @brief Get azimuth
* 
* @return amazimuth
*/
float get_azimuth();
/*
* @brief Get polar
* 
* @return polar
*/
float get_polar();

#endif