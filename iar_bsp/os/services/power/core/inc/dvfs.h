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
*       This file contains private data structure definitions,  
*       constants and functions of DVFS subcomponent.
*
***********************************************************************/
#ifndef PMS_DVFS_H
#define PMS_DVFS_H

#ifdef __cplusplus
extern "C" {
#endif

#define PM_DVFS_MAX_CPU_COUNT    1
#define PM_OP_REQUEST_ID         0x4F504944UL
#define PM_DVFS_REG_ID           0x44564653UL
#define PM_MAX_DURATION          0xFFFFFFFFUL
#define PM_SET_OP_STACK_SIZE     CFG_NU_OS_SVCS_PWR_CORE_SET_OP_STACK_SIZE
#define PM_SET_OP_QUEUE_SIZE     1          /* Size in terms of size of UNSIGNED */
#define PM_SET_OP_TASK_PRIORITY  0
#define PM_SET_OP_TASK_TIMESLICE 0


typedef struct PM_OP_CB
{
    UINT8 pm_op_id;                         /* Driver's op ID */
    UINT8 pm_freq_id;                       /* Index to frequency array */
    UINT8 pm_volt_id;                       /* Index to voltage array */
#if PAD_3
    UINT8 pm_padding[PAD_3];
#endif
} PM_OP;

typedef struct PM_VOLTAGE_CB
{
    UINT16 pm_voltage;
#if PAD_2
    UINT8  pm_padding[PAD_2];
#endif
} PM_VOLTAGE;

typedef struct PM_FREQUENCY_CB
{
    UINT32 pm_frequency;                    /* Frequency in Hz */
    UINT8  pm_volt_id;                      /* Index to voltage array */
#if PAD_1
    UINT8  pm_padding[PAD_1];
#endif
} PM_FREQUENCY;

typedef struct PM_DVFS_REG_CB
{
    CS_NODE                pm_node;         /* Link list node */
    UINT32                 pm_id;           /* Used to determine if registry control block is valid */
    PM_NOTIFY_FUNC         pm_notify_func;  /* Function used to park/resume */
    VOID                  *pm_instance;     /* Pointer to the instance handle */
    PM_DVFS_NOTIFY         pm_notify;       /* indicates if the driver needs to be notified or used in the calculation for MPL */
    PM_MPL                 pm_mpl;          /* Device MPL */
} PM_DVFS_REG;

typedef struct PM_OP_REQUEST_CB
{
    CS_NODE  pm_node;                       /* Link list node */
    UNSIGNED pm_id;                         /* Used to determine if OP Request control block is valid */
    UINT8    pm_op_id;                      /* Minimum OP requested */
#if PAD_1
    UINT8    pm_padding[PAD_1];
#endif
} PM_OP_REQUEST;


STATUS PMS_DVFS_Status_Check(VOID);
STATUS PMS_DVFS_Set_OP_Initialize(NU_MEMORY_POOL *mem_pool_ptr);
VOID   PMS_DVFS_Set_OP_Task_Entry(UNSIGNED argc, VOID *argv);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_DVFS_H */


