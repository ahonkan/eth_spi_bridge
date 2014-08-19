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
*       bcmms_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Device Layer Block Cache Manager - Multi-sector
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Data structure and type definitions for the the multi-sector 
*       block cache manager.                                 
*                                                                       
* DATA STRUCTURES                                                       
*       
*       BCMMS_SLOT              Slot structure as allocated from the control
*                               block memory partition
*       BCMMS_CB                Device multi-sector cache control block                                                
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None                      
*                                                                       
*************************************************************************/
#ifndef BCMMS_DEFS_H
#define BCMMS_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#define BCMMS_SLOT_SIZE                         ((sizeof(BCMMS_SLOT) + sizeof(UNSIGNED) \
                                                  - 1)/sizeof(UNSIGNED)) *    \
                                                  sizeof(UNSIGNED)
#define BCMMS_PARTITION_OVERHEAD                PM_OVERHEAD /* Overhead associated with a partition */
#define BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT     8           /* Number of blocks in
                                                               BCMMS_CB.xfer_buffer */   
#define BCMMS_BUFFER_BLOCK_COUNT                24          /* Number of buffered blocks available
                                                               for reordering */

typedef struct _bcmms_slot_struct
{
    struct _bcmms_slot_struct   *next;          /* Next slot in increasing block order */       
    struct _bcmms_slot_struct   *prev;          /* Previous slot in decreasing block order */
    UINT32                      block_number;   /* Logical block number of the contained data */
    VOID                        *block_data;    /* Data for the block */
    
} BCMMS_SLOT;

typedef struct _bcmms_cb_struct
{
    struct _bcmms_cb_struct *next;          /* Next bcmms control block */
    UINT16                  dh;             /* Disk handle for the associated device */
    UNSIGNED                num_members;    /* Max number of members */
    UNSIGNED                empty_count;    /* Number of empty slots in cache */
    UNSIGNED                high_threshold; /* Number of occupied blocks to trigger processing */    
    UNSIGNED                low_threshold;  /* Max number of occupied blocks after processing */
    UNSIGNED                block_size;     /* Number of bytes per block */                                       
    BCMMS_SLOT              *slot_head;     /* Pointer to a list of slots sorted in increasing order by
                                               block number */
    NU_PARTITION_POOL       block_pool;     /* Partition pool controlling data block allocation */
    NU_PARTITION_POOL       slot_pool;      /* Partition pool controlling slot allocation */
    VOID                    *block_memory;  /* Raw memory allocated to hold block partition pool */    
    VOID                    *slot_memory;   /* Ram memory allocated to hold slot partition pool */
    VOID                    *xfer_buffer;   /* Large buffer used to group blocks for MS IO request */     
      
} BCMMS_CB;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* BCMMS_DEFS_H */

