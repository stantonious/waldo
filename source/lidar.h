
#ifndef LIDAR_H
#define LIDAR_H

#include "FreeRTOS.h"
#include "cyhal.h"
#include "cybsp.h"
#include "semphr.h"


cy_rslt_t get_dist(cyhal_i2c_t* i2c,float* val);

#endif