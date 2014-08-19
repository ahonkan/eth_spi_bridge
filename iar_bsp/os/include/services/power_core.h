/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains public data structure definitions, constants 
*       and functions of Power Management Services.
*
***********************************************************************/
#ifndef POWER_CORE_H
#define POWER_CORE_H

#ifdef __cplusplus
extern "C" {
#endif


/*********************************
	PMS Common Definitions
**********************************/
/* Minimum request handle, utilized by
   DVFS and peripheral components */
typedef VOID * PM_MIN_REQ_HANDLE;

/* Power services Event types */ 
#define PM_NO_NOTIFICATIONS         0x00000000
#define PM_ACTIVE_NOTIFICATIONS     0x00000001
#define PM_INACTIVE_NOTIFICATIONS   0x00000002
#define PM_DELETED_NOTIFICATIONS    0x00000004
#define PM_STATE_CHANGE             0x00000008
#define PM_OP_CHANGE                0x00000010
#define PM_HIBERNATE_NOTIFICATIONS  0x00000020
#define PM_RESUME_NOTIFICATIONS     0x00000040


/*********************************
	Error Management Sub-component
**********************************/
/* Error values */
#define PM_NOT_IMPLEMENTED          -1
#define PM_NOT_INITIALIZED          -2
#define PM_INVALID_POINTER          -3
#define PM_UNEXPECTED_ERROR         -4
#define PM_INVALID_DEVICE_ID        -5
#define PM_DEFERRED                 -6
#define PM_INVALID_FREQ_ID          -7
#define PM_INVALID_VOLTAGE_ID       -8
#define PM_INVALID_OP_ID            -9
#define PM_INVALID_REQ_HANDLE       -10
#define PM_INVALID_POWER_STATE      -11
#define PM_NOT_A_FULL_WINDOW        -12
#define PM_SAMPLE_TIME_NOT_SET      -13
#define PM_INVALID_STATE            -14
#define PM_INVALID_SYSTEM_STATE     -15
#define PM_SYSTEM_STATE_NEED_INIT   -16
#define PM_DVFS_NOT_INITIALIZED     -17
#define PM_TRANSITION_FAILED        -18
#define PM_ALREADY_REGISTERED       -19
#define PM_NOT_REGISTERED           -20
#define PM_DUPLICATE_INIT           -21
#define PM_DEVICE_DEFERRED          -22
#define PM_INVALID_WD_HANDLE        -23
#define PM_WD_DELETED               -24
#define PM_WD_EXPIRED               -25
#define PM_WD_NOT_EXPIRED           -26
#define PM_INVALID_REG_HANDLE       -27
#define PM_INVALID_MPL              -28
#define PM_TRANSITION_IN_PROGRESS   -29
#define PM_DVFS_DRIVER_NOT_FOUND    -30
#define PM_INVALID_WD_STATUS        -31
#define PM_INVALID_SIZE             -32
#define PM_DVFS_INFO_FAILURE        -33
#define PM_INVALID_PARAMETER        -34
#define PM_REQUEST_BELOW_MIN        -35

STATUS NU_PM_Register_Error_Handler(VOID(*error_entry)(STATUS, VOID *, VOID *, UINT32),
                                       VOID(**old_error)(STATUS, VOID *, VOID *, UINT32));
STATUS NU_PM_Get_Error_Handler(VOID(**error_ptr)(STATUS, VOID *, VOID *, UINT32));
VOID   NU_PM_Throw_Error(STATUS status, VOID *info_ptr, UINT32 length);

/*********************************
    Idle Scheduler Sub-component
**********************************/
/* Current tick Interval */
extern  UINT32  PMS_Tick_Interval;

/* Single tick value */
extern  UINT32  PMS_Single_Tick;

/* Last HW Timer Value */
extern  UINT32  PMS_Last_HW_Timer_Value;

/* Number of hardware ticks per second */
extern  UINT32  PMS_HW_Timer_Ticks_Per_Sec;

/* Internal API required by macro used by CPU drivers to change scaling factors  */
VOID        PMS_SCHED_Update_Scaling_Factors(UINT32 numerator, 
                                             UINT32 denominator);
                                           
/* Retrieve the current tick */
#define PMS_GET_SINGLE_TICK()                       PMS_Single_Tick

/* Retrieve the current tick interval */
#define PMS_GET_TICK_INTERVAL()                     PMS_Tick_Interval

/* Retrieve last HW timer */
#define PMS_GET_LAST_HW_TIMER()                     PMS_Single_Tick

/* Change one tick to another value. With this function, HW tick value will no longer be SINGLE_TICK. */
#define PMS_SET_SINGLE_TICK(value)                  ((value == 0) ? PMS_Single_Tick : (PMS_Single_Tick = value))

/* Change timer interval value, on some platforms this value will need to be scaled by the CPU driver. */
#define PMS_SET_TICK_INTERVAL(value)                ((value == 0) ? PMS_Tick_Interval : (PMS_Tick_Interval = value))

/* Change last HW value, on some platforms this value will need to be scaled by the CPU driver. */
#define PMS_SET_LAST_HW_TIMER(value)                ((value == 0) ? PMS_Last_HW_Timer_Value : (PMS_Last_HW_Timer_Value = value))
    
/* Change last HW ticks per second, on some platforms this value will need to be scaled by the CPU driver. */
#define PMS_SET_HW_TICKS_PER_SEC(value)             ((value == 0) ? PMS_HW_Timer_Ticks_Per_Sec : (PMS_HW_Timer_Ticks_Per_Sec = value))

/* Microseconds per hardware counter tick */
#define HW_COUNTER_TICKS_PER_US(value)              ((value / (1000/NU_PLUS_Ticks_Per_Second))/1000)

/* Change scaling factors, this will be updated by the CPU driver. */
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)))
#define PMS_SET_SCALING_FACTORS(numerator, denominator) PMS_SCHED_Update_Scaling_Factors(numerator, denominator)
#else
#define PMS_SET_SCALING_FACTORS(numerator, denominator)
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))) */

