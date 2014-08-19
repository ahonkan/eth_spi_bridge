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
*       constants of PMS error management subcomponent.
*
***********************************************************************/
#ifndef PMS_ERROR_MANAGEMENT_H
#define PMS_ERROR_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Max number of messages that can be handled
   simultaneously by the error system */
#define PM_ERROR_MAX_MESSAGES   3
#define PM_ERROR_QUEUE_AREA_SIZE (sizeof(PM_ERROR) * PM_ERROR_MAX_MESSAGES)

/* Used to verify a valid error control block */
#define PM_ERROR_ID             0x504D4552UL

typedef struct PM_ERROR_CB
{
    UINT32     pm_error_id;                 /* Error ID */
    STATUS     pm_status;                   /* Error status code */
    VOID      *pm_thread;                   /* Thread pointer */
    VOID      *pm_info;                     /* Extra information pointer */
    UINT32     pm_length;                   /* Length of the information */
} PM_ERROR;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_ERROR_MANAGEMENT_H */


