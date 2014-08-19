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
*       dh_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Disk handle
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for disk handle services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "nucleus.h"
#include "storage/dh_defs.h"

#ifndef DH_EXTR_H
#define DH_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Prototypes for disk handle services */
STATUS fsdh_init_handles(VOID);
STATUS fsdh_allocate_dh(UINT16 *dh);
STATUS fsdh_free_dh(UINT16 dh);
STATUS fsdh_set_dh_specific(UINT16 dh, VOID *specifc, CHAR *disk_name);
STATUS fsdh_get_dh_specific(UINT16 dh, VOID **specific, CHAR *disk_name);
STATUS fsdh_set_fs_specific(UINT16 dh, VOID *specific);
STATUS fsdh_get_fs_specific(UINT16 dh, VOID **specific);
STATUS fsdh_get_fsdh_struct(UINT16 dh, FSDH_S **dhs);
STATUS fsdh_dh_to_devname(UINT16 dh, CHAR *devname);
STATUS fsdh_devname_to_dh(CHAR *devname, UINT16 *dh);
STATUS fsdh_dh_idx_to_dh(UINT16 dh_idx, UINT16 *dh);
STATUS fsdh_set_semaphore(UINT16 dh, NU_SEMAPHORE *semaphore);
STATUS fsdh_get_semaphore(UINT16 dh, NU_SEMAPHORE **semaphore);
STATUS fsdh_set_user_state(UINT16 dh, UINT32 idx, UINT8 state);
STATUS fsdh_get_user_state(UINT16 dh, UINT32 idx, UINT8 *state);

/* This macro is used to verify that the dh passed in is valid.
   dh is an in parameter.
   sts is a STATUS value and an out parameter */
#define FSDH_VERIFY_DH(dh,sts)                                                                                      \
{                                                                                                                   \
    sts = NUF_BADPARM;                                                                                              \
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)                                                                  \
    {                                                                                                               \
        if(((FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_flags & FSDH_F_VALID)) &&                               \
            ((UINT8)(FSDH_GET_DISK_ID_FROM_DH(dh)) == (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_disk_id)))    \
           sts = NU_SUCCESS;                                                                                        \
    }                                                                                                               \
}

/* A dh contains both the disk id and a index into FS_Disk_Handle.
   The most significant 8 bits are the disk id and the least 
   significant 8 bits are the position in the FS_Disk_Handle array. */
#define FSDH_GET_DISK_ID_FROM_DH(X) (X>>8)
#define FSDH_GET_IDX_FROM_DH(X) (X&0x00FF)

/* Min disk id value is 1. */
#define FSDH_GET_NEXT_DISK_ID(disk_id) ((++disk_id==0) ? ++disk_id : disk_id)

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* DH_EXTR_H */
