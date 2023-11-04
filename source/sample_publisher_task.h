#ifndef SAMPLE_PUBLISHER_TASK_H_
#define SAMPLE_PUBLISHER_TASK_H_

#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"

#define BATCH_SIZE 32

typedef struct
{
    uint32_t xy_step;
    uint32_t yz_step;
    float x;
    float y;
    float z;
    uint16_t xy_dir;
    uint16_t yz_dir;
} measurement;

typedef measurement measurement_batch[BATCH_SIZE];

/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
void sample_publisher_task(void *arg);

/*
 * @brief Init the wifi
 *
 * @return the result
 */
cy_rslt_t init_wifi();

/*
 * @brief Create publisher task
 *
 * @return the result
 */
cy_rslt_t create_sample_publisher_task();

#endif
