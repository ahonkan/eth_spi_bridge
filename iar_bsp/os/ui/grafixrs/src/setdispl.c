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
*  setdispl.c                                                   
*
* DESCRIPTION
*
*  Screen display functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  SetDisplay
*  BorderColor
*
* DEPENDENCIES
*
*  rs_base.h
*  devc.h
*  setdispl.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/devc.h"
#include "ui/setdispl.h"
#include "ui/gfxstack.h"

/* Default palette settings */
static const UINT16 DefPal[16][4] =
{
    {0x0000, 0x0000, 0x0000, 0x0000},     /* Color 0, black */
    {0xA800, 0x0000, 0x0000, 0x0000},     /* Color 1, red */
    {0x0000, 0xA800, 0x0000, 0x0000},     /* Color 2, green */
    {0xA800, 0x5400, 0x0000, 0x0000},     /* Color 3, brown */
    {0x0000, 0x0000, 0xA800, 0x0000},     /* Color 4, blue */
    {0xA800, 0x0000, 0xA800, 0x0000},     /* Color 5, magenta */
    {0x0000, 0xA800, 0xA800, 0x0000},     /* Color 6, cyan, turquoise */
    {0xA800, 0xA800, 0xA800, 0x0000},     /* Color 7, white, light grey */
    {0x5400, 0x5400, 0x5400, 0x0000},     /* Color 8, dark grey */
    {0xFFFF, 0x5400, 0x5400, 0x0000},     /* Color 9, light red */
    {0x5400, 0xFFFF, 0x5400, 0x0000},     /* Color A, light green */
    {0xFFFF, 0xFFFF, 0x5400, 0x0000},     /* Color B, yellow */
    {0x5400, 0x5400, 0xFFFF, 0x0000},     /* Color C, light blue */
    {0xFFFF, 0x5400, 0xFFFF, 0x0000},     /* Color D, light magenta */
    {0x5400, 0xFFFF, 0xFFFF, 0x0000},     /* Color E, light cyan */
    {0xFFFF, 0xFFFF, 0xFFFF, 0x0000}      /* Color F, bright white */
};

#define PalWhite DefPal[15][0]

/***************************************************************************
* FUNCTION
*
*    SetDisplay
*
* DESCRIPTION
*
*    Function SetDisplay enables the current ports grafMap to be displayed.
*
*       PAGE specifies the associated display screens:
*       bit FEDCBA98 76543210
*           -------- xxxxxxxx   page number
*           -------x --------   0 = text page  1 = graphics page
*
*    When enabling the grafMap to display graphics, palette operations are
*    performed to set the first 16 color indexes to the EGA default and the
*    maximum color index to white.
*
* INPUTS
*
*    INT32 argPAGE - Specifies the associated display screens.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetDisplay( INT32 argPAGE)
{
    INT16       Done = NU_FALSE;
    argsPalInfo dmPalParam;
    INT16       dmFunc;
    SIGNED      dmParam;
    SIGNED      dmPage;
    grafMap     *currentPortMap;
    INT32       colors;
    INT16       grafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* get the current ports grafmap */
    currentPortMap = thePort->portMap;

    /* get page number only */
    dmPage = argPAGE & 0xff; 

    /* text or graphics? */
    if( argPAGE & 0x0100 )
    {
        /* set graphics mode */
        /* are we already in graphics mode? */
        if( (currentPortMap->mapFlags & mfDspGraf) == 0 )
        {
            /* no, set it */
            /* devMgr set graphics function */
            dmFunc = DMGRAFIX;
            dmParam = 0;

            /* call the device manager */
            currentPortMap->mapDevMgr(currentPortMap, dmFunc, dmParam);

            /* set grafMaps display status bit to indicate 'in graphics mode' */
            currentPortMap->mapFlags |= mfDspGraf;

            /* set default palette */
            dmPalParam.palNum     = 0;
            dmPalParam.palBgn     = 0;
            dmPalParam.palEnd     = 15;
            dmPalParam.palDataPtr = (palData *) &DefPal[0][0];

            /* call the device manager with write palette function */
            currentPortMap->mapDevMgr(currentPortMap, DMWPAL, &dmPalParam);

            /* set max color to white */
            if( currentPortMap->pixPlanes <= 1 )
            {
                colors = (1 << currentPortMap->pixBits) - 1;
            }
            else
            {
                colors = (1 << currentPortMap->pixPlanes) - 1;
            }

            dmPalParam.palBgn     = colors;
            dmPalParam.palEnd     = colors;
            dmPalParam.palDataPtr = (palData *) &PalWhite;

            currentPortMap->mapDevMgr(currentPortMap, DMWPAL, &dmPalParam);

            /* do we also need to flip the page? */
            if( dmPage == 0 )
            {
                Done = NU_TRUE;
            }

        } /* if( (currentPortMap->mapFlags & mfDspGraf) == 0 ) */
        else
        {
            /* flip to requested page */
            dmFunc = DMFLIP;
            dmParam = dmPage;

            if( currentPortMap->mapDevMgr(currentPortMap, dmFunc, dmParam) )
            {
                /* error if here */
                grafErrValue = c_SetDispl + c_InvDevFunc;
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            }
            Done = NU_TRUE;
        }
    } /* if( argPAGE & 0x0100 ) */
    else    
    {
        /* set text mode */
        dmFunc = DMTEXT;
        dmParam = dmPage;

        currentPortMap->mapDevMgr(currentPortMap, dmFunc, dmParam);

        /* clear grafMaps display status bit to indicate 'in text mode' */
        currentPortMap->mapFlags &= ~mfDspGraf;

        /* done if not a banked device */
        if( !(currentPortMap->mapFlags & mfBankMgr) )
        {
            Done = NU_TRUE;
        }

    } /* else */

    if( !Done )
    {
        /* invalidate bank (no active bank) */
        currentPortMap->mapWinYmin[0] = -1;
        currentPortMap->mapWinYmin[1] = -1;
        currentPortMap->mapWinYmax[0] = -1;
        currentPortMap->mapWinYmax[1] = -1;
    }

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    BorderColor
*
* DESCRIPTION
*
*    Function BorderColor allows you set the border (overscan) color for the
*    area of the screen outside the bit image area on adapters which support
*    such a feature.
*
* INPUTS
*
*    INT32 argColor - Color of the border overscan.
*
* OUTPUTS
*
*    INT32 minDistIndex - Returns closest index.
*
***************************************************************************/
VOID BorderColor(INT32 argColor)
{
    SIGNED dmParam;
    INT16  devMgrScan;
    INT16  grafErrValue; 

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dmParam = argColor;

    /* device manager set overscan color function */
    devMgrScan = DMOVERSCAN;    

    /* call the device manager */
    grafErrValue = thePort->portMap->mapDevMgr(thePort->portMap, devMgrScan,dmParam);
    if( grafErrValue != 0 )
    {
        grafErrValue = c_BorderCo +  c_InvDevFunc;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    /* Return to user mode */
    NU_USER_MODE();
}
