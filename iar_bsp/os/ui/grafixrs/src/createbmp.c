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
*  createbmp.c                                                  
*
* DESCRIPTION
*
*  This file contains the CreateBitmap, DestroyBitmap and CloseBitmap
*  functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CreateBitmap
*  DestroyBitmap
*  CloseBitmap
*
* DEPENDENCIES
*
*  rs_base.h
*  createbmp.h
*  global.h
*  memrymgr.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/createbmp.h"
#include "ui/global.h"
#include "ui/memrymgr.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    CreateBitmap
*
* DESCRIPTION
*
*    Function CreateBitmap creates an off-screen port and bitmap of a specified
*    width and height that matches the format of the currently active (normally
*    screen) bitmap.  aMEMTYPE specifies where the bitmap is to located in
*    conventional memory (cMEMORY).
*
*    CreateBitmap returns with a pointer to the new port to access the bitmap,
*    or a NULL pointer if the port and bitmap creation fails (eg. insufficient
*    memory for the bitmap size specified).
*
* INPUTS
*
*    INT32 aMEMTYPE - Supported: cMEMORY.
*    INT32 aWIDTH   - bitmap width in pixels.
*    INT32 aHEIGHT  - bitmap height in pixels.
*
* OUTPUTS
*
*    rsPort - Returns a pointer to the offscreen bitmap if successful.
*               Returns NULL for fail.
*
***************************************************************************/
rsPort *CreateBitmap(INT32 aMEMTYPE, INT32 aWIDTH, INT32 aHEIGHT)
{
    /* offscreen grafMap record */
    grafMap  *offBitmap;  

    /* offscreen grafPort record */
    rsPort *offScreen;    

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* only memory based bitmaps are support at this time */
    if( aMEMTYPE != cMEMORY)
    {
        offScreen = NU_NULL;

    }
    else
    {
        /* allocate offscreen grafMap data record */
        offBitmap = MEM_malloc(sizeof(grafMap));

        if(!offBitmap)
        {
            offScreen = NU_NULL;
        }
        else
        {
            /* allocate offscreen grafPort data record */
            offScreen = MEM_malloc(sizeof(rsPort));

            if(!offScreen)
            {
                GRAFIX_Deallocation(offBitmap);
            }
            else
            {
                /* Set the dimension of the offscreen bitmap to the aWIDTH & aHEIGHT */
                offBitmap->pixWidth  = aWIDTH;
                offBitmap->pixHeight = aHEIGHT;

                /* Set the device color type equal to the screen bitmap's */
                offBitmap->pixBits   = theGrafPort.portMap->pixBits;
                offBitmap->pixPlanes = theGrafPort.portMap->pixPlanes;
                offBitmap->pixResX   = theGrafPort.portMap->pixResX;
                offBitmap->pixResY   = theGrafPort.portMap->pixResY;

                /* Init the new memory bitmap */
                if( SCREENI_InitBitmap( aMEMTYPE, offBitmap) != 0 )
                {
                    CloseBitmap( offBitmap);
                    GRAFIX_Deallocation(offScreen);
                    GRAFIX_Deallocation(offBitmap);
                    offScreen = NU_NULL;
                }
                else
                {
                    /* initialize the offscreen port */
                    InitPort( offScreen);
                    PortBitmap( offBitmap);
                    PortSize( aWIDTH, aHEIGHT);
                    ClipRect( &offScreen->portRect);
                }
            }
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* the offscreen port and bitmap are now ready */
    return( offScreen );
}

/***************************************************************************
* FUNCTION
*
*    DestroyBitmap
*
* DESCRIPTION
*
*    Function DestroyBitmap frees a port and bitmap previously created with CreateBitmap.
*
* INPUTS
*
*    rsPort *offPort - pointer to the offscreen bitmap to destroy.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DestroyBitmap(rsPort *offPort)
{
    /* offscreen grafMap record */
    grafMap *offBmap; 

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* save pointer to the bitmap */
    offBmap = offPort->portMap;

    /* Close the bitmap */
    CloseBitmap( offBmap );

    /* Deallocate memory */
    GRAFIX_Deallocation( offPort );
    GRAFIX_Deallocation( offBmap );

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    CloseBitmap
*
* DESCRIPTION
*
*    Function CloseBitmap deallocates memory assigned to the grafMap.  It
*    returns 0 if successful or a GrafError style error code (and posts an error)
*    if not.
*
* INPUTS
*
*    rsPort *offPort - Pointer to the offscreen bitmap to destroy.
*
* OUTPUTS
*
*    INT32             - Returns 0 if successful.
*                      - Returns GrafError style error code (and posts an error).
*
***************************************************************************/
INT32 CloseBitmap( grafMap *argCGMAP)
{
    /* do not free rows flag */
    INT32 norows;     

    /* error return code */
    INT32 retcode;    
    INT32 myPlane;
    INT32 myRaster;
    UINT32 *rowTablePtr;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* clear return code */
    retcode = 0;    

    /* release driver */
    if( SCREENS_CloseGrafDriver(argCGMAP) )
    {
        retcode = c_CloseBit + c_BadDevTech;

        /* report error */
        nuGrafErr( (INT16) retcode, __LINE__, __FILE__); 
    }

    /* if one of our 'special' devices, do special close routine */
    switch (argCGMAP->devMode)
    {
    /* close a memory grafMap */
    case cMEMORY:   

        /* this flag is used to skip the portion of the loop which frees raster 
           lines, so that only rowtable arrays are freed */
        norows = 0; 
        for (myPlane = 0; myPlane < argCGMAP->pixPlanes; myPlane++)
        {
            if( argCGMAP->mapTable[myPlane] == NU_NULL )
            {
                break; 
            }

            /* check flag */
            if( norows == 0 )
            {
                /* free the rasters */
                rowTablePtr = (UINT32 *)*argCGMAP->mapTable[myPlane];
                for( myRaster = 0; myRaster < argCGMAP->pixHeight; myRaster++ )
                {
                    if( rowTablePtr == NU_NULL )
                    {
                        norows = 1;
                        break;
                    }

                    GRAFIX_Deallocation(rowTablePtr);

                    /* if it's a linear grafMap, then it was allocated in
                       one big chunk */
                    if( argCGMAP->mapFlags & mfRowTabl )
                    {
                        norows = 1;
                        break;
                    }

                    rowTablePtr++;
                }
            }

            /* free this planes rowtable */
            GRAFIX_Deallocation( argCGMAP->mapTable[myPlane] );
        }
        break;

    /* user device, hands off rowtable */
    case cUSER: 
        break;

    default:
        /* free rowtable for each plane. */
        for( myPlane = 0; myPlane < argCGMAP->pixPlanes; myPlane++ )
        {
            /* check for null, exit on any null pointer */
            if( argCGMAP->mapTable[myPlane] == NU_NULL )
            {
                break; 
            }
            GRAFIX_Deallocation( argCGMAP->mapTable[myPlane] );

            /* check if using same rowtable for other planes */
            if( argCGMAP->mapTable[myPlane] ==
                argCGMAP->mapTable[myPlane + 1])
            {
                break; 
            }
        }

    } /* switch (argCGMAP->devMode) */

    /* Return to user mode */
    NU_USER_MODE();

    return(retcode);
}
