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
*  coords.h                                                     
*
* DESCRIPTION
*
*  This file contains prototypes and externs for coords.c
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
#ifndef _COORDS_H_
#define _COORDS_H_

/* Local Functions */
VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
VOID G2UP(INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY);
VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);
VOID G2UR(rect UserRect, rect *RtnRect);
VOID V2GSIZE(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY);

extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);

/* Local Functions */
VOID COORDS_rsGblPenCoord(VOID);
VOID COORDS_rsGblClip( rsPort *inpClpPort, rect *outClipR);
VOID COORDS_rsVirtCoord( rsPort *inpPort);
VOID COORDS_rsGblCoord(VOID);
VOID COORDS_rsV2GP( INT32 *virtX, INT32 *virtY);
VOID COORDS_rsV2GR( rect *virRect);
VOID COORDS_rsG2VP( INT32 *virtX, INT32 *virtY);
VOID COORDS_rsG2VR( rect *virRect);
VOID Gbl2LclPt(point *argPT);
VOID Gbl2LclRect(rect *argR);
VOID Gbl2VirPt(point *argPT);
VOID Gbl2VirRect(rect *argR);
VOID Lcl2GblPt(point *argPT);
VOID Lcl2GblRect(rect *argR);
VOID Lcl2VirPt(point *argPT);
VOID Lcl2VirRect(rect *argR);
VOID Vir2GblPt(point *argPT);
VOID Vir2GblRect(rect *argR);
VOID Vir2LclPt(point *argPT);
VOID Vir2LclRect(rect *argR);
VOID Port2Gbl(rect *inpRect, rsPort *inpPort, rect *dstRect);

#endif /* _COORDS_H_ */





