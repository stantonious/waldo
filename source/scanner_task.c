
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
#include "mag_task.h"

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

static void *scan_args[2];
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
    uint8_t yz_step_idx = 0;
    uint8_t xy_step_idx = 0;
    float dist = 0.;

    // Auto align
    auto_align(xy_step_idx, yz_step_idx);

    for (;;)
    {
        // xy_step_idx = step_anticlockwise(xy_step_idx, XY_MOTOR_ID);
        xSemaphoreTake(*sema, portMAX_DELAY);
        get_dist(i2c, &dist);
        xSemaphoreGive(*sema);
        vTaskDelay(1);
        printf("dist %f", dist);
    }
}

#define STEP_SIZE 32
void auto_align(uint8_t xy_step_idx, uint8_t yz_step_idx)
{
    float last_r = -1;
    int last_step = -1; // 0 - no step; -1 - anti 1 clock
    xy_step_idx = 0;
    yz_step_idx = 0;
    float xy_max_r = 0;
    int arm_mag_idx = 0;

    int max_step_idx = -1;
    int num_steps = 4096 / STEP_SIZE;

    for (int _xy_step = 0; _xy_step < num_steps; _xy_step++)
    {
        xy_step_idx = step(xy_step_idx, XY_MOTOR_ID, false, STEP_SIZE);
        if (get_R() > xy_max_r)
        {
            xy_max_r = get_R();
            max_step_idx = _xy_step;
            printf("MAX %f", xy_max_r);
        }
    }

    arm_mag_idx = (max_step_idx + (num_steps / 2)) % num_steps;

    // print('idx', max_step_idx, arm_mag_idx, num_steps, num_steps - arm_mag_idx)

    for (int _xy_rev_step = 0; _xy_rev_step < (num_steps - arm_mag_idx); _xy_rev_step++)
    {
        xy_step_idx = step(xy_step_idx, XY_MOTOR_ID, true, STEP_SIZE);
    }

    // align yz
    for (int tries = 0; tries < 2; tries++)
    {
        int max_align_steps = 32;

        while (max_align_steps > 0)
        {

            if (get_R() > last_r)
            {
                last_r = get_R();
                // redo
                if (last_step == -1)
                {
                    yz_step_idx = step(yz_step_idx, YZ_MOTOR_ID, false, max_align_steps);
                    last_step = -1;
                }
                else
                {

                    yz_step_idx = step(yz_step_idx, YZ_MOTOR_ID, true, max_align_steps);
                    last_step = 1;
                }
            }
            else
            {
                last_r = get_R();
                max_align_steps -= 1;
                if (last_step == -1)
                {

                    yz_step_idx = step(yz_step_idx, YZ_MOTOR_ID, true, max_align_steps);
                    last_step = 1;
                }
                else
                {

                    yz_step_idx = step(yz_step_idx, YZ_MOTOR_ID, false, max_align_steps);
                    last_step = -1;
                }
            }
            cyhal_system_delay_us(1000);
        }
    }
}