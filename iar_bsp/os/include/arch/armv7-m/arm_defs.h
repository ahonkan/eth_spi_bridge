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
*       arm_defs.h
*
*   DESCRIPTION
*
*       This file contains all definitions, structures, etc for the
*       base ARM architecture.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
***********************************************************************/

#ifndef         ARM_DEFS_H
#define         ARM_DEFS_H

/* Define if the OS timer is part of the architecture (NU_TRUE)
   NOTE:  The OS timer can be located at only one of the following levels of
          abstraction: the processor level or the architecture level.
          If ESAL_AR_OS_TIMER_USED is NU_TRUE, the OS timer is contained
          within the architecture.  If ESAL_AR_OS_TIMER_USED is NU_FALSE,
          the OS timer is contained within the processor component. */
#define         ESAL_AR_OS_TIMER_USED                   NU_TRUE

/* Define if architecture supports unaligned 16-bit accesses to memory. */
#define         ESAL_AR_UNALIGNED_16BIT_SPT             NU_FALSE

/* Define if architecture supports unaligned 32-bit accesses to memory. */
#define         ESAL_AR_UNALIGNED_32BIT_SPT             NU_FALSE

/* Define number of accesses required to read or write a pointer */
#define         ESAL_AR_PTR_ACCESS                      1

/* Define number of accesses required to read or write a 32-bit value */
#define         ESAL_AR_32BIT_ACCESS                    1

/* Size, in bytes, of architecture system stack.  This stack will be
   utilized when servicing interrupts. */
#define         ESAL_AR_SYSTEM_STACK_SIZE               CFG_NU_OS_ARCH_ARMV7_M_COM_SYSTEM_STACK_SIZE

#define         ESAL_AR_STK_XPSR_INIT                   0x01000000

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

/* Define number of single-precision floating point registers (s0-s31) */
#define         ESAL_AR_STK_NUM_FPU_REGS                32

#endif /* (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT == 2) */

/* Define stack frame structure for minimum architecture registers required
   to be saved in order to enter a C environment during in interrupt / exception.
   These registers are the "scratch" registers that will not be preserved across
   a function call boundary and any interrupt state registers that must preserved
   to allow interrupt nesting. */
typedef struct
{
    UINT32              r0;
    UINT32              r1;
    UINT32              r2;
    UINT32              r3;
    UINT32              r12;
    UINT32              lr;
    UINT32              rtn_address;
    UINT32              xpsr;

} ESAL_AR_STK_MIN;

/* Define stack frame structure for the architecture supported.
   This stack frame contains all registers that must be preserved
   across an (unsolicited) interrupt context switch.
   NOTE:  This stack frame includes the minimum stack frame
          defined above AND all other registers for the given
          architecture. */
typedef struct  ESAL_AR_STK_STRUCT
{
    UINT32              stack_type;
    UINT32              r4;
    UINT32              r5;
    UINT32              r6;
    UINT32              r7;
    UINT32              r8;
    UINT32              r9;
    UINT32              r10;
    UINT32              r11;

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    UINT32              fpscr;
    UINT32              s[ESAL_AR_STK_NUM_FPU_REGS];

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    ESAL_AR_STK_MIN     min_stack;

} ESAL_AR_STK;

/* Define stack frame structure for the toolset / architecture supported. */
typedef struct  ESAL_TS_STK_STRUCT
{
    UINT32          stack_type;
    UINT32          r4;
    UINT32          r5;
    UINT32          r6;
    UINT32          r7;
    UINT32          r8;
    UINT32          r9;
    UINT32          r10;
    UINT32          r11;
    UINT32          rtn_address;

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    UINT32          fpscr;
    UINT32          s[ESAL_AR_STK_NUM_FPU_REGS];

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

} ESAL_TS_STK;


/* Size, in bytes, of architecture exception stack.  This stack will be
   utilized when servicing exceptions. */
#define         ESAL_AR_EXCEPTION_STACK_SIZE            0

/* Define if interrupt servicing initialization is required at the
   architecture level. */
#define         ESAL_AR_ISR_INIT_REQUIRED               NU_TRUE

/* Define if architecture mandates that all interrupt handlers perform a
   "return from interrupt" (RTI) instruction in order for the hardware to
   correctly restore the state of execution to the pre-interrupt condition.
   NOTE:  Most architectures allow the state of execution to be restored
          without needing to perform an RTI.  In most cases, this will be set
          to NU_FALSE */
#define         ESAL_AR_ISR_RTI_MANDATORY               NU_TRUE

