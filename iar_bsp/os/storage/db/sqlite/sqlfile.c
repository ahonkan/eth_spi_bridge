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
*     sqlfile.c
*
* COMPONENT
*     Nucleus SQLite.
*
* DESCRIPTION
*     This file contains definitions of mutex protected files.
*
* DATA STRUCTURES
*     
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
*     stdio.h
*     string.h
*     sqliteInt.h
*     sqlfile.h
*     mutex_nucleus.h
*
**************************************************************************/

#include <stdio.h>
#include <string.h>
#include "sqliteInt.h"
#include "../sqlfile.h"
#include "mutex_nucleus.h"

struct SQLITE_SQL_FILE_STRUCT Sql_File_List[SQLITE_MAX_SQL_FILE];

struct SQLITE_BINARY_MUTEX_STRUCT Sql_File_Mutex;

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Init_SQL_File
*
*   DESCRIPTION
*
*       Performs SQLite specific file initialization.
*
*   CALLED BY
*
*       SQLITE_Init_Sqlite
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       1				error
*       NU_SUCCESS		success
*
*
***********************************************************************/
STATUS SQLITE_Init_SQL_File(VOID)
{
    static INT32 called_once = 0L;
    INT32 i;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return 1;
    }
    
    if(called_once == 0L)
    {
        called_once = 1L;
        for(i=0; i < SQLITE_MAX_SQL_FILE; i++)
        {
            Sql_File_List[i].is_allocated = 0L;
            Sql_File_List[i].file_name[0] = '\0';
            Sql_File_List[i].task_counter = 0L;
            Sql_File_List[i].mutex_ptr = NU_NULL;
            Sql_File_List[i].mutex_created = 0L;
            Sql_File_List[i].global_data_size = 0L;
            Sql_File_List[i].global_data_ptr = NU_NULL;
        }
        return SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
    }
    return SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
}

