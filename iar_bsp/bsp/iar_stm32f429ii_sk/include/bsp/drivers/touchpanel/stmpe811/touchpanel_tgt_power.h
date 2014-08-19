/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       touchpanel_tgt_power.h
*
*   COMPONENT
*
*       STMPE811 - STMPE811 touch panel controller driver
*
*   DESCRIPTION
*
*       This file contains defines, data structures and function prototypes
*       for STMPE811 touch panel controller driver.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef STMPE811_TGT_PWR_H
#define STMPE811_TGT_PWR_H

STATUS Touchpanel_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state);
STATUS Touchpanel_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
STATUS Touchpanel_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
STATUS Touchpanel_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
STATUS Touchpanel_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);

#endif /* STMPE811_TGT_PWR_H */
