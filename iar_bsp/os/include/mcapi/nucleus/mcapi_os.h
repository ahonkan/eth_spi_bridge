/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/****************************************************************************
*
* FILENAME
*
*       mcapi_os.h
*
* DESCRIPTION
*
*       This file will hold all of those defines and setups used by the
*       operating system.  This file is unique to each specific operating
*       system and must be ported accordingly.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
****************************************************************************/
#ifndef MCAPI_OS_NUCLEUS_H
#define MCAPI_OS_NUCLEUS_H

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/********************* Porting Section **************************/

typedef UINT16              mcapi_node_t;
typedef INT32               mcapi_port_t;
typedef UINT32              mcapi_endpoint_t;
typedef UINT32              mcapi_pktchan_recv_hndl_t;
typedef UINT32              mcapi_pktchan_send_hndl_t;
typedef UINT32              mcapi_sclchan_recv_hndl_t;
typedef UINT32              mcapi_sclchan_send_hndl_t;
typedef unsigned long long  mcapi_uint64_t;
typedef UINT32              mcapi_uint32_t;
typedef UINT16              mcapi_uint16_t;
typedef UINT8               mcapi_uint8_t;
typedef int                 mcapi_int_t;
typedef int                 mcapi_uint_t;
typedef STATUS              mcapi_status_t;
typedef int                 mcapi_timeout_t;
typedef UINT8               mcapi_boolean_t;
typedef int                 mcapi_version_t;
typedef UINT32              mcapi_priority_t;
typedef UNSIGNED            mcapi_unsigned_t;
typedef NU_EVENT_GROUP      mcapi_cond_t;

/* This type must match across all reachable nodes in the system. 
 * This type is used within the MCAPI_BUFFER data structure to represent
 * data pointers in that structure.  Since the MCAPI_BUFFER structure
 * is shared across reachable nodes, this type must be consistent.
 */
typedef mcapi_uint32_t      MCAPI_POINTER;
typedef NU_TASK             MCAPI_THREAD;

#define MCAPI_TRUE                  NU_TRUE
#define MCAPI_FALSE                 NU_FALSE
#define MCAPI_NULL                  NU_NULL
#define MCAPI_MUTEX                 NU_SEMAPHORE

#define MCAPI_THREAD_ENTRY(f)       void f(UNSIGNED argc, void *argv)
#define MCAPI_THREAD_PTR_ENTRY(f)   void (*f)(UNSIGNED argc, void *argv)

/********************* End Porting Section **********************/

void mcapi_process_input(mcapi_unsigned_t argc, void *argv);
mcapi_status_t mcapi_retrieve_rx_event(void);
                                     
#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* MCAPI_OS_NUCLEUS_H */
