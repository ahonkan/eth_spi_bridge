/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
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
*       dbg.h
*
*   COMPONENT
*
*       Debug Agent
*
*   DESCRIPTION
*
*       This file contains the C external interface for the component.
*
*   DATA STRUCTURES
*
*       DBG_LOGGING_LEVEL
*       DBG_CB
*
*   FUNCTIONS
*
*       nu_os_svcs_dbg_init
*
*   DEPENDENCIES
*
*       dbg_cfg.h
*       dbg_sts.h
*       nucleus.h
*       nu_net.h
*       pcdisk.h
*       runlevel_init.h
*       serial.h
*       dbg_os.h
*       dbg_set.h
*       dbg_str.h
*       dbg_mem.h
*       dbg_eng_api.h
*       dbg_eng_bkpt.h
*       dbg_eng_exec.h
*       dbg_eng_reg.h
*       dbg_eng_mem.h
*       dbg_eng_evt.h
*       dbg_eng.h
*       dbg_com.h
*       dbg_com_serial.h
*       dbg_com_tcp.h
*       dbg_rsp_defs.h
*       dbg_rsp_extr.h
*       dbg_rsp_thd.h
*       dbg_rsp_tmr.h
*       dbg_rsp_utils.h
*       dbg_rsp.h
*       shell_extr.h
*
*************************************************************************/

#ifndef DBG_H
#define DBG_H

/* Configuration component */

#include "services/dbg_cfg.h"

/* Status component */

#include "services/dbg_sts.h"

/* OS component */

#include "nucleus.h"
#include "services/runlevel_init.h"
#include "kernel/nu_kernel.h"

#ifdef CFG_NU_OS_NET_STACK_ENABLE

#include "networking/nu_net.h"

#endif /* CFG_NU_OS_NET_STACK_ENABLE */

#ifdef CFG_NU_OS_DRVR_SERIAL_ENABLE

#include "drivers/serial.h"

#endif /* CFG_NU_OS_DRVR_SERIAL_ENABLE */

#include "services/dbg_os.h"

/* Utility component */

#include "services/dbg_set.h"
#include "services/dbg_str.h"
#include "services/dbg_mem.h"

/* Debug Engine component */

#include "services/dbg_eng_api.h"
#include "services/dbg_eng_bkpt.h"
#include "services/dbg_eng_exec.h"
#include "services/dbg_eng_reg.h"
#include "services/dbg_eng_mem.h"
#include "services/dbg_eng_evt.h"
#include "services/dbg_eng.h"

/* Communications component */

#include "services/dbg_com.h"
#include "services/dbg_com_serial.h"
#include "services/dbg_com_tcp.h"

/* RSP component */

#include "services/dbg_rsp_defs.h"
#include "services/dbg_rsp_extr.h"
#include "services/dbg_rsp_thd.h"
#include "services/dbg_rsp_tmr.h"
#include "services/dbg_rsp_utils.h"
#include "services/dbg_rsp.h"

#ifdef CFG_NU_OS_SVCS_SHELL_ENABLE

/* Shell service API. */

#include "services/shell_extern.h"

#endif /* CFG_NU_OS_SVCS_SHELL_ENABLE */

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* Debug service control block. */

typedef struct _dbg_cb_struct
{
    DBG_COM_CB              com;            /* Communication control block */

} DBG_CB;

/***** Global functions */

STATUS nu_os_svcs_dbg_init(CHAR *path, INT startstop);

#ifdef __cplusplus
}
#endif

#endif /* DBG_H */
