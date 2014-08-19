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
@*      csgnu_arm.s
@*
@*  DESCRIPTION
@*
@*      This file contains the base ARM CSGNU specific functions
@*
@*  FUNCTIONS
@*
@*      ESAL_AR_ISR_Vector_Table
@*      ESAL_Entry
@*      ESAL_AR_STK_Unsolicited_Restore
@*      ESAL_AR_STK_Unsolicited_Switch
@*      ESAL_AR_STK_Startup_SP_Set
@*      ESAL_TS_RTE_Lowlevel_Initialize
@*      ESAL_TS_RTE_SP_Write
@*      ESAL_TS_STK_Solicited_Restore
@*      ESAL_TS_STK_Solicited_Switch
@*      ESAL_AR_ISR_Interrupt_Handler
@*      ESAL_AR_ISR_Exception_Handler
@*      ESAL_AR_ISR_SVC_Handler
@*      ESAL_AR_ISR_Return
@*
@*  DEPENDENCIES
@*
@*      csgnu_arm_defs.inc                  CSGNU include file
@*
@*************************************************************************

@****************************
@* Enable full instruction set
@* for thumb2 code generation.
@****************************
     .syntax unified

@****************************
@* INCLUDE NECESSARY FILES  *
@****************************

#include    "nucleus_gen_cfg.h"
#include    "csgnu_arm_defs.inc"

@**********************************
@* EXTERNAL FUNCTION DECLARATIONS *
@**********************************

@VOID           OS_Init_Entry(VOID)@
@extern VOID    ESAL_AR_ISR_HOOK(VOID)@

    .extern OS_Init_Entry

    .if ESAL_AR_ISR_HOOK_ENABLED

    .extern ESAL_AR_ISR_HOOK

    .endif @ ESAL_AR_ISR_HOOK_ENABLED

@**********************************
@* EXTERNAL VARIABLE DECLARATIONS *
@**********************************

@extern VOID    *ESAL_GE_STK_System_SP@
@extern INT     ESAL_GE_STK_Unsol_Switch_Req@
@extern VOID    (*ESAL_GE_STK_Unsol_Switch_OS_Entry)(VOID)@
@extern INT     const ESAL_GE_MEM_ROM_Support_Enabled@
@extern VOID    **(*ESAL_GE_ISR_OS_Entry)(INT vector, VOID *stack_ptr)@
@extern VOID    (*ESAL_GE_ISR_OS_Nested_Entry)(INT vector)@
@extern VOID    (*ESAL_GE_ISR_Exception_Handler[ESAL_AR_NUM_EXCEPTIONS])(INT except_num, VOID *frame_ptr)@
@extern INT     ESAL_GE_ISR_Executing@

    .extern ESAL_GE_STK_System_SP
    .extern ESAL_GE_STK_Unsol_Switch_Req
    .extern ESAL_GE_STK_Unsol_Switch_OS_Entry
    .extern ESAL_GE_MEM_ROM_Support_Enabled
    .extern ESAL_GE_ISR_OS_Entry
    .extern ESAL_GE_ISR_OS_Nested_Entry
    .extern ESAL_GE_ISR_Exception_Handler
    .extern ESAL_GE_ISR_Executing
#if (CFG_NU_OS_SVCS_TRACE_CORE_PC_HOTSPOT_SUPPORT == 1)
    .extern Trace_PC_Sample
#endif
    
@ Import linker produced labels

    .extern     _ld_bss_end

@****************************
@* FUNCTIONS DECLARATIONS   *
@****************************

    .section        esal_code, "ax"

@**********************************
@* LOCAL VARIABLE DECLARATIONS    *
@**********************************

ESAL_AR_STK_Unsol_Switch_OS_Entry:
    .long ESAL_GE_STK_Unsol_Switch_OS_Entry
ESAL_AR_STK_BSS_End:
    .long _ld_bss_end
ESAL_AR_STK_System_SP:
    .long ESAL_GE_STK_System_SP
