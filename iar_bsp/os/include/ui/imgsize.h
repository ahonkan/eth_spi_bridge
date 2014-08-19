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
*  imgsize.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for imgsize.c
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
#ifndef _IMGSIZE_H_
#define _IMGSIZE_H_

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)

extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);

/* Local Functions */
SIGNED ImageSize( rect *sR);
SIGNED ImagePara( rect * argPara);

#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
#endif /* _IMGSIZE_H_ */






