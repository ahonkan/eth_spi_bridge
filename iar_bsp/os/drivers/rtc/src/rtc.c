/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       rtc.c
*
*   COMPONENT
*
*       RTC                            - Generic Nucleus RTC Driver
*
*   DESCRIPTION
*
*       This file contains the generic RTC Driver functions.
*
*   FUNCTIONS
*
*       NU_Retrieve_RTC_Time
*       NU_Set_RTC_Time
*       NU_Close_RTC
*
*   DEPENDENCIES
*
*       time.h
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <time.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* Device handle for RTC driver */
static DV_DEV_HANDLE RTC_Driver;


/***********************************************************************
*
*   FUNCTION
*
*       NU_Retrieve_RTC_Time
*
*   DESCRIPTION
*
*       This function retrieves the current RTC time.
*
*   INPUTS
*
*       tm      *new_time_ptr       - Pointer to time structure defined in time.h
*
*   OUTPUTS
*
*       A constant value of NU_SUCCESS if time is retrieved, else -1
*
***********************************************************************/
STATUS NU_Retrieve_RTC_Time(struct tm *cur_time_ptr)
{
    STATUS           status = NU_SUCCESS;
    DV_DEV_LABEL     rtc_label[1] = {{RTC_LABEL}};

    if(cur_time_ptr != NU_NULL)
    {
        /* See if RTC driver still needs to be open. */
        if (RTC_Driver == NU_NULL)
        {
            /* Try to open the RTC driver */
            status = DVC_Dev_Open(rtc_label, &RTC_Driver);
        }

        /* Check the completion status of last operation. */
        if(status == NU_SUCCESS)
        {
            /* Read the current time from RTC driver. */
            status = DVC_Dev_Read(RTC_Driver, (VOID*)cur_time_ptr, sizeof(struct tm), NU_NULL, NU_NULL);
        }
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Set_RTC_Time
*
*   DESCRIPTION
*
*       This function sets the RTC time.
*
*   INPUTS
*
*       tm      *new_time_ptr       - Pointer to time structure defined in time.h
*
*   OUTPUTS
*
*       A constant value of NU_SUCCESS if time is set, else -1
*
***********************************************************************/
STATUS NU_Set_RTC_Time(struct tm *cur_time_ptr)
{
    STATUS           status = NU_SUCCESS;
    DV_DEV_LABEL     rtc_label[1] = {{RTC_LABEL}};

    if(cur_time_ptr != NU_NULL)
    {
        /* See if RTC driver still needs to be open. */
        if (RTC_Driver == NU_NULL)
        {
            /* Try to open the RTC driver */
            status = DVC_Dev_Open(rtc_label, &RTC_Driver);
        }

        /* Check the completion status of last operation. */
        if(status == NU_SUCCESS)
        {
            /* Write the time information to the RTC driver. */
            status = DVC_Dev_Write(RTC_Driver, (VOID*)cur_time_ptr, sizeof(struct tm), NU_NULL, NU_NULL);
        }
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Close_RTC
*
*   DESCRIPTION
*
*       This function closes the RTC.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Close_RTC(VOID)
{
    if(RTC_Driver != NU_NULL)
    {
        /* Close the RTC driver. */
        DVC_Dev_Close(RTC_Driver);
    }
}

