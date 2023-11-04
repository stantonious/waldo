
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
#define SCAN_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define SCAN_TASK_STACK_SIZE (512u)

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
cy_rslt_t create_scanner_task(cyhal_i2c_t *i2c, SemaphoreHandle_t *i2c_semaphore);

/*
 * @brief Autonomous alignmen
 *
 * @param xy_step_idx Starting xy step index
 * @param yz_step_idx Starting yz step index
 */
void auto_align(uint8_t *xy_step_idx, uint8_t *yz_step_idx);

/*
 * @brief Main run loop for motor activity and scan pattern
 *
 * @param i2c i2c handle
 * @param starting_xy_step_idx Starting xy step index
 * @param starting_yz_step_idx Starting yz step index
 * @param total_steps Total number of motor steps
 * @param xy_run XY run
 * @param yz_run YZ run
 * @param xy_step_period Number of steps between direction switch
 * @param yz_step_period Number of steps between direction switch
 * @param xy_direction initial direction of xy
 * @param yz_direction initial direction of yz`
 * @param step_sleep sleep between steps
 */
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
    uint16_t step_sleep);

/*
 * @brief Process lidar data for step
 *
 * @param period_idx The period index
 * @param period The current period
 * @param step_period The current step period
 * @param motor_id Motor identifier
 * @param step_idx current step_index
 * @param stepped true if stepped
 */
void process_step(
    uint32_t period_idx,
    uint32_t period,
    uint32_t step_period,
    int motor_id,
    bool *direction,
    uint8_t *step_idx,
    bool *stepped);

#endif /* SCANNER_TASK_H_ */