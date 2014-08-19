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
*       esal_dbg.c
*
*   COMPONENT
*
*       ESAL - Embedded Software Abstraction Layer
*
*   DESCRIPTION
*
*       This file contains the generic functions related to debugging.
*
*   FUNCTIONS
*
*       ESAL_GE_DBG_Initialize
*       ESAL_GE_DBG_Default_Exception_Handler
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

/**********************************************************************************/
/*                  Debug                                                        */
/**********************************************************************************/

/* Generic exception handlers */
VOID ** 			(*ESAL_GE_DBG_OS_Breakpoint_Handler)(VOID * stack_ptr);
VOID ** 			(*ESAL_GE_DBG_OS_Hardware_Step_Handler)(VOID * stack_ptr);
VOID ** 			(*ESAL_GE_DBG_OS_Data_Abort_Handler)(VOID * stack_ptr);

/* Debug Operation */

/* Debug Operation - Global variable that is used to indicate if a debug
   operation is occuring.  It should be set to 1 to indicate debug
   processing and 0 to indicate normal operation. */
INT 				ESAL_GE_DBG_Debug_Operation;

/* Local functions */

static VOID ** 		ESAL_GE_DBG_Default_Exception_Handler(VOID * stack_ptr);

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_GE_DBG_Default_Exception_Handler
*
*   DESCRIPTION
*
*       Default OS exception handler
*
*   CALLED BY
*
*       debug exception
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       stack_ptr - Stack pointer of executing thread.
*
*   OUTPUTS
*
*       Pointer to stack pointer save location for executing thread.
*
*************************************************************************/
static VOID ** ESAL_GE_DBG_Default_Exception_Handler(VOID * stack_ptr)
{
    /* Reference unused parameter to avoid toolset warnings */
    NU_UNUSED_PARAM(stack_ptr);

    /* Spin forever */
    while(1);

    /* Return to remove warnings */
    ESAL_TS_NO_RETURN (NU_NULL);
}

/* Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_GE_DBG_Initialize
*
*   DESCRIPTION
*
*       Initialize the debugging components.
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       ESAL_AR_DBG_Initialize
*
*   INPUTS
*
*       breakpoint_handler - Pointer to the (software) breakpoint handler.
*
*       hardware_step_handler - Pointer to the hardware step handler.
*
*       data_abort_handler - Pointer to the data abort handler.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ESAL_GE_DBG_Initialize(VOID **(*breakpoint_handler)(VOID *),
                            VOID **(*hardware_step_handler)(VOID *),
                            VOID **(*data_abort_handler)(VOID *))
{
    /* Initialize the architecture level debug handling component. */
    ESAL_AR_DBG_Initialize();

    /* Set initial debug operation flag value. */
    ESAL_GE_DBG_Debug_Operation = 0;

    /* Setup breakpoint exception handler. */
    if (breakpoint_handler != NU_NULL)
    {
        ESAL_GE_DBG_OS_Breakpoint_Handler = breakpoint_handler;
    }
    else
    {
        ESAL_GE_DBG_OS_Breakpoint_Handler = ESAL_GE_DBG_Default_Exception_Handler;
        
    } /* if */
    
    /* Setup hardware step exception handler. */
    if (hardware_step_handler != NU_NULL)
    {
        ESAL_GE_DBG_OS_Hardware_Step_Handler = hardware_step_handler;
    }
    else
    {
        ESAL_GE_DBG_OS_Hardware_Step_Handler = ESAL_GE_DBG_Default_Exception_Handler;
        
    } /* if */     
    
    /* Setup data abort exception handler. */
    if (data_abort_handler != NU_NULL)
    {
        ESAL_GE_DBG_OS_Data_Abort_Handler = data_abort_handler;
    }
    else
    {
        ESAL_GE_DBG_OS_Data_Abort_Handler = ESAL_GE_DBG_Default_Exception_Handler;
        
    } /* if */    
    
    return;
}

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_GE_DBG_Terminate
*
*   DESCRIPTION
*
*       Terminate the debugging component support.
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       ESAL_AR_DBG_Terminate
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
VOID ESAL_GE_DBG_Terminate(VOID)
{
    /* Terminate the architecture level debug handling component. */
    ESAL_AR_DBG_Terminate();
    
    return;
}
