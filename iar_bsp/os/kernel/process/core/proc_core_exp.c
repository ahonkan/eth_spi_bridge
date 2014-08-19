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
*       proc_core_exp.c
*
*   COMPONENT
*
*       Process Core
*
*   DESCRIPTION
*
*       Export symbols for Process Core component.
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

#if (CFG_NU_OS_KERN_PROCESS_CORE_EXPORT_SYMBOLS == NU_TRUE)

#include "kernel/proc_extern.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_KERN_PROCESS_CORE);

/* Export core process APIs. */
NU_EXPORT_SYMBOL (NU_Load);
NU_EXPORT_SYMBOL (NU_Start);
NU_EXPORT_SYMBOL (NU_Stop);
NU_EXPORT_SYMBOL (NU_Unload);
NU_EXPORT_SYMBOL (NU_Kill);
NU_EXPORT_SYMBOL (NU_Symbol);
NU_EXPORT_SYMBOL (NU_Symbol_Close);

/* Export memory management APIs. */
#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
NU_EXPORT_SYMBOL (NU_Memory_Map);
NU_EXPORT_SYMBOL (NU_Memory_Unmap);
NU_EXPORT_SYMBOL (NU_Memory_Get_ID);
NU_EXPORT_SYMBOL (NU_Memory_Share);
NU_EXPORT_SYMBOL (NU_Memory_Map_Information);
#endif

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_EXPORT_SYMBOLS == NU_TRUE */
