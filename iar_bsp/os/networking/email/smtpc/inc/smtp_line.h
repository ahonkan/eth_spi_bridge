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
*       smtp_line.h
*
* COMPONENT
*
*       SMTP Client - Line Utility.
*
* DESCRIPTION
*
*       This file contains function prototypes used in SMTP line utility.
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
#ifndef _SMTP_LINE_H
#define _SMTP_LINE_H

#include "networking/smtp_client_api.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Function prototypes. */
STATUS SMTP_Init_Line_Obj(SMTP_LINE *line_obj, CHAR *data_ptr,
        CHAR *delimiter, UINT32 data_len, UINT32 maxline);
CHAR * SMTP_Get_Line(SMTP_LINE *line_obj);
STATUS SMTP_Find_Keyword(SMTP_LINE *line_obj, CHAR *keyword);
VOID SMTP_Reset_Line_Obj(SMTP_LINE *line_obj);
VOID SMTP_Delete_Line_Obj(SMTP_LINE *line_obj);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_LINE_H */
