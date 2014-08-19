/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
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
*       display_tgt_power.h
*
*   COMPONENT
*
*       Display                           - LTDC controller driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the LCD hardware
*
*************************************************************************/
#ifndef DISPLAY_TGT_PWR_H
#define DISPLAY_TGT_PWR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
STATUS   Display_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
STATUS   Display_Tgt_Min_OP_Pt_Calc (UINT8* min_op_pt);
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS */
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !DISPLAY_TGT_PWR_H */
