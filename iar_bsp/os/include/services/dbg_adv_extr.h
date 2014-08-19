/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       dbg_adv_extr.h
*
*   COMPONENT
*
*       Debug Agent - Advertising - External interface
*
*   DESCRIPTION
*
*       This file contains the C source code declarations for the
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DBG_ADV_Send_To_Queue
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_ADV_EXTR_H
#define DBG_ADV_EXTR_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global functions */

STATUS DBG_ADV_Send_To_Queue(UINT8 *, UINT8);

#ifdef __cplusplus
}
#endif

#endif /* DBG_ADV_EXTR_H */
