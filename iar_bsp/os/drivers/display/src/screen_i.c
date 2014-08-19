/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*   
*       screen_i.c
*   
*   DESCRIPTION
*   
*       This file contains the display (VGA or LCD) device generic 
*       functions.
*   
*   DATA STRUCTURES
*   
*       Variables required are in globalv.h.
*   
*   FUNCTIONS
*   
*       SCREENI_InitGraphics 
*       SCREENI_InitDisplayDevTable
*       SCREENI_InitBitmap
*       SCREENI_InitGrafDriver
*       SCREENI_DisplayDevMgr
*       SCREENI_InitDisplay 
*   
*   DEPENDENCIES
*   
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_services.h
*       nu_drivers.h
*       display_inc.h
*       screen_path.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/nu_ui.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "drivers/display_inc.h"
#include "drivers/screen_path.h"

extern VOID InitPort(rsPort *argPORT);

static STATUS  SCREENI_InitDisplay(VOID);
static INT32  SCREENI_DisplayDevMgr(grafMap *argGRAFMAP, INT32 dmFunc, void *dmParam);

DISPLAY_DEVICE      SCREENI_Display_Device;      

static  BOOLEAN     SCREENI_Display_Enabled = NU_FALSE;

extern  DspDevc displayTable[numDisplays+1];

/***************************************************************************
* FUNCTION
*
*    SCREENI_InitGraphics
*
* DESCRIPTION
*
*    Initializes the system, and a default rsPort and grafMap.
*
*       It also allocates memory using the GrafAlloc function for:
*           1) a driver and rowtables (via SCREENI_InitBitmap() ).
*           2) the memory pool.
*           3) load a default font.
*
* INPUTS
*
*    INT32 argDEVICE - defines the graphics display type
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
INT32 SCREENI_InitGraphics(VOID)
{
    /* place to keep error code */
    INT32  rtnCode;
    INT16  status = NU_SUCCESS;
    INT32  argDEVICE;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set some GRAFIX_DATA globals to zero */
    if( gFlags & gfGrafInit )
    {
        /* have we already been here? */
        /* yes, leave with error returned */
        status = AlreadyInitd;

        /* Set the return code. */
        rtnCode = AlreadyInitd;
    }

    if( status == NU_SUCCESS)
    {
        /* Initialize certain graphics variables */
        NU_InitGrafVars();

        /* initialize the cursor save pointers */
#ifdef  USE_CURSOR
        curs_ega();
#endif      /* USE_CURSOR */
        SCREENI_InitDisplayDevTable();

        grafError = 0;
        LocX      = 0;
        LocY      = 0;

#ifdef  USE_CURSOR

        /* set cursor defaults */
        /* cursor style 0 */
        CursorNumbr = 0;

        /* cursor back color = black */
        CursBackColor = Black;

        /* cursor fore color = white */
        CursForeColor = White;

        /* cursor is not visible */
        CursorLevel = -1;

        /* disable protect rect, force off grafMap */
        CursProtXmin = 0x7FFF;

        /* leave some calculation range */
        ProtXmin = 0x7F00;

#endif      /* USE_CURSOR */

        /* set default input device record */
        curInput = &defMouse;

        /* Set blitRcd vars */
        /* rectangular clipping */
        grafBlit.blitFlags = bfClipRect;
        grafBlit.blitCnt   = 1;
        grafBlit.blitClip  = &ViewClip;
        grafBlit.blitList  = (SIGNED) &grafBlist;
        grafBlit.blitAlloc = 2 * sizeof(rect);

        /* Clear out all the indirect vector pointers */
        lineExecIDV   = (SIGNED) &StubIDV;
        
#ifdef  THIN_LINE_OPTIMIZE
        lineThinIDV   = (SIGNED) &StubIDV;
#endif  /* THIN_LINE_OPTIMIZE */

#ifdef  DASHED_LINE_SUPPORT
        lineDashIDV   = (SIGNED) &StubIDV;
#endif  /* DASHED_LINE_SUPPORT */

        linePattIDV   = (SIGNED) &StubIDV;
        lineOvalIDV   = (SIGNED) &StubIDV;
        lineOvPolyIDV = (SIGNED) &StubIDV;
        lineSqPolyIDV = (SIGNED) &StubIDV;
#ifdef  USE_CURSOR
        MovCursIDV    = (VOID (*)()) StubIDV;
#endif      /* USE_CURSOR */
        txtAlnIDV     = (INT32 (*)()) StubIDV;
        txtDrwIDV     = (INT32 (*)()) StubIDV;
        txtAPPIDV     = (INT32 (*)()) StubIDV;
#ifdef      USE_STROKEDFONT
        txtStrokeIDV  = (INT32 (*)()) StubIDV;
#endif      /* USE_STROKEDFONT */
        stpEventIDV   = (VOID (*)()) StubIDV;
        stpMouseIDV   = (INT32 (*)()) StubIDVI;

        /* Clear out memory pool pointers */
        /* Global pointer to free pool area */
        mpWorkSpace = 0;

        /* Global pointer to end of pool area */
        mpWorkEnd   = 0;

        /* Global size allocated workGrafMaps rowtable */
        mpRowTblSz  = 0;

        /* Initialize the device according to the mode supported by the hardware. */

#ifdef INCLUDE_8_BIT
        /* Use the 8 BPP mode. */
        argDEVICE = cLCD;
#endif
#ifdef INCLUDE_16_BIT
        /* Use the 16 BPP mode. */
        argDEVICE = cLCD16;
#endif
#ifdef INCLUDE_1_BIT
        /* Use the 1 BPP mode. */
        argDEVICE = cLCD1;
#endif
#ifdef INCLUDE_2_4_BIT
        /* Use the 2, 4 BPP mode. */
        argDEVICE = cLCD4;
#endif
#ifdef INCLUDE_24_BIT
        /* Use the 24 BPP mode. */
        argDEVICE = cLCD24;
#endif
#ifdef INCLUDE_32_BIT
        /* Use the 32 BPP mode. */
        argDEVICE = cLCD32;
#endif

        /* initialize the default structures */

        /* initialize the Default grafMap, use memory allocation for driver
           and rowtables */
        rtnCode = SCREENI_InitBitmap(argDEVICE, &defGrafMap);

#ifdef  INCLUDE_DEFAULT_FONT
        defFont = imbFnt;
#endif  /* INCLUDE_DEFAULT_FONT */
        
        status = GRAFIX_Allocation(&System_Memory, (VOID**) &mpWorkSpace, mpSize, 0);
        if( status == NU_SUCCESS )
        {
            mpWorkEnd = (UINT8 *)(mpWorkSpace + mpSize);
        }

        if(status == NU_SUCCESS)
        {
            
#ifdef  USE_CURSOR
            
            /* make the cursor grafMap = the default grafMap */
            cursBlit.blitDmap = &defGrafMap;

            /* place default cursor position on grafMap */
            /* use half-way point */
            CursorX = (defGrafMap.pixWidth >> 1);
            CursorY = (defGrafMap.pixHeight >> 1);

#endif      /* USE_CURSOR */
            
            InitPort(&defPort);

            /* reset and mark as been initialized */
            gFlags = gfGrafInit;
        }
    }

    NU_USER_MODE();

    return(rtnCode);
}

/***************************************************************************
* FUNCTION
*
*    SCREENI_InitDisplayDevTable
*
* DESCRIPTION
*
*    Initializes the device table with the display drivers.
*    Initialize bank manager structures.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID SCREENI_InitDisplayDevTable(VOID)
{
    INT32 i = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();


    /* 8-bit LCD */
#ifdef INCLUDE_8_BIT

    /* Initialize bank manager structures */
    tLCD8.wnType = 0;
    tLCD8.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD8.wnOff  = 0;
    tLCD8.bnkMgr = &BankStub;
    tLCD8.plnMgr = &PlaneStub;

    /* initialize display driver table */
    displayTable[i].devcName    = cLCD;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif

    /* 1-bit LCD */
#ifdef INCLUDE_1_BIT

    /* Initialize bank manager structures */
    tLCD1.wnType = 0;
    tLCD1.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD1.wnOff  = 0;
    tLCD1.bnkMgr = &BankStub;
    tLCD1.plnMgr = &PlaneStub;

    /* initialize display driver table */
    displayTable[i].devcName    = cLCD1;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif

    /* 2-bit LCD */
    /* 4-bit LCD */
#ifdef INCLUDE_2_4_BIT

    /* Initialize bank manager structures */
    tLCD2.wnType = 0;
    tLCD2.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD2.wnOff  = 0;
    tLCD2.bnkMgr = &BankStub;
    tLCD2.plnMgr = &PlaneStub;

    tLCD4.wnType = 0;
    tLCD4.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD4.wnOff  = 0;
    tLCD4.bnkMgr = &BankStub;
    tLCD4.plnMgr = &PlaneStub;

    /* initialize display driver table */
    /* 2-bit LCD */
    displayTable[i].devcName    = cLCD2;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;

    /* 4-bit LCD */
    displayTable[i].devcName    = cLCD4;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif

    /* 16-bit LCD */
#ifdef INCLUDE_16_BIT

    /* Initialize bank manager structures */
    tLCD16.wnType = 0;
    tLCD16.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD16.wnOff  = 0;
    tLCD16.bnkMgr = &BankStub;
    tLCD16.plnMgr = &PlaneStub;

    /* initialize display driver table */
    displayTable[i].devcName    = cLCD16;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif

    /* 24-bit LCD */
#ifdef INCLUDE_24_BIT

    /* Initialize bank manager structures */
    tLCD24.wnType = 0;
    tLCD24.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD24.wnOff  = 0;
    tLCD24.bnkMgr = &BankStub;
    tLCD24.plnMgr = &PlaneStub;

    /* initialize display driver table */
    displayTable[i].devcName    = cLCD24;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif

    /* 32-bit LCD */
