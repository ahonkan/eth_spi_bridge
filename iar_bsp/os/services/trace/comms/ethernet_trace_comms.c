/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       ethernet_trace_comms.c
*
*   COMPONENT
*
*       Trace Communications over Ethernet
*
*   DESCRIPTION
*
*       This file implements the APIs required for trace communication
*       over a Ethernet using TCP.
*
*************************************************************************/

#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/services/trace/comms/trace_comms.h"

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == ETHERNET_INTERFACE)

#include        "drivers/nu_drivers.h"
#include        "networking/nu_networking.h"

/* Definitions */
#define         TRACE_COMMS_UP_TSK_PRIORITY     2
#define         TRACE_COMMS_UP_TSK_STK_SZ       (NU_MIN_STACK_SIZE * 8)
#define         TRACE_COMMS_PORT_NUM            2160

/* Data structures */
typedef struct _tcp_port_struct_
{
    INT                     cli_sock;   /* Nucleus NET Client socket */
    struct addr_struct      cli_addr;   /* Nucleus NET Client address */
    INT                     serv_sock;  /* Nucleus NET Server socket */
    struct addr_struct      serv_addr;  /* Nucleus NET Server address */
    UINT                    port_num;   /* Port number */
    BOOLEAN                 initialized;

} TRACE_COMMS_TCP_PORT;

/* Globals */
TRACE_COMMS_TCP_PORT    Trace_Comms_TCP_Port;
static NU_TASK          *ETH__Comms_Up_Task;