/***********************************************************************
*
*   FUNCTION
*
*       Create_File_Mutex
*
*   DESCRIPTION
*
*       Creates file mutex
*
*   CALLED BY
*
*       SQLITE_Get_File_Mutex
*
*   CALLS
*
*       strcpy
*       sprintf
*       strcat
*       sqlite3_malloc
*       SQLITE_Create_Binary_Mutex
*
*   INPUTS
*
*       file_index
*
*   OUTPUTS
*
*       0			success
*       1			binary mutex error
*       2			memory allocation error
*
*
***********************************************************************/
static STATUS Create_File_Mutex(const INT32 file_index)
{
    CHAR mutex_name[16];
    CHAR ztemp[8];
    STATUS rc;
    struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr;
    mutex_ptr = NU_NULL;
    if(!(Sql_File_List[file_index].task_counter) && 
        !(Sql_File_List[file_index].mutex_created))
    {
        strcpy(mutex_name,"MU");
        if(SQLITE_MAX_SQL_FILE<100)
        {
            sprintf(ztemp,"%02d",file_index);
        }
        else
        {
            sprintf(ztemp,"%03d",file_index);
        }
        strcat(mutex_name,ztemp);
        mutex_ptr = (sqlite_bin_mutex*)sqlite3_malloc(sizeof(struct SQLITE_BINARY_MUTEX_STRUCT));
        if(mutex_ptr!=NU_NULL)
        {
            rc =  SQLITE_Create_Binary_Mutex(mutex_ptr,mutex_name);
            if(rc == NU_SUCCESS)
            {
                Sql_File_List[file_index].mutex_created = SQLITE_BIN_MUTEX_CREATED_TRUE;
                Sql_File_List[file_index].mutex_ptr = mutex_ptr;
                return (0);
            }
            else
            {
                return (1);
            }
        }
        else
        {
            return (1);
        }
    }
    else
    {
        return (2);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       Create_File_Shared_Memory
*
*   DESCRIPTION
*
*       creates file shared memory
*
*   CALLED BY
*
*       SQLITE_Get_File_Shared_Memory
*
*   CALLS
*
*       sqlite3_malloc
*
*   INPUTS
*
*       file_index
*		file_data_size
*
*   OUTPUTS
*
*       0			success
*       1			memory allocation error
*       2			invalid index
*
***********************************************************************/
static STATUS Create_File_Shared_Memory(const INT32 file_index, const INT32 file_data_size)
{
    void * shared_data_ptr;
    if(Sql_File_List[file_index].global_data_ptr == NU_NULL)
    {
        shared_data_ptr = sqlite3_malloc(file_data_size);
        if(shared_data_ptr!=NU_NULL)
        {
            Sql_File_List[file_index].global_data_size =  file_data_size;
            Sql_File_List[file_index].global_data_ptr = shared_data_ptr;
            return (0);
        }
        else
        {
            return (1);
        }
    }
    else
    {
        return (2);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       Register_New_File
*
*   DESCRIPTION
*
*       
*
*   CALLED BY
*
*       SQLITE_Get_File_Mutex
*       SQLITE_Get_File_Shared_Memory
*
*   CALLS
*
*       strlen
*       strcpy
*
*   INPUTS
*
*       new_file_name
*		new_file_index
*
*   OUTPUTS
*
*       0			success
*       1			invalid length of file name
*       2			unallocated index
*
*
***********************************************************************/
static STATUS Register_New_File(const char *new_file_name,const INT32 new_file_index)
{
    INT32 file_name_length = 0L;
    file_name_length = strlen(new_file_name);
    if(file_name_length<SQLITE_MAX_SQL_FILE_NAME_LENGTH)
    {
        if((Sql_File_List[new_file_index].is_allocated) == 0)
        {
            Sql_File_List[new_file_index].is_allocated = 1L;
            strcpy(Sql_File_List[new_file_index].file_name,new_file_name);
            return (0);
        }
        else
        {
            return (2);
        }
    }
    else
    {
        return (1);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       Find_File_In_List
*
*   DESCRIPTION
*
*       retrieves file index
*
*   CALLED BY
*
*       SQLITE_Get_File_Mutex
*       SQLITE_Close_File_Mutex
*       SQLITE_Get_File_Shared_Memory
*       SQLITE_Close_File_Shared_Memory
*
*   CALLS
*
*       strcmp
*
*   INPUTS
*
*       find_file_name
*       index_for_new_file
*
*   OUTPUTS
*
*       index
*
***********************************************************************/
static INT32 Find_File_In_List(const char *find_file_name,INT32 *index_for_new_file)
{
    INT32 i,j,k;
    j = SQLITE_MAX_SQL_FILE;
    k = SQLITE_MAX_SQL_FILE;
    for(i = 0L; i < SQLITE_MAX_SQL_FILE; i++)
    {
        if(Sql_File_List[i].is_allocated == 1)
        {
            if(0 == strcmp(find_file_name,Sql_File_List[i].file_name))
            {
                k = i;
                break;
            }
        }
        else
        {
            if(j > i)
            {
                j = i;
            }
        }
    }
    *index_for_new_file = j;
    return (k);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Get_File_Mutex
*
*   DESCRIPTION
*
*       gets file mutex
*
*   CALLED BY
*
*       nucleusCreateLock
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       Find_File_In_List
*       Register_New_File
*       Create_File_Mutex
*       SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       file_name
*       created_flag
*
*   OUTPUTS
*
*       NU_NULL			in case of error, valid pointer otherwise
*
***********************************************************************/
struct SQLITE_BINARY_MUTEX_STRUCT * SQLITE_Get_File_Mutex(const char *file_name,INT32 *created_flag)
{
    INT32 i,j;
    STATUS rc;
    struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (NU_NULL);
    }
    
    i = Find_File_In_List(file_name,&j);
    
    if(i == SQLITE_MAX_SQL_FILE) /*this file does not exist already*/
    {
        if(j<SQLITE_MAX_SQL_FILE) /*this location is free*/
        {
            if(0 != Register_New_File(file_name,j))
            {
                return (NU_NULL);
            }
            
            rc = Create_File_Mutex(j);
            if(rc == 0)
            {
                *created_flag = 1;
                mutex_ptr = Sql_File_List[j].mutex_ptr;
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (mutex_ptr);
            }
            else
            {
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (NU_NULL);
            }
        }
        else /*no more files can be occupied*/
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (NU_NULL);
        }
    }
    else
    {
        if(Sql_File_List[i].mutex_created)
        {
            *created_flag = 0;
            mutex_ptr = Sql_File_List[i].mutex_ptr;
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (mutex_ptr);
        }
        else
        {
            rc = Create_File_Mutex(i);
            if(rc == 0)
            {
                *created_flag = 1;
                mutex_ptr = Sql_File_List[i].mutex_ptr;
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (mutex_ptr);
            }
            else
            {
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (NU_NULL);
            }
        }
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Close_File_Mutex
*
*   DESCRIPTION
*
*       closes file mutex
*
*   CALLED BY
*
*       nucleusCreateLock
*       nucleusDestroyLock
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       Find_File_In_List
*       SQLITE_Binary_Mutex_Delete
*       SQLITE_Binary_Mutex_UnLock
*		sqlite3_free
*
*   INPUTS
*
*       file_name
*
*   OUTPUTS
*
*       0		success
*       1		binary mutex error
*       2		other tasks using file
*
*
***********************************************************************/
STATUS SQLITE_Close_File_Mutex(const char *file_name)
{
    INT32 i,j;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (1);
    }
    
    i = Find_File_In_List(file_name,&j);

    if(i < SQLITE_MAX_SQL_FILE)
    {
        /*Is it the last task that is using this mutex*/
        if(!(Sql_File_List[i].task_counter) &&
            Sql_File_List[i].mutex_created &&
            Sql_File_List[i].mutex_ptr &&
            !(Sql_File_List[i].global_data_ptr)
            )
        {   
            SQLITE_Binary_Mutex_Delete(Sql_File_List[i].mutex_ptr);
            sqlite3_free(Sql_File_List[i].mutex_ptr);
            Sql_File_List[i].mutex_created = 0L;
            Sql_File_List[i].mutex_ptr = NU_NULL;
            Sql_File_List[i].file_name[0] = '\0';
            Sql_File_List[i].is_allocated = 0L;
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (0);
        }
        else
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (2);
        }
    }
    else
    {
        SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
        return (1);
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Close_File_Mutex_Indexed
*
*   DESCRIPTION
*
*       closes file mutex indexed
*
*   CALLED BY
*
*       nucleusDestroyLock
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       sqlite3_free
*		SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       index
*
*   OUTPUTS
*
*       0		success
*		1		binary mutex error
*		2		other tasks using file
*
*
***********************************************************************/
STATUS SQLITE_Close_File_Mutex_Indexed(const INT32 index)
{
    INT32 i;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (1);
    }
    
    i = index;

    if((i < SQLITE_MAX_SQL_FILE) && (i > -1))
    {
        /*Is it the last task that is using this mutex*/
        if(!(Sql_File_List[i].task_counter) &&
            Sql_File_List[i].mutex_created &&
            Sql_File_List[i].mutex_ptr &&
            !(Sql_File_List[i].global_data_ptr)
            )
        {   
            SQLITE_Binary_Mutex_Delete(Sql_File_List[i].mutex_ptr);
            sqlite3_free(Sql_File_List[i].mutex_ptr);
            Sql_File_List[i].mutex_created = 0L;
            Sql_File_List[i].mutex_ptr = NU_NULL;
            Sql_File_List[i].file_name[0] = '\0';
            Sql_File_List[i].is_allocated = 0L;
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (0);
        }
        else
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (2);
        }
    }
    else
    {
        SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
        return (1);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Get_File_Shared_Memory
*
*   DESCRIPTION
*
*       gets a shared memory file
*
*   CALLED BY
*
*       nucleusCreateLock
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       Find_File_In_List
*       Register_New_File
*       Create_File_Shared_Memory
*       SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       file_name
*		memory_size
*		created_flag
*		assigned_index
*
*   OUTPUTS
*
*       NU_NULL			in case of error, valid pointer otherwise.
*
*
***********************************************************************/
void * SQLITE_Get_File_Shared_Memory(const char *file_name,INT32 memory_size,INT32 *created_flag, INT32 *assigned_index)
{
    INT32 i,j;
    STATUS rc;
    void * data_ptr;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (NU_NULL);
    }
    
    i = Find_File_In_List(file_name,&j);

    *assigned_index = -1;

    if(i == SQLITE_MAX_SQL_FILE) /*this file does not exist already*/
    {
        if(j < SQLITE_MAX_SQL_FILE) /*this location is free*/
        {
            Register_New_File(file_name,j);
            rc = Create_File_Shared_Memory(j,memory_size);
            if(rc == 0)
            {
                *created_flag = 1;
                Sql_File_List[j].task_counter++;
                data_ptr = Sql_File_List[j].global_data_ptr;
                *assigned_index = j;
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (data_ptr);
            }
            else
            {
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (NU_NULL);
            }
        }
        else /*no more files can be occupied*/
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (NU_NULL);
        }
    }
    else
    {
        if(Sql_File_List[i].global_data_ptr != NU_NULL)
        {
            *created_flag = 0;
            Sql_File_List[i].task_counter++;
            data_ptr = Sql_File_List[i].global_data_ptr;
            *assigned_index = i;
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (data_ptr);
        }
        else
        {
            rc = Create_File_Shared_Memory(i,memory_size);
            if(rc == 0)
            {
                *created_flag = 1;
                Sql_File_List[i].task_counter++;
                data_ptr = Sql_File_List[i].global_data_ptr;
                *assigned_index = i;
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (data_ptr);
            }
            else
            {
                SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
                return (NU_NULL);
            }
        }
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Close_File_Shared_Memory
*
*   DESCRIPTION
*
*       closes shared file memory
*
*   CALLED BY
*
*       None.
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*		Find_File_In_List
*		sqlite3_free
*		SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       file_name
*       memory_size
*       is_last
*
*   OUTPUTS
*
*       0		success
*       1		binary mutex error
*       2		invalid index
*
*
***********************************************************************/
STATUS SQLITE_Close_File_Shared_Memory(const char *file_name,INT32 memory_size, INT32 *is_last)
{
    INT32 i,j;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (1);
    }
    
    i = Find_File_In_List(file_name,&j);

    *is_last = 0;
    if(i < SQLITE_MAX_SQL_FILE)
    {
        if(Sql_File_List[i].task_counter && 
            Sql_File_List[i].global_data_ptr
            )
        {
            Sql_File_List[i].task_counter--;
            
            /*It is the last task that is using this shared memory*/
            if(Sql_File_List[i].task_counter == 0) 
            {
                sqlite3_free(Sql_File_List[i].global_data_ptr);
                Sql_File_List[i].global_data_size = 0L;
                Sql_File_List[i].global_data_ptr = NU_NULL;
                *is_last = 1L;
            }
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (0);
        }
        else
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (2);
        }
    }
    else
    {
        SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
        return (1);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Close_File_Shared_Memory_Indexed
*
*   DESCRIPTION
*
*       closes shared file memory
*
*   CALLED BY
*
*       None.
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*		Find_File_In_List
*		sqlite3_free
*		SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       file_name
*       memory_size
*       is_last
*
*   OUTPUTS
*
*       0		success
*       1		binary mutex error
*       2		invalid index
*
*
***********************************************************************/
STATUS SQLITE_Close_File_Shared_Memory_Indexed(const INT32 index, INT32 memory_size, INT32 *is_last)
{
    INT32 i;

    if(NU_SUCCESS != SQLITE_Binary_Mutex_Lock(&Sql_File_Mutex))
    {
        return (1);
    }
    
    i = index;

    *is_last = 0;
    if((i < SQLITE_MAX_SQL_FILE) && (i > -1))
    {
        if(Sql_File_List[i].task_counter && 
            Sql_File_List[i].global_data_ptr
            )
        {
            Sql_File_List[i].task_counter--;
            
            /*It is the last task that is using this shared memory*/
            if(Sql_File_List[i].task_counter == 0) 
            {
                sqlite3_free(Sql_File_List[i].global_data_ptr);
                Sql_File_List[i].global_data_size = 0L;
                Sql_File_List[i].global_data_ptr = NU_NULL;
                *is_last = 1L;
            }
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (0);
        }
        else
        {
            SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
            return (2);
        }
    }
    else
    {
        SQLITE_Binary_Mutex_UnLock(&Sql_File_Mutex);
        return (1);
    }
}
