/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*  screen_s.h
*
* DESCRIPTION
*
*  This file contains defines, externs, structures and prototypes for
*  screen support functions.
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
#ifndef _SCREEN_S_H_
#define _SCREEN_S_H_

extern NU_MEMORY_POOL  System_Memory;

/* Unknown device, not in table */
#define QRYDEVUNKN      2

#define PGALL           255
#define pageCur         mapTable[4]
#define pageMax         mapTable[5]
#define pageScans       mapTable[6]
#define numDev2Pg       1

/* Display Bank Parameter Structure */
typedef struct _DBPstruc{
    UINT8       wnType;        /* hardware window type             */
    signed char pad[3];        /* Padding for the structure */
    SIGNED      wnSize;        /* scan lines/bank                  */
    SIGNED      wnOff;         /* offset to second hardware window */
    INT32       (*bnkMgr)();   /* offset of bank manager           */
    VOID        (*plnMgr)();   /* offset of plane manager          */
    VOID        (*altMgr)();   /* offset of alt plane manager      */
} DBPstruc;


 /* Display Device Structure */
typedef struct _DspDevc{
    INT32    devcName;         /* Name of device                   */
    DBPstruc *bnkTblPtr;       /* Pointer to bank manager table    */
    INT32    (*devTblPtr)();   /* Pointer to device driver table   */
} DspDevc;

/* Display Parameter Structure */
typedef struct _DSPstruc{
    UINT8 initFlags;   /* initgrafix flags            */
    INT16 rowByte;     /* Bytes per scan line/row     */
    INT16 dvTech;      /* Device technology           */
    INT16 interLeave;  /* Scan line interleave factor */
    INT16 pixDev;      /* Device code & font #        */
    INT32 pixWth;      /* Pixel width                 */
    INT32 pixHgt;      /* Pixel height                */
    INT32 pixRsX;      /* Pixel resolution horz.      */
    INT32 pixRsY;      /* Pixel resolution vert.      */
    UINT8 pixBit;      /* Color bits per pixel        */
    UINT8 pixPln;      /* Color planes per pixel      */
    INT16 interOff;    /* Interleave offset           */
    INT16 interSeg;    /* Interleave segment          */
    INT16 pad;         /* Padding for the structure   */
    INT32 baseAdr[4];  /* Base address (4 planes max) */
} DSPstruc;


/*-------- Device Bank and Plane Parameter Tables start here  --------*/
extern  DBPstruc tLCD;          /* Generic LCD  */
extern  DBPstruc tLCD1;         /* 1-bit   LCD  */
extern  DBPstruc tLCD2;         /* 2-bit   LCD  */
extern  DBPstruc tLCD4;         /* 4-bit   LCD  */
extern  DBPstruc tLCD8;         /* 8-bit   LCD  */
extern  DBPstruc tLCD16;        /* 16-bit  LCD  */
extern  DBPstruc tLCD24;        /* 24-bit  LCD  */
extern  DBPstruc tLCD32;        /* 32-bit  LCD  */


/* Local Functions */
VOID   SCREENS_Resume(grafMap *argGRAFMAP);
INT32  SCREENS_InitMemoryForGrafMap(grafMap *gmap);
VOID   SetBitmap(INT32 argPAGE, grafMap *argGMAP);
INT32  SCREENS_CloseGrafDriver(grafMap *argGRAFMAP);
VOID   SCREENS_InitBankManager(grafMap *argGRAFMAP);
VOID   SCREENS_InitDeviceManager(grafMap *argGRAFMAP);
VOID   SCREENS_InitRowTable( grafMap *argBitMap, INT32 argInrLve, INT32 argInrSeg, INT32 argInrSize);

extern VOID curs_ega(VOID);

/* These are the local stub functions that will never be changed */
VOID   PlaneStub(VOID);
VOID   PrimErr(VOID);
INT32  iPrimErr(VOID);
SIGNED lPrimErr(VOID);
VOID   StubIDV(VOID);
INT32  StubIDVI(VOID);
INT32  nuMapPhysAdr(SIGNED *physMem, INT32 blkSize, SIGNED *logMem);
VOID   EVENTH_nuSegInfo(INT16 *aePtr, INT16 *bePtr, INT16 *dsPtr);
INT32  BankVESA(VOID);
INT32  BankStub(VOID);


#endif /* _SCREEN_S_H_ */

