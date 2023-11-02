#ifndef MAG_TASK_H_
#define MAG_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "semphr.h"

/*******************************************************************************
* Macros
********************************************************************************/

/* Task priority and stack size for the Motion sensor task */
#define TASK_MAG_PRIORITY     (configMAX_PRIORITIES - 1)
#define TASK_MAG_STACK_SIZE   (512u)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_mag_task(cyhal_i2c_t* i2c,SemaphoreHandle_t* i2c_semaphore);

#endif 