/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_init_ext.c
*
* COMPONENT
*
*        Nucleus USB Software : Initialization
*
* DESCRIPTION
*
*        This file contain functions implementation which are common for
*        function and host side.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       nu_os_conn_usb_com_stack_init       Creates the USB Memory pools
*                                           used by stack and class drivers.
*       NU_Printf_USB_Msg                   Custom print routine, for
*                                           debugging purposes if necessary.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
************************************************************************/
#ifndef USB_INIT_EXT_C
#define USB_INIT_EXT_C

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern  NU_MEMORY_POOL           System_Memory;

/******************************************************************************
*
*   FUNCTION
*
*       nu_os_usb_init
*
*   DESCRIPTION
*
*       Creates the USB Memory pools used by stack and class drivers.
*
*   INPUTS
*
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       status                              Status of initialization.
*
******************************************************************************/
STATUS nu_os_conn_usb_com_stack_init(INT startstop)
{
    STATUS  status;
    NU_MEMORY_POOL *usys_pool_ptr;


    if (startstop)
    {
        /* Get system uncached memory pool pointer */
        status = NU_System_Memory_Get(NU_NULL, &usys_pool_ptr);

        if (status == NU_SUCCESS)
        {
            /* Initialize cached and uncached memory pools for USB subsystem. */
            status = USB_Initialize_Memory_Pools(&System_Memory, usys_pool_ptr);
        }
    }
    else
    {
		/* Uninitialize USB subsystem memory pools. */
		status = USB_Uninitialize_Memory_Pools();
    }

    return (status);

}   /* USB_Initialization_Entry */

/*************************************************************************
* FUNCTION
*
*      NU_Printf_USB_Msg
*
* DESCRIPTION
*
*      Custom print routine, for debugging purposes if necessary.
*
******************************************************************************/
VOID NU_Printf_USB_Msg(CHAR *string)
{
    NU_UNUSED_PARAM(string);

    /* Nothing to do. Functionality may be added for debugging purposes. */
}

/*************************************************************************/

#endif /* USB_INIT_EXT_C */
/*************************** end of file ********************************/
