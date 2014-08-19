/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_user_scsi_cfg.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the configuration macros for the Mass storage
*       SCSI media container driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
**************************************************************************/

#ifndef _NU_USBF_USER_SCSI_CFG_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_SCSI_CFG_H

/* ======================  #defines  =================================== */
#define NU_USBF_MS_FILE                      NU_FALSE
#define NU_USBF_USER_SCSI_NUM_DISKS          1
#define NU_USBF_MS_TASK_STACK_SIZE           4096*100*4
#define NU_USBF_MS_TASK_PRIORITY             0x20
#define NU_USBF_MS_TASK_PREEMPTION           NU_PREEMPT

/* To use mass storage function in task mode, just make a define
 * NU_USBF_MS_TASK_MODE '#define NU_USBF_MS_TASK_MODE'. Do not declare this
 * define if mass storage command processing needs to be done in HISR mode.
 */

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_SCSI_CFG_H */

/* =======================  End Of File  =============================== */
