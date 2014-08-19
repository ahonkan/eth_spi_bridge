/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
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
*  copyb.h
*
* DESCRIPTION
*
*  This file contains prototypes and externs for copyb.c
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
#ifndef _COPYB_H_
#define _COPYB_H_

extern VOID Port2Gbl(rect *inpRect, rsPort *inpPort, rect *dstRect);
extern VOID COORDS_rsGblClip(rsPort *inpClpPort, rect *outClipR);

/* Local Functions */
STATUS CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR );
STATUS CopyBits( grafMap *srcBMAP, grafMap *dstBMAP, rect *srcRECT,
                 rect *dstCLIP, INT32 dstRASOP );

#endif /* _COPYB_H_ */