ESAL_AR_STK_Unsol_Switch_Req:
    .long ESAL_GE_STK_Unsol_Switch_Req
ESAL_Entry_ROM_Support_Enabled:
    .long ESAL_GE_MEM_ROM_Support_Enabled
ESAL_AR_ISR_Executing:
    .long ESAL_GE_ISR_Executing
ESAL_AR_ISR_Executing_Counter:
    .long ESAL_GE_ISR_Executing
ESAL_AR_ISR_OS_Entry:
    .long ESAL_GE_ISR_OS_Entry
ESAL_AR_ISR_OS_Nested_Entry:
    .long ESAL_GE_ISR_OS_Nested_Entry
ESAL_TS_STK_System_SP:
    .long ESAL_GE_STK_System_SP

#if (CFG_NU_OS_SVCS_TRACE_CORE_PC_HOTSPOT_SUPPORT == 1)
ESAL_AR_ISR_PC_Sample:
    .long Trace_PC_Sample
#endif

@************************************************************************
@*
@*   FUNCTION
@*
@*       Board_Lowlevel_Init
@*
@*   DESCRIPTION
@*
@*       Stub Board Init function that will be called if the BSP doesn't
@*       contain ROM support
@*
@*   CALLED BY
@*
@*       Reset
@*
@*   CALLS
@*
@*       None
@*
@*   INPUTS
@*
@*       None
@*
@*   OUTPUTS
@*
@*       None
@*
@***********************************************************************/
@VOID    Board_Lowlevel_Init(VOID)

    .thumb @ Board_Lowlevel_Init
    .weak   Board_Lowlevel_Init
    .type Board_Lowlevel_Init, %function
Board_Lowlevel_Init:

    @ Return to caller

    BX      lr

@************************************************************************
@*
@*   FUNCTION
@*
@*       ESAL_Entry
@*
@*   DESCRIPTION
@*
@*       This function is the entry point into ESAL.  Entry to this
@*       function is normally done through the reset mechanism
@*       (reset vector, jump from reset handler, etc) on the given
@*       processor / architecture.
@*
@*   CALLED BY
@*
@*       Reset
@*
@*   CALLS
@*
@*       ESAL_AR_STK_Startup_SP_Set
@*
@*   INPUTS
@*
@*       None
@*
@*   OUTPUTS
@*
@*       None
@*
@***********************************************************************/
@VOID    ESAL_Entry(VOID)

    @ Define the entry point for the debugger

    .global  start
start:
    .thumb @ ESAL_Entry
    .global ESAL_Entry
    .type ESAL_Entry, %function
ESAL_Entry:

    @ Globally disable all interrupts

    CPSID   i

    @ Check if running from ROM (ROM support enabled)

    LDR     r0,ESAL_Entry_ROM_Support_Enabled
    LDR     r0,[r0]
    CMP     r0,#0
    BEQ     ESAL_Entry_ROM_Support_End

    @ Initialize memory controllers, chip-selects, etc, to allow
    @ access to volatile memory (RAM) when running from ROM.  Access
    @ to this memory must be done before entering a C environment
    @ so the stack can be set-up / utilized.
    @ NOTE:    When executing from RAM (debug environment) these operations
    @          are normally performed by the debugger (via an initialization
    @          script) or by a monitor or boot code on the target hardware.

    BL      Board_Lowlevel_Init

ESAL_Entry_ROM_Support_End:

    @ Transfer control to ESAL_AR_STK_Startup_SP_Set function
    @ NOTE:   Control will not return here

    B       ESAL_AR_STK_Startup_SP_Set

    .balign   4    @ Align to 4 byte

