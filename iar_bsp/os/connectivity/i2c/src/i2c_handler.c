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
*       i2c_handler.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains the routines responsible for handling the
*       reception or transmission for Nucleus I2C.
*
* DATA STRUCTURES
*
*       I2C_Handler_Block                   Control block for Nucleus
*                                           I2C handler.
*
* FUNCTIONS
*
*       I2C_Handler                         Function for activating a
*                                           thread handling the respective
*                                           Nucleus I2C notification event.
*
*       I2C_Handler_Entry                   Thread for handling Nucleus
*                                           I2C notifications.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C services.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"

/* Define external inter component global data references. */


/* HISRs for handling Nucleus I2C notification events. */
NU_HISR             I2C_Handler_Block;
extern I2C_CB I2c_cbs[I2C_MAX_INSTANCES];

/*************************************************************************
* FUNCTION
*
*       I2C_Handler
*
* DESCRIPTION
*
*       This function is used to activate Nucleus I2C handler with
*       specified flags.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
*       use_hisr                            Whether HISR will be used
*                                           or execution from ISR will
*                                           continue.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID I2C_Handler    (I2C_CB *i2c_cb, BOOLEAN use_hisr)
{
    STATUS      status;

    /* Check if HISR will be used. */
    if (use_hisr == NU_TRUE)
    {
        /* Activate I2C notification handler for application. */
        status = NU_Activate_HISR(&I2C_Handler_Block);

        /* Check if error occurred while activating notification handler. */
        if (status != NU_SUCCESS)
        {
            I2C_APP_CALLBACKS *callbacks;

            /* Get the callback structure for the slave. */
            callbacks = I2CMS_Get_Callbacks_Struct(i2c_cb,
                i2c_cb->i2c_io_buffer.i2cbm_slave_address);

            /* Check if error notification callback is configured. */
            if (callbacks->i2c_error != NU_NULL)
            {
                /* Set the error code. */
                (*callbacks->i2c_error)(i2c_cb->i2c_dv_id, status);
                    
            }
        }
    }

    else
    {
        /* Call the handler directly. */
        I2C_Handler_Entry();
    }
}

/*************************************************************************
* HANDLER
*
*       I2C_Handler_Entry
*
* DESCRIPTION
*
*       This I2C handler is responsible for calling application callback
*       functions based upon the type of functionality specified in the
*       interrupt service routine.
*
*************************************************************************/
VOID I2C_Handler_Entry(VOID)
{
    I2C_CB      *i2c_cb;
    I2C_HANDLE   i2c_handle;
    int i;
    
    /* Loop until all the activated transmission handler requests are
       satisfied. */
    for (i=0, i2c_cb = &I2c_cbs[0]; i<I2C_MAX_INSTANCES; i++, i2c_cb++)
    {
        I2C_APP_CALLBACKS *callbacks;
     
        
        /* Get control block pointer for the device. */
        I2CS_Get_Handle(i2c_cb, &i2c_handle);

        /* Check if the device is initialized and has a pending
           notification. */
        if ((i2c_cb != NU_NULL) && (i2c_cb->device_opened != 0) &&
            (i2c_cb->i2c_handler_type != I2C_NO_ACTIVE_NOTIFICATION))
        {
            /* Get the callback structure for the slave. */
            callbacks = I2CMS_Get_Callbacks_Struct(i2c_cb,
                i2c_cb->i2c_io_buffer.i2cbm_slave_address);


            /* Check if it is the indication for master data reception. */
            if (i2c_cb->i2c_handler_type & I2C_MASTER_DATA_RECEIVED)
            {
                /* Check if data indication callback is configured. */
                if (callbacks->i2c_data_indication != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know that data is ready for reception
                       in the buffer. */
                    (*callbacks->i2c_data_indication)(i2c_handle,
                        (UINT8)I2C_MASTER_NODE,
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                      ((UINT8)~I2C_MASTER_DATA_RECEIVED);
            }

            /* Check if it is the indication for slave data reception. */
            if (i2c_cb->i2c_handler_type & I2C_SLAVE_DATA_RECEIVED)
            {
                /* Check if data indication callback is configured. */
                if (callbacks->i2c_data_indication != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know that data is ready for reception
                       in the buffer. */
                    (*callbacks->i2c_data_indication)(i2c_handle,
                        (UINT8)I2C_SLAVE_NODE,
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                        ((UINT8)~I2C_SLAVE_DATA_RECEIVED);
            }

            /* Check if it is the acknowledgment indication for data. */
            if (i2c_cb->i2c_handler_type & I2C_DATA_ACK_RECEIVED)
            {
                /* Check if acknowledgment indication callback is
                   configured. */
                if (callbacks->i2c_ack_indication != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know about acknowledgment reception. */
                    (*callbacks->i2c_ack_indication)(i2c_handle,
                                                    (UINT8)I2C_DATA_ACK);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                        ((UINT8)~I2C_DATA_ACK_RECEIVED);
            }

            /* Check if it is the acknowledgment indication for address. */
            if (i2c_cb->i2c_handler_type & I2C_ADDRESS_ACK_RECEIVED)
            {
                /* Check if acknowledgment indication callback is
                   configured. */
                if (callbacks->i2c_ack_indication != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know about acknowledgment reception. */
                    (*callbacks->i2c_ack_indication)(i2c_handle,
                                                  (UINT8)I2C_ADDRESS_ACK);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                     ((UINT8)~I2C_ADDRESS_ACK_RECEIVED);
            }

            /* Check if master completed sending data. */
            else if (i2c_cb->i2c_handler_type &
                                            I2C_MASTER_DATA_TX_COMPLETE)
            {
                /* Check if transfer complete indication callback is
                   configured. */
                if (callbacks->i2c_transmission_complete != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know about transmission completion. */
                    (*callbacks->i2c_transmission_complete)
                                     (i2c_handle, (UINT8)I2C_MASTER_NODE);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                     ((UINT8)~I2C_MASTER_DATA_TX_COMPLETE);
            }

            /* Check if slave completed sending data. */
            else if (i2c_cb->i2c_handler_type & I2C_SLAVE_DATA_TX_COMPLETE)
            {
                /* Check if transfer complete indication callback is
                   configured. */
                if (callbacks->i2c_transmission_complete != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know about transmission completion. */
                    (*callbacks->i2c_transmission_complete)
                                     (i2c_handle, (UINT8)I2C_SLAVE_NODE);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                     ((UINT8)~I2C_SLAVE_DATA_TX_COMPLETE);
            }

            /* Check if slave address received. */
            else if (i2c_cb->i2c_handler_type & I2C_SLAVE_ADDRESS_RECEIVED)
            {
                /* Check if address complete indication callback is
                   configured. */
                if (callbacks->i2c_address_indication != NU_NULL)
                {
                    /* Call the user callback function to let the
                       application know about incoming slave address. */
                    (*callbacks->i2c_address_indication)
                                     (i2c_handle, (UINT8)I2C_READ);
                }

                /* Reset the handler activation flag. */
                i2c_cb->i2c_handler_type &=
                                     ((UINT8)I2C_SLAVE_ADDRESS_RECEIVED);
            }

            else
            {
                /* Empty else put for MISRA compliance. */
            }

        }

        /* Increment Nucleus I2C device ID. */
        i2c_handle++;
    }
}
