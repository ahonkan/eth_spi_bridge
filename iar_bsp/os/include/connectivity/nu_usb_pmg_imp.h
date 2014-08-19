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
************************************************************************/

/************************************************************************ 
* 
* 	FILE NAME                                                  
*
*      nu_usb_pmg_imp.h                                           
* 
* 	COMPONENT 
*
*      Nucleus USB Software 
* 
* 	DESCRIPTION 
*                                                                      
*        This file contains the internal data strcuture definitions.
* 
* 	DATA STRUCTURES 
*
*      None.
* 
* 	FUNCTIONS 
* 
*      None
*
* 	DEPENDENCIES 
*
*      nu_usb_pmg_imp.h       Power Manager Internal Definitions.
* 
*************************************************************************/
/* ==================================================================== */
#ifndef _NU_USB_PMG_IMP_H_
#define _NU_USB_PMG_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */

/* ========= Only valid if stack is configured for USB 3.0 ===========  */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/* Link State Definitions. */
#define USB_LINK_STATE_U0           0   /* Representing link state
                                           U0.                          */
#define USB_LINK_STATE_U1           1   /* Representing link state
                                           U1.                          */
#define USB_LINK_STATE_U2           2   /* Representing link state
                                           U2.                          */
#define USB_LINK_STATE_U3           3   /* Representing link state
                                           U3.                          */
#define USB_LINK_STATE_SSDISABLE    4   /* Representing link state
                                           SS.Disable.                  */
#define USB_LINK_STATE_RXDETECT     5   /* Representing link state
                                           RX.Detect.                   */
#define USB_LINK_STATE_SSINACTIVE   6   /* Representing link state
                                           SS.Inactive.                 */
#define USB_LINK_STATE_POLLING      7   /* Representing link state
                                           Polling.                     */
#define USB_LINK_STATE_RECOVERY     8   /* Representing link state
                                           Recovery.                    */
#define USB_LINK_STATE_HOT_RESET    9   /* Representing link is in
                                           Hot Reset state.             */
#define USB_LINK_STATE_COMPLIANCE   10  /* Representing link is in
                                           Compliance Mode state.       */
#define USB_LINK_STATE_LOOPBACK     11  /* Representing link state
                                           loopback.                    */
#define USB_LINK_STATE_INVLD        12  /* Representing Invalid link
                                           state.                       */

/* Power Mode definititons. */
#define USB_POWER_SAVING_MODE0      0   /* Default. Normal operation.   */
#define USB_POWER_SAVING_MODE1      1   /* Power saving level 1.        */
#define USB_POWER_SAVING_MODE2      2   /* Power saving level 2.        */
#define USB_POWER_SAVING_MODE3      3   /* Power saving level 3.        */

/*Worst case U1SEL and U2SEL definitions. */
#define USB_PMG_MAX_U1_LATENCY      145  /* Worst case U1SEL in us       */
#define USB_PMG_MAX_U2_LATENCY      3100 /* Worst case U2SEL in us       */

/* SEL parameters. */
#define USB_PMG_HUB_ERDY_DELAY      2100  /* Maximum packet size delay
                                             by hub 2100 ns            */
#define USB_PMG_HUB_ADD_ERDY_DELAY  250   /* Additional delay by hub in
                                               forwarding the ERDY 250 ns*/
#define USB_PMG_SCALING_FACTOR      1000  /* Scaling factor 1000.      */

#define USB_PMG_MAX_T3_LATENCY      0x03  /* Maximum t3 latency 3 us   */

#define tHubPort2PortExitLat        0x01  /* Hub port2port exit latency */

/* Macro for calculating the (t2+t3+t4) delay component of system exit
 * latency.
 */
#define USB_PMG_CALC_SEL_PARAMETER(hub_count) \
      ((((USB_PMG_HUB_ERDY_DELAY + USB_PMG_HUB_ADD_ERDY_DELAY*(hub_count) \
      - USB_PMG_HUB_ADD_ERDY_DELAY )/USB_PMG_SCALING_FACTOR)+1) \
      + USB_PMG_MAX_T3_LATENCY)
#define USB_PMG_U2_TRANSITION_ENABLE          NU_FALSE

typedef struct nu_usb_device_sel
{
    /* Time in us for U1 system exit latency. */
    UINT8 u1_sel; 

    /* Time in us for U1 device to host exit latency. */
    UINT8 u1_pel;
    
    /* Time in us for U2 system exit latency. */
    UINT16 u2_sel;
    
    /* Time in us for U2 device to host exit latency. */
    UINT16 u2_pel;
    
    /* Padding for alignment on 4 byte boundary. */
    UINT8 pad[2];
}NU_USB_DEVICE_SEL;

typedef struct _nu_usb_intf_pw_attrib
{
    /* Field showing whether function has the capability
       of remote wakeup. */
    UINT8   function_rw_capable;
    /* Field showing whether function is enabled for remote wakeup 
       or not. SET FEATURE is used to enable/disable remote wakeup.*/
    BOOLEAN function_rw_state;
    /* Field showing whether function is in suspend or normal mode.*/
    BOOLEAN function_suspend_state;
    /* Padding to align structure on 4 byte boundary.*/
    UINT8   pad[1];
}NU_USB_INTF_PW_ATTRIB;
 
typedef struct 
{
    /* Field showing whether device is self-powered or not. */
    BOOLEAN self_powered;
    /* Remote wakeup capability. Must be 0 for SS devices. */
    BOOLEAN remote_wakeup;
    /* Field showing whether device is enabled to initiate U1 entry or 
       not. Host software can set/clear this field using SET/CLEAR Feature 
       request. */
    BOOLEAN u1_enable;
    /* Field showing whether device is enabled to initiate U2 entry or 
       not. Host software can set/clear this field using SET/CLEAR Feature 
       request. */
    BOOLEAN u2_enable;
    /* Field showing whether device is enabled to generate Latency 
       Tolerant Messaging. */
    BOOLEAN ltm_enable;
   	/* The System Exit Latency will be saved in this field.*/
	NU_USB_DEVICE_SEL sel;
    /* Padding for alignment on 4 byte boundary. */
    UINT8   pad[3];
}NU_USB_DEV_PW_ATTRIB;

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ====================  Function Prototypes ========================== */

/* ==================================================================== */

#endif /* _NU_USB_PMG_IMP_H_ */

/* ======================  End Of File  =============================== */

