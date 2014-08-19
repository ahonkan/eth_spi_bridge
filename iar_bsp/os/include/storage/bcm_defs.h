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
*       bcm_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Device Layer Block Cache Manager
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Data structure and type definitions for the the block cache
*       manager.                                 
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       BCM_SLOT_STATE      Enumerated slot state 
*       BCM_BLOCK_STATE     Enumerated block state
*       BCM_SLOT            Slot data structure for cache contents
*       BCM_SLOT_SET_CB     Control block for managing a set of slots
*       BCM_OLE             Ordered list entry
*       BCM_OL_CB           Control block for managing an ordered list
*       BCM_CB              Cache control block 
*       BCM_CACHE_TYPE      Enumerated cache types available
*       BCM_DL_LRU_CONFIG   Configuration parameter structure for an
*                           Least Recently Used cache
*       BCM_ERROR_OPERATION Enumerated operation resulting in error
*       BCM_ERROR           Error log entry data structure
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None                      
*                                                                       
*************************************************************************/
#ifndef BCM_DEFS_H
#define BCM_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Define valid range for block sizes in case a driver blindly returns
   success to requests for disk info */
#define BCM_BLOCK_SIZE_4K   4096
#define BCM_BLOCK_SIZE_512B 512

/* Task creation settings for each periodic flush task */
#define BCM_PERIODIC_FLUSH_TASK_PRIORITY        255         /* Lowest priority */        
#define BCM_PERIODIC_FLUSH_TASK_STACK_SIZE      0x1200
#define BCM_PERIODIC_FLUSH_TASK_TIME_SLICE      0           /* Disabled */
#define BCM_PERIODIC_FLUSH_TASK_PREEMPT         NU_PREEMPT

/* Number of message entries in the block cache error log */
#define BCM_BLOCK_CACHE_ERR_LOG_SIZE            10

/* Periodic Task events */
#define BCM_EVG_TIMEOUT_MODIFIED    0x00000001  /* Configuration change to timeout value */
#define BCM_EVG_TIMEOUT_START       0x00000002  /* Dirty block entered cache, start timer */
#define BCM_EVG_ASYNC_FLUSH         0x00000004  /* Perform an async flush */

/* Value for uninitialized block used during transition from newly allocated slot
   to clean slot */
#define BCM_UNINITIALIZED_BLOCK     0xFFFFFFFF

typedef enum _bcm_slot_state_enum
{
    BCM_SLOT_STATE_EMPTY,   /* Slot is empty */    
    BCM_SLOT_STATE_OCCUPIED /* Slot is occupied */
    
} BCM_SLOT_STATE;

typedef enum _bcm_block_state_enum
{
    BCM_BLOCK_STATE_DIRTY,   /* Block is dirty */
    BCM_BLOCK_STATE_CLEAN   /* Block is clean */
    
} BCM_BLOCK_STATE;


/* Individual slot contents */
typedef struct _bcm_slot_struct
{
    BCM_SLOT_STATE  slot_state;     /* Slot state [empty or occupied] */
    UINT32          block_number;   /* Block number contents (only valid if slot is occupied) */
    BCM_BLOCK_STATE block_state;    /* Block state [dirty, clean] (only valid if slot is occupied) */
    STATUS          error_pending;  /* Block error pending (only valid if slot is occupied) */
    
} BCM_SLOT;

typedef struct _bcm_slot_set_cb_struct
{
    UNSIGNED        num_members;    /* Number of slots and blocks, there is a one-to-one slot to block
                                       relationship */
    UNSIGNED        empty_count;    /* Number of empty slots in cache */                                       
    UNSIGNED        block_size;     /* Number of bytes per block */                                       
    BCM_SLOT        *slot_set;      /* Pointer to an array of slots[num_members] */
    UINT8           *block_set;     /* Pointer to an array of blocks[num_members] */
      
} BCM_SLOT_SET_CB;


/* Ordered list entry */
typedef struct _bcm_ole_struct
{
  struct _bcm_ole_struct  *next;          /* Next item in ordered list */
  struct _bcm_ole_struct  *prev;          /* Previous item in ordered list */
  UNSIGNED                slot_idx;       /* index in to the slot set */
      
} BCM_OLE;

/* Control block for the block cache's ordered list */
typedef struct _bcm_ol_cb_struct
{
    BCM_OLE               *bcm_lru;       /* Pointer to least recently used cache entry */
    BCM_OLE               *bcm_mru;       /* Pointer to most recently used cache entry */
    VOID                  *core;          /* Pointer to memory used for the list entries */
     
} BCM_OL_CB;

typedef struct _bcm_cb_struct
{
    struct _bcm_cb_struct *next;          /* Next control block in list */    
    UINT16                dh;             /* Disk handle that control block applies to */
    NU_SEMAPHORE          cb_sema;       /* Access control for the control block */
    NU_TASK               control_task;  /* Period flush control task */
    VOID                  *task_stack;   /* Stack for periodic flush task */
    NU_EVENT_GROUP        control_group; /* Event group for communication with control task */
    UNSIGNED              low_threshold;  /* Low threshold value for flushing */
    UNSIGNED              high_threshold; /* High threshold value for triggering a flush */
    
    UNSIGNED              periodic_flush; /* Ticks per periodic flush */
    UNSIGNED              dirty_count;    /* Number of dirty blocks in cache */
        
    BCM_SLOT_SET_CB       ss_cb;         /* Slot set control block */
    BCM_OL_CB             ol_cb;         /* Ordered list control block */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)    
    BCMMS_CB              *bcmms_cb;     /* Control block for block re-ordering */
#endif         
} BCM_CB;

typedef enum _bcm_cache_type_enum
{
    BCM_CACHE_TYPE_DL_NONE,             /* Invalid cache type */
    BCM_CACHE_TYPE_DL_LRU               /* Device layer LRU cache */
    
} BCM_CACHE_TYPE;


/* Configuration parameters for the device layer LRU cache */
typedef struct _bcm_dl_lru_config_struct
{
    UNSIGNED    size;                   /* Size in KB where 1KB is 1024 bytes */
    UNSIGNED    periodic_flush;         /* Ticks per periodic flush */
    UNSIGNED    low_threshold;          /* Low threshold value for flushing */
    UNSIGNED    high_threshold;         /* High threshold value for triggering a flush */
    
} BCM_DL_LRU_CONFIG;

/* Enum for encoding operation type */
typedef enum _bcm_error_operation_enum
{
    BCM_ERROR_OPERATION_READ,
    BCM_ERROR_OPERATION_WRITE
    
} BCM_ERROR_OPERATION;

/* Error log data type */
typedef struct _bcm_error_struct
{
   BCM_ERROR_OPERATION  operation;      /* Type of operation requested resulting in error */
   UINT32               block;          /* Logical block number requested */
   UNSIGNED             time;           /* System time when error occurred */
   STATUS               device_error;   /* Error as reported by device */ 
   CHAR                 path[2];        /* Encoded drive letter "A:", "B:" */
    
} BCM_ERROR;

#define NU_FILE_CACHE_ERROR     BCM_ERROR

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* BCM_DEFS_H */

