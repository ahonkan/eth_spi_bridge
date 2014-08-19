/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
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
*       sshsrv_api.h
*
* COMPONENT
*
*       SSH Server - API
*
* DESCRIPTION
*
*       This file includes the decelerations of all the Public APIs
*       and all the header files needed.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*
*************************************************************************/
#ifndef SSHSRV_API_H
#define SSHSRV_API_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

STATUS NUSSH_Start(VOID);
STATUS NUSSH_Stop(VOID);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* SSHSRV_API_H */
