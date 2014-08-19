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
/************************************************************************
* FILE NAME
*
*       chkdsk_defs.h
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*      This file contains the defines used by Nucleus FILE's FAT 
*      check disk utility. It also contains the structure definitions 
*      that are used.
*
* DATA STRUCTURES
*
*       FAT_TABLE_TRAV_BLK                  Used to traverse the FAT
*                                           Tables.
*       DIR_REC_LOC                         Gives location that
*                                           a directory record is at.
*       DIR_REC_OBJ                         Contains the locations
*                                           on the disk of information
*                                           relating to this directory
*                                           record.
*       DIR_REC_LOC_STACK                   Used to encapsulate the
*                                           DIR_REC_LOC structure
*                                           and stack information.
*       LOG_RECORD                          Used to store information
*                                           about errors found on the 
*                                           disk.
*       CHKDSK_LOG_ID                       Unique values used to represent
*                                           the different log ids.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#include "storage/file_cfg.h"
#include "storage/fat_defs.h"

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
#ifndef __CHKDSK_DEFS__
#define __CHKDSK_DEFS__ 

#ifdef          __cplusplus
    extern  "C" {                           /* C declarations in C++ */
#endif /* _cplusplus */


/* =======DEBUG CONSTANT=======*/
#define CHKDSK_DEBUG NU_FALSE

/* Check Disk Utility Defines. */
/* If the user wishes for the check disk utility to attempt to reconstruct 
   the directory hierarchy. When performing Lost Cluster Chain(LCC) tests, 
   then change to NU_FALSE. NOTE: If set to NU_FALSE, this can cause cross
   linked chains in the reconstructed directory hierarchy, so the errors
   produces by the LCC test should be processed before attempting to run
   the check disk utility again on the same drive. */
#define CHKDSK_LCC_ERROR_FILES_ONLY NU_TRUE

/* Valid wait time values: 1 – 4,294,967,293 or NU_SUSPEND
   Setting this to zero indicates that the check disk mutex should 
   suspend until free. */
#define CHKDSK_MUTEX_WAIT_TIME NU_SUSPEND
/************************************************************************/
/*                     API Level Flags and Modes                        */
/************************************************************************/       
        
/* Flags */
#define CHK_CHECK_LOST_CL_CHAIN         0x01U
#define CHK_CHECK_CROSS_LINKED_CHAIN    0x02U
#define CHK_CHECK_FILES_SIZES           0x04U
#define CHK_CHECK_DIR_RECORDS           0x08U
#define CHK_CHECK_FAT_TABLES            0x10U

/* Modes */
#define CHK_REPORT_ERRORS_ONLY          0x01U
#define CHK_FIX_ERRORS                  0x02U


/************************************************************************/
/*                     Check Disk Constants.                            */
/************************************************************************/       

#define CHKDSK_SECTOR_SIZE 512
#define MAX_SIZE_OF_LFN_NAME 260

#define CHK_LFN_ATTR 0x0F       

/* Cluster specific constants. */
#define CHK_START_CL            0x2
#define CHK_FREE_CL             0x0
#define CHK_RESV_SPECIAL_CASE   0x1
#define CHK_RESV_CL_START       0x0FFFFFF0
#define CHK_RESV_CL_END         0x0FFFFFF6
#define CHK_BAD_CL              0x0FFFFFF7
#define CHK_EOC                 0x0FFFFFF8

#define CHK_FAT12_CL_MASK 0xFFF
#define CHK_FAT16_CL_MASK 0xFFFF
#define CHK_FAT32_CL_MASK 0xFFFFFFFF
        
/* Directory Name to store Lost Cluster Chain Entries. */
#define CHK_FOLDER_FOR_LOST_CL_CHAIN_ENTRIES        "LOST_CL"

/* Directory Name to store Cross Linked Cluster Entries. */
#define CHK_FOLDER_FOR_CROSS_LINK_ENTRIES           "CROSS_CL"

/* Invalid File Lengths folder. */
#define CHK_FOLDER_FOR_INVALID_FILE_LEN_ENTRIES     "FILE_LEN"


/* ============= Error File/Directory Names ==================== */

/* Used for a file whose size exceeds that of what is found in its
   directory record. */
#define CHK_INVALID_FILE_LEN_NAME       "FL_EL"

/* Cross-Link Chain Error Files/Directories. */
#define CHK_FILE_CROSS_LINK_NAME        "FL_CL"
#define CHK_DIR_CROSS_LINK_NAME         "DIR_CL"