@************************************************************************
@*
@*   FUNCTION
@*
@*       ESAL_AR_STK_Unsolicited_Restore
@*
@*   DESCRIPTION
@*
@*       This function restores the context of a stack frame as required
@*       by a given architecture (stack frame that contains all registers
@*       used by a given architecture)
@*
@*   CALLED BY
@*
@*       Operating System Services
@*
@*   CALLS
@*
@*       None
@*
@*   INPUTS
@*
@*       r0                                  Stack pointer of
@*                                           stack frame to be restored
@*
@*   OUTPUTS
@*
@*       None
@*
@***********************************************************************
@VOID    ESAL_AR_STK_Unsolicited_Restore(VOID *stack_ptr)

    .thumb @ ESAL_AR_STK_Unsolicited_Restore
    .global ESAL_AR_STK_Unsolicited_Restore
    .type ESAL_AR_STK_Unsolicited_Restore, %function
ESAL_AR_STK_Unsolicited_Restore:

    @ Switch to new stack

    MOV     sp,r0

    @ Restore r3-r11 from stack (stack type in r3)

    LDMIA   sp!,{r3-r11}

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    @ Restore required floating point registers from the current stack.

    LDMIA   sp!,{r2}

    VMSR    FPSCR,r2

    VLDMIA  sp!,{d0-d15}

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    @ Disable priority 1 and above interrupts

    MOV     r0,#(1<<(8-CFG_NU_OS_ARCH_ARMV7_M_COM_NUM_PRI_BITS))
    MSR     basepri,r0

    @ Re-enable interrupts to allow switch to handler mode (using a SVC call)

    CPSIE   i

    @ Perform a service call to switch to handler mode and finish the
    @ unsolicited stack restore.
    @ NOTE:  Control will not return here.

    SVC     0


@***********************************************************************
@*
@*   FUNCTION
@*
@*       ESAL_AR_STK_Unsolicited_Switch
@*
@*   DESCRIPTION
@*
@*       This function saves the entire architecture context on the
@*       given stack and passes control to the OS
@*
@*   CALLED BY
@*
@*       Operating System Services
@*
@*   CALLS
@*
@*       None
@*
@*   INPUTS
@*
@*       r0                                  Pointer to stack pointer
@*
@*   OUTPUTS
@*
@*       None
@*
@***********************************************************************
@VOID  ESAL_AR_STK_Unsolicited_Switch(VOID **stack_ptr)

    .thumb @ ESAL_AR_STK_Unsolicited_Switch
    .global ESAL_AR_STK_Unsolicited_Switch
    .type ESAL_AR_STK_Unsolicited_Switch, %function
ESAL_AR_STK_Unsolicited_Switch:

    @ Get stack pointer

    LDR     sp,[r0]

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    @ Save required floating point registers on the current stack.

    VSTMDB  sp!,{d0-d15}

    VMRS    r3,FPSCR

    STMDB   sp!,{r3}

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    @ Put stack type in r3

    MOV     r3,#1

    @ Save remaining registers and stack type on stack

    STMDB   sp!,{r3-r11}

    @ Save new stack pointer in save location

    STR     sp,[r0]

    @ Clear switch flag

    LDR     r0,ESAL_AR_STK_Unsol_Switch_Req
    MOV     r1,#0
    STR     r1,[r0]

    @ Switch to the system stack

    LDR     r0,ESAL_AR_STK_System_SP
    LDR     sp,[r0]

    @ Transfer control to OS unsolicited stack switch entry point

    LDR     r0,ESAL_AR_STK_Unsol_Switch_OS_Entry
    LDR     r0,[r0]

    @ Call return from ISR function to return to OS in thread mode

    B       ESAL_AR_ISR_Return

@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_STK_Startup_SP_Set
@*
@*  DESCRIPTION
@*
@*      This function sets the architecture stack pointer to an address
@*      that can be used during initialization.  This can include on-chip
@*      SRAM, available RAM not used by the application during
@*      initialization, etc.
@*
@*  CALLED BY
@*
@*      ESAL_Entry
@*
@*  CALLS
@*
@*      ESAL_TS_RTE_Lowlevel_Initialize
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
@VOID    ESAL_AR_STK_Startup_SP_Set(VOID)

    .thumb @ ESAL_AR_STK_Startup_SP_Set
    .global ESAL_AR_STK_Startup_SP_Set
    .type ESAL_AR_STK_Startup_SP_Set, %function
