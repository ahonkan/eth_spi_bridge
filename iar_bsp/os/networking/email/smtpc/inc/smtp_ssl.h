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
*       smtp_ssl.h
*
* COMPONENT
*
*       SMTP Client - SSL Support.
*
* DESCRIPTION
*
*       This file contains function prototypes used in SMTP SSL component.
*
* DATA STRUCTURES
*
*       NONE
*
* DEPENDENCIES
*
*       ssl.h
*
*************************************************************************/

#ifndef _SMTP_SSL_H
#define _SMTP_SSL_H

#include "openssl/ssl.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* OpenSSL functions that will be used in SMTP Client API.  */
STATUS SMTP_SSL_Init(SSL_CTX **ssl_ctx);
STATUS SMTP_SSLD_Connect(SSL **new_ssl, INT socketd, SSL_CTX **ssl_ctx);
DH *SMTP_Get_Dh512(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_SSL_H */