/* Lost Cluster Chain Error Files/Directories. */
#define CHK_LOST_CL_CHAIN_DIR_NAME      "DIR_LC"
#define CHK_LOST_CL_CHAIN_FL_NAME       "FL_LC"

/* Error File/Directory Extension. */
#define CHK_FILE_ERROR_EXTENSION        "CHK"

/* Max number of number of digits in an error file/directory name. */
#define CHK_MAX_DIGIT_IDX_IN_ERR_NAME   6



/* Cross-linked and Lost Cluster chain Bitmap constants. */
/* A bitmap block is the size that makes up one array index. */
#define MAX_BITS_PER_BITMAP_BLOCK      8
#define MAX_BITS_PER_BITMAP_ENTRY      2


/* Cross Link constants */
/* BitMap values. */ 
#define BITMAP_FREE_CL                  0
#define VALID_ON_DISK                   3

/* Cross Link Chain */
#define CROSS_LINKED                    1
#define CL_CHAIN_RESOLVED               2

/* Lost Cluster Chain. */
#define CL_ISSUE                        1                 
#define NOT_START_OF_LOST_CL_CHAIN      2


/************************************************************************/
/*                  Check Disk Logger Constants                         */
/************************************************************************/ 

#define LOG_EXT                             "FLG"

#define LOG_NAME(ext)                       "CHK_DSK""."ext

/* Default logger file name. */
#define DEFAULT_LOG_FILE                    LOG_NAME(LOG_EXT)

/* Lost cluster chain error message. */
#define LOST_CL_CHAIN_ERR_MESS              "Total Lost Cluster Chains Found"

/* Cross-linked chain error message. */
#define CROSS_LINKED_ERR_MESS               "Total Cross-link Chains Found"

/* Invalid File Length error messages. */
#define FILES_LEN_ERR_MESS                  "Total Number Error Files Found"
#define FILES_GREATER_ERR_MESS              "Files with cluster chains greater than size in directory record"
#define FILES_LESS_ERR_MESS                 "Files with cluster chains less than size in directory record"

/* Damaged Directory Record error messages. */
#define DOT_ERR_MESS                        "Number of dots that reference wrong cluster"
#define DOT_DOT_ERR_MESS                    "Number of dot dots that reference wrong cluster"
#define DIR_SIZE_ERR_MESS                   "Number of directory's whose size isn't zero"
#define LFN_CL_ERR_MESS                     "Number of LFN directory records whose cluster value isn't zero"
#define CL_OUT_RANGE_ERR_MESS               "Number of clusters out of range"
#define CL_VALUE_BAD_ERR_MESS               "Number of clusters whose chain starts with BAD cluster value"
#define CL_VALUE_FREE_ERR_MESS              "Number of clusters whose chain starts with FREE cluster value"
#define CL_VALUE_FREE_IN_CL_CHAIN_ERR_MESS  "Number of cluster chains that contain a FREE cluster value"
#define CL_VALUE_RESERVED_ERR_MESS          "Number of clusters whose is marked as RESERVED"
#define ROOT_DIR_RANGE_ERR_MESS             "Root directory range invalid"
#define ROOT_START_CL_ERR_MESS              "Root directory start cluster invalid"
#define SFN_ILLEGAL_CHAR_ERR_MESS           "Number of SFN using illegal characters"
#define LFN_EXCEED_MAX_ERR_MESS             "Number of LFN entries exceeding max length"
#define LFN_CHKSUM_ERR_MESS                 "Number of LFN entries whose checksum doesn't match SFN checksum"

/* Fat table mismatch message. */
#define FAT_TABLE_ERR_MESS                  "FAT Tables Mismatch"


/* Max number of error records our array will hold. */
#define MAX_NUM_LOG_RECORD_ENTRIES     21

