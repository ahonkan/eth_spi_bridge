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
*  texts.c                                                      
*
* DESCRIPTION
*
*  Contains text support functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  SetFont
*  SetWorkGrafmap
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  texts.h
*  textd.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/texts.h"
#include "ui/textd.h"
#include "ui/gfxstack.h"

extern NU_MEMORY_POOL  System_Memory;
#include "ui/mwfonts.h"

static VOID SetWorkGrafmap(INT16 font_Height);

/***************************************************************************
* FUNCTION
*
*    SetFont
*
* DESCRIPTION
*
*    Function SetFont sets the specified font record, FONT, to be the current font.
*
* INPUTS
*
*    fontRcd *FONT - Pointer to font record..
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetFont( fontRcd *FONT)
{
    INT32   i;
    grafMap *bmapPtr;
    SIGNED fontPtr;
    SIGNED *adsRowTbl;
    INT16  ErrValue;
    UINT8  RSSIG[] = "NU__FONT";

    NU_SUPERV_USER_VARIABLES

#ifdef  INCLUDE_DEFAULT_FONT

    ErrValue = 0;

#endif  /* INCLUDE_DEFAULT_FONT */
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( FONT != 0 )
    {
        /* Verify NU__FONT signature */
        for( i=0; i< 8; i++)
        {
            if( FONT->fontSign[i] != RSSIG[i] )
            {
                ErrValue = c_SetFont + c_BadFontVer;
                nuGrafErr(ErrValue, __LINE__, __FILE__);
                
#ifdef  INCLUDE_DEFAULT_FONT
                
                FONT = (fontRcd *) imbFnt;
                
#endif  /* INCLUDE_DEFAULT_FONT */
                
                break;
            }
        }

        /* Check if font version meets requirement */
        if( ( (FONT->fontVer & 0x0F) != MAJORVER) || (FONT->fontVer < MINORVER) )
        {
            ErrValue = c_SetFont + c_BadFontVer;
            nuGrafErr(ErrValue, __LINE__, __FILE__);
            
#ifdef  INCLUDE_DEFAULT_FONT
            
            FONT = (fontRcd *) imbFnt;

#endif  /* INCLUDE_DEFAULT_FONT */
            
        }

#ifdef  INCLUDE_DEFAULT_FONT
        
        if (ErrValue == 0)
        {

#endif  /* INCLUDE_DEFAULT_FONT */
            
        /* At this point we have a validated font buffer */
        theGrafPort.txFont = FONT;
        thePort->txFont = FONT;

        fntVertTbl[alignTop] = 0;

        fntVertTbl[alignBaseline] = FONT->ascent - 1;

        fntVertTbl[alignBottom] = FONT->chHeight - 1;

        fntVertTbl[alignMiddle] = FONT->chHeight >> 1;

        /* Set current vertical alignment */
        fntVertAln = fntVertTbl[theGrafPort.txAlign.Y];

        /* Initialize the Bitmapped and bitmap font's grafMap */
        if( !(FONT->fontFlags & fontType) )
        {
            /* offset to font grafMap structure */
            bmapPtr   = (grafMap *) ((SIGNED) FONT + FONT->grafMapTbl);
            fontPtr   = (SIGNED) FONT + FONT->fontTbl;
            adsRowTbl = (SIGNED *) ((SIGNED) FONT + FONT->rowTbl);

            /* Install into font's grafMap record */
            bmapPtr->mapTable[0] = (UINT8 **) adsRowTbl;
            *adsRowTbl = fontPtr;

            bmapPtr->devTech = dtUser;

            /* Initialize a user defined grafMap */
            SCREENI_InitBitmap(cUSER, bmapPtr);

            /* Set flag indicating a linear bitmap for best speed */
            bmapPtr->mapFlags = (INT16) mfRowTabl;

            SCREENS_InitRowTable(bmapPtr, 0, 0, 0);

            /* Set the current bitmapped text draw vector */
            txtDrwIDV = (INT32 (*)()) BMAPF_TextBlitHorizontal;

            /* Initialize the workspace grafmap for syth facing */
            SetWorkGrafmap(FONT->chHeight);
        }
#ifdef      USE_STROKEDFONT
        /* InitStroked set the current stroked text draw vectors to initialize */
        else
        {
            txtDrwIDV = txtStrokeIDV;
            if( theGrafPort.txAlign.X == 0 )
            {
                txtAlnIDV = txtStrokeIDV;
            }
        }       
#endif      /* USE_STROKEDFONT */

        /* Set current text align vector */
        txtAlnIDV = txtDrwIDV;
        if( theGrafPort.txAlign.X != alignLeft )
        {
            txtAlnIDV = txtAPPIDV;
        }
        
#ifdef  INCLUDE_DEFAULT_FONT
        }
#endif  /* INCLUDE_DEFAULT_FONT */
        
    } /* if( FONT != 0 ) */

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SetWorkGrafmap
*
* DESCRIPTION
*
*    Function SetWorkGrafmap setups the workspace's grafmap and rowtable to allow 
*    blits from the workspace. The grafmap height is passed in font_Height, and
*    the width is set for the maximum allowable in the current workspace buffer.
*
* INPUTS
*
*    INT16 font_Height - The grafmap height is passed in font_Height.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID SetWorkGrafmap(INT16 font_Height)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* de-allocate old rowtable */
    mpWorkEnd += mpRowTblSz;

    /* Allocate this rowtable */
    mpRowTblSz = font_Height << 2;
    mpWorkEnd -= mpRowTblSz;

    /* mpWorkEnd now is a pointer to the workspace rowtable */
     workGrafMap.mapTable[0] = (UINT8 **) mpWorkEnd;
    *workGrafMap.mapTable[0] = (UINT8 *) mpWorkSpace;

    /* Compute size of remaining workspace */
    /* Per raster calculation */   
    workGrafMap.pixBytes  = ( (INT16) (mpWorkEnd - mpWorkSpace) ) / font_Height;
    workGrafMap.pixWidth  = workGrafMap.pixBytes << 3;
    workGrafMap.pixHeight = font_Height;

    /* Set as monoChrome - no driver */
    workGrafMap.pixBits   = 1;
    workGrafMap.pixPlanes = 1;
    /* Set devTech so no primitives loaded */
    workGrafMap.devTech   = dtUser;

    /* initialize the grafmap */
    SCREENI_InitBitmap(cUSER, &workGrafMap);

    /* Set flag indicating a linear bitmap for best speed */
    workGrafMap.mapFlags = mfRowTabl;

    SCREENS_InitRowTable(&workGrafMap, 0, 0, 0);

    /* Return to user mode */
    NU_USER_MODE();
}
