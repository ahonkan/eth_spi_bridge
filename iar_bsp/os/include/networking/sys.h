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
*       sys.h                                                    
*
*   DESCRIPTION
*
*       This file contains the function declarations for those functions
*       used to maintain the system objects.
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

#ifndef SYS_H
#define SYS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

VOID   get_system_contact (UINT8 *);
VOID   get_system_description (UINT8 *);
VOID   get_system_location (UINT8 *);
VOID   get_system_objectid (UINT8 *);
VOID   sc_init (VOID);
INT32  get_system_services (VOID);
UINT32 SysTime (VOID);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
