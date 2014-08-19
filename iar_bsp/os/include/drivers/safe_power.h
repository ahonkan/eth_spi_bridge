/*************************************************************************/
/*                                                                       */
/*               Copyright 2011 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       safe_power.h
*
* COMPONENT
*
*       SAFE Disk Driver
*
* DESCRIPTION
*
*       Configuration and interface defines for SAFE device driver
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SAFE_Set_State
*
*************************************************************************/
#ifndef SAFE_POWER_H
#define SAFE_POWER_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

STATUS   SAFE_Set_State(VOID *inst_handle, PM_STATE_ID *state);

#endif


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* SAFE_H */

