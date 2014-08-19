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
*       bcm_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Device Layer Block Cache Manager
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for block cache manager                                 
*                                                                       
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       NONE         
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       NONE               
*                                                                       
*************************************************************************/
#ifndef BCM_EXTR_H
#define BCM_EXTR_H

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
    #include "storage/bcmms_extr.h"
#endif
#include "storage/bcm_defs.h"
    
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


STATUS bcm_block_cache_feature_init(VOID);
STATUS bcm_device_has_cache(UINT16 dh, BCM_CB **bc_cb);                       
STATUS bcm_device_read_request ( UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count);
STATUS bcm_device_write_request ( UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count);
VOID   bcm_report_error(UINT16 dh, UINT32 sector, BCM_ERROR_OPERATION operation, 
                        STATUS io_status);
#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* BCM_EXTR_H */
