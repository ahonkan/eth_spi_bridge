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
*       i2cms_callbacks.c
*
* COMPONENT
*
*       I2CM - I2C Master
*
* DESCRIPTION
*
*       This file contains the callbacks related functions for Nucleus I2C
*       master.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2CMS_Set_Callbacks                 Sets the callback functions
*                                           for the specified I2C slave
*                                           device.
*
*       I2CMS_Get_Callbacks                 Gets the callbacks for the
*                                           specified I2C slave device.
*
*       I2CMS_Remove_Callbacks              Removes the callbacks for the
*                                           specified I2C slave device.
*
*       I2CMS_Delete_All_Callbacks          Deletes all the callbacks
*                                           registered including the
*                                           default callbacks.
*
*       I2CMS_Get_Callback_Struct           Gets the callback structure
*                                           for the specified I2C slave
*                                           device.
*
*       I2CMS_Get_Mode                      Gets the mode (polling or
*                                           interrupt) of the Nucleus
*                                           I2C device.
*
* DEPENDENCIES
*
*       i2cm_extr.h                         Function prototypes for
*                                           Nucleus I2C master.
*
*************************************************************************/
#define    NU_I2C_SOURCE_FILE

#include    "connectivity/i2cm_extr.h"

/* Scale out this function if only the slave functionality is to be used
   always for all I2C device on this target. */

#if         (!(NU_I2C_NODE_TYPE == I2C_SLAVE_NODE))

