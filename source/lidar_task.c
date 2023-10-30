#include "lidar_task.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "core_cm4.h"
/******************************************************************************
 * Macros
 ******************************************************************************/
/* Macro to check if the result of an operation was successful. When it has
 * failed, print the error message and suspend the task.
 */
#define CHECK_RESULT(result, error_message...)    \
    do                                            \
    {                                             \
        if ((cy_rslt_t)result != CY_RSLT_SUCCESS) \
        {                                         \
            printf(error_message);                \
            vTaskSuspend(NULL);                   \
        }                                         \
    } while (0)

/*******************************************************************************
 * Global Variables
 ********************************************************************************/

static TaskHandle_t lidar_task_handle;

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
/* These static functions are used by the Motion Sensor Task. These are not
 * available outside this file. See the respective function definitions for
 * more details.
 */
static void lidar_task(void *pvParameters);
static cy_rslt_t lidar_init(void);

static SemaphoreHandle_t *i2c_semaphore_ptr;

cy_rslt_t create_lidar_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *i2c_semaphore)
{
    BaseType_t status;
    i2c_semaphore_ptr = i2c_semaphore;

    lidar_init();

    status = xTaskCreate(lidar_task, "LIDAR Task", TASK_LIDAR_STACK_SIZE,
                         i2c, TASK_LIDAR_PRIORITY, &lidar_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t)status;
}

#define I2C_SLAVE_ADDR 0x10
#define LIDAR_ADDR 16
#define PACKET_SIZE 3
#define REG_DIST_L 0
#define REG_DIST_H 1
#define REG_TEMP_L 4
#define REG_TEMP_H 5

uint32_t get_t_dist(uint8_t dist_l, uint8_t dist_h)
{
    return (dist_h * 0xFF) + dist_l;
}

uint32_t foo(uint32_t a)
{
    return a * a;
}

#pragma GCC push_options
#pragma GCC optimize("O0")
static void lidar_task(void *pvParameters)
{
    /* Status variable to indicate the result of various operations */
    cy_rslt_t result;

    cyhal_i2c_t *i2c = (cyhal_i2c_t *)pvParameters;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("starting");

    printf(" successfully initialized.\n");

    uint8_t buffer[PACKET_SIZE] = {0};
    uint32_t t_dist;


    for (;;)
    {
        vPortEnterCritical();
        xSemaphoreTake(*i2c_semaphore_ptr, portMAX_DELAY);

        if (CY_RSLT_SUCCESS == cyhal_i2c_master_mem_read(i2c, LIDAR_ADDR, REG_DIST_L, 1, buffer, 1, 10))
        {
            printf("success");
        }
        else
        {
            printf("doh");
        }
        if (CY_RSLT_SUCCESS == cyhal_i2c_master_mem_read(i2c, LIDAR_ADDR, REG_DIST_H, 1, buffer + 1, 1, 10))
        {
            printf("success");
        }
        else
        {
            printf("doh");
        }

        xSemaphoreGive(*i2c_semaphore_ptr);
        vPortExitCritical();

        t_dist = get_t_dist(buffer[0], buffer[1]);

        foo(t_dist);
        vTaskDelay(5);
    }
}
#pragma GCC pop_options

static cy_rslt_t lidar_init(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    return result;
}
