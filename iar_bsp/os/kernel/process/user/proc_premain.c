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
*       proc_premain.c
*
*   COMPONENT
*
*       Nucleus Processes - User
*
*   DESCRIPTION
*
*       Support for pre-"main" functionality.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       [proc_rte_invoke_initializers]
*       [__gnu_Unwind_Find_exidx]
*       PROC_Premain
*       main (weak)
*       NU_Protect
*       NU_Unprotect
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include <stdlib.h>

/* External function declarations */
NU_WEAK_REF(int main(int argc, char *argv[]));

/* Local variables */
static NU_SEMAPHORE     protect_semaphore;

/* Extern atexit calling function */
extern VOID call_atexit_funcs(VOID);

#if (CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT == NU_TRUE)

/* C++ Static Object support. */
extern void (*_cxx_ctor_start[])();         /* Symbol taken from linker command file. */
extern void (*_cxx_ctor_end[])();           /* Symbol taken from linker command file. */

/* C++ exceptions support. */
extern  UINT32 __exidx_start;
extern  UINT32 __exidx_end;

/* C++ support - Handle required by tool library functions. */
VOID *__dso_handle = &__dso_handle;

/* C++ exception support - This typedef is needed to match the signature
   of __gnu_Unwind_Find_exidx. */
typedef UINT32 *_Unwind_Ptr;

/* C++ atexit function */
extern VOID call_cxa_atexit_funcs(VOID);


/*************************************************************************
*
*   FUNCTION
*
*       proc_rte_invoke_initializers
*
*   DESCRIPTION
*
*       Invoke initializer functions in the process.
*
*   INPUTS
*
*       initializers_start - Starting address of the initializer list.
*
*       initializers_end - Ending address of the initializer list.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID proc_rte_invoke_initializers(VOID *     initializers_start,
                                         VOID *     initializers_end)
{
    INT         cursor;
    INT         quantity;
    UINT *      init_start;


    VOID (*ptr_to_function)();

    /* Get the start address of the initializers list */
    init_start = (UINT *)initializers_start;

    /* Calculate the number of initializers in the list */
    quantity = ((INT)((UINT) initializers_end - (UINT) initializers_start)) / sizeof(ptr_to_function);

    for (cursor = 0; cursor < quantity; cursor++)
    {
        /* Get the next initializer function */
        ptr_to_function = (VOID (*)()) (*(init_start + cursor));

        /*  Ensure initializer function is not NULL. */
        if (*ptr_to_function != NU_NULL)
        {
            /* Call the initializer function. */
            (*ptr_to_function)();
        }

    }
}


/*************************************************************************
*
*   FUNCTION
*
*       __gnu_Unwind_Find_exidx
*
*   DESCRIPTION
*
*       Minimal implementation of __gnu_Unwind_Find_exidx.
*
*   CALLED BY
*
*       (C++ library)
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       UINT32 *
*
*************************************************************************/
_Unwind_Ptr __gnu_Unwind_Find_exidx (_Unwind_Ptr pc, int *pcount)
{
	UINT32	*eitp;


	eitp = (_Unwind_Ptr)&__exidx_start;
	*pcount = &__exidx_end - &__exidx_start;

	return eitp;
}

#endif /* CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT */

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Premain
*
*   DESCRIPTION
*
*       Perform the pre-"main" functionality necessary for a Nucleus
*       Process
*
*   INPUTS
*
*       argc                Value used to determine logic path in pre-main
*       argv                TBD
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    PROC_Premain(UNSIGNED argc, VOID * argv)
{
    int         exit_code;


    /* Determine if the process is being started or stopped */
    if (argc == PROC_CMD_START)
    {
        /* Initialize the protect semaphore */
        (VOID)NU_Create_Semaphore(&protect_semaphore, "protect" , 1, NU_PRIORITY_INHERIT);

#if (CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT == NU_TRUE)

        /* Call C++ static object constructors in process */
        proc_rte_invoke_initializers(_cxx_ctor_start,
                                     _cxx_ctor_end);

#endif /* CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT */

        /* Call main */
        exit_code = main(0, NU_NULL);

        /* Check to see if main requested a continue on exit... */
        if (exit_code != EXIT_CONTINUE)
        {
            /* Call exit() */
            exit(exit_code);
        }
    }
    else if (argc == PROC_CMD_STOP)
    {
        /* Ensure abort flag isn't true (passed in argv) - skip atexit calls if so */
        if ((INT)argv != NU_TRUE)
        {
            /* Call atexit registered functions. */
            call_atexit_funcs();

#if (CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT == NU_TRUE)

            /* Call C++ global object destructors. */
            call_cxa_atexit_funcs();

#endif /* CFG_NU_OS_KERN_PROCESS_USER_CXX_SUPPORT */

        }

        /* Delete the protect semaphore */
        (VOID)NU_Delete_Semaphore(&protect_semaphore);
    }
    else
    {
        /* Error - trap */
        while(1);
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       main
*
*   DESCRIPTION
*
*       main stub function that will be used if a main is not defined in
*       the process
*
*   INPUTS
*
*       argc                Number of parameters
*       argv                Array of parameters
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
ESAL_TS_WEAK_DEF(int main(int argc, char *argv[]))
{
    /* Return continue exit code so process not killed */
    return (EXIT_CONTINUE);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Protect
*
*   DESCRIPTION
*
*       User space version of legacy NU_Protect that uses a PI mutex
*
*   INPUTS
*
*       protect_ptr                         Pointer to protection structure
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    NU_Protect(NU_PROTECT * protect_ptr)
{
    /* Ignore the parameter */
    NU_UNUSED_PARAM(protect_ptr);

    /* Obtain the PI mutex */
    (VOID)NU_Obtain_Semaphore(&protect_semaphore, NU_SUSPEND);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Unprotect
*
*   DESCRIPTION
*
*       User space version of legacy NU_Unprotect that uses PI mutex
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
VOID    NU_Unprotect(VOID)
{
    /* Release the PI mutex */
    NU_Release_Semaphore(&protect_semaphore);
}
