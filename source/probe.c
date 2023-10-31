

/* Header file includes */
#include "probe.h"
#include "cyhal.h"
#include "cybsp.h"
#include <stdio.h>


void probe(cyhal_i2c_t* i2c)
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
        rval = cyhal_i2c_master_read(i2c, i2caddress, buffer, 0, 100, true);
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
