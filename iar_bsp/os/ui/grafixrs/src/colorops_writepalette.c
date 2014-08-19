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
*  colorops_writepalette.c                                                   
*
* DESCRIPTION
*
*  Contains the function that writes the palette.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  WritePalette
*
* DEPENDENCIES
*
*  rs_base.h
*  devc.h
*  colorops.h
*  globalrsv.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/devc.h"
#include "ui/colorops.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*    WritePalette
*
* DESCRIPTION
*
*    Function WritePalette sets indexes from BGNINDX to ENDINDX inclusive.
*    The palette data is represented as three UINT16 s (16 bit) per index and ordered
*    red first, then green, blue last, for each index.  The value of each UINT16  is
*    0 = fully off, and FFFF = fully on.  Note that if the device supports less
*    than 16 bits per palette color, then it is the most significant bits in the
*    UINT16  that are used.
*
*    The plNum defines which palette table to modify, the default is 0.  If other
*    palette tables are supported, they are numbered 1..nindexes from bgnIdx to
*    endIdx inclusive.  The palette data is represented as three UINT16 s (16 bit)
*    per index and ordered red first, then green, blue last, for each index.  The
*    value of each UINT16  is 0 = fully off, and FFFF = fully on. Note that if the
*    device supports less than 16 bits per palette color, then it is the most
*    significant bits in the UINT16  that are used.
*
* INPUTS
*
*    INT32 plNum - Defines which palette table to modify, the default is 0.
*
*    INT32 bgnIdx - Index number in palette array table to begin writing.
*
*    INT32 endIdx - Index number in palette array table to end writing.
*
*    palData *palPtr - Pointer to palette.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID WritePalette(INT32 plNum, INT32 bgnIdx, INT32 endIdx, palData *palPtr)
{
    INT32  dmParam;
    INT32  devMgrPal;

    /* error value */
    INT16 grafErrValue;     

    /* temp pal data */
    argsPalInfo dmParamPal; 

    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* setup the device manager dmParam buffer */
    dmParamPal.palNum     = plNum;
    dmParamPal.palBgn     = bgnIdx;
    dmParamPal.palEnd     = endIdx;
    dmParamPal.palDataPtr = palPtr;

    /* device manager write palette function */
    dmParam = (INT32) &dmParamPal;
    devMgrPal = DMWPAL;

    /* call the device manager */
    grafErrValue = (INT16)(thePort->portMap->mapDevMgr(thePort->portMap, devMgrPal, dmParam));
    if( grafErrValue != 0 )
    {
        grafErrValue = c_WritePal +  c_InvDevFunc;

        /* report error */
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    /* Return to user mode */
    NU_USER_MODE();
}

