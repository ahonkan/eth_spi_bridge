@************************************************************************
@*
@*             Copyright 2010 Mentor Graphics Corporation
@*                         All Rights Reserved.
@*
@* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
@* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
@* SUBJECT TO LICENSE TERMS.
@*
@************************************************************************

@************************************************************************
@*
@*  FILE NAME
@*
@*      csgnu_arm_debug.s
@*
@*  DESCRIPTION
@*
@*      This file contains the ARM CSGNU Debug specific functions
@*
@*  FUNCTIONS
@*
@*      ESAL_AR_DBG_Exception_Exit
@*
@*  DEPENDENCIES
@*
@*      csgnu_arm_defs.inc                              CSGNU include file
@*
@*************************************************************************

@****************************
@* INCLUDE NECESSARY FILES  *
@****************************

    .include     "csgnu_arm_defs.inc"

@**********************************
@* EXTERNAL FUNCTION DECLARATIONS *
@**********************************

@extern VOID    **(*ESAL_AR_DBG_OS_Exception_Handler)(VOID *stk_ptr);
@extern VOID    ESAL_AR_STK_Unsolicited_Switch(VOID **);

    .extern      ESAL_AR_DBG_OS_Exception_Handler
    .extern      ESAL_AR_STK_Unsolicited_Switch    
    
@**********************************
@* EXTERNAL VARIABLE DECLARATIONS *
@**********************************

@UINT32         ESAL_AR_DBG_Exception_Return;
@UINT32         ESAL_AR_DBG_Exception_SPSR;

    .extern      ESAL_AR_DBG_Exception_Return
    .extern      ESAL_AR_DBG_Exception_SPSR

@****************************
@* FUNCTIONS DECLARATIONS   *
@****************************

    .section        esal_code, "ax"

@**********************************
@* LOCAL VARIABLE DECLARATIONS    *
@**********************************

ESAL_AR_DBG_OS_Exception_Handler_ASM:
    .long ESAL_AR_DBG_OS_Exception_Handler
ESAL_AR_DBG_Exception_Return_ASM:
    .long ESAL_AR_DBG_Exception_Return
ESAL_AR_DBG_Exception_SPSR_ASM:
    .long ESAL_AR_DBG_Exception_SPSR


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_DBG_Exception_Exit
@*
@*  DESCRIPTION
@*
@*      Exit function used when a breakpoint is hit - saves the
@*      minimal context on the stack and calls the unsolicited context
@*      switch function
@*
@*  CALLED BY
@*
@*      Exception
@*
@*  CALLS
@*
@*      ESAL_AR_STK_Unsolicited_Switch
@*
@*  INPUTS
@*
@*      None
@*
@*  OUTPUTS
@*
@*      None
@*
@************************************************************************
@VOID    ESAL_AR_DBG_Exception_Exit(VOID)

    .thumb @ ESAL_AR_DBG_Exception_Exit
    .global ESAL_AR_DBG_Exception_Exit
ESAL_AR_DBG_Exception_Exit:

    
    .end
