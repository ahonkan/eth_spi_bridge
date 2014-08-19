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
*  inigrafv.c                                                   
*
* DESCRIPTION
*
*  This file contains initialization functions for Grafix RS.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  nu_os_ui_grafixrs_init
*  NU_InitGrafVars
*
* DEPENDENCIES
*
*  nucleus.h
*  error_management.h
*  rs_base.h
*  rsfonts.h
*  global.h
*  stopgfx.h
*  display_config.h
*  inigrafv.h
*  textd.h
*  marin_11.h
*  mplus_1c_7.h
*  fonti.h
*
***************************************************************************/
#include "nucleus.h"
#include "os/kernel/plus/supplement/inc/error_management.h"

#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/global.h"
#include "ui/stopgfx.h"

#include "drivers/display_config.h"

#include "ui/inigrafv.h"
#include "services/runlevel_init.h"

extern VOID  RS_Init_Text_And_Pen(VOID);
extern INT32 SCREENI_InitGraphics(VOID);

extern VOID SetFont( fontRcd *FONT);
extern VOID ERC_System_Error (INT);

/* screen semaphore */
extern NU_SEMAPHORE    ScreenSema;

#ifdef  INCLUDE_DEFAULT_FONT

#ifdef USE_UNICODE

#include "ui/textd.h"             /* needed for struct definition */
#include "ui/marin_11.h"        /* add desired Unicode font header */

#else

/* define mplus_1c_7.h to define the global font variable for the system font */
/* Other font header files may need to be defined for creation of other font variables */
#include "ui/mplus_1c_7.h"
#endif

#ifndef USE_UNICODE

/* Create a global variable to be used for the mplus_1c_7.h */
/* mplus_1c_7 was #defined in mplus_1c_7.h */
fontRcd * mplus_1c7 = mplus_1c_7;

#endif

#endif  /* INCLUDE_DEFAULT_FONT */

extern VOID ERC_System_Error (INT);

#include "ui/fonti.h"

PenSetUp    thePenSetUp;
TextSetUp   theTextSetUp;

#ifdef  USE_CURSOR

    /* Default cursor data */
    /* Arrow Cursor */
static const UINT8 CursorMask0Data[32] = {
    0xC0, 0x00, 0xE0, 0x00, 0xF0, 0x00, 0xF8, 0x00,
    0xFC, 0x00, 0xFE, 0x00, 0xFF, 0x00, 0xFF, 0x80,
    0xFF, 0xC0, 0xFF, 0xE0, 0xFE, 0x00, 0xEF, 0x00,
    0xCF, 0x00, 0x87, 0x80, 0x07, 0x80, 0x03, 0x80};

static const UINT8 CursorImag0Data[32] = {
    0xC0, 0x00, 0xA0, 0x00, 0x90, 0x00, 0x88, 0x00,
    0x84, 0x00, 0x82, 0x00, 0x81, 0x00, 0x80, 0x80,
    0x80, 0x40, 0x83, 0xE0, 0x92, 0x00, 0xA9, 0x00,
    0xC9, 0x00, 0x84, 0x80, 0x04, 0x80, 0x03, 0x80};

    /* Checkmark Cursor */
static const UINT8 CursorMask1Data[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0x00, 0x0F, 0x00, 0x1E, 0x00, 0x3C,
    0x00, 0x78, 0x00, 0xF0, 0xF1, 0xE0, 0xFB, 0xC0,
    0x7F, 0x80, 0x1F, 0x00, 0x0E, 0x00, 0x04, 0x00};

static const UINT8 CursorImag1Data[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0x00, 0x09, 0x00, 0x12, 0x00, 0x24,
    0x00, 0x48, 0x00, 0x90, 0xF1, 0x20, 0x8A, 0x40,
    0x64, 0x80, 0x11, 0x00, 0x0A, 0x00, 0x04, 0x00};

    /* Crosshair Cursor */
