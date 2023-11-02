
#include <motor.h>
#include "cyhal.h"
#include <stdio.h>

#define STEPPER_DELAY 2

cyhal_gpio_psoc6_02_124_bga_t yz_motor_pins[NUM_MOTOR_PINS] = {P10_7, P10_6, P10_5, P10_4};
cyhal_gpio_psoc6_02_124_bga_t xy_motor_pins[NUM_MOTOR_PINS] = {P9_7, P9_6, P9_5, P9_4};

cyhal_gpio_psoc6_02_124_bga_t *get_pins(int motor_id)
{
    if (motor_id == XY_MOTOR_ID)
        return xy_motor_pins;
    if (motor_id == YZ_MOTOR_ID)
        return yz_motor_pins;
    return NULL;
}

uint8_t step_sequence[STEP_SEQ_LEN][NUM_MOTOR_PINS] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
};

uint8_t step_clockwise(uint8_t step_idx, int motor_id)
{
    cyhal_gpio_psoc6_02_124_bga_t *pins = get_pins(motor_id);

    uint8_t l_step_idx = (step_idx - 1) % STEP_SEQ_LEN;
    for (int i = 0; i < NUM_MOTOR_PINS; i++)
    {
        vTaskDelay(STEPPER_DELAY);
        cyhal_gpio_write(pins[i], step_sequence[l_step_idx][i]);
    }
    return l_step_idx;
}

uint8_t step_anticlockwise(uint8_t step_idx, int motor_id)
{
    cyhal_gpio_psoc6_02_124_bga_t *pins = get_pins(motor_id);

    uint8_t l_step_idx = (step_idx + 1) % STEP_SEQ_LEN;
    for (int i = 0; i < NUM_MOTOR_PINS; i++)
    {
        vTaskDelay(STEPPER_DELAY);
        cyhal_gpio_write(pins[i], step_sequence[l_step_idx][i]);
    }
    return l_step_idx;
}

cy_rslt_t init_motors()
{
    cy_rslt_t result;
    for (int i = 0; i < NUM_MOTOR_PINS; i++)
    {

        /* Initialize the user button */
        result = cyhal_gpio_init(yz_motor_pins[i], CYHAL_GPIO_DIR_OUTPUT,
                                 CYHAL_GPIO_DRIVE_PULLUP, 0);
        // result = cyhal_gpio_configure(yz_motor_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP);
        if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
        {
            // printf("gio failed");
        }
        cyhal_gpio_write(yz_motor_pins[i], 0);
        // result = cyhal_gpio_init(xy_motor_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP, 0);
        result = cyhal_gpio_configure(xy_motor_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP);
        cyhal_gpio_write(xy_motor_pins[i], 0);
        if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
        {
            // printf("gio failed");
        }
    }

    return result;
}

uint8_t step(uint8_t step_idx, int motor_id, bool clockwise, int num)
{
    uint8_t idx=step_idx;
    for (int i = 0; i < num; i++)
    {
        if (clockwise) idx = step_clockwise(idx,motor_id);
        else idx = step_anticlockwise(idx,motor_id);
    }
    return idx;
}