/* Determines where the ISR nesting counter is incremented.  
   When set to 0 the increment occurs in assembly files, when 
   set to 1 the increment will occur in c files. */
#define         ESAL_AR_ISR_INCREMENT_IN_C              NU_FALSE


/* Define bit values for the architecture's status register / machine state register /
   etc that are used to enable and disable interrupts for the given architecture. */
#define         ESAL_AR_INTERRUPTS_DISABLE_BITS         0x00000001
#define         ESAL_AR_INTERRUPTS_ENABLE_BITS          0x00000000

/* Address of system handler and control state register */
#define         ESAL_AR_INT_SHCSR_ADDR                  0xE000ED24

/* System handler and control state register bit definitions */
#define         ESAL_AR_INT_SHCSR_USGFAULTENA           ESAL_GE_MEM_32BIT_SET(18)
#define         ESAL_AR_INT_SHCSR_BUSFAULTENA           ESAL_GE_MEM_32BIT_SET(17)
#define         ESAL_AR_INT_SHCSR_MEMFAULTENA           ESAL_GE_MEM_32BIT_SET(16)

/* Define base address of the Nested Vectored Interrupt Controller (NVIC). */
#define         ESAL_AR_INT_NVIC_BASE_ADDR              0xE000E000

/* Define offsets of various NVIC registers used for interrupt control. */
#define         ESAL_AR_INT_NVIC_SETENA0_OFFSET         0x100
#define         ESAL_AR_INT_NVIC_SETENA1_OFFSET         0x104
#define         ESAL_AR_INT_NVIC_CLRENA0_OFFSET         0x180
#define         ESAL_AR_INT_NVIC_CLRENA1_OFFSET         0x184
#define         ESAL_AR_INT_NVIC_PRIORITY_OFFSET        0x400

/* Define NVIC related constants */
#define         ESAL_AR_INT_NVIC_REG_SIZE               4
#define         ESAL_AR_INT_NVIC_MAX_CLR_REGS           7
#define         ESAL_AR_INT_NVIC_ENABITS_PER_REG        32
#define         ESAL_AR_INT_NVIC_ENABIT_MASK            (ESAL_AR_INT_NVIC_ENABITS_PER_REG - 1)
#define         ESAL_AR_INT_NVIC_PRI_PER_REG            4
#define         ESAL_AR_INT_NVIC_PRI_BITS               8
#define         ESAL_AR_INT_NVIC_PRI_BIT_MASK           0xFF
#define         ESAL_AR_INT_NVIC_PRI_SHIFT_MASK         (ESAL_AR_INT_NVIC_PRI_PER_REG - 1)
#define         ESAL_AR_INT_NVIC_DISABLE_VALUE          0xFFFFFFFF


/* Define NVIC register offsets related to interrupt servicing */
#define         ESAL_AR_ISR_NVIC_VECTTBL_OFFSET         0xD08
#define         ESAL_AR_ISR_NVIC_AIRCR_OFFSET           0xD0C

/* Define init value for application interrupt and reset control register
   NOTE:  This sets no pre-emption priority and resets interrupt / fault state (for debugging) */
#define         ESAL_AR_ISR_NVIC_AIRCR_INIT             0x05FA0002


/* Define ESAL interrupt vector IDs for this architecture.
These IDs match up with architecture interrupts.
Values correspond to the index of entries in ESAL_GE_ISR_Interrupt_Handler[].
Names are of the form ESAL_AR_<Name>_INT_VECTOR_ID, where <Name> comes
directly from the hardware documentation */
#define         ESAL_AR_SYSTICK_INT_VECTOR_ID           0

/* Define the last ESAL interrupt vector ID for this architecture + 1 */
#define         ESAL_AR_INT_VECTOR_ID_DELIMITER         (ESAL_AR_SYSTICK_INT_VECTOR_ID + 1)

/* Define ESAL exception vector IDs for the architecture.
These IDs match up with architecture exceptions.
Values correspond to the index of entries in ESAL_GE_ISR_Exception_Handler[].
Names are of the form ESAL_AR_<Name>_EXCEPT_VECTOR_ID, where <Name> comes
directly from the hardware documentation */
#define         ESAL_AR_NMI_EXCEPT_VECTOR_ID            0
#define         ESAL_AR_HARDFAULT_EXCEPT_VECTOR_ID      1
#define         ESAL_AR_MEM_MGMT_EXCEPT_VECTOR_ID       2
#define         ESAL_AR_BUS_FAULT_EXCEPT_VECTOR_ID      3
#define         ESAL_AR_USAGE_FAULT_EXCEPT_VECTOR_ID    4
#define         ESAL_AR_SVCALL_EXCEPT_VECTOR_ID         9
#define         ESAL_AR_DEBUG_EXCEPT_VECTOR_ID          10
#define         ESAL_AR_PENDSV_EXCEPT_VECTOR_ID         12

