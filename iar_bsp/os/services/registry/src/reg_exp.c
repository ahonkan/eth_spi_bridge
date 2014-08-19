/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       reg_exp.c
*
*   COMPONENT
*
*       Registry
*
*   DESCRIPTION
*
*       Export symbols for the Registry component.
*
*   DEPENDENCIES
*
*       nucleus.h
*       proc_extern.h
*       reg_api.h
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_SVCS_REGISTRY_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/proc_extern.h"
#include "services/reg_api.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_SVCS_REGISTRY);

// TODO: the list of exports is incomplete, yet.
NU_EXPORT_SYMBOL (REG_Get_UINT32_Value);
NU_EXPORT_SYMBOL (REG_Get_UINT16_Value);
NU_EXPORT_SYMBOL (REG_Get_UINT8_Value);

#endif /* CFG_NU_OS_SVCS_REGISTRY_EXPORT_SYMBOLS == NU_TRUE */
