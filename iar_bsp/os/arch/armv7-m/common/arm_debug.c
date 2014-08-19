/***********************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*       arm_debug.c
*
*   DESCRIPTION
*
*       This file contains the ARM architecture debug functions
*
*   FUNCTIONS
*
*
*
*   DEPENDENCIES
*
*       nucleus.h
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"

/* Exception handler */
VOID **(*ESAL_AR_DBG_OS_Exception_Handler)(VOID *stack_ptr);
UINT32                                  ESAL_AR_DBG_Exception_Return;
UINT32                                  ESAL_AR_DBG_Exception_SPSR;

/* Generic debug operation flag */
extern INT                              ESAL_GE_DBG_Debug_Operation;

/* Generic exception handlers */
extern VOID **                          (*ESAL_GE_DBG_OS_Breakpoint_Handler)(VOID * stack_ptr);
extern VOID **                          (*ESAL_GE_DBG_OS_Hardware_Step_Handler)(VOID * stack_ptr);
extern VOID **                          (*ESAL_GE_DBG_OS_Data_Abort_Handler)(VOID * stack_ptr);

/* Local definitions */
typedef VOID (*ESAL_AR_DBG_EXCEPTION_HANDLER)(INT       exception_vector,
                                              VOID *    stack_ptr);

/* Original exception handlers */
ESAL_AR_DBG_EXCEPTION_HANDLER           ESAL_AR_DBG_Orig_Breakpoint_Handler;
ESAL_AR_DBG_EXCEPTION_HANDLER           ESAL_AR_DBG_Orig_Data_Abort_Handler;


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Initialize
*
*   DESCRIPTION
*
*       This function initializes debugging at architecture level
*
*   CALLED BY
*
*       OS Services
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
VOID    ESAL_AR_DBG_Initialize(VOID)
{
}

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Terminate
*
*   DESCRIPTION
*
*       This function terminates debugging at architecture level
*
*   CALLED BY
*
*       OS Services
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
VOID    ESAL_AR_DBG_Terminate(VOID)
{
    /* Restore all exception handlers. */
}
/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Reg_Read
*
*   DESCRIPTION
*
*       This function reads register stored in stack
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       p_stack_frame                       Start address of stack memory
*       stack_frame_type                    stack frame type
*       reg_no                              Register number
*       reg_val                             Register value for return
*
*   OUTPUTS
*
*       TRUE or FALSE
*
***********************************************************************/
INT    ESAL_AR_DBG_Reg_Read(VOID * p_stack_frame, INT stack_frame_type, INT reg_no, ESAL_GE_DBG_REG *reg_val)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(p_stack_frame);
    NU_UNUSED_PARAM(stack_frame_type);
    NU_UNUSED_PARAM(reg_no);
    NU_UNUSED_PARAM(reg_val);

    /* Return TRUE to caller */
    return (NU_TRUE);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Reg_Write
*
*   DESCRIPTION
*
*       This function write register stored in stack
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       p_stack_frame                       Start address of stack memory
*       stack_frame_type                    stack frame type
*       reg_no                              Register number
*       reg_val                             Register value to write
*
*   OUTPUTS
*
*       TRUE or FALSE
*
***********************************************************************/
INT    ESAL_AR_DBG_Reg_Write(VOID *p_stack_frame, INT stack_frame_type, INT reg_no, ESAL_GE_DBG_REG *reg_val)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(p_stack_frame);
	NU_UNUSED_PARAM(stack_frame_type);
	NU_UNUSED_PARAM(reg_no);
    NU_UNUSED_PARAM(reg_val);

    /* Return TRUE to caller */
    return (NU_TRUE);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Opcode_Read
*
*   DESCRIPTION
*
*       This function reads 4 byte (or 2 byte for thumb) instruction
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       read_addr                           Address to read
*
*   OUTPUTS
*
*       instruction
*
***********************************************************************/
ESAL_GE_DBG_OPCODE  ESAL_AR_DBG_Opcode_Read(VOID *read_addr)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(read_addr);

    /* Return opcode to caller */
    return (NU_NULL);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Opcode_Write
*
*   DESCRIPTION
*
*       This function writes 4 byte (or 2 byte for thumb) instruction
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       write_addr                          Address to write
*       opcode                              Value to write
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_AR_DBG_Opcode_Write(VOID *write_addr,
                                 ESAL_GE_DBG_OPCODE opcode)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(write_addr);
    NU_UNUSED_PARAM(opcode);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Opcode_Brk_Get
*
*   DESCRIPTION
*
*       This function returns breakpoint opcode depending on its mode
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       addr                                Address to check
*
*   OUTPUTS
*
*       Breakpoint opcode
*
***********************************************************************/
ESAL_GE_DBG_OPCODE  ESAL_AR_DBG_Opcode_Brk_Get(VOID *addr)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(addr);

    /* Return opcode to caller */
    return (NU_NULL);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Step_Addr_Get
*
*   DESCRIPTION
*
*       This function returns next program counter from given instruction
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       addr                                Address of current instruction
*       p_stack_frame                       Start address of stack memory
*       stack_frame_type                    stack frame type
*
*   OUTPUTS
*
*       Next program counter
*
***********************************************************************/
VOID    *ESAL_AR_DBG_Step_Addr_Get(VOID *addr, VOID *p_stack_frame, INT stack_frame_type)
{
    /* Reference unused parameters to avoid compiler warnings */
    NU_UNUSED_PARAM(addr);
    NU_UNUSED_PARAM(p_stack_frame);
    NU_UNUSED_PARAM(stack_frame_type);

    /* Return NULL to caller */
    return (NU_NULL);
}


/*************************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Hardware_Step
*
*   DESCRIPTION
*
*       This function performs a hardware-supported step operation.
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       p_stack_frame                       Start address of stack mem
*       stack_frame_type                    Type of stack frame
*
*   OUTPUTS
*
*       TRUE or FALSE
*
*************************************************************************/
INT ESAL_AR_DBG_Hardware_Step(VOID * p_stack_frame, INT stack_frame_type)
{
    return(NU_TRUE);
}

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_AR_DBG_Get_Support_Flags
*
*   DESCRIPTION
*
*       This function retreives the debug support flags.
*
*   CALLED BY
*
*       OS Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       flags                           The debug support flags value.
*
*   OUTPUTS
*
*       TRUE or FALSE
*
*************************************************************************/
INT ESAL_AR_DBG_Get_Support_Flags(UINT32 * flags)
{
    *flags = 0x0;
    return(NU_TRUE);
}
