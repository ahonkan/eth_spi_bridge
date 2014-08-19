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
*       ike_psk_dini.c
*
* COMPONENT
*
*       IKE - Pre-shared Keys
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE pre-shared
*       keys de-initialization function.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Deinitialize_Preshared_Keys
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
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
*       IKE_Deinitialize_Preshared_Keys
*
* DESCRIPTION
*
*       This function de-initializes the IKE pre-shared keys
*       database. It removes all items which are present in
*       the database.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful initialization.
*
************************************************************************/
STATUS IKE_Deinitialize_Preshared_Keys(VOID)
{
    IKE_PRESHARED_KEY   *psk;
    IKE_PRESHARED_KEY   *next_psk;

    /* Log debug message. */
    IKE_DEBUG_LOG("De-initializing Pre-shared Keys");

    /* Start from the first item in the database. */
    psk = IKE_Preshared_Key_DB.ike_flink;

    /* Loop for all items in the database. */
    while(psk != NU_NULL)
    {
        /* Store pointer to the next item. */
        next_psk = psk->ike_flink;

        /* Deallocate the pre-shared key memory. */
        if(NU_Deallocate_Memory(psk) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Move to the next item in the list. */
        psk = next_psk;
    }

    /* Set link pointers of the pre-shared key DB to NULL. */
    IKE_Preshared_Key_DB.ike_flink = NU_NULL;
    IKE_Preshared_Key_DB.ike_last = NU_NULL;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Deinitialize_Preshared_Keys */

#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */
