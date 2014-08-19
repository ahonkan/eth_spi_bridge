/**************************************************************************
*                                                                         *
*                Copyright 2008 Mentor Graphics Corporation               *
*                          All Rights Reserved.                           *
*                                                                         *
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS   *
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS    *
*  SUBJECT TO LICENSE TERMS.                                              *
*                                                                         *
***************************************************************************

***************************************************************************
* FILE NAME                                    
*     sqlfile.h
*
* COMPONENT
*     Nucleus SQLite.
*
* DESCRIPTION
*     This file contains declarations of mutex protected files.
*
* DATA STRUCTURES
*     SQLITE_SQL_FILE_STRUCT
*
* FUNCTIONS
*     SQLITE_Init_SQL_File
*     SQLITE_Get_File_Mutex
*     SQLITE_Close_File_Mutex
*     SQLITE_Close_File_Mutex_Indexed
*     SQLITE_Get_File_Shared_Memory
*     SQLITE_Close_File_Shared_Memory
*     SQLITE_Close_File_Shared_Memory_Indexed
*
* DEPENDENCIES
*     nu_storage.h
*
**************************************************************************/

#ifndef SQLITE_SQLFILE_H_
#define SQLITE_SQLFILE_H_

#include "storage/nu_storage.h"

#ifndef SQLITE_MAX_SQL_FILE
#define SQLITE_MAX_SQL_FILE 8L
#endif

#ifndef SQLITE_MAX_SQL_FILE_NAME_LENGTH
#define SQLITE_MAX_SQL_FILE_NAME_LENGTH 128
#endif

#ifdef __cplusplus

/* C declarations in C++     */
extern "C" {

#endif


struct SQLITE_SQL_FILE_STRUCT
{
    INT32 is_allocated;
    CHAR file_name[SQLITE_MAX_SQL_FILE_NAME_LENGTH];
    INT32 task_counter;
    struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr;
    INT32 mutex_created;
    INT32 global_data_size;
    void * global_data_ptr;
};

STATUS SQLITE_Init_SQL_File(VOID);
struct SQLITE_BINARY_MUTEX_STRUCT * SQLITE_Get_File_Mutex(const char *file_name,INT32 *created_flag);
STATUS SQLITE_Close_File_Mutex(const char *file_name);
STATUS SQLITE_Close_File_Mutex_Indexed(const INT32 index);

void * SQLITE_Get_File_Shared_Memory(const char *file_name,INT32 memory_size,INT32 *created_flag, INT32 *assigned_index);
STATUS SQLITE_Close_File_Shared_Memory(const char *file_name,INT32 memory_size, INT32 *is_last);
STATUS SQLITE_Close_File_Shared_Memory_Indexed(const INT32 index, INT32 memory_size, INT32 *is_last);

#ifdef __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
