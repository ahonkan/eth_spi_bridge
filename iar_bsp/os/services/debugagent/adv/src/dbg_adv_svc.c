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
*       dbg_adv_svc.c
*
*   COMPONENT
*
*       Debug Agent Advertising service
*
*   DESCRIPTION
*
*       This file contains the C main functions source code for the
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nu_os_svcs_dbg_adv_init
*       DBG_ADV_Task
*       DBG_ADV_Send_To_Queue
*
*   DEPENDENCIES
*
*       Net
*       Serial IO
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/serial.h"
#include "networking/nu_net.h"
#include "services/nu_services.h"

#define DBG_ADV_ADDR_SIZE       4  /* IPv4 is numbers */
#define DBG_ADV_ADDR_BUFFERED   5  /* Queue size */

#define DBG_ADV_TASK_STACKSIZE  4096
#define DBG_ADV_TASK_PRIORITY   0   /* Arbitrarily high priority as this task is short-lived. */

NU_TASK     DBG_ADV_Task_TCB;
NU_QUEUE    DBG_ADV_Queue;
VOID        *DBG_ADV_Task_Stack;
UINT8       DBG_ADV_Initialized = 0;

/* Buffer to be used for the queue */
UNSIGNED DGB_ADV_Buffer[DBG_ADV_ADDR_SIZE*DBG_ADV_ADDR_BUFFERED];
CHAR    DBG_ADV_Regpath[REG_MAX_KEY_LENGTH];

extern NU_MEMORY_POOL           System_Memory;


VOID   DBG_ADV_Task(UNSIGNED argc, VOID *argv);

/*************************************************************************
*
*   FUNCTION
*
*       nu_os_svcs_dbg_adv_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the Debug Agent Advertising service.
*
*   INPUTS
*
*       path - Path of the Nucleus OS registry entry for the Nucleus
*              Agent advertising service.
*
*       startstop - Value that indicates if Nucleus Agent system should be
*                   started (1) or stopped (0).
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       NU_INVALID_OPERATION - Indicates operation failed.
*
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
STATUS nu_os_svcs_dbg_adv_init(CHAR *path, INT startstop)
{
    STATUS  status;

    /* Save a copy of our registry path so we can shutdown from the
       debug agent */
    strcpy( DBG_ADV_Regpath, path);

    switch(startstop)
    {
        /* Stop */
        case 0:
        {
            if (DBG_ADV_Initialized == 1)
            {
                /* Turn it off */
                DBG_ADV_Initialized = 0;
                
            }
            else
            {
                /* Service is not running */
                status = NU_SUCCESS;
            }

            break;
        }

        /* Start the service */
        case 1:
        {
            if (DBG_ADV_Initialized == 0)
            {                
                /* Create the queue so we can receive messages */
                status = NU_Create_Queue(&DBG_ADV_Queue, "DBGADVQ", &DGB_ADV_Buffer[0], sizeof(DGB_ADV_Buffer), NU_FIXED_SIZE, DBG_ADV_ADDR_SIZE, NU_FIFO);
                if (status == NU_SUCCESS)
                {
                    status = NU_Allocate_Memory(&System_Memory, &DBG_ADV_Task_Stack, DBG_ADV_TASK_STACKSIZE, NU_NO_SUSPEND);
                    if (status == NU_SUCCESS)
                    {
                        status = NU_Create_Task(&DBG_ADV_Task_TCB, "DBD_ADV", DBG_ADV_Task, 0, NU_NULL, DBG_ADV_Task_Stack, DBG_ADV_TASK_STACKSIZE, DBG_ADV_TASK_PRIORITY, 0, NU_PREEMPT, NU_START);
                        if (status == NU_SUCCESS)
                        {
                            DBG_ADV_Initialized = 1;
                        }
                        else
                        {
                            /* Return our resources */
                            (VOID) NU_Deallocate_Memory(DBG_ADV_Task_Stack);
                            (VOID) NU_Delete_Queue(&DBG_ADV_Queue);
                        }
                    }
                    else
                    {
                        /* Remove previously created queue */
                        (VOID) NU_Delete_Queue(&DBG_ADV_Queue);
                    }
                }
            }
            else
            {
                /* Service is running */
                status = NU_SUCCESS;
            }
            break;
        }

        default :
        {
            status = NU_INVALID_OPERATION;
            break;
        }
    }


    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ADV_Task
*
*   DESCRIPTION
*
*       Entry point for the service.
*
*   INPUTS
*
*       argc
*       argv
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID   DBG_ADV_Task(UNSIGNED argc, VOID *argv)
{
    STATUS status = NU_SUCCESS;
    UNSIGNED message[DBG_ADV_ADDR_SIZE];
    CHAR buf[20];
    UINT8 ipaddr[DBG_ADV_ADDR_SIZE];
    UNSIGNED msg_size = 0;
    UNSIGNED i;
    NU_SERIAL_PORT *port = NU_NULL;

    /* We need to wait around for a serial port to show up. */
    do
    {
        NU_Sleep(10);
        port = NU_SIO_Get_Port();
    } while (port == NU_NULL);

    if (status == NU_SUCCESS)
    {
        while (1)
        {
            status = NU_Receive_From_Queue(&DBG_ADV_Queue, &message[0], DBG_ADV_ADDR_SIZE, &msg_size, NU_SUSPEND);
            if ( (status == NU_SUCCESS) && (msg_size == DBG_ADV_ADDR_SIZE))
            {
                /* Convert the message to ipaddr */
                for (i=0; i < msg_size; i++)
                {
                    ipaddr[i] = message[i];
                }

                /* We have an address, convert it to a string
                   and output it */
                status = NU_Inet_NTOP(NU_FAMILY_IP, ipaddr, &buf[0], 16);
                if (status == NU_SUCCESS)
                {
                    NU_SIO_Puts(buf);
                    NU_SIO_Puts("\n\r");
                }
            }
            else
            {
                /* We received an unhandled return from the queue. Stop advertising. */
                NU_SIO_Puts("Invalid message received for IP advertising service. Stopping service.\n\r");
                break;
            }
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ADV_Send_To_Queue
*
*   DESCRIPTION
*
*       External interface for sending out IP addresses. NOTE: Only IPv4
*       addresses are supported
*
*   INPUTS
*
*       ipaddr
*       addr_len
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POINTER
*
*************************************************************************/
STATUS DBG_ADV_Send_To_Queue(UINT8 *ipaddr, UINT8 addr_len)
{
    UNSIGNED message[DBG_ADV_ADDR_SIZE];
    UINT8 i;
    STATUS status = NU_NOT_PRESENT;

    /* Verify the service is still up, we don't want to output stuff
       once the debug agent has started */
    if (DBG_ADV_Initialized == 1)
    {
        /* Only supports v4 addresses */
        if (ipaddr && (addr_len == 4) && (addr_len <= DBG_ADV_ADDR_SIZE))
        {
            /* Convert the IP address to a message */
            for (i=0; i < addr_len; i++)
            {
                message[i] = (UNSIGNED) *ipaddr;
                ipaddr++;
            }
    
            /* Put an IP address in the queue, the message will get picked up
               once the advertising service is started and can output messages */
            status = NU_Send_To_Queue(&DBG_ADV_Queue, &message[0], DBG_ADV_ADDR_SIZE, NU_NO_SUSPEND);
        }
    }
    
    return status;
}
