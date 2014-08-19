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
*       csgnu_arm_defs.h
*
*   DESCRIPTION
*
*       This file contains all definitions, structures, etc for the
*       CS GNU for ARM toolset
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

#ifndef         CSGNU_ARM_DEFS_H
#define         CSGNU_ARM_DEFS_H

/* Define required for RTL support. */
#ifndef         ESAL_TS_RTL_SUPPORT
#define         ESAL_TS_RTL_SUPPORT                     NU_TRUE
#endif

/* Define required stack pointer alignment for the given toolset.*/
#define         ESAL_TS_REQ_STK_ALIGNMENT               8

/* Define if toolset supports 64-bit data types (long long) */
#define         ESAL_TS_64BIT_SUPPORT                   NU_TRUE

/* Define, in bytes, toolset minimum required alignment for structures */
#define         ESAL_TS_STRUCT_ALIGNMENT                4

/* Size, in bits, of integers for the given toolset / architecture */
#define         ESAL_TS_INTEGER_SIZE                    32

/* Size, in bits, of code pointer for the given toolset / architecture */
#define         ESAL_TS_CODE_PTR_SIZE                   32

/* Size, in bits, of data pointer for the given toolset / architecture */
#define         ESAL_TS_DATA_PTR_SIZE                   32

/* Define structure padding requirement. */

#define         ESAL_TS_PAD_1BYTE_ALIGN 3
#define         ESAL_TS_PAD_2BYTE_ALIGN 2
#define         ESAL_TS_PAD_3BYTE_ALIGN 1

/* Define if necessary to copy code / data from ROM to RAM */
#define         ESAL_TS_ROM_TO_RAM_COPY_SUPPORT         CFG_NU_OS_KERN_PLUS_CORE_ROM_TO_RAM_COPY

/* Define, in bytes, toolset maximum alignment for data types. */
#define         ESAL_TS_MAX_TYPE_ALIGNMENT              ESAL_TS_REQ_STK_ALIGNMENT

/* Define tool specific type for HUGE and FAR data pointers - these will usually
   be defined to nothing.  Some 16-bit architectures may require these
   to be defined differently to access data across memory pages */
#define         ESAL_TS_HUGE_PTR_TYPE
#define         ESAL_TS_FAR_PTR_TYPE

/* Define if position-independent code / data (PIC/PID) support (if available)
   is enabled.
   NOTE:  This may be required to be set to NU_TRUE when utilizing any
          OS components requiring position-independent code / data */
#define         ESAL_TS_PIC_PID_SUPPORT                 NU_FALSE

/* External variable declarations */
extern UINT32       _ld_bss_start;
extern UINT32       _ld_bss_end;
extern UINT32       _ld_ram_data_start;
extern UINT32       _ld_ram_data_end;
extern UINT32       _ld_rom_data_start;

/* Macros for memory definitions */
#define TOOLSET_BSS_START_ADDR      (VOID *)&_ld_bss_start
#define TOOLSET_BSS_END_ADDR        (VOID *)&_ld_bss_end
#define TOOLSET_BSS_SIZE            ((UINT32)&_ld_bss_end - (UINT32)&_ld_bss_start)
#define TOOLSET_IBSS_START_ADDR     (VOID *)&_ld_ibss_start
#define TOOLSET_IBSS_SIZE           (UINT32)&_ld_ibss_end
#define TOOLSET_DATA_SRC_ADDR       (VOID *)&_ld_rom_data_start
#define TOOLSET_DATA_DST_ADDR       (VOID *)&_ld_ram_data_start
#define TOOLSET_DATA_SIZE           ((UINT32)&_ld_ram_data_end - (UINT32)&_ld_ram_data_start)

/* This define is used to add quotes to anything passed in */
#define         ESAL_TS_RTE_QUOTES(x)           #x

/* This macro writes the stack pointer. */
#define         ESAL_TS_RTE_SP_WRITE(stack_ptr)                                     \
                {                                                                   \
                    /* Set hardware stack pointer to passed in address */           \
                    asm volatile(" MOV     sp, %0"                                  \
                                 : : "r" (stack_ptr) );                             \
                }

/* This macro reads the stack pointer. */
#define         ESAL_TS_RTE_SP_READ()                                               \
                ({                                                                  \
                    VOID * stk_ptr;                                                 \
                    asm(" MOV   %0,sp": "=r" (stk_ptr));                            \
                    stk_ptr;                                                        \
                })

/* This macro reads the current primask value */
#define         ESAL_TS_RTE_PRIMASK_READ(primask_ptr)                               \
                {                                                                   \
                    asm volatile("    MRS     %0, primask"                          \
                                     : "=r" (*(primask_ptr))                        \
                                     : /* No inputs */ );                           \
                }

