@***********************************************************************
@*
@*             Copyright 2010 Mentor Graphics Corporation
@*                         All Rights Reserved.
@*
@* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
@* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
@* SUBJECT TO LICENSE TERMS.
@*
@***********************************************************************

@***********************************************************************
@*
@*  FILE NAME
@*
@*      csgnu_arm_defs.inc
@*
@*  DESCRIPTION
@*
@*      This file contains the architecture specific definitions,
@*      constants, etc, for CSGNU for ARM
@*
@*  DATA STRUCTURES
@*
@*      None
@*
@*  DEPENDENCIES
@*
@*      None
@*
@***********************************************************************

.set ESAL_AR_STK_CONTROL_FP_MASK,               0xFFFFFFFB
.set ESAL_AR_STK_CPACR_ADDR,                    0xE000ED88
.set ESAL_AR_STK_CPACR_CP11_CP10_CLR,           0xFF0FFFFF
.set ESAL_AR_STK_FPCCR_ADDR,                    0xE000EF34
.set ESAL_AR_STK_FPCCR_LAZY_MASK,               0x3FFFFFFF

@ Define if v7 architecture support is available.

    .global ESAL_AR_STK_V7_SUPPORT
.set ESAL_AR_STK_V7_SUPPORT,                    0

@ Define to allow conditional assembling of lowlevel ISR hook.
@ Setting this to TRUE will allow a low-level hook to be executed in the
@ interrupt handlers

    .global ESAL_AR_ISR_HOOK_ENABLED
.set ESAL_AR_ISR_HOOK_ENABLED,                  0

@ Define to allow conditional assembling of reset vector type (branch versus load).
@ Setting this to TRUE will force the reset vector to utilize a branch instruction.
@ This is necessary for certain processors that re-map memory and require a PC
@ relative jump to the entry point versus a full 32-bit address (before remapping memory).

    .global ESAL_AR_ISR_RESET_BRANCH
.set ESAL_AR_ISR_RESET_BRANCH,                  0

@ Startup / initialization stack size in bytes

.equ ESAL_AR_STK_STARTUP_STACK_SIZE,            1024

@ Constant used to perform a return from exception.
@ The upper 28 bits are always 1 - the bottom 4 bits
@ are set to b1001 to allow a return from handler state
@ to thread state and to use the main stack

.equ ESAL_AR_ISR_RTN_EXCEPT,                    0xFFFFFFF9

@ Constant used to perform a nested return from exception.
@ The upper 28 bits are always 1 - the bottom 4 bits
@ are set to b0001 to allow a return from handler state
@ to handler state and to use the main stack (for nested interrupts)

.equ ESAL_AR_ISR_RTN_EXCEPT_NEST,               0xFFFFFFF1

@ Define offset on stack for special registers required
@ to perform a return from exception

.equ ESAL_AR_ISR_PC_OFFSET,                     0x18
.equ ESAL_AR_ISR_XPSR_OFFSET,                   0x1C

@ Define XPSR value that must be inserted in exception stack
@ when performing a return from exception

.equ ESAL_AR_ISR_XPSR_VALUE,                    0x01000000

@ Define register address to get exception number
@ This is the Interrupt Control State register

.equ ESAL_AR_ISR_NVIC_ICSR_ADDR,                0xE000ED04

@ Define ICSR masks and adjustment values to obtain
@ interrupt vector IDs and exception vector IDs

.equ ESAL_AR_ISR_INTERRUPT_VECT_ADJUST,         15
.equ ESAL_AR_ISR_EXCEPT_VECT_ADJUST,            2
.equ ESAL_AR_ISR_VECT_MASK,                     0xFF

@ Size, in bytes, of max architecture stack frames (without floating point)

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

@ Size, in bytes, of max architecture stack frames (with floating point)

.equ ESAL_AR_STK_MAX_FRAME_SIZE,                200

#else

@ Size, in bytes, of max architecture stack frames (without floating point)

.equ ESAL_AR_STK_MAX_FRAME_SIZE,                68

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

@ Offset, in bytes, of LR in full stack frame

.equ ESAL_AR_STK_PC_OFFSET,                     (ESAL_AR_STK_MAX_FRAME_SIZE-4)
.equ ESAL_AR_STK_LR_OFFSET,                     (ESAL_AR_STK_MAX_FRAME_SIZE-8)
.equ ESAL_AR_STK_SPSR_OFFSET,                   (ESAL_AR_STK_MAX_FRAME_SIZE-12)
.equ ESAL_AR_STK_R12_OFFSET,                    (ESAL_AR_STK_MAX_FRAME_SIZE-16)

@ Size, in bytes, of min architecture stack frames

.equ ESAL_AR_STK_MIN_FRAME_SIZE,                32

@ Based on ESAL_TS_REQ_STK_ALIGNMENT in esal_ts_cfg.h

.equ ESAL_AR_STK_ALIGNMENT_MASK,                0x00000007

@ Offsets, in bytes, of registers on temp stack

.equ ESAL_AR_STK_TMP_R0_OFFSET,                 0
.equ ESAL_AR_STK_TMP_SPSR_OFFSET,               4
.equ ESAL_AR_STK_TMP_LR_OFFSET,                 8

@ Offsets, in bytes, of registers in minimum stack frame

.equ ESAL_AR_STK_MIN_PC_OFFSET,                 0x18
.equ ESAL_AR_STK_MIN_LR_OFFSET,                 24
.equ ESAL_AR_STK_MIN_SPSR_OFFSET,               20
.equ ESAL_AR_STK_MIN_R12_OFFSET,                16

