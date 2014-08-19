/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME               
*
*       inv6_extr.h                
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       This file contains function prototypes of all functions
*       accessible to other components.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef _INV6_EXTR_H
#define _INV6_EXTR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Internal function */
STATUS  POSIX_SYS_PROC_Init_Netv6(pid_t pid, struct id_struct *um_ip6_addr, 
                               struct id_struct *um_ip6_loopback_addr);
                               
#ifdef __cplusplus
}
#endif

#endif /* _INV6_EXTR_H */
