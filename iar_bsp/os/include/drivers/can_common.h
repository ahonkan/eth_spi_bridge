/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       can_common.h
*
* COMPONENT
*
*       CAN Driver - Nucleus CAN Driver integration file
*
* DESCRIPTION
*
*       This file contains the integration code for integrating Nucleus
*       CAN Driver with Nucleus Device Manager.
*
*************************************************************************/

/* Check to avoid multiple file inclusion. */
#ifndef     CAN_COMMON_H
#define     CAN_COMMON_H

STATUS CAN_Get_Registry(const CHAR * key, CAN_INSTANCE_HANDLE *inst_handle);

#endif      /* !CAN_COMMON_H */
