/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       pms_boot_state.c
*
*   COMPONENT
*
*       Hibernate
*
*   DESCRIPTION
*
*       Contains all functionality for boot state API.
*
*   DATA STRUCTURES
*
*       PMS_Prev_Boot_State
*
*   FUNCTIONS
*
*       NU_PM_Hibernate_Boot
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       power_core.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"

/* This value is initialized to normal boot by default,
   when the system is resumed and data is restored this
   value will be changed to hibernate resume */
UINT32 PMS_Prev_Boot_State = PM_NORMAL_BOOT;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Hibernate_Boot
*
*   DESCRIPTION
*
*       This function retrieves the boot state for the system.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_TRUE - Indicates the currently running Nucleus OS system booted
*                 from a hibernate level (i.e. the system resumed from a
*                 hibernate level).
*
*       NU_FALSE - Indicates the currently running Nucleus OS system did
*                  not boot from a hibernate level (i.e. the system booted
*                  normally).
*
*************************************************************************/
BOOLEAN NU_PM_Hibernate_Boot(VOID)
{
    BOOLEAN     is_hibernate_boot;
    
    if (PMS_Prev_Boot_State == PM_HIBERNATE_RESUME)
    {
        is_hibernate_boot = NU_TRUE;
    }
    else
    {
        is_hibernate_boot = NU_FALSE;
    }

    return (is_hibernate_boot);
}