ESAL_AR_STK_Startup_SP_Set:

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    MRS     r0,control
    AND     r0,r0,#ESAL_AR_STK_CONTROL_FP_MASK
    MSR     control,r0

    @ CPACR is located at address 0xE000ED88

    LDR.W   r0,=ESAL_AR_STK_CPACR_ADDR
    LDR     r1,[r0]
    AND     r1,r1,#ESAL_AR_STK_CPACR_CP11_CP10_CLR
    STR     r1,[r0]

    @ Enable automatic and lazy state preservation for the floating-point unit.

    LDR.W   r0,=ESAL_AR_STK_FPCCR_ADDR
    LDR     r1,[r0]
    AND     r1,r1,#ESAL_AR_STK_FPCCR_LAZY_MASK
    STR     r1,[r0]
    LDR     r1,[r0]

    @ CPACR is located at address 0xE000ED88

    LDR.W   r0,=ESAL_AR_STK_CPACR_ADDR

    @ Read CPACR

    LDR     r1,[r0]

    @ Set bits 20-23 to enable CP10 and CP11 coprocessors

    ORR     r1,r1,#(0xF << 20)

    @ Write back the modified value to the CPACR

    STR     r1,[r0]

    @ Enable the VFP

    MOV     r0,#0x03000000
    VMSR    FPSCR,r0

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    @ Get start address of available memory that can be used for the
    @ startup stack.

    LDR     r0,ESAL_AR_STK_BSS_End

    @ Add stack size to get usable stack pointer

    ADD     r0,r0,#ESAL_AR_STK_STARTUP_STACK_SIZE

    @ Align the stack pointer as required.

    BIC     r0,r0,#ESAL_AR_STK_ALIGNMENT_MASK
    MOV     sp,r0

    @ Jump to toolset run-time environment setup function - control will not return.

    B       ESAL_TS_RTE_Lowlevel_Initialize

    .balign   4    @ Align to 4 byte

@************************************************************************
@*
@*   FUNCTION
@*
@*       ESAL_TS_RTE_Lowlevel_Initialize
@*
@*   DESCRIPTION
@*
@*       This function initializes registers and hardware as required
@*       for a run-time environment.  Many toolsets require a certain
@*       environment to be set-up to allow correct run-time execution
@*       in a C environment.  This may include initializing certain
@*       architecture registers (base registers, mode registers, etc)
@*       and anything else specified in the toolset documentation.
@*
@*   CALLED BY
@*
@*       ESAL_AR_STK_Startup_SP_Set
@*
@*   CALLS
@*
@*       OS_Init_Entry                          OS Entry Point
@*
@*   INPUTS
@*
@*       None
@*
@*   OUTPUTS
@*
@*       None
@*
@************************************************************************
@VOID    ESAL_TS_RTE_Lowlevel_Initialize(VOID)

    .thumb @ ESAL_TS_RTE_Lowlevel_Initialize
    .global ESAL_TS_RTE_Lowlevel_Initialize
    .type ESAL_TS_RTE_Lowlevel_Initialize, %function
ESAL_TS_RTE_Lowlevel_Initialize:

    @ Zero out the lr to provide a nicer debug environment (call stack-trace)

    MOV     r0,#0
    MOV     r14,r0

    @ Call OS entry point
    @ NOTE:   Control will not return here

    B       OS_Init_Entry


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_TS_STK_Solicited_Restore
@*
@*  DESCRIPTION
@*
@*      This function restores the context of a stack frame as required
@*      by a given toolset (stack frame that contains registers
@*      required to be saved across function call boundaries)
@*
@*  CALLED BY
@*
@*      Operating System Services
@*
@*  CALLS
@*
@*      None
@*
@*  INPUTS
@*
@*      stack_ptr                           Stack pointer of
@*                                          stack frame to be restored
@*
@*  OUTPUTS
@*
@*      None
@*
@************************************************************************
@VOID    ESAL_TS_STK_Solicited_Restore(VOID *stack_ptr)

    .thumb @ ESAL_TS_STK_Solicited_Restore
    .global ESAL_TS_STK_Solicited_Restore
    .type ESAL_TS_STK_Solicited_Restore, %function
