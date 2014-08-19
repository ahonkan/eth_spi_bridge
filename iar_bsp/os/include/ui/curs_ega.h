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
*  curs_ega.h                                                   
*
* DESCRIPTION
*
*  This file contains cursor specific data structures.
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
#ifndef _CURS_EGA_H_
#define _CURS_EGA_H_

#ifdef      USE_CURSOR

typedef struct _CursComp
{
    /* pointer to 1st data byte */
    SIGNED pntr;    

    /* height minus 1           */
    TYPENAT hgtM1;  

    /* bytes minus 1            */
    TYPENAT bytM1;  
    
    /* min byte                 */
    TYPENAT cXmin;  

    /* min Y raster line        */
    TYPENAT cYmin;  
    
    /* max byte                 */
    TYPENAT cXmax;  

    /* max Y raster line        */
    TYPENAT cYmax;  
} CursComp;

extern CursComp *oldBuff;
extern CursComp *newBuff;

/* "curFlags" Bit Flag Values */
/* newScrn1 -> newSave1 -> oldCurs1 */
#define step1   0x0001          

/* newScrn2 -> newSave2 -> oldCurs2 */
#define step2   0x0002          

/* oldCurs3 -> newSave3 */
#define step3   0x0004          

/* curData  -> oldCurs */
#define step4   0x0008          

/* oldSave  -> oldScrn */
#define step5   0x0010          

/* oldCurs1 -> newScrn1 */
#define step6   0x0020          

/* oldCurs2 -> newScrn2 */
#define step7   0x0040          

/* Local Functions */
VOID curs_ega(VOID);

#endif /* USE_CURSOR */
#endif /* _CURS_EGA_H_ */






