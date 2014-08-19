/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       rtc.h
*
*   COMPONENT
*
*       RTC                          - RTC Middleware
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the RTC Middleware Driver module.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef RTC_H
#define RTC_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*******************/
/* External API    */
/*******************/
STATUS  NU_Set_RTC_Time(struct tm *new_time_ptr);
STATUS  NU_Retrieve_RTC_Time(struct tm *cur_time_ptr);
VOID    NU_Close_RTC(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !RTC_H */