static const UINT8 CursorMask2Data[32] = {
    0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80,
    0x03, 0x80, 0x03, 0x80, 0xFF, 0xFE, 0xFF, 0xFE,
    0xFF, 0xFE, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80,
    0x03, 0x80, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00};

static const UINT8 CursorImag2Data[32] = {
    0x00, 0x00, 0x03, 0x80, 0x02, 0x80, 0x02, 0x80,
    0x02, 0x80, 0x02, 0x80, 0xFE, 0xFE, 0x80, 0x02,
    0xFE, 0xFE, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80,
    0x02, 0x80, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00};

    /* Box Cursor */
static const UINT8 CursorMask3Data[32] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07,
    0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};

static const UINT8 CursorImag3Data[32] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x80, 0x01,
    0xBF, 0xFD, 0xA0, 0x05, 0xA0, 0x05, 0xA0, 0x05,
    0xA0, 0x05, 0xA0, 0x05, 0xA0, 0x05, 0xA0, 0x05,
    0xBF, 0xFD, 0x80, 0x01, 0xFF, 0xFF, 0x00, 0x00};

    /* Pointing-Hand Cursor */
static const UINT8 CursorMask4Data[32] = {
    0x00, 0x00, 0x07, 0x00, 0x0F, 0x80, 0x1F, 0x80,
    0x3F, 0x00, 0x7F, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xF0, 0xFF, 0xE0,
    0xFF, 0xE0, 0xFF, 0xC0, 0x7F, 0xC0, 0x3F, 0xC0};

static const UINT8 CursorImag4Data[32] = {
    0x00, 0x00, 0x07, 0x00, 0x08, 0x80, 0x16, 0x80,
    0x2D, 0x00, 0x59, 0xFE, 0x98, 0x01, 0xB3, 0xFE,
    0xA8, 0x01, 0x9B, 0xDE, 0xB8, 0x10, 0xBB, 0xA0,
    0xB8, 0x20, 0x9B, 0x40, 0x40, 0x40, 0x3F, 0xC0};

    /* "Hold"ing Hand Cursor */
static const UINT8 CursorMask5Data[32] = {
    0x01, 0xC0, 0x07, 0xF0, 0x0F, 0xF8, 0x0F, 0xFC,
    0x0F, 0xFE, 0x0F, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
    0xFF, 0xFE, 0xFF, 0xFE, 0x7F, 0xFE, 0x3F, 0xFE,
    0x3F, 0xFE, 0x1F, 0xFC, 0x0F, 0xF8, 0x07, 0xF0};

static const UINT8 CursorImag5Data[32] = {
    0x01, 0xC0, 0x06, 0x30, 0x08, 0x88, 0x0A, 0xAC,
    0x0A, 0xA2, 0x0A, 0xAA, 0xFA, 0xAA, 0x8A, 0xAA,
    0xAA, 0xAA, 0xB0, 0x02, 0x57, 0xFA, 0x2F, 0xFA,
    0x27, 0xF2, 0x13, 0xE4, 0x08, 0x08, 0x07, 0xF0};

    /* Question Cursor */
static const UINT8 CursorMask6Data[32] = {
    0x00, 0x00, 0x1F, 0xF0, 0x3F, 0xF8, 0x7F, 0xFC,
    0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
    0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
    0xFF, 0xFE, 0x7F, 0xFC, 0x3F, 0xF8, 0x1F, 0xF0};

static const UINT8 CursorImag6Data[32] = {
    0x00, 0x00, 0x1F, 0xF0, 0x20, 0x08, 0x40, 0x04,
    0x87, 0xC2, 0x8C, 0x62, 0x8C, 0x62, 0x80, 0xC2,
    0x81, 0x82, 0x81, 0x82, 0x80, 0x02, 0x81, 0x82,
    0x81, 0x82, 0x40, 0x04, 0x20, 0x08, 0x1F, 0xF0};

    /* Hourglass Cursor */
static const UINT8 CursorMask7Data[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const UINT8 CursorImag7Data[32] = {
    0x00, 0x00, 0x00, 0x00, 0xBF, 0xFD, 0x8F, 0xF1,
    0xA3, 0xC5, 0xB0, 0x0D, 0xB8, 0x1D, 0xBC, 0x3D,
    0xBD, 0xBD, 0xBB, 0xDD, 0xB6, 0x6D, 0xAC, 0x35,
    0x98, 0x19, 0xB0, 0x0D, 0x00, 0x00, 0x00, 0x00};

#endif      /* USE_CURSOR */

STATUS nu_os_ui_grafixrs_init(const CHAR * key, INT startorstop);


#ifdef  NU_PROCESS

INT main(INT c, CHAR **argv)
{
    STATUS  status;
    
    status = nu_os_ui_grafixrs_init("/nu/os/ui/grafixrs", RUNLEVEL_START);
    
    /* Suspend on exit if successful init */
    if (status == NU_SUCCESS)
    {
        status = EXIT_CONTINUE;
    }

    return (status);
}

VOID deinit(VOID)
{
    (VOID)nu_os_ui_grafixrs_init("/nu/os/ui/grafixrs", RUNLEVEL_STOP);
}

#endif  /* NU_PROCESS */

/*************************************************************************
*
* FUNCTION
*
*       NU_Serial_Init
*
* DESCRIPTION
*
*       This function initializes serial middleware
*
* INPUTS
*
*       VOID
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS nu_os_ui_grafixrs_init(const CHAR * key, INT startorstop)
{
    STATUS          status = NU_SUCCESS;

#if (CFG_NU_OS_UI_GRAFIXRS_EXPORT_SYMBOLS == NU_TRUE)
    /* Keep symbols for nu.os.ui.grafixrs */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_UI_GRAFIXRS);
#endif /* CFG_NU_OS_UI_GRAFIXRS_EXPORT_SYMBOLS */
    
    if (startorstop == RUNLEVEL_START)
    {
	    /* Initialize the text and pen */
	    RS_Init_Text_And_Pen();

	    /* Initialize the display driver. */
	    status = SCREENI_InitGraphics();
    }
    else if (startorstop == RUNLEVEL_STOP)
    {
        status = StopGraphics();
    }
    
    return status;
}

/***************************************************************************
* FUNCTION
*
*    NU_InitGrafVars
*
* DESCRIPTION
*
*    Function NU_InitGrafVars generates grafdata code all others reference
*    it by offset.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID NU_InitGrafVars(VOID)
{

#if     (defined(USE_CURSOR) || defined(FILL_PATTERNS_SUPPORT))
    
    /*  Initializing certain variables  */
    register INT32 i;

#endif  /* (defined(USE_CURSOR) || defined(FILL_PATTERNS_SUPPORT)) */

#ifdef  FILL_PATTERNS_SUPPORT
    
    pattern        *FillPtr;

#endif /* FILL_PATTERNS_SUPPORT */
    
    STATUS         status;
  
    /*  Do the actual initializations  */
    gFlags = 0;               /* Graphic flags */

