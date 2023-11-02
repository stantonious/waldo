
#ifndef TLI493_W2BW_H
#define TLI493_W2BW_H

#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "semphr.h"


cy_rslt_t init_sensor(cyhal_i2c_t *i2c, cyhal_gpio_event_callback_t cb);

void update_mag_vals(cyhal_i2c_t *i2c);

float get_X();
float get_Y();
float get_Z();
float get_R();
float get_T();
float get_N();
float get_azimuth();
float get_polar();

#endif