typedef enum _chkdsk_logid_enum
{
    CHKDSK_LOGID_NONE = 0, 
    CHKDSK_LOGID_FAT_TABLE=1,                       /* FAT Table mismatch log index. 								*/
    CHKDSK_LOGID_DDR_START_IDX = 2,                 /* This value must be equal to the first DDR issue. 			*/
    CHKDSK_LOGID_DDR_DOT=2,                         /* DDR DOT entries cluster value is incorrect. 					*/
    CHKDSK_LOGID_DDR_DOT_DOT=3,                     /* DDR DOT DOT entries cluster value is incorrect . 			*/
    CHKDSK_LOGID_DDR_DIR_SIZE=4,                    /* DDR Directory size isn't zero. 								*/
    CHKDSK_LOGID_DDR_LFN_CL=5,                      /* DDR LFN's directory record was not zero. 					*/
    CHKDSK_LOGID_DDR_CL_OUT_RANGE=6,                /* DDR Cluster is out of range. 								*/
    CHKDSK_LOGID_DDR_CL_VALUE_BAD=7,                /* DDR Cluster in directory record is BAD. 						*/
    CHKDSK_LOGID_DDR_CL_VALUE_FREE=8,               /* DDR Cluster in cluster chain is FREE. 						*/
    CHKDSK_LOGID_DDR_CL_VALUE_FREE_IN_CL_CHAIN=9,   /* DDR Cluster in cluster chain is BAD. 						*/
    CHKDSK_LOGID_DDR_CL_VALUE_RESERVED=10,          /* DDR Cluster value is a reserved value. 						*/
    CHKDSK_LOGID_DDR_ROOT_DIR_RANGE=11,             /* DDR Root directory is out of range. 							*/
    CHKDSK_LOGID_DDR_ROOT_START_CL=12,              /* DDR Root start cluster is invalid. 							*/
    CHKDSK_LOGID_DDR_SFN_ILLEGAL_CHAR=13,           /* DDR SFN contains illegal characters. 						*/
    CHKDSK_LOGID_DDR_LFN_EXCEED_MAX=14,             /* DDR LFN exceeds max number of characters.  					*/
    CHKDSK_LOGID_DDR_LFN_CHKSUM=15,                 /* DDR LFN checksum doesn't match checksum on SFN. 				*/
    CHKDSK_LOGID_DDR_END_IDX = 15,                  /* This value must be equal to the last DDR issue. 				*/
    CHKDSK_LOGID_IFL_FILES_LEN=16,                  /* IFL Disk contains files with invalid lengths. 				*/
    CHKDSK_LOGID_IFL_FILES_LESS=17,                 /* IFL Calculated size is less than value in directory record. 	*/
    CHKDSK_LOGID_IFL_FILES_GREATER=18,              /* IFL Calculated size is greater than value in directory record.*/
    CHKDSK_LOGID_CROSS_LINKED=19,                   /* CLC Cross-linked clusters found. 							*/
    CHKDSK_LOGID_LOST_CL_CHAIN=20                   /* LCC Lost cluster chains found. 								*/

} CHKDSK_LOGID; /* Check disk logger id used to index into . */

/************************************************************************/
/* Structure Definitions                                                */
/************************************************************************/
typedef struct fat_table_trav_block_struct
{
	UINT16 dh;                  
	UINT32 start_fat_sector;        /* Sector that this FAT table starts at. */
	UINT32 current_fat_sector;		/* Current sector in the FAT table this block is describing.*/
	UINT32 last_fat_sector;	        /* Sector that this FAT table ends at. */	
    UINT16 fat_table;               /* Which FAT table this info block pertains to. */
    INT8 data[CHKDSK_SECTOR_SIZE];
	

}FAT_TABLE_TRAV_BLK;

/* Directory record location block. 
   This structure is used to traverse all the file and directories
   in the specified system. */

typedef struct dir_rec_loc_struct
{ 
	UINT16 offset;                  /* Offset in 512 byte array. */
	UINT32 cl_num;                  /* Cluster number on disk. */
	UINT32 sec_num;                 /* Sector location on disk. */ 
    UINT16  fat_12_16_root;

}DIR_REC_LOC; 

typedef struct dir_rec_object
{
    UINT16 dh;                                  /* Disk Handle. */ 
    DOSINODE *pdos_rec;                         /* SFN directory record block. */
	DIR_REC_LOC *pdos_rec_loc;                  /* Used to tell where the
                                                   dos_rec_ptr is located. */
    DIR_REC_LOC dos_rec_moms_loc;               /* Used to tell where the
                                                   dos_rec_ptr parent is located. */
    UINT8 disk_updated;                         /* Flag used to indicated if,
                                                   the disk was updated. */  
    DIR_REC_LOC lfn_start_loc;                  /* Start location of the lfn 
                                                   for this file. */                        

}DIR_REC_OBJ; 

/* Structure used to manage stack. */
typedef struct dir_rec_loc_stack_struct
{    
    DIR_REC_LOC *dir_trav_blk_array;
    UINT16 stack_top_offset;
    
}DIR_REC_LOC_STACK;

/* This structure is used to keep track of the number of a particular error
   that is encountered. */
typedef struct log_record_struct {
    CHAR *mess;
    UINT8 num;    
}LOG_RECORD;

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif   /* __CHKDSK_DEFS__ */

#endif   /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */


