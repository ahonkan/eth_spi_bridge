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
*  regions.c                                                    
*
* DESCRIPTION
*
*  Contains the API function: RS_Regions_Merge.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Regions_Merge
*
* DEPENDENCIES
*
*  rs_base.h
*  regions.h
*  global.h
*  memrymgr.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/regions.h"
#include "ui/global.h"
#include "ui/memrymgr.h"

/***************************************************************************
* FUNCTION
*
*    RS_Regions_Merge
*
* DESCRIPTION
*
*    The API function RS_Regions_Merge creates a region from two regions.
*
*    0 = REG_UNION     - Region is the mutual intersection of two input regions: rgn1 and rgn2.
*    1 = REG_INTERSECT - Region is the mutual intersection of REGION1 and REGION2.
*    2 = REG_SUBTRACT  - Region is the area of REGION1 which is not part of REGION2
*                        (i.e. the area of REGION1 with REGION2 subtracted from it.
*    3 = REG_XOR       - Region is the difference between the union and intersection
*                        of REGION1 and REGION2 (i.e. the area that is one region or the other
*                        but *not* both).
*
*    When finished using the new region, the program should deallocate the memory used.
*
* INPUTS
*
*    INT32 rgnOP  - REG_UNION      Union (combine) RECTLIST1 with RECTLIST2.
*                   REG_INTERSECT  Intersection of RECTLIST1 and RECTLIST2.
*                   REG_SUBTRACT   Subtract - RECTLIST1 with RECTLIST2 subtracted from it.
*                   REG_XOR        XOR - areas of RECTLIST1 and RECTLIST2 not in both.
*
*    region *rgn1 - Pointer to the first region.
*
*    region *rgn2 - Pointer to the second region.
*
* OUTPUTS
*
*    region       - Pointer to the created region.
*
***************************************************************************/
region *RS_Regions_Merge(INT32 rgnOP, region *rgn1, region *rgn2)
{
    rect   *ptrList1;
    rect   *ptrList2;
    INT32  numList1;
    INT32  numList2;
    region *ptrRgn;
    UINT32 sizRgn;
    INT16  grafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    ptrList1 = rgn1->rgnList;

    /* Calculate number of rects in list 1 */
    numList1 = (rgn1->rgnSize - sizeof(region) - 2*sizeof(rect)) >> 4;

    ptrList2 = rgn2->rgnList;

    /* Calculate number of rects in list 2 */
    numList2 = (rgn2->rgnSize - sizeof(region) - 2*sizeof(rect)) >> 4;

    /* Determine how much memory is needed for new region  */
    sizRgn = REGMERGE_rsMergeRegion(numList1,ptrList1,numList2,ptrList2,0,0, rgnOP);

    ptrRgn = MEM_malloc(sizRgn);

    if( !ptrRgn )
    {
        grafErrValue = c_UnionReg + c_OutofMem;
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        ptrRgn = NU_NULL;
    }
    else
    {
        REGMERGE_rsMergeRegion(numList1,ptrList1,numList2,ptrList2,sizRgn, ptrRgn, rgnOP);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(ptrRgn);
}
