/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       part_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Partition
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for partition services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/pcdisk.h"

#ifndef PART_EXTR_H
#define PART_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


VOID fpart_cyl_read(FPART_TAB_ENT_S entry, UINT16 *start_cyl, UINT16 *end_cyl);
STATUS fpart_mount_check(FDEV_S *phys_fdev);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* PART_EXTR_H */
