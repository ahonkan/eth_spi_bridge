/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
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
*       error_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Error
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for error reporting 
*       services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/error_defs.h"

#ifndef ERROR_EXTR_H
#define ERROR_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


#if (CFG_NU_OS_STOR_FILE_VFS_ERROR_SUPPORT == 1)
VOID pc_report_error(INT error_number);
#else
#define pc_report_error(x)
#endif


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* ERROR_EXTR_H */
