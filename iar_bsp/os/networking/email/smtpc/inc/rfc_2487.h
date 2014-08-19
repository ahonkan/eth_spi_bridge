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
*       rfc_2487.h
*
* COMPONENT
*
*       SMTP Client - RFC-2487.
*
* DESCRIPTION
*
*       This file contains function prototypes used in RFC-2487
*       Implementation.
*
* DATA STRUCTURES
*
*       NONE
*
* DEPENDENCIES
*
*       smtp_client_api.h
*
*************************************************************************/
#ifndef _RFC_2487_H
#define _RFC_2487_H

#include "networking/smtp_client_api.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* RFC-2487 states. */
#define SMTP_TLS_NOT_DONE  0
#define SMTP_TLS_DONE      1

/* Function prototypes. */
INT16 SMTP_TLS_Is_Enabled(SMTP_SESSION *session);
STATUS SMTP_TLS_Setting(SMTP_SESSION *session, INT16 setting);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _RFC_2487_H */
