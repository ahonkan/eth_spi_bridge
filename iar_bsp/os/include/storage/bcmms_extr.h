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
*       bcmms_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Device Layer Block Cache Manager - Multi-sector
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for the the multi-sector 
*       block cache manager.                                 
*                                                                       
* DATA STRUCTURES                                                       
*                         
*       None                                              
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None                      
*                                                                       
*************************************************************************/
#ifndef BCMMS_EXTR_H
#define BCMMS_EXTR_H

#include "storage/bcmms_defs.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

STATUS bcmms_create(UINT16 dh, UNSIGNED num_members, UNSIGNED block_size);
STATUS bcmms_delete(UINT16 dh);
STATUS bcmms_read(BCMMS_CB *cb, UINT32 block_number, VOID *block_data);
STATUS bcmms_write(BCMMS_CB *cb, UINT32 block_number, VOID *block_data);
VOID   bcmms_flush(BCMMS_CB *cb);
STATUS bcmms_feature_init(VOID);
STATUS bcmms_device_has_cache(UINT16 dh, BCMMS_CB **ret_cb);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* BCMMS_EXTR_H */

