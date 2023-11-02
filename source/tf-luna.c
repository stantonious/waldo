
#include "tf-luna.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

#define I2C_SLAVE_ADDR 0x10
#define LIDAR_ADDR 16
#define PACKET_SIZE 3
#define REG_DIST_L 0
#define REG_DIST_H 1
#define REG_TEMP_L 4
#define REG_TEMP_H 5

uint32_t get_total_dist(uint8_t dist_l, uint8_t dist_h)
{
    return (dist_h * 0xFF) + dist_l;
}

cy_rslt_t get_dist(cyhal_i2c_t *i2c, float *val)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    static uint8_t lidar_buf[PACKET_SIZE] = {0};

    if (CY_RSLT_SUCCESS != cyhal_i2c_master_mem_read(i2c, LIDAR_ADDR, REG_DIST_L, 1, lidar_buf, 1, 10))
    {
        result = CY_RSLT_TYPE_ERROR;
    }
    if (CY_RSLT_SUCCESS != cyhal_i2c_master_mem_read(i2c, LIDAR_ADDR, REG_DIST_H, 1, lidar_buf + 1, 1, 10))
    {
        result = CY_RSLT_TYPE_ERROR;
    }

    *val = get_total_dist(lidar_buf[0], lidar_buf[1]);
    return result;
}
