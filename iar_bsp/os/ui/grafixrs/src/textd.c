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
*  textd.c                                                      
*
* DESCRIPTION
*
*  Contains the API RS_Text_Draw function and the text draw supporting 
*  functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Text_Draw
*  TXTD_GetUnicodeCharWidth
*  TXTD_SetWCHARRowPointers
*  TXTD_DrawCharW
*  TXTD_DrawStringW
*
* DEPENDENCIES
*
*  rs_base.h
*  textd.h
*  str_utils.h
*  gfxstack.h
*  rsfonts.h (USE_UNICODE)
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/textd.h"
#include "ui/str_utils.h"
#include "ui/gfxstack.h"

#ifdef USE_UNICODE
#include "ui/rsfonts.h"
#endif

BOOLEAN strAsChar = NU_FALSE;

#ifdef USE_UNICODE

/* Functions with local scope. */
static UINT16 TXTD_GetUnicodeCharWidth(fontRcd  *theFont,UNICHAR uniChar);  
static INT32 TXTD_SetWCHARRowPointers(fontRcd  *theFont, INT32 iRow);
static VOID TXTD_DrawCharW(UNICHAR uniChar,INT32 index, INT32 count, INT16 charSize);

#endif /* USE_UNICODE */

/***************************************************************************
* FUNCTION
*
*    RS_Text_Draw
*
* DESCRIPTION
*
*    The API function RS_Text_Draw is used for drawing characters and text strings. 
*    A default FONT is set up at initialization. For a different FONT, call 
*    SetFont( fontRcd *theFont) first.
*
* INPUTS
*
*    VOID* strings         - This is the pointer for all types of strings text and 
*                            characters that will be drawn.
*
*    DrawTextType textType - This is the width of the characters to draw and also 
*                            if it is a string (STR),(STR16), or (STRWIDE), char (CH),
*                            (CH16), or (CHWIDE) or TEXT (TXT), (TXT16), or (TXTWIDE).
*                
*    INT32 index           - This is the index of the string, this is set at 0 or if using 
*                            TEXT (TXT),(TXT16),or (TXTWIDE) then it is user defined, 
*                            set only if TEXT (TXT),(TXT16),or (TXTWIDE)
*
*    INT32 count           - This is the count of characters, 
*                            Set to 1 for char/char16/charw.
*                            Set to 127 for string(STR)/string16(STR16)/stringw(STRWIDE).  
*                            If TEXT it is user defined, set only if TEXT (TXT),(TXT16),
*                            or (TXTWIDE).
*
* OUTPUTS
*
*    STATUS                - Returns NU_SUCCESS, if successful.
*
***************************************************************************/
STATUS RS_Text_Draw( VOID * strings, DrawTextType textType, INT32 index, INT32 count)
{
    STATUS status    = ~NU_SUCCESS;
    INT16  charSize;
    INT32  savTxTerm;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    (VOID)(INT16)status;
    /* If the text type is simple character, string, or Text then
       define charSize as 0 because it will not be needed */
    if( (textType == CH) || (textType == STR) || (textType == TXT) )
    {
        charSize = 0;
    }
    else
    {
        charSize = 1;
    }

    /* The index and count are sent in as a parameter but if they are not what 
       can be used with the different character, string or text then we will set
       them to the correct values here */
    switch(textType)
    {
        case CH:
        case CH16:
        case CHWIDE:

            /* All character CH, CH16, and CHWIDE must have a 0 index and a count of 1. 
               These will only do one character at a time no matter what. */
            index = 0;
            count = 1;

            /* Save the txTerm so that it can be restored after the character 
               or characters have been written */        
            savTxTerm = theGrafPort.txTerm;

            /* Set the txTerm to the not of the character or characters pointer */ 
            theGrafPort.txTerm = ~( (INT32) strings );
            
            break;

        case STR:
        case STR16:
        case STRWIDE:
            /* All character STR, STR16, and STRWIDE must have a 0 index and a count of 127. 
               These will only 0 through 127 characters in a string. */
            index = 0;
            count = 127;
            break;

        case TXT:
        case TXT16:
        case TXTWIDE:
            /* All of these can be user defined, so whatever is passed in can be used for
               TXT, TXT16, and TXTWIDE */
            break;

        default:
            /* If the textType is invalid, then instead if tailing we will set it to 
               character type.  This should not happen, but just in case. */
            index = 0;
            count = 1;
            textType = CH;
            break;
    }
#ifdef USE_UNICODE
    if((textType == STRWIDE) || (textType == STR16) 
	|| (textType == TXTWIDE) || (textType == TXT16) 
	|| (textType == CHWIDE) || (textType ==CH16)) 
    {
        TXTD_DrawStringW(strings, index, count ,charSize);
    }
    else
#endif
      /* Now write to display or memory. */
      txtAlnIDV( strings, index, count, charSize);

    /* Reset the txTerm to the initial txTerm */
    if( (textType == CH) || (textType == CH16) || (textType == CHWIDE) )
    {
        theGrafPort.txTerm = savTxTerm;
    }

    /* This should always return success because we have set in the default to allow for 
       misuse, it will just not appear one would have thought. */
    status = NU_SUCCESS;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(status);
}

