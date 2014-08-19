/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   nu_usbf_dfu_dat.h
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver
*
* DESCRIPTION
*
*   This file contains definitions for dispatch table of DFU class driver.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*   None.
*
* DEPENDENCIES
*
*   None.
*
**************************************************************************/
#ifndef _NU_USBF_DFU_DAT_H_

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_DFU_DAT_H_

/*======================= extern declarations ==========================*/
extern NU_USBF_DFU *NU_USBF_DFU_Cb_Pt;
extern const NU_USBF_DFU_DISPATCH DFUF_Usbf_Dispatch;

extern const DFU_RQST_HANDLER
                        DFUF_Rqst_Handlers[NUM_DFU_STATE_HANDLERS];

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif /* _NU_USBF_DFU_DAT_H_ */ 

/*============================  End Of File  ===========================*/
