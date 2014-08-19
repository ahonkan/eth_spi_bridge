/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/***************************************************************************
*
* IMPORTANT :
*
* Beginning with version 4.3 of Nucleus NET. This file is obsolete and should
* no longer be used. This file and TCP_ERRS.C have been replaced by NERRS.H
* and NERRS.C. The only purpose of this file is to provide backwards
* compatibility for those applications, drivers, etc. that made use of
* the old TCP_ERRS.H and TCP_ERRS.C files.
*
*
****************************************************************************/

/**************************************************************************
*
* FILENAME
*
*       tcp_errs.h
*
* DESCRIPTION
*
*       This file is obsolete
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       target.h
*       nerrs.h
*
****************************************************************************/

#ifndef TCP_ERRS_H
#define TCP_ERRS_H

#include "networking/target.h"
#include "networking/nerrs.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define NU_Tcp_Log_Error(a,b,c,d)       NLOG_Error_Log("No error msg",b,c,d)

#define NU_Tcp_Clear_All_Errors         NLOG_Clear_All_Errors
#define NU_Tcp_Error_String             NERRS_Error_String



#define TCP_RECOVERABLE         NERR_RECOVERABLE
#define UDP_RECOVERABLE         NERR_RECOVERABLE
#define IP_RAW_RECOVERABLE      NERR_RECOVERABLE
#define TCP_TRIVIAL             NERR_INFORMATIONAL
#define TCP_SEVERE              NERR_SEVERE
#define TCP_FATAL               NERR_FATAL

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif  /* TCPERRS_H */
