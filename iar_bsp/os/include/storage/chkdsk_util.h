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
*      chkdsk_util.h
*
* COMPONENT
*
*      FAT
*
* DESCRIPTION
*
*      Function prototypes for Nucleus FILE's FAT file system check disk
*      utility functions.
*
* DATA STRUCTURES
*
*      None.
*
* FUNCTIONS
*
*      None.
*
* DEPENDENCIES
*
*      None.
*
*************************************************************************/
#include    "storage/chkdsk_defs.h"
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

    /* FAT */
STATUS chk_get_first_fat_trav_blk(FAT_TABLE_TRAV_BLK *pfat_trav_blk,UINT16 dh,UINT8 fat_table);
STATUS chk_get_next_fat_trav_blk(FAT_TABLE_TRAV_BLK *pfat_trav_blk);

/* Bit Map functions. */
STATUS chk_bitmap_mark_cl_chain(UINT32 start_cl,DDRIVE *pddrive, UINT8 mark_value);
UINT8  chk_bitmap_get_value(UINT32 idx);
STATUS chk_bitmap_set_value(UINT32 idx,UINT8 value);
STATUS chk_bitmap_replace_if(UINT16 dh, UINT8 condition, UINT8 replace_value);
STATUS chk_bitmap_mark_cl_on_disk(DIR_REC_OBJ *pdir_rec_obj);

                          /* This gives us the index position into our array. */
#define CHK_GET_IDX(idx) (idx/(MAX_BITS_PER_BITMAP_BLOCK/MAX_BITS_PER_BITMAP_ENTRY)) 


/* Utility */
STATUS chk_traverse_all_dir_paths(UINT16 dh,STATUS (*pfunc_ptr)(DIR_REC_OBJ *pdir_rec_obj),UINT8 skip_lfn_entries, INT8 b_check_chk_dir);
STATUS chk_create_error_name(CHAR *perror_name, CHAR *perror_str, UINT16 error_num,UINT8 bool_fl_name);
VOID   chk_int_to_str(CHAR *pstring,INT pstring_size,UINT32 number);
UINT32 chk_get_dir_rec_cl(DDRIVE *pddrive,DOSINODE *pdir_rec);
UINT32 chk_get_eoc_value_for_drive(DDRIVE *pddrive);
STATUS chk_make_err_dir(CHAR* dirname);
UINT8  chk_is_cl_range(DDRIVE *pddrive, UINT32 cl);
UINT8  chk_is_cl_res(DDRIVE *pddrive, UINT32 cl);
UINT8  chk_is_cl_free(DDRIVE *pddrive, UINT32 cl);
UINT8  chk_is_cl_bad(DDRIVE *pddrive, UINT32 cl);
UINT32 chk_get_cl_mask(DDRIVE *pddrive);

/* Mutex */
STATUS chk_get_chkdsk_lock(VOID);
STATUS chk_release_chkdsk_lock(VOID);



/* Used to determine what FAT type the current drive is using. */
#define CHK_IS_DRIVE_FAT12(drive) (drive->fasize == 3 ? NU_TRUE : NU_FALSE)
#define CHK_IS_DRIVE_FAT16(drive) (drive->fasize == 4 ? NU_TRUE : NU_FALSE)
#define CHK_IS_DRIVE_FAT32(drive) (drive->fasize == 8 ? NU_TRUE : NU_FALSE)

/* Stack functions. */
STATUS chk_stack_init(DIR_REC_LOC_STACK *pstack,INT size);
STATUS chk_stack_cleanup(DIR_REC_LOC_STACK *pstack);
STATUS chk_stack_push(DIR_REC_LOC_STACK *pstack,DIR_REC_LOC *pdir_rec_blk);
STATUS chk_stack_pop(DIR_REC_LOC_STACK *pstack,DIR_REC_LOC *pdir_rec_blk);
STATUS chk_stack_peek(DIR_REC_LOC_STACK *pstack,DIR_REC_LOC *pdir_rec_blk);
#define CHK_STACK_ISEMPTY(pstack) ((*pstack).stack_top_offset ? NU_FALSE : NU_TRUE)



#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