/***********************************************************************
*
*   FUNCTION
*
*       TCP_Setup_Server
*
*   DESCRIPTION
*
*       This function sets up a TCP server for trace communications
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_SUCCESS or an error code
*
***********************************************************************/
static STATUS TCP_Setup_Server(VOID)
{
    STATUS  status = NU_SUCCESS;
    INT     ret;

    /* Open socket */
    Trace_Comms_TCP_Port.serv_sock = NU_Socket(NU_FAMILY_IP, NU_TYPE_STREAM, NU_NONE);

    if (Trace_Comms_TCP_Port.serv_sock < 0)
    {
        status = NU_TRACE_COMMS_OPEN_ERROR;
    }

    /* Disable the Naigle algorithm. */
    if (status == NU_SUCCESS)
    {
        status = NU_Push(Trace_Comms_TCP_Port.serv_sock);

        if (status == NU_SUCCESS)
        {
            /* Fill in a server address data structure and bind it with the socket. */
            Trace_Comms_TCP_Port.serv_addr.family = NU_FAMILY_IP;
            Trace_Comms_TCP_Port.serv_addr.port = Trace_Comms_TCP_Port.port_num;
            *(UINT32*)&Trace_Comms_TCP_Port.serv_addr.id.is_ip_addrs[0] = IP_ADDR_ANY;
            Trace_Comms_TCP_Port.serv_addr.name = "TRACE_COM";

            ret = NU_Bind(Trace_Comms_TCP_Port.serv_sock, &Trace_Comms_TCP_Port.serv_addr, 0);

            if (ret < 0)
            {
                status = NU_TRACE_COMMS_OPEN_ERROR;
            }
        }
    }

    /* Listen for a client connection */
    if (status == NU_SUCCESS)
    {
        status = NU_Listen(Trace_Comms_TCP_Port.serv_sock, 1);
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       TCP_Wait_For_Client
*
*   DESCRIPTION
*
*       This function will wait for client to connect to
*       server and block till successful connection from client
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_SUCCESS or an error code
*
***********************************************************************/
static STATUS   TCP_Wait_For_Client(VOID)
{
    STATUS  status = NU_SUCCESS;

    /* Block in accept until a client attempts connection. */
    Trace_Comms_TCP_Port.cli_sock = NU_Accept(Trace_Comms_TCP_Port.serv_sock,
                                              &Trace_Comms_TCP_Port.cli_addr, 0);

    if (Trace_Comms_TCP_Port.cli_sock < 0)
    {
        status = NU_TRACE_COMMS_OPEN_ERROR;
    }

    /* Turn on blocking on reads for the new client connection. */
    if (status == NU_SUCCESS)
    {
        status = NU_Fcntl(Trace_Comms_TCP_Port.cli_sock, NU_SETFLAG, NU_BLOCK);
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Up_Task_Entry
*
*   DESCRIPTION
*
*       This is the task entry function for trace communications
*       up task.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static  VOID    Trace_Comms_Up_Task_Entry(UNSIGNED argc, VOID * argv)
{
	STATUS      status;

    /* Wait for net stack to be up */
    status = NETBOOT_Wait_For_Network_Up(NU_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Initialize trace port */
        Trace_Comms_TCP_Port.port_num = TRACE_COMMS_PORT_NUM;

        /* Setup server. */
        status = TCP_Setup_Server();

        /* Ensure a client is connected. */
        if (status == NU_SUCCESS)
        {
            /* Setup client */
            status = TCP_Wait_For_Client();

            /* Ethernet interface for trace comms is initialized */
            if(status == NU_SUCCESS)
            {
                /* Set client initialize complete flag */
                Trace_Comms_TCP_Port.initialized = NU_TRUE;
            }
        }
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       Ethernet_Trace_Comms_Open
*
*   DESCRIPTION
*
*       Open TCP socket based communications to enable trace data
*       communications to host
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_SUCCESS or an error code
*
***********************************************************************/
STATUS Ethernet_Trace_Comms_Open(VOID)
{
    NU_MEMORY_POOL*     sys_pool_ptr;
    STATUS              status;


    /* Get system memory pool */
    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Create a auto-clean task to block until the networking interface
         * is up */
        status = NU_Create_Auto_Clean_Task(&ETH__Comms_Up_Task, "COMMS_UP",
                                           Trace_Comms_Up_Task_Entry, 0, NU_NULL, sys_pool_ptr,
                                           TRACE_COMMS_UP_TSK_STK_SZ, TRACE_COMMS_UP_TSK_PRIORITY, 0,
                                           NU_PREEMPT, NU_START);
    }

    if(status != NU_SUCCESS)
    {
        status = NU_TRACE_COMMS_OPEN_ERROR;
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Ethernet_Trace_Comms_Close
*
*   DESCRIPTION
*
*       CLose Ethernet TCP communication to host
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_SUCCESS or an error code
*
***********************************************************************/
STATUS   Ethernet_Trace_Comms_Close(VOID)
{
    STATUS status;


    /* Close any client socket */
    (VOID)NU_Close_Socket(Trace_Comms_TCP_Port.cli_sock);

    /* Close the server socket. */
    (VOID)NU_Close_Socket(Trace_Comms_TCP_Port.serv_sock);

    /* Terminate Ethernet comms setup task */
    status = NU_Terminate_Task(ETH__Comms_Up_Task);

    /* Delete task
     * NOTE: Memory cleanup is done by kernel */
    if (status == NU_SUCCESS)
    {
        status = NU_Delete_Task(ETH__Comms_Up_Task);
    }

    /* Clear initialization flag */
    Trace_Comms_TCP_Port.initialized = NU_FALSE;

    /* Clear structure */
    memset(&Trace_Comms_TCP_Port, 0, sizeof(TRACE_COMMS_TCP_PORT));

    /* Invalidate socked values */
    Trace_Comms_TCP_Port.cli_sock = -1;
    Trace_Comms_TCP_Port.serv_sock = -1;

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Ethernet_Trace_Comms_Is_Ready
*
*   DESCRIPTION
*
*       Is Ethernet interface ready for trace communications ?
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       BOOLEAN        NU_TRUE or NU_FALSE
*
***********************************************************************/
BOOLEAN   Ethernet_Trace_Comms_Is_Ready(VOID)
{
    return (Trace_Comms_TCP_Port.initialized);
}

/***********************************************************************
*
*   FUNCTION
*
*       Ethernet_Trace_Comms_Transmit
*
*   DESCRIPTION
*
*       This function transmits requested size of trace data to host
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        Transmission return status
*
***********************************************************************/
STATUS Ethernet_Trace_Comms_Transmit(CHAR* buff, UINT16 size)
{
    INT32   bytes_sent;
    STATUS  status = NU_SUCCESS;

    if(Trace_Comms_TCP_Port.initialized == NU_TRUE)
    {
		bytes_sent = NU_Send(Trace_Comms_TCP_Port.cli_sock, buff, size, 0);

		if ((bytes_sent < 0) || (bytes_sent != size))
		{
			status = NU_TRACE_COMMS_TX_ERROR;
		}
    }
    else
    {
    	status = NU_TRACE_COMMS_OPEN_ERROR;
    }

    return (status);
}

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == ETHERNET_INTERFACE */
