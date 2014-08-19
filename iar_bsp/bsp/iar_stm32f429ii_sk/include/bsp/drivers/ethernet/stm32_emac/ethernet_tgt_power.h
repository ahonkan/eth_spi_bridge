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
*       ethernet_tgt_power.h
*
*   COMPONENT
*
*       Ethernet                           - Ethernet controller driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the Ethernet hardware
*
*************************************************************************/
#ifndef ETHERNET_TGT_PWR_H
#define ETHERNET_TGT_PWR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
VOID        Ethernet_Tgt_Pwr_Default_State (VOID *inst_handle);
STATUS      Ethernet_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !ETHERNET_TGT_PWR_H */
