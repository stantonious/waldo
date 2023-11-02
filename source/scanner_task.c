
#include "scanner_task.h"
#include "uln2003.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cyabs_rtos.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "core_cm4.h"
#include "math.h"
#include "tf-luna.h"
#include "mag_task.h"
#include "sample_publisher_task.h"

extern cy_queue_t meas_q;
extern cy_queue_t scanner_command_data_q;

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

#define YZ_RUN true
#define YZ_DIR true
#define YZ_STEP_PERIOD 1
#define YZ_PERIOD 1024

#define XY_RUN true
#define XY_DIR true
// #define XY_STEP_PERIOD YZ_PERIOD * 2    // 2 full cycle 2 channels (up/down)
#define XY_STEP_PERIOD 4
#define XY_PERIOD XY_STEP_PERIOD * 4096 // full 360
#define TOTAL_STEPS XY_PERIOD           // run 1 full xy period

#define STEP_DELAY 5

static void scanner_task(void *pvParameters)
{
    void **args = pvParameters;
    cyhal_i2c_t *i2c = (cyhal_i2c_t *)args[0];
    SemaphoreHandle_t *sema = args[1];
    uint8_t yz_step_idx = 0;
    uint8_t xy_step_idx = 0;
    float dist = 0.;
    scanner_command_data_t scanner_cmd;

    // Auto align
    // cy_rslt_t result;
    // result = cy_rtos_get_queue(&scanner_command_data_q, &scanner_cmd, CY_RTOS_NEVER_TIMEOUT, false);
    // Time for network to init
    vTaskDelay(10000);
    auto_align(&xy_step_idx, &yz_step_idx);

    for (;;)
    {
        // xy_step_idx = step_anticlockwise(xy_step_idx, XY_MOTOR_ID);
        // xSemaphoreTake(*sema, portMAX_DELAY);
        // xSemaphoreGive(*sema);
        run_motors(i2c, xy_step_idx, yz_step_idx, TOTAL_STEPS, XY_RUN, YZ_RUN, XY_PERIOD, YZ_PERIOD, XY_STEP_PERIOD, YZ_STEP_PERIOD, XY_DIR, YZ_DIR, STEP_DELAY);
        printf("dist %f", dist);
    }
}

#define STEP_SIZE 32
void auto_align(uint8_t *xy_step_idx, uint8_t *yz_step_idx)
{
    float last_r = -1;
    int last_step = -1; // 0 - no step; -1 - anti 1 clock
    *xy_step_idx = 0;
    *yz_step_idx = 0;
    float xy_max_r = 0;
    int arm_mag_idx = 0;

    int max_step_idx = -1;
    int num_steps = 4096 / STEP_SIZE;

    for (int _xy_step = 0; _xy_step < num_steps; _xy_step++)
    {
        *xy_step_idx = step(*xy_step_idx, XY_MOTOR_ID, false, STEP_SIZE);
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
        *xy_step_idx = step(*xy_step_idx, XY_MOTOR_ID, true, STEP_SIZE);
    }

    // align yz
    for (int tries = 0; tries < 3; tries++)
    {
        int max_align_steps = 32;

        while (max_align_steps > 0)
        {

            max_align_steps -= 1;
            if (get_R() > last_r)
            {
                last_r = get_R();
                // redo
                if (last_step == -1)
                {
                    *yz_step_idx = step(*yz_step_idx, YZ_MOTOR_ID, false, max_align_steps);
                    last_step = -1;
                }
                else
                {

                    *yz_step_idx = step(*yz_step_idx, YZ_MOTOR_ID, true, max_align_steps);
                    last_step = 1;
                }
            }
            else
            {
                last_r = get_R();
                if (last_step == -1)
                {

                    *yz_step_idx = step(*yz_step_idx, YZ_MOTOR_ID, true, max_align_steps);
                    last_step = 1;
                }
                else
                {

                    *yz_step_idx = step(*yz_step_idx, YZ_MOTOR_ID, false, max_align_steps);
                    last_step = -1;
                }
            }
            vTaskDelay(2);
        }
    }
}

