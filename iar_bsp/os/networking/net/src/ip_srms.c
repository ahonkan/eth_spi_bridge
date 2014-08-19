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
*       ip_srms.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Set_Reasm_Max_Size.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Set_Reasm_Max_Size
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
*       NU_Set_Reasm_Max_Size
*
*   DESCRIPTION
*
*       This function changes the Maximum Reassembly size for a
*       device.
*
*   INPUTS
*
*       *dev_name               A pointer to the name of the device
*                               for which to modify the maximum
*                               reassembly size.
*       reasm_max_size          The new maximum reassembly size for
*                               the device.
*
*   OUTPUTS
*
*       NU_SUCCESS              The maximum reassembly size for the
*                               device has been changed.
*       NU_INVALID_PARM         One of the parameters is invalid.
*
*************************************************************************/
STATUS NU_Set_Reasm_Max_Size(const CHAR *dev_name, UINT32 reasm_max_size)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_ptr;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* If the new maximum reassembly size is less than the minimum
     * specified by RFC 1122, return an error.
     */
    if (reasm_max_size < MIN_REASM_MAX_SIZE)
        status = NU_INVALID_PARM;

    /* Otherwise, set the new size. */
    else
    {
        /* Grab the semaphore */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the device */
            dev_ptr = DEV_Get_Dev_By_Name(dev_name);

            /* If the device exists on the node, set the new maximum
             * reassembly size.
             */
            if (dev_ptr)
                dev_ptr->dev_reasm_max_size = reasm_max_size;
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

} /* NU_Set_Reasm_Max_Size */
