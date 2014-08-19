/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       sck6_dpe.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Delete_Prefix_Entry.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Delete_Prefix_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*       prefix6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Delete_Prefix_Entry                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function deletes a prefix entry from the Prefix List for a 
*       specified interface.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       dev_index               The interface index of the interface to 
*                               configure.
*       *prefix                 A pointer to the Prefix to delete.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Delete_Prefix_Entry(UINT32 device_index, const UINT8 *prefix)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Validate the interface and prefix */
    if (prefix == NU_NULL)
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /* Get a pointer to the device */
    dev_ptr = DEV_Get_Dev_By_Index(device_index);

    /* Validate the interface */
    if (dev_ptr != NU_NULL)
    {
        /* Delete the prefix entry */
        PREFIX6_Delete_Prefix(dev_ptr, prefix);
    }

    else
        status = NU_INVALID_PARM;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Delete_Prefix_Entry */
