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
*       i2cbm_out.c
*
* COMPONENT
*
*       I2CBM - I2C Buffer Management
*
* DESCRIPTION
*
*       This file contains functions for handling output buffer of
*       Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CBM_Put_Output_Buffer             Puts data in output buffer.
*
*       I2CBM_Get_Output_Buffer             Gets data from output buffer.
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
*       I2CBM_Put_Output_Buffer
*
* DESCRIPTION
*
*       This function places the data in the output buffer.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*      *data                                Data pointer.
*
*       length                              Number of data bytes.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_TX_BUFFER_NOT_ENOUGH            Insufficient transmission
*                                           buffer memory space.
*
*************************************************************************/
STATUS  I2CBM_Put_Output_Buffer  (I2C_CB      *i2c_cb,
                                  UINT8       *data,
                                  UNSIGNED_INT length)
{
    STATUS      status;
    
#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    I2C_BUFFER     *tx_buffer;
    UNSIGNED_INT    i;
    
    status = NU_SUCCESS;

    /* Set the output buffer pointer. */
    tx_buffer = &(i2c_cb->i2c_io_buffer.i2cbm_tx_buffer);

    /* Check for the valid length of the data. */
    if (length <= i2c_cb->i2c_io_buffer.i2cbm_tx_buffer_size)
    {
        /* Set buffer write pointer to buffer start position. */
        tx_buffer->i2cbm_data_write = tx_buffer->i2cbm_data_start;

        /* Set buffer read pointer to buffer start position. */
        tx_buffer->i2cbm_data_read = tx_buffer->i2cbm_data_start;

        /* Reset the output buffer byte counter. */
        tx_buffer->i2cbm_count = 0;

        /* Put all the data bytes in the buffer. */
        for (i = 0; i < length; i++)
        {
            /* Put one byte in the buffer. */
            *tx_buffer->i2cbm_data_write = *data;

            /* Increment data pointer. */
            data++;

            /* Increment the buffer write pointer. */
            tx_buffer->i2cbm_data_write++;

            /* Increment the byte count in the buffer. */
            tx_buffer->i2cbm_count++;
        }
    }

    /* Data length is not valid. */
    else
    {
        /* Set status to indicate that given data length is more than the
           buffer size. */
        status = I2C_TX_BUFFER_NOT_ENOUGH;
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
*       I2CBM_Get_Output_Buffer
*
* DESCRIPTION
*
*       This function gets data from the output buffer byte by byte.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*      *data                                Pointer to the location where
*                                           data will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_BUFFER_EMPTY                    No more data bytes left in the
*                                           buffer.
*
*************************************************************************/
STATUS    I2CBM_Get_Output_Buffer(I2C_CB *i2c_cb, UINT8 *data)
{
    STATUS      status;

#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    I2C_BUFFER     *tx_buffer;

    status = NU_SUCCESS;

    /* Set the output buffer pointer. */
    tx_buffer = &(i2c_cb->i2c_io_buffer.i2cbm_tx_buffer);

    /* Check if there are any bytes left in the buffer. */
    if (tx_buffer->i2cbm_count > 0)
    {
        /* Get data byte pointed to by read pointer. */
        *data = *(tx_buffer->i2cbm_data_read);

        /* Increment the read pointer. */
        tx_buffer->i2cbm_data_read++;

        /* Decrement the counter. */
        tx_buffer->i2cbm_count--;
    }
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
