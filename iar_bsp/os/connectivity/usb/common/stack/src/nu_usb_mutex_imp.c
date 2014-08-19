/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME 
 * 
 *      nu_usb_mutex_imp.c 
 * 
 * COMPONENT 
 *        OS Services: Mutex 
 * 
 * DESCRIPTION 
 *      Mutex related data structure and functions are deprecated. 
 *
 *      This file contains the implementation of mutex that a task can 
 * gain exclusive access to and once grabbed, it can grab again any 
 * no. of times without getting blocked. So unlike a counting semaphore, 
 * this binary semaphore's count is decremented, only when its grabbed 
 * for the first time. And is again incremented only when its released 
 * by equal no. of times. 
 * 
 * 
 * DATA STRUCTURES 
 *     None 
 * 
 * FUNCTIONS 
 *    usb_create_mutex   Creates the mutex object. 
 *    usb_obtain_mutex   A task gains exclusive access to mutex through 
 *                          this API. 
 *    usb_release_mutex  Relinquish exclusive ownership of the mutex. 
 *    usb_destroy_mutex  It destroys the mutex object 
 * 
 * DEPENDENCIES 
 *    nu_usb.h               All USB definitions 
 * 
 ************************************************************************/
#ifndef USB_MUTEX_IMP_C
#define	USB_MUTEX_IMP_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/************************************************************************ 
 * 
 * FUNCTION 
 *     usb_create_mutex 
 * 
 * DESCRIPTION 
 *     This function creates mutex that can be recursively grabbed 
 *     multiple times by the the same task, without getting blocked. And 
 *     also they ensure mutual exclusion among competing tasks. It makes 
 *     use of 2 binary semaphores of Nucleus Plus, to achieve this 
 *     objective. 
 * 
 * INPUTS 
 *    OS_Mutex  *mutex            A ptr to mutex object 
 *    CHAR      *name             Name of the Mutex 
 * OUTPUTS 
 *    NU_SUCCESS                  Creation Successful. 
 *    NU_INVALID_SEMAPHORE        mutex object is invalid. 
 *
 *************************************************************************/
STATUS usb_create_mutex (OS_Mutex * mutex,
                         CHAR * name)
{
    STATUS status;

	if (mutex == NU_NULL
	    || name == NU_NULL)
        return NU_INVALID_POINTER;
    status = NU_Create_Semaphore (&(mutex->sem1), name, 1, NU_PRIORITY);
    if (status != NU_SUCCESS)
        return (status);
    status = NU_Create_Semaphore (&(mutex->sem2), name, 1, NU_PRIORITY);
    if (status != NU_SUCCESS)
    {
        status = NU_Delete_Semaphore (&(mutex->sem1));
        return (status);
    }
    mutex->task = NU_NULL;
    mutex->recursive_cnt = 0;
    return (NU_SUCCESS);
}

/************************************************************************* 
 * 
 * FUNCTION 
 *   usb_obtain_mutex 
 * 
 * DESCRIPTION 
 *   This function is used to obtain mutex. Unlike a binary semaphore 
 *   a task can obtain this mutex any no.of times without getting blocked 
 *   and is expected to release it equal no. of times to loose ownership 
 *   of it. The caller gets blocked,if the mutex is currently held by a 
 *   different task. 
 * 
 * INPUTS 
 *    OS_Mutex    *mutex   A ptr to the mutex object 
 * 
 * OUTPUTS 
 *    NU_SUCCESS     the calling task now has an exclusive access to the 
 *                   mutex. 
 *    NU_INVALID_SEMAPHORE the mutex object is invalid 
 * 
 *************************************************************************/
