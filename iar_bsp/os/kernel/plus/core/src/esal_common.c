/*************************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
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
*       esal_common.c
*
*   COMPONENT
*
*       ESAL - Embedded Software Abstraction Layer
*
*   DESCRIPTION
*
*       This file contains the generic functions.
*
*   FUNCTIONS
*
*       ESAL_GE_TMR_OS_Timer_Start          Starts the OS timer
*       ESAL_GE_TMR_OS_ISR_Register         Registers ISR for the
*                                           OS timer
*
*       ESAL_GE_STK_Initialize              Initialize stacks
*       ESAL_GE_STK_System_SP_Start_Get     Get start address of System SP
*       ESAL_GE_STK_System_SP_End_Get       Get end address of System SP
*       ESAL_GE_STK_Unsol_Switch_Default    Default unsolicited stack
*                                           switch entry function
*       ESAL_GE_STK_Except_Stack_Init       Initialize exception stack
*
*       ESAL_GE_RTE_Initialize              Initialize run-time environment
*                                           as required
*
*       ESAL_GE_MEM_Initialize              Initialize necessary
*                                           memory components
*
*       ESAL_GE_ISR_Initialize              Initialize interrupt servicing
*                                           components of ESAL
*       ESAL_GE_ISR_Default_Int_Handler     Default interrupt handler for
*                                           all interrupt sources
*                                           configured in ESAL
*       ESAL_GE_ISR_Default_Exc_Handler     Default exception handler for
*                                           all exception sources
*                                           configured in ESAL
*       ESAL_GE_ISR_OS_Default_Entry        Default OS entry point called
*                                           after interrupt servicing
*
*       ESAL_GE_INT_All_Disable             Disable all interrupt
*                                           sources configured
*       ESAL_GE_INT_Global_Set              Set global interrupt lockout
*                                           level
*       ESAL_GE_INT_Enable                  Enables a configured interrupt
*                                           through the appropriate component
*       ESAL_GE_INT_Disable                 Disables a configured interrupt
*                                           through the appropriate component
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       esal.h                              ESAL Internal functions
*
*************************************************************************/

/* Include files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/esal.h"
#include        "services/nu_trace_os_mark.h"

/**********************************************************************************/
/*                  Stack                                                         */
/**********************************************************************************/

/* Locally referenced functions */
static VOID         ESAL_GE_STK_Unsol_Switch_Default(VOID);

/* Global variables */
UINT8                ESAL_GE_STK_System_SP_Memory[(UINT32)ESAL_AR_SYSTEM_STACK_SIZE +
                                                  (UINT32)ESAL_GE_STK_MIN_FRAME_SIZE];
VOID                *ESAL_GE_STK_System_SP;

#if (ESAL_AR_EXCEPTION_STACK_SIZE != 0)

/* Locally referenced functions */
static VOID         ESAL_GE_STK_Except_Stack_Init(VOID);

/* Global variables */
UINT8                ESAL_GE_STK_Except_SP_Memory[(UINT32)ESAL_AR_EXCEPTION_STACK_SIZE +
                                                  (UINT32)ESAL_GE_STK_MIN_FRAME_SIZE];
VOID                *ESAL_GE_STK_Exception_SP;
#endif  /* ESAL_AR_EXCEPTION_STACK_SIZE != 0 */

VOID               	(*ESAL_GE_STK_Unsol_Switch_OS_Entry)(VOID) = ESAL_GE_STK_Unsol_Switch_Default;
INT                 ESAL_GE_STK_Unsol_Switch_Req;

/**********************************************************************************/
/*                  Run-Time Environment                                          */
/**********************************************************************************/

INT     			(*ESAL_GE_RTE_Byte_Write)(INT);
INT     			(*ESAL_GE_RTE_Byte_Read)(VOID);

/**********************************************************************************/
/*                  Interrupt Service                                             */
/**********************************************************************************/

/* Define locally referenced function prototypes */
static VOID     	ESAL_GE_ISR_Default_Int_Handler(INT int_vector_id);
static VOID     	ESAL_GE_ISR_Default_Exc_Handler(INT     except_vector_id, VOID *stack_ptr);
static VOID     	**ESAL_GE_ISR_OS_Default_Entry(INT  int_vector_id, VOID *stack_ptr);