#ifdef USE_UNICODE
/***************************************************************************
* FUNCTION
*
*    TXTD_GetUnicodeCharWidth
*
* DESCRIPTION
*
*    The function used for getting the UNICODE character width
*
* INPUTS
*
*    fontRcd  *theFont
*    UNICHAR uniChar
*
* OUTPUTS
*
*    UINT16  - Returns width of the UNICHAR.
*
***************************************************************************/
static UINT16 TXTD_GetUnicodeCharWidth(fontRcd  *theFont, UNICHAR uniChar)  
{
    /* translated glyph-index */
    UINT16     glyphIndex = 0;    

    /* binary search indexes */
    INT32      i = 0;         

    /* pointer to character mapping table */
    charMap   *cmapTable;         

    /* index of last cmap record used */
    INT32      lastCMap;          

    /* number of character map table records */
    INT32      numCMaps;          
    INT16      done = NU_FALSE;

    /* if the pointer to the font record is NULL, there's nothing to do */
    if ( theFont == NU_NULL )
    {
        done = NU_TRUE;
    }

    if( !done )
    {
        /* if there's no character mapping table we can't translate this character */
        if ( theFont->cmapTbl == 0 )
        {
            done = NU_TRUE;
        }
    }

    if( !done )
    {
        /* compute pointer to the character mapping table */
        cmapTable = (charMap *)( (UINT8 *)theFont + (UINT16)theFont->cmapTbl );

        /* get the number of records in the font's character mapping table */
        numCMaps  = theFont->numCMaps;

        /* get the index of the last character map record that matched a character */
        /* (initially 0) */
        lastCMap  = (INT32)(theFont->offwidTbl[0]);  

        /* quick check to see if the character falls within the last prior map record */
        if ( _InRange(cmapTable[lastCMap].charStart, uniChar, cmapTable[lastCMap].charEnd)   )
        {   
            /* character fall within this record, compute its glyphIndex */
            glyphIndex = cmapTable[lastCMap].glyphStart + ( uniChar - cmapTable[lastCMap].charStart );
        }

        /* if the character is not in the last prior record,         */
        /* we need to search through the character map table for it  */
        /* if not in s_MWCmapTable[0] or s_MWCmapTable[s_MWCmapLast] */
        else if ( numCMaps > 1 ) 
        {

			while( i < numCMaps)
            {

                /* if it's within the midpoint record, we're done */
                if (  _InRange( cmapTable[i].charStart, uniChar, cmapTable[i].charEnd )  )
                {   
                    /* character fall within this record, compute its glyphIndex */
                    glyphIndex = cmapTable[i].glyphStart + 
                                ( uniChar - cmapTable[i].charStart );

                    /* save the cmapTable index in the font record */
                    theFont->offwidTbl[0] = i;  

                    /* save the glyphRow in fontRcd */
                    theFont->offwidTbl[1] = cmapTable[i].glyphRow; 
                    break;
                }

				i++;

            } /* end:while */

        } /* end:else if ( numCMaps > 1 ) */
    }
    return ( glyphIndex );
} 

