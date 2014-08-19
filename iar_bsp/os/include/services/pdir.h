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
*       pdir.h
*
* COMPONENT
*
*       PFS - POSIX file system
*
* DESCRIPTION
*
*       This file contains definitions and data structure for Nucleus
*       POSIX directory.
*
* DATA STRUCTURES
*
*       PSX_OPEN_DIR
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef __PDIR_H_
#define __PDIR_H_

#ifndef DIR_DEFS_H
#include "storage/dir_defs.h"
#endif

/* Directory structure */
typedef struct psx_open_dir_struct
{
    DIR*             user_dir;      /* Internal ADF Resource ID */
    DSTAT            dir;           /* DSTAT Directory from FILE */
    unsigned         pos;           /* Directory positions */
    signed short     eod;           /* End of directory */
    signed short     pad;           /* padding */
    char*            pathname;      /* Directory pathname */
}PSX_OPEN_DIR;

#endif  /* __PDIR_H_   */
