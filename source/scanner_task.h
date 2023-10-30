
#ifndef SCANNER_TASK_H_
#define SCANNER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"
#include "semphr.h"

/*******************************************************************************
* Macros
********************************************************************************/

/* Task priority and stack size for the Motion sensor task */
#define SCAN_TASK_PRIORITY     (configMAX_PRIORITIES - 1)
#define SCAN_TASK_STACK_SIZE   (512u)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_scanner_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *i2c_semaphore);

#endif /* SCANNER_TASK_H_ */