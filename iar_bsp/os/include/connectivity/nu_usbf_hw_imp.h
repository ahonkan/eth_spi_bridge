/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_hw_imp.h 
*
* COMPONENT
*
*       Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the common
*       controller driver component.
*
* DATA STRUCTURES
*
*       NU_USBF_HW                          Stack hardware controller
*                                           control block.
*       NU_USBF_HW_DISPATCH                 Stack hardware controller
*                                           dispatch table.
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

#ifndef _NU_USBF_HW_IMP_H
#define _NU_USBF_HW_IMP_H

/* ====================  Data Types ===================================  */

typedef struct nu_usbf_stack NU_USBF_STACK;

typedef struct nu_usbf_hw
{
    NU_USB_HW cb;
    /* Automatic Hardware processing?   */
    UINT32 capability;
    /* Remote wakeup enabled ?           */
    UINT8 remote_wakeup;
    /* Test mode */
    UINT8 test_mode;
}

NU_USBF_HW;

typedef struct nu_usbf_hw_dispatch
{
    NU_USB_HW_DISPATCH dispatch;
}
NU_USBF_HW_DISPATCH;


/* GUID definition for USB Function Hardware. */
#define USBFHW_LABEL          {0x31,0x56,0x08,0x5f,0x50,0xf6,0x4b,0xa6,0x9d,0xd1,0x65,0x36,0xd7,0xf7,0x9d,0xdb}


/* IOCTL commands definitions for use with Nucleus USB Function 
 * Stack. 
 */
#define NU_USBF_IOCTL_BASE                  (TOTAL_USB_IOCTLS + 1)
#define NU_USBF_IOCTL_GET_CAPABILITY        (NU_USBF_IOCTL_BASE + 0)
#define NU_USBF_IOCTL_SET_ADDRESS           (NU_USBF_IOCTL_BASE + 1)
#define NU_USBF_IOCTL_GET_DEV_STATUS        (NU_USBF_IOCTL_BASE + 2)
#define NU_USBF_IOCTL_GET_ENDP_STATUS       (NU_USBF_IOCTL_BASE + 3)
#define NU_USBF_IOCTL_STALL_ENDP            (NU_USBF_IOCTL_BASE + 4)
#define NU_USBF_IOCTL_UNSTALL_ENDP          (NU_USBF_IOCTL_BASE + 5)
#define NU_USBF_IOCTL_START_HNP             (NU_USBF_IOCTL_BASE + 6)
#define NU_USBF_IOCTL_ACQUIRE_ENDP          (NU_USBF_IOCTL_BASE + 7)
#define NU_USBF_IOCTL_RELEASE_ENDP          (NU_USBF_IOCTL_BASE + 8)
#define NU_USBF_IOCTL_ENABLE_PULLUP         (NU_USBF_IOCTL_BASE + 9)
#define NU_USBF_IOCTL_DISABLE_PULLUP        (NU_USBF_IOCTL_BASE + 10)
#define NU_USBF_IOCTL_GET_EP0_MAXP          (NU_USBF_IOCTL_BASE + 11)
#define NU_USBF_IOCTL_SEND_FUNCWAKENOTIF    (NU_USBF_IOCTL_BASE + 12)
#define NU_USBF_IOCTL_SET_FEATURE_U0_ENABLE (NU_USBF_IOCTL_BASE + 13)
#define NU_USBF_IOCTL_SET_FEATURE_U1_ENABLE (NU_USBF_IOCTL_BASE + 14)
#define NU_USBF_IOCTL_SET_LTM_ENABLE        (NU_USBF_IOCTL_BASE + 15)
#define NU_USBF_IOCTL_CHECK_LTM_CAPABLE		(NU_USBF_IOCTL_BASE + 16)
#define NU_USBF_IOCTL_GET_SUPPORTED_SPEEDS	(NU_USBF_IOCTL_BASE + 17)
#define NU_USBF_IOCTL_GET_U1DEVEXITLAT		(NU_USBF_IOCTL_BASE + 18)
#define NU_USBF_IOCTL_GET_U2DEVEXITLAT		(NU_USBF_IOCTL_BASE + 19)

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
#define NU_USBF_PWR_HIB_RESTORE             (NU_USBF_IOCTL_BASE + 20)
#define TOTAL_USBF_IOCTLS                   21
#else
#define TOTAL_USBF_IOCTLS                   20
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */

/* ====================  Function Prototypes =========================== */
VOID usbf_hisr( VOID );

VOID USBF_LISR (INT vector);

STATUS USBF_HW_Register_LISR (NU_USBF_HW * controller,
                              INT vector_no);

STATUS USBF_HW_Deregister_LISR (NU_USBF_HW * controller,
                                INT vector_no);

/* ===================================================================== */

/* ===================================================================== */

#endif      /* _NU_USBF_HW_IMP_H         */

/* =======================  End Of File  =============================== */
