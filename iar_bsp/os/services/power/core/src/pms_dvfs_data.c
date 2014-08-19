/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*        pms_dvfs_data.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all the definitions of the global DVFS data structures
*
*   DATA STRUCTURES
*
*       PM_DVFS_Frequency_Count
*       PM_DVFS_OP_Count
*       PM_DVFS_Voltage_Count
*       PM_DVFS_Current_OP
*       PM_DVFS_Minimum_OP
*       PM_DVFS_OP_Array
*       PM_DVFS_Voltage_Array
*       PM_DVFS_Frequency_Array
*       PM_DVFS_Device_ID
*       PM_DVFS_CPU_Handle
*       PM_DVFS_CPU_Base
*       PM_DVFS_Device_List
*       PM_DVFS_Device_Count
*       PM_DVFS_List_Protect
*       PM_DVFS_Min_Request_List
*       PM_DVFS_Smallest_Duration
*       PM_DVFS_Resume_Array
*       PM_DVFS_Transitions
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       power_core.h
*     ` dvfs.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/dvfs.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

/* The number of unique frequencies available in the CPU driver */
UINT8          PM_DVFS_Frequency_Count;

/* The number of operating points available in the CPU driver */
UINT8          PM_DVFS_OP_Count;

/* The number of unique voltages available in the CPU driver */
UINT8          PM_DVFS_Voltage_Count;

/* The Current Operating point in use */
UINT8          PM_DVFS_Current_OP;

/* The minimum operating point that can be set */
UINT8          PM_DVFS_Minimum_OP;

/* Pointer array that points to each of the operating points */
PM_OP        **PM_DVFS_OP_Array;

/* Pointer array that points to each of the unique voltages */
PM_VOLTAGE   **PM_DVFS_Voltage_Array;

/* Pointer array that points to each of the unique frequencies */
PM_FREQUENCY **PM_DVFS_Frequency_Array;

/* Driver ID for DVFS */
DV_DEV_ID      PM_DVFS_Device_ID;

/* Handle to the currently open CPU driver */
DV_DEV_HANDLE  PM_DVFS_CPU_Handle;

/* Base to CPU ioctls in the currently open driver */
UINT32         PM_DVFS_CPU_Base;

/* List of registered devices */
CS_NODE       *PM_DVFS_Device_List;

/* List of minimum OP requests */
CS_NODE       *PM_DVFS_Min_Request_List;

/* Number of registered devices */
UINT32         PM_DVFS_Device_Count;

/* Number of min requests */
UINT32         PM_DVFS_Min_Request_Count;

/* Protect structure for the driver list */
NU_PROTECT     PM_DVFS_List_Protect;

/* Smallest Duration */
UINT32         PM_DVFS_Smallest_Duration;

/* Array of the devices waiting to be resumed */
PM_DVFS_REG   *PM_DVFS_Resume_Array[PM_MAX_DEV_CNT];

/* Determines if DVFS transitions are allowed to run */
BOOLEAN        PM_DVFS_Transitions = NU_TRUE;

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


