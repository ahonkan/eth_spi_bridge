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
*       pms_selfrefresh.c
*
*   COMPONENT
*
*       Self-Refresh
*
*   DESCRIPTION
*
*       Contains all functionality for self-refresh
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_SelfRefresh_Enter
*       PMS_SelfRefresh_Exit
*
*   DEPENDENCIES
*
*       <stdio.h>
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       runlevel.h
*       selfrefresh.h
*       error_management.h
*       device_manager.h
*
*************************************************************************/


#include  <stdio.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/services/init/inc/runlevel.h"
#include "os/services/power/core/inc/selfrefresh.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "os/kernel/devmgr/inc/device_manager.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)

static CPU_SELF_REFRESH_CB PMS_Self_Refresh_Handle;
static DV_DEV_HANDLE   PMS_SelfRefresh_CPU_Handle;
static STATUS   PMS_SelfRefresh_Device_Register_Callback(DV_DEV_ID device, VOID *context);

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SelfRefresh_Initialize
*
*   DESCRIPTION
*
*       This function initialize the SDRAM self-refresh mode 
*
*   INPUT
*
*       mem_pool                              Memory pool pointer.      
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_SelfRefresh_Initialize(NU_MEMORY_POOL* mem_pool)
{
    DV_DEV_LABEL       selfrefresh_class_id = {CPU_SELFREFRESH_CLASS_LABEL};
    DV_LISTENER_HANDLE listener_id;

    /* Avoid compile-time warnings. */
    NU_UNUSED_PARAM(mem_pool);

    /* Call DM API to add callback for device register event. We are not
     * interested in un-registration at the moment. */
    (VOID)DVC_Reg_Change_Notify(&selfrefresh_class_id,
                                DV_GET_LABEL_COUNT(selfrefresh_class_id),
                                &PMS_SelfRefresh_Device_Register_Callback,
                                NU_NULL,
                                NU_NULL,
                                &listener_id);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SelfRefresh_Device_Register_Callback
*
*   DESCRIPTION
*
*       This function initializes the Self Refresh component of the PMS.  
*       This function is called when any device with 
*       CPU_SELFREFRESH_CLASS_LABEL is registered with device manager. 
*       This function opens the CPU driver and updates all related 
*       Self-Refresh structures.
*
*   INPUT
*
*       device                              Newly registered Device ID.
*       context                             Any context from caller.
*
*   OUTPUT
*
*       NU_SUCCESS                          Function returns success.
*       (Device Manager error values)
*
*************************************************************************/
static STATUS PMS_SelfRefresh_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS           status;
    DV_DEV_LABEL     selfrefresh_class_id = {CPU_SELFREFRESH_CLASS_LABEL};
    DV_IOCTL0_STRUCT ioctl0;

    /* Open the CPU driver for Self-Refresh usage, save the handle for later usage */
    status = DVC_Dev_ID_Open(device, &selfrefresh_class_id, DV_GET_LABEL_COUNT(selfrefresh_class_id), &PMS_SelfRefresh_CPU_Handle);
    if (status == NU_SUCCESS)
    {
        /* Get ioctl base */
        ioctl0.label = selfrefresh_class_id;
        status = DVC_Dev_Ioctl(PMS_SelfRefresh_CPU_Handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));

    }
        
    if (status == NU_SUCCESS)
    {
        /* Retrieve the self refresh calls. */
        status = DVC_Dev_Ioctl(PMS_SelfRefresh_CPU_Handle, (ioctl0.base + CPU_SELFREFRESH_IOCTL_INIT), &PMS_Self_Refresh_Handle, sizeof(CPU_SELF_REFRESH_CB));
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SelfRefresh_Enter
*
*   DESCRIPTION
*
*       This function takes SDRAM into self-refresh mode 
*
*   INPUT
*
*       None       
*
*   OUTPUT
*
*       NU_SUCCESS                          Function returns success.
*       NU_NOT_REGISTERED                   Self-Refresh callbacks not 
*                                           registered.
*
*************************************************************************/
STATUS PMS_SelfRefresh_Enter(VOID)
{   
    STATUS status = NU_NOT_REGISTERED;
 
    /* Check for a valid self refresh entry function. */
    if((PMS_Self_Refresh_Handle.selfrefresh_enter_func) &&
      (PMS_Self_Refresh_Handle.self_refresh == NU_FALSE))
    {
        /* Execute self-refresh enter callback. */
        PMS_Self_Refresh_Handle.selfrefresh_enter_func();

        /* Set the self-refresh flag to TRUE. */
        PMS_Self_Refresh_Handle.self_refresh = NU_TRUE;	

        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SelfRefresh_Exit
*
*   DESCRIPTION
*
*       This function takes SDRAM out of self-refresh mode
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS                          Function returns success.
*       NU_NOT_REGISTERED                   Self-Refresh callbacks not 
*                                           registered.
*
*************************************************************************/
STATUS PMS_SelfRefresh_Exit(VOID)
{
    STATUS status = NU_NOT_REGISTERED;
 
    /* Check for a valid sefl refresh exit function. */
    if((PMS_Self_Refresh_Handle.selfrefresh_exit_func) &&
      (PMS_Self_Refresh_Handle.self_refresh == NU_TRUE))
    {
        /* Execute self-refresh exit callback. */
        PMS_Self_Refresh_Handle.selfrefresh_exit_func();

        /* Set the self-refresh flag to FALSE. */
        PMS_Self_Refresh_Handle.self_refresh = NU_FALSE;

        status = NU_SUCCESS;
    }

    return status;
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE) */
