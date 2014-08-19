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
*       pfile_error.h                
*
* COMPONENT
*
*       Nucleus POSIX - file system
*
* DESCRIPTION
*
*       Contains the internal routines for error conversion 
*       on Devices, and Nucleus FILE into
*       Nucleus POSIX error.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef __PFILE_ERROR_H_
#define __PFILE_ERROR_H_


#ifdef __cplusplus
extern "C" {
#endif

int     cnvt_posix_error(int error_no);
int     pfile_err_check(int error_no);
VOID    pfile_open_err(INT sts);
VOID    pfile_close_err(STATUS sts);
VOID    pfile_read_err(INT32 sts);
VOID    pfile_write_err(INT32 sts);
VOID    pfile_lseek_err(INT32 sts);

#ifdef __cplusplus
}
#endif

#endif /*  __PFILE_ERROR_H_  */
