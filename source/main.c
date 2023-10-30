
/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "lidar_task.h"
#include "mag_task.h"
#include "scanner_task.h"

/* RTOS header file */
#include "cyabs_rtos.h"
#if defined(COMPONENT_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>

#elif defined(COMPONENT_THREADX)
#include "tx_api.h"
#include "tx_initialize.h"
#endif

/* TCP server task header file. */
#include "tcp_server.h"
#include "motor.h"

/* Include serial flash library and QSPI memory configurations only for the
 * kits that require the Wi-Fi firmware to be loaded in external QSPI NOR flash.
 */
#if defined(CY_DEVICE_PSOC6A512K)
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#endif

/*******************************************************************************
 * Macros
 ********************************************************************************/
/* RTOS related macros for TCP server task. */
#if defined(COMPONENT_FREERTOS)
#define TCP_SERVER_TASK_STACK_SIZE (1024 * 5)
#define TCP_SERVER_TASK_PRIORITY (1)
#endif

/* Queue lengths of message queues used in this project */
#define SINGLE_ELEMENT_QUEUE (1u)

/*******************************************************************************
 * Global Variables
 ********************************************************************************/
/* Queue handler */
cy_queue_t led_command_q;

/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

static SemaphoreHandle_t i2c_semaphore;

static cyhal_i2c_t glb_i2c;
static cyhal_i2c_cfg_t i2c_cfg = {
    .is_slave = false,
    .address = 0,
    .frequencyhal_hz = 100000};


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

void probe()
{
    cy_rslt_t rval;

    // Setup the screen and print the header
    printf("\n\n   ");
    for (unsigned int i = 0; i < 0x10; i++)
    {
        printf("%02X ", i);
    }

    // Iterate through the address starting at 0x00
    for (uint32_t i2caddress = 0; i2caddress < 0x80; i2caddress++)
    {
        if (i2caddress % 0x10 == 0)
            printf("\n%02X ", (unsigned int)i2caddress);

        uint8_t buffer[1] = {0}; // You can change this to your specific data to send
        rval = cyhal_i2c_master_read(&glb_i2c, i2caddress, buffer, 0, 100, true);
        if (rval == CY_RSLT_SUCCESS) // If you get ACK, then print the address
        {
            printf("%02X ", (unsigned int)i2caddress);
        }
        else //  Otherwise print a --
        {
            printf("-- ");
        }
    }
    printf("\n");
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

    // probe();

#if defined(CY_DEVICE_PSOC6A512K)
    const uint32_t bus_frequency = 50000000lu;
    cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1, CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC, CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);
    cy_serial_flash_qspi_enable_xip(true);
#endif

    /* Initialize a queue to receive command. */
    cy_rtos_queue_init(&led_command_q, SINGLE_ELEMENT_QUEUE, sizeof(uint8_t));

#if defined(COMPONENT_FREERTOS)

    /* Create the tasks. */
    /* Create the lidar task */
    // result = create_lidar_task(&glb_i2c,&i2c_semaphore);



    result = create_scanner_task(&glb_i2c, &i2c_semaphore);

    cyhal_system_delay_us(10000);

    result = create_mag_task(&glb_i2c,&i2c_semaphore);

    /* If the task creation failed stop the program execution */
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    xTaskCreate(tcp_server_task, "Network task", TCP_SERVER_TASK_STACK_SIZE, NULL,
                TCP_SERVER_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);

#elif defined(COMPONENT_THREADX)
    /*
     * Start the ThreadX kernel.
     * This routine never returns.
     */

    tx_kernel_enter();

    /* Should never get here. */
    CY_ASSERT(0);
#endif
}

#if defined(COMPONENT_THREADX)
void application_start(void)
{
    tcp_server_task(NULL);
}
#endif

/* [] END OF FILE */
