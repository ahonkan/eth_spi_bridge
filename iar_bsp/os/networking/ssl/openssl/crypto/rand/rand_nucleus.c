/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/**************************************************************************
*
*   FILENAME
*
*       rand_nucleus.c
*
*   DESCRIPTION
*
*       This file implements Nucleus specific functions for rand component.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       RAND_poll
*
*   DEPENDENCIES
*
*       cryptlib.h
*       rand.h
*       rand_lcl.h
*
****************************************************************************/

#include "cryptlib.h"
#include <openssl/rand.h>
#include "rand_lcl.h"

#if defined (OPENSSL_SYS_NUCLEUS)


/* The FAQ indicates we need to provide at least 20 bytes (160 bits) of seed.
*/

INT RAND_poll(VOID)
{
    UINT32 l;
    INT i; 
    
    /* Start with Nucleus clock. */
    l = NU_Retrieve_Clock();
    RAND_add(&l, sizeof(l), 1);
    
    /* Make use of current task pointer which sould be random to some extent. */
    l = (UINT32) NU_Current_Task_Pointer();
    RAND_add(&l, sizeof(l), 1);
    
    for (i = 2; i < ENTROPY_NEEDED; i++)
    {
        NU_Retrieve_Hardware_Clock(l);
        
        RAND_add(&l, sizeof(l), 1);
        
        l = NU_Retrieve_Clock();
        RAND_add(&l, sizeof(l), 0);

        /* Introduce some (non-deterministic) delay. */
        NU_Relinquish();
    }

    return 1;
}

#endif /* OPENSSL_SYS_NUCLEUS */ 
