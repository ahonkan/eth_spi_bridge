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
*      chkdsk_extr.h
*
* COMPONENT
*
*      FAT
*
* DESCRIPTION
*
*      Function prototypes for Nucleus FILE's FAT file system check disk
*      utility.
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

STATUS chk_is_cl_valid(DDRIVE *pddrive, UINT32 cl);
STATUS chk_is_cl_value_valid(DDRIVE *pddrive, UINT32 cl);
STATUS chk_create_error_record(UINT16 dh, DDRIVE *pddrive, CHAR *pparent_folder, CHAR *perr_name, UINT16 err_cnt,UINT32 cl,UINT32 dir_rec_size,UINT8 b_is_fl);
STATUS chk_find_end_of_data_sector(UINT16 *plast_valid_element, UINT8 *psec_data);

STATUS chk_lfn_traverse_records(DIR_REC_OBJ *pdir_rec_obj,STATUS (*plfn_record_op)(UINT8 *pupdate,UINT8 *plfn_record,UINT8 sfn_chksum),UINT8 sfn_chksum);
STATUS chk_lfn_del_records(UINT8 *update,UINT8 *plfn_record,UINT8 sfn_chksum);
STATUS chk_lfn_check(UINT8 *pupdate,UINT8 *plfn_record,UINT8 sfn_chksum);

/* FAT table. */
STATUS chk_fat_table_compare_test(UINT16 dh);

/* Check file size. */
STATUS chk_file_size_test(UINT16 dh);
STATUS chk_file_size_test_op(DIR_REC_OBJ *pdir_rec_obj);
STATUS chk_calc_file_size(UINT32 *pcalc_file_size,UINT16 dh,UINT32 start_cl);
STATUS chk_calc_size_on_disk(UINT32 *pcalc_file_size,UINT16 dh,UINT32 start_cl);


/* Check directory entry */
STATUS chk_dir_rec_entry_test(UINT16 dh);       
STATUS chk_dir_rec_entry_test_op(DIR_REC_OBJ *pdir_rec_obj);
STATUS chk_dir_rec_check_for_iv_cl_values(UINT16 dh);
STATUS chk_check_dir(DIR_REC_OBJ *pdir_rec_obj);
STATUS chk_check_sfn_chars(DOSINODE *psfn_dir_rec);



STATUS create_FR4_T3_test(DDRIVE *pddrive);

/* Cross-link functions. */
STATUS chk_crosslink_test(UINT16 dh);
STATUS chk_crosslink_check_disk(DIR_REC_OBJ *pdir_rec_obj);
STATUS chk_crosslink_resolve(DIR_REC_OBJ *pdir_rec_obj);

/* Lost Cluster Chain Functions. */
STATUS chk_lost_cl_chain_test(UINT16 dh);
STATUS chk_lost_cl_chains_find_start(UINT16 dh);
STATUS chk_lost_cl_chain_resolve(UINT16 dh);

/* Handle invalid cluster values. */
STATUS chk_change_iv_cl_in_fat_to_eoc(UINT16 dh);
STATUS chk_validate_all_cl_chains_on_disk(UINT16 dh);
STATUS chk_validate_cl_chains(DIR_REC_OBJ *pdir_rec_obj);
STATUS chk_handle_bad_cl_in_cl_chain(DDRIVE *pddrive, DIR_REC_OBJ *pdir_rec_obj, UINT32 prev_cl, UINT32 cl);
STATUS chk_handle_free_cl_in_cl_chain(DDRIVE *pddrive, DIR_REC_OBJ *pdir_rec_obj, UINT32 prev_cl, UINT32 cl);
STATUS chk_check_for_bad_cl_values(UINT16 dh);
STATUS chk_process_bad_cl_op(DIR_REC_OBJ *pdir_rec_obj);


#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

