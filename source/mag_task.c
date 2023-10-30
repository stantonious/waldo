#include "mag_task.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "core_cm4.h"
#include "math.h"

#define MAG_GPIO P10_0
#define MAG_ADDR 0x35
#define MAG_CONF_ADDR 0x10
#define MAG_MOD1_ADDR 0x11

#define FULL_MULT 1.0 / 7.7
#define TEMP_OFFSET 1180
#define TEMP_MULT .24
#define TEMP_25 25
static const uint8_t TRIG_BYTE = 0b00100000;

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
static cy_rslt_t init_mag(cyhal_i2c_t *i2c);

static void* mag_args[2];

cy_rslt_t create_mag_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *sema)
{
    BaseType_t status;
    mag_args[0] = i2c;
    mag_args[1] = sema;

    status = xTaskCreate(mag_task, "MAG Task", TASK_MAG_STACK_SIZE,
                         mag_args, TASK_MAG_PRIORITY, &mag_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t)status;
}

static void update_mag_vals(cyhal_i2c_t *i2c)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static uint8_t buf[23] = {0b00000000};

    result = cyhal_i2c_master_read(i2c, MAG_ADDR, buf, 23, 0, true);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        //   printf("doh");
    }
    else
    {
        int32_t _X = ((buf[0] << 8) | (buf[4] & 0xF0)) >> 4;
        int32_t _Y = ((buf[1] << 8) | ((buf[4] & 0x0F) << 4)) >> 4;
        int32_t _Z = ((buf[2] << 8) | ((buf[5] & 0x0F) << 4)) >> 4;
        int32_t _T = (buf[3] << 4) | ((buf[5] >> 6) << 1);

        if (_X & 0x800)
        {
            _X = (_X ^ 0xFFF) + 1;
            _X = -(_X & 0x7FF);
        }
        if (_Y & 0x800)
        {
            _Y = (_Y ^ 0xFFF) + 1;
            _Y = -(_Y & 0x7FF);
        }
        if (_Z & 0x800)
        {

            _Z = (_Z ^ 0xFFF) + 1;
            _Z = -(_Z & 0x7FF);
        }
        /*
        if (T & 0x500) //only 11 bit temp
        {
            T = (T ^ 0xFFF) + 1;
            T = -(T & 0x7FF);
        }
        */

        /*
            X = -X if negate_field else X
            Y = -Y if negate_field else Y
            Z = -Z if negate_field else Z
        */
        float R = sqrt(pow(X, 2) + pow(Y, 2) + pow(Z, 2));

        float _N = sqrt(pow(X, 2) + pow(Y, 2) + pow(Z, 2)) * FULL_MULT;
        X = _X * FULL_MULT;
        Y = _Y * FULL_MULT;
        T = _T;
        T = (T - TEMP_OFFSET) * TEMP_MULT + TEMP_25;

        float theta = acos((float)Z / (float)R);
        float phi = atan2((float)Y, (float)X);

        azimuth = atan2((float)Y, (float)X);
        polar = atan2((float)Z, sqrt(pow((float)X, 2) + pow((float)Y, 2)));
    }
    result = cyhal_i2c_master_write(i2c, MAG_ADDR, &TRIG_BYTE, 1, 0, true);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }
}

