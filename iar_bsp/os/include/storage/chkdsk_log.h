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
*      chkdsk_log.h
*
* COMPONENT
*
*      FAT
*
* DESCRIPTION
*
*      Function prototypes for Nucleus FILE's FAT file system check disk
*      utility logger.
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
#include "storage/file_cfg.h"
#include "storage/chkdsk_defs.h"

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


STATUS chk_start_error_logger(UINT16 dh,CHAR *pfile_name);
STATUS chk_stop_error_logger(VOID);
STATUS chk_del_error_logger(CHAR *pfile_name);
STATUS chk_start_test_case(UINT8 test_case_id);
STATUS chk_write_mess(CHAR *pmess,UINT16 num);
STATUS chk_flush_error_logger(VOID);
STATUS chk_end_test_case(VOID);

/* Logger Utility Functions */
VOID   chk_log_add_ddr_iv_cl_err(DDRIVE *pddrive, UINT32 cl_value);
STATUS chk_log_write_errors_to_file(CHKDSK_LOGID log_id);
VOID   chk_log_add_error(CHKDSK_LOGID log_id);
UINT8  chk_log_get_error_num(CHKDSK_LOGID log_id);
UINT8  chk_log_is_error_num_zero(CHKDSK_LOGID log_id);


/* Make sure our error long counter values are zero. */
#define CHK_CLEAR_LOG_REC() (pc_memfill(&g_chkdsk_log_rec,sizeof(LOG_RECORD) * MAX_NUM_LOG_RECORD_ENTRIES,0));


#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