#ifdef  USE_CURSOR
    
    /* Initialize cursor data */
    for (i = 0; i < 8; i++)
    {
        CursorMasks[i].imWidth  = 16;    /* Image pixel width (X) */
        CursorMasks[i].imHeight = 16;    /* Image pixel height (Y) */
        CursorMasks[i].imAlign  = 0;     /* Image alignment */
        CursorMasks[i].imFlags  = 0;     /* Image flags */
        CursorMasks[i].pad      = 0;     /* Image padding */
        CursorMasks[i].imBytes  = 2;     /* Image bytes per row */
        CursorMasks[i].imBits   = 1;     /* Image bits per pixel */
        CursorMasks[i].imPlanes = 1;     /* Image planes per pixel */

        CursorImags[i].imWidth  = 16;    /* Image pixel width (X) */
        CursorImags[i].imHeight = 16;    /* Image pixel height (Y) */
        CursorImags[i].imAlign  = 0;     /* Image alignment */
        CursorImags[i].imFlags  = 0;     /* Image flags */
        CursorImags[i].pad      = 0;     /* Image padding */
        CursorImags[i].imBytes  = 2;     /* Image bytes per row */
        CursorImags[i].imBits   = 1;     /* Image bits per pixel */
        CursorImags[i].imPlanes = 1;     /* Image planes per pixel */
    }

    for (i = 0; i < 32; i++)
    {
        CursorMasks[0].IMGDATA[i] = CursorMask0Data[i];
        CursorImags[0].IMGDATA[i] = CursorImag0Data[i];
        CursorMasks[1].IMGDATA[i] = CursorMask1Data[i];
        CursorImags[1].IMGDATA[i] = CursorImag1Data[i];
        CursorMasks[2].IMGDATA[i] = CursorMask2Data[i];
        CursorImags[2].IMGDATA[i] = CursorImag2Data[i];
        CursorMasks[3].IMGDATA[i] = CursorMask3Data[i];
        CursorImags[3].IMGDATA[i] = CursorImag3Data[i];
        CursorMasks[4].IMGDATA[i] = CursorMask4Data[i];
        CursorImags[4].IMGDATA[i] = CursorImag4Data[i];
        CursorMasks[5].IMGDATA[i] = CursorMask5Data[i];
        CursorImags[5].IMGDATA[i] = CursorImag5Data[i];
        CursorMasks[6].IMGDATA[i] = CursorMask6Data[i];
        CursorImags[6].IMGDATA[i] = CursorImag6Data[i];
        CursorMasks[7].IMGDATA[i] = CursorMask7Data[i];
        CursorImags[7].IMGDATA[i] = CursorImag7Data[i];
    }

#endif      /* USE_CURSOR */
    
#ifdef  FILL_PATTERNS_SUPPORT

    /* build patPtr table */
    for( i = 0; i < 32; i++)
    {
        FillPtr = (pattern *) &FillPat[i];
        patTable.patPtr[i] = FillPtr;
        patTable.patAlignX[i] = 0;
        patTable.patAlignY[i] = 0;
    }

#endif /* FILL_PATTERNS_SUPPORT */
    
    mpSize = WORKSPACE_BUFFER_SIZE;      /* pool size */

