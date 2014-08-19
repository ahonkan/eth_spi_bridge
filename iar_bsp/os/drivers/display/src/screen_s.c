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
*       screen_s.c
*   
*   DESCRIPTION
*   
*       This file contains the Display (VGA or LCD) device generic functions.
*   
*   DATA STRUCTURES
*   
*       tLCD1 
*       tLCD2 
*       tLCD4 
*       tLCD8 
*       tLCD16
*       tLCD24
*       tLCD32
*       displayTable
*   
*   FUNCTIONS
*   
*       SCREENS_Resume
*       SetBitmap
*       SCREENS_CloseGrafDriver
*       SCREENS_InitBankManager
*       SCREENS_InitDeviceManager
*       SCREENS_QueryGraphics
*       SCREENS_CheckRes
*       SCREENS_FillDeviceTech
*       SCREENS_InitMemoryForGrafMap
*       SCREENS_InitRowTable
*       PrimErr
*       iPrimErr
*       lPrimErr
*       StubIDV
*       StubIDVI
*       BankStub
*       PlaneStub
*       DevStub
*       BankVESA
*       nuSegInfo
*       nuMapPhysAdr
*       CallmrInputMgr
*       SCREENS_Nop
*       
*   DEPENDENCIES
*   
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_drivers.h
*       screen_path.h
*       display_inc.h
*                
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/nu_ui.h"
#include "drivers/nu_drivers.h"
#include "drivers/screen_path.h"
#include "drivers/display_inc.h"

VOID INPUTT_InitInputDevTable(VOID);

#ifdef INCLUDE_1_BIT
DBPstruc tLCD1;         /* 1-bit   LCD  */
#endif

#ifdef INCLUDE_2_4_BIT
DBPstruc tLCD2;         /* 2-bit   LCD  */
DBPstruc tLCD4;         /* 4-bit   LCD  */
#endif

#ifdef INCLUDE_8_BIT
DBPstruc tLCD8;         /* 8-bit   LCD  */
#endif

#ifdef INCLUDE_16_BIT
DBPstruc tLCD16;        /* 16-bit  LCD  */
#endif

#ifdef INCLUDE_24_BIT
DBPstruc tLCD24;        /* 24-bit  LCD  */
#endif

#ifdef INCLUDE_32_BIT
DBPstruc tLCD32;        /* 32-bit  LCD  */
#endif

/* Display device table. Last entry is end of list marker. */
DspDevc displayTable[numDisplays+1];

/* Functions with local scope. */
static VOID SCREENS_CheckRes(grafMap *gmap);
static VOID SCREENS_FillDeviceTech(grafMap *gmap);
static INT32 DevStub(VOID);

#ifdef  USE_CURSOR

/***************************************************************************
* FUNCTION
*
*    SCREENS_Resume
*
* DESCRIPTION
*
*    Catches up the cursor tracking when the updating was held off by the
*    grafMap being busy.  This routine is called when a primitive operation
*    increments the grafMaps mapLock semaphore to zero, and the grafMaps
*    mfPending flag has been set. It is also called if the semaphore somehow
*    is incremented greater than 0.
*
* INPUTS
*
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID SCREENS_Resume(grafMap *argGRAFMAP)
{
    INT16  done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* check if we were called because of lock level problems */
    if( argGRAFMAP->mapLock > 0 )
    {
        /* yes */
        /* reset on error */
        argGRAFMAP->mapLock = 0;
        done = NU_TRUE;
    }

    if( !done )
    {
        /* lock grafMap */
        argGRAFMAP->mapLock--;
        do
        {
            /* clear deferred flag */
            argGRAFMAP->mapFlags &= ~mfPending;

            /* get the current input device X,Y */
            MovCursIDV(curInput->mrEvent.eventX, curInput->mrEvent.eventY);

        } while ( argGRAFMAP->mapFlags & mfPending );

        /* unlock grafMap */
        argGRAFMAP->mapLock++;
    }

    NU_USER_MODE();
}

