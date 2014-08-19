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
*  portops.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for portops.c
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
#ifndef _PORTOPS_H_
#define _PORTOPS_H_

extern SIGNED  FontFile_BaseAddress;
extern grafMap *ATgrafMap;

extern VOID COORDS_rsV2GP( INT32 *virtX, INT32 *virtY);
extern VOID COORDS_rsV2GR( rect *virRect);
extern VOID COORDS_rsG2VP( INT32 *virtX, INT32 *virtY);
extern VOID COORDS_rsG2VR( rect *virRect);
extern VOID COORDS_rsGblCoord(VOID);
extern VOID COORDS_rsGblPenCoord(VOID);
extern VOID PenMode( INT32 Mode);
extern VOID COORDS_rsGblClip( rsPort *inpClpPort, rect *outClipR);
extern VOID SetFont( fontRcd *FONT);

/* Local Functions */
VOID PORTOPS_rsDefaultPenValues( rsPort *penPort);
VOID InitPort( rsPort *argPORT);
VOID PlaneMask( SIGNED PLNMSK);
VOID GetPort( rsPort **gpPORT);
VOID PortBitmap( grafMap *ptrBMAP);
VOID PortSize( INT32 psWDX,  INT32 psHTY);
VOID PortPattern( patList *argPLIST);
VOID PortOrigin( INT32 poTB);
VOID SetOrigin( INT32 SOX, INT32 SOY);
VOID MovePortTo( INT32 gblLEFT, INT32 gblTOP);
VOID SetPort( rsPort *portPtr);
VOID SetLocal(VOID);
VOID SetVirtual(VOID);


#endif /* _PORTOPS_H_ */