/***************************************************************************
* FUNCTION
*
*    TXTD_SetWCHARRowPointers
*
* DESCRIPTION
*
*    Set font bitmap character row pointers
*
* INPUTS
*
*    fontRcd  *theFont
*    INT32 iRow
*
* OUTPUTS
*
*    INT32  - Returns success or failure of setting the row pointer.
*
***************************************************************************/
static INT32 TXTD_SetWCHARRowPointers(fontRcd  *theFont, INT32 iRow)
{
    INT32     i;

    /* character row pixel height */
    INT32     glyphHeight;           

    /* bytes between bitmap raster lines */
    INT32     rowBytes;              

    /* pointer to font bitmap grafMap record */
    grafMap   *grafmap;              

    /* pointer to font rowTable */
    UINT8     **rowTable;            

    /* pointer to font bitmap pixel surface */
    UINT8     *surface;              

    /* function return code */
    INT32     rtnResult = NU_SUCCESS;
    INT16     done      = NU_FALSE;

    /* if the pointer to the font record is NULL, there's nothing to do */
    if ( theFont == NU_NULL )
    {
        rtnResult = -1;
        done = NU_TRUE;
    }

    if( !done )
    {
        /* if iRow is different from the currently active row, adjust the bitmap rowTable */
        if ( iRow != theFont->offwidTbl[2] )
        {
            /* set pointers to font's grafmap, rowTable and bitmap pixel surface */
            grafmap  = (grafMap *)((UINT8 *)theFont + theFont->grafMapTbl);
            rowTable = (UINT8 **)((UINT8 *)theFont + theFont->rowTbl);
            surface  = (UINT8 *)((UINT8 *)theFont + theFont->fontTbl);

            /* get the pixel height of each character row */
            glyphHeight = theFont->chHeight;

            /* get the number of bytes in each bitmap raster line */
            rowBytes = grafmap->pixBytes;

            /* adjust the surface pointer to the start of the character row */
            surface += iRow * glyphHeight * rowBytes;

            /* adjust the font rowTable ptrs to point to the new character row */
            for ( i = 0; i < glyphHeight; i++ )
            {
                /* store the ptr to this raster line */
                rowTable[i] = surface;  

                /* increment to the next raster line */
                surface    += rowBytes; 
            }

            /* save the new active character row value */
            theFont->offwidTbl[2] = iRow; 
        }
    }
    return ( rtnResult );

} /* TXTD_SetWCHARRowPointers */

/***************************************************************************
* FUNCTION
*
*    TXTD_DrawCharW
*
* DESCRIPTION
*
*    This function draws the UNICODE wide character. To translate the 
*    Unicode character code to a font glyphIndex that can then be used to 
*    call the function txtAlnIDV.
*
* INPUTS
*
*    UNICHAR uniChar
*    INT32   index
*    INT32   count
*    INT16   charSize
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
static VOID TXTD_DrawCharW(UNICHAR uniChar, INT32 index, INT32 count, INT16 charSize)
{
	/* pointer to current font */
    fontRcd     *theFont;          

    /* translated glyph-index */
    UINT16      glyphIndex;        
    INT32       saveActiveCRow;    

	/* get the pointer to the currently active font */
    theFont = (fontRcd *)(theGrafPort.txFont);
    
    /* remember the active font bitmap character row on entry */
    saveActiveCRow = theFont->offwidTbl[2];

    /* translate the Unicode character to a font glyph-index */
    glyphIndex = TXTD_GetUnicodeCharWidth( theFont, uniChar );
    
	/* theFont_lastCRow is updated by mwFont_GetGlyphW() */

    /* if the character is not-defined, draw the font's "missing symbol" */
    if(glyphIndex == 0)
    {
        /* switch to the font bitmap character row for chBad glyph*/
        if(theFont->offwidTbl[2] != 0)
		{
			TXTD_SetWCHARRowPointers(theFont, 0);
		}

        /* Now write to display or memory. */
        txtAlnIDV(&(theFont->chBad), index, count, charSize);
    }

    /* if the character is defined, draw the character glyph */
    else
    {
        /* switch to the font bitmap character row for this glyph */
        if(theFont->offwidTbl[2] != theFont->offwidTbl[1])
		{
			TXTD_SetWCHARRowPointers(theFont, theFont->offwidTbl[1]);
		}

		glyphIndex += theFont->fontMin;

		txtAlnIDV( &glyphIndex, 0, 1,charSize);

    }

    /* if different, switch back to the entry character row  */
    /* (needed if DrawChar() or DrawString() is called next) */
    if(theFont->offwidTbl[2] != saveActiveCRow)
	{        
		TXTD_SetWCHARRowPointers( theFont, saveActiveCRow );
	}


} /* mwPort_DrawCharW() */


