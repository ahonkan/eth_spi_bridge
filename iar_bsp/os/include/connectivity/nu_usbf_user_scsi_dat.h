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
*       nu_usbf_user_scsi_dat.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the shared data declarations for the
*       Mass storage SCSI Media container driver.
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
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

#ifndef _NU_USBF_USER_SCSI_DAT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_SCSI_DAT_H

/* ======================  Global data  ================================ */

extern const NU_USBF_USER_SCSI_DISPATCH usbf_user_scsi_dispatch;
extern NU_USBF_USER_SCSI    *NU_USBF_USER_SCSI_Cb_Pt;
extern NU_USBF_USER_SCSI_CMD
       nu_usbf_user_scsi_command_table[USBF_SCSI_MAX_COMMANDS];

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_SCSI_DAT_H */

/* =======================  End Of File  =============================== */
