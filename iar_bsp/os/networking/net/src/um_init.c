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
*       um_init.c
*
*   DESCRIPTION
*
*       Initialize the User Management Module.
*
*   DATA STRUCTURES
*
*       UM_List
*       UM_Protect
*
*   FUNCTIONS
*
*       UM_Initialize
*
*   DEPENDENCIES
*
*       nu_net.h
*       um_defs.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/um_defs.h"

/* Reserve a STATIC address for the anchor node of the UM Double Linked List */
/* List header anchor for UM database entries */
UM_USER_LIST UM_List;

/** Nucleus Protection for Global Variables **/
NU_PROTECT UM_Protect;

/*************************************************************************
*
*   FUNCTION
*
*       UM_Initialize
*
*   DESCRIPTION
*
*       Initialize the User Management module
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID UM_Initialize(VOID)
{
    /* Initialize UM_Protect structure */
    UTL_Zero(&UM_Protect, sizeof(NU_PROTECT));

    /* Set the forward and backward link pointers in the UM_List */
    UM_List.flink = NU_NULL;
    UM_List.blink = NU_NULL;

} /* UM_Initialize */
