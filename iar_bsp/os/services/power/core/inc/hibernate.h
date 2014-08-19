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
*       This file contains private data structure definitions and 
*       constants of PMS hibernate subcomponent.
*
***********************************************************************/
#ifndef PMS_HIBERNATE_H
#define PMS_HIBERNATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum Region Count - The maximum number of regions that can be
   stored in NVM. */
#define HB_MAX_REGION_COUNT         20

/* Hibernate Resume Stack Size - The size (in bytes) of the stack used
   during a hibernate shutdown resume operation. */
#define HB_RESUME_STACK_SIZE        CFG_NU_OS_SVCS_PWR_CORE_HIBERNATE_RESUME_TASK_STACK_SIZE

/* Hibernate Exit Function type */
typedef VOID (*HB_EXIT_FUNC)(VOID);

/* Hibernate Resume control block */
typedef struct HB_RESUME_CB_STRUCT
{
    BOOLEAN                 resume_pending;
    HB_EXIT_FUNC            hb_exit_func;
    VOID *                  restore_addr[HB_MAX_REGION_COUNT];

} HB_RESUME_CB;

VOID PMS_Hibernate_Enter(UINT8 hibernate_mode);
VOID PMS_Hibernate_Exit(VOID);
VOID PMS_Hibernate_Resume_Check(VOID);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_HIBERNATE_H */