/* This macro writes the current primask value */
#define         ESAL_TS_RTE_PRIMASK_WRITE(primask_value)                            \
                {                                                                   \
                    asm volatile("    MSR     primask, %0"                          \
                                     : /* No outputs */                             \
                                     : "r" (primask_value) );                       \
                    asm volatile("    ISB");                                        \
                }

/* This macro reads the current basepri value */
#define         ESAL_TS_RTE_BASEPRI_READ(basepri_ptr)                               \
                {                                                                   \
                    asm volatile("    MRS     %0, basepri"                          \
                                     : "=r" (*(basepri_ptr))                        \
                                     : /* No inputs */ );                           \
                }

/* This macro writes the current basepri value */
#define         ESAL_TS_RTE_BASEPRI_WRITE(basepri_value)                            \
                {                                                                   \
                    asm volatile("    MSR     basepri, %0"                          \
                                     : /* No outputs */                             \
                                     : "r" (basepri_value) );                       \
                    asm volatile("    ISB");                                        \
                }

/* This macro disables all interrupts */
#define         ESAL_TS_RTE_CPSID_EXECUTE()                                         \
                {                                                                   \
                    asm volatile("    CPSID   i");                                  \
                    asm volatile("    ISB");                                        \
                }

/* This macro enables all interrupts */
#define         ESAL_TS_RTE_CPSIE_EXECUTE()                                         \
                {                                                                   \
                    asm volatile("    CPSIE   i");                                  \
                }

/* This macro executes a ISB instruction */
#define         ESAL_TS_RTE_ISB_EXECUTE()                                           \
                {                                                                   \
                    asm volatile("    ISB");                                        \
                }

/* This macro executes a DSB instruction */
#define         ESAL_TS_RTE_DSB_EXECUTE()                                           \
                {                                                                   \
                    asm volatile("    NOP");                                        \
                }

/* This macro executes a NOP instruction */
#define         ESAL_TS_RTE_NOP_EXECUTE()                                           \
                {                                                                   \
                    asm volatile("    NOP");                                        \
                }

/* This macro executes a WFI instruction */
#define         ESAL_TS_RTE_WFI_EXECUTE()                                           \
                {                                                                   \
                    asm volatile("    WFI");                                        \
                }

/* This macro executes a breakpoint instruction
   NOTE:  This instruction is only usable by ARM v5 cores. */
#define         ESAL_TS_RTE_BRK_EXECUTE(brk_point_val)                              \
                {                                                                   \
                    asm volatile("    BKPT    %0"                                   \
                             : /* No Outputs */                                     \
                             : "n" (brk_point_val) );                               \
                }

#if (ESAL_TS_PIC_PID_SUPPORT == NU_TRUE)

/* This macro sets the PIC/PID base address register */
#define         ESAL_TS_RTE_PIC_PID_BASE_SET(pic_base, pid_base)                    \
                {                                                                   \
                    /* Access unused param */                                       \
                    NU_UNUSED_PARAM(pic_base);                                    \
                                                                                    \
                    /* Set the PIC/PID register (r9) */                             \
                    asm volatile(" MOV     r9, %0"                                  \
                                 : : "r" (pid_base) );                              \
                }

/* This macro gets the PIC/PID base address register */
#define         ESAL_TS_RTE_PIC_PID_BASE_GET(pic_base_ptr, pid_base_ptr)            \
                {                                                                   \
                    /* Access unused param */                                       \
                    NU_UNUSED_PARAM(pic_base_ptr);                                \
                                                                                    \
                    /* Read the PIC/PID register (r9) */                            \
                    asm volatile(" MOV   %0,r9": "=r" (*pid_base_ptr));             \
                }

#endif  /* ESAL_TS_PIC_PID_SUPPORT == NU_TRUE */

/* This macro gets the current function's return address, see GCC manual for argument usage */
#define         ESAL_GET_RETURN_ADDRESS(level) __builtin_return_address(level)

/* This macro marks a symbol declaration as weakly linked */
#define         ESAL_TS_WEAK_REF(decl) decl __attribute((weak))

/* This macro marks a symbol definition as weakly linked */
#define         ESAL_TS_WEAK_DEF(decl) __attribute((weak)) decl

/* This macro returns the passed value */
#define         ESAL_TS_NO_RETURN(return_val) return(return_val)

/* This macro generates deprecation warnings */
#define         ESAL_TS_RTE_DEPRECATED __attribute__((deprecated))

/* This macro places a compiler memory barrier to ensure read / write commands
 * cannot be re-ordered around it */
#define         ESAL_TS_RTE_COMPILE_MEM_BARRIER()   asm volatile("" ::: "memory")

#endif  /* CSGNU_ARM_DEFS_H */

