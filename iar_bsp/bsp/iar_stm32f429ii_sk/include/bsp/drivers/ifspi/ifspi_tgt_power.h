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
*       ifspi_tgt_power.h
*
*   COMPONENT
*
*       Ethernet                           - Ethernet  controller driver 
*       LWSPI                              - Light weight SPI controller driver 
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the interface power management
*
*************************************************************************/
#ifndef IFSPI_TGT_PWR_H
#define IFSPI_TGT_PWR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
VOID        IF_SPI_Tgt_Pwr_Default_State (VOID *inst_handle);
STATUS      IF_SPI_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !IFSPI_TGT_PWR_H */
