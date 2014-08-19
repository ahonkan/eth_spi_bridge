/*************************************************************************/
/*                                                                       */
/*               Copyright 2010 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       safe.h
*
* COMPONENT
*
*       SAFE Disk Driver
*
* DESCRIPTION
*
*       Configuration and interface defines for SAFE device driver
*
* DATA STRUCTURES
*
*       SAFE_DEV                            - SAFE device
*       SAFE_TGT                            - SAFE target info
*       SAFE_INSTANCE_HANDLE                - SAFE instance handle
*       SAFE_SESSION_HANDLE                 - SAFE session handle
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef SAFE_H
#define SAFE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "drivers/power_interface.h"
#include "drivers/safe_dv_interface.h"


STATUS      SAFE_Init(SAFE_INSTANCE_HANDLE *inst_handle, const CHAR *key);
STATUS      Safe_Get_Target_Info(const CHAR * key, SAFE_INSTANCE_HANDLE *inst_info);

typedef struct _safe_dev_struct
{
    BOOLEAN     device_in_use;
    PM_STATE_ID current_state;

} SAFE_DEV;


typedef struct _safe_tgt_struct
{
    FS_DRVMOUNT   mount_func;
    FS_PHYGETID   phy_func;
    long          (*getmem_func)(FS_PHYGETID);

} SAFE_TGT;


/* Public function prototypes */
STATUS      SAFE_Register (const CHAR *key,
                           INT startstop,
                           DV_DEV_ID *dev_id,
                           VOID *tgt_info);
STATUS      SAFE_Unregister (DV_DEV_ID dev_id);


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* SAFE_H */