ESAL_TS_STK_Solicited_Restore:

    @ Switch to the stack pointer passed-in and skip stack type

    MOV     sp,r0

    @ Restore all registers stored in the solicited
    @ (stack type in r3)

    LDMIA   sp!,{r3-r11,lr}

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    @ Restore required floating point registers from the current stack.

    LDMIA   sp!,{r2}

    VMSR    FPSCR,r2

    VLDMIA  sp!,{d0-d15}

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    @ Return to caller

    BX      lr


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_TS_STK_Solicited_Switch
@*
@*  DESCRIPTION
@*
@*      This function saves the necessary registers, designated by a
@*      given toolset, to be preserved across a function call
@*      boundary.  Control is then transferred to the designated
@*      call-back function with the designated call-back parameter.
@*
@*  CALLED BY
@*
@*      Operating System Services
@*
@*  CALLS
@*
@*      <call back function>
@*
@*  INPUTS
@*
@*      call_back_param                     Parameter passed to callback
@*                                          function
@*      call_back                           Call back function pointer
@*      stack_ptr                           Pointer to stack pointer
@*                                          storage address
@*
@*  OUTPUTS
@*
@*      None
@*
@************************************************************************
@VOID    ESAL_TS_STK_Solicited_Switch(VOID *call_back_param,
@                                     VOID (*call_back)(VOID),
@                                     VOID **stack_ptr)

    .thumb @ ESAL_TS_STK_Solicited_Switch
    .global ESAL_TS_STK_Solicited_Switch
    .type ESAL_TS_STK_Solicited_Switch, %function
ESAL_TS_STK_Solicited_Switch:

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    @ Save required floating point registers on the current stack.

    VSTMDB   sp!,{d0-d15}

    VMRS    r3,FPSCR

    STMDB   sp!,{r3}

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    @ Put stack type in r3

    MOV     r3,#0

    @ Save the toolset required registers to the current stack.
    @ NOTE:   The format of this stack frame is contained in esal_ts_stk_defs.h
    @         (structure name is ESAL_TS_STK).

    STMDB   sp!,{r3-r11,lr}

    @ Save resultant stack pointer back to location passed in

    STR     sp,[r2]

    @ Switch to the system stack

    LDR     r2,ESAL_TS_STK_System_SP
    LDR     sp,[r2]

    @ Transfer control to callback function (call-back parameter is
    @ already in register r0).

    BX      r1

    .balign   4    @ Align to 4 byte


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_ISR_Interrupt_Handler
@*
@*  DESCRIPTION
@*
@*      Handler for Cortex SysTick and External interrupts
@*
@*  CALLED BY
@*
@*      Interrupt / Exception Vector Table
@*
@*  CALLS
@*
@*      Registered Exception Handler
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
@VOID    ESAL_AR_ISR_Interrupt_Handler(VOID)

    .thumb @ ESAL_AR_ISR_Interrupt_Handler
    .global  ESAL_AR_ISR_Interrupt_Handler
    .type ESAL_AR_ISR_Interrupt_Handler, %function
ESAL_AR_ISR_Interrupt_Handler:

    @ Disable interrupts during critical section

    CPSID   i
    ISB

