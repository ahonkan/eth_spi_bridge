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
*  screen_i.h
*
* DESCRIPTION
*
*  This file contains the display device initialization functions.
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
#ifndef _SCREEN_I_H_
#define _SCREEN_I_H_

#define hasAltMgr         0x80   /* high bit of wnType indicates altmgr */
#define inFldRT           1      /* fold rowtables into one */
#define inXRow            2      /* add extra row for off-screen cursor routines */
#define inLinear          4      /* rowtable is linear */
#define inVgaHw           8      /* can do VGA chain-4 mode */
#define in565             16     /* use 5:6:5 hicolor format */

/* return codes */
#define AlreadyInitd      1      /* InitGraphics has already been called */
#define NoShell          -1      /* The Metagraphics Driver ( METASHEL.EXE ) is not installed */
#define Bad_DEV          -2      /* Passed value is not a known device */
#define RowTblERR        -3      /* Error allocating rowtables */
#define MapError         -4      /* Can't map in device's physical memory */
#define SleepyDev        -5      /* Device won't respond */

/* Set the pixel device code.  By default set to 0 */
#define     PIXEL_DEV_CODE          0

/* Global variable for the screen */
extern SIGNED    bsAdr;

/* Local functions  */
VOID   SCREENI_InitDisplayDevTable(VOID);
INT32  SCREENI_InitBitmap(INT32 argDEVICE, grafMap *argGRAFMAP);
INT32  SCREENI_InitGrafDriver(grafMap *gmap);
INT32  SCREENI_InitGraphics(VOID);

/* Externs needed for allocation */
extern NU_MEMORY_POOL  System_Memory;

/* Initialization function extern from the Grafix RS library */
extern VOID INPUTT_InitInputDevTable(VOID);

#endif /* _SCREEN_I_H_ */

