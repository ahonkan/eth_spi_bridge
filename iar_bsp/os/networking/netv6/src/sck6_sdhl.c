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
*       sck6_sdhl.c                                    
*
*   DESCRIPTION
*
*        This file contains the implementation of
*        NU_Update_Default_Hop_Limit6.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        NU_Set_Default_Hop_Limit
*
*   DEPENDENCIES
*
*        nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern UINT8 IP6_Hop_Limit;

/************************************************************************
*
*   FUNCTION
*
*       NU_Set_Default_Hop_Limit
*
*   DESCRIPTION
*
*        This function is used to update the value of default Hop Limit.
*
*   INPUTS
*
*        new_hop_limit          The new value of Default Hop Limit.
*
*   OUTPUTS
*
*        NU_SUCCESS             Successfully set the default Hop Limit.
*
************************************************************************/
STATUS NU_Set_Default_Hop_Limit(UINT8 new_hop_limit)
{
    STATUS      status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    /* If we successfully grab the semaphore. */
    if (status == NU_SUCCESS)
    {
        /* Update the value of default hop limit. */
        IP6_Hop_Limit = new_hop_limit;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to obtain the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (status);

} /* NU_Set_Default_Hop_Limit */
