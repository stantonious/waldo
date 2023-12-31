
/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mag_task.h"
#include "uln2003.h"
#include "scanner_task.h"
#include "sample_publisher_task.h"

/* RTOS header file */
#include "cyabs_rtos.h"
#include <FreeRTOS.h>
#include <task.h>


/*******************************************************************************
 * Global Variables
 ********************************************************************************/

/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

static SemaphoreHandle_t i2c_semaphore;

cy_queue_t meas_q;

static cyhal_i2c_t glb_i2c;
static cyhal_i2c_cfg_t i2c_cfg = {
    .is_slave = false,
    .address = 0,
    .frequencyhal_hz = 400000};

void init_i2c()
{
    cy_rslt_t result;
    result = cyhal_i2c_init(&glb_i2c, (cyhal_gpio_t)CYBSP_I2C_SDA, (cyhal_gpio_t)CYBSP_I2C_SCL, NULL);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf(" Error : I2C initialization failed !!\n [Error code: 0x%lx]\n", (long unsigned int)result);
    }

    /* Configure the I2C master interface with the desired clock frequency */
    result = cyhal_i2c_configure(&glb_i2c, &i2c_cfg);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        printf(" Error : I2C configuration failed !!\n [Error code: 0x%lx]\n", (long unsigned int)result);
    }
    cyhal_system_delay_us(10000);

    /* Create binary semaphore and suspend the task upon failure */
    i2c_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(i2c_semaphore);
}

/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  System entrance point. This function sets up user tasks and then starts
 *  the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main()
{
    cy_rslt_t result;

#if defined(COMPONENT_FREERTOS)
    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;
#elif defined(COMPONENT_THREADX)
    uxTopUsedPriority = TX_MAX_PRIORITIES - 1;
#endif

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    /* To avoid compiler warnings. */
    (void)result;

    /* Enable global interrupts. */
    init_i2c();
    __enable_irq();
    init_motors();

    /* Initialize a queue to receive command. */
    cy_rtos_queue_init(&meas_q, 100u, sizeof(measurement_batch));

    /* If the task creation failed stop the program execution */
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Create the tasks. */
    result = create_sample_publisher_task();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    result = create_mag_task(&glb_i2c, &i2c_semaphore);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    result = create_scanner_task(&glb_i2c, &i2c_semaphore);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
