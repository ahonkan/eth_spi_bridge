/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
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
*       cxx_exp.c
*
*   COMPONENT
*
*       Run-Time Library (RTL)
*
*   DESCRIPTION
*
*       Export symbols for C++ language support component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/proc_extern.h"

/* Extern of C++ static object destruction support */
extern void __cxa_atexit(void (*)(void*), void*, void*);

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_SVCS_CXX);

/* C++ static object support */
NU_EXPORT_SYMBOL (__cxa_atexit);

#endif /* CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS == NU_TRUE */