#ifdef  USE_CURSOR
    
    /* Current cursor list */
    /*  CursorMask0, CursorImag0,     1,         0 */
    CursorTable[0][0] = (SIGNED) &CursorMasks[0];
    CursorTable[0][1] = (SIGNED) &CursorImags[0];
    CursorTable[0][2] = 1;
    CursorTable[0][3] = 0;

    /*  CursorMask1, CursorImag1,     5,        12 */
    CursorTable[1][0] = (SIGNED) &CursorMasks[1];
    CursorTable[1][1] = (SIGNED) &CursorImags[1];
    CursorTable[1][2] = 5;
    CursorTable[1][3] = 12;

    /*  CursorMask2, CursorImag2,     7,         7 */
    CursorTable[2][0] = (SIGNED) &CursorMasks[2];
    CursorTable[2][1] = (SIGNED) &CursorImags[2];
    CursorTable[2][2] = 7;
    CursorTable[2][3] = 7;

    /*  CursorMask3, CursorImag3,     7,         8 */
    CursorTable[3][0] = (SIGNED) &CursorMasks[3];
    CursorTable[3][1] = (SIGNED) &CursorImags[3];
    CursorTable[3][2] = 7;
    CursorTable[3][3] = 8;

    /*  CursorMask4, CursorImag4,    16,         7 */
    CursorTable[4][0] = (SIGNED) &CursorMasks[4];
    CursorTable[4][1] = (SIGNED) &CursorImags[4];
    CursorTable[4][2] = 16;
    CursorTable[4][3] = 7;

    /*  CursorMask5, CursorImag5,     7,         8 */
    CursorTable[5][0] = (SIGNED) &CursorMasks[5];
    CursorTable[5][1] = (SIGNED) &CursorImags[5];
    CursorTable[5][2] = 7;
    CursorTable[5][3] = 8;

    /*  CursorMask6, CursorImag6,     7,         6 */
    CursorTable[6][0] = (SIGNED) &CursorMasks[6];
    CursorTable[6][1] = (SIGNED) &CursorImags[6];
    CursorTable[6][2] = 7;
    CursorTable[6][3] = 6;

    /*  CursorMask7, CursorImag7,     7,         8};*/
    CursorTable[7][0] = (SIGNED) &CursorMasks[7];
    CursorTable[7][1] = (SIGNED) &CursorImags[7];
    CursorTable[7][2] = 7;
    CursorTable[7][3] = 8;

    /* Default cursor list */
    /*  CursorMask0, CursorImag0,     1,         0 */
    DefCursorTable[0][0] = (SIGNED) &CursorMasks[0];
    DefCursorTable[0][1] = (SIGNED) &CursorImags[0];
    DefCursorTable[0][2] = 1;
    DefCursorTable[0][3] = 0;

    /*  CursorMask1, CursorImag1,     5,        12 */
    DefCursorTable[1][0] = (SIGNED) &CursorMasks[1];
    DefCursorTable[1][1] = (SIGNED) &CursorImags[1];
    DefCursorTable[1][2] = 5;
    DefCursorTable[1][3] = 12;

    /*  CursorMask2, CursorImag2,     7,         7 */
    DefCursorTable[2][0] = (SIGNED) &CursorMasks[2];
    DefCursorTable[2][1] = (SIGNED) &CursorImags[2];
    DefCursorTable[2][2] = 7;
    DefCursorTable[2][3] = 7;

    /*  CursorMask3, CursorImag3,     7,         8 */
    DefCursorTable[3][0] = (SIGNED) &CursorMasks[3];
    DefCursorTable[3][1] = (SIGNED) &CursorImags[3];
    DefCursorTable[3][2] = 7;
    DefCursorTable[3][3] = 8;

    /*  CursorMask4, CursorImag4,    16,         7 */
    DefCursorTable[4][0] = (SIGNED) &CursorMasks[4];
    DefCursorTable[4][1] = (SIGNED) &CursorImags[4];
    DefCursorTable[4][2] = 16;
    DefCursorTable[4][3] = 7;

    /*  CursorMask5, CursorImag5,     7,         8 */
    DefCursorTable[5][0] = (SIGNED) &CursorMasks[5];
    DefCursorTable[5][1] = (SIGNED) &CursorImags[5];
    DefCursorTable[5][2] = 7;
    DefCursorTable[5][3] = 8;

    /*  CursorMask6, CursorImag6,     7,         6 */
    DefCursorTable[6][0] = (SIGNED) &CursorMasks[6];
    DefCursorTable[6][1] = (SIGNED) &CursorImags[6];
    DefCursorTable[6][2] = 7;
    DefCursorTable[6][3] = 6;

    /*  CursorMask7, CursorImag7,     7,         8};*/
    DefCursorTable[7][0] = (SIGNED) &CursorMasks[7];
    DefCursorTable[7][1] = (SIGNED) &CursorImags[7];
    DefCursorTable[7][2] = 7;
    DefCursorTable[7][3] = 8;

#endif      /* USE_CURSOR */

    /* Create Display synchronization semaphore.  */
    status = NU_Create_Semaphore(&ScreenSema, "Display", 1, NU_FIFO);

    /* Was the Semaphore created? */
    if (status != NU_SUCCESS)
    {
        /* Bad semaphore creation, call system error */
        ERC_System_Error(status);
    }

#ifdef  INCLUDE_DEFAULT_FONT
    
#ifdef USE_UNICODE
    imbFnt = (signed char *) marin_11;   
#else
    imbFnt = (signed char *) mplus_1c7;
#endif

#endif  /* INCLUDE_DEFAULT_FONT */

}
