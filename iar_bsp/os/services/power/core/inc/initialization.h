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
*       constants and functions of PMS initialization subcomponent.
*
***********************************************************************/
#ifndef PMS_INITIALIZATION_H
#define PMS_INITIALIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

#define PMS_INIT_TASK_STACK_SIZE    CFG_NU_OS_SVCS_PWR_CORE_INIT_TASK_STACK_SIZE
#define PMS_INIT_TASK_PRIORITY      0
#define PMS_INIT_TASK_TIME_SLICE    10

typedef struct PM_INIT_CB
{
    NU_MEMORY_POOL *pm_mem_pool;
    VOID          (*pm_task_entry)(NU_MEMORY_POOL *);
    VOID           *pm_stack;
    NU_TASK        *pm_task;
} PM_INIT;

VOID NU_PM_Initialize(NU_MEMORY_POOL* mem_pool);
STATUS PMS_Initialization_Status_Check(VOID);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_INITIALIZATION_H */


