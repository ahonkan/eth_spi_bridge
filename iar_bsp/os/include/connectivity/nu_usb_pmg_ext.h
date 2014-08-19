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
*      nu_usb_pmg_ext.h
* 
* 	COMPONENT 
*
*      Nucleus USB Software 
* 
* 	DESCRIPTION 
*
*        This file contains the exported function names and data structures
*        for the power manager.
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
#ifndef _NU_USB_PMG_EXT_H_
#define _NU_USB_PMG_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_pmg_imp.h"

/* ========= Only valid if stack is configured for USB 3.0 ===========  */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/* ====================  Function Prototypes ========================== */

/* NU_USB_PMG  API. */

STATUS NU_USB_PMG_Update_Dev_Pwr_Src ( NU_USB_DEVICE    *cb,      
                                       BOOLEAN          self_powered);

STATUS NU_USB_PMG_Update_Dev_U1_Enable ( NU_USB_DEVICE   *cb,      
                                         BOOLEAN         u1_enable);

STATUS NU_USB_PMG_Update_Dev_U2_Enable ( NU_USB_DEVICE   *cb,
                                         BOOLEAN         u2_enable);

STATUS NU_USB_PMG_Update_Dev_LTM_Enable ( NU_USB_DEVICE   *cb,
                                          BOOLEAN         ltm_enable);

STATUS NU_USB_PMG_Get_Dev_Pwr_Attrib (  NU_USB_DEVICE        *cb,
                                        NU_USB_DEV_PW_ATTRIB **pwr_attrib);

STATUS NU_USB_PMG_Set_Dev_Pwr_Attrib (  NU_USB_DEVICE        *cb,
                                        NU_USB_DEV_PW_ATTRIB *pwr_attrib);

STATUS NU_USB_PMG_Update_Intf_Pwr_Attrib ( NU_USB_DEVICE   *cb,
                                           UINT8           intf_number,
                                    NU_USB_INTF_PW_ATTRIB  *pwr_attrib);

STATUS NU_USB_PMG_Set_Link_State (  NU_USB_DEVICE   *device,
                                    UINT8           link_state  );

STATUS NU_USB_PMG_Calculate_SEL (  NU_USB_DEVICE     *device,
                                   NU_USB_DEVICE_SEL *sel  );
                                   
STATUS NU_USB_PMG_Calculate_BELT_Value(NU_USB_DEVICE *device,
                                       UINT16        *belt_value,
                                       UINT8          new_link_state);

STATUS NU_USB_PMG_Generate_LTM(NU_USB_DEVICE *device,
                               UINT16        new_link_state);

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) .*/

/* ==================================================================== */

#endif /* _NU_USB_PMG_EXT_H_ */

/* ======================  End Of File  =============================== */

