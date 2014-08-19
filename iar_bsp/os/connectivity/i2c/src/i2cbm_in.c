/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       i2cbm_in.c
*
* COMPONENT
*
*       I2CBM - I2C Buffer Management
*
* DESCRIPTION
*
*       This file contains functions for input buffer handling of
*       Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CBM_Put_Input_Buffer              Puts data in input buffer.
*
*       I2CBM_Get_Input_Buffer              Gets data from input buffer.
*
* DEPENDENCIES
*
*       i2cbm_extr.h                        Function prototypes for
*                                           Nucleus I2C buffer management.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2cbm_extr.h"

/*************************************************************************
* FUNCTION
*
*       I2CBM_Put_Input_Buffer
*
* DESCRIPTION
*
*       This function puts data in the input buffer.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*       data                                Buffer data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_BUFFER_FULL                     Data buffer is full.
*
*************************************************************************/
STATUS  I2CBM_Put_Input_Buffer (I2C_CB *i2c_cb, UINT8 data)
{
    STATUS      status;
    
#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    I2C_BUFFER     *rx_buffer;

    status = NU_SUCCESS;
    
    /* Set the input buffer pointer. */
    rx_buffer = &(i2c_cb->i2c_io_buffer.i2cbm_rx_buffer);

    /* Check if buffer is not full. */
    if (rx_buffer->i2cbm_count !=
        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer_size)
    {
        /* Check if this is the first byte to write. */
        if (rx_buffer->i2cbm_count == 0)
        {
            /* Reset the write data pointer for the input buffer. */
            rx_buffer->i2cbm_data_write = rx_buffer->i2cbm_data_start;
        }

        /* Write byte at current location pointed to by write pointer. */
        *rx_buffer->i2cbm_data_write = data;

        /* Increment the write pointer. */
        rx_buffer->i2cbm_data_write++;

        /* Increment the data byte counter. */
        rx_buffer->i2cbm_count++;
    }

    /* Buffer is full. */
    else
    {
        /* Set status to indicate that the buffer is already full. */
        status = I2C_BUFFER_FULL;
    }
#else
    /* This function is only allowed to be called while I2C is built for 
     * interrupt driven I/O.
     */
    status = I2C_INVALID_DRIVER_MODE;
#endif

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CBM_Get_Input_Buffer
*
* DESCRIPTION
*
*       This function gets data from the input buffer.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*      *data                                Pointer to the location where
*                                           data will be returned.
*
*       length                              Number of data bytes to read
*                                           from the buffer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_BUFFER_EMPTY                    No data bytes in the buffer.
*
*       I2C_BUFFER_HAS_LESS_DATA            Requested data is more than the
*                                           buffer has.
*
*       I2C_BUFFER_NOT_EMPTY                Buffer has some more bytes
*                                           even after the reading.
*
*************************************************************************/
STATUS  I2CBM_Get_Input_Buffer (I2C_CB      *i2c_cb,
                                    UINT8       *data,
                                    UNSIGNED_INT length)
{
    STATUS      status;
    
#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    I2C_BUFFER     *rx_buffer;
    UNSIGNED_INT    i;
    
    status = NU_SUCCESS;

    /* Set the input buffer pointer. */
    rx_buffer = &(i2c_cb->i2c_io_buffer.i2cbm_rx_buffer);

    /* Check if any data is available in the buffer. */
    if (rx_buffer->i2cbm_count > 0)
    {
        /* Check if the requested number of bytes are available
           in the buffer. */
        if (length <= rx_buffer->i2cbm_count)
        {
            /* Set the buffer read pointer to buffer start position. */
            rx_buffer->i2cbm_data_read = rx_buffer->i2cbm_data_start;

            /* Get the data from the buffer and put into the
               provided location. */
            for (i = 0; i < length; i++)
            {
                /* Get the data byte from the input buffer. */
                *data = *rx_buffer->i2cbm_data_read;

                /* Increment the read pointer. */
                rx_buffer->i2cbm_data_read++;

                /* Increment the data pointer. */
                data++;

                /* Decrement the byte count for the input buffer. */
                rx_buffer->i2cbm_count--;
            }

            /* Check if any data is left in the buffer. */
            if (rx_buffer->i2cbm_count > 0)
            {
                /* Set the status to indicate that still some data is
                   available in the buffer. */
                status = I2C_BUFFER_NOT_EMPTY;
            }
        }
        else
        {
            /* Set status to indicate that the requested number of bytes
               is more than the actual data in the buffer. */
            status = I2C_BUFFER_HAS_LESS_DATA;
        }
    }

    /* Buffer is empty. */
    else
    {
        /* Set the status to indicate that the buffer is empty. */
        status = I2C_BUFFER_EMPTY;
    }
#else
    /* This function is only allowed to be called while I2C is built for 
     * interrupt driven I/O.
     */
    status = I2C_INVALID_DRIVER_MODE;
#endif

    /* Return the completion status of the service. */
    return (status);
}
