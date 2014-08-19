/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
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
*       ike_psk_rp.c
*
* COMPONENT
*
*       IKE - Pre-shared Keys
*
* DESCRIPTION
*
*       This file contains the implementation of functions for
*       removing a pre-shared key from the IKE database.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Remove_Preshared_Key
*       IKE_Get_Preshared_Key_Entry
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)

/* External variables. */
extern IKE_PRESHARED_KEY_DB IKE_Preshared_Key_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_Preshared_Key
*
* DESCRIPTION
*
*       This function removes a pre-shared key from the IKE
*       pre-shared database. The pre-shared key to be removed
*       is identified by its unique index.
*
*       This is an IKE API function.
*
* INPUTS
*
*       index                   Index of the pre-shared key
*                               to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_NOT_FOUND           A pre-shared key with the
*                               specified index was not found.
*
************************************************************************/
STATUS IKE_Remove_Preshared_Key(UINT16 index)
{
    STATUS              status;
    IKE_PRESHARED_KEY   *psk_ptr = NU_NULL;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* First grab the semaphore. */
    status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Get pre-shared key based on index. */
        status = IKE_Get_Preshared_Key_Entry(&psk_ptr, index);

        if(status == NU_SUCCESS)
        {
            /* Remove the pre-shared key from the database. */
            SLL_Remove(&IKE_Preshared_Key_DB, psk_ptr);
        }

        /* Now everything is done, release the semaphore. */
        if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* If successfully removed from the list. */
        if(status == NU_SUCCESS)
        {
            /* Deallocate the pre-shared key memory. This
             * need not be executed within the semaphore.
             */
            if(NU_Deallocate_Memory(psk_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate the memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain IKE semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IKE_Remove_Preshared_Key */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Preshared_Key_Entry
*
* DESCRIPTION
*
*       This is an internal utility function. It returns
*       the pointer to the required pre-shared key, given
*       the unique index of the pre-shared key.
*
* INPUTS
*
*       **ret_psk               On return, contains a pointer
*                               to the pre-shared key.
*       index                   Index of the pre-shared key to
*                               be searched.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           A pre-shared key with the
*                               specified index was not found.
*
************************************************************************/
STATUS IKE_Get_Preshared_Key_Entry(IKE_PRESHARED_KEY **ret_psk,
                                   UINT16 index)
{
    STATUS              status = IKE_NOT_FOUND;
    IKE_PRESHARED_KEY   *cur_psk;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure that return pre-shared key pointer is not NULL. */
    if(ret_psk == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to first item in the pre-shared key database. */
    cur_psk = IKE_Preshared_Key_DB.ike_flink;

    /* Traverse all items in the pre-shared key database. */
    while(cur_psk != NU_NULL)
    {
        /* Compare the two indices. */
        if(cur_psk->ike_index == index)
        {
            /* Return pointer of the pre-shared key to the caller. */
            *ret_psk = cur_psk;

            /* Set status to success. */
            status = NU_SUCCESS;

            /* Leave while loop. */
            break;
        }

        /* Move to the next item in the database. */
        cur_psk = cur_psk->ike_flink;
    }

    /* Return the status value. */
    return (status);

} /* IKE_Get_Preshared_Key_Entry */

#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */
