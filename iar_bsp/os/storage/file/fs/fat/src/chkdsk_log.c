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
*       chkdsk_log.c
*
* COMPONENT
*
*       FAT Check Disk logger.
*
* DESCRIPTION
*
*       Logger used to report the errors found when running FAT's
*       Check Disk Utility.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       chk_start_error_logger              Creates log file.
*       chk_stop_error_logger               Closes log file.
*       chk_del_error_logger                Deletes log file.
*       chk_get_str_length                  Determines the length
*                                           of a string.
*       chk_start_test_case                 Writes test case header(tag)
*                                           to log file.
*       chk_write_mess                      Writes a message to the
*                                           log file.
*       chk_end_test_case                   Writes end of test case
*                                           tag to log file.
*       chk_log_add_ddr_iv_cl_err           Used to write Damage Directory
*                                           Record for Invalid Cluster
*                                           value to log file.
*       chk_log_write_errors_to_file        Writes log message at log id
*                                           to log file. 
*       chk_log_add_error                   Adds error message to log buffer.
*       chk_log_get_error_num               Returns current number of errors
*                                           found at for requested log entry.
*       chk_log_is_error_num_zero           Determines if number of errors
*                                           at log_id is zero.
*       
************************************************************************/
#include    "storage/chkdsk_extr.h"
#include    "storage/chkdsk_util.h"
#include    "storage/chkdsk_log.h"
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)

/************************************************************************/
/* String constants for the logger.                                     */
/************************************************************************/
#define START_ERROR_TEST_RESULT         "<START "
#define START_ERROR_STR_LEN             7

#define END_ERROR_TEST_RESULT           "<END TEST>"
#define END_ERROR_STR_LEN               10

#define TEST_DONE_FOR_LOST_CL           "LOST CLUSTER CHAINS TEST RESULTS>"
#define TEST_LOST_CL_STR_LEN            33

#define TEST_DONE_FOR_CROSS_LINKED      "CROSS-LINKED CHAINS TEST RESULTS>"
#define TEST_CROSS_LINKED_STR_LEN       33

#define TEST_DONE_FOR_FILES_SIZES       "FILES SIZES TEST RESULTS>"
#define TEST_FILES_SIZES_STR_LEN        25

#define TEST_DONE_FOR_DIR_RECORDS       "DIRECTORY RECORDS TEST RESULTS>"
#define TEST_DIR_RECORDS_STR_LEN        31

#define TEST_DONE_DAMAGED_FAT_TABLES    "COMPARED FAT TABLES TEST RESULTS>"
#define TEST_FAT_TABLE_STR_LEN          33

#define ERROR_MESS_HEADER               "<\"ERROR MESSAGE\" , NUMBER>"
#define ERROR_MESS_HEADER_LEN           26

#define START_ERROR_MESS                "<ERROR MESSAGE: "
#define START_ERROR_MESS_STR_LEN        16

#define START_TAG                       "<"
#define END_TAG                         ">"
#define TAG_STR_LEN                     1

#define MAX_ERROR_BUFF_SIZE             512
#define CR                              0x0D
#define LF                              0x0A

#define CHK_CR_LF_LEN                   2


/************************************************************************/
/* Marcos                                                               */
/************************************************************************/
#define CHK_ADD_CR_LF(str,str_cnt) str[str_cnt++] = CR;str[str_cnt++] = LF

/* Used to add a message to our buffer. */
#define CHK_ADD_MESS_TO_BUFF(str,str_len,add_str,add_str_len) NUF_Copybuff(&str[str_len],add_str,add_str_len); \
                                                              str_len += add_str_len                                                              

#define CHK_ADD_MESS_TO_BUFF_WITH_CRLF(str,str_len,str_add,str_add_len) CHK_ADD_MESS_TO_BUFF(str,str_len,str_add,str_add_len); \
                                                                        CHK_ADD_CR_LF(str,str_len)

/************************************************************************/
/* Global Data                                                          */
/************************************************************************/
INT g_chkdsk_fd;
static CHAR g_chkdsk_buff[MAX_ERROR_BUFF_SIZE];
static INT32 g_chkdsk_buff_cnt;    
static DDRIVE *g_chkdsk_pddrive;
/* Array of log record entries used to by each test to remember the 
   errors it encountered. Each test will write its log records to the
   log file at the end of there operation. */
LOG_RECORD g_chkdsk_log_rec[MAX_NUM_LOG_RECORD_ENTRIES];

/* The order of these error messages must match 
   CHKDSK_LOGID enum order. */
static CHAR *g_chkdsk_log_mess_tbl[MAX_NUM_LOG_RECORD_ENTRIES] = 
{
    "",
    FAT_TABLE_ERR_MESS,
    DOT_ERR_MESS,                
    DOT_DOT_ERR_MESS,                    
    DIR_SIZE_ERR_MESS,                   
    LFN_CL_ERR_MESS,                     
    CL_OUT_RANGE_ERR_MESS,               
    CL_VALUE_BAD_ERR_MESS,               
    CL_VALUE_FREE_ERR_MESS,              
    CL_VALUE_FREE_IN_CL_CHAIN_ERR_MESS,  
    CL_VALUE_RESERVED_ERR_MESS,          
    ROOT_DIR_RANGE_ERR_MESS,             
    ROOT_START_CL_ERR_MESS,              
    SFN_ILLEGAL_CHAR_ERR_MESS,           
    LFN_EXCEED_MAX_ERR_MESS,             
    LFN_CHKSUM_ERR_MESS,        
    FILES_LEN_ERR_MESS,                 
    FILES_LESS_ERR_MESS,  
    FILES_GREATER_ERR_MESS,
    CROSS_LINKED_ERR_MESS,              
    LOST_CL_CHAIN_ERR_MESS,         

};

/************************************************************************
* FUNCTION
*
*   chk_start_error_logger
*
* DESCRIPTION
*
*   Used to start the logger.     
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*   *pfile_name(in)             Pointer to a file name that will 
*                               be used as the log file.
*                               
* OUTPUTS
*
*   NU_SUCCESS                  Logger was started.
*
*************************************************************************/
STATUS chk_start_error_logger(UINT16 dh, CHAR *pfile_name)
{
    STATUS ret_val;
    UINT8 attr;
    FAT_CB *pfs_cb = NU_NULL;    

    if(pfile_name != NU_NULL)
    {
        ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
        if(ret_val == NU_SUCCESS)
        {
            g_chkdsk_pddrive = pfs_cb->ddrive;
        
            /* Make sure all our buffers are free. */
            pc_free_all_blk(g_chkdsk_pddrive);        

            /* Create our error log file. */
            g_chkdsk_fd = NU_Open(pfile_name,(PO_TEXT|PO_RDWR|PO_CREAT|PO_TRUNC),(PS_IWRITE | PS_IREAD));
            if(g_chkdsk_fd >= 0)
            {
                ret_val = NU_Get_Attributes(&attr,pfile_name);
                if(ret_val == NU_SUCCESS)
                {
                    attr |= ASYSTEM | AHIDDEN;
                    /* Make it a system file and hide it. */
                    ret_val = NU_Set_Attributes(pfile_name, attr);
                }
            }
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}
/************************************************************************
* FUNCTION
*
*   chk_stop_error_logger
*
* DESCRIPTION
*
*   Used to stop the logger.     
*          
*
* INPUTS
*  
*   None.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Logger was stopped.
*
*************************************************************************/
STATUS chk_stop_error_logger(VOID)
{
    STATUS ret_val;

    ret_val = NU_Close(g_chkdsk_fd);
    /* Clear our global buffer. */
    if(ret_val == NU_SUCCESS)
    {
        pc_memfill((VOID*)&g_chkdsk_buff[0],MAX_ERROR_BUFF_SIZE,0x00);
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_flush_error_logger
*
* DESCRIPTION
*
*   Used to flush possible log data from cache.     
*          
*
* INPUTS
*  
*   None.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Log data was flushed.
*
*************************************************************************/
STATUS chk_flush_error_logger(VOID)
{
    STATUS ret_val;

    ret_val = NU_Flush(g_chkdsk_fd);
    
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_del_error_logger
*
* DESCRIPTION
*
*   Used to delete the log file. This is called from fat_check_disk
*   whenever no errors are found.
*          
*
* INPUTS
*  
*   None.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Log file was deleted.
*
*************************************************************************/
STATUS chk_del_error_logger(CHAR *pfile_name)
{
    STATUS ret_val;    
    UINT8 attr;

    if(pfile_name != NU_NULL)
    {
        ret_val = NU_Get_Attributes(&attr,pfile_name);
        if(ret_val == NU_SUCCESS)
        {
            /* Need zero out system and hidden attribute, because we can't 
               delete a file that has those attributes. */        
            attr &= 0xF9;
            ret_val = NU_Set_Attributes(pfile_name,attr);
            if(ret_val == NU_SUCCESS)
            {
                ret_val = NU_Delete(pfile_name);
            }        
        }   
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_start_test_case
*
* DESCRIPTION
*
*   Used to write out a specific test case tag in the logger. This
*   tag is used to indicate that information following this tag pertains
*   to that tags test case.
*          
*
* INPUTS
*  
*   test_case_id(in)            The test case id.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  The test cases tags where
*                               written to the log successfully.
*
*************************************************************************/
STATUS chk_start_test_case(UINT8 test_case_id)
{
    STATUS ret_val = NU_SUCCESS;
    g_chkdsk_buff_cnt = 0;
    
    /* Copy the start test tag. */
    CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,START_ERROR_TEST_RESULT,START_ERROR_STR_LEN);

    /* Now copy the test specific tag. */
    if(test_case_id & CHK_CHECK_LOST_CL_CHAIN)    
    {
        CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,TEST_DONE_FOR_LOST_CL,TEST_LOST_CL_STR_LEN);        
    }
    else
    {
        if(test_case_id & CHK_CHECK_CROSS_LINKED_CHAIN)
        {
            CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,TEST_DONE_FOR_CROSS_LINKED,TEST_CROSS_LINKED_STR_LEN);
        }
        else
        {
            if(test_case_id & CHK_CHECK_FILES_SIZES)
            {
                CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,TEST_DONE_FOR_FILES_SIZES,TEST_FILES_SIZES_STR_LEN);
            }
            else
            {
                if(test_case_id & CHK_CHECK_DIR_RECORDS)
                {
                    CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,TEST_DONE_FOR_DIR_RECORDS,TEST_DIR_RECORDS_STR_LEN);
                }
                else
                {   
                    if(test_case_id & CHK_CHECK_FAT_TABLES)
                    {
                        CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,TEST_DONE_DAMAGED_FAT_TABLES,TEST_FAT_TABLE_STR_LEN);
                    }                    
                }                
            }            
        }
    }    
        
    CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,ERROR_MESS_HEADER,ERROR_MESS_HEADER_LEN);

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_write_mess
*
* DESCRIPTION
*
*   Used to write out an error message and the number of its occurrences.
*   If the buffer isn't full then the message is just added to it. However,
*   if the buffer is full it will be flushed and the message will then be
*   added to the buffer.
*          
*
* INPUTS
*  
*   pmess(in)                   Pointer to an error message that will be 
*                               written to the logger.
*   num(in)                     Number of this type of error occurring.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  pmess and num where successfully written
*                               to the logger.
*   NUF_INTERNAL                If the size written to the disk 
*                               doesn't equal the size expected
*                               to be written.
*
*************************************************************************/
STATUS chk_write_mess(CHAR *pmess, UINT16 num)
{
    STATUS ret_val = NU_SUCCESS;
    UINT32 mess_length;
    UINT8 num_str[CHK_MAX_DIGIT_IDX_IN_ERR_NAME];
    UINT8 i; 
    INT32 size_written;

    if(pmess != NU_NULL)
    {
        mess_length = NUF_Get_Str_Len(pmess);

        /* Make sure the message isn't going to put us over our limit,
           if it is then write it out to the disk. Add 3 to account 
           for the quotes around the string and a delimiter(comma)
           between the string and number of that error found. */
        if((mess_length + g_chkdsk_buff_cnt + CHK_CR_LF_LEN + 3) > MAX_ERROR_BUFF_SIZE)
        {           
            /* Make sure all our buffers are free. */
            pc_free_all_blk(g_chkdsk_pddrive); 

            size_written = NU_Write(g_chkdsk_fd,g_chkdsk_buff,g_chkdsk_buff_cnt);
            /* Set error condition */
            if(size_written != g_chkdsk_buff_cnt)
            {
                ret_val = NUF_INTERNAL;
            }

            /* Make sure we rest our buffer count. */
            g_chkdsk_buff_cnt = 0;
        }
        
        /* Add the start tag "<" */        
        CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,START_TAG,TAG_STR_LEN);

        /* Add the quote marks around the message, */
        CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,"\"",1);
        CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,pmess,mess_length);
        CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,"\",",2);

        /* Output the number. */
        chk_int_to_str((CHAR*)&num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,num);
    
        for(i = 0; i < CHK_MAX_DIGIT_IDX_IN_ERR_NAME; ++i)
        {
           if(num_str[i] == ' ')
               continue;
           else
           {
               CHK_ADD_MESS_TO_BUFF(g_chkdsk_buff,g_chkdsk_buff_cnt,&num_str[i],1);               
           }
               
        }
        /* Add the end tag ">". */
        CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,END_TAG,TAG_STR_LEN);        
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_end_test_case
*
* DESCRIPTION
*
*   Used to write out the test case end tag. This way the user knows
*   all messages between the start test case tag and end test case 
*   tag occurred while executing that test case.
*          
*
* INPUTS
*  
*   None.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  The test case tags where
*                               written to the log successfully.
*   NUF_INTERNAL                If the size written to the disk 
*                               doesn't equal the size expected
*                               to be written.
*
*************************************************************************/
STATUS chk_end_test_case(VOID)
{
    STATUS ret_val = NU_SUCCESS;
    INT32 size_written;

    /* Make sure we don't need to flush any data. */
    if((g_chkdsk_buff_cnt + END_ERROR_STR_LEN + CHK_CR_LF_LEN) > MAX_ERROR_BUFF_SIZE)
    {
        /* Make sure all our buffers are free. */
        pc_free_all_blk(g_chkdsk_pddrive); 

        /* Flush buffer so we can add end test tag. */
        size_written = NU_Write(g_chkdsk_fd,&g_chkdsk_buff[0],g_chkdsk_buff_cnt);
    
        if(size_written != g_chkdsk_buff_cnt)    
            ret_val = NUF_INTERNAL;

        /* Reset our count after writing. */
        g_chkdsk_buff_cnt = 0;
    }

    if(ret_val == NU_SUCCESS)
    {
        CHK_ADD_MESS_TO_BUFF_WITH_CRLF(g_chkdsk_buff,g_chkdsk_buff_cnt,END_ERROR_TEST_RESULT,END_ERROR_STR_LEN);       

        /* Make sure all our buffers are free. */
        pc_free_all_blk(g_chkdsk_pddrive); 

        size_written = NU_Write(g_chkdsk_fd,&g_chkdsk_buff[0],g_chkdsk_buff_cnt);    

        if(size_written != g_chkdsk_buff_cnt)    
            ret_val = NUF_INTERNAL;
        else
            ret_val = NU_Flush(g_chkdsk_fd);

        /* Reset our buffer count. */
        g_chkdsk_buff_cnt = 0;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_log_add_ddr_iv_cl_err
*
* DESCRIPTION
*
*   Used to add an Damage Directory Record, Invalid Cluster error to our
*   global error log. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value.
*                               
*
* OUTPUTS
*
*  None.
*
*************************************************************************/
VOID chk_log_add_ddr_iv_cl_err(DDRIVE *pddrive, UINT32 cl_value)
{    
    UINT32 cl_mask;

    if(pddrive != NU_NULL)
    {
        /* Determine our FAT cluster mask. */    
        cl_mask = chk_get_cl_mask(pddrive);

        /* If the cluster value isn't an eoc marker check its value,
           use < because 0x?FF8 - 0xFFF are all valid EOC and we
           only want to check clusters values that are less than this
           to make sure they are valid. */
          
        if(cl_value < (cl_mask & CHK_EOC))
        {       
            /* Check to see if cluster is bad. 
               (cl_mask & CHK_BAD_CL) = 0x?FF7 */
            if(chk_is_cl_bad(pddrive, cl_value) == NU_TRUE)
            {
                /* Log errors. */
                chk_log_add_error(CHKDSK_LOGID_DDR_CL_VALUE_BAD);                                    
            }
            else
            {            
                if(chk_is_cl_res(pddrive, cl_value) == NU_TRUE)
                {
                    /* Log Errors. */
                    chk_log_add_error(CHKDSK_LOGID_DDR_CL_VALUE_RESERVED);                                      
                }
                else
                {               
                    /* Make sure the cluster value doesn't indicate its free. */
                    if(chk_is_cl_free(pddrive, cl_value) == NU_TRUE)
                    {
                        /* Log Errors. */
                        chk_log_add_error(CHKDSK_LOGID_DDR_CL_VALUE_FREE_IN_CL_CHAIN);          
                    }
                    else
                    {
                        /* Make sure the cluster is in range.
                          (pddrive->numsecs /  pddrive->secpalloc) is equal to the number of clusters 
                          in the system. */
                        if(chk_is_cl_range(pddrive, cl_value) == NU_TRUE)
                        {
                            /* Log Errors. */
                            chk_log_add_error(CHKDSK_LOGID_DDR_CL_OUT_RANGE);                                   
                        }
                    }
                }
            }
        }        
    }
}

/************************************************************************
* FUNCTION
*
*   chk_log_write_errors_to_file
*
* DESCRIPTION
*
*   Writes current log message at log_id, into log file. 
*          
*
* INPUTS
*  
*   log_id(in)                Log id.
*                               
*
* OUTPUTS
*
*  NU_SUCCESS                   Message at mess_idx was written 
*                               to log file.
*
*************************************************************************/
STATUS chk_log_write_errors_to_file(CHKDSK_LOGID log_id)
{
    STATUS ret_val;
    
    ret_val = chk_write_mess(g_chkdsk_log_rec[log_id].mess, g_chkdsk_log_rec[log_id].num);
    
    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_log_add_error
*
* DESCRIPTION
*
*   Adds error message to log buffer. log_id is assumed to be valid, 
*   but the boundary is still tested. 
*          
*
* INPUTS
*  
*   log_id(in)                Log id.
*                               
*
* OUTPUTS
*
*   None
*
*************************************************************************/
VOID chk_log_add_error(CHKDSK_LOGID log_id)
{
    if(log_id < MAX_NUM_LOG_RECORD_ENTRIES)
    {
        if((CHKDSK_LOGID_IFL_FILES_GREATER == log_id) || (CHKDSK_LOGID_IFL_FILES_LESS == log_id))
        {
            if(g_chkdsk_log_rec[CHKDSK_LOGID_IFL_FILES_LEN].num == 0)
            {
                g_chkdsk_log_rec[CHKDSK_LOGID_IFL_FILES_LEN].mess = g_chkdsk_log_mess_tbl[CHKDSK_LOGID_IFL_FILES_LEN];
            }
            
            ++(g_chkdsk_log_rec[CHKDSK_LOGID_IFL_FILES_LEN].num);  

        }

        if(g_chkdsk_log_rec[log_id].num == 0)
        {
            g_chkdsk_log_rec[log_id].mess = g_chkdsk_log_mess_tbl[log_id];
        }
        
        ++(g_chkdsk_log_rec[log_id].num);
    }
}

/************************************************************************
* FUNCTION
*
*   chk_log_get_error_num
*
* DESCRIPTION
*
*   Returns the current number of errors found at log_id.  
*          
*
* INPUTS
*  
*  log_id(in)                Log id.
*                               
*
* OUTPUTS
*
*   None
*
*************************************************************************/
UINT8 chk_log_get_error_num(CHKDSK_LOGID log_id)
{
    return (g_chkdsk_log_rec[log_id].num);
}

/************************************************************************
* FUNCTION
*
*   chk_log_is_error_num_zero
*
* DESCRIPTION
*
*   Returns a boolean to indicate if number of error messages at
*   log_id is zero or not. 
*          
*
* INPUTS
*  
*  log_id(in)                Log id.
*                               
*
* OUTPUTS
*
*   NU_TRUE                     There aren't any messages at mess_idx.
*   NU_FALSE                    mess_idx does contain errors. 
*
*************************************************************************/
UINT8 chk_log_is_error_num_zero(CHKDSK_LOGID log_id)
{
    UINT8 b_ret_val = NU_FALSE;
    
    if(g_chkdsk_log_rec[log_id].num == 0)
    {
        b_ret_val = NU_TRUE;
    }

    return (b_ret_val);
}

#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