/* Define global variables */
VOID            	**(*ESAL_GE_ISR_OS_Entry)(INT, VOID *) = ESAL_GE_ISR_OS_Default_Entry;
VOID            	(*ESAL_GE_ISR_OS_Nested_Entry)(INT) = ESAL_GE_ISR_Default_Int_Handler;
VOID            	(*ESAL_GE_ISR_Interrupt_Handler[ESAL_DP_INT_VECTOR_ID_DELIMITER])(INT);
VOID            	(*ESAL_GE_ISR_Exception_Handler[ESAL_AR_EXCEPT_VECTOR_ID_DELIMITER])(INT, VOID *);
INT             	ESAL_GE_ISR_Executing;
VOID            	*ESAL_GE_ISR_Interrupt_Vector_Data[ESAL_DP_INT_VECTOR_ID_DELIMITER];
VOID            	*ESAL_GE_ISR_Exception_Vector_Data[ESAL_AR_EXCEPT_VECTOR_ID_DELIMITER];
VOID            	(*ESAL_GE_ISR_Execute_Hook)(INT vector);

/**********************************************************************************/
/*                  Stack                                                         */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_STK_Initialize
*
*   DESCRIPTION
*
*       This function initializes the system stack pointer variable
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       unsol_stk_switch_entry              OS entry point after a
*                                           unsolicited stack switch
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_STK_Initialize(VOID (*unsol_stk_switch_entry)(VOID))
{
    /* Get pointer to end of allocated system stack memory */
    ESAL_GE_STK_System_SP = ESAL_GE_STK_System_SP_End_Get();

    /* Align system stack pointer as required */
    ESAL_GE_STK_System_SP = ESAL_GE_STK_ALIGN(ESAL_GE_STK_System_SP);

#if (ESAL_AR_EXCEPTION_STACK_SIZE != 0)

    /* Initialize the exception stack */
    ESAL_GE_STK_Except_Stack_Init();

#endif  /* ESAL_AR_EXCEPTION_STACK_SIZE != 0 */

    /* Check if a valid OS entry for unsolicited stack switch */
    if (unsol_stk_switch_entry)
    {
        /* Set unsolicited stack switch entry global variable */
        ESAL_GE_STK_Unsol_Switch_OS_Entry = unsol_stk_switch_entry;
    }

}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_STK_System_SP_Start_Get
*
*   DESCRIPTION
*
*       This function gets the start address of the system stack
*
*   CALLED BY
*
*       Operating System Services
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
*       start_addr                          Start address of system stack
*
***********************************************************************/
VOID    *ESAL_GE_STK_System_SP_Start_Get(VOID)
{
    VOID    *start_addr;


    /* Get start address of system stack */
    start_addr = (VOID *)&ESAL_GE_STK_System_SP_Memory[0];

    /* Return start address to caller */
    return (start_addr);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_STK_System_SP_End_Get
*
*   DESCRIPTION
*
*       This function gets the end address of the system stack
*
*   CALLED BY
*
*       Operating System Services
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
*       end_addr                            End address of system stack
*
***********************************************************************/
VOID    *ESAL_GE_STK_System_SP_End_Get(VOID)
{
    VOID    *end_addr;


    /* Calculate end address of system stack */
    end_addr = (VOID *)((VOID_CAST)&ESAL_GE_STK_System_SP_Memory[0] +
                        ESAL_AR_SYSTEM_STACK_SIZE);

    /* Return end address to caller */
    return (end_addr);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_STK_Unsol_Switch_Default
*
*   DESCRIPTION
*
*       This function is the default unsolicited stack switch entry
*
*   CALLED BY
*
*       ESAL_AR_STK_Unsolicited_Switch
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
*       None
*
***********************************************************************/
static VOID     ESAL_GE_STK_Unsol_Switch_Default(VOID)
{
    /* Loop forever */
    while (1){}
}


#if (ESAL_AR_EXCEPTION_STACK_SIZE != 0)
/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_STK_Except_Stack_Init
*
*   DESCRIPTION
*
*       This function initializes the exception handling stack
*
*   CALLED BY
*
*       ESAL_GE_STK_Initialize
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
*       None
*
***********************************************************************/
static VOID     ESAL_GE_STK_Except_Stack_Init(VOID)
{
    /* Calculate the exception stack pointer */
    ESAL_GE_STK_Exception_SP = (VOID *)((VOID_CAST)&ESAL_GE_STK_Except_SP_Memory[0] +
                                        ESAL_AR_EXCEPTION_STACK_SIZE);

    /* Align the exception stack pointer as required */
    ESAL_GE_STK_Exception_SP = ESAL_GE_STK_ALIGN(ESAL_GE_STK_Exception_SP);
}
#endif  /* ESAL_AR_EXCEPTION_STACK_SIZE != 0 */

/**********************************************************************************/
/*                  Interrupt                                                     */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_INT_All_Disable
*
*   DESCRIPTION
*
*       This function calls all the configured components to disable all
*       interrupt sources
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       [ESAL_DP_INT_All_Disable]           Disable all development
*                                           platform interrupt sources
*       [ESAL_PR_INT_All_Disable]           Disable all processor
*                                           interrupt sources
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_INT_All_Disable(VOID)
{

#if (ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Disable all development platform interrupt sources */
    ESAL_DP_INT_All_Disable();

#endif  /* ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE */

#if (ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Disable all processor interrupt sources */
    ESAL_PR_INT_All_Disable();

#endif  /* ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE */

}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_INT_Global_Set
*
*   DESCRIPTION
*
*       This function uses architecturally specific macros to enable /
*       disable interrupts to the specified level passed-in.
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_value                           New interrupt lockout value
*
*   OUTPUTS
*
*       INT                                 Old interrupt lockout value
*
***********************************************************************/
INT ESAL_GE_INT_Global_Set(INT new_value)
{
    INT    old_value = 0;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE_ISR();

    /* Get current (old) interrupt bits */
    ESAL_AR_INT_BITS_GET(&old_value);

    /* Set new interrupt bits */
    ESAL_AR_INT_BITS_SET(new_value);

    /* Return to user mode */
    NU_USER_MODE_ISR();

    /* Return old interrupt bits to caller */
    return (old_value);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_INT_Enable
*
*   DESCRIPTION
*
*       This function calls the appropriate component to enable the
*       interrupt source associated with the specified vector ID
*
*       NOTE:  This does not enable the interrupt at the source (ie
*              a device may need to enable it's interrupt within
*              a device control register).  This enables the
*              interrupt at the appropriate component level (ie
*              architectures interrupt control mechanism,
*              processor's interrupt controller or development
*              platform's interrupt controller)
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       [ESAL_AR_INT_Enable]
*       [ESAL_DP_INT_Enable]
*       [ESAL_PR_INT_Enable]
*
*   INPUTS
*
*       vector_id                           vector ID of interrupt
*                                           source being enabled
*       trigger_type                        trigger method for given
*                                           interrupt vector ID
*       priority                            priority for the given
*                                           interrupt vector ID
*
*   OUTPUTS
*
*       INT                                 vector_id if passed
*                                           ESAL_DP_INT_VECTOR_ID_DELIMITER
*                                           if failed
*
***********************************************************************/
INT     ESAL_GE_INT_Enable(INT                     vector_id,
                           ESAL_GE_INT_TRIG_TYPE   trigger_type,
                           INT                     priority)
{
    INT     result = ESAL_DP_INT_VECTOR_ID_DELIMITER;
    ESAL_AR_INT_CONTROL_VARS


#if (ESAL_AR_INT_VECTOR_ID_DELIMITER != 0)

    /* Check if vector ID is an architecture vector ID */
    if ( (vector_id >= 0) &&
         (vector_id < ESAL_AR_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call architecture function to enable the
           interrupt source associated with this vector ID */
        result = ESAL_AR_INT_Enable(vector_id, trigger_type, priority);
    }

#endif  /* ESAL_AR_INT_VECTOR_ID_DELIMITER != 0 */

    /* Lockout interrupts during critical section
       NOTE:  This is placed after the architecture
              interrupt disable call because it would
              overwrite any changes to the architecture
              registers when restoring. */
    ESAL_AR_INT_ALL_DISABLE();

#if (ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Check if vector ID is a processor vector ID */
    if ( (vector_id >= ESAL_AR_INT_VECTOR_ID_DELIMITER) &&
         (vector_id < ESAL_PR_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call processor function to enable the
           interrupt source associated with this vector ID */
        result = ESAL_PR_INT_Enable(vector_id, trigger_type, priority);
    }

#endif  /* ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE */

#if (ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Check if vector ID is a development platform vector ID */
    if ( (vector_id >= ESAL_PR_INT_VECTOR_ID_DELIMITER) &&
         (vector_id < ESAL_DP_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call development platform function to enable the
           interrupt source associated with this vector ID */
        result = ESAL_DP_INT_Enable(vector_id, trigger_type, priority);
    }

#endif  /* ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE */

    /* Trace log */
    T_IRQ_ENABLED(vector_id, result);

    /* Restore interrupts to the original level */
    ESAL_AR_INT_ALL_RESTORE();

    /* Return result of enabling interrupt */
    return (result);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_INT_Disable
*
*   DESCRIPTION
*
*       This function calls the appropriate component to disable the
*       interrupt source associated with the specified vector ID
*
*       NOTE:  This does not disable the interrupt at the source (ie
*              a device may need to disable it's interrupt within
*              a device control register).  This disables the
*              interrupt at the appropriate component level (ie
*              architectures interrupt control mechanism,
*              processor's interrupt controller or development
*              platform's interrupt controller)
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       [ESAL_AR_INT_Disable]
*       [ESAL_DP_INT_Disable]
*       [ESAL_PR_INT_Disable]
*
*   INPUTS
*
*       vector_id                           vector ID of interrupt
*                                           source being disabled
*
*   OUTPUTS
*
*       INT                                 vector_id if passed
*                                           ESAL_DP_INT_VECTOR_ID_DELIMITER
*                                           if failed
*
***********************************************************************/
INT     ESAL_GE_INT_Disable(INT     vector_id)
{
    INT     result = ESAL_DP_INT_VECTOR_ID_DELIMITER;
    ESAL_AR_INT_CONTROL_VARS


#if (ESAL_AR_INT_VECTOR_ID_DELIMITER != 0)

    /* Check if vector ID is an architecture vector ID */
    if ( (vector_id >= 0) &&
         (vector_id < ESAL_AR_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call architecture function to disable the
           interrupt source associated with this vector ID */
        result = ESAL_AR_INT_Disable(vector_id);
    }

#endif  /* ESAL_AR_INT_VECTOR_ID_DELIMITER != 0 */

    /* Lockout interrupts during critical section
       NOTE:  This is placed after the architecture
              interrupt disable call because it would
              overwrite any changes to the architecture
              registers when restoring. */
    ESAL_AR_INT_ALL_DISABLE();

#if (ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Check if vector ID is a processor vector ID */
    if ( (vector_id >= ESAL_AR_INT_VECTOR_ID_DELIMITER) &&
         (vector_id < ESAL_PR_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call processor function to enable the
           interrupt source associated with this vector ID */
        result = ESAL_PR_INT_Disable(vector_id);
    }

#endif  /* ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE */

#if (ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Check if vector ID is a development platform vector ID */
    if ( (vector_id >= ESAL_PR_INT_VECTOR_ID_DELIMITER) &&
         (vector_id < ESAL_DP_INT_VECTOR_ID_DELIMITER) )
    {
        /* Call development platform function to enable the
           interrupt source associated with this vector ID */
        result = ESAL_DP_INT_Disable(vector_id);
    }

#endif  /* ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE */

    /* Trace log */
    T_IRQ_DISABLED(vector_id, result);

    /* Restore interrupts to the original level */
    ESAL_AR_INT_ALL_RESTORE();

    /* Return result of disabling interrupt */
    return (result);
}

/**********************************************************************************/
/*                  Interrupt Service                                             */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_ISR_Initialize
*
*   DESCRIPTION
*
*       This function initializes the interrupt servicing portion
*       of ESAL
*
*   CALLED BY
*
*       Operating system services
*
*   CALLS
*
*       ESAL_AR_ISR_Vector_Table_Install    Install the architecture
*                                           vector table
*       [ESAL_AR_ISR_Initialize]            Initialize interrupt
*                                           handling at architecture
*       [ESAL_PR_ISR_Initialize]            Initialize interrupt
*                                           handling at processor
*       [ESAL_DP_ISR_Initialize]            Initialize interrupt
*                                           handling at dev platform
*
*   INPUTS
*
*       default_isr                         Function pointer to default
*                                           OS interrupt handler
*       default_except                      Function pointer to default
*                                           OS exception handler
*       os_isr_entry                        Function pointer to OS
*                                           interrupt handling entry
*       os_nested_isr_entry                 Function pointer to OS
*                                           nested interrupt handling
*                                           entry
*       hibernate_wake                      Determines what gets
*                                           initialized, items that are
*                                           stored in memory will be
*                                           skipped in wake up scenario
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_ISR_Initialize(VOID (*default_isr)(INT),
                               VOID (*default_except)(INT, VOID *),
                               VOID **(*os_isr_entry)(INT, VOID *),
                               VOID (*os_nested_isr_entry)(INT),
                               BOOLEAN hibernate_wake)
{
    INT     vector_id;


    /* Check if the OS non-nested handler is valid */
    if (os_isr_entry != NU_NULL)
    {
        /* Set the OS non-nested handler */
        ESAL_GE_ISR_OS_Entry = os_isr_entry;
    }

    /* Check if the OS nested handler is valid */
    if (os_nested_isr_entry != NU_NULL)
    {
        /* Set the OS nested handler */
        ESAL_GE_ISR_OS_Nested_Entry = os_nested_isr_entry;
    }

    /* Check if default isr parameter is valid */
    if (default_isr == NU_NULL)
    {
        /* Set default handler to ESAL default interrupt handler */
        default_isr = ESAL_GE_ISR_Default_Int_Handler;
    }

    /* Check if default except parameter is valid */
    if (default_except == NU_NULL)
    {
        /* Set default handler to ESAL default interrupt handler */
        default_except = ESAL_GE_ISR_Default_Exc_Handler;
    }

    if (hibernate_wake == NU_FALSE)
    {
        /* Loop through all interrupt vector IDs and set default interrupt handler for
           each vector */
        for (vector_id = 0; vector_id < ESAL_DP_INT_VECTOR_ID_DELIMITER; vector_id++)
        {
            /* Set default interrupt handler address for each interrupt source */
            ESAL_GE_ISR_HANDLER_SET(vector_id, default_isr);
        }

        /* Set default wake up handle */
        ESAL_GE_ISR_EXECUTE_HOOK_SET(NU_NULL);

        /* Loop through all exception vector IDs and set default exception handler for
           each vector */
        for (vector_id = 0; vector_id < ESAL_AR_EXCEPT_VECTOR_ID_DELIMITER; vector_id++)
        {
            /* Set default exception handler address for each exception source */
            ESAL_GE_EXCEPT_HANDLER_SET(vector_id, default_except);
        }
    }

    /* Install vector table for this architecture */
    ESAL_AR_ISR_Vector_Table_Install();

#if (ESAL_AR_ISR_INIT_REQUIRED == NU_TRUE)

    /* Execute architecture ISR initialization */
    ESAL_AR_ISR_Initialize();

#endif  /* ESAL_AR_ISR_INIT_REQUIRED == NU_TRUE */

#if (ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Execute processor ISR initialization */
    ESAL_PR_ISR_Initialize();

#endif  /* ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE */

#if (ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE)

    /* Execute development platform ISR initialization */
    ESAL_DP_ISR_Initialize();

#endif  /* ESAL_DP_INTERRUPTS_AVAILABLE == NU_TRUE */

}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_ISR_Default_Int_Handler
*
*   DESCRIPTION
*
*       This function is the default interrupt handler for
*       all interrupts supported by the given architecture, processor,
*       and development platform
*
*   CALLED BY
*
*       Interrupt
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       int_vector_id                       Interrupt vector ID
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID     ESAL_GE_ISR_Default_Int_Handler(INT int_vector_id)
{
    /* Reference unused parameters to prevent toolset warnings */
    NU_UNUSED_PARAM(int_vector_id);

    /* Loop forever */
    while (1){}
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_ISR_Default_Exc_Handler
*
*   DESCRIPTION
*
*       This function is the default exception handler for
*       all exceptions supported by the given architecture
*
*   CALLED BY
*
*       Interrupt
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       except_vector_id                    Vector ID of exception
*                                           source being serviced
*       stack_ptr                           Stack pointer of interrupted
*                                           code
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID     ESAL_GE_ISR_Default_Exc_Handler(INT     except_vector_id,
                                                VOID    *stack_ptr)
{
    /* Reference unused parameters to prevent toolset warnings */
    NU_UNUSED_PARAM(except_vector_id);
    NU_UNUSED_PARAM(stack_ptr);

    /* Loop forever */
    while (1){}
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_ISR_OS_Default_Entry
*
*   DESCRIPTION
*
*       This function is the default OS ISR entry point.  This is the
*       function that will execute, by default, for all non-nested
*       interrupts
*
*   CALLED BY
*
*       Interrupt
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       int_vector_id                       Vector ID of interrupt
*                                           source being serviced
*       stack_ptr                           Stack pointer of interrupted
*                                           code
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID     **ESAL_GE_ISR_OS_Default_Entry(INT  int_vector_id,
                                               VOID *stack_ptr)
{
    /* Reference unused parameters to prevent toolset warnings */
    NU_UNUSED_PARAM(int_vector_id);
    NU_UNUSED_PARAM(stack_ptr);

    /* Loop forever */
    while (1){}

    /* Return to remove compiler warnings */
    ESAL_TS_NO_RETURN (NU_NULL);
}

/**********************************************************************************/
/*                  Run-Time Environment                                          */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_RTE_Initialize
*
*   DESCRIPTION
*
*       This function initializes the run-time environment as required
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       ESAL_TS_MEM_BSS_Clear               Clear BSS
*       ESAL_TS_MEM_ROM_To_RAM_Copy         Copy Initialized Data
*                                           from ROM to RAM
*       ESAL_TS_RTE_Initialize              Initialize toolset
*                                           specific run-time components
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_RTE_Initialize(INT (*os_write)(INT), INT (*os_read)(VOID))
{
    /* Clear BSS (C zero-initialized data) */
    ESAL_TS_MEM_BSS_Clear();

    /* Copy C initialized data from ROM to RAM */
    ESAL_TS_MEM_ROM_To_RAM_Copy();

    /* Initialize the low-level I/O function pointers to the OS routines */
    ESAL_GE_RTE_Byte_Write = os_write;
    ESAL_GE_RTE_Byte_Read = os_read;
}

/**********************************************************************************/
/*                  Timer                                                         */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_TMR_OS_Timer_Start
*
*   DESCRIPTION
*
*       This function calls the necessary sub-components to start the
*       OS timer
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       ESAL_PR_TMR_OS_Timer_Start
*       [ESAL_AR_TMR_OS_Timer_Start]
*
*   INPUTS
*
*       ticks_per_sec                       Number of timer interrupts
*                                           that occur in a 1 sec period
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_TMR_OS_Timer_Start(UINT32 ticks_per_sec)
{
    /* Start / initialize OS timer at the processor level */
    ESAL_PR_TMR_OS_Timer_Start(ticks_per_sec);

#if (ESAL_AR_OS_TIMER_USED == NU_TRUE)

    /* Start OS timer at the architecture level */
    ESAL_AR_TMR_OS_Timer_Start(ticks_per_sec);

#endif  /* ESAL_AR_OS_TIMER_USED == NU_TRUE */

}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_TMR_OS_ISR_Register
*
*   DESCRIPTION
*
*       This function registers the OS timer interrupt service routine
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       isr_func_ptr                        Function pointer to ISR
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_GE_TMR_OS_ISR_Register(VOID (*isr_func_ptr)(INT))
{
    /* Attached OS Timer ISR to the OS timer vector ID */
    ESAL_GE_ISR_HANDLER_SET(ESAL_GE_TMR_OS_VECTOR, isr_func_ptr);

    /* Trace log */
    T_LISR_REGISTERED(ESAL_GE_TMR_OS_VECTOR, NU_SUCCESS);

    /* Trace log */
    T_IRQ_ENABLED(ESAL_GE_TMR_OS_VECTOR, NU_SUCCESS);
}

/**********************************************************************************/
/*                  MEMORY                                                        */
/**********************************************************************************/

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_MEM_Initialize
*
*   DESCRIPTION
*
*       This function initializes required target memory components
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       ESAL_TS_MEM_First_Avail_Get         Get first available memory
*                                           address usable by application
*       [ESAL_CO_MEM_Cache_Enable]          Enables core cache
*       [ESAL_PR_MEM_Cache_Enable]          Enables processor cache
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       VOID *                              Available memory
*
***********************************************************************/
VOID    *ESAL_GE_MEM_Initialize(VOID)
{
    VOID    *avail_mem;


    /* Get available memory address */
    avail_mem = ESAL_TS_MEM_First_Avail_Get();

#if ((ESAL_CO_CACHE_AVAILABLE == NU_TRUE) && (!defined(CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE)))

    /* Enable core level cache */
    avail_mem = ESAL_CO_MEM_Cache_Enable(avail_mem);

#endif  /* ESAL_CO_CACHE_AVAILABLE == NU_TRUE */

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) && (!defined(CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE)))

    /* Enable processor level cache */
    avail_mem = ESAL_PR_MEM_Cache_Enable(avail_mem);

#endif  /* ESAL_PR_CACHE_AVAILABLE == NU_TRUE */

    /* Align available memory address. */
    avail_mem = ESAL_GE_MEM_PTR_ALIGN(avail_mem, sizeof(UNSIGNED));

    /* Return updated available memory address */
    return (avail_mem);
}
