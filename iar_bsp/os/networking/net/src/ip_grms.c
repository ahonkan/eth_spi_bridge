/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       ip_grms.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Get_Reasm_Max_Size.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Get_Reasm_Max_Size
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_Get_Reasm_Max_Size
*
*   DESCRIPTION
*
*       This function returns the Maximum Reassembly size for a device.
*
*   INPUTS
*
*       *dev_name               A pointer to the name of the device
*                               for which to retrieve the maximum
*                               reassembly size.
*       *reasm_max_size         A pointer to the area of memory into
*                               which the Maximum Reassembly size will
*                               be stored.
*
*   OUTPUTS
*
*       NU_SUCCESS              The maximum reassembly size for the
*                               device has been retrieved.
*       NU_INVALID_PARM         One of the parameters is invalid.
*
*************************************************************************/
STATUS NU_Get_Reasm_Max_Size(const CHAR *dev_name, UINT32 *reasm_max_size)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_ptr;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* If one of the parameters is NULL, return an error. */
    if ( (dev_name == NU_NULL) || (reasm_max_size == NU_NULL) )
        status = NU_INVALID_PARM;

    /* Otherwise, get the size. */
    else
    {
        /* Grab the semaphore */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the device */
            dev_ptr = DEV_Get_Dev_By_Name(dev_name);

            /* If the device exists on the node, get the maximum
             * reassembly size.
             */
            if (dev_ptr)
                *reasm_max_size = dev_ptr->dev_reasm_max_size;
            else
                status = NU_INVALID_PARM;

            /* Release the semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }
    }

    NU_USER_MODE();

    return (status);

} /* NU_Get_Reasm_Max_Size */
