/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       serial_tgt_power.h
*
*   COMPONENT
*
*       SMT32_USART                           - SMT32_USART controller driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the UART hardware
*
*************************************************************************/
#ifndef SERIAL_TGT_PWR_H
#define SERIAL_TGT_PWR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

VOID      Serial_Tgt_Pwr_Default_State (SERIAL_INSTANCE_HANDLE *inst_handle);
STATUS    Serial_Tgt_Pwr_Set_State (VOID *inst_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

STATUS    Serial_Tgt_Pwr_Min_OP_Pt_Calc (SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate,
                                         UINT8* min_op_pt);
STATUS    Serial_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
STATUS    Serial_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
STATUS    Serial_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
STATUS    Serial_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
STATUS    Serial_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* !SERIAL_TGT_PWR_H */
