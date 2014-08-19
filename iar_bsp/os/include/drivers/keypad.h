/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       keypad.h
*
*   COMPONENT
*
*       Keypad Driver
*
*   DESCRIPTION
*
*       This file is the main API file for Nucleus Keypad Driver.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus_gen_cfg.h
*       rsconst.h
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     KEYPAD_H
#define     KEYPAD_H

#include "nucleus_gen_cfg.h"

/* Mapping FUSE generated defines to driver generic defines. */

/* Specify whether a task is required for keypad processing. */
#define		KP_TASK_REQUIRED		        CFG_NU_OS_DRVR_KEYPAD_TASK_REQUIRED

/* First delay after the key press for debouncing. */
#define     KP_INITIAL_INTERVAL             CFG_NU_OS_DRVR_KEYPAD_INITIAL_INTERVAL

/* Sampling interval for key release detection. */
#define     KP_SAMPLING_INTERVAL            CFG_NU_OS_DRVR_KEYPAD_SAMPLING_INTERVAL

/* Specify whether key up (release) interrupt functionality is available
   and can be used. Set to NU_TRUE to enable this. In some cases this
   functionality is available but results in problems because of noise. In
   that case this should be set to NU_FALSE. */
#define     KP_USE_RELEASE_INTERRUPT        CFG_NU_OS_DRVR_KEYPAD_USE_RELEASE_INTERRUPT


/* Structure for Keypad callback routines to be implemented by the user. */
typedef struct _keypad_callback_struct_
{
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
    PMI_DEV_HANDLE kp_pmi_dev;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */
    VOID         (*kp_enable_key_down_interrupts)(VOID);
    VOID         (*kp_disable_key_down_interrupts)(VOID);
    VOID         (*kp_enable_key_up_interrupts)(VOID);
    VOID         (*kp_disable_key_up_interrupts)(VOID);
    UINT16       (*kp_scan)(VOID);

} KEYPAD_CALLBACKS;


/* Nucleus Keypad Driver API services. */

INT32       KP_Device_Mgr(mouseRcd *rcd, INT16 md);
VOID        KP_Register_Callbacks(KEYPAD_CALLBACKS *kp_cb);

#define KEYPAD_LABEL	{0x04,0x29,0xbc,0x98,0xd0,0xd2,0x4b,0x39,0xbf,0x98,0x74,0x27,0x49,0xeb,0xe7,0x2c}

#endif      /* KEYPAD_H */

