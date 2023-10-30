/******************************************************************************
* File Name:   low_level_init.c
*
* Description: AnyCloud ThreadX CM4 initialization routines
*
*******************************************************************************
* Copyright 2019-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
#include <stdint.h>

#include "tx_api.h"
#include "tx_initialize.h"

#include "cy_device.h"
#include "cycfg_system.h"
#include "cyabs_rtos.h"

/******************************************************
 *                    Constants
 ******************************************************/

/* The number of system ticks per second */
#define SYSTICK_FREQUENCY   (TX_TIMER_TICKS_PER_SECOND)

/* CPU clock : 100 MHz */
/* CLK_HF[0]:CLK_FAST is setup in Cy_SystemInit() in peripheral library */
#define CPU_CLOCK_HZ        (SystemCoreClock)

#define CYCLES_PER_SYSTICK  ((CPU_CLOCK_HZ / SYSTICK_FREQUENCY) - 1)

#ifndef DEFAULT_APPLICATION_STACK_SIZE
#define DEFAULT_APPLICATION_STACK_SIZE      (15*1024)
#endif

#ifndef DEFAULT_APPLICATION_PRIORITY
#define DEFAULT_APPLICATION_PRIORITY        (CY_RTOS_PRIORITY_NORMAL)
#endif

/* Entry point for user Application */
extern void application_start(void);

/* ThreadX timer interrupt processing */
extern void _tx_timer_interrupt(void);

extern void *_tx_initialize_unused_memory;
extern void *_tx_thread_system_stack_ptr;

void application_thread_cleanup(TX_THREAD *thread_ptr, UINT condition);
void application_thread_main(ULONG thread_input);

/******************************************************************************
* Global Variables
******************************************************************************/

/*******************************************************************************
* Function definitions
********************************************************************************/

/* ThreadX System Tick IRQ handler */
__attribute__(( naked, interrupt, used ))
void SysTick_Handler(void)
{
    __asm__( "PUSH {r0, lr}" );
    _tx_timer_interrupt();
    __asm__( "POP {r0, lr}" );
    __asm__( "BX LR" );
}

/*
 * ThreadX interrupt priority setup.
 * Taken from WICED ThreadX initialization.
 */
void init_threadx_irq_priorities(void)
{
    /* Re-register SysTick_Handler.
     * mtb-pdl-cat1 version 2.4 and higher overwrites Systick handler in Cy_SysTick_Init(). So need to re-register it.
     */
    #if defined (COMPONENT_CAT1)
    NVIC_SetVector (SysTick_IRQn, (long unsigned int)&SysTick_Handler);
    #endif

    /* Setup the system handler priorities */

    /*
     * Note: PendSV should be the lowest priority interrupt in the system.
     * If there is only one thread in the system and it goes to sleep,
     * other interrupts at the PendSV or lower level will be locked out where
     * the PendSV ISR is waiting for a thread to become runable.
     */

    NVIC_SetPriority(MemoryManagement_IRQn,  0);    /* Mem Manage system handler priority    */
    NVIC_SetPriority(BusFault_IRQn        ,  0);    /* Bus Fault system handler priority     */
    NVIC_SetPriority(UsageFault_IRQn      ,  0);    /* Usage Fault system handler priority   */
    NVIC_SetPriority(SVCall_IRQn          , 15);    /* SVCall system handler priority        */
    NVIC_SetPriority(DebugMonitor_IRQn    ,  0);    /* Debug Monitor system handler priority */
    NVIC_SetPriority(PendSV_IRQn          , 15);    /* PendSV system handler priority        */
    NVIC_SetPriority(SysTick_IRQn         ,  4);    /* SysTick system handler priority       */

    /*
     * Setup the system tick.
     * This needs to happen after cybsp_init() as SystemCoreClock may be modified as part of
     * the init process.
     */

    SYSTICK_LOAD = (uint32_t)(CYCLES_PER_SYSTICK);
    SYSTICK_CTRL = (uint32_t)(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk);  /* clock source is processor clock */
}

/* ThreadX kernel initialization function */
void _tx_initialize_low_level(void)
{
    /* Setup some ThreadX internal values */
    _tx_initialize_unused_memory = (void *)0xbaadbaad;  /* TODO : add proper value here */
    _tx_thread_system_stack_ptr  = (void *)__Vectors[0];
}

/**
 *  Application Define function - creates and starts the application thread
 *  Called by ThreadX whilst it is initializing
 *
 *  @param first_unused_memory: unused parameter - required to match prototype
 */
void tx_application_define(void *first_unused_memory)
{
    TX_THREAD *app_thread_handle;
    char      *app_thread_stack;
    UINT      status;

    (void)first_unused_memory;

    init_threadx_irq_priorities();

    /* Create the application thread.  */
    app_thread_handle = (TX_THREAD *)malloc(sizeof(TX_THREAD));
    app_thread_stack  = (char *)malloc(DEFAULT_APPLICATION_STACK_SIZE);

    status = tx_thread_create(app_thread_handle, (char *)"app thread", application_thread_main, 0, app_thread_stack,
                              DEFAULT_APPLICATION_STACK_SIZE, DEFAULT_APPLICATION_PRIORITY, DEFAULT_APPLICATION_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if (TX_SUCCESS != status)
    {
        free(app_thread_handle);
        free(app_thread_stack);
        app_thread_handle = NULL;
        app_thread_stack  = NULL;
    }
    else
    {
        (void)tx_thread_entry_exit_notify(app_thread_handle, application_thread_cleanup);
    }
}

void application_thread_cleanup(TX_THREAD *thread_ptr, UINT condition)
{
    /* Determine if the thread was exited. */
    if (thread_ptr && condition == TX_THREAD_EXIT)
    {
        tx_thread_terminate(thread_ptr);
        tx_thread_delete(thread_ptr);
        free(thread_ptr->tx_thread_stack_start);
        free(thread_ptr);
    }
}

void application_thread_main(ULONG thread_input)
{
    (void)thread_input;

    /*
     * Perform any system wide initialization desired here before
     * calling application_start().
     */


    /*
     * Start the application.
     */

    application_start();
}
