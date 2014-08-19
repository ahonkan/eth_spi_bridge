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
*  regions.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for regions.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _REGIONS_H_
#define _REGIONS_H_

extern NU_MEMORY_POOL  System_Memory;

extern UINT32 REGMERGE_rsMergeRegion( INT32 numRects1, rect *rectList1,
                                      INT32 numRects2, rect *rectList2,
                                      INT32 sizeRGN,  region *destRGN,
                                      INT32 rgnOP);

/* Local Functions */
region *RS_Regions_Merge(INT32 rgnOP, region *rgn1, region *rgn2);


#endif /* _REGIONS_H_ */




