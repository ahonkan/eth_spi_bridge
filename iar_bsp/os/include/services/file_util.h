/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME               
*
*       file_util.h          
*
* COMPONENT
*
*       Nucleus POSIX - file system
*
* DESCRIPTION
*
*       This file contains the various internal utility routines used by
*       POSIX file system.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef __FILE_UTIL_H_
#define __FILE_UTIL_H_

typedef struct tmdate_struct
{
    unsigned  time;
    unsigned  date;
}tmdate;


#ifdef __cplusplus
extern "C" {
#endif

int pfile_isdir(unsigned char attr);
unsigned long tm2secs(tmdate *tp);

#ifdef __cplusplus
}
#endif

#endif  /*  __FILE_UTIL_H_  */




