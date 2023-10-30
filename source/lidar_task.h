#ifndef LIDAR_TASK_H_
#define LIDAR_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "semphr.h"

/*******************************************************************************
* Macros
********************************************************************************/

/* Task priority and stack size for the Motion sensor task */
#define TASK_LIDAR_PRIORITY     (configMAX_PRIORITIES - 1)
#define TASK_LIDAR_STACK_SIZE   (512u)

/* I2C Clock frequency in Hz */
#define I2C_CLK_FREQ_HZ                 (1000000u)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_lidar_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *i2c_semaphore);

#endif /* MOTION_TASK_H_ */

/* [] END OF FILE */
