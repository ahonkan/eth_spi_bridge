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
*  pattops.c                                                    
*
* DESCRIPTION
*
*  This file contains palette functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  DefinePattern
*  AlignPattern
*
* DEPENDENCIES
*
*  rs_base.h
*  pattops.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pattops.h"

/***************************************************************************
* FUNCTION
*
*    DefinePattern
*
* DESCRIPTION
*
*    Function DefinePattern redefines the specified pattern index (patNDX)
*    to a user specified pattern.  If the pattern address (adsPAT) is NULL
*    then the original internal pattern is re-loaded.  Only patterns 8 thru
*    31 may be redefined.
*
*    NOTE:  Pattern alignments are *always* set to zero, even when re-loaded
*    from the default pattern set.  Thus, if the internal pattern was
*    (a) re-aligned and (b) reset by passing a NULL image ptr, the alignment
*    value will be false because the old alignment value is not saved.
*
* INPUTS
*
*    INT32 patNDX    - Pattern index.
*
*    pattern *adsPAT - Pointer to the pattern address.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DefinePattern(INT32 patNDX, pattern *adsPAT)
{
    
#ifdef  FILL_PATTERNS_SUPPORT
    
    INT16   Done = NU_FALSE;
    INT16   grafErrValue;
    pattern *patPtr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( !Done )
    {
        /* check if null pointer */
        if( adsPAT == NU_NULL )
        {
            /* use default internal pattern data */
            patPtr = (pattern *) &FillPat[patNDX];
        }
        else
        {
            patPtr = adsPAT;
        }

        if( (patPtr->patWidth > maxPatSize) || (patPtr->patHeight > maxPatSize) )
        {
            grafErrValue = c_DefinePa + c_BadPattSize;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    }

    if( !Done )
    {
        thePort->portPat->patAlignX[patNDX] = 0;
        thePort->portPat->patAlignY[patNDX] = 0;

        thePort->portPat->patPtr[patNDX] = patPtr;
    }

    /* Return to user mode */
    NU_USER_MODE();
    
#else

    NU_UNUSED_PARAM(patNDX);
    NU_UNUSED_PARAM(adsPAT);

#endif /* FILL_PATTERNS_SUPPORT */

}

