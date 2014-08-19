/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       proc_libc.c
*
*   COMPONENT
*
*       Nucleus Processes - User
*
*   DESCRIPTION
*
*       Support for user c library.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       atexit
*       exit
*       abort
*       __cxa_pure_virtual
*       __cxa_atexit
*       call_cxa_atexit_funcs
*       call_atexit_funcs
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"

#if (CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT == NU_TRUE)

/* C++ Static Object support. */
extern void (*_cxx_ctor_start[])();         /* Symbol taken from linker command file. */
extern void (*_cxx_ctor_end[])();           /* Symbol taken from linker command file. */

/* __cxa_atexit() support. */
typedef struct cxa_atexit_func_entry_struct
{
    void    (*destructor_func)(void *);
    void    *obj_ptr;

} CXA_ATEXIT_FUNC_ENTRY;

/* External variables */
extern  CXA_ATEXIT_FUNC_ENTRY   _cxa_atexit_funcs[];

/* C++ atexit function count */
static  UNSIGNED                proc_cxa_atexit_func_count;

#endif /* CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT */

/* Local variables */
static  VOID                    (*proc_atexit_funcs[CFG_NU_OS_KERN_PROCESS_USER_ATEXIT_MAX_FUNCS])(VOID);
static  INT                     proc_atexit_func_count;


/*************************************************************************
*
*   FUNCTION
*
*       atexit
*
*   DESCRIPTION
*
*       atexit() implementation for Nucleus process runtime.
*
*   INPUTS
*
*       func - pointer to the function to be called at exit.
*
*   OUTPUTS
*
*       0 - Specified function successfully registered.
*       -1 - Specified function could not be registered (ran out of slots).
*
*************************************************************************/
int     atexit(void (*func)(void))
{
    int     status = 0;


    /* Check if a free slot is available to register the specified function. */
    if (proc_atexit_func_count < CFG_NU_OS_KERN_PROCESS_USER_ATEXIT_MAX_FUNCS)
    {
        /* Yes, register the function. */
        proc_atexit_funcs[proc_atexit_func_count] = func;
        proc_atexit_func_count++;
    }
    else
    {
        /* No, set the satus to a non-zero value to indicate error. */
        status = -1;
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       exit
*
*   DESCRIPTION
*
*       exit() implementation for Nucleus process runtime.
*
*   INPUTS
*
*       exit_code   - reason for exit
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void    exit(int exit_code)
{
    /* Stop the process with the passed-in exit code */
    (VOID)NU_Stop(NU_Getpid(), exit_code, NU_SUSPEND);

    /* Loop forever - noreturn */
    while(1);
}


/*************************************************************************
*
*   FUNCTION
*
*       abort
*
*   DESCRIPTION
*
*       abort() implementation for Nucleus process runtime.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void    abort(void)
{
    /* Exit with an abort exit code */
    exit(EXIT_ABORT);

    /* Loop forever - noreturn */
    while(1);
}

#if (CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       __cxa_pure_virtual
*
*   DESCRIPTION
*
*       Error handler for C++ call of pure virtual function.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void __cxa_pure_virtual(void)
{
    abort();
}

/*************************************************************************
*
*   FUNCTION
*
*       __cxa_atexit
*
*   DESCRIPTION
*
*       __cxa_atexit() implementation for Nucleus process runtime.
*
*   INPUTS
*
*       func - Pointer to the destructor function.
*       obj_ptr - Pointer to the object to be destroyed.
*       dso_handle - Dynamic Shared Object (DSO) handle.
*
*   OUTPUTS
*
*       0   - Specified function successfully registered.
*       -1  - Specified function could not be registered (ran out of slots).
*
*************************************************************************/
int     __cxa_atexit(void (*func)(void *), void *obj_ptr, void *dso_handle)
{
    int	    status = 0;


    /* We don't need to keep track of DSO because we are sitting inside the DSO. */
    NU_UNUSED_PARAM(dso_handle);

    /* Check if a free slot is available to register the specified function. */
    if (proc_cxa_atexit_func_count < (_cxx_ctor_end - _cxx_ctor_start))
    {
        /* Yes, register the destructor function. */
        _cxa_atexit_funcs[proc_cxa_atexit_func_count].destructor_func = func;
        _cxa_atexit_funcs[proc_cxa_atexit_func_count].obj_ptr = obj_ptr;
        proc_cxa_atexit_func_count++;
    }
    else
    {
        /* No, set the satus to a non-zero value to indicate error. */
        status = -1;
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       call_cxa_atexit_funcs
*
*   DESCRIPTION
*
*       Calls the destructor functions registered with __cxa_atexit().
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    call_cxa_atexit_funcs(VOID)
{
    INT    i = proc_cxa_atexit_func_count;


    /* Call the desctructors in reverse order. */
    while (i--)
    {
        /* Protect critical section */
        NU_Protect(NU_NULL);
        
        if (_cxa_atexit_funcs[i].destructor_func)
        {
            (*_cxa_atexit_funcs[i].destructor_func)(_cxa_atexit_funcs[i].obj_ptr);

            /* Clear the function pointer. */
            _cxa_atexit_funcs[i].destructor_func = NU_NULL;
        }

        /* Release protection */
        NU_Unprotect();
    }
}

#endif /* CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT */

/*************************************************************************
*
*   FUNCTION
*
*       call_atexit_funcs
*
*   DESCRIPTION
*
*       Calls the functions registered with atexit().
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID call_atexit_funcs(VOID)
{
    INT     i = proc_atexit_func_count;


    /* Call the atexit functions in reverse order. */
    while (i--)
    {
        /* Protect critical section */
        NU_Protect(NU_NULL);
        
        if (proc_atexit_funcs[i])
        {
            (*proc_atexit_funcs[i])();
            
            /* Clear the function pointer. */
            proc_atexit_funcs[i] = NU_NULL;
        }

        /* Release protection */
        NU_Unprotect();
    }
}

