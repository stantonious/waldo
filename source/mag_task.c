#include "mag_task.h"
#include "tli493d-w2bw.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "core_cm4.h"
#include "math.h"

/*******************************************************************************
 * Global Variables
 ********************************************************************************/

static TaskHandle_t mag_task_handle;

/* Handle for the semaphore that locks the I2C resource while reading the
 * Motion Sensor data
 */

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
/* These static functions are used by the Motion Sensor Task. These are not
 * available outside this file. See the respective function definitions for
 * more details.
 */
static void mag_task(void *pvParameters);
static void mag_handler(void *handler_arg, cyhal_gpio_event_t event);

static void* mag_args[2];

cy_rslt_t create_mag_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *sema)
{
    BaseType_t status;
    mag_args[0] = i2c;
    mag_args[1] = sema;

    cy_rslt_t result = CY_RSLT_SUCCESS;

    status = xTaskCreate(mag_task, "MAG Task", TASK_MAG_STACK_SIZE,
                         mag_args, TASK_MAG_PRIORITY, &mag_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t)status;
}


static void mag_task(void *pvParameters)
{
    void **args = pvParameters;
    cyhal_i2c_t *i2c = (cyhal_i2c_t *)args[0];
    SemaphoreHandle_t *sema = (SemaphoreHandle_t *)args[1];

    init_sensor(i2c,mag_handler);

    printf("\x1b[2J\x1b[;H");
    printf("starting");

    printf(" successfully initialized.\n");

    for (;;)
    {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        xSemaphoreTake(*sema, portMAX_DELAY);
        update_mag_vals(i2c);
        vTaskDelay(5);
        xSemaphoreGive(*sema);
    }
}

static void mag_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* To avoid compiler warnings */
    (void)handler_arg;
    (void)event;

    /* Notify the Motion sensor task */
    xTaskNotifyFromISR(mag_task_handle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
