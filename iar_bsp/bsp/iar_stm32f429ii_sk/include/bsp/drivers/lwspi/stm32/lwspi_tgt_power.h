/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       lwspi_tgt_power.h
*
* COMPONENT
*
*       Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains power services related target specific
*       constants / defines / function headers for the SPI hardware.
*
**************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     LWSPI_TGT_POWER_H
#define     LWSPI_TGT_POWER_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Minimum DVFS OP for SPI to perform operations correctly. */
#define     SPI_MIN_DVFS_OP             1

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

STATUS      LWSPI_TGT_Pwr_Default_State(LWSPI_INSTANCE_HANDLE*);
STATUS      LWSPI_TGT_Pwr_Set_State(VOID*, PM_STATE_ID*);
STATUS      LWSPI_TGT_Pwr_Get_Clock_Rate(LWSPI_INSTANCE_HANDLE*, UINT32*);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

STATUS      LWSPI_TGT_Pwr_Pre_Park(VOID*);
STATUS      LWSPI_TGT_Pwr_Post_Park(VOID*);
STATUS      LWSPI_TGT_Pwr_Pre_Resume(VOID*);
STATUS      LWSPI_TGT_Pwr_Post_Resume(VOID*);
STATUS      LWSPI_TGT_Pwr_Resume_End(VOID*);

#endif

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* !LWSPI_TGT_POWER_H */
