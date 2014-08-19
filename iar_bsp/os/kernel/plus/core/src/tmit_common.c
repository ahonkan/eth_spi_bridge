/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       tmit_common.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the common target dependent initialization
*       routines for this component.
*
*   FUNCTIONS
*
*       TMIT_Initialize                     Timer Management Initialize
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/supplement/inc/error_management.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* Define external inner-component global data references.  */

extern TC_HCB   TMD_HISR;
extern UINT8    TMD_HISR_Stack[NU_TIMER_HISR_STACK_SIZE];

/* Define inner-component function prototypes.  */

VOID            TMC_Timer_HISR(VOID);
VOID            TMCT_Timer_Interrupt(INT);

/***********************************************************************
*
*   FUNCTION
*
*       TMIT_Initialize
*
*   DESCRIPTION
*
*       This function initializes the data structures that control the
*       operation of the timer component (TM).  There are no application
*       timers created initially.  This routine must be called from
*       Supervisor mode in Supervisor/User mode switching kernels.
*
*   CALLED BY
*
*       INC_Initialize                      System initialization
*
*   CALLS
*
*       ESAL_GE_TMR_OS_ISR_Register         Registers OS Timer
*                                           interrupt service routine
*       ERC_System_Error                    System error handing
*                                           function
*       NU_Create_HISR                      Create timer HISR
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
VOID  TMIT_Initialize(VOID)
{
    STATUS          status;

    /* Call ESAL to register OS timer interrupt service routine */
    ESAL_GE_TMR_OS_ISR_Register(TMCT_Timer_Interrupt);

    /* Create the timer HISR.  The timer HISR is responsible for performing
       the time slice function and activating the timer task if necessary.  */
    status = NU_Create_HISR((NU_HISR *) &TMD_HISR, "SYSTEM H",
                            TMC_Timer_HISR, (OPTION) NU_TIMER_HISR_PRIORITY,
                            (VOID *)&TMD_HISR_Stack[0], NU_TIMER_HISR_STACK_SIZE);

    /* Check for a system error creating the timer HISR.  */
    if (status != NU_SUCCESS)
    {
        /* Call the system error handler.  */
        ERC_System_Error(NU_ERROR_CREATING_TIMER_HISR);
    }

}