/***************************************************************************
* FUNCTION
*
*    TXTD_DrawStringW
*
* DESCRIPTION
*
*    This function draws the UNICODE wide string
*
* INPUTS
*
*    UNICHAR *uniString
*    INT32 index
*    INT32 count
*    INT16 charSize
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
INT32 TXTD_DrawStringW(UNICHAR *uniString, INT32 index, INT32 count, INT16 charSize)  
{
    INT32       i,j;

    /* pointer to current font */
    fontRcd     *theFont;          

    /* translated glyph-index */
    UINT16      glyphIndex;        

    /* bitmap character row for 1st char */
    INT32       cRow; 
	INT32 oldX;
	INT32 tempX;
    INT32 origX;
	
	INT32       stringWidth = 0;

    /* character string is contained in */
    /* a single bitmap character row. */
    INT16       singleCRow;        
    INT16       done  = NU_FALSE;

    UNICHAR     *temp_string;

    temp_string = &uniString[index];

    /* get the pointer to the currently active font */
    theFont = (fontRcd *)(theGrafPort.txFont);

    origX = LocX;

	/* When this case occurs character alignment needs to be */
	/* treated differently in BMAPF_TextAlignHorizontalCR.   */
	strAsChar = NU_TRUE;


    /* if this is a NULL string, just exit */
    if ( temp_string[0] == 0 )
    {
        done = NU_TRUE;
    }

    if( !done )
    {

        /* Hide the pen */
		theGrafPort.pnLevel-- ;  

        /* process the first character */
        /* translate the Unicode character to its font glyph-index */
        glyphIndex = TXTD_GetUnicodeCharWidth( theFont, temp_string[0] );


        /* if the glyphIndex is defined, store the DrawString16() character code */
        if ( glyphIndex > 0 )
        {

            glyphIndex += theFont->fontMin;

            /* save the character row */
            cRow = theFont->offwidTbl[1]; 

            /* Now write to display or memory. */
            txtDrwIDV( &glyphIndex, 0, 1,charSize);
        }

        /* if the glyph-index is undefined, store the font's "missing symbol" */
        else
        {
			glyphIndex += 1;

            /* save the character row */
            cRow = theFont->offwidTbl[1]; 

            /* Now write to display or memory. */
            txtDrwIDV( &glyphIndex, 0, 1,charSize);

        }

        if ( !done )
        {
            /* so far everything is in a single bitmap character row */
            singleCRow = NU_TRUE;

            /* translate the Unicode string to a font glyph-index string */
            for ( i = 1; (i < count) && (i < (INT32)STR_str_len(temp_string)); i++ )
            {
                /* if we're at a NULL character, we're at the end of the string */
                if ( temp_string[i] == 0 )
                {
                    /* terminate the glyph-string */
                    break;
                }

                /* translate the Unicode character to its font glyph-index */
                glyphIndex = TXTD_GetUnicodeCharWidth( theFont, temp_string[i] );

                /* if the glyphIndex is defined, store the DrawString16() character code */
                if ( glyphIndex > 0 )
                {
                    glyphIndex += theFont->fontMin;

                    /* check to see if this character is in the same bitmap row */
                    if ( cRow != theFont->offwidTbl[1] )
                    {   
                        /* if it's not in the same row, we can't use DrawString16() */
                        singleCRow = NU_FALSE;

                        /* for ( i = 0;... */
                        break; 
                    }
                }

                /* if the glyph-index is undefined, store the font's "missing symbol" */
                else /* if ( glyphIndex == 0 ) */
                {
                   
					glyphIndex += 1;

					/* save the character row */
					cRow = theFont->offwidTbl[1]; 

			        /* check to see if this character is in the same bitmap row */
                    if ( cRow != 0 )
                    {
                        singleCRow = NU_FALSE;

                        /* for ( i = 0;... */
                        break; 
                    }
                }

                /* Now write to display or memory. */
               	txtDrwIDV( &glyphIndex, 0, 1, charSize);
            }

            /* Restore the pen */
			theGrafPort.pnLevel++ ; 

             /* switch to the font bitmap character row for the first glyph */
            if ( theFont->offwidTbl[2] != cRow )
            {
                TXTD_SetWCHARRowPointers( theFont, cRow );
            }

            stringWidth = LocX - origX;

            /* if the characters are in different bitmap character       */
            /* rows, we have to draw the string one character at a time. */
            /* if ( singleCRow == FALSE ) */
            if ( singleCRow == NU_FALSE )
		    {
				
				/* Loop through the string and find the width of each character */
				for(j = i; (j < count ) && (j < (INT32)STR_str_len(temp_string)); j++)
				{
					/* Current LocX needs to be stored so the character width can */
					/* be calculated. txtDrwIDV modifies LocX.                    */
					oldX = LocX;

					/* Find the glyphIndex for the current character */
					glyphIndex = TXTD_GetUnicodeCharWidth( theFont, temp_string[j] );
					glyphIndex += theFont->fontMin;

					/* This means that we have a bad character */
					/* the bad character needs to be drawn so we */
					/* must find the width.  The bad character is */
					/* on the first row */
					if( glyphIndex == 0)
					{
						glyphIndex += 1;
					}

					/* Hide the pen */
					theGrafPort.pnLevel-- ;  

					/* Call drawing procedure to determine character width. */
					txtDrwIDV(&glyphIndex, 0, 1, charSize );
    
					/* Restore the pen */
					theGrafPort.pnLevel++ ; 

					/* Calculate character width and at it to the total string width. */
					stringWidth += (LocX - oldX);
				}

								
			}

			/* Restore LocX to its original position. */
			LocX = origX;
			
            /* If text alignment is set to alignCenter. */
			if( theGrafPort.txAlign.X == alignCenter )
			{
				/* Shift LocX over by half the width of the string. */
				LocX -= (stringWidth/2);
				if(LocX < theGrafPort.portClip.Xmin)
				{
					LocX = theGrafPort.portClip.Xmin;
				}
				else if(LocX > theGrafPort.portClip.Xmax)
				{
					LocX = theGrafPort.portClip.Xmax;
				}
			}
			/* If text alignment is set to alignRight. */
			else if(( theGrafPort.txAlign.X == alignRight && theGrafPort.txPath == pathRight)
				|| (theGrafPort.txPath == pathLeft && theGrafPort.txAlign.X == alignLeft) )
			{
				/* Shift LocX over by the complete width of the string. */
				tempX = origX;
				tempX -= stringWidth;
				
				if((tempX > theGrafPort.portClip.Xmax) && (theGrafPort.txPath == pathRight))
				{
					tempX = theGrafPort.portClip.Xmax;
				}
	
				LocX = tempX;
			}

            /* Loop through and draw each character of the string separately. */
		    for ( i = 0;  ( i < count ) && (i < (INT32)STR_str_len(temp_string)); i++ )
			{
				TXTD_DrawCharW( temp_string[i], 0, 1, charSize );
			}

			/* Drawing is complete for this string so reset the global. */
			strAsChar = NU_FALSE;

            /* if not already at row 0, switch back to row 0         */
            /* (needed if DrawChar() or DrawString() is called next) */
            if ( theFont->offwidTbl[2] != 0 )
            {
                TXTD_SetWCHARRowPointers( theFont, 0 );
            }
        }
    }
    return(stringWidth);

} /* TXTD_DrawStringW() */
#endif