/* Define the last ESAL exception vector ID for this architecture + 1 */
#define         ESAL_AR_EXCEPT_VECTOR_ID_DELIMITER  (ESAL_AR_PENDSV_EXCEPT_VECTOR_ID + 1)

/* Define variable(s) required to save / restore architecture interrupt state.
   These variable(s) are used in conjunction with the ESAL_AR_INT_ALL_DISABLE() and
   ESAL_AR_INT_ALL_RESTORE() macros to hold any data that must be preserved in
   order to allow these macros to function correctly. */
#define         ESAL_AR_INT_CONTROL_VARS            INT  esal_tmp_val;

/* This macro locks out interrupts and saves the current
   architecture status register / state register to the specified
   address.  This function does not attempt to mask any bits in
   the return register value and can be used as a quick method
   to guard a critical section.
   NOTE:  This macro is used in conjunction with ESAL_AR_INT_ALL_RESTORE
          defined below and ESAL_AR_INT_CONTROL_VARS defined above. */
#define         ESAL_AR_INT_ALL_DISABLE()                                       \
                {                                                               \
                    ESAL_TS_RTE_PRIMASK_READ(&esal_tmp_val);                    \
                    ESAL_TS_RTE_CPSID_EXECUTE();                                \
                }

/* This macro restores the architecture status / state register
   used to lockout interrupts to the value provided.  The
   intent of this function is to be a fast mechanism to restore the
   interrupt level at the end of a critical section to its
   original level.
   NOTE:  This macro is used in conjunction with ESAL_AR_INT_ALL_DISABLE
          and ESAL_AR_INT_CONTROL_VARS defined above. */
#define         ESAL_AR_INT_ALL_RESTORE()                                       \
                    ESAL_TS_RTE_PRIMASK_WRITE(esal_tmp_val)


/* This macro locks-out interrupts but doesn't save the status
   register / control register value. */
#define         ESAL_AR_INT_FAST_ALL_DISABLE()                                  \
                    ESAL_TS_RTE_CPSID_EXECUTE()

/* This macro unlocks interrupts but doesn't save the status
   register / control register value. */
#define         ESAL_AR_INT_FAST_ALL_ENABLE()                                   \
                    ESAL_TS_RTE_CPSIE_EXECUTE()

/* This macro sets the interrupt related bits in the status register / control
   register to the specified value. */
#define         ESAL_AR_INT_BITS_SET(set_bits)                                  \
                    ESAL_TS_RTE_PRIMASK_WRITE(set_bits)

/* This macro gets the interrupt related bits from the status register / control
   register. */
#define         ESAL_AR_INT_BITS_GET(get_bits_ptr)                              \
                    ESAL_TS_RTE_PRIMASK_READ(get_bits_ptr)

/* System Tick priority
NOTE:  The priority should always be greater than 0 */
#define ESAL_AR_TMR_SYSTICK_PRIORITY            3UL

#define ESAL_AR_TMR_SYSTICK

/* System Tick register address defines */
#define ESAL_AR_TMR_SYSTICK_CTRL                0xE000E010
#define ESAL_AR_TMR_SYSTICK_RELOAD              0xE000E014
#define ESAL_AR_TMR_SYSTICK_CURRENT             0xE000E018
#define ESAL_AR_TMR_SYSTICK_CALIBRATE           0xE000E01C
#define ESAL_AR_TMR_SYS_HANDLER_PRI             0xE000ED20

/* System Tick register bit defines */
#define ESAL_AR_TMR_SYSTICK_CTRL_ENABLE_BIT     ESAL_GE_MEM_32BIT_SET(0)
#define ESAL_AR_TMR_SYSTICK_CTRL_TICKINT_BIT    ESAL_GE_MEM_32BIT_SET(1)
#define ESAL_AR_TMR_SYSTICK_CTRL_CLKSRC_BIT     ESAL_GE_MEM_32BIT_SET(2)
#define ESAL_AR_TMR_SYSTICK_CTRL_COUNT_FLAG     ESAL_GE_MEM_32BIT_SET(16)

#define ESAL_AR_TMR_SYS_HANDLER_PRI_SHIFT       (32 - ESAL_PR_INT_NUM_PRIORITY_BITS)

