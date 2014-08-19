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
*       proc_symbols.c
*
*   COMPONENT
*
*       Nucleus Processes - Linker / Loader
*
*   DESCRIPTION
*
*       Support for (exported) Nucleus symbol tables.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       proc_use_exp_sym
*       PROC_Validate_Symbols
*       PROC_Get_Exported_Symbol_Address
*       NU_Symbol
*       NU_Symbol_Close
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "os/kernel/process/core/proc_core.h"

#include <string.h>
#include <stdlib.h>

#include "kernel/proc_extern.h"
#include "proc_linkload.h"

#ifdef  CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE
#include "services/nu_services.h"
extern  BOOLEAN     PROC_Shell_Tryload;
extern  STATUS      PROC_Shell_Tryload_Status;
extern  NU_SHELL *  PROC_Shell_Tryload_Session;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       proc_use_exp_sym
*
*   DESCRIPTION
*
*       Retrieve the address of a symbol from the exported Nucleus symbol
*       table specified.
*
*   INPUTS
*
*       process_owner - Pointer to process control block that owns
*                       symbols.  May be NULL to indicate no symbol use
*                       update.
*
*       process_share - Pointer to process control block seeks use the
*                       symbol.  May be NULL to indicate no symbol use
*                       update.
*
*       sym_table - The exported symbol table.
*
*       sym_name - The name of the symbol to search for (NULL-terminated
*                  string).
*
*       sym_addr - Returned parameter that will contain the address of the
*                  symbol in the loaded module if the operation is
*                  successful.  May be NULL to indicate no return of
*                  symbol address.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation
*       NU_NOT_PRESENT          Indicates that the symbol could not be
*                               found
*
*************************************************************************/
static STATUS proc_use_exp_sym(PROC_CB *        process_owner,
                               PROC_CB *        process_user,
                               NU_SYMBOL_ENTRY *sym_table,
                               CHAR *           sym_name,
                               VOID **          sym_addr)
{
    STATUS              status = NU_NOT_PRESENT;
    UINT                i;

    /* Search for matching symbol in table and return address if found. */
    i = 0;
    while ((status == NU_NOT_PRESENT) &&
           (sym_table[i].symbol_address != NU_NULL))
    {
        if (strcmp(sym_table[i].symbol_name, sym_name) == 0)
        {
            /* Indicate symbol found. */
            status = NU_SUCCESS;

            /* Conditionally return the symbol address */
            if (sym_addr != NU_NULL)
            {
                *sym_addr = sym_table[i].symbol_address;
            }

            /* Conditionally update symbol use. */
            if ((process_owner != NU_NULL) &&
                (process_user != NU_NULL))
            {
                /* Update process which owns the symbols to indicate use by
                   the requesting process. */
                (VOID)PROC_Symbols_Use(process_owner, process_user);
            }
        }
        else
        {
            i++;
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PROC_Validate_Symbols
*
*   DESCRIPTION
*
*       Validate all symbols in the symbol table.
*
*   INPUTS
*
*       sym_table - Pointer to symbol table to be validated.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates symbols found in symbol table are valid
*                    (i.e. there are no duplicates of symbols already
*                    present in the system).
*
*       NU_INVALID_ENTRY - Indicates an invalid symbol table entry is
*                          contained in the symbol table (i.e. the symbol
*                          is a duplicate of a symbol already in the
*                          system).
*
*************************************************************************/
STATUS PROC_Validate_Symbols(NU_SYMBOL_ENTRY * sym_table)
{
    STATUS              status;
    UINT                i;
    PROC_CB *           current_process;

    i = 0;
    status = NU_NOT_PRESENT;
    while ((status == NU_NOT_PRESENT) &&
           (sym_table[i].symbol_address != NU_NULL))
    {
        /* Protect access to process list.  */
        TCCT_Schedule_Lock();

        /* Get the first process. */
        current_process = PROC_GET_FIRST();

        /* Search the process symbol tables for a match. */
        do
        {
            if (current_process -> state == PROC_STARTED_STATE)
            {
                /* Search any user-mode symbols in the process. */
                if (current_process -> symbols)
                {
                    status = proc_use_exp_sym(NU_NULL,
                                              NU_NULL,
                                              current_process -> symbols,
                                              (CHAR *)sym_table[i].symbol_name,
                                              NU_NULL);
                }

                if (status == NU_NOT_PRESENT)
                {
                    /* Search any kernel-mode symbols in the process. */
                    if ((current_process -> kernel_mode == NU_TRUE) &&
                        (current_process -> ksymbols))
                    {
                        status = proc_use_exp_sym(NU_NULL,
                                                  NU_NULL,
                                                  current_process -> ksymbols,
                                                  (CHAR *)sym_table[i].symbol_name,
                                                  NU_NULL);
                    }
                }
            }

            /* Move to the next process. */
            current_process = PROC_GET_NEXT(current_process);

        } while ((status == NU_NOT_PRESENT) &&
                 (current_process != PROC_GET_FIRST()));

        /* Release protection on process list. */
        TCCT_Schedule_Unlock();

#ifdef CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE

        if ((status != NU_NOT_PRESENT) &&
            (PROC_Shell_Tryload == NU_TRUE))
        {
            /* Check to see if this is the first error */
            if (PROC_Shell_Tryload_Status == NU_SUCCESS)
            {
                /* Print string stating duplicate symbols found */
                NU_Shell_Puts(PROC_Shell_Tryload_Session,"Duplicate symbol(s) found:\n\r");
            }

            /* Print duplicate symbol name to the appropriate shell session */
            NU_Shell_Puts(PROC_Shell_Tryload_Session, "    ");
            NU_Shell_Puts(PROC_Shell_Tryload_Session, (CHAR *)sym_table[i].symbol_name);
            NU_Shell_Puts(PROC_Shell_Tryload_Session, "\n\r");

            /* Set another status to show that duplicate symbol found. */
            PROC_Shell_Tryload_Status = NU_INVALID_ENTRY;

            /* Set status to not present so that other duplicate symbols can be found */
            status = NU_NOT_PRESENT;
        }

#endif /* CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE */

        /* Move to the next symbol. */
        i++;
    }

    if (status == NU_NOT_PRESENT)
    {
        /* No duplicate symbol found.  Symbol table valid. */
        status = NU_SUCCESS;
    }
    else if (status == NU_SUCCESS)
    {
        /* Duplicate symbol found.  Symbol table is invalid. */
        status = NU_INVALID_ENTRY;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Get_Exported_Symbol_Address
*
*   DESCRIPTION
*
*       Retrieve the address of a symbol from the set of exported symbols
*       from all processes.
*
*   INPUTS
*
*       process - The process requesting the symbol.
*
*       sym_name - The name of the symbol to search for (NULL-terminated
*                  string).
*
*       sym_addr - Returned parameter that will contain the address of the
*                  symbol in the loaded module if the operation is
*                  successful and NULL otherwise.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation
*       NU_NOT_PRESENT          Indicates that the symbol could not be
*                               found
*       NU_INVALID_OPERATION    Indicates an unexpected error has
*                               occurred
*
*************************************************************************/
STATUS PROC_Get_Exported_Symbol_Address(PROC_CB *process, CHAR *sym_name, VOID **sym_addr)
{
    STATUS              status = NU_UNAVAILABLE;
    NU_SYMBOL_ENTRY *   symbols;
    PROC_CB *           current_process;

    /* Initialize the returned parameter value to indicate failure. */
    *sym_addr = NU_NULL;

    /* Protect access to process list.  */
    TCCT_Schedule_Lock();

    /* Get the first process. */
    current_process = PROC_GET_FIRST();

    /* Search the processes symbol tables for a match. */
    do
    {
        if (current_process->state == PROC_STARTED_STATE)
        {
            /* Get the exported symbol table for the current process. */

#if ((CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE) || defined(CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE))

            /* Use kernel mode symbol table if both processes are kernel mode. */

            if (process->kernel_mode && current_process->kernel_mode)
            {
                symbols = current_process->ksymbols;
            }
            else

#endif

            /* If either or both processes are user mode or mode switching is not enabled
               in the root kernel image, then use the ordinary symbol table. */

            {
                symbols = current_process->symbols;
            }

            /* Ensure the process has a symbol table. */
            if (symbols)
            {
                /* Attempt to find the symbol in the exported symbols for the
                   current module. */
                status = proc_use_exp_sym(current_process, process, symbols,
                                          sym_name, sym_addr);
            }
        }

        /* Move to the next process. */
        current_process = PROC_GET_NEXT(current_process);

    } while ((*sym_addr == NU_NULL) &&
             (current_process != PROC_GET_FIRST()));

    /* Release protection on process list. */
    TCCT_Schedule_Unlock();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Symbol
*
*   DESCRIPTION
*
*       Retrieve the address of a symbol from a specified process
*
*   INPUTS
*
*       pid - ID of the process from which to retrieve the symbol address.
*
*       sym_name - The name of the symbol to search for (NULL-terminated
*                  string).
*
*       sym_addr - Returned parameter that will contain the address of the
*                  symbol in the loaded process if the operation is
*                  successful and NULL otherwise.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates successful operation.
*       NU_NOT_PRESENT      Indicates that the symbol could not be found.
*       NU_INVALID_PROCESS  Invalid process ID.
*       NU_INVALID_POINTER  Invalid pointer for sym_name or sym_addr
*
*************************************************************************/
STATUS NU_Symbol (INT pid, CHAR * sym_name, VOID ** sym_addr)
{
    PROC_CB *           process;
    PROC_CB *           process_user;
    STATUS              status = NU_SUCCESS;
    NU_SYMBOL_ENTRY *   symbols;


    /* Error checking */
    NU_ERROR_CHECK((sym_name == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((sym_addr == NU_NULL), status, NU_INVALID_POINTER);

    /* Ensure successful */
    if (status == NU_SUCCESS)
    {
        /* Get the process control block for given pid */
        process = PROC_Get_Pointer(pid);
        process_user = PROC_Get_Pointer(NU_Getpid());

        /* Check if valid process */
        if (process != NU_NULL)
        {
            /* Get process mutex to ensure process state not changed  */
            status = NU_Obtain_Semaphore(&process->semaphore, NU_SUSPEND);

            /* Ensure mutex obtained */
            if (status == NU_SUCCESS)
            {
                /* Check for valid state - process must be in started state */
                if (process->state == PROC_STARTED_STATE)
                {
                    /* Get the exported symbol table for the current process. */
                    symbols = process->symbols;

                    /* Ensure the process has a symbol table. */
                    if (symbols)
                    {
                        /* Lock critical section */
                        TCCT_Schedule_Lock();

                        /* Attempt to find the symbol in the exported symbols for the
                           specified process. */
                        status = proc_use_exp_sym(process, process_user, symbols,
                                                  sym_name, sym_addr);

                        /* Unlock critical section */
                        TCCT_Schedule_Unlock();
                    }
                    else
                    {
                        /* Set status to show symbol not found (no symbols in specified process) */
                        status = NU_NOT_PRESENT;
                    }
                }
                else
                {
                    /* Return invalid state error */
                    status = NU_INVALID_STATE;
                }

                /* Release the mutex */
                (VOID)NU_Release_Semaphore(&process->semaphore);
            }
        }
        else
        {
            status = NU_INVALID_PROCESS;
        }
    }

    /* Return status to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_Symbol_Close
*
*   DESCRIPTION
*
*       Releases all symbols used by the current process and provided by
*       the specified process.  If the specified process is no longer
*       providing any symbols (to any process) it will be stopped.
*
*   INPUTS
*
*       pid - ID of the process that is providing the symbols (symbol
*             owner).
*
*       stop - NU_TRUE will halt the process if all symbols are free,
*              NU_FALSE will allow the process to continue operation
*
*       stopped - Serves as a return parameter that will be updated to
*                 indicate whether the process was stopped as a result of
*                 the call or not.  If a NULL is provided for this
*                 parameter then the resulting state of the process
*                 will not be returned.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation
*       NU_INVALID_PROCESS      Indicates an invalid process specified
*       <other>                 Indicates (other) internal error occurred
*
*************************************************************************/
STATUS NU_Symbol_Close(INT pid, BOOLEAN stop, BOOLEAN *stopped)
{
    STATUS   status = NU_SUCCESS;
    PROC_CB *sym_owner;
    PROC_CB *sym_user;
    BOOLEAN  stop_proc = NU_FALSE;

    /* Get current process (symbol user). */
    sym_user = PROC_Get_Pointer(NU_Getpid());

    /* Get process providing symbols (symbol owner). */
    sym_owner = PROC_Get_Pointer(pid);
    NU_ERROR_CHECK((sym_owner == NU_NULL), status, NU_INVALID_PROCESS);

    if (status == NU_SUCCESS)
    {
        /* Un-use symbols provided by the specified process that are being
           used by this process. */
        status = PROC_Symbols_Unuse(sym_owner, sym_user);
    }

    if (status == NU_SUCCESS)
    {
        /* Determine if symbol owner is still providing symbols for any
           other processes. */
        if ((stop == NU_TRUE) && (PROC_Symbols_In_Use(sym_owner) == NU_FALSE))
        {
            /* Stop the symbol owner. */
            status = NU_Stop(pid, EXIT_STOP, NU_SUSPEND);

            /* Ensure symbol owner stopped (or was already stopped). */
            if ((status == NU_SUCCESS) ||
                (status == NU_INVALID_STATE))
            {
                /* Indicate stop operation successful. */
                status = NU_SUCCESS;

                /* Process has successfully been stopped */
                stop_proc = NU_TRUE;
            }
        }

        /* Conditionally update symbol owner stopped value. */
        if (stopped != NU_NULL)
        {
            /* Update return value to indicate if the process
               was stopped. */
            *stopped = stop_proc;
        }
    }

    /* Return status to caller */
    return (status);
}
