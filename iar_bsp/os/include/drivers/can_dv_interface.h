/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       can_dv_interface.h
*
* COMPONENT
*
*       CAN Driver - Nucleus CAN Driver integration file
*
* DESCRIPTION
*
*       This file contains the integration code for integrating Nucleus
*       CAN Driver with Nucleus Device Manager.
*
*************************************************************************/

/* Check to avoid multiple file inclusion. */
#ifndef     CAN_DV_INTERFACE_H
#define     CAN_DV_INTERFACE_H

/* Power management support. Undef following if PM is not supported. */
#define CAN_MAX_LABEL_CNT              3
#define CAN_TOTAL_POWER_STATE_COUNT    2

/* Power States. */
#define CAN_OFF                        0
#define CAN_ON                         1

#define CAN_OPEN_MODE                  0x01

/* Minimum DVFS OP for CAN to perform correctly. */
#define CAN_MIN_DVFS_OP                1

#define NU_CAN_POWER_BASE  (NU_CAN_IOCTL_BASE + TOTAL_NU_CAN_IOCTLS)

#define CAN_HISR_STK_SIZE           (NU_MIN_STACK_SIZE * 2)
#define CAN_HANDLER_PRIORITY        0

/*================ Data structures ===================================== */

/* This structure serves as a instance handle for CAN hardware controller
 * driver.
 */
typedef struct _can_instance_handle
{
    DV_DEV_ID               device_id;
    BOOLEAN                 device_in_use;
    CHAR                    name[10];
    UINT32                  base_address;
    INT                     irq;
    INT                     irq_priority;
    ESAL_GE_INT_TRIG_TYPE   irq_type;
    UINT32                  baud_rate;
    DV_DEV_ID               dev_id;
    VOID                    *can_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev;
    
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    CHAR                    ref_clock[NU_DRVR_REF_CLOCK_LEN];
    NU_HISR                 can_hisr;
    VOID                    *can_hisr_stk_ptr;
    
    /* Setup and Cleanup function pointers. */
    VOID                    (*cleanup_func)(VOID);
    VOID                    (*setup_func)(VOID);

}CAN_INSTANCE_HANDLE;

/* This structure serves as a session handle for CAN hardware controller
 * driver.
 */
typedef struct _can_session_handle
{
    CAN_INSTANCE_HANDLE    *instance_handle;
    UINT32                  open_modes;

}CAN_SESSION_HANDLE;

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS CAN_Dv_Register (const CHAR *key, CAN_INSTANCE_HANDLE *inst_handle);
STATUS CAN_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);

#endif      /* !CAN_DV_INTERFACE_H */
