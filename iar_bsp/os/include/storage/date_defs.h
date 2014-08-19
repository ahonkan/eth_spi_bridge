/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved                           */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       date_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Date and time
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains date and time defines 
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       DATESTR                     Generic date structure       
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef DATE_DEFS_H
#define DATE_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Date stamping buffer */
#define DSET_ACCESS         0           /* Set the only access-time,date */
#define DSET_UPDATE         1           /* Set the access-time,date and update-time,date */
#define DSET_CREATE         2           /* Set the all time and date.(access-time,date, update-time,date 
                                           create-time,date) */
#define DSET_MANUAL_UPDATE  3           /* Set none of the dates or times, all dates and times
                                           have been set manual and shouldn't be altered. */
typedef struct datestr
{
    UINT8       cmsec;              /* Centesimal mili second */
    UINT16      date;               /* Date */
    UINT16      time;               /* Time */

} DATESTR;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* DATE_DEFS_H */
