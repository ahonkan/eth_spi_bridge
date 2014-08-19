/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  imagerw.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for imagerw.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _IMAGERW_H_
#define _IMAGERW_H_

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)

extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);

/* Local Functions */
VOID ReadImage(rect *argR, image *argImage);
VOID WriteImage(rect * dstR, image * srcImage);

#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
#endif /* _IMAGERW_H_ */






