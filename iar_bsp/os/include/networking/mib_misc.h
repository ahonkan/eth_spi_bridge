/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       mib_misc.h                                               
*
*   DESCRIPTION
*
*       This file contains functions which set the row status or
*       storage type for a particular row.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/

#ifndef MIB_MISC_H
#define MIB_MISC_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 MIB_Set_Status(UINT8 *status, UINT32 new_status, UINT8 *row_flag,
                      UINT8 storage_type);

UINT16 MIB_Set_StorageType(UINT8 *storage_type, UINT32 new_storage_type,
                           UINT8 *row_flag, UINT8 status);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* MIB_MISC_H */