STATUS NU_PM_Get_CPU_Counters (UINT32 *total_time_ptr,
                                  UINT32 *idle_time_ptr);
STATUS NU_PM_Start_Tick_Suppress(VOID);
STATUS NU_PM_Stop_Tick_Suppress(VOID);

                                             
/*********************************************
	Peripheral State Services Sub-component
*********************************************/
/* Power Aware IOCTLs */
#define POWER_IOCTL_GET_STATE           0
#define POWER_IOCTL_SET_STATE           1
#define POWER_IOCTL_GET_STATE_COUNT     2

#define POWER_IOCTL_TOTAL               3

/* State Constants for device driver states */
#define POWER_OFF_STATE                 0
#define POWER_ON_STATE    		        255

#define POWER_CLASS_LABEL               {0x1a,0x20,0x74,0x16,0x2f,0x5e,0x41,0x1b,0x9b,0xbf,0x2d,0x2c,0x0d,0x7c,0x3b,0xfd}

#define PM_UII_SET_TIMEOUT				0
#define PM_UII_GET_TIMEOUT				1
#define PM_UII_GET_INACTIVE_TYPE		2
#define PM_UII_GET_ACTIVE_TYPE			3
#define PM_UII_GET_ACTIVITY_MASK		4

#define PM_UII_IOCTL_TOTAL              5

typedef struct PM_UII_ARGS_STRUCT
{
	UINT32 value;
	UINT32 mask;
} PM_UII_ARGS;

#define PM_UII_CLASS_LABEL              {0x74,0x5a,0x62,0xa1,0xbe,0xa6,0x4f,0x2b,0x99,0x80,0xcf,0x7b,0x08,0x09,0x6f,0x3b}

typedef UINT32 PM_STATE_ID;

/* Maximum power devices that can be registered in this system */
#define  PM_MAX_DEV_CNT                 DV_DISCOVERY_TASK_MAX_ID_CNT

STATUS NU_PM_Get_Power_State(DV_DEV_ID dev_id, PM_STATE_ID *state_id_ptr);
STATUS NU_PM_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);
STATUS NU_PM_Get_Power_State_Count(DV_DEV_ID dev_id, PM_STATE_ID *state_count_ptr);
STATUS NU_PM_Min_Power_State_Request (DV_DEV_ID dev_id, PM_STATE_ID state,
                                     PM_MIN_REQ_HANDLE *handle_ptr);
STATUS NU_PM_Min_Power_State_Release (DV_DEV_ID dev_id, PM_MIN_REQ_HANDLE handle);


/*********************************************
	System State Services Sub-component
*********************************************/

/* Structure to attach System State Change notification message */
typedef struct PM_SYSTEM_NOTIFICATION_STRUCT
{
    UINT32 event_type;
    UINT8  old_state;
    UINT8  new_state;
}PM_SYSTEM_NOTIFICATION;