static void mag_task(void *pvParameters)
{
    void **args = pvParameters;
    cyhal_i2c_t *i2c = (cyhal_i2c_t *)args[0];
    SemaphoreHandle_t *sema = (SemaphoreHandle_t *)args[1];

    init_mag(i2c);

    printf("\x1b[2J\x1b[;H");
    printf("starting");

    printf(" successfully initialized.\n");

    for (;;)
    {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        xSemaphoreTake(*sema, portMAX_DELAY);
        update_mag_vals(i2c);
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

cy_rslt_t init_mag(cyhal_i2c_t *i2c)
{

    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static cyhal_gpio_callback_data_t mag_cb_t;

    /* Configure GPIO interrupt */
    mag_cb_t.callback = mag_handler;
    result = cyhal_gpio_init(MAG_GPIO, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }
    cyhal_gpio_register_callback(MAG_GPIO, &mag_cb_t);
    cyhal_gpio_enable_event(MAG_GPIO, CYHAL_GPIO_IRQ_FALL, 7u, true);

    uint8_t cmd[3] = {0b00000000};
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_CONF_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }

    cmd[0] = 0b00011001;
    // cmd[0] = 0b00001001;
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_MOD1_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }

    result = cyhal_i2c_master_write(i2c, MAG_ADDR, &TRIG_BYTE, 1, 0, true);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }

    return result;
}

/*
void _mag_cb(void *handler_arg, cyhal_gpio_event_t event)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static uint8_t buf[23] = {0b00000000};

    xSemaphoreTakeFromISR(*i2c_semaphore_ptr, portMAX_DELAY);
    result = cyhal_i2c_master_read(handler_arg, MAG_ADDR, buf, 23, 0, true);
    xSemaphoreGiveFromISR(*i2c_semaphore_ptr,portMAX_DELAY);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        //   printf("doh");
    }
    else
    {
        X = ((buf[0] << 8) | (buf[4] & 0xF0)) >> 4;
        Y = ((buf[1] << 8) | ((buf[4] & 0x0F) << 4)) >> 4;
        Z = ((buf[2] << 8) | ((buf[5] & 0x0F) << 4)) >> 4;
        T = (buf[3] << 4) | ((buf[5] >> 6) << 1);

        if (X & 0x800)
        {
            X = (X ^ 0xFFF) + 1;
            X = -(X & 0x7FF);
        }
        if (Y & 0x800)
        {
            Y = (Y ^ 0xFFF) + 1;
            Y = -(Y & 0x7FF);
        }
        if (Z & 0x800)
        {

            Z = (Z ^ 0xFFF) + 1;
            Z = -(Z & 0x7FF);
        }
            float R = sqrt(pow(X,2)+pow(Y,2)+pow(Z,2));


            float _N = sqrt(pow(X,2) +pow(Y,2) + pow(Z,2)) * FULL_MULT;
            float _X = X* FULL_MULT;
            float _Y = Y* FULL_MULT;
            float _Z = Z* FULL_MULT;
            _T = T;
            _T = (T - TEMP_OFFSET) * TEMP_MULT + TEMP_25;

            theta = acos((float)Z/(float)R);
            phi = atan2((float)Y,(float)X);

            azimuth = atan2((float)Y,(float)X);
            polar = atan2((float)Z,sqrt(pow((float)X,2) + pow((float)Y,2)));

    }

    xSemaphoreTakeFromISR(*i2c_semaphore_ptr, portMAX_DELAY);
    result = cyhal_i2c_master_write(handler_arg, MAG_ADDR, &TRIG_BYTE, 1, 0, true);
    xSemaphoreGiveFromISR(*i2c_semaphore_ptr,portMAX_DELAY);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        // printf("doh");
        cnt+=3;

    }
    cnt++;
}

static cy_rslt_t mag_init(cyhal_i2c_t *i2c)
{

    cy_rslt_t result = CY_RSLT_SUCCESS;
    static cyhal_gpio_callback_data_t mag_cb_t;

    mag_cb_t.callback = &_mag_cb;
    mag_cb_t.callback_arg = i2c;
    result = cyhal_gpio_init(MAG_GPIO, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }
    cyhal_gpio_register_callback(MAG_GPIO, &mag_cb_t);
    cyhal_gpio_enable_event(MAG_GPIO, CYHAL_GPIO_IRQ_FALL, 7u, true);


    uint8_t cmd[3] = {0b00000000};
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_CONF_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }

    cmd[0] = 0b00011001;
    // cmd[0] = 0b00001001;
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_MOD1_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf("doh");
    }

    return result;
}
*/
