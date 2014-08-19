/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************/

/************************************************************************
*
* FILE NAME
*
*       smtp_client_cfg.h
*
* COMPONENT
*
*       SMTP Client.
*
* DESCRIPTION
*
*       This file contains SMTP Client API configurations.
*
* DATA STRUCTURES
*
*       NONE
*
* DEPENDENCIES
*
*       NONE
*
*************************************************************************/
#ifndef _SMTP_CLIENT_CFG_H
#define _SMTP_CLIENT_CFG_H

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define SMTP_PRINT_RESPONSE     CFG_NU_OS_NET_EMAIL_SMTPC_PRINT_DEBUG

/* RFC-5321 4.5.3.1. */
#define SMTP_MAX_CMD_LINE       512
#define SMTP_MAX_TXT_LINE       1000

#define SMTP_TIMEOUT            10 * NU_TICKS_PER_SECOND

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_CLIENT_CFG_H */