/* The following definitions / macros / etc are only used if the architecture
   is configured (in esal_ar_cfg.h) to use an architecture level timer for
   the OS timer. */
#if (ESAL_AR_OS_TIMER_USED == NU_TRUE)

/* Define the processor OS timer type (count-down or count-up) */
#define         ESAL_AR_TMR_OS_COUNT_DIR        ESAL_COUNT_DOWN

/* Define for the processor OS timer interrupt vector */
#define         ESAL_AR_TMR_OS_VECTOR           ESAL_AR_SYSTICK_INT_VECTOR_ID

/* Define a macro to read the processor OS timer hardware count.  The
   resultant size of the count must be 32-bits, regardless of the actual
   size of the timer used (8-bit, 16-bit, 32-bit, etc). */
#define         ESAL_AR_TMR_OS_COUNT_READ()                         \
                    ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CURRENT)

/* Define the EOI logic for the processor OS timer */
#define         ESAL_AR_TMR_OS_TIMER_EOI(vector)                    \
                    ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL)


/* Define generic macro for OS timer pending. The macro reads COUNTFLAG field
    to determine if value counted to 0 since last time it is read. */
#define         ESAL_AR_TMR_PENDING()                               \
    ((ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL) & ESAL_AR_TMR_SYSTICK_CTRL_COUNT_FLAG) != 0)

/* Load timer count value */
#define ESAL_AR_TMR_TICK_VALUE_SET(interval)                           \
{{                                                                     \
    UINT32 __ctrl_32, __reload_32 = interval, __int_level;             \
    /* Disable interrupts. */                                          \
    __int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS); \
    __ctrl_32 = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL);          \
    /* Clear all bits except clock source. */                          \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CTRL,                      \
              (__ctrl_32 & ESAL_AR_TMR_SYSTICK_CTRL_CLKSRC_BIT));      \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_RELOAD, __reload_32);      \
    /* Set count value to zero so that next reload has correct counter */     \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CURRENT, 0);               \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CTRL, __ctrl_32);          \
    /* Restore interrupts to previous level */                         \
    NU_Local_Control_Interrupts (__int_level);                         \
}}

/* Define method for which PMS will work with the counter.  In most cases this will
   match the OS timer direction.  On some rarer cases it may be needed to differ, such
   cases include timers that don't start at 0 but count up. */
#define ESAL_AR_TMR_PMS_COUNT_METHOD        ESAL_AR_TMR_OS_COUNT_DIR

/*
 * ESAL_AR_TMR_PMS_IS_TIMER_INT_PENDING() checks whether a hardware tick timer interrupt is
 * pending at this time.
 * It is used to check if a race condition occurred, CPU woke up due to
 * other HW interrupt but a tick occurred between the interrupt and any
 * hardware tick counter sampling.
 */
#define ESAL_AR_TMR_PMS_IS_TIMER_INT_PENDING()  ESAL_GE_TMR_OS_PENDING()

/*
 * ESAL_AR_TMR_PMS_SET_HW_TICK_INTERVAL(interval) sets the hardware tick timer interval
 * It is used and required only for UP counting hardware timer counters.
 */

#if(ESAL_AR_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
#define ESAL_AR_TMR_PMS_SET_HW_TICK_INTERVAL(interval)
#endif

/*
 * ESAL_AR_TMR_PMS_GET_HW_TICK_CNT_VALUE() reads the current hardware tick timer counter value
 * This typically can be left mapped to ESAL_GE_TMR_OS_COUNT_READ
 */
#define ESAL_AR_TMR_PMS_GET_HW_TICK_CNT_VALUE()         ESAL_GE_TMR_OS_COUNT_READ()

/* This macro sets the current hardware tick timer counter value
 * It is used and required only for DOWN counting hardware timer counters
 * and only if ESAL_PR_TMR_PMS_ADJUST_HW_TICK_VALUE is not defined.
 * ESAL_PR_TMR_PMS_SET_HW_TICK_VALUE should only be used if ESAL_PMS_ADJUST_HW_TICK function
 * in unachievable because it potentially introduces small tick drift
 * when the software does read-modify-write adjustments to the counter value.
 */
#if(ESAL_AR_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN)
#ifndef ESAL_AR_TMR_PMS_ADJUST_HW_TICK_VALUE
#define ESAL_AR_TMR_PMS_SET_HW_TICK_VALUE(value)        ESAL_AR_TMR_TICK_VALUE_SET(value)
#endif
#endif

#endif  /* ESAL_AR_OS_TIMER_USED == NU_TRUE */


#endif  /* ARM_DEFS_H */
