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

/*************************************************************************
*
*   FILENAME
*
*       dvd.c
*
*   COMPONENT
*
*       DV - Device Manager
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       Device Manager component.
*
*   DATA STRUCTURES
*
*       DVD_Dev_Registry                    The device registry
*       DVD_Reg_Active_Cnt                  Number of active devices in
*                                           the device registry
*       DVD_Reg_Next_Dev_Index              The next available device index
*       DVD_Reg_Change_Event                Event control block to monitor
*                                           device registry changes
*       DVD_Dev_Reg_Listener_Array          Listeners for device registry
*                                           changes
*       DVD_Dev_Reg_Listener_Semaphore      Protection semaphore for
*                                           listener array
*       DVD_Dev_Discovery_Task              Task for dynamic device discovery
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       device_manager.h
*
*************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/devmgr/inc/device_manager.h"

/* Device Manager initialized flag */
UNSIGNED         DVD_Initialized = 0;

/* Maximum number of device ids */
INT              DVD_Max_Dev_Id_Cnt;

/* Device Registry */
DV_DEV_REGISTRY  *DVD_Dev_Registry;

/* Device Registry device count */
INT              DVD_Reg_Active_Cnt;

/* Device Registry next available device id */
INT32            DVD_Reg_Next_Dev_Index;

/* Device Registry Change Event Group control block */
NU_EVENT_GROUP   DVD_Reg_Change_Event;

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
/* Saved listeners to listen device registry changes. */
DV_DEV_LISTENER *DVD_Dev_Reg_Listener_Array[DV_MAX_DEVICE_LISTENERS];

/* DVD_Dev_Reg_Listener_Semaphore will protect simultaneous
   write access to device listener array by two threads.  */
NU_SEMAPHORE     DVD_Dev_Reg_Listener_Semaphore;

/* Control block for device discovery task. */
NU_TASK          DVD_Dev_Discovery_Task;
#endif /* (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE) */
