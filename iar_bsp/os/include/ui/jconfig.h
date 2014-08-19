/******************************************************************************
*                                                                             
*              Copyright Mentor Graphics Corporation 2006                     
*                        All Rights Reserved.                                 
*                                                                             
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS       
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS        
*  SUBJECT TO LICENSE TERMS.                                                  
*                                                                             
*                                                                             
*******************************************************************************
*******************************************************************************
* Copyright (C) 1991-1997, Thomas G. Lane.
* This file is part of the Independent JPEG Group's software.
* For conditions of distribution and use, see the accompanying README file.
* The version of source is at version 6b.
*******************************************************************************
*******************************************************************************
*
* FILE NAME                                                       
*
*  jconfig.h                                                    
*
* DESCRIPTION
*
*  jconfig.h for Microsoft Visual C++ on Windows 95 or NT
*
******************************************************************************/
/* see jconfig.doc for explanations */

#ifndef JCONFIG_H
#define JCONFIG_H

#include "ui/img.h"

#ifdef IMG_INCLUDED

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT

#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */


#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#undef BMP_SUPPORTED		/* BMP image file format */
#undef GIF_SUPPORTED		/* GIF image file format */
#undef PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#undef TARGA_SUPPORTED		/* Targa image file format */

#undef TWO_FILE_COMMANDLINE	/* optional */
#undef USE_SETMODE		/* Microsoft has setmode() */
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */

#endif /* IMG_INCLUDED */
#endif
