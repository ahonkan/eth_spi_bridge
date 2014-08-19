/*************************************************************************
*
*               Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       spics.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the support routines for the Nucleus SPI Core
*       Services component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICS_Get_CBs                       Gets port and device control
*                                           block pointers.
*
*       SPICS_Get_CB_Index                  Returns the index of the CB in
*                                           CB array for a given device ID
*
*       SPICS_Get_Config_Struct             Gets the transfer attributes
*                                           configuration structure.
*
*       SPICS_Get_Callbacks_Struct          Gets the callback structure.
*
*       SPICS_Check_Init_Params             Checks Nucleus SPI device
*                                           initialization parameters
*                                           for validity.
*
*       SPICS_Check_Address                 Checks the specified slave
*                                           address for validity.
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of the
*                                           Nucleus SPI Core Services.
*
*       spiq_extr.h                         Function prototypes for
*                                           for Nucleus SPI Queue
*                                           Management component.
*
*       spi_handler.h                       Function prototypes for
*                                           Nucleus SPI Notification
*                                           Handler component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"
#include    "connectivity/spiq_extr.h"
#include    "connectivity/spi_handler.h"

/* Define external inner-component global data references. */

extern      SPI_CB        SPI_Devices[SPI_MAX_DEV_COUNT];

/*************************************************************************
* FUNCTION
*
*       SPICS_Get_CBs
*
* DESCRIPTION
*
*       This function validates the specified Nucleus SPI device handle.
*       If this is valid then it returns the pointers to associated driver
*       port and device control blocks.
*
* INPUTS
*
*       spi_dev                             Nucleus SPI device handle.
*
*
*     **spi_cb_ptr                          Location where Nucleus SPI
*                                           device control block pointer
*                                           will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                              The specified device handle
*                                           is valid.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*************************************************************************/
STATUS  SPICS_Get_CBs(SPI_HANDLE    spi_dev,
                          SPI_CB    **spi_cb_ptr)
{
    STATUS      status = NU_SUCCESS;

    if(SPI_Devices[spi_dev].init_flag == 1)
    {
        /* Get the control block pointer of the specified device. */
        *spi_cb_ptr = &SPI_Devices[spi_dev];
    }
    else
    {
        status = SPI_INVALID_HANDLE;
    }

    /* Return the completion status of the service. */
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       SPICS_Get_CB_Index
*
* DESCRIPTION
*
*       This function validates the specified Nucleus SPI device id.
*       If this is valid then it returns the index of the SPI CB
*       in SPI_Devices array.
*
* INPUTS
*
*       dev_id                              Device ID
*
* OUTPUTS
*
*       INT index                           Index of SPI_CB
*
*************************************************************************/
INT SPICS_Get_CB_Index (DV_DEV_ID dev_id)
{
    INT i = 0;
    INT index = SPI_MAX_DEV_COUNT;
    

    for(i = 0; i<SPI_MAX_DEV_COUNT; i++)
    {
        if(SPI_Devices[i].spi_dev_id == dev_id)
        {
            index = i;
            break;
        }
    }
    
    return index;
}


/*************************************************************************
* FUNCTION
*
*       SPICS_Get_Config_Struct
*
* DESCRIPTION
*
*       This function returns the pointer to transfer attribute
*       configuration structure associated with the given slave of
*       the specified SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*       address                             The slave address whose
*                                           transfer attributes
*                                           configuration structure is
*                                           requested.
*
* OUTPUTS
*
*       SPI_TRANSFER_CONFIG *               Pointer to the transfer
*                                           attribute configuration
*                                           structure.
*
*************************************************************************/
SPI_TRANSFER_CONFIG *SPICS_Get_Config_Struct(SPI_CB    *spi_cb,
                                             UINT16     address)
{
    UINT16                  i;
    SPI_TRANSFER_CONFIG    *spi_config = NU_NULL;
    BOOLEAN                 found = NU_FALSE;

    /* Search the transfer attributes list for a record associated
       with the specified slave address. */
    for (i = 0; (i < spi_cb->spi_slaves_count) && (!found); i++)
    {
        /* Check if the associated address of record at current index
           matches the specified address. */
        if (spi_cb->spi_slaves_attribs[i].spi_address == address)
        {
            /* Yes. Get the pointer to the transfer attribute record. */
            spi_config = &(spi_cb->spi_slaves_attribs[i]);

            /* Set the flag to indicate that required transfer attribute
               record has been found. */
            found = NU_TRUE;
        }
    }

    /* Return the transfer attribute configuration structure pointer. */
    return (spi_config);
}

/*************************************************************************
* FUNCTION
*
*       SPICS_Get_Callbacks_Struct
*
* DESCRIPTION
*
*       This function returns the pointer to the callback structure 
*       associated with the given slave of the specified SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*       address                             The slave address whose
*                                           callback structure is
*                                           requested.
*
* OUTPUTS
*
*       SPI_APP_CALLBACKS *                 Pointer to the callback
*                                           structure.
*
*************************************************************************/
SPI_APP_CALLBACKS *SPICS_Get_Callbacks_Struct(SPI_CB    *spi_cb,
                                              UINT16     address)
{
    UINT16                  i;
    SPI_APP_CALLBACKS      *spi_callbacks = NU_NULL;
    BOOLEAN                 found = NU_FALSE;

    /* Search the callback list for a record associated with the
       specified slave address. */
    for (i = 0; (i < spi_cb->spi_slaves_count) && (!found); i++)
    {
        /* Check if the associated address of record at current index
           matches the specified address. */
        if (spi_cb->spi_ucb[i].spi_address == address)
        {
            /* Yes. Get the pointer to the callback record. */
            spi_callbacks = &(spi_cb->spi_ucb[i]);

            /* Set the flag to indicate that required callback record
               has been found. */
            found = NU_TRUE;
        }
    }

    /* Return the callback structure pointer. */
    return (spi_callbacks);
}

#if         (NU_SPI_ERROR_CHECKING)

/*************************************************************************
* FUNCTION
*
*       SPICS_Check_Init_Params
*
* DESCRIPTION
*
*       This function checks Nucleus SPI device initialization parameters
*       for validity.
*
* INPUTS
*
*      *spi_init                            Nucleus SPI device
*                                           initialization block
*                                           pointer.
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_NULL_GIVEN_FOR_INIT             Initialization structure
*                                           pointer is null.
*
*       SPI_NULL_GIVEN_FOR_MEM_POOL         Memory pool pointer is null.
*
*       SPI_INVALID_PORT_ID                 Specified port is not valid.
*
*       SPI_INVALID_DEVICE_ID               Specified device is not valid.
*
*       SPI_INVALID_DRIVER_MODE             Specified driver mode is not
*                                           valid.
*
*       SPI_INVALID_MODE                    Specified SPI mode is not
*                                           valid.
*
*       SPI_INVALID_BIT_ORDER               Specified bit order is invalid.
*
*       SPI_INVALID_QUEUE_SIZE              Specified queue size is not
*                                           valid.
*
*       SPI_INVALID_BUFFER_SIZE             Specified buffer size is not
*                                           valid.
*
*************************************************************************/
STATUS  SPICS_Check_Init_Params(SPI_INIT *spi_init, SPI_CB    *spi_cb)
{
    STATUS      status = NU_SUCCESS;

    /* Check the initialization control block pointer. */
    if (spi_init == NU_NULL)
    {
        /* Set status to indicate that initialization structure pointer
           is null. */
        status = SPI_NULL_GIVEN_FOR_INIT;
    }
    
    /* Check the specified driver mode. */
    else if ((spi_cb->spi_driver_mode != SPI_POLLING_MODE) &&
             (spi_cb->spi_driver_mode != SPI_INTERRUPT_MODE) &&
             (spi_cb->spi_driver_mode != (SPI_INTERRUPT_MODE |
                                          SPI_USER_BUFFERING)))
    {
        /* Set status to indicate that the specified driver mode is not
           valid. */
        status = SPI_INVALID_DRIVER_MODE;
    }

    /* Check if polling mode is specified for an SPI slave device. */
    else if ((spi_cb->spi_driver_mode == SPI_POLLING_MODE) &&
             (spi_cb->spi_master_mode == NU_FALSE))
    {
        /* Set status to indicate that the specified driver mode is not
           valid. */
        status = SPI_INVALID_DRIVER_MODE;
    }


    /* Check default clock polarity and clock phase for validity. */
    else if (((spi_init->spi_clock_polarity != SPI_CPOL_0) &&
              (spi_init->spi_clock_polarity != SPI_CPOL_1)) ||
             ((spi_init->spi_clock_phase != SPI_CPHA_0) &&
              (spi_init->spi_clock_phase != SPI_CPHA_1)))
    {
        /* Specified SPI mode is not valid. */
        status = SPI_INVALID_MODE;
    }

    /* Check default bit order for validity. */
    else if ((spi_init->spi_bit_order != SPI_MSB_FIRST) &&
             (spi_init->spi_bit_order != SPI_LSB_FIRST))
    {
        /* The bit order parameter is not valid. */
        status = SPI_INVALID_BIT_ORDER;
    }
    
    /* Check other initialization parameters. */
    else
    {
        /* If the interrupt driven driver mode is requested then the
           specified queue size and buffer size must be valid. */
        if ((spi_cb->spi_driver_mode & SPI_INTERRUPT_MODE) != 0)
        {
            /* Check if the specified queue size is valid. */
            if (spi_cb->spi_queue_size == 0)
            {
                /* Set status to indicate that the specified queue size
                   is not valid. */
                status = SPI_INVALID_QUEUE_SIZE;
            }

            /* Check if the specified buffer size is valid - if
               user buffers are not being used. */
            else if ((spi_cb->spi_driver_mode == SPI_INTERRUPT_MODE) &&
                     (spi_cb->spi_buffer_size == 0UL))
            {
                /* Set status to indicate that the specified buffer size
                   is not valid. */
                status = SPI_INVALID_BUFFER_SIZE;
            }

            else
            {
                /* No other parameter to check. */
            }
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPICS_Check_Address
*
* DESCRIPTION
*
*       This function checks the specified slave address to see if
*       it is valid for the given SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*       address                             Slave address whose validity
*                                           is to be checked.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_ADDRESS                 Specified slave address is not
*                                           valid for the specified SPI
*                                           device.
*
*************************************************************************/
STATUS  SPICS_Check_Address(SPI_CB     *spi_cb,
                            UINT16      address)
{
    STATUS  status = SPI_INVALID_ADDRESS;
    UINT16  i;

    /* Search the slave attributes list of the device for the specified
       address. */
    for (i = 0; (i < spi_cb->spi_slaves_count) && (status != NU_SUCCESS); i++)
    {
        /* Check if the specified address is valid. */
        if (spi_cb->spi_slaves_attribs[i].spi_address == address)
        {
            /* Yes. The specified slave address is valid. */
            status = NU_SUCCESS;
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* NU_SPI_ERROR_CHECKING */