#define MAX_STEPS_PER_REV 4096 // 360 degs
#define DEG_PER_STEP 360. / MAX_STEPS_PER_REV
#define DEG_TO_RAD(d) d *M_PI / 180.
#define DIST_MIN 2
#define DIST_MAX 7000

void process_step(
    uint32_t period_idx,
    uint32_t period,
    uint32_t step_period,
    int motor_id,
    bool *direction,
    uint8_t *step_idx,
    bool *stepped)
{
    if (period_idx % step_period != 0)
    {
        *direction = *direction;
        *step_idx = *step_idx;
        *stepped = false;
        return;
    }
    if (period_idx != 0 && (period_idx % (period * step_period) == 0))
    {
        *direction = !(*direction); // switch directions
    }
    *step_idx = step(*step_idx, motor_id, *direction, 1);
    *direction = *direction;
    *step_idx = *step_idx;
    *stepped = true;
    return;
}

void run_motors(
    cyhal_i2c_t *i2c,
    uint8_t starting_xy_step_idx,
    uint8_t starting_yz_step_idx,
    uint32_t total_steps,
    bool xy_run,
    bool yz_run,
    uint32_t xy_period,
    uint32_t yz_period,
    uint32_t xy_step_period,
    uint32_t yz_step_period,
    bool xy_direction,
    bool yz_direction,
    uint16_t step_sleep)
{
    measurement_batch meas_batch;

    float dist;
    uint8_t xy_step_idx = starting_xy_step_idx;
    uint8_t yz_step_idx = starting_yz_step_idx;
    bool xy_dir = xy_direction;
    bool yz_dir = yz_direction;

    uint32_t xy_steps_in_dir = 0;
    uint32_t yz_steps_in_dir = 1024;
    bool xy_stepped = false;
    bool yz_stepped = false;

    uint16_t num_meas = 0;

    for (int i = 0; i < total_steps; i++)
    {
        if (xy_run)
        {
            process_step(i, xy_period, xy_step_period, XY_MOTOR_ID, &xy_dir, &xy_step_idx, &xy_stepped);
        }
        if (yz_run)
        {
            process_step(i, yz_period, yz_step_period, YZ_MOTOR_ID, &yz_dir, &yz_step_idx, &yz_stepped);
        }

        vTaskDelay(step_sleep);

        get_dist(i2c, &dist);

        // filter
        if (dist < DIST_MIN || dist > DIST_MAX)
        {
            continue;
        }

        if (xy_stepped)
        {
            xy_steps_in_dir += (xy_dir == xy_dir ? 1 : -1);
        }
        if (yz_stepped)
        {

            yz_steps_in_dir += (yz_dir == yz_dir ? 1 : -1);
        }

        float xy_rads_in_dir = DEG_TO_RAD(xy_steps_in_dir * DEG_PER_STEP);
        float yz_rads_in_dir = DEG_TO_RAD(yz_steps_in_dir * DEG_PER_STEP);

        float x = dist * cos(yz_rads_in_dir) * sin(xy_rads_in_dir);
        float y = dist * cos(yz_rads_in_dir) * cos(xy_rads_in_dir);
        float z = dist * sin(yz_rads_in_dir);

        if (num_meas == BATCH_SIZE)
        {
            cy_rslt_t r = 0;
            r = cy_rtos_put_queue(&meas_q, &meas_batch, 0, true);

            if (r != CY_RSLT_SUCCESS)
            {
                printf("doh");
            }

            num_meas = 0;
        }

        meas_batch[num_meas].xy_step = xy_steps_in_dir;
        meas_batch[num_meas].yz_step = yz_steps_in_dir;
        meas_batch[num_meas].x = x;
        meas_batch[num_meas].y = y;
        meas_batch[num_meas].z = z;

        num_meas++;

        /* Capture measurement


        if yz_dir==yz_direction:
            a_fd.write(' '.join([str(x),str(y),str(z)]))
            a_fd.write('\n')
        else:
            b_fd.write(' '.join([str(x),str(y),str(z)]))
            b_fd.write('\n')
        #if xy_stepped or yz_stepped:
        #    print(' '.join([str(x), str(y), str(z)]))
        */
    }
}