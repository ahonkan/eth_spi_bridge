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
*       dvs_exp.c
*
*   COMPONENT
*
*       DV - Device Manager
*
*   DESCRIPTION
*
*       Export symbols for the Device Manager component.
*
*   DEPENDENCIES
*
*       nucleus.h
*       proc_extern.h
*       dev_mgr.h
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/proc_extern.h"
#include "kernel/dev_mgr.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_KERN_DEVMGR);

// TODO: the list of exports is incomplete, yet.
NU_EXPORT_SYMBOL (DVC_Dev_Close);
NU_EXPORT_SYMBOL (DVC_Dev_ID_Open);
NU_EXPORT_SYMBOL (DVC_Dev_ID_Get);
NU_EXPORT_SYMBOL (DVC_Dev_Ioctl);

#endif /* CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS == NU_TRUE */