/*************************************************************************
* FUNCTION
*
*       I2CMS_Set_Callbacks
*
* DESCRIPTION
*
*       This function sets the callback functions for the specified
*       I2C slave device.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       i2c_slave_address                   The I2C slave device for which
*                                           the callbacks are being added.
*
*      *callbacks                           Callbacks to add for the
*                                           specified I2C slave device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_PARAM_POINTER           Null given instead of a variable
*                                           pointer.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_OS_ERROR                        The call to OS API has returned
*                                           error message.
*
*************************************************************************/
STATUS  I2CMS_Set_Callbacks(I2C_HANDLE i2c_handle,
                            UINT16 i2c_slave_address,
                            I2C_APP_CALLBACKS *callbacks)
{
    I2C_CB         *i2c_cb;

    STATUS          status = NU_SUCCESS;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the callbacks pointer for validity. */
    if (callbacks == NU_NULL)
    {
        /* Indicate that invalid pointer is passed. */
        status = I2C_INVALID_PARAM_POINTER;
    }

    /* Check if above operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    }

    /* Check if control blocks retrieved successfully. */
    if (status == NU_SUCCESS)
    {
        I2C_APP_CALLBACKS_NODE *finger;
        BOOLEAN                 found     = NU_FALSE;

        /* Point to head node. */
        finger   = &i2c_cb->i2c_ucb;

        /* Find the place for callbacks in the list. */
        while ((finger->next != NU_NULL) && !found)
        {
            /* See if callbacks are already registered for the slave. */
            if (finger->next->callback->i2c_slave_address == i2c_slave_address)
            {
                /* Found the callbacks for the slave. */
                found = NU_TRUE;
            }

            /* Not found, move ahead. */
            else
            {
                /* Move to next callbacks. */
                finger   = finger->next;
            }
        }

        /* See if callbacks are not already registered for the slave. */
        if (found == NU_FALSE)
        {
            /* Allocate memory of the callbacks node. */
            status = NU_Allocate_Aligned_Memory(i2c_cb->i2c_memory_pool,
                                                (VOID **)&finger->next,
                                                sizeof(I2C_APP_CALLBACKS_NODE), sizeof(INT), NU_NO_SUSPEND);

            /* Set the next pointer to null. */
            if (finger->next != NU_NULL)
                finger->next->next = NU_NULL;

            if (status == NU_SUCCESS)
            {
                /* Allocate memory of the callbacks. */
                status = NU_Allocate_Aligned_Memory(i2c_cb->i2c_memory_pool,
                                                    (VOID **)&finger->next->callback,
                                                    sizeof(I2C_APP_CALLBACKS), sizeof(INT), NU_NO_SUSPEND);
            }
        }

        /* Check if memory allocated successfully. */
        if (status == NU_SUCCESS)
        {
            /* Point to the new callback structure. */
            finger = finger->next;

            /* Register callbacks. */

            finger->callback->i2c_ack_indication
                = callbacks->i2c_ack_indication;
            finger->callback->i2c_address_indication
                = callbacks->i2c_address_indication;
            finger->callback->i2c_data_indication
                = callbacks->i2c_data_indication;
            finger->callback->i2c_error
                = callbacks->i2c_error;
            finger->callback->i2c_transmission_complete
                = callbacks->i2c_transmission_complete;

            /* Set the I2C address. */
            finger->callback->i2c_slave_address
                = i2c_slave_address;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMS_Get_Callbacks
*
* DESCRIPTION
*
*       This function gets the callbacks for the specified I2C slave
*       device.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       i2c_slave_address                   The I2C slave address for
*                                           which the callbacks are
*                                           required.
*
*       **callbacks                         Pointer for returning the
*                                           callbacks.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*       I2C_INVALID_PARAM_POINTER           Pointer given to return
*                                           callbacks is null.
*
*************************************************************************/
STATUS    I2CMS_Get_Callbacks(I2C_HANDLE i2c_handle,
                              UINT16 i2c_slave_address,
                              I2C_APP_CALLBACKS **callbacks)
{
    I2C_CB           *i2c_cb;

    STATUS            status = NU_SUCCESS;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the callbacks pointer for validity. */
    if (callbacks == NU_NULL)
    {
        /* Indicate that invalid pointer is passed. */
        status = I2C_INVALID_PARAM_POINTER;
    }

    /* Check if above operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Check if device handle is valid and get the port control block
           and device control block. */
         I2CS_Get_CB(i2c_handle, &i2c_cb);
    }

    /* Check if control blocks retrieved successfully. */
    if (status == NU_SUCCESS)
    {
        I2C_APP_CALLBACKS *temp_callbacks;

        /* Get the callbacks structure. */
        temp_callbacks = I2CMS_Get_Callbacks_Struct(i2c_cb, i2c_slave_address);

        /* Check if the callbacks are returned for specified
           slave; not the default callbacks. */
        if (temp_callbacks->i2c_slave_address == i2c_slave_address)
        {
            /* Return the callbacks to caller. */
            *callbacks = temp_callbacks;
        }

        else
        {
            /* Indicate that no callbacks were set for the spedified slave. */
            *callbacks = NU_NULL;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMS_Remove_Callbacks
*
* DESCRIPTION
*
*       This function removes the callbacks for the specified I2C slave
*       device.
*
* INPUTS
*
*       i2c_handle                          Nucleus I2C device handle.
*
*       i2c_slave_address                   I2C Slave device address.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_HANDLE                  Nucleus I2C device handle is
*                                           not valid.
*
*************************************************************************/
STATUS    I2CMS_Remove_Callbacks(I2C_HANDLE i2c_handle,
                                     UINT16 i2c_slave_address)
{
    I2C_CB         *i2c_cb;
    STATUS          status = NU_SUCCESS;
    BOOLEAN         found = NU_FALSE;
    I2C_APP_CALLBACKS_NODE *finger;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
     I2CS_Get_CB(i2c_handle, &i2c_cb);


    /* Point to head node. */
    finger  = &i2c_cb->i2c_ucb;

    /* Find the required callback struct. */
    while ((finger->next != NU_NULL) && (!found))
    {
        /* Check if the finger's next points to the required callback struct. */
        if (finger->next->callback->i2c_slave_address == i2c_slave_address)
        {
            /* Yes. The callbacks are found. */
            found = NU_TRUE;
        }

        /* Else, this is not the required callback struct. */
        else
        {
            /* Move to the next callback node. */
            finger   = finger->next;
        }
    }

    /* Check if callbacks for the requested slave device are registered. */
    if (found == NU_TRUE)
    {
        I2C_APP_CALLBACKS_NODE *to_delete;

        /* Make a temporary pointer to to-be-deleted callbacks node. */
        to_delete = finger->next;

        /* Remove the node from the list. */
        finger->next = to_delete->next;

        /* De-allocate memory of the callbacks. */
        status = NU_Deallocate_Memory((VOID *)to_delete->callback);

        if (status == NU_SUCCESS)
        {
            /* De-allocate memory of the callbacks node. */
            status = NU_Deallocate_Memory((VOID *)to_delete);
        }
    }


    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMS_Delete_All_Callbacks
*
* DESCRIPTION
*
*       This function deletes all the callbacks registered including the
*       default callbacks.
*
* INPUTS
*
*       *i2c_cb                             Pointer to Nucleus I2C control
*                                           block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS    I2CMS_Delete_All_Callbacks(I2C_CB *i2c_cb)
{
    STATUS                  status = NU_SUCCESS;
    I2C_APP_CALLBACKS_NODE *finger;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Point to head node. */
    finger  = &i2c_cb->i2c_ucb;

    /* Find the required callback struct. */
    while (finger->next != NU_NULL)
    {
        I2C_APP_CALLBACKS_NODE *to_delete;

        /* Make a temporary pointer to to-be-deleted callbacks node. */
        to_delete = finger->next;

        /* Remove the node from the list. */
        finger->next = to_delete->next;

        /* De-allocate memory of the callbacks. */
        status |= NU_Deallocate_Memory((VOID *)to_delete->callback);

        /* De-allocate memory of the callbacks. */
        status |= NU_Deallocate_Memory((VOID *)to_delete);
    }

    /* De-allocate memory of the default callbacks. */
    status |= NU_Deallocate_Memory((VOID *)finger->callback);

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2CMS_Get_Callbacks_Struct
*
* DESCRIPTION
*
*       This function returns the callback structure for the specified
*       slave device.
*
* INPUTS
*
*       *i2c_cb                             Pointer to Nucleus I2C control
*                                           block.
*
*       i2c_slave_address                   I2C slave device address.
*
* OUTPUTS
*
*       I2C_APP_CALLBACKS                   Callback structure for the
*                                           specified slave device.
*
*************************************************************************/
I2C_APP_CALLBACKS *I2CMS_Get_Callbacks_Struct(I2C_CB *i2c_cb,
                                              UINT16  i2c_slave_address)
{
    I2C_APP_CALLBACKS      *callbacks;
    I2C_APP_CALLBACKS_NODE *finger;
    BOOLEAN                 found = NU_FALSE;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Point to head node. */
    finger     = &i2c_cb->i2c_ucb;

    /* Find the required callback struct. */
    while ((finger != NU_NULL) && (!found))
    {
        /* Check if the finger points to the required callback struct. */
        if ((finger->callback != NU_NULL) && (finger->callback->i2c_slave_address == i2c_slave_address))
        {
            /* Yes. the callbacks are found. */
            found = NU_TRUE;
        }

        /* This is not the required callback struct. */
        else
        {
            /* Move to next callback node. */
            finger = finger->next;
        }
    }

    /* Check if callbacks for the requested slave device are registered. */
    if (found == NU_TRUE)
    {
        /* Return the callbacks for the slave. */
        callbacks = finger->callback;
    }

    /* Callbacks for the requested slave device are not registered. */
    else
    {
        /* Return the default callbacks. */
        callbacks = i2c_cb->i2c_ucb.callback;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (callbacks);
}

#endif      /* !(NU_I2C_NODE_TYPE == I2C_SLAVE_NODE) */
