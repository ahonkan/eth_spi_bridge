/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       nu_storage.h
*
*   COMPONENT
*
*       Storage - Application include file.
*
*   DESCRIPTION
*
*       This file includes all required header files to allow
*       access to the Nucleus Storage API. This includes all
*       related data structures.
*
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*		nucleus_gen_cfg.h
*		pcdisk.h
*
*************************************************************************/

#ifndef NU_STORAGE_H
#define NU_STORAGE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus_gen_cfg.h"

#include "storage/pcdisk.h"

#ifdef CFG_NU_OS_STOR_DB_SQLITE_ENABLE
#include "os/storage/db/sqlite/src/sqlite3.h"
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* NU_STORAGE_H */