/***************************************************************************
* FUNCTION
*
*    AlignPattern
*
* DESCRIPTION
*
*    Function AlignPattern aligns the specified pattern number, PATNBR,
*    with the specified pixel position.  Patterns can be any size so long
*    as they meet the conditions listed below.
*
*    ERROR CONDITIONS (c_BadPatt):
*
*    a)  pattern number is < 0 or > 31.   [ noop, nothing happens ]
*    b)  patAlign must be zero            [ noop,    "       "    ]
*    c)  rowBytes > 1024.                 [ noop,    "       "    ]
*    d)  X alignment mod patWidth  > 255. [ skip column alignment ]
*    e)  Y alignment mod patHeight > 255. [ skip row alignment    ]
*
* INPUTS
*
*    INT32 patNbr - Pattern number
*
*    INT32 pixX   - X alignment.
*
*    INT32 pixY   - Y alignment.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID AlignPattern(INT32 patNbr, INT32 pixX, INT32 pixY)
{
    
#ifdef  FILL_PATTERNS_SUPPORT
    
    INT16   Done         = NU_FALSE;
    INT16   JumpAPYalign = NU_FALSE;
    signed char    bitP1;
    signed char    bitP2;
    signed char    bitP2All;
    signed char    sizlByte;
    INT16   bytP1;
    INT16   bytP2;
    INT16   tCnt;
    INT32   rowCnt;
    INT32   numRows;
    INT16   plnCnt;
    INT32   xdelta;
    INT32   ydelta;
    INT32   rot;
    INT32   sizRow;
    INT32   sizPln;
    UINT8   *bgnData;
    UINT8   *curRow;
    UINT8   *savCurRow;
    UINT8   *srcCurRow;
    INT32   BytDlt;
    INT32   lastByte;
    INT32   Part1;
    INT32   Part2;
    patList *patL;
    pattern *patR;
    INT16   grafErrValue;
    INT32   xyAlign;
    INT32   oldxyAlign;
    INT32   bitsPerRow;
    INT32   alnPatData;
    UINT8   tempRow[32];

    /* Mask bits left */
    UINT8 BitsLf[8] = {0x00,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE};

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( patNbr > 31 )
    {
        /* number too large - post an error & exit */
        grafErrValue = c_AlignPat + c_BadPatt;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* current pattern list */
        patL = thePort->portPat;     

        /* current pattern record */
        patR = patL->patPtr[patNbr]; 

        if( (patR->patAlign != 0) || (patR->patBytes > 1024) )
        {
            grafErrValue = c_AlignPat + c_BadPatt;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    }
    
    if( !Done )
    {
        while( !JumpAPYalign )
        {    
            /* First do X alignment */
            /* calculate mod of X alignment */
            xyAlign = pixX % patR->patWidth; 

            if( xyAlign < 0 )
            {
                xyAlign += patR->patWidth;
            }

            if( xyAlign > 255 )
            {
                /* alignment > 255 - post an error & move on to Y alignment */
                grafErrValue = c_AlignPat + c_BadPatt;
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                JumpAPYalign = NU_TRUE;
                break; 
            }

            /* get old X alignment */
            oldxyAlign = patL->patAlignX[patNbr];  

            /* store new alignment */
            patL->patAlignX[patNbr] = xyAlign;     
            xdelta = xyAlign - oldxyAlign;

            if( xdelta == 0 )
            {
                JumpAPYalign = NU_TRUE;
                break; 
            }

            if( xdelta < 0 )
            {
                xdelta += patR->patWidth;
            }

            /* calculate bits to shift */
            xdelta *= patR->patBits;    
            bitsPerRow = patR->patBits * patR->patWidth;

            if( bitsPerRow == 8 )
            {
                /* X align 8 bit pattern - rotate bits in each byte */
                curRow = &patR->patData[0];
                rot = 8 - xdelta;
                for( plnCnt = 0; plnCnt < patR->patPlanes; plnCnt++)
                {
                    for( rowCnt = 0; rowCnt < patR->patHeight; rowCnt++)
                    {
                        alnPatData = *curRow;
                        *curRow = ( ((alnPatData >> xdelta) | (alnPatData << rot)) & 0xFF );
                        curRow += patR->patBytes;
                    }
                }
                JumpAPYalign = NU_TRUE;
                break; 
            }

            if( bitsPerRow < 8 )
            {
                /* X align < 8 bit pattern */
                rot = patR->patWidth - xdelta;
                curRow = &patR->patData[0];
                for( plnCnt = 0; plnCnt < patR->patPlanes; plnCnt++)
                {
                    for( rowCnt = 0; rowCnt < patR->patHeight; rowCnt++)
                    {
                        alnPatData = *curRow & BitsLf[patR->patWidth];
                        *curRow = ( ( (alnPatData >> xdelta) | (alnPatData << rot)) & 0xFF );
                        curRow += patR->patBytes;
                    }
                }

                JumpAPYalign = NU_TRUE;

                /* to remove paradigm warning */
                (VOID)JumpAPYalign;
                break; 
            }

            /* X align > 8 bit pattern */
            /* load plane count */
            plnCnt = patR->patPlanes;   

            /* load row count */
            numRows = patR->patHeight;  

            /* load row size */
            sizRow = patR->patBytes;    

            /* byte X delta */
            BytDlt = xdelta >> 3;       

            /* bit X delta */
            bitP1 = xdelta & 7;         

            lastByte = patR->patWidth * patR->patBits;

            /* # bits in last byte */
            sizlByte = lastByte & 7;    
            lastByte = lastByte >> 3;
    
            if( sizlByte != 0 )
            {
                lastByte++;
            }

            /* part1 = total - byte_delta */
            bytP1 = lastByte - BytDlt;  
            bytP2 = BytDlt;

            /* lastByte = last pat byte in row */
            lastByte--; 

            if( sizlByte == 0 )
            {
                bitP2All = bitP1;
            }
            else
            {
                bitP2All = (8 - sizlByte + bitP1);
            }
            bitP2 = bitP2All & 7;

            if( bitP1 == 0 )
            {
                /* part 1 is aligned */
                Part1 = APAP1;  
            }
            else
            {
                /* part 1 is non-aligned */
                Part1 = APNAP1; 
            }

            if( bytP2 == 0 )
            {
                /* no second part */
                Part2 = APBbot;
            }
            else
            {
                if( bitP2All == 0 )
                {
                    /* part 2 is aligned */
                    Part2 = APAP2;
                }
                else
                {
                    if( bitP2 == 0 )
                    {
                        /* part part 2 is aligned (dec src) */
                        Part2 = APAP2D;
                    }
                    else
                    {
                        if( !(bitP2All & 8) )
                        {
                            /* part 2 is non aligned */
                            Part2 = APNAP2;
                        }
                        else
                        {
                            /* part 2 is non aligned (dec src) */
                            Part2 = APNAP2D;
                        }
                    }
                }
            }

            curRow = &patR->patData[0];

            while( plnCnt-- )
            {
                rowCnt = numRows;
                while( rowCnt-- )
                {
                    savCurRow = curRow;
                    switch(Part1)
                    {
                    /* aligned part 1 */
                    case APAP1:     
                        for( tCnt = 0; tCnt < bytP1; tCnt++)
                        {
                            /* copy first part (xdelta bytes) to shifted position */
                            tempRow[tCnt + BytDlt] = *curRow++;
                        }
                        break;
                    /* non-aligned part 1 */
                    case APNAP1:    
                        if( sizlByte == 0 )
                        {
                            alnPatData = *(curRow + lastByte);
                        }
                        else
                        {
                            rot = 8 - sizlByte;
                            alnPatData = (((*(curRow + lastByte - 1) << sizlByte))
                                & 0xFF) | (*(curRow + lastByte) >> rot);
                        }

                        rot = 8 - bitP1;
                        for( tCnt = 0; tCnt < bytP1; tCnt++)
                        {
                            /* copy first part (xdelta bytes) to shifted position */
                            tempRow[tCnt + BytDlt] = ((alnPatData << rot)
                                & 0xFF) | (*curRow >> bitP1);
                            alnPatData = *curRow++;
                        }
                    }
        
                    switch(Part2)
                    {
                    /* aligned part 2 (dec src) */
                    case APAP2D:    
                        curRow--;
                    
                    /* aligned part 2 */
                    case APAP2:     
                        for( tCnt = 0; tCnt < bytP2; tCnt++)
                        {
                            /* copy second part (total - xdelta bytes) */
                            tempRow[tCnt] = *curRow++;
                        }
                        break;
                    
                    /* non-aligned part 2 (dec src) */
                    case APNAP2D:  
                        curRow--;
                    
                    /* non-aligned part 2 */
                    case APNAP2:    
                        rot = 8 - bitP2;
                        alnPatData = *(curRow - 1);
                        for( tCnt = 0; tCnt < bytP2; tCnt++)
                        {
                            /* copy second part (total - xdelta bytes) */
                            tempRow[tCnt] = ((alnPatData << rot)
                                & 0xFF) | (*curRow >> bitP1);
                            alnPatData = *curRow++;
                        }
                        break;

                    case APBbot:
                        break;
                    }
        
                    /* copy temp row back to pattern space */
                    curRow = savCurRow;
                    for( tCnt = 0; tCnt < sizRow; tCnt++)
                    {
                        /* copy second part (total - xdelta bytes) */
                        *savCurRow++ = tempRow[tCnt];
                    }

                    /* inc curRow */
                    curRow += sizRow;
                } /* while( rowCnt-- ) */

            } /* while( plnCnt-- ) */

            break; 
        } /* while( !JumpAPYalign ) */

    } /* if( !Done ) */

    /* Now do Y alignment */

    if( !Done )
    {
        /* calculate mod of X alignment */
        xyAlign = pixY % patR->patHeight;   
        if( xyAlign < 0 )
        {
            xyAlign += patR->patHeight;
        }

        if( xyAlign > 255 )
        {
            /* alignment > 255 - post an error & move on to Y alignment */
            grafErrValue = c_AlignPat + c_BadPatt;
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        /* get old X alignment */
        oldxyAlign = patL->patAlignY[patNbr];   

        /* store new alignment */
        patL->patAlignY[patNbr] = xyAlign;      

        ydelta = xyAlign - oldxyAlign;
        if( ydelta == 0 )
        {
            Done = NU_TRUE;
        }
    } /* if( !Done ) */
    
    if( !Done )
    {
        if( ydelta < 0 )
        {
            ydelta += patR->patHeight;
        }

        rot = ydelta * patR->patBytes;

        /* load row count */
        numRows = patR->patHeight;                  

        /* size of plane */
        sizPln = patR->patHeight * patR->patBytes;  

        /* start of data */
        bgnData = &patR->patData[0];                
        plnCnt = patR->patPlanes;
        curRow = bgnData;
    
        while( plnCnt-- )
        {
            rowCnt = numRows;
            savCurRow = curRow;
            for( tCnt = 0; tCnt < patR->patBytes; tCnt++)
            {
                tempRow[tCnt] = *curRow++;
            }

            srcCurRow = savCurRow;
            while( rowCnt-- )
            {
                curRow = srcCurRow;
                srcCurRow -= rot;
                if( srcCurRow < bgnData)
                {
                    srcCurRow += sizPln;
                }
            
                if( srcCurRow != savCurRow)
                {
                    for( tCnt = 0; tCnt < patR->patBytes; tCnt++)
                    {
                        *curRow++ = *srcCurRow++;
                    }
                    srcCurRow -= patR->patBytes;
                    continue;
                }

                for( tCnt = 0; tCnt < patR->patBytes; tCnt++)
                {
                    *curRow++ = tempRow[tCnt];
                }
                srcCurRow = curRow;
                savCurRow = curRow;
            
                for( tCnt = 0; tCnt < patR->patBytes; tCnt++)
                {
                    tempRow[tCnt] = *curRow++;
                }
            }

            bgnData += sizPln;
        } /* while( plnCnt-- ) */

    } /* if( !Done ) */

    /* to remove paradigm warning */
    (VOID)JumpAPYalign;

    /* Return to user mode */
    NU_USER_MODE();

#else

    NU_UNUSED_PARAM(patNbr);
    NU_UNUSED_PARAM(pixX);
    NU_UNUSED_PARAM(pixY);
    
#endif /* FILL_PATTERNS_SUPPORT */
    
}