#if (CFG_NU_OS_SVCS_TRACE_CORE_PC_HOTSPOT_SUPPORT == 1)

    @ Retrieve Link register from interrupt stack

    LDR   r0,[sp,#ESAL_AR_STK_MIN_LR_OFFSET]

    @ Save LR

    LDR     r2,=Trace_PC_Sample
    STR     r0,[r2]

#endif

    .if ESAL_AR_ISR_HOOK_ENABLED

    @ Jump to hook function

    BL      ESAL_AR_ISR_HOOK

    .endif @ ESAL_AR_ISR_HOOK_ENABLED

    @ Calculate the vector using the
    @ interrupt / program status register (IPSR)

    MRS     r0,ipsr
    SUB     r0,r0,#ESAL_AR_ISR_INTERRUPT_VECT_ADJUST

    @ Move SP into r1 (2nd parameter)

    MOV     r1,sp

    @ Set counter showing ISR is executing

    LDR     r2,=ESAL_GE_ISR_Executing
    LDR     r3,[r2]
    ADD     r3,r3,#1
    STR     r3,[r2]

    @ Check if this interrupt occurred during execution of
    @ another interrupt (nested)
    @ NOTE:  This uses the saved XPSR value instead of the
    @        ESAL_GE_ISR_Executing flag.  The ensures nested
    @        conditions are correctly detected.

    LDR     r2,[sp,#ESAL_AR_ISR_XPSR_OFFSET]
    AND     r2,r2,#ESAL_AR_ISR_VECT_MASK
    CMP     r2,#ESAL_AR_ISR_INTERRUPT_VECT_ADJUST
    BGE     ESAL_AR_ISR_Interrupt_Nested

    @ Switch to the system stack

    LDR     sp,=ESAL_GE_STK_System_SP
    LDR     sp,[sp]

    @ Get address of non-nested entry to OS

    LDR     r2,=ESAL_GE_ISR_OS_Entry
    LDR     r2,[r2]

    @ Enable all interrupts to allow nesting

    CPSIE   i

    @ Execute handler

    BLX     r2

    @ Control returns here after ISR with interrupts disabled
    @ Check if context switch is required as a result of
    @ the interrupt service routine

    LDR     r2,=ESAL_GE_STK_Unsol_Switch_Req
    LDR     r2,[r2]
    CMP     r2,#0

    @ Perform context switch if required

    BNE     ESAL_AR_STK_Unsolicited_Switch

    @ Get stack pointer if not performing context switch

    LDR     sp,[r0]

    @ Get return from exception value

    MOV     r0,#ESAL_AR_ISR_RTN_EXCEPT

    @ Enable all interrupts to allow nesting

    CPSIE    i

    @ Return from exception (non-nested)

    BX      r0

ESAL_AR_ISR_Interrupt_Nested:

    @ Align stack pointer to 8-byte boundary (r1 contains current SP)

    BIC     r2,r1,#ESAL_AR_STK_ALIGNMENT_MASK

    @ Switch to the aligned SP (original SP still in r1)

    MOV     sp,r2

    @ Save r1 (original SP) and lr (exception return value) on the stack
    @ NOTE:  This preserves 8-byte alignment

    STMDB   sp!,{r1,lr}

    @ Get address of nested entry to OS

    LDR     r2,=ESAL_GE_ISR_OS_Nested_Entry
    LDR     r2,[r2]

    @ Enable all interrupts to allow nesting

    CPSIE   i

    @ Execute handler

    BLX     r2

    @ Restore r1 and lr from stack

    LDMIA   sp!,{r1,lr}

    @ Switch back to original SP (before forced alignment)

    MOV     sp,r1

    @ See if nested count is zero

    @ Set counter showing ISR is executing

    LDR     r2,=ESAL_GE_ISR_Executing
    LDR     r2,[r2]
    CMP     r2,#0
    BEQ     ESAL_AR_ISR_Interrupt_Nested_Return

    @ Enable interrupts before returning from exception

    CPSIE   i

ESAL_AR_ISR_Interrupt_Nested_Return:

    @ Return from exception (nested)

    BX      lr


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_ISR_Exception_Handler
@*
@*  DESCRIPTION
@*
@*      Handler for Cortex Exceptions
@*
@*  CALLED BY
@*
@*      Interrupt / Exception Vector Table
@*
@*  CALLS
@*
@*      Registered exception handler
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
@VOID    ESAL_AR_ISR_Exception_Handler(VOID)

    .thumb @ ESAL_AR_ISR_Exception_Handler
    .global  ESAL_AR_ISR_Exception_Handler
    .type ESAL_AR_ISR_Exception_Handler, %function
ESAL_AR_ISR_Exception_Handler:

    @ Read the Control and State Register (ICSR)

    LDR     r0,=ESAL_AR_ISR_NVIC_ICSR_ADDR
    LDR     r0,[r0]

    @ Calculate the exception vector ID in r0 (1st parameter)

    AND     r0,r0,#ESAL_AR_ISR_VECT_MASK
    SUB     r0,r0,#ESAL_AR_ISR_EXCEPT_VECT_ADJUST

    @ Put the stack pointer in r1 (2nd parameter)

    MOV     r1,sp

    @ Get the address of the exception handler dispatch table

    LDR     r2,=ESAL_GE_ISR_Exception_Handler

    @ Get address of the exception handler for this vector ID

    LDR     r2,[r2,r0,LSL #2]

    @ Execute the exception handler

    BLX     r2

    @ Return from exception

    MOV     r0,#ESAL_AR_ISR_RTN_EXCEPT
    BX      r0


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_ISR_SVC_Handler
@*
@*  DESCRIPTION
@*
@*      Handler for Cortex SVC Exception.  This handler is
@*      used to allow the an unsolicited stack restore to
@*      be completed.  Because exceptions can only be
@*      returned from handler mode, the SVC exception is
@*      entered to perform the final restoration of an
@*      unsolicited stack frame.
@*
@*  CALLED BY
@*
@*      Interrupt / Exception Vector Table
@*
@*  CALLS
@*
@*      None
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
@VOID    ESAL_AR_ISR_SVC_Handler(VOID)

    .thumb @ ESAL_AR_ISR_SVC_Handler
    .global  ESAL_AR_ISR_SVC_Handler
    .type ESAL_AR_ISR_SVC_Handler, %function
ESAL_AR_ISR_SVC_Handler:

    @ Adjust stack pointer to remove registers saved
    @ by hardware for the SVC exception

    ADD     sp,sp,#32

    @ Re-enable priority 1 and above interrupts before returning from exception

    MOV     r0,#0x00
    MSR     basepri,r0

    @ Return from exception (restores remaining registers
    @ from an unsolicited stack frame)

    BX      lr


@************************************************************************
@*
@*  FUNCTION
@*
@*      ESAL_AR_ISR_Return
@*
@*  DESCRIPTION
@*
@*      For the Cortex-M3, a special return from exception must be
@*      performed to change from handler state to thread state.  Lower
@*      priority exceptions are disabled until this return occurs.
@*
@*  CALLED BY
@*
@*      OS Services
@*
@*  CALLS
@*
@*      None
@*
@*  INPUTS
@*
@*      rtn_addr                            Function return address
@*
@*  OUTPUTS
@*
@*      None
@*
@************************************************************************
@VOID    ESAL_AR_ISR_Return(VOID (*rtn_addr)(VOID))

    .thumb @ ESAL_AR_ISR_Return
    .global  ESAL_AR_ISR_Return
    .type ESAL_AR_ISR_Return, %function
ESAL_AR_ISR_Return:

    @ Subtract space on current stack to build exception stack

    SUB     sp,sp,#32

    @ Save return address on stack

    STR     r0,[sp,#ESAL_AR_ISR_PC_OFFSET]

    @ Get psr value and put on stack

    MOV     r0,#ESAL_AR_ISR_XPSR_VALUE
    STR     r0,[sp,#ESAL_AR_ISR_XPSR_OFFSET]

    @ Perform return from exception

    MOV     r0,#ESAL_AR_ISR_RTN_EXCEPT
    BX      r0

    .balign   4    @ Align to 4 byte

    .end