STATUS NU_PM_System_State_Init(UINT8 state_count);
STATUS NU_PM_Get_System_State(UINT8 *state_ptr);
STATUS NU_PM_Set_System_State(UINT8 state_id);
STATUS NU_PM_Get_System_State_Count(UINT8 *state_count_ptr);
STATUS NU_PM_Get_System_State_Map(UINT8 system_state_id, DV_DEV_ID dev_id,
                                 PM_STATE_ID *state_id_ptr);
STATUS NU_PM_Unmap_System_Power_State(DV_DEV_ID dev_id);
STATUS NU_PM_Map_System_Power_State(UINT8 system_state_id, DV_DEV_ID dev_id,
                                   PM_STATE_ID state);
STATUS NU_PM_System_Min_State_Request(UINT8 state, PM_MIN_REQ_HANDLE *handle_ptr);
STATUS NU_PM_System_Min_State_Release(PM_MIN_REQ_HANDLE handle);
STATUS NU_PM_Emergency_State_Request(UINT8 state, PM_MIN_REQ_HANDLE *handle_ptr);
STATUS NU_PM_Emergency_State_Release(PM_MIN_REQ_HANDLE handle);


/*********************************************
	DVFS Services Sub-component
*********************************************/
#define PM_STARTUP_OP                   255
#define PM_MAX_SPECIFIC_INFO_LEN        20
#define PM_DVFS_SET_OP_THD_OP_OFFSET    0
#define PM_DVFS_SET_OP_THD_LEVEL_OFFSET 256

/* Structure to attach OP Change notification message */
typedef struct PM_OP_NOTIFICATION_CB
{
    UINT32   pm_event_type;                 /* Event type   */
    UINT8    pm_old_op;                     /* Old OP index */
    UINT8    pm_new_op;                     /* New OP index */
#if PAD_2
    UINT8    pm_padding[PAD_2];
#endif
} PM_OP_NOTIFICATION;


/* Enumerated type to pass to device
   park/resume notifications */
typedef enum
{
    PM_PARK                     = 0,
    PM_RESUME                   = 1
} PM_DVFS_NOTIFY_TYPE;

/* Enumerated type to toggle notification
   on and off */
typedef enum
{
    PM_NOTIFY_OFF               = 0,
    PM_NOTIFY_ON                = 1
} PM_DVFS_NOTIFY;

typedef struct PM_MPL_CB
{
    UINT32 pm_duration;                     /* Maximum time driver can be parked */
    UINT32 pm_park_time;                    /* Maximum time to park driver */
    UINT32 pm_resume_time;                  /* Maximum time needed to resume */
} PM_MPL;

/* Handle used for DVFS notification */
typedef VOID *PM_DVFS_HANDLE;

/* Define a type for the function pointer to notify
   devices to park and resume */
typedef STATUS (*PM_NOTIFY_FUNC)(VOID *, PM_DVFS_NOTIFY_TYPE);

STATUS NU_PM_Get_Freq_Info(UINT8 freq_id, UINT32 *freq_ptr);
STATUS NU_PM_Get_Clock_Info(UINT8 freq_id, UINT32 *freq_ptr, UINT8 *voltage_id_ptr);
STATUS NU_PM_Get_Voltage_Info(UINT8 voltage_point_id, UINT16 *voltage_ptr);
STATUS NU_PM_Get_Current_OP(UINT8 * op_id_ptr);
STATUS NU_PM_Get_Current_VF(UINT8 *freq_id_ptr, UINT8 *voltage_id_ptr);
STATUS NU_PM_Get_OP_Additional_Info(UINT8 op_id, VOID *info_ptr, UINT32 size);
STATUS NU_PM_Get_OP_Specific_Info(UINT8 op_id, CHAR *identifier, VOID *info_ptr, UINT32 size);
STATUS NU_PM_Get_OP_VF(UINT8 op_id, UINT8 *freq_id_ptr, UINT8 *voltage_id_ptr);
STATUS NU_PM_Get_OP_Count(UINT8 *op_count_ptr);
STATUS NU_PM_Get_Freq_Count(UINT8 *freq_count_ptr);
STATUS NU_PM_Get_Voltage_Count(UINT8 *voltage_count_ptr);
STATUS NU_PM_Get_VF_Counts(UINT8 *freq_count_ptr, UINT8 *voltage_count_ptr);
STATUS NU_PM_Set_Current_OP(UINT8 op_id);
STATUS NU_PM_Request_Min_OP(UINT8 op_id, PM_MIN_REQ_HANDLE *handle_ptr);
STATUS NU_PM_Release_Min_OP(PM_MIN_REQ_HANDLE handle);
STATUS NU_PM_Set_Current_Clock(UINT8 freq_id);
STATUS NU_PM_Set_Current_Freq(UINT8 freq_id);
STATUS NU_PM_Set_Current_Voltage(UINT8 voltage_point_id);
STATUS NU_PM_DVFS_Register(PM_DVFS_HANDLE *dvfs_handle, VOID *instance_handle, PM_NOTIFY_FUNC dvfs_notify_cb);
STATUS NU_PM_DVFS_Unregister(PM_DVFS_HANDLE dvfs_handle);
STATUS NU_PM_DVFS_Update_MPL(PM_DVFS_HANDLE dvfs_handle, PM_MPL *mpl, PM_DVFS_NOTIFY dvfs_notify);
STATUS NU_PM_DVFS_Control_Transition(BOOLEAN new_value, BOOLEAN *previous_value);


