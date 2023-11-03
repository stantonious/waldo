
#include "tli493d-w2bw.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "math.h"
#include "cy_retarget_io.h"

#define MAG_GPIO P10_0
#define MAG_ADDR 0x35
#define MAG_CONF_ADDR 0x10
#define MAG_MOD1_ADDR 0x11

#define FULL_MULT 1.0 / 7.7
#define TEMP_OFFSET 1180
#define TEMP_MULT .24
#define TEMP_25 25

static float X, Y, Z, R, T, N;
static float azimuth, polar;

static const uint8_t TRIG_BYTE = 0b00100000;
cy_rslt_t init_sensor(cyhal_i2c_t *i2c, cyhal_gpio_event_callback_t cb)
{

    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static cyhal_gpio_callback_data_t mag_cb_t;

    /* Configure GPIO interrupt */
    mag_cb_t.callback = cb;
    result = cyhal_gpio_init(MAG_GPIO, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        result = CY_RSLT_TYPE_ERROR;
    }
    cyhal_gpio_register_callback(MAG_GPIO, &mag_cb_t);
    cyhal_gpio_enable_event(MAG_GPIO, CYHAL_GPIO_IRQ_FALL, 7u, true);

    uint8_t cmd[3] = {0b00000000};
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_CONF_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        result = CY_RSLT_TYPE_ERROR;
    }

    cmd[0] = 0b00011001;
    // cmd[0] = 0b00001001;
    result = cyhal_i2c_master_mem_write(i2c, MAG_ADDR, MAG_MOD1_ADDR, 1, cmd, 1, 0);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        result = CY_RSLT_TYPE_ERROR;
    }

    result = cyhal_i2c_master_write(i2c, MAG_ADDR, &TRIG_BYTE, 1, 0, true);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        result = CY_RSLT_TYPE_ERROR;
    }

    return result;
}

void update_mag_vals(cyhal_i2c_t *i2c)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static uint8_t buf[23] = {0b00000000};

    result = cyhal_i2c_master_read(i2c, MAG_ADDR, buf, 23, 10, true);
    if ((cy_rslt_t)result != CY_RSLT_SUCCESS)
    {
        result = CY_RSLT_TYPE_ERROR;
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

        R = sqrt(pow(X, 2) + pow(Y, 2) + pow(Z, 2));

        N = sqrt(pow(X, 2) + pow(Y, 2) + pow(Z, 2)) * FULL_MULT;
        X = _X * FULL_MULT;
        Y = _Y * FULL_MULT;
        Z = _Z * FULL_MULT;
        T = _T;
        T = (T - TEMP_OFFSET) * TEMP_MULT + TEMP_25;

        azimuth = atan2((float)Y, (float)X);
        polar = atan2((float)Z, sqrt(pow((float)X, 2) + pow((float)Y, 2)));
    }
    result = 1;
    while (result != CY_RSLT_SUCCESS)
    {
        result = cyhal_i2c_master_write(i2c, MAG_ADDR, &TRIG_BYTE, 1, 1, true);

    }
}

float get_X()
{
    return X;
}
float get_Y()
{
    return Y;
}
float get_Z()
{
    return Z;
}
float get_R()
{
    return R;
}
float get_T()
{
    return T;
}
float get_N()
{
    return N;
}
float get_azimuth()
{
    return azimuth;
}

float get_polar()
{
    return polar;
}