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

*************************************************************************
*
*    FILE NAME
*
*        nu_usb_bos_ext.h
*
*    COMPONENT
*
*        Nucleus USB Base Stack
*
*    DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_BOS.
*                                                                 
*    DATA STRUCTURES    
*
*        None
*                                                                      
*    FUNCTIONS      
*
*        None                         
*                                                                      
*    DEPENDENCIES  
*
*        None                         
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_BOS_EXT_H
#define _NU_USB_BOS_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ==============  USB Include Files =================================  */

/* ========= Only valid if stack is configured for USB 3.0 ===========  */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/* =======================  #defines =================================  */
#define LPM_MASK        0x02
#define LTM_MASK        0x02
#define LS_MASK         0x01
#define FS_MASK         0x02
#define HS_MASK         0x04
#define SS_MASK         0x08

/* ================== Function Prototypes  =========================== */

STATUS NU_USB_BOS_Get_Num_DevCap       (NU_USB_BOS *cb,
                                        UINT8  *num_devcap_out);

STATUS NU_USB_BOS_Get_Total_Length      (NU_USB_BOS *cb,
                                        UINT16  *totallength_out);
                                    
STATUS NU_USB_DEVCAP_USB2Ext_Get_LPM    (NU_USB_BOS *cb,
                                        BOOLEAN *lpm_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_LTM    (NU_USB_BOS *cb,
                                        BOOLEAN *ltm_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_LS     (NU_USB_BOS *cb,
                                        BOOLEAN *ls_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_FS     (NU_USB_BOS *cb,
                                        BOOLEAN *fs_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_HS     (NU_USB_BOS *cb,
                                        BOOLEAN *hs_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_SS     (NU_USB_BOS *cb,
                                        BOOLEAN *ss_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_Functionality (NU_USB_BOS *cb,
                                        UINT8 *func_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat (NU_USB_BOS *cb,
                                        UINT8 *u1exitlat_out);

STATUS NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat (NU_USB_BOS *cb,
                                        UINT16 *u2exitlat_out);

STATUS NU_USB_DEVCAP_CntnrID_Get_CID    (NU_USB_BOS *cb,
                                         UINT8 **containerid_out);

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ==================================================================== */

#endif /* _NU_USB_BOS_EXT_H_ */

/* ======================  End Of File  =============================== */