/*********************************************
	Watchdog Services Sub-component
*********************************************/
/* Minimum request handle, utilized by Watchdog component */
typedef VOID *PM_WD_HANDLE;

/* Ticks per 100 ms (100 ms is the range in which watchdog timeout is defined) */
#define PM_TICKS_PER_100MS          (NU_PLUS_TICKS_PER_SEC * 100 / 1000)

/* Structure to attach device activity/inactivity change notification message */
typedef struct PM_WD_NOTIFICATION_STRUCT
{
    UINT32       pm_event_type;             /* Event type   */
    DV_DEV_ID    pm_dev_id;                 /* Sender device ID */
} PM_WD_NOTIFICATION;

STATUS NU_PM_Create_Watchdog(UINT16 timeout_value, PM_WD_HANDLE *handle_ptr);
STATUS NU_PM_Delete_Watchdog(PM_WD_HANDLE handle);
STATUS NU_PM_Reset_Watchdog(PM_WD_HANDLE handle);
STATUS NU_PM_Is_Watchdog_Expired(PM_WD_HANDLE handle, UINT8 blocking_flag);
STATUS NU_PM_Is_Watchdog_Active(PM_WD_HANDLE handle, UINT8 blocking_flag);
STATUS NU_PM_Set_Watchdog_Notify(PM_WD_HANDLE handle, STATUS wd_status,
                                DV_DEV_ID sender_dev_id, UINT32 notify_type) ESAL_TS_RTE_DEPRECATED;
STATUS NU_PM_Set_Watchdog_Notification(PM_WD_HANDLE handle, STATUS wd_status,
                                      DV_DEV_ID sender_dev_id);
                                  
                                  
/*********************************************
	  Hibernate Services Sub-component
*********************************************/   
/* Hibernate Boot State Definitions */
#define PM_NORMAL_BOOT              0
#define PM_HIBERNATE_RESUME         1

/* Hibernate Level Definitions */
#define NU_DORMANT                  0
#define NU_STANDBY                  1

/* Self Refresh RAM check */
#define PM_SELF_REFRESH_OFF         0
#define PM_SELF_REFRESH_ON          0x53454C46UL

/* Structure to attach hibernate/resume notifications */                                   
typedef struct PM_HIB_RES_NOTIFICATION_STRUCT
{
    UINT32       pm_event_type;             
} PM_HIB_RES_NOTIFICATION;                                      

BOOLEAN NU_PM_Hibernate_Boot(VOID);
STATUS NU_PM_Set_Hibernate_Exit_OP (UINT8 op_id);
STATUS NU_PM_Get_Hibernate_Exit_OP (UINT8 *op_id_ptr);
STATUS NU_PM_Set_Hibernate_Level(UINT8 level);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
STATUS NU_PM_Restore_Hibernate_Device (DV_DEV_ID        dev_id,
                                       INT              ioctl_num);
#define NU_PM_Hibernate_Level_Count()   CFG_NU_OS_DRVR_HIBERNATE_LEVEL_COUNT
#else
#define NU_PM_Hibernate_Level_Count()   0
#endif

#ifdef __cplusplus
}
#endif

#endif  /* POWER_CORE_H */
