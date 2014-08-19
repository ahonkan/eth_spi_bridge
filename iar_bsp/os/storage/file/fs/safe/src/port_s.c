/*************************************************************************/
/*                                                                       */
/*               Copyright 2007 Mentor Graphics Corporation              */
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
*       port_s.c
*
* COMPONENT
*
*       Nucleus Safe File System 
*
* DESCRIPTION
*
*       Contains the time and date functions that will need to be
*       ported by the user.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       fs_getdate                          Retrieves current date.
*       fs_gettime                          Retrieves current time.
*       fs_mutex_create                     Creates a mutex.
*       fs_mutex_get                        Gets a mutex.
*       fs_mutex_put                        Release a mutex.
*       fn_gettaskID                        Gets current task ID.
*
************************************************************************/

#include "storage/fsf.h"
#if(USE_VFS)
#include "storage/user_defs.h"
#include "storage/user_extr.h"
#endif

/****************************************************************************
 *
 * fs_getcurrenttimedate
 *
 * need to be ported depending on system, it retreives the
 * current time and date in DOS format. User must solve roll-over when reading
 * time and date. Roll-over problem to read a date at 23.59.59 and then reading time at
 * 00:00.00.
 *
 * INPUTS
 *
 * ptime - pointer where to store time or 0 if don't store time
 * pdata - pointer where to store date or 0 if don't store date
 *
 ***************************************************************************/
 
#if (!FS_CAPI_USED)
void fs_getcurrenttimedate(unsigned short *ptime, unsigned short *pdate)
{
	unsigned short hour=19;
	unsigned short min=37;
	unsigned short sec=14;

	unsigned short time= (unsigned short)(((hour     <<  FS_CTIME_HOUR_SHIFT) & FS_CTIME_HOUR_MASK) |
						   ((min      <<  FS_CTIME_MIN_SHIFT)  & FS_CTIME_MIN_MASK) |
						   (((sec) <<  FS_CTIME_SEC_SHIFT)  & FS_CTIME_SEC_MASK));

	unsigned short year=1988;
	unsigned short month=3;
	unsigned short day=28;

	unsigned short date= (unsigned short)((((unsigned short)(year-1980) <<  FS_CDATE_YEAR_SHIFT)  & FS_CDATE_YEAR_MASK) |
						   ((month		 <<  FS_CDATE_MONTH_SHIFT) & FS_CDATE_MONTH_MASK) |
						   ((day		 <<  FS_CDATE_DAY_SHIFT)   & FS_CDATE_DAY_MASK));

	if (ptime)
	{
		*ptime = time;
	}

	if (pdate)
	{
		*pdate = date;
	}
}
#endif


/****************************************************************************
 *
 * fs_mutex_create
 *
 * user function to create a mutex.
 *
 * RETURNS
 *   0 - success
 *   1 - error
 *
 ***************************************************************************/

#if (!FS_CAPI_USED)
int fs_mutex_create (FS_MUTEX_TYPE *mutex)
{
	fsm_memset(mutex, 0, sizeof(FS_MUTEX_TYPE));
	if(NU_Create_Semaphore(mutex, "fs_safe_mutex", 1, NU_FIFO) == NU_SUCCESS)
        return 0;
    else
        return 1;
}
#endif

/****************************************************************************
 *
 * fs_mutex_delete
 *
 * user function to delete a mutex.
 *
 * RETURNS
 *   0 - success
 *   1 - error
 *
 ***************************************************************************/

#if (!FS_CAPI_USED)
int fs_mutex_delete (FS_MUTEX_TYPE *mutex)
{
	if(NU_Delete_Semaphore(mutex) == NU_SUCCESS)
        return 0;
    else
        return 1;
}
#endif

/****************************************************************************
 *
 * fs_mutex_get
 *
 * user function to get a mutex.
 *
 * RETURNS
 *   0 - success
 *   1 - error
 *
 ***************************************************************************/

#if (!FS_CAPI_USED)
int fs_mutex_get (FS_MUTEX_TYPE *mutex)
{
	if(NU_Obtain_Semaphore(mutex, NU_SUSPEND) == NU_SUCCESS)
        return 0;
    else
        return 1;
}
#endif

/****************************************************************************
 *
 * fs_mutex_put
 *
 * user function to release a mutex.
 *
 * RETURNS
 *   0 - success
 *   1 - error
 *
 ***************************************************************************/

#if (!FS_CAPI_USED)
int fs_mutex_put (FS_MUTEX_TYPE *mutex)
{
	if(NU_Release_Semaphore(mutex) == NU_SUCCESS)
        return 0;
    else
        return 1;

}
#endif

/****************************************************************************
 *
 * fn_gettaskID
 *
 * user function to get current task ID, zero cannot be a valid task ID
 *
 * RETURNS
 *
 * task ID
 *
 ***************************************************************************/

#if (!FS_CAPI_USED)
#ifndef _FN_GETTASKID_

long fn_gettaskID(void)
{
	
#if(USE_VFS)
	return fs_user->context_handle;
#else
    return ((long)NU_Current_Task_Pointer());
#endif
}

#endif
#endif

/****************************************************************************
 *
 * end of port_s.c
 *
 ***************************************************************************/