#endif      /* USE_CURSOR */

/***************************************************************************
* FUNCTION
*
*    SCREENS_CloseGrafDriver
*
* DESCRIPTION
*
*    Frees the display memory allocation
*
* INPUTS
*
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    Returns NU_SUCCESS always.
*
****************************************************************************/
INT32 SCREENS_CloseGrafDriver(grafMap *argGRAFMAP)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    argGRAFMAP->mapDevMgr(argGRAFMAP, DMSHUTDOWN, 0);

    NU_USER_MODE();

    return(NU_SUCCESS);
}

/***************************************************************************
* FUNCTION
*
*    SCREENS_InitBankManager
*
* DESCRIPTION
*
*    Initializes the passed grafMaps bank/plane managers and map fields
*    using internal data associated with the grafMaps devMode code.  If
*    a device has no entry, BankStub and PlaneStub are used. BankStub
*    posts an error, as it should never be called.  PlaneStub is a
*    null routine, it just updates mapWinPlane.
*
* INPUTS
*
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID SCREENS_InitBankManager(grafMap *argGRAFMAP)
{
    DBPstruc *bnkTblPtr;
    INT32 i;
    INT16 done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* look up device code in Graphics Device Table */
    for( i = 0; i <= numDisplays && !done; i++ )
    {
        /* not found */
        if( displayTable[i].devcName == -1 )
        {
            argGRAFMAP->mapPlaneMgr = PlaneStub;
            done = NU_TRUE;
        }

        if( !done )
        {
            if( (argGRAFMAP->devMode == displayTable[i].devcName)
                &&
                (displayTable[i].bnkTblPtr != NU_NULL) )
                {
                /* Initialize the passed GrafMap for bank management */
                bnkTblPtr = displayTable[i].bnkTblPtr;
                argGRAFMAP->mapBankMgr = bnkTblPtr->bnkMgr;
                argGRAFMAP->mapPlaneMgr = bnkTblPtr->plnMgr;
        
                if( bnkTblPtr->wnType >= 0x80 )
                {
                    /* has altMgr */
                    argGRAFMAP->mapFlags |= mfAltMgr;
                    argGRAFMAP->mapAltMgr = bnkTblPtr->altMgr;
                }
        
                argGRAFMAP->mapWinType   = bnkTblPtr->wnType & 0x0f;
                argGRAFMAP->mapWinScans  = bnkTblPtr->wnSize;
                argGRAFMAP->mapWinOffset = bnkTblPtr->wnOff;
				
	            done = NU_TRUE;
            }
        }
    }

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SCREENS_InitDeviceManager
*
* DESCRIPTION
*
*    Initializes the passed grafMaps device manager and map handle fields
*    using internal data associated with the grafMaps devMode code.
*
* INPUTS
*
*    grafMap *argGRAFMAP
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID SCREENS_InitDeviceManager(grafMap *argGRAFMAP)
{
    INT16  i;
    INT16  done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();


    /* look up device code in Graphics Device Table */
    for( i = 0; i <= numDisplays && !done; i++ )
    {
        if( displayTable[i].devcName == -1 )
        {
            /* not found */
            argGRAFMAP->mapDevMgr = DevStub;
            done = NU_TRUE;
        }

        if( !done &&  (argGRAFMAP->devMode == displayTable[i].devcName) )
        {
            /* install  device manager */
            argGRAFMAP->mapDevMgr = displayTable[i].devTblPtr;
            done = NU_TRUE;
        }
    }

    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    SCREENS_CheckRes
*
* DESCRIPTION
*
*    Checks the pixResX and pixResY values for range.
*
* INPUTS
*
*    grafMap *gmap
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
static VOID SCREENS_CheckRes(grafMap *gmap)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( gmap->pixResX > 0x0FFF )
    {
        gmap->pixResX = 100;
    }

    if( gmap->pixResY > 0x0FFF )
    {
        gmap->pixResY = 100;
    }

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SCREENS_FillDeviceTech
*
* DESCRIPTION
*
*    Sets the device tech based on the number of bits per pixel.
*
* INPUTS
*
*    grafMap *gmap
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
static VOID SCREENS_FillDeviceTech(grafMap *gmap)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( gmap->pixPlanes > 1 )
    {
        /* devTech to planar */
        gmap->devTech = dtPlan;
    }
    else
    {
#if (defined(INCLUDE_1_BIT))
        /* devTech to mono */
        gmap->devTech = dtMono;
#elif (defined(INCLUDE_8_BIT))
        /* devTech to 8 bit */
        gmap->devTech = dt8Bit;
#elif (defined(INCLUDE_16_BIT))
        /* devTech to 16 bit */
        gmap->devTech = dt16Bit;
#elif (defined(INCLUDE_24_BIT))
        /* devTech to 24 bit */
        gmap->devTech = dt24Bit;
#elif (defined(INCLUDE_32_BIT))
        /* devTech to 32 bit */
        gmap->devTech = dt32Bit;
#else
        /* n bits fall through */
        gmap->devTech = dtBits;
#endif
    }

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SCREENS_InitMemoryForGrafMap
*
* DESCRIPTION
*
*    Initializes a memory grafMap.  This routine allocates rowtable(s)
*    and local memory bitmap.  Rowtable entries should be freed, as
*    should the rowtable (maptable entries) themselves upon either an
*    error return from this routine, or when the grafMap is no SIGNEDer
*    required.
*
* INPUTS
*
*    grafMap *gmap
*
* OUTPUTS
*
*    NU_SUCCESS   - this is Initializing Memory for the GrafMap is
*                   successful
*    c_BadDevTech - Loading the driver failed
*    c_OutofMem   - The allocation pool is out of memory
*    c_DivOflow   - Divide Overflow (result of divide more than 16 bits)
*
****************************************************************************/
INT32 SCREENS_InitMemoryForGrafMap( grafMap *gmap)
{
    INT32   rtSize;
    INT32   myPlane;
    INT32   myRow;
    SIGNED  pixelBytes;
    SIGNED  rowTablePtr;
    SIGNED  *rowTablePtrPtr;
    INT16   status = NU_SUCCESS;
    INT16   done   = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();


    /* check resX and resY */
    SCREENS_CheckRes(gmap);

    /* zero maptable entries */
    gmap->mapTable[0] = 0;
    gmap->mapTable[1] = 0;
    gmap->mapTable[2] = 0;
    gmap->mapTable[3] = 0;

    /* set managers and drivers */
    SCREENS_FillDeviceTech(gmap);

    /* initialize primitives; error return if problem loading driver */
    if( SCREENI_InitGrafDriver(gmap) != 0 )
    {
        status = c_BadDevTech;
    }

    if( status == NU_SUCCESS)
    {
        /* calculate the size necessary for one planes rowtable */
        rtSize = (gmap->pixHeight << 2);

        for( myPlane = 0; (myPlane < gmap->pixPlanes) && (status == NU_SUCCESS); myPlane++)
        {
            /* allocate each rowtable */
            status = GRAFIX_Allocation(&System_Memory, (VOID**) &gmap->mapTable[myPlane],
                                                rtSize, 0);

            if( gmap->mapTable[myPlane] == NU_NULL )
            {
                status = c_OutofMem;
            }
        }

        if( status == NU_SUCCESS)
        {
            /* set mapflags */
            gmap->mapFlags |= mfRowTabl;

            /* calculate the size in bytes necessary for each raster */
            pixelBytes = (((gmap->pixBits * gmap->pixWidth) + 7) >> 3);
            if( pixelBytes > 0xFFFF )
            {
                status = c_DivOflow;
            }

            if( status == NU_SUCCESS)
            {
                gmap->pixBytes = (INT16 ) pixelBytes;

                /* try to allocate the whole thing in one chunk */
                pixelBytes *= gmap->pixHeight;
                if( pixelBytes <= 0x7FFFFFFF )
                {
                    /* so far okay */
                    /* save plane size */
                    rtSize = (int) pixelBytes;
                    pixelBytes *= gmap->pixPlanes;
                    if( pixelBytes <= 0x7FFFFFFF )
                    {
                        /* yes we can */

#if         (GRAFMAP_MEM_ALIGN_SUPPORT == NU_FALSE)

                        status = GRAFIX_Allocation(&System_Memory, (VOID**) &rowTablePtr,
                                                    pixelBytes, 0);

#else

                        /* Allocate the aligned memory. */
                        status = NU_Allocate_Aligned_Memory(&GRAFMAP_MEMORY_POOL,
                                                            (VOID**) &rowTablePtr,
                                                            pixelBytes,
                                                            GRAFMAP_MEMORY_ALIGNMENT,
                                                            NU_NO_SUSPEND);
#endif

                        if( rowTablePtr != NU_NULL )
                        {
                            for( myPlane = 0; myPlane < gmap->pixPlanes; myPlane++)
                            {
                                *gmap->mapTable[myPlane] = (UINT8 *) rowTablePtr;
                                rowTablePtr += rtSize;
                            }

                            SCREENS_InitRowTable(gmap, 0, 0, 1);
                            status = NU_SUCCESS;
                            done   = NU_TRUE;
                        }
                        else
                        {
                            /* Null out the pointer since our memory allocation failed */
                            for( myPlane = 0; myPlane < gmap->pixPlanes; myPlane++)
                            {
                                *gmap->mapTable[myPlane] = NU_NULL;
                            }

                            /* record the error */
                            nuGrafErr ( status ,__LINE__, __FILE__);
                        }
                    }
                }

                if(!done && (status == NU_SUCCESS))
                {
                    /* can't allocate big enough chunk so allocate each raster
                    individually; since we have to do it a raster at a time, do the
                    rowtable(s) as well */
                    gmap->mapFlags &= ~mfRowTabl;   /* now it's interleaved */

                    for( myPlane = 0; (myPlane < gmap->pixPlanes) && (status == NU_SUCCESS); myPlane++)
                    {
                        rowTablePtrPtr = (SIGNED *) gmap->mapTable[myPlane];
                        for( myRow = 0; myRow < gmap->pixHeight; myRow++)
                        {
                            status = GRAFIX_Allocation(&System_Memory, (VOID**) &rowTablePtr,
                                                        gmap->pixBytes, 0);
                            if( rowTablePtr == NU_NULL )
                            {
                                status = c_OutofMem;
                            }

                            if(status == NU_SUCCESS)
                            {
                                *rowTablePtrPtr++ = rowTablePtr;
                            }
                        }
                    }
                }
            }
        }
    }

    NU_USER_MODE();
    return(status);
}

/***************************************************************************
* FUNCTION
*
*    SCREENS_InitRowTable
*
* DESCRIPTION
*
*    Initializes the rowtables specified in the grafmap data record
*    (BMAP) to point to each raster in the bitmap. The remaining procedure
*    parameters are used to determine how the rowtable addresses should
*    progress. interLeave specifies the delta between rowtable entries
*    to address the bitmap in sequence from top to bottom. interSeg and
*    interOff are the constants that should be added to offset each rowtable
*    entry to the next sequential raster.
*
*    Note: if the grafMaps pixHeight is greater than its mapWinScans value,
*    then the rowtables will repeat for each mapWinScans interval (each bank
*    will have identical rowtables).
*
* INPUTS
*
*    grafMap *argBitMap
*    INT32   argInrLve
*    INT32   argInrSeg
*    INT32   argInrSize
*
* OUTPUTS
*    None.
*
****************************************************************************/
VOID SCREENS_InitRowTable( grafMap *argBitMap, INT32 argInrLve, INT32 argInrSeg, INT32 argInrSize )
{
    SIGNED       baseAdr;
    SIGNED       scanAdr;
    SIGNED       rowAddr;
    INT32        rwBytes;
    UINT32       winSize;
    INT32        iPln;
    SIGNED       *rowTable;
    INT32        iRow;
    INT32        nRow;
    UINT32       wRow;
    INT32        intrlv;
    INT32        segInc;
    const SIGNED segOvrFlow = 65536;
    INT16        done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* just carry into high UINT16  */
    segInc = segOvrFlow;

    /* if interleave <= 0, make = 1 */
    if( argInrLve <= 0 )
    {
        argInrLve = 1;
    }

    /* get some things from the bitmap */
    nRow    = argBitMap->pixHeight;
    rwBytes = argBitMap->pixBytes;
    winSize = argBitMap->mapWinScans;

    if( winSize == 0xffff )
    {
        /* disable segment addition */
        segInc = 0;
    }

    for( iPln = argBitMap->pixPlanes - 1; iPln >= 0 && !done; iPln--)
    {
        rowTable = (SIGNED*) argBitMap->mapTable[iPln];
        baseAdr  = *rowTable;
        scanAdr  = 0;
        iRow     = 0;
        wRow     = 0;

        while (1 && !done)
        {
            rowAddr = baseAdr + scanAdr;

            /* for each interleave */
            for( intrlv = argInrLve; intrlv >0 && !done; intrlv--)
            {
                *rowTable = rowAddr;
                iRow++;
                wRow++;
                if( iRow>= nRow )
                {
                    done = NU_TRUE;
                }

                if( !done )
                {
                    rowTable += 1;
                    rowAddr = rowAddr + argInrSize;
                    if( wRow >= winSize )
                    {
                        wRow = 0;
                        scanAdr = 0;
                    }
                }
            }  /* next interleave */

            if( !done && wRow > 0 )
            {
                scanAdr = scanAdr + rwBytes;
                /* check for segment overflow */
                if( scanAdr >= segOvrFlow )
                {
                    baseAdr += segInc;
                    scanAdr -= segOvrFlow;
                }
            }
        }  /* still looping */
    }   /* next iPln */

    NU_USER_MODE();
}

/************************************************************************/
/* Stub functions that indicate that a load of a primitive function has */
/* failed or was not set up properly in initialization                  */
/************************************************************************/
VOID PrimErr(VOID)
{}


INT32 iPrimErr(VOID)
{
    return(0);
}

SIGNED lPrimErr(VOID)
{
    return(0);
}

/************************************************************************/

/************************************************************************/
/* Function StubIDV is a stub routine placed in all indirect vectors    */
/*   just in case they get called Inadvertently                         */
/************************************************************************/
VOID StubIDV(VOID)
{
    /* report error */
    nuGrafErr(c_IDVectNotSet,__LINE__, __FILE__);
}


INT32 StubIDVI(VOID)
{
    /* report error */
    nuGrafErr(c_IDVectNotSet,__LINE__, __FILE__);
    return(0);
}

/************************************************************************/


/********************************************************************************/
/* These are stubs used when there is either no function that needs to be called*/
/* or to indicate that you may have something set up incorrectly                */
/* This is usually dependent on the driver. These functions are never modified  */
/********************************************************************************/
INT32 BankStub(VOID)
{
    return(0);
}

VOID PlaneStub(VOID)
{}

static INT32 DevStub(VOID)
{
    return(0);
}

INT32 nuMapPhysAdr(SIGNED *physMem, INT32 blkSize, SIGNED *logMem)
{
    *logMem = *physMem;
    return(0);
}

VOID SCREENS_Nop(VOID)
{}

/*  END THE STUB functions                                                         */
/***********************************************************************************/