#ifdef INCLUDE_32_BIT

    /* Initialize bank manager structures */
    tLCD32.wnType = 0;
    tLCD32.wnSize = MAX_SCREEN_WIDTH_X;
    tLCD32.wnOff  = 0;
    tLCD32.bnkMgr = &BankStub;
    tLCD32.plnMgr = &PlaneStub;

    /* initialize display driver table */
    displayTable[i].devcName    = cLCD32;
    displayTable[i].bnkTblPtr   = NU_NULL;
    displayTable[i++].devTblPtr = &SCREENI_DisplayDevMgr;
#endif
    /* set end value */
    displayTable[i].devcName = -1;

    NU_USER_MODE();

} /* end of SCREENI_InitDisplayDevTable() */


/***************************************************************************
* FUNCTION
*
*    SCREENI_InitBitmap
*
* DESCRIPTION
*
*    Initializes the passed grafMap using internal data associated
*    with the passed device code.  For the 'virtual' devices MEM, EMS,
*    XMS, or DISK, the grafMaps size items must be set as requested.
*    For the 'USER' device the grafMaps size items as well as mapFlags,
*    and devTech must be set as requested. RowTables are not allocated.
*
*    Primitives/managers will be registered (or allocated and loaded).
*    Rowtables will be allocated and initialized (unless cUSER).
*    The device manager will be called for wake up.
*    This module also obtains the pointer to access screen memory.
*
* INPUTS
*
*    INT32 argDEVICE
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    This module also obtains the pointer to access screen memory.
*    Returns:         0 = ok
*                    -1 = (reserved for no TSR shell present)
*                    -2 = Unknown device
*                    -3 = Can't allocate rowtables or other (out of memory)
*                    -4 = can't map in devices physical memory (protected mode)
*                    -5 = device doesn't respond
*                    -6 = can't load driver.
*
****************************************************************************/
INT32 SCREENI_InitBitmap(INT32 argDEVICE, grafMap *argGRAFMAP)
{
    INT32    rtnCode;
    INT32    plns;
    INT32    rtSize;
    DSPstruc *devTblPtr = NU_NULL;
    STATUS   Memory_Status;
    INT16    loadNow  = NU_FALSE;
    INT16    finishUp = NU_FALSE;
    INT16    done     = NU_FALSE;
    INT16    noDevice = NU_FALSE;

    /* Display Parameter Tables */

    DSPstruc tMEM1x1 = {                /* Worlds smallest bitmap               */
        inLinear,                       /* initFlags                            */
        1,                              /* rowBytes    - Bytes/Scan             */
        dtUser,                         /* dvTech     - Device technology       */
        1,                              /* interLeave -                         */
        0,                              /* pixDev     - Device code             */
        1,                              /* pixWth     - Pixel width             */
        1,                              /* pixHgt     - Pixel height            */
        100,                            /* pixRsX     - Pixel resolution horz.  */
        100,                            /* PixRsY     - Pixel resolution vert.  */
        1,                              /* pixBit     - Bits per pixel          */
        1,                              /* pixPln     - Planes per pixel        */
        0,                              /* interOff   - Interleave offset       */
        0,                              /* interSeg   - Interleave segment      */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */

#ifdef INCLUDE_1_BIT
    DSPstruc dtLCD1 = {                 /* Generic LCD - Monocolor              */
        inLinear,                       /* initFlags                            */
        MAX_SCREEN_WIDTH_X/8,           /* rowBytes - Bytes/Scan                */
        dtMono,                         /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0                               /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */
#endif

#ifdef INCLUDE_2_4_BIT
    DSPstruc dtLCD2 = {                 /* Generic LCD - grayscale              */
        inLinear,                       /* initFlags                            */
        MAX_SCREEN_WIDTH_X/4,           /* rowBytes - Bytes/Scan                */
        dtBits,                         /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */

    DSPstruc dtLCD4 = {                 /* Generic LCD - grayscale              */
        inLinear,                       /* initFlags                            */
        MAX_SCREEN_WIDTH_X/2,           /* rowBytes - Bytes/Scan                */
        dtBits,                         /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */
#endif

#ifdef INCLUDE_8_BIT
    DSPstruc dtLCD8 = {                 /* Generic LCD - 256-Color              */
        inLinear,                       /* initFlags                            */
        MAX_SCREEN_WIDTH_X,             /* rowBytes - Bytes/Scan                */
        0,                              /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        8,                              /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */
#endif

#ifdef INCLUDE_16_BIT
    DSPstruc dtLCD16 = {                /* 16-bit LCD                           */
#ifdef CM565
        inLinear | in565,               /* initFlags                            */
#else
        inLinear,
#endif
        2 * MAX_SCREEN_WIDTH_X,/* rowBytes - Bytes/Scan           */
        dt16Bit,                        /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */
#endif

#ifdef INCLUDE_24_BIT
    DSPstruc dtLCD24 = {                /* 24-bit LCD                           */
        inLinear,                       /* initFlags                            */
        3 * MAX_SCREEN_WIDTH_X,         /* rowBytes - Bytes/Scan                */
        dt24Bit,                        /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */

#endif

#ifdef INCLUDE_32_BIT
    DSPstruc dtLCD32 = {                /* 16-bit LCD                           */
        inLinear,                       /* initFlags                            */
        4 * MAX_SCREEN_WIDTH_X,         /* rowBytes - Bytes/Scan                */
        dt32Bit,                        /* dvTech - Device technology           */
        INTERLEAVE,                     /* InterLeave -                         */
        PIXEL_DEV_CODE,                 /* pixDev - Device code                 */
        MAX_SCREEN_WIDTH_X,             /* pixWth - Pixel width                 */
        MAX_SCREEN_HEIGHT_Y,            /* pixHgt - Pixel height                */
        0,                              /* pixRsX - resolution horz.            */
        0,                              /* PixRsY - resolution vert.            */
        BPP,                            /* pixBit - Bits per pixel              */
        0,                              /* pixPln - Planes per pixel            */
        INTERLEAVE_OFFSET,              /* interOff - Interleave offset         */
        INTERLEAVE_SEGMENT,             /* interSeg - Interleave segment        */
        0,                              /* Padding to align the structure       */
        {0}};                           /* baseAdr0   - none                    */
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* start with good return code */
    rtnCode = 0;

#ifdef  USE_CURSOR
    
    /* first set fields that all types of grafMap must have set */
    /* call back to cursor resync function */
    argGRAFMAP->cbSyncFunc = SCREENS_Resume;

#endif      /* USE_CURSOR */
    
    /* call back to post an error */
    argGRAFMAP->cbPostErr  = NU_NULL;

    /* start grafMap busy semaphore locked */
    argGRAFMAP->mapLock    = -1;

    /* clear mapFlags */
    argGRAFMAP->mapFlags   = 0;

    /* clear device class */
    argGRAFMAP->devClass   = 0;

    /* clear bank management fields */
    argGRAFMAP->mapWinType    = 0;
    argGRAFMAP->mapWinPlane   = 0;
    argGRAFMAP->mapWinYmin[0] = 0;
    argGRAFMAP->mapWinYmin[1] = 0;
    argGRAFMAP->mapWinScans   = -2;
    argGRAFMAP->mapWinYmax[0] = -2;
    argGRAFMAP->mapWinYmax[1] = -2;

    /* set device code */
    argGRAFMAP->devMode = argDEVICE;

    /* is this a real device or one of our 'virtual' types? */
    switch (argDEVICE)
    {
    case cUSER:
        /* User bitmap */
        devTblPtr = &tMEM1x1;
        loadNow = NU_TRUE;
    break;

    case cMEMORY:
        /* memory bitmap */
        /* init a memory bitmap */
        rtnCode = SCREENS_InitMemoryForGrafMap(argGRAFMAP);
        if (rtnCode != 0)
        {
             /* problem */
            if( rtnCode == c_OutofMem )
            {
                /* say out of memory */
                rtnCode = -3;
            }
            else
            {
                if( rtnCode == -1 )
                {
                    /* say can't load driver */
                    rtnCode = -6;
                }
                else
                {
                    /* say device did not respond */
                    rtnCode = -5;
                }
            }
        }

#ifdef INCLUDE_16_BIT

        /* memory bitmap was initialized */
        if( defGrafMap.mapFlags & mf565 )
        {
            argGRAFMAP->mapFlags |= mf565;
        }

#endif  /* INCLUDE_16_BIT */

        /* exit switch (argDEVICE) */
        finishUp = NU_TRUE;

    break;

         /* ok, it's not one of ours, look up device code in Graphics Device Table */
         /* Check the device code in Graphics Device Table */
         /* Check for the remaining cases */

#ifdef INCLUDE_8_BIT
    case cLCD:
        /* Generic LCD */
        devTblPtr = &dtLCD8;
    break;
#endif

#ifdef INCLUDE_1_BIT
    case cLCD1:
        /* 1-bit LCD */
        devTblPtr = &dtLCD1;
    break;
#endif

#ifdef INCLUDE_2_4_BIT
    case cLCD2:
        /* 2-bit LCD */
        devTblPtr = &dtLCD2;
    break;

    case cLCD4:
        /* 4-bit LCD */
        devTblPtr = &dtLCD4;
    break;
#endif

#ifdef INCLUDE_16_BIT
    case cLCD16:
        /* 16-bit LCD */
        devTblPtr = &dtLCD16;
    break;
#endif

#ifdef INCLUDE_24_BIT
    case cLCD24:
        /* Generic LCD */
        devTblPtr = &dtLCD24;
    break;
#endif

#ifdef INCLUDE_32_BIT
    case cLCD32:
        /* Generic LCD */
        devTblPtr = &dtLCD32;
    break;
#endif

    case cNODEVICE:
         /* no device required */
        devTblPtr = &tMEM1x1;
    break;

    default:

        /* can't find that device code */
        rtnCode = -2;
        argDEVICE = cNODEVICE;
        argGRAFMAP->devMode = argDEVICE;
        devTblPtr = &tMEM1x1;
    break;
    }

    if (argDEVICE != cMEMORY && argDEVICE != cUSER)
    {
        /* Read in the planes per pixel attribute from the device structure.
           Note that this value is ignored while control comes here during 
           initialization of the display device. */
        devTblPtr->pixPln = SCREENI_Display_Device.display_planes_per_pixel;                    
    }
    
    do
    {
        if(noDevice)
        {
            argDEVICE = cNODEVICE;
            argGRAFMAP->devMode = argDEVICE;
            devTblPtr = &tMEM1x1;
            noDevice = NU_FALSE;
        }

        if( !loadNow && !finishUp)
        {
            /* Initialize the passed GrafMap */
            /* in linear mode? */
            if( devTblPtr->initFlags & inLinear )
            {
                argGRAFMAP->mapFlags |= mfRowTabl;
            }

            /* in VGA mode? */
            if( devTblPtr->initFlags & inVgaHw )
            {
                argGRAFMAP->mapFlags |= mfVgaHw;
            }

#ifdef INCLUDE_16_BIT

            /* hicolor format? */
            if( devTblPtr->initFlags & in565 )
            {
                argGRAFMAP->mapFlags |= mf565;
            }
            
#endif  /* INCLUDE_16_BIT */            

            /* Set the argGRAFMAP elements */
            argGRAFMAP->devClass  = devTblPtr->pixDev;
            argGRAFMAP->devTech   = devTblPtr->dvTech;
            argGRAFMAP->pixWidth  = devTblPtr->pixWth;
            argGRAFMAP->pixHeight = devTblPtr->pixHgt;
            argGRAFMAP->pixBits   = devTblPtr->pixBit;
            argGRAFMAP->pixPlanes = devTblPtr->pixPln;
            argGRAFMAP->pixBytes  = devTblPtr->rowByte;
        }

        if (!finishUp && !noDevice )
        {
            /* initialize primitives */
            if (SCREENI_InitGrafDriver(argGRAFMAP) != 0)
            {
                /* error initializing the device */
                rtnCode = -6;

                /* do null device */
                noDevice = NU_TRUE;
            }

            if (!noDevice)
            {
                /* issue devMgr call to wake-up device - this could modify fields
                in the grafMap if it wanted to */
                if (argGRAFMAP->mapDevMgr(argGRAFMAP, DMWAKEUP, devTblPtr->baseAdr[0]) != 0)
                {
                    /* error - won't wake up */
                    rtnCode = -5;
                }
                else
                {
                    devTblPtr->baseAdr[0] = (INT32)(SCREENI_Display_Device.display_frame_buffer);
                    argGRAFMAP->pixResX   = SCREENI_Display_Device.display_horizontal_dpi;
                    argGRAFMAP->pixResY   = SCREENI_Display_Device.display_vertical_dpi;
                    argGRAFMAP->pixPlanes = devTblPtr->pixPln = SCREENI_Display_Device.display_planes_per_pixel;                    
                }

                /* Initialize the rowtable(s) & mapTable pointers
                Note: mapWinScans must be set correctly at this point */
                if( (argDEVICE == cUSER) || (argDEVICE == cNODEVICE) )
                {
                    finishUp = NU_TRUE;
                }
            }
        }

        if (!finishUp && !noDevice)
        {
            /* null out first mapTable pointer */
            argGRAFMAP->mapTable[0] = 0;
            argGRAFMAP->pixHeight++;

            /* size of a rowtable */
            rtSize = argGRAFMAP->pixHeight << 2;

            /* build a rowtable for each plane */
            for (plns = 0; plns < argGRAFMAP->pixPlanes; plns++)
            {
                Memory_Status = GRAFIX_Allocation(&System_Memory, (VOID**) &argGRAFMAP->mapTable[plns],
                                                   rtSize, 0);

                if (Memory_Status != NU_SUCCESS )
                {
                    rtnCode = -3;

                    /* do null device */
                    noDevice = NU_TRUE;
                }

                if( !noDevice )
                {
                    /* map physical video memory to logical address */
                    if( nuMapPhysAdr(&devTblPtr->baseAdr[plns], 2, (INT32  *)argGRAFMAP->mapTable[plns]) != 0 )
                    {
                        /* can't map it in! */
                        rtnCode = -4;

                        /* do null device */
                        noDevice = NU_TRUE;
                    }

                    if( !noDevice )
                    {
                        /* is the fold rowtables flag on? */
                        if( devTblPtr->initFlags & inFldRT )
                        {
                            /* yes, point all four mapTable entries to the one rowtable */
                            argGRAFMAP->mapTable[1] = argGRAFMAP->mapTable[0];
                            argGRAFMAP->mapTable[2] = argGRAFMAP->mapTable[0];
                            argGRAFMAP->mapTable[3] = argGRAFMAP->mapTable[0];
                            break;
                        }
                    }
                }
            }

            if(!noDevice)
            {
                /* Now, all the planes have Rowtables, the address of the rowtables are
                   placed into grafMap.mapTable[n], and the address of each raster line in
                   the bitmap is in grafMap.mapTable[n]->rowtable[0].
                   Now we can fill out the rest of the rowtable entries */

                SCREENS_InitRowTable(argGRAFMAP, devTblPtr->interLeave, devTblPtr->interSeg, devTblPtr->interOff);

                /* put back to normal */
                argGRAFMAP->pixHeight--;
            }
        }
    } while(noDevice);

    if( argGRAFMAP->mapWinType == 0 )
    {
        /* done if mapWinType = 0 */
        done = NU_TRUE;
    }

    if( !done )
    {
        /* Initialize for bank management */
        argGRAFMAP->mapFlags |= mfBankMgr;
        if( argGRAFMAP->pixPlanes > 1 )
        {
            /* requires plane management */
            argGRAFMAP->mapFlags |= mfPlaneMgr;
        }

        /* invalidate bank (no active bank) */
        argGRAFMAP->mapWinYmin[0] = -1;
        argGRAFMAP->mapWinYmin[1] = -1;
        argGRAFMAP->mapWinYmax[0] = -1;
        argGRAFMAP->mapWinYmax[1] = -1;
    }

    NU_USER_MODE();

    return(rtnCode);
} /* end of SCREENI_InitBitmap() */

/***************************************************************************
* FUNCTION
*
*    SCREENI_InitGrafDriver
*
* DESCRIPTION
*
*    Initializes the passed grafMap primitive pointers using internal
*    data associated with the passed grafMaps devTech field.  It
*    also inits bank manager and device manager using internal data associated
*    associated with the passed grafMaps devMode field.
*
* INPUTS
*
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    Returns:         0 = ok
*                    -1 = can't resolve driver.
*
****************************************************************************/
INT32 SCREENI_InitGrafDriver(grafMap *argGRAFMAP)
{
    INT32 retCode = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();


    /* check that the devTech is in range */
    if( argGRAFMAP->devTech > 12 )
    {
        /* error */
        retCode = -1;
    }

    switch (argGRAFMAP->devTech)
    {

#ifdef INCLUDE_8_BIT
    case 8: /* 8-bit Memory */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M8BF_Fill8Bit;                /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M8BD_BlitMonoToSelf8Bit;      /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M8BL_ThinLine8Bit;            /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M8BD_SetPixelFor8Bit;         /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M8BD_GetPixelFor8Bit;         /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M8BD_BlitSelfToSelf8Bit;      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M8BD_BlitSelfToSelf8Bit;      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M8BD_BlitSelfToSelf8Bit;      /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif
#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M8BD_ReadImage8Bit;           /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M8BD_WriteImage8Bit;          /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif
        break;
#endif

#ifdef INCLUDE_1_BIT
    case 0:
    case 1: /* 1-bit LCD/Memory */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M1BF_Fill1Bit;                /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M1BD_BlitMonoToSelf1Bit;      /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M1BL_ThinLine1Bit;            /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M1BD_SetPixelFor1Bit;         /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M1BD_GetPixelFor1Bit;         /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M1BD_BlitSelfToSelf1Bit;      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M1BD_BlitSelfToSelf1Bit;      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M1BD_BlitSelfToSelf1Bit;      /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif
#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M1BD_ReadImage1Bit;           /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M1BD_WriteImage1Bit;          /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif

        break;
#endif

#ifdef INCLUDE_2_4_BIT /* case: 0 2 */
    case 0:
    case 2: /* 2/4-bit LCD/Memory */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M2B4F_Fill2_4Bit;             /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M2B4D_BlitMonoToSelf2_4Bit;   /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M2B4L_ThinLine2_4Bit;         /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M2B4D_SetPixelFor2_4Bit;      /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M2B4D_GetPixelFor2_4Bit;      /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M2B4D_BlitSelfToSelf2_4Bit;   /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M2B4D_BlitSelfToSelf2_4Bit;   /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M2B4D_BlitSelfToSelf2_4Bit;   /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif
#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M2B4D_ReadImage2_4Bit;        /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M2B4D_WriteImage2_4Bit;       /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif
        break;
#endif

#ifdef INCLUDE_16_BIT /* case: 0 9 */
    case 0:
    case 9: /* 16-Bit */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M16BF_Fill16Bit;              /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M16BD_BlitMonoToSelf16Bit;    /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M16BL_ThinLine16Bit;          /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M16BD_SetPixelFor16Bit;       /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M16BD_GetPixelFor16Bit;       /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M16BD_BlitSelfToSelf16Bit;    /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M16BD_BlitSelfToSelf16Bit;    /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M16BD_BlitSelfToSelf16Bit;    /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif
#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M16BD_ReadImage16Bit;         /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M16BD_WriteImage16Bit;        /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif

        break;
#endif

#ifdef INCLUDE_24_BIT /* case: 0 10 */
    case 0:
    case 10:    /* 24-Bit */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M24BF_Fill24Bit;              /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M24BD_BlitMonoToSelf24Bit;    /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M24BL_ThinLine24Bit;          /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M24BD_SetPixelFor24Bit;       /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M24BD_GetPixelFor24Bit;       /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M24BD_BlitSelfToSelf24Bit;    /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M24BD_BlitSelfToSelf24Bit;    /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M24BD_BlitSelfToSelf24Bit;    /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif

#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M24BD_ReadImage24Bit;         /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M24BD_WriteImage24Bit;        /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif

        break;
#endif

#ifdef INCLUDE_32_BIT /* case: 0 10 */
    case 0:
    case 11:    /* 32-Bit */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = M32BF_Fill32Bit;              /* primitive vector for fills    */
        argGRAFMAP->prBlit1S    = M32BD_BlitMonoToSelf32Bit;    /* primitive vector for mono-self blits */
#ifdef  THIN_LINE_OPTIMIZE
        argGRAFMAP->prLine      = M32BL_ThinLine32Bit;          /* primitive vector for thin lines */
#else
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
#endif  /* THIN_LINE_OPTIMIZE */
        argGRAFMAP->prSetPx     = M32BD_SetPixelFor32Bit;       /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = M32BD_GetPixelFor32Bit;       /* primitive vector for get pixel */
#ifndef NO_BLIT_SUPPORT
        argGRAFMAP->prBlitSS    = M32BD_BlitSelfToSelf32Bit;    /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = M32BD_BlitSelfToSelf32Bit;    /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = M32BD_BlitSelfToSelf32Bit;    /* primitive vector for self-mem  blits */
#else
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
#endif

#ifndef NO_IMAGE_SUPPORT
        argGRAFMAP->prRdImg     = M32BD_ReadImage32Bit;         /* primitive vector for read image */
        argGRAFMAP->prWrImg     = M32BD_WriteImage32Bit;        /* primitive vector for write image */
#else
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
#endif

        break;
#endif

    default:/* Null */
        argGRAFMAP->mapBankMgr  = iPrimErr;                     /* Ptr to bank manager function  */
        argGRAFMAP->mapPlaneMgr = PrimErr;                      /* Ptr to plane manager function */
        argGRAFMAP->mapAltMgr   = PrimErr;                      /* Ptr to alt manager function   */
        argGRAFMAP->mapDevMgr   = iPrimErr;                     /* Ptr to device manager list    */
        argGRAFMAP->prFill      = PrimErr;
        argGRAFMAP->prBlitSS    = PrimErr;                      /* primitive vector for self-self blits */
        argGRAFMAP->prBlit1S    = PrimErr;                      /* primitive vector for mono-self blits */
        argGRAFMAP->prBlitMS    = PrimErr;                      /* primitive vector for mem-self  blits */
        argGRAFMAP->prBlitSM    = PrimErr;                      /* primitive vector for self-mem  blits */
        argGRAFMAP->prRdImg     = PrimErr;                      /* primitive vector for read image */
        argGRAFMAP->prWrImg     = PrimErr;                      /* primitive vector for write image */
        argGRAFMAP->prLine      = PrimErr;                      /* primitive vector for thin lines */
        argGRAFMAP->prSetPx     = PrimErr;                      /* primitive vector for set pixel */
        argGRAFMAP->prGetPx     = lPrimErr;                     /* primitive vector for get pixel */
    }

    /* init the bank and device managers from internal tables */
    SCREENS_InitBankManager(argGRAFMAP);
    SCREENS_InitDeviceManager(argGRAFMAP);

    NU_USER_MODE();

    return(retCode);
} /* end  of GrafDriver() */

/***************************************************************************
* FUNCTION
*
*    SCREENI_DisplayDevMgr
*
* DESCRIPTION
*
*    Device manager for the display that is going to be used.  It manages
*    the enabling of the lcd or vga, the setting up of a hardware palette,
*    checks to see if the lcd or vga are enabled, and shutdown the lcd or
*    vga driver.
*
* INPUTS
*
*    grafMap *argGRAFMAP
*    INT32 dmFunc
*    INT32 dmParam
*
* OUTPUTS
*
*    Returns:         NU_SUCCESS
*
****************************************************************************/
static INT32 SCREENI_DisplayDevMgr(grafMap *argGRAFMAP, INT32 dmFunc, void *dmParam)
{
    INT32   status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    switch( dmFunc )
    {

    /* turns on the display and set up use */
    case DMWAKEUP:
        status = SCREENI_InitDisplay();
        break;

#if         (BPP < 16)
        
    /* read the palette based on the data in dmParam */
    case DMRPAL:
        SCREENI_Display_Device.display_read_palette(dmParam);
        break;

    /* write the palette based on the data in dmParam */
    case DMWPAL:
        SCREENI_Display_Device.display_write_palette(dmParam);
        break;

#endif      /* (BPP < 16) */
        
    /* check if the display is enabled */
    case DMQRY:
        if (SCREENI_Display_Enabled == NU_FALSE)
        {
            status = -1;
        }
        break;

    /* check if the display is to be turned off */
    case DMSHUTDOWN:
    default:
        status = DVC_Dev_Close(SCREENI_Display_Device.display_device);
        break;
    }

    NU_USER_MODE();

    return(status);
} /* end of INT32 SCREENI_DisplayDevMgr() */

/***************************************************************************
* FUNCTION
*
*    SCREENI_InitDisplay
*
* DESCRIPTION
*
*    Opens the display device.
*
* INPUTS
*
*    None
*
* OUTPUTS
*
*    status                                 Completion status.
*
****************************************************************************/
static STATUS  SCREENI_InitDisplay(VOID)
{
    STATUS              status;
    DV_DEV_HANDLE       display_ctrl_sess_hd;
    DV_DEV_LABEL        labels[2] = {{DISPLAY_LABEL}};
    INT                 label_cnt = 1;
    DV_DEV_ID           display_dev_id_list[CFG_NU_OS_DRVR_DISPLAY_MAX_DEVS_SUPPORTED];
    INT                 dev_id_cnt = CFG_NU_OS_DRVR_DISPLAY_MAX_DEVS_SUPPORTED;
    
    DV_IOCTL0_STRUCT    dev_ioctl0;
    CHAR                mw_config_path[REG_MAX_KEY_LENGTH-15];
    CHAR                reg_path[REG_MAX_KEY_LENGTH];
    
    /* Get all devices with these labels; DISPLAY_LABEL */
    status = DVC_Dev_ID_Get (labels, label_cnt, display_dev_id_list, &dev_id_cnt);
    
    /* Check if some display device was actually found. */
    if ((status == NU_SUCCESS) && (dev_id_cnt > 0))
    {
        /* Hardware initialization. Open the display device.
         * As there is support for only one display device so pass first ID only. */
        status = DVC_Dev_ID_Open(display_dev_id_list[0], labels, 1, &display_ctrl_sess_hd);

        /* Check if the device was opened successfully. */
        if (status == NU_SUCCESS)
        {
            /* Get IOCTL base address */
            memcpy(&dev_ioctl0.label, labels, sizeof(DV_DEV_LABEL));
            status = DVC_Dev_Ioctl(display_ctrl_sess_hd, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

            if (status == NU_SUCCESS)
            {
                /* Get frame buffer address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_FRAME_BUFFER,
                                        (void*)&(SCREENI_Display_Device.display_frame_buffer),
                                        sizeof(VOID *));
            }
            
#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)
            
            if (status == NU_SUCCESS)
            {
                /* Get pre-process hook function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_PREPROCESS_HOOK,
                                        (void*)&(SCREENI_Display_Device.display_pre_process_hook),
                                        sizeof(VOID *));
            }

#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */


#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

            if (status == NU_SUCCESS)
            {
                /* Get post-process hook function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_POSTPROCESS_HOOK,
                                        (void*)&(SCREENI_Display_Device.display_post_process_hook),
                                        sizeof(VOID *));
            }

#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef SMART_LCD

            if (status == NU_SUCCESS)
            {
                /* Get fill_rect hook function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_FILLRECT_HOOK,
                                        (void*)&(SCREENI_Display_Device.display_fill_rect_hook),
                                        sizeof(VOID *));
            }

            if (status == NU_SUCCESS)
            {
                /* Get set_pixel hook function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_SETPIXEL_HOOK,
                                        (void*)&(SCREENI_Display_Device.display_set_pixel_hook),
                                        sizeof(VOID *));
            }

            if (status == NU_SUCCESS)
            {
                /* Get get_pixel hook function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_GETPIXEL_HOOK,
                                        (void*)&(SCREENI_Display_Device.display_get_pixel_hook),
                                        sizeof(VOID *));
            }

#endif      /* SMART_LCD */

#if         (BPP < 16)

            if (status == NU_SUCCESS)
            {
                /* Get read palette function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_READ_PALETTE_FUNC,
                                        (void*)&(SCREENI_Display_Device.display_read_palette),
                                        sizeof(VOID *));
            }

            if (status == NU_SUCCESS)
            {
                /* Get write palette function address. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_WRITE_PALETTE_FUNC,
                                        (void*)&(SCREENI_Display_Device.display_write_palette),
                                        sizeof(VOID *));
            }
            
#endif      /* (BPP < 16) */

            if (status == NU_SUCCESS)
            {
                /* Get MW config path. */
                status  = DVC_Dev_Ioctl(display_ctrl_sess_hd,
                                        dev_ioctl0.base + DISPLAY_GET_MW_CONFIG_PATH,
                                        mw_config_path,
                                        64);
            }
            
            if (status == NU_SUCCESS)
            {
                /* Read various display device attributes and settings. */
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT32 (strcat(reg_path,"/screen_width"), &(SCREENI_Display_Device.display_screen_width));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT32 (strcat(reg_path,"/screen_height"), &(SCREENI_Display_Device.display_screen_height));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT16 (strcat(reg_path,"/h_dpi"), &(SCREENI_Display_Device.display_horizontal_dpi));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT16 (strcat(reg_path,"/v_dpi"), &(SCREENI_Display_Device.display_vertical_dpi));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT16 (strcat(reg_path,"/refresh_rate"), &(SCREENI_Display_Device.display_refresh_rate));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT8 (strcat(reg_path,"/planes_per_pix"), &(SCREENI_Display_Device.display_planes_per_pixel));
                strcpy(reg_path, mw_config_path);
                REG_Get_UINT8 (strcat(reg_path,"/bits_per_pix"), &(SCREENI_Display_Device.display_bits_per_pixel));
            }

            if (status == NU_SUCCESS)
            {
                /* Record the device handle for future reference. */
                SCREENI_Display_Device.display_device = display_ctrl_sess_hd;
            }
        }
    }
    
    /* Return the completion status. */
    return (status);
}
    
