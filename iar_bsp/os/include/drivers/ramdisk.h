/*************************************************************************/
/*                                                                       */
/*               Copyright 2010 Mentor Graphics Corporation              */
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
*       ramdisk.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       RAM Disk Driver
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Configuration and interface defines for RAMDISK device driver
*                                                                       
* DATA STRUCTURES
*
*       RD_INSTANCE_HANDLE
*       RD_SESSION_HANDLE
*
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef RAMDISK_H
#define RAMDISK_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "kernel/dev_mgr.h"
#include "services/power_core.h"

/* ============= RAMDISK =============== */

/* For the demonstration application, we auto format the RAMDISK with 
   a FAT file system. Doing so will allow mount to complete successfully.
   Otherwise, a mount request for FAT on the RAMDISK will cause a format
   error. Set to NU_FALSE to handle formatting the device in your 
   application.
*/
#define AUTOFORMAT_RAMDISK (NU_TRUE)


/* For the demonstration port we create a 50K ram disk.  If you purchased
   MGC's RAM Disk Driver, you can change the following definitions to
   any size you desire (with the exceptions as listed below).

   Note on Intel the RAM Disk is always in the far heap.  So you may create
   a large ram disk even in small model. Note the ram disk driver will
   allocate this much core when it is first mounted.

   If you don't need a ramdisk eliminate it from devtable.c
*/


/* Set the following to one if you wish to allocate ram disk memory from
   a PLUS memory pool. This affects code in nufp.c and ramdisk.c. This should
   be set to 1 for 32 bit systems where it is possible to use allocate > 64K
   to a memory pool.  If you wish to allocate a RAM Disk larger than 48K on
   an Intel real-mode based Nucleus PLUS port, then you should set this
   manifest to 0.  In that case, the pool will be created by using a DOS
   malloc call.
*/
#define RAMDISK_FROMPOOL        1      /* Plus only */

#if (RAMDISK_FROMPOOL)

/* Nucleus PLUS: If allocating the ram disk from a partition pool we
   allocated 12 pages (48K). This is because 8086 real mode ports under plus
   may only allocate 64K at a time for a pool. For 32 bit ports this
   restriction does not exist.  */
#define NUM_RAMDISK_PAGES       CFG_NU_OS_DRVR_FAT_RD_NUM_RAMDISK_PAGES     /*  (Must be at least 1) */
#else
#define NUM_RAMDISK_PAGES       16     /*  (Must be at least 1) */
#endif


#define RAMDISK_PAGE_SIZE       8      /*  8 blocks ='s 4 k (don't exceed 32) */
#define NRAMDISKBLOCKS          (NUM_RAMDISK_PAGES * RAMDISK_PAGE_SIZE)

#define POOL_SIZE \
    ((unsigned)(((unsigned)NUM_RAMDISK_PAGES) * \
                (((unsigned)NUF_RAMDISK_PARTITION_SIZE) + \
                 ((unsigned)PARTITION_SIZE))))


extern  NU_PARTITION_POOL       NUF_RAMDISK_PARTITION;

#define ALLOC_SIZE              20
#define PARTITION_SIZE          20

/* The ram disk memory is allocate from the fixed partition at
   NUF_RAMDISK_PARTITION . There are NUM_RAMDISK_PAGES equal sized
   memory blocks of size NUF_RAMDISK_PARTITION_SIZE. This value need not
   be changed. To modify the ram disk size you should modify 
   RAMDISK_PAGE_SIZE */

#define HEAPGRAN                1

#define NUF_RAMDISK_PARTITION_SIZE  (RAMDISK_PAGE_SIZE*512/HEAPGRAN)


STATUS  RD_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte,
                 OFFSET_T byte_offset, UINT32 *bytes_read_ptr);
STATUS  RD_Write (VOID *session_handle, const VOID *buffer,
                  UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written_ptr);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* RAMDISK_H */