STATUS usb_obtain_mutex (OS_Mutex * mutex)
{
    STATUS status;

	if (mutex == NU_NULL)
        return NU_INVALID_POINTER;
    /* The objective is to get an exclusive hold of mutex->sem2 recursively
     * for any no.of times. mutex->sem1 is used to ensure exclusive read/write
     * on mutex->recursive_cnt, mutex->task. Lock grabbing order is mutex->sem2
     * and then mutex->sem1, to prevent deadlocks.
     */
    status = NU_Obtain_Semaphore (&(mutex->sem1), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    if (mutex->task == NU_NULL)
    {                           /* Resource is available. Grab it */
        status = NU_Release_Semaphore (&(mutex->sem1));
        status |= NU_Obtain_Semaphore (&(mutex->sem2), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            return (status);
        }
        status = NU_Obtain_Semaphore (&(mutex->sem1), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            status = NU_Release_Semaphore (&(mutex->sem2));
            return (status);
        }
        mutex->task = NU_Current_Task_Pointer ();
        mutex->recursive_cnt++;
        status = NU_Release_Semaphore (&(mutex->sem1));
        /* We are now holding Sem2 */
        return (status);
    }
    if (mutex->task == NU_Current_Task_Pointer ())
    {
        mutex->recursive_cnt++;
        /* We are already holding Sem2 */
        status = NU_Release_Semaphore (&(mutex->sem1));
        return (status);
    }
    /* someone else is holding Sem2 */
    status = NU_Release_Semaphore (&(mutex->sem1));
    status |= NU_Obtain_Semaphore (&(mutex->sem2), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    status = NU_Obtain_Semaphore (&(mutex->sem1), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    mutex->recursive_cnt++;
    mutex->task = NU_Current_Task_Pointer ();

    /* We are now holding Sem2 */
    status = NU_Release_Semaphore (&(mutex->sem1));
    return (status);
}

/************************************************************************* 
 * 
 * FUNCTION 
 *    usb_release_mutex 
 * 
 * DESCRIPTION 
 *   The function relinquishes the calling task's exclusive access to the 
 *   mutex object,only if it has released the same no.of times it did a 
 *   usb_obtain_mutex 
 * 
 * INPUTS 
 *   OS_Mutex   *mutex          Ptr to the mutex object 
 * 
 * OUTPUTS 
 *  STATUS  NU_SUCCESS      Release Cnt is decremented and if its 0 
 *                              the calling task lost exclusive access to 
 *                              the mutex object. 
 *          NU_INVALID_SEMAPHORE   The ptr to mutex object is invalid 
 * 
  *************************************************************************/
STATUS usb_release_mutex (OS_Mutex * mutex)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
	
	if (mutex == NU_NULL)
        return NU_INVALID_POINTER;
		
    status = NU_Obtain_Semaphore (&(mutex->sem1), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* The task that never owned it is attempting a 'release'!! */
    if (mutex->task != NU_Current_Task_Pointer ())
        NU_USB_ASSERT (0);

    /* Decrement the ownership cnt of the task */
    mutex->recursive_cnt--;
    if (mutex->recursive_cnt == 0)
    {
        /* Release the ownership on the mutex */
        mutex->task = NU_NULL;
        internal_sts |= NU_Release_Semaphore (&(mutex->sem1));
        internal_sts |= NU_Release_Semaphore (&(mutex->sem2));
		NU_UNUSED_PARAM(internal_sts);
        return (NU_SUCCESS);
    }
    /* We are still holding Sem2, so the task has still the exclusive ownership */
    status = NU_Release_Semaphore (&(mutex->sem1));
    return (status);
}

/************************************************************************ 
 * 
 * FUNCTION 
 *    usb_destroy_mutex 
 * 
 * DESCRIPTION 
 *    It destroys the mutex object 
 * 
 * INPUTS 
 *   OS_Mutex   *mutex          Ptr to the mutex object 
 * 
 * OUTPUTS 
 *   STATUS     NU_SUCCESS      Mutex object is destroyed. 
 *              NU_INVALID_POINTER  mutex passed is NU_NULL 
 *              NU_INVALID_SEMAPHORE The ptr to mutex object is invalid 
 *
 *************************************************************************/
STATUS usb_destroy_mutex (OS_Mutex * mutex)
{
    STATUS status ;

    if (mutex == NU_NULL)
        return NU_INVALID_POINTER;

    /* free the  kernel resources */
    status  = NU_Delete_Semaphore (&(mutex->sem1));
    status |= NU_Delete_Semaphore (&(mutex->sem2));
    return (status);
}

/*************************************************************************/

#endif /* USB_MUTEX_IMP_C */
/*************************** end of file ********************************/

