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
*       setjmp.h
*
*   COMPONENT
*
*       Nucleus POSIX - jump instructions
*
*       NOTE: Standard C RTL header.
*
*   DESCRIPTION
*
*       This file contains toolset specific jump instruction.
*
*   DATA STRUCTURES
*
*       NONE
*
*   DEPENDENCIES
*
*       setjmp.h            POSIX setjmp.h definitions
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_SETJMP_H
#define NU_PSX_SETJMP_H

#if ((defined(__arm)) || (defined(CFG_NU_OS_ARCH_ARMV7_M_ENABLE)))
#include <setjmp.h>
#else

#include "services/config.h"
#include "services/compiler.h"

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SETJMP_H_
#define _SETJMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ONLY Minimal amount of registers saved!! */
#if defined(CFG_TOOLSETS_CSGNU_ARM_ENABLE)
    #define _JMPLEN         18
#else
    #if defined(CFG_TOOLSETS_CSGNU_PPC_ENABLE)
        #define _JMPLEN         40
    #else
        #define _JMPLEN         18
    #endif
#endif

/*  Stack environment declarations
    Compiler specific register buffers +1 signal */
typedef long jmp_buf[_JMPLEN+1];
typedef long sigjmp_buf[_JMPLEN+1];

/* Function mapping */
#define _setjmp(env) sigsetjmp(env,0)
#define setjmp(env) sigsetjmp(env,1)
#define longjmp(env,val) siglongjmp(env,val)
#define _longjmp(env,val) siglongjmp(env,val)

/* Function prototypes */
#if defined(CFG_TOOLSETS_CSGNU_ARM_ENABLE)

int    sigsetjmp(sigjmp_buf env, int savemask) __attribute__ ((naked));
void   siglongjmp(sigjmp_buf env, int val) __attribute__ ((naked));

#else

int    sigsetjmp(sigjmp_buf env, int savemask);
void   siglongjmp(sigjmp_buf env, int val);

#endif /* defined(CFG_TOOLSETS_CSGNU_ARM_ENABLE) */

#ifdef __cplusplus
}
#endif

#endif /* _SETJMP_H_ */

#endif /* ((defined(__arm)) || (defined(CFG_NU_OS_ARCH_ARMV7_M_ENABLE))) */

#endif /* NU_PSX_SETJMP_H */
