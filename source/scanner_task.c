
#include "scanner_task.h"
#include "motor.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "core_cm4.h"
#include "math.h"
#include "lidar.h"

/*******************************************************************************
 * Global Variables
 ********************************************************************************/

static TaskHandle_t scanner_task_handle;

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
/* These static functions are used by the Motion Sensor Task. These are not
 * available outside this file. See the respective function definitions for
 * more details.
 */
static void scanner_task(void *pvParameters);

cy_rslt_t scanner_init(cyhal_i2c_t *i2c, SemaphoreHandle_t *sema)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    return result;
}

static void* scan_args[2];
cy_rslt_t create_scanner_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *sema)
{
    BaseType_t status;
    scan_args[0] = i2c;
    scan_args[1] = sema;

    scanner_init(i2c, sema);

    status = xTaskCreate(scanner_task, "Scanner Task", SCAN_TASK_STACK_SIZE,
                         scan_args, SCAN_TASK_PRIORITY, &scanner_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t)status;
}

static void scanner_task(void *pvParameters)
{
    void **args = pvParameters;
    cyhal_i2c_t *i2c = (cyhal_i2c_t *)args[0];
    SemaphoreHandle_t *sema = args[1];
    int8_t step_idx = 0;
    float dist = 0.;

    for (;;)
    {
        step_idx = step_anticlockwise(step_idx, XY_MOTOR_ID);
        xSemaphoreTake(*sema, portMAX_DELAY);
        get_dist(i2c, &dist);
        xSemaphoreGive(*sema);
        vTaskDelay(5);
        printf("dist %f", dist);
    }
}
