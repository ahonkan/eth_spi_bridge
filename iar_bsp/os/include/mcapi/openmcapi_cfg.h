/*
 * Copyright (c) 2010, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the <ORGANIZATION> nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/* XXX Many of these should be internal to libmcapi, not exposed to
 * applications. */

#ifndef OPENMCAPI_CFG_H
#define OPENMCAPI_CFG_H

#include <mcapi_os.h>

#define MCAPI_VERSION           0   /* Initial Beta version. */

/* Do not redefine these macros if using the Nucleus builder to compile the
 * code.  If not using the Nucleus builder, all macros must be explicitly
 * configured here; otherwise, configure the respective .metadata file.
 */
#ifndef CFG_NU_OS_KERN_IPC_MCAPI_CFG_H

/* Disable this macro to disable the loopback interface within the MCAPI
 * generic code. */
#define MCAPI_ENABLE_LOOPBACK       MCAPI_FALSE

/* Disable this macro to disable forwarding capabilities for this node. */
#define MCAPI_ENABLE_FORWARDING     MCAPI_FALSE

/* The maximum number of endpoints that can be simultaneously open on the
 * node. */
#define MCAPI_MAX_ENDPOINTS         32

/* The total number of nodes in the system, including this node. */
#define MCAPI_NODE_COUNT            2

/* The total number of priorities in the system.  This value should be set
 * according to the interface in the system that supports the highest
 * number of priorities.  */
#define MCAPI_PRIO_COUNT            2

/* The initial value of the global port counter to use when an endpoint
 * is created with no specific port. */
#define MCAPI_ENDP_PORT_INIT        10000

/* The total number of buffers in the system that can be used to send
 * and receive data on the node. */
#define MCAPI_BUF_COUNT             128

/* The maximum size of data that can be sent from an endpoint.  This
 * is also the length of each buffer in the system. */
#define MCAPI_MAX_DATA_LEN          1024

/* The port used to receive control messages from other nodes in the
 * sytem. */
#define MCAPI_RX_CONTROL_PORT       900

/* The max length to accept for control messages. */
#define MCAPI_CONTROL_MSG_LEN       128

/* Control task settings. */
#define MCAPI_CONTROL_TASK_SIZE     2000
#define MCAPI_CONTROL_TASK_PRIO     3

/* The default priority of all new endpoints in the system. */
#define MCAPI_DEFAULT_PRIO          1

/* The total number of routes on this node, including a route for the
 * loopback interface. */
#define MCAPI_ROUTE_COUNT           2

/* The total number of interfaces on this node, including the
 * loopback interface if loopback is enabled. */
#define MCAPI_INTERFACE_COUNT       1

/* The number of available request structures that can service a pending
 * get endpoint request from a remote node. */
#define MCAPI_FREE_REQUEST_COUNT    16

/* The max length of an interface name. */
#define MCAPI_INT_NAME_LEN          8

/* The name of the loopback interface. */
#define MCAPI_LOOPBACK_NAME         "loop"

#else

/* Using Nucleus builder, so get the values from the configuration generated
 * from the .metadata file. */

#define MCAPI_ENABLE_LOOPBACK       CFG_NU_OS_KERN_IPC_MCAPI_ENABLE_LOOPBACK
#define MCAPI_ENABLE_SHM       	    CFG_NU_OS_KERN_IPC_MCAPI_ENABLE_SHM
#define MCAPI_ENABLE_FORWARDING     CFG_NU_OS_KERN_IPC_MCAPI_ENABLE_FORWARDING
#define MCAPI_MAX_ENDPOINTS         CFG_NU_OS_KERN_IPC_MCAPI_MAX_ENDPOINTS
#define MCAPI_NODE_COUNT            CFG_NU_OS_KERN_IPC_MCAPI_NODE_COUNT
#define MCAPI_PRIO_COUNT            CFG_NU_OS_KERN_IPC_MCAPI_PRIO_COUNT
#define MCAPI_ENDP_PORT_INIT        CFG_NU_OS_KERN_IPC_MCAPI_ENDP_PORT_INIT
#define MCAPI_BUF_COUNT             CFG_NU_OS_KERN_IPC_MCAPI_BUF_COUNT
#define MCAPI_MAX_DATA_LEN          CFG_NU_OS_KERN_IPC_MCAPI_MAX_DATA_LEN
#define MCAPI_RX_CONTROL_PORT       CFG_NU_OS_KERN_IPC_MCAPI_RX_CONTROL_PORT
#define MCAPI_CONTROL_MSG_LEN       CFG_NU_OS_KERN_IPC_MCAPI_CONTROL_MSG_LEN
#define MCAPI_CONTROL_TASK_SIZE     CFG_NU_OS_KERN_IPC_MCAPI_CONTROL_TASK_SIZE
#define MCAPI_CONTROL_TASK_PRIO     CFG_NU_OS_KERN_IPC_MCAPI_CONTROL_TASK_PRIO
#define MCAPI_DEFAULT_PRIO          CFG_NU_OS_KERN_IPC_MCAPI_DEFAULT_PRIO
#define MCAPI_ROUTE_COUNT           CFG_NU_OS_KERN_IPC_MCAPI_ROUTE_COUNT
#define MCAPI_INTERFACE_COUNT       CFG_NU_OS_KERN_IPC_MCAPI_INTERFACE_COUNT
#define MCAPI_FREE_REQUEST_COUNT    CFG_NU_OS_KERN_IPC_MCAPI_FREE_REQUEST_COUNT
#define MCAPI_INT_NAME_LEN          CFG_NU_OS_KERN_IPC_MCAPI_INT_NAME_LEN
#define MCAPI_LOOPBACK_NAME         CFG_NU_OS_KERN_IPC_MCAPI_LOOPBACK_NAME

#endif  /* #ifndef CFG_NU_OS_KERN_IPC_MCAPI_CFG_H */

#endif  /* #ifndef MCAPI_CFG_H */
