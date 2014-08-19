/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*        compiler.h
*
*   COMPONENT
*
*
*   DESCRIPTION
*
*       Contains compiler-specific type definitions and standard mapping
*       for different toolsets and architectures.
*
*       NOTE: Math functionality is supplied exclusively by the toolset.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       "config.h"          Nucleus POSIX configuration defines.
*
*************************************************************************/

#ifndef NU_PSX_COMPILER_H
#define NU_PSX_COMPILER_H

/* Main configurations */
#include "services/config.h"

/* IEEE floating point format for POSIX RTL */
#define IEEE

/* Size of pointer is 4 bytes for POSIX RTL */
#define __SIZEOF_PTR                        4

/* Special Mot switch */
#ifndef PSX_MOT
#define PSX_MOT          0
#endif /* MOT */

/* Choose EXTMEM, SARAM or nothing but not both for MOT IPC */
#define PSX_MOT_EXTMEM            0
#define PSX_MOT_SARAM             0

#if PSX_MOT_EXTMEM
#ifndef EXTMEM
#define EXTMEM
#undef  SARAM
#endif /* EXTMEM */
#endif /* PSX_MOT_EXTMEM */

#if PSX_MOT_SARAM
#ifndef SARAM
#define SARAM
#undef  EXTMEM
#endif /* SARAM */
#endif /* PSX_MOT_SARAM */

/*
Toolset Selections:
    PSX_NIOS2,PSX_RHGNU,PSX_CSGNU,PSX_EDGEARM,
    PSX_RV,PSX_TI,PSX_SDE_GNU,PSX_MTEC68K,PSX_MTECCF,
    PSX_MTECPPC,PSX_DIAB,PSX_XTENSA,PSX_HIT,PSX_H8,
    PSX_I960,PSX_I386,PSX_IAPX86,PSX_PPU
*/

#ifndef PSX_CSGNU
#define PSX_CSGNU
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Compiler-specific type definitions for compiling the RTL for different
Toolsets */
/* CS GNU, RVCT and Edge Toolsets*/
#if ( defined(PSX_CSGNU) || defined(PSX_RV) \
      || defined(PSX_EDGEARM) )
#ifndef ARM
#define ARM
#endif

/* Set signed char data type setting. */

#ifdef __FEATURE_SIGNED_CHAR
#define PSX_COMPILER_SIGNED_CHAR        1
#else
#define PSX_COMPILER_SIGNED_CHAR        0
#endif /* __FEATURE_SIGNED_CHAR */

#endif /* ARM architecture */

/* TI toolset */
#if defined(PSX_TI) && defined(__TMS470__)
#ifndef ARM
#define ARM
#endif /* ARM */
/* C55X */
#elif defined(PSX_TI) && !defined(__TMS470__)
#ifndef C55X
#define C55X
#endif /* C55X */
#endif /* ARM or C55X architecture */

#if PSX_MOT
#ifndef MOT
#define MOT
#endif /* MOT */
#ifdef C55X
#ifndef DSP
#define DSP
#endif /* DSP */
#endif /* C55X */
#endif /* PSX_MOT */

/* NIOS2 Toolset */
#ifdef PSX_NIOS2
#ifndef NIOS_2
#define NIOS_2
#endif
#endif /* NIOS2 architecture */

/* RH GNU Toolset */
#ifdef PSX_RHGNU
#ifndef INTEL_XSCALE
#define INTEL_XSCALE
#endif
#endif /* XSCALE architecture */

/* SDE GNU Toolset */
#ifdef PSX_SDE_GNU
#ifndef MIPS
#define MIPS
#endif
#endif /* MIPS architecture */

/* Mtec 68k Toolset */
#ifdef PSX_MTEC68K
#ifndef M68K
#define M68K
#endif
#endif /* M68k architecture */

/* Mtec Coldfire Toolset */
#ifdef PSX_MTECCF
#ifndef MCF
#define MCF
#endif
#endif /* Cold Fire architecture */

/* Mtec PPC and IBM Cell Toolset */
#if defined(PSX_MTECPPC) || defined(PSX_PPU)
#ifndef PPC
#define PPC
#endif
#endif /* PowerPc architecture */

/* Diab Toolset */
#ifdef PSX_DIAB
#if defined(__m68k)
#ifndef M68K
#define M68K
#endif
#elif defined(__ppc)
#ifndef PPC
#define PPC
#endif
#elif defined(__mips)
/*currently not supported*/
#ifndef MIPS
#define MIPS
#endif
#elif defined(sh) || defined(__sh)
/*currently not supported*/
#ifndef SH_HIT
#define SH_HIT
#endif
#elif defined(__arm)
/*currently not supported*/
#ifndef ARM
#define ARM
#endif /* ARM */
#elif defined(m88k) || defined(__m88k)
/*currently not supported*/
#elif defined(__rce)
/*currently not supported*/
#elif defined(__nec)
/*currently not supported*/
#elif defined(__m32r)
/*currently not supported*/
#elif defined(sparc) || defined(__sparc)
/*currently not supported*/
#elif defined(__386)
/*currently not supported*/
#elif defined(__sc)
/*currently not supported*/
#endif 
/* __m68k __ppc  __mips __sh __arm __m88k __rce 
__nec  __m32r __sparc __386 __sc architectures */
#endif /* PSX_DIAB */

/* Xtensa Toolset */
#ifdef PSX_XTENSA
#ifndef XTENSA
#define XTENSA
#endif
#endif /* XTENSA architecture */

/* Hitachi Toolset */
#ifdef PSX_HIT
#ifndef SH_HIT
#define SH_HIT
#endif
#endif /* Hitachi architecture */

/* Hitachi H8 Toolset */
#ifdef PSX_H8
#ifndef H83
#define H83
#endif
#endif /* H8 architecture */

/* Intel Toolset */
#ifdef PSX_I386
#ifndef I386
#define I386
#endif
#endif /* I386 architecture */

/* Intel Toolset */
#ifdef PSX_IAPX86
#ifndef IAPX86
#define IAPX86
#endif
#endif /* IAPX86 architecture */

/* Intel Toolset */
#ifdef PSX_I960
#ifndef I960
#define I960
#endif
#endif /* I960 architecture */

#ifndef NCPU
# ifndef MCF
#  ifndef INTEL_XSCALE
#   ifndef NIOS_2
#    ifndef M_I86
#      ifndef MIPS 
#       ifndef M68K
#        ifndef H85
#         ifndef H83
#          ifndef SH_HIT
#           ifndef SUN3
#            ifndef SUN4
#             ifndef HP700
#              ifndef INTELx86
#               ifndef SPARC
#                ifndef I386
#                 ifndef AMD29K
#                  ifndef PPC
#                   ifndef I960
#                    ifndef ARM
#                     ifndef C54X
#                      ifndef C55X
#                       ifndef XTENSA
#                        define M68K  /* default to M68K */
#                       endif
#                      endif
#                     endif
#                    endif
#                   endif
#                  endif
#                 endif
#                endif
#               endif
#              endif
#             endif
#            endif
#           endif
#          endif
#         endif
#        endif
#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
#endif

/* Ensure a value for the signed char setting.  Default to "unsigned". */

#ifndef PSX_COMPILER_SIGNED_CHAR
#define PSX_COMPILER_SIGNED_CHAR    0
#endif

#ifdef __cplusplus
}
#endif

#endif /* NU_PSX_COMPILER_H */
