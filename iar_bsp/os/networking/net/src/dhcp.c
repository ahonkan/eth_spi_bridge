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
*   FILENAME
*
*       dhcp.c
*
*   DESCRIPTION
*
*       This file will contain all the DHCP routines.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Dhcp
*       NU_Dhcp_Release
*       DHCP_Release_Address
*       DHCP_Build_Message
*       DHCP_Event_Handler
*       DHCP_Initialize
*       DHCP_Init_IP_Layer
*       DHCP_Init_UDP_Layer
*       DHCP_Init_DHCP_Layer
*       DHCP_Message_Type
*       DHCP_Pkt_Copy
*       DHCP_Process_ACK
*       DHCP_Process_Offer
*       DHCP_Queue_Event
*       DHCP_Send
*       DHCP_Send_Request
*       DHCP_Vendor_Options
*       DHCP_Validate
*       DHCP_Init_Timers
*       DHCP_Update_Timers
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       net_cfg.h
*       nu_net.h
*
**************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/net_cfg.h"
#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)


#if (INCLUDE_DHCP == NU_TRUE)

/* Define prototypes for function references. */
STATIC STATUS DHCP_Build_Message(NET_BUFFER **, const DHCP_STRUCT *,
                                 const DV_DEVICE_ENTRY *, const DEV_IF_ADDR_ENTRY *,
                                 UINT8);
STATIC STATUS DHCP_Pkt_Copy(UINT8 *, NET_BUFFER *, INT, const DV_DEVICE_ENTRY *);
STATIC STATUS DHCP_Send_Request(DHCP_STRUCT *, INT, const CHAR *);
STATIC VOID DHCP_Init_UDP_Layer(NET_BUFFER *, UINT32, UINT32);
STATIC VOID DHCP_Init_IP_Layer(NET_BUFFER *, UINT32, UINT32);
STATIC STATUS DHCP_Init_DHCP_Layer(NET_BUFFER *, const DHCP_STRUCT *,
                                   const DV_DEVICE_ENTRY *, const DEV_IF_ADDR_ENTRY *,
                                   UINT8);
STATIC STATUS DHCP_Send(const CHAR *, const DHCP_STRUCT *, UINT8);
STATIC STATUS DHCP_Vendor_Options(const CHAR *, UINT8, UINT16, UINT8 HUGE *,
                                  DHCP_STRUCT *);
STATIC STATUS DHCP_Process_ACK(INT, DHCP_STRUCT *, const CHAR *, UINT32);
STATIC STATUS DHCP_Process_Offer(INT, DHCP_STRUCT *, const CHAR *, UINT32);
STATIC UINT8 DHCP_Message_Type(const UINT8 *);
STATIC VOID DHCP_Event_Handler(UNSIGNED, VOID *);
STATIC VOID DHCP_Init_Timers(DV_DEVICE_ENTRY *);
STATIC STATUS DHCP_Update_Timers(const CHAR *, TQ_EVENT, UINT32);

static  NU_TASK         *DHCP_Event_Handler_Tcb;
static  NU_QUEUE        *DHCP_Event_Queue;

/* These two globals will be initialized with the well known port numbers for a
   DHCP client and a DHCP server. */
static  UINT16          DHCP_Server_Port;
static  UINT16          DHCP_Client_Port;

/* DHCP Events */
static  TQ_EVENT        DHCP_Renew;
static  TQ_EVENT        DHCP_Rebind;
static  TQ_EVENT        DHCP_New_Lease;
static  TQ_EVENT        DHCP_Release;

/* Timer backoff array, used for exponential backoff. */
INT     DHCP_Backoff[DHCP_MAX_BACKOFFS + 1];

extern UINT32   NET_Initialized_Modules;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for  DHCP options */
CHAR       DHCP_Option_Memory[NET_MAX_DHCP_OPTS_LEN * NET_MAX_DHCP_DEVICES];

/* Counter to count the number of devices */
INT        No_of_Devices = 0;

/* Declare memory for the DHCP initialization function */
CHAR       DHCP_Initialize_Memory[NET_DHCP_INIT_MEMORY];
#endif

/******************************************************************************
*
*   FUNCTION
*
*       NU_Dhcp
*
*   DESCRIPTION
*
*       This function creates a DHCP request packet and sends it
*       to the DHCP server to obtain an IP address for the specified
*       named device.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCP Structure that contains data
*                               that can be obtained at the application layer.
*       *dv_name                Pointer to Device name.
*
*   OUTPUTS
*
*      NU_SUCCESS               Successful operation.
*      NU_INVALID_PARM          ds_ptr or dv_name are NU_NULL
*      NU_DHCP_INIT_FAILED      DHCP initialization failed
*      NU_INVALID_PROTOCOL      Invalid protocol was sent to NU_Socket
*      NU_NO_MEMORY             Allocation of Memory failed
*      NU_NO_SOCK_MEMORY        No more socket memory
*      NU_DHCP_REQUEST_FAILED   DHCP_Send_Request failed
*
******************************************************************************/
STATUS NU_Dhcp(DHCP_STRUCT *ds_ptr, const CHAR *dv_name)
{
    INT                         i;
    INT                         ret;
    INT                         found = 0;
    STATUS                      socketd = -1;
    INT32                       delay;
    UINT32                      rand_num;
    STATUS                      retval;
    struct addr_struct          clientaddr;
    DV_DEVICE_ENTRY             *device;
    STATUS                      status;
    UINT32                      abstime;
    UINT32                      abstime1;
    UINT32                      flags;
    DEV_IF_ADDR_ENTRY           *dev_addr_entry;
    UINT8                       null_addr[] = {0, 0, 0, 0};
    UINT32                      acquired_addr;

    NU_SUPERV_USER_VARIABLES

    if ( (ds_ptr == NU_NULL) || (dv_name == NU_NULL) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If the DHCP Module has not yet been initialize it, do so now */
    if (!(NET_Initialized_Modules & DHCP_CLIENT_MODULE))
    {
        status = DHCP_Initialize();

        if (status != NU_SUCCESS)
        {
            retval = NU_DHCP_INIT_FAILED;
            goto DHCP_Exit;
        }
    }

    /* Indicate in the cache for this device that DHCP has been enabled.  In
     * case of link DOWN / link UP, the system will know to re-invoke DHCP.
     */
    NU_Ifconfig_Set_DHCP4_Enabled(dv_name, NU_TRUE);

    /* Create a socket and bind to it, so that we can receive packets. */
    socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, NU_NONE);

    if (socketd < NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to create socket", NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (socketd);
    }

    /* build local address and port to bind to. */
    clientaddr.family = NU_FAMILY_IP;
    clientaddr.port   = DHCP_Client_Port;
    *(UINT32 *)clientaddr.id.is_ip_addrs = 0;
    clientaddr.name = "DHCP";

    /* Bind the socket. */
    ret = NU_Bind(socketd, &clientaddr, 0);

    if (ret != socketd)
    {
        NLOG_Error_Log("Unable to bind to socket", NERR_FATAL, __FILE__, __LINE__);
        retval = NU_DHCP_INIT_FAILED;
        goto DHCP_Exit;
    }

    UTL_Zero(ds_ptr->dhcp_siaddr, sizeof(ds_ptr->dhcp_siaddr));
    UTL_Zero(ds_ptr->dhcp_giaddr, sizeof(ds_ptr->dhcp_giaddr));
    UTL_Zero(ds_ptr->dhcp_sname, sizeof(ds_ptr->dhcp_sname));
    UTL_Zero(ds_ptr->dhcp_file, sizeof(ds_ptr->dhcp_file));

    /* Initialize the transaction ID. */
    ds_ptr->dhcp_xid = UTL_Rand();

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain semaphore", NERR_FATAL, __FILE__, __LINE__);
        retval = status;
        goto DHCP_Exit;
    }

    /* Get a pointer to the interface. */
    device = DEV_Get_Dev_By_Name(dv_name);

    if (device == NU_NULL)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                           __FILE__, __LINE__);

        retval = NU_DHCP_INIT_FAILED;
        goto DHCP_Exit;
    }

    /* Get a pointer to the address structure being obtained via DHCP */
    dev_addr_entry =
        DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

    /* If there is no DHCP entry, add one now */
    if (dev_addr_entry == NU_NULL)
    {
        if (DEV_Attach_IP_To_Device(device->dev_net_if_name, null_addr,
                                    null_addr) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to attach IP address", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* Get a pointer to the address structure being obtained via DHCP */
        dev_addr_entry =
            DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

        /* If there is no blank entry, add one now */
        if (dev_addr_entry == NU_NULL)
        {
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                               __FILE__, __LINE__);

            retval = NU_DHCP_INIT_FAILED;
            goto DHCP_Exit;
        }
    }

    /* Copy device MAC address */
    memcpy(ds_ptr->dhcp_mac_addr, device->dev_mac_addr, DADDLEN);

    /* If options were provided and this is the first time an attempt has been
       made to obtain a lease for this device then memory needs to be allocated
       to store the options in. */
    if ( (ds_ptr->dhcp_opts != NU_NULL) &&
         (device->dev_addr.dev_dhcp_options == NU_NULL))
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        status = NU_Allocate_Memory(MEM_Cached,
                                    (VOID **)&device->dev_addr.dev_dhcp_options,
                                    (UNSIGNED)ds_ptr->dhcp_opts_len,
                                    (UNSIGNED)NU_NO_SUSPEND);

#else

        if (No_of_Devices != NET_MAX_DHCP_DEVICES)
        {
            /* Assign memory to the dhcp options */
            device->dev_addr.dev_dhcp_options =
                &DHCP_Option_Memory[NET_MAX_DHCP_OPTS_LEN * No_of_Devices++];

            status = NU_SUCCESS;
        }
        else
            status = NU_NO_MEMORY;          /* if memory could not be found */
#endif

        if (status != NU_SUCCESS)
        {
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                               __FILE__, __LINE__);

            retval = status;
            goto DHCP_Exit;
        }

        /* Copy the user specified options. */
        memcpy(device->dev_addr.dev_dhcp_options, ds_ptr->dhcp_opts,
               ds_ptr->dhcp_opts_len);

        /* Preserve the length of the options. */
        device->dev_addr.dev_dhcp_opts_length = ds_ptr->dhcp_opts_len;
    }

    /* Set the DHCP state. */
    device->dev_addr.dev_dhcp_state = DHCP_INIT_STATE;

    /* Set the flag to indicate that address configuration is in progress
     * on this interface.
     */
    device->dev_flags |= DV_ADDR_CFG;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* The (i + 2) check is because the third element of the
       DHCP backoff array is used first and we don't want to
       extend past the end of the backoff array. See its use below. */
    for (i = 0; (i < RETRIES_COUNT) && ( (i + 2) < DHCP_MAX_BACKOFFS); i++)
    {
        if (i == 0)
        {
            /* this is the first discover message */
            ds_ptr->dhcp_secs = 0;
        }
        else
        {
            abstime = 0;

            /* taking care of wrapping of the clock */
            abstime1 = NU_Retrieve_Clock() / SCK_Ticks_Per_Second;
            if (abstime1 >= abstime)
                ds_ptr->dhcp_secs = (UINT16)( abstime1 - abstime);
            else
                ds_ptr->dhcp_secs = (UINT16)(0xFFFFFFFFUL - (abstime + abstime1));
        }

        /* Send the packet. */
        status = DHCP_Send(dv_name, ds_ptr, DHCPDISCOVER);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error occurred while sending DHCPDISCOVER packet",
                           NERR_SEVERE, __FILE__, __LINE__);

            retval = status;
            goto DHCP_Exit;
        }

        /* Compute a delay between retransmissions of DHCP messages. RFC 2131
           specifies that a randomized exponential backoff algorithm must be
           used. The first retransmission should be at 4 seconds, then 8, then
           16, etc., up to a max of 64. This value should be randomized by plus
           or minus a second. */

        /* Get a random number. */
        rand_num = UTL_Rand();

        /* Compute a number that is between 0 and SCK_Ticks_Per_Second. */
        delay = (INT32)(rand_num % SCK_Ticks_Per_Second);

        /* At this point we have a delay that is equal to some value between
           0 and 1 second. Now an arbitrary bit has been chosen from the random
           number. If this bit is set then change the sign of the delay. About
           half the time we should end up with a negative number. */
        if (rand_num & 0x80)
            delay = (-delay);

        /* Here we make use of the DHCP backoff array. This array looks like
           {1, 2, 4, 8, 16, .... 64, 64,..}. The first value taken from this
           array will be 4 (i+2). 4 will be multiplied by SCK_Ticks_Per_Second and
           the original delay will be added to it. The original delay
           will be a number of ticks that is in the range of -1 to +1 seconds.
           The final delay computed will be between 3 and 5 seconds (4 +|- 1). */
        delay = (INT32)((UINT32)DHCP_Backoff[i+2] * SCK_Ticks_Per_Second) + delay;

        /* Look at each packet on buffer_list to see if it is mine */
        found = DHCP_Process_Offer(socketd, ds_ptr, dv_name, (UINT32)delay);

        /* An Offer was received. Get out of the loop. */
        if (found)
            break;

        found = 0;
    }

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain semaphore", NERR_FATAL, __FILE__,
                       __LINE__);
        retval = status;
        goto DHCP_Exit;
    }

    /* Get a pointer to the interface. */
    device = DEV_Get_Dev_By_Name(dv_name);

    if (device == NU_NULL)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                           __FILE__, __LINE__);

        retval = NU_DHCP_INIT_FAILED;
        goto DHCP_Exit;
    }

    /* The returned value from DHCP_Process_Packet is tested here */
    /* to see if the IP address was accepted by the caller. */

    /* Set the DHCP state. */
    device->dev_addr.dev_dhcp_state = DHCP_REQUESTING_STATE;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    if ( (found & ACCEPT_BIT) == ACCEPT_BIT)
    {
        retval = DHCP_Send_Request(ds_ptr, socketd, dv_name);

        if (retval == NU_SUCCESS)
        {
            /* Obtain the TCP semaphore to protect the stack global variables */
            status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to obtain semaphore", NERR_FATAL,
                               __FILE__, __LINE__);

                retval = status;
                goto DHCP_Exit;
            }

            /* Get a pointer to the interface. */
            device = DEV_Get_Dev_By_Name(dv_name);

            if (device == NU_NULL)
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                retval = NU_DHCP_INIT_FAILED;
                goto DHCP_Exit;
            }

            /* Get a pointer to the address structure being obtained via DHCP */
            dev_addr_entry =
                DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

            if (dev_addr_entry == NU_NULL)
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                retval = NU_DHCP_INIT_FAILED;
                goto DHCP_Exit;
            }

            /* Before accepting the address make sure it is not in use
               by some other node on the LAN */

            /* Zero the duplicate address detections. */
            dev_addr_entry->dev_entry_dup_addr_detections = 0;

            /* Temporarily store off the IP address so that ARP
               can check against it. */
            dev_addr_entry->dev_entry_ip_addr = IP_ADDR(ds_ptr->dhcp_yiaddr);

            /* Temporarily store the IP address into a local array for
                use later.. */
            acquired_addr = IP_ADDR(ds_ptr->dhcp_yiaddr);

            /* The device will not send packets until an IP address is attached.
               Temporarily trick it into thinking an IP address is attached so this
               request can be sent.  Then set it back. */
            flags = device->dev_flags;
            device->dev_flags |= (DV_UP | DV_RUNNING);

            /* Send an ARP request to check if the offered IP address
               is already in use. */
            if ((ARP_Request(device, &dev_addr_entry->dev_entry_ip_addr,
                        (UINT8 *)"\0\0\0\0\0\0", EARP, ARPREQ)) != NU_SUCCESS)
            {
                /* If the ARP failed the processing still continues.
                   But at least log the error. */
                NLOG_Error_Log ("ARP Request failed", NERR_RECOVERABLE,
                                    __FILE__, __LINE__);
            }

            /* put the flags back */
            device->dev_flags = flags;

            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Wait half a second then check if a reply was
               received. */
            NET_Sleep(SCK_Ticks_Per_Second / 4);

            /* Obtain the TCP semaphore to protect the stack global variables */
            status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to obtain semaphore", NERR_FATAL,
                               __FILE__, __LINE__);

                retval = status;
                goto DHCP_Exit;
            }

            /* Get a pointer to the interface. */
            device = DEV_Get_Dev_By_Name(dv_name);

            if (device == NU_NULL)
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                retval = NU_DHCP_INIT_FAILED;
                goto DHCP_Exit;
            }

            /* Get a pointer to the address structure being obtained via DHCP */
            dev_addr_entry =
                DEV_Find_Target_Address(device, acquired_addr);

            if (dev_addr_entry == NU_NULL)
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                retval = NU_DHCP_INIT_FAILED;
                goto DHCP_Exit;
            }

            /* Clear the addr */
            dev_addr_entry->dev_entry_ip_addr = 0;

            /* Was a duplicate address detected? */
            if (dev_addr_entry->dev_entry_dup_addr_detections == 0)
            {
                if (DEV_Initialize_IP(device, ds_ptr->dhcp_yiaddr,
                                      ds_ptr->dhcp_net_mask,
                                      dev_addr_entry) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Unable to attach DHCP addr to device", NERR_SEVERE,
                                   __FILE__, __LINE__);
                    retval = NU_DHCP_INIT_FAILED;

                }  /* if status */
                else
                {
                    device->dev_addr.dev_dhcp_addr = IP_ADDR(ds_ptr->dhcp_yiaddr);

                    /* Set the DHCP state. */
                    device->dev_addr.dev_dhcp_state = DHCP_BOUND_STATE;

                    /* Set up the timer events that will renew the lease. */
                    DHCP_Init_Timers(device);
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }
            else
            {
                /* Someone else is using the IP address that we were provided.
                    Send a DECLINE message. */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                /* Send a DHCP decline msg */
                status = DHCP_Send(dv_name, ds_ptr, DHCPDECLINE);

                if (status != NU_SUCCESS)
                    NLOG_Error_Log("Error occurred while sending a DHCPDECLINE packet",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                /* return with an error */
                retval = NU_DHCP_REQUEST_FAILED;
            }

        } /*  If found */
    } /*  Accept Bit  */
    else
    {
         NLOG_Error_Log("DHCP request failed", NERR_FATAL, __FILE__, __LINE__);

         retval = NU_DHCP_REQUEST_FAILED;
    }

DHCP_Exit:

    /* Obtain the TCP semaphore to protect the device structure. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        device = DEV_Get_Dev_By_Name(dv_name);

        if (device)
        {
            /* Clear the flag to indicate that address configuration is
             * finished for this interface.
             */
            device->dev_flags &= ~DV_ADDR_CFG;
        }

        else
        {
            NLOG_Error_Log("Device removed while resolving DHCP address",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_FATAL,
                           __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Could not get TCP semaphore in NU_Dhcp", NERR_FATAL,
                       __FILE__, __LINE__);
    }

    /* If a socket was successfully opened, close it now. */
    if (socketd >= 0)
    {
        if (NU_Close_Socket(socketd) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (retval);

} /* NU_Dhcp */

/******************************************************************************
*
*   FUNCTION
*
*       DHCP_Send_Request
*
*   DESCRIPTION
*
*       Send a DHCP Request message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCP structure.
*       socketd                 Socket descriptor.
*       *device                 Pointer to device structure.
*       *dev_addr_entry         Pointer to the device address structure that
*                               is being filled in via DHCP.
*
*   OUTPUTS
*
*       NU_SUCCESS              The address was successfully renewed.
*       NU_NO_ACTION            The DHCP Server could not be found.
*       NU_DHCP_REQUEST_FAILED  If the Renew request was denied.
*
******************************************************************************/
STATIC STATUS DHCP_Send_Request(DHCP_STRUCT *ds_ptr, INT socketd,
                                const CHAR *dv_name)
{
    INT                 i;
    INT32               delay;
    UINT32              rand_num;
    INT                 found;
    STATUS              status;

    found = 0;

    /* The (i + 2) check is because the third element of the
       DHCP backoff array is used first and we don't want to
       extend past the end of the backoff array. See it's use below. */
    for (i = 0; (i < RETRIES_COUNT) && ( (i + 2) < DHCP_MAX_BACKOFFS); i++)
    {
        /* Send the packet. */
        status = DHCP_Send(dv_name, ds_ptr, DHCPREQUEST);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error occurred while sending DHCPREQUEST packet",
                           NERR_FATAL, __FILE__, __LINE__);
            break;
        }

        /* Compute a delay between retransmissions of DHCP messages. RFC 2131
           specifies that a randomized exponential backoff algorithm must be
           used. The first retransmission should be at 4 seconds, then 8, then
           16, etc., up to a max of 64. This value should be randomized by plus
           or minus a second. */

        /* Get a random number. */
        rand_num = UTL_Rand();

        /* Compute a number that is between 0 and SCK_Ticks_Per_Second. */
        delay = (INT32)(rand_num % SCK_Ticks_Per_Second);

        /* At this point we have a delay that is equal to some value between
           0 and 1 second. Now an arbitrary bit has been chosen from the random
           number. If this bit is set then change the sign of the delay. About
           half the time we should end up with a negative number. */
        if (rand_num & 0x80)
            delay = (-delay);

        /* Here we make use of the DHCP backoff array. This array looks like
           {1, 2, 4, 8, 16, .... 64, 64,..}. The first value taken from this
           array will be 4 (i+2). 4 will be multiplied by SCK_Ticks_Per_Second and
           the original delay will be added to it. The original delay
           will be a number of ticks that is in the range of -1 to +1 seconds.
           The final delay computed will be between 3 and 5 seconds (4 +|- 1). */
        delay = (INT32)((UINT32)DHCP_Backoff[i+2] * SCK_Ticks_Per_Second) + delay;

        found = DHCP_Process_ACK(socketd, ds_ptr, dv_name, (UINT32)delay);

        if (found != 0)
        {
            break;
        }

        found = 0;
    }

    /* The address was successfully renewed. */
    if (found == 1)
        return (NU_SUCCESS);

    /* The DHCP Server could not be found to renew the address.  Stay in
     * the Renew state until the Rebind timer expires.
     */
    else if (found == 0)
        return (NU_NO_ACTION);

    /* The DHCP Server denied the Renew request. */
    else
        return (NU_DHCP_REQUEST_FAILED);

} /* DHCP_Send_Request */

/**************************************************************************
*
*   FUNCTION
*
*       NU_Dhcp_Release
*
*   DESCRIPTION
*
*       This function releases the address acquired by the NU_Dhcp
*       function call.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that can be obtained at the application
*                               layer.
*       *dv_name                Pointer to Devices name.
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_DHCP_INIT_FAILED     A resource (memory, socket, etc.) was not
*                               available or could not be initialized properly
*       NU_INVALID_PARM         If ds_ptr or dv_name are NU_NULL
*       NU_NO_BUFFERS           DHCP_Build_Buffers has no more buffers
*       -1                      Device output function may return this
*
**************************************************************************/
STATUS NU_Dhcp_Release(const DHCP_STRUCT *ds_ptr, const CHAR *dv_name)
{
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Indicate in the cache for this device that DHCP has been disabled.  In
     * case of link DOWN / link UP, the system will know not to re-invoke DHCP.
     */
    NU_Ifconfig_Set_DHCP4_Enabled(dv_name, NU_FALSE);

    /* Release the IP address and transmit a release message. */
    status = DHCP_Release_Address(ds_ptr, dv_name, NU_TRUE);

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

}  /* NU_Dhcp_Release */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP_Release_Address
*
*   DESCRIPTION
*
*       This function releases the address acquired by the NU_Dhcp
*       function call.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that can be obtained at the application
*                               layer.
*       *dv_name                Pointer to Devices name.
*       tx_release              Indicates whether a release message should
*                               be sent to the server.  If this routine is
*                               called from the API, it will send a release
*                               message.  If called internally due to link
*                               down, a message will not be sent.
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_DHCP_INIT_FAILED     A resource (memory, socket, etc.) was not
*                               available or could not be initialized properly
*       NU_INVALID_PARM         If ds_ptr or dv_name are NU_NULL
*       NU_NO_BUFFERS           DHCP_Build_Buffers has no more buffers
*       -1                      Device output function may return this
*
**************************************************************************/
STATUS DHCP_Release_Address(const DHCP_STRUCT *ds_ptr, const CHAR *dv_name,
                            UINT8 tx_release)
{
    DV_DEVICE_ENTRY     *device;
    STATUS              status;
    UINT8               ip_addr[IP_ADDR_LEN];

    /* ds_ptr can be NULL if tx_release is FALSE. */
    if ( ((ds_ptr == NU_NULL) && (tx_release != NU_FALSE)) ||
         (dv_name == NU_NULL) )
        return (NU_INVALID_PARM);

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* get a pointer to the interface structure. */
        device = DEV_Get_Dev_By_Name(dv_name);

        /* Was a valid device found? */
        if ( (device) &&
             (device->dev_mtu >= (UINT32)(device->dev_hdrlen + DHCP_MAX_HEADER_SIZE)) )
        {
            /* Go ahead and clear the IP address renewal, rebinding and release
             * events.
             */
            TQ_Timerunset(DHCP_Renew, TQ_CLEAR_EXACT, (UNSIGNED)device->dev_index, 0);
            TQ_Timerunset(DHCP_Rebind, TQ_CLEAR_EXACT, (UNSIGNED)device->dev_index, 0);
            TQ_Timerunset(DHCP_New_Lease, TQ_CLEAR_EXACT, (UNSIGNED)device->dev_index, 0);
            TQ_Timerunset(DHCP_Release, TQ_CLEAR_EXACT, (UNSIGNED)device->dev_index, 0);

            /* Perform the following only if a release message should be
             * transmitted to the server.
             */
            if (tx_release == NU_TRUE)
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                /* Send the release message. */
                status = DHCP_Send(dv_name, ds_ptr, DHCPRELEASE);

                if (status == NU_SUCCESS)
                {
                    /* Ping the server to make sure the address is resolved, then detach
                       the IP from the device. Note that regardless of the response, the
                       DHCP release packet will have been sent, so the IP will be detached. */
                    NU_Ping(ds_ptr->dhcp_siaddr, SCK_Ticks_Per_Second);

                    /* Obtain the TCP semaphore to protect the stack global variables */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* get a pointer to the interface structure. */
                        device = DEV_Get_Dev_By_Name(dv_name);

                        if (!device)
                        {
                            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            NLOG_Error_Log("Invalid device", NERR_SEVERE, __FILE__,
                                           __LINE__);
                        }
                    }

                    else
                        NLOG_Error_Log("Failed to obtain semaphore",
                                       NERR_FATAL, __FILE__, __LINE__);
                }

                else
                    NLOG_Error_Log("Error occurred while sending DHCPRELEASE packet",
                                   NERR_FATAL, __FILE__, __LINE__);
            }

            if ( (status == NU_SUCCESS) && (device) )
            {
                /* Save off the IP address */
                PUT32(ip_addr, 0, device->dev_addr.dev_dhcp_addr);

                /* Zero out the IP address being maintained by DHCP */
                device->dev_addr.dev_dhcp_addr = 0;

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                /* Delete the address from the device */
                if (NU_Remove_IP_From_Device(dv_name, ip_addr,
                                             NU_FAMILY_IP) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to detach IP address from device",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Obtain the TCP semaphore to protect the stack global variables */
            status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* get a pointer to the interface structure. */
                device = DEV_Get_Dev_By_Name(dv_name);

                /* If there are DHCP options associated with the device, deallocate
                 * the memory for the options and set the options pointer to NULL
                 * so a subsequent call to NU_Dhcp can specify new options if
                 * desired.
                 */
                if ( (device) && (device->dev_addr.dev_dhcp_options) )

                {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    if (NU_Deallocate_Memory((VOID*)device->dev_addr.dev_dhcp_options)
                        != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory for DHCP options",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
#else
                    No_of_Devices--;
#endif

                    device->dev_addr.dev_dhcp_options = NU_NULL;
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }
        }

        else
        {
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            status = NU_DHCP_INIT_FAILED;
        }
    }

    return (status);

}  /* DHCP_Release_Address */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Process_Offer
*
*   DESCRIPTION
*
*       Process a DHCP offer message.
*
*   INPUTS
*
*       socketd                 Socket Descriptor to retrieve data from.
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that can be obtained at the application
*                               layer.
*       *device                 Pointer to the device.
*       timeout                 Timeout value used for NU_Select.
*
*   OUTPUTS
*
*      NU_TRUE                  If process if successful
*      NU_NO_SOCK_MEMORY        Allocation of memory cannot occur
*
******************************************************************************/
STATIC STATUS DHCP_Process_Offer(INT socketd, DHCP_STRUCT *ds_ptr,
                                 const CHAR *dv_name, UINT32 timeout)
{
    INT32           ret;
    INT16           flen;
    INT16           found = 0;
    UINT32          vend_cookie;
    UINT8           *pt;
    FD_SET          readfs;
    struct addr_struct fromaddr;
    UINT32          clock_begin,
                    clock_end,
                    time_left;
    DHCPLAYER HUGE  *dhcp_ptr;          /* DHCP uses a Bootp type struct */
    INT             opts_byte_count;    /* Tracks number of bytes of option fields. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    STATUS      status;
#endif


#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&dhcp_ptr, DHCP_PACKET_LEN,
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to alloc memory for DHCP recv buffer", NERR_FATAL,
                            __FILE__, __LINE__);
        return (NU_NO_SOCK_MEMORY);
    }

#else

    /* Declare memory and assign it to the dhcp ptr */
    CHAR        dhcp_process_memory[DHCP_PACKET_LEN];
    dhcp_ptr = (DHCPLAYER HUGE *) dhcp_process_memory;

#endif

    /* Retrieve and store the clock ticks for a comparison of elapsed time */
    clock_begin = NU_Retrieve_Clock();

    /* Set the time that that the timeout will expire. */
    clock_end = clock_begin + timeout;

    do
    {
        NU_FD_Init(&readfs);
        NU_FD_Set(socketd, &readfs);

        /* Wait for a response to arrive. We are only interested in arriving
           packets so the 3rd and 4th parameters are not needed. */
        ret = NU_Select(socketd + 1, &readfs, NU_NULL, NU_NULL, timeout);

        if (ret == NU_NO_DATA)
            break;

        if (NU_FD_Check(socketd, &readfs) == NU_FALSE)
            break;

        fromaddr.family = NU_FAMILY_IP;
        fromaddr.port = 0;

        ret = NU_Recv_From(socketd, (CHAR *)dhcp_ptr, DHCP_PACKET_LEN, 0, &fromaddr, &flen);

        if (ret <= 0)
        {
            NLOG_Error_Log("Error occurred while performing NU_Recv_From", NERR_FATAL,
                                __FILE__, __LINE__);
            break;
        }
        else if (ret < (DHCP_PKT_FIXED_PART_LEN + 4 + 1))
        {
            /* Size is less than fixed-part header plus cookie and room for 
             * at least END option. Seems an invalid DHCP message; try again.
             */
            NLOG_Error_Log("Invalid DHCP message received", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
            continue; 
        }

        if (dhcp_ptr->dp_xid != ds_ptr->dhcp_xid)
        {
            /* Determine if the timeout value has expired. */
            time_left = TQ_Check_Duetime(clock_end);

            /* Determine if the timeout value for waiting on a reply from a server has
                past.  If so, we can break out of this loop.  If not, continue. */
            if (time_left > 0)
                continue;

            else
                break;
        }

        /* At this point we are looking for a DHCP offer. If we did not get
           one then move on to the next one. Add 4 to get past the cookie. */
        if (DHCP_Message_Type(dhcp_ptr->dp_vend + 4) != DHCPOFFER)
            continue;

        /* Validate the DHCPOFFER. */
        if (DHCP_Validate(ds_ptr, dv_name, dhcp_ptr->dp_siaddr) != 0 )
            IP_ADDR_COPY(ds_ptr->dhcp_siaddr, dhcp_ptr->dp_siaddr);

        IP_ADDR_COPY(ds_ptr->dhcp_giaddr, dhcp_ptr->dp_giaddr);
        IP_ADDR_COPY(ds_ptr->dhcp_yiaddr, dhcp_ptr->dp_yiaddr);

        memcpy(ds_ptr->dhcp_sname, dhcp_ptr->dp_sname, sizeof(ds_ptr->dhcp_sname) );
        memcpy(ds_ptr->dhcp_file, dhcp_ptr->dp_file, sizeof(ds_ptr->dhcp_file) );
        vend_cookie = GET32(dhcp_ptr->dp_vend, 0);

        /* test to see if cookie is a vendor cookie */
        if (vend_cookie == DHCP_COOKIE)
        {
            /* Now loop through vendor options, At this point the only option
               we are interested in is the DHCP_SERVER_ID. RFC 2131 recommends
               using the configuration parameters from the ACK rather than the
               offer. */
            pt = dhcp_ptr->dp_vend;

            pt += 4;                /* move past cookie */
            opts_byte_count = 4;

            /* Loop until END option is found, or we process all the message. */
            while( (*pt != DHCP_END) && (opts_byte_count < (ret - DHCP_PKT_FIXED_PART_LEN)) )
            {
                /* The only option we are interested in at this point is the
                   server ID. */
                if (*pt == DHCP_SERVER_ID)
                {
                    /* Step past the option type and option length. */
                    pt += 2;
                    opts_byte_count += 2;

                    /* Validate the DHCPOFFER. */
                    if (DHCP_Validate(ds_ptr, dv_name, pt) == 0)
                         continue;

                    /* Return the type of the DHCP message. */
                    IP_ADDR_COPY(ds_ptr->dhcp_siaddr, pt);
                    found = NU_TRUE;

                    break;
                }
                /* If it is a PAD option just step past it. */
                else if (*pt == DHCP_PAD)     /* move past PAD bytes */
                {
                    pt++;
                    opts_byte_count++;
                    continue;
                }
                /* Step past any other options. */
                else
                {
                    /* First step past the option type. */
                    pt++;
                    opts_byte_count++;
                    /* Now step past the length of the option and the value. */
                    opts_byte_count += (*pt + 1);
                    pt += (*pt + 1);
                }
            }
        }

    } while( found == 0 );

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    if (NU_Deallocate_Memory(dhcp_ptr) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for DHCP pointer",
                       NERR_SEVERE, __FILE__, __LINE__);
#endif

    return (found);

} /* DHCP_Process_Offer */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Process_ACK
*
*   DESCRIPTION
*
*       Process a DHCP ACK message.
*
*   INPUTS
*
*       socketd                 Socket Descriptor to retrieve data from.
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that can be obtained at the application
*                               layer.
*       *device                 Pointer to device structure
*       timeout                 Timeout value used for NU_Select.
*       dev_addr_entry          A pointer to the address structure.
*
*   OUTPUTS
*
*       -1                      NAK received.
*       1                       Acknowledgement found.
*       0                       Acknowledgement not found.
*
******************************************************************************/
STATIC STATUS DHCP_Process_ACK(INT socketd, DHCP_STRUCT *ds_ptr,
                               const CHAR *dv_name, UINT32 timeout)
{
    INT32               ret;
    INT16               flen;
    INT16               found = 0;
    UINT8               *pt;
    DHCPLAYER  HUGE     *inbp;                  /* DHCP uses a Bootp type struct */
    FD_SET              readfs;
    struct              addr_struct fromaddr;
    UINT8               opcode;
    INT16               length;
    DV_DEVICE_ENTRY     *device;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;
    INT                 opts_byte_count;        /* Tracks number of bytes of option fields. */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&inbp, DHCP_PACKET_LEN,
                           (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
    {
        return (0);
    }
#else
    /* Declare memory and assign it to the inbp */
    CHAR        dhcp_ack_memory[DHCP_PACKET_LEN];
    inbp   =    (DHCPLAYER HUGE *)dhcp_ack_memory;
#endif

    do
    {
        NU_FD_Init(&readfs);
        NU_FD_Set(socketd, &readfs);

        /* Wait for a response to arrive. We are only interested in arriving
           packets so the 3rd and 4th parameters are not needed. */
        ret = NU_Select(socketd + 1, &readfs, NU_NULL, NU_NULL, timeout);

        if (ret == NU_NO_DATA)
            break;

        if (NU_FD_Check(socketd, &readfs) == NU_FALSE)
            break;

        fromaddr.family = NU_FAMILY_IP;
        fromaddr.port = 0;
        ret = NU_Recv_From(socketd, (CHAR *)inbp, DHCP_PACKET_LEN, 0, &fromaddr, &flen);

        if (ret <= 0)
        {
            NLOG_Error_Log("Error occurred while performing NU_Recv_From", NERR_FATAL,
                                __FILE__, __LINE__);
            break;
        }
        else if (ret < (DHCP_PKT_FIXED_PART_LEN + 4 + 1))
        {
            /* Size is less than fixed-part header plus cookie and room for 
             * at least END option. Seems an invalid DHCP message; try again.
             */
            NLOG_Error_Log("Invalid DHCP message received", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
            continue; 
        }

        /* See if this message is returning a response to the message I sent */
        if (inbp->dp_xid != ds_ptr->dhcp_xid)
            continue;

        /* test to see if cookie is a vendor cookie */
        if (GET32(inbp->dp_vend, 0) != DHCP_COOKIE)
            continue;

        /* As per RFC 2131, if we receive a NAK, we need to restart the process */
        if (DHCP_Message_Type(inbp->dp_vend + 4) == DHCPNAK)
        {
            found = -1;
            break;
        }

        /* At this point we are only interested in receiving a DHCP ACK.
           Make sure that is what we have received. The 4 is added to get past
           the cookie. */
        if (DHCP_Message_Type(inbp->dp_vend + 4) != DHCPACK)
            break;

        found = 1;

        /* Now loop through vendor options, passing them to user call */
        /* back function. */
        pt = inbp->dp_vend;
        pt += 4;                        /* move past cookie */
        opts_byte_count = 4;

        /* Obtain the TCP semaphore to protect the stack global variables */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
        {
            /* Get a pointer to the interface. */
            device = DEV_Get_Dev_By_Name(dv_name);

            if (device)
            {
                /* Zero out the RENEW and REBIND timers.  The new times will be
                 * provided by the DHCP Server in the new lease. If not, the timers
                 * will be set to default values as per RFC 2131.
                 */
                device->dev_addr.dev_dhcp_renew = 0;
                device->dev_addr.dev_dhcp_rebind = 0;

                /* Get a pointer to the address structure being obtained via DHCP */
                dev_addr_entry =
                    DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

                if (dev_addr_entry)
                {
                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Loop until END option is found, or we process all the message. */
                    while ( (*pt != DHCP_END) && (opts_byte_count < (ret - DHCP_PKT_FIXED_PART_LEN)) )
                    {
                        if (*pt == DHCP_PAD)          /* move past PAD bytes */
                        {
                            pt++;
                            opts_byte_count++;
                            continue;
                        }

                        opcode = *pt++;                /* save opcode */
                        length = *pt++;                /* save length */
                        opts_byte_count += 2;

                        /* Process the options in the message. */
                        if (DHCP_Vendor_Options(dv_name, opcode, (UINT16)length, pt,
                                                ds_ptr) != NU_SUCCESS)
                            NLOG_Error_Log("Error with DHCP Vendor Options",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                        pt += length;
                        opts_byte_count += length;
                    }
                }

                else
                {
                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    break;
                }
            }

            else
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                break;
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
            break;
        }

    } while (found == 0);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    if (NU_Deallocate_Memory(inbp) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif
    return (found);

} /* DHCP_Process_ACK */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP_Build_Message
*
*   DESCRIPTION
*
*       This function handles the initing of the DHCP packet.
*
*   INPUTS
*
*       **return_buf            This is a pointer to the buffer that the
*                               DHCP message is built in. It is undefined
*                               upon function entry. It will be set to
*                               point to a buffer upon exit.
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that is provide by the application.
*       *device                 Pointer to device structure.
*       *dev_addr_entry         Pointer to the device address structure.
*       msg_type                Type of DHCP message to build.
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_DHCP_INIT_FAILED     DHCP Initialization failed
*       NU_NO_BUFFERS           All the NET buffers are used up
*
**************************************************************************/
STATIC STATUS DHCP_Build_Message(NET_BUFFER **return_buf, const DHCP_STRUCT *ds_ptr,
                                 const DV_DEVICE_ENTRY *device,
                                 const DEV_IF_ADDR_ENTRY *dev_addr_entry,
                                 UINT8 msg_type)
{
    STATUS      status;
    STATUS      retval = NU_SUCCESS;
    NET_BUFFER  *buf_ptr;
    UINT32      ip_src, ip_dst;

    /* Allocate a Nucleus NET buffer chain to put the DHCP discover packet in. */
    buf_ptr =
        MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                 (device->dev_hdrlen + DHCP_MAX_HEADER_SIZE));

    if (buf_ptr != NU_NULL)
    {
        /* After the packet is sent deallocate the buffer to the DHCP List. */
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
        buf_ptr->data_len = 0;
        buf_ptr->mem_total_data_len = 0;

        /* build the first DHCP discovery message */
        status = DHCP_Init_DHCP_Layer(buf_ptr, ds_ptr, device, dev_addr_entry,
                                      msg_type);

        if (status == NU_SUCCESS)
        {
            switch (device->dev_addr.dev_dhcp_state)
            {
            case DHCP_RENEW_STATE :
            case DHCP_BOUND_STATE :

                ip_src = dev_addr_entry->dev_entry_ip_addr;
                ip_dst = device->dev_addr.dev_dhcp_server_addr;
                break;

            case DHCP_REBIND_STATE :

                ip_src = dev_addr_entry->dev_entry_ip_addr;
                ip_dst = IP_ADDR_BROADCAST;

                /* The request will be broadcasted. */
                buf_ptr->mem_flags |= NET_BCAST;

                break;

            default :

                /* This is most likely the requesting state. */
                ip_src = IP_ADDR_ANY;
                ip_dst = IP_ADDR_BROADCAST;

                /* The request will be broadcasted. */
                buf_ptr->mem_flags |= NET_BCAST;

                break;
            }

            DHCP_Init_UDP_Layer(buf_ptr, ip_src, ip_dst);
            DHCP_Init_IP_Layer(buf_ptr, ip_src, ip_dst);
        }
        else
        {
            /* DHCP_Init_DHCP_Layer failed. Free the buffer and return the
               reason for the failure.*/
            retval = status;
            MEM_One_Buffer_Chain_Free (buf_ptr, &MEM_Buffer_Freelist);
        }
    }
    else
    {
        retval = NU_NO_BUFFERS;
    }

    *return_buf = buf_ptr;

    return (retval);

} /* DHCP_Build_Message */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP_Init_DHCP_Layer
*
*   DESCRIPTION
*
*       Initialize the DHCP layer of the message.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the buffer in which the
*                               DHCP message is being built.
*       *ds_ptr                 Pointer to DHCP Structure that contains
*                               data that is provide by the application.
*       *device                 Pointer to the device for which we are
*                               trying to acquire an IP address.
*       *dev_addr_entry         Pointer to the device address structure that
*                               is being filled in via DHCP.
*       msg_type                Type of DHCP message being built (discover,
*                               request, release).
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_NO_MEMORY            No memory to allocate from
*       NU_DHCP_INIT_FAILED     DHCP initialization has failed
*
**************************************************************************/
STATIC STATUS DHCP_Init_DHCP_Layer(NET_BUFFER *buf_ptr, const DHCP_STRUCT *ds_ptr,
                                   const DV_DEVICE_ENTRY *device,
                                   const DEV_IF_ADDR_ENTRY *dev_addr_entry,
                                   UINT8 msg_type)
{
    INT             len, i;
    UINT8           *pt;
    STATUS          status;
    DHCPLAYER HUGE  *pkt;
    UINT8           *opts_ptr;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    DHCPLAYER       dhcp_layer_memory;
#endif

    /* The size of the Nucleus NET buffer chains are user configurable. This
       makes it difficult build the DHCP packet in the Nucleus NET buffer chain.
       To avoid these complexities build the buffer in contiguous memory and
       then copy it to the buffer chain. Allocate the contiguous memory here. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&pkt,
                                 sizeof(DHCPLAYER), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
        return (status);
#else

    /* Declare memory and assign it to pkt */
    pkt    =  &dhcp_layer_memory;

#endif

    memset(pkt, 0, sizeof(DHCPLAYER) );

    pkt->dp_op      = BOOTREQUEST;      /* opcode, 1 octet */
    pkt->dp_htype   = HARDWARE_TYPE;    /* hardware type, 1 octet */
    pkt->dp_hlen    = DADDLEN;          /* hardware address length, 1 octet */
    pkt->dp_xid     = ds_ptr->dhcp_xid;

    if ((msg_type == DHCPDISCOVER) || (msg_type == DHCPREQUEST))
        PUT16(&(pkt->dp_secs), 0, ds_ptr->dhcp_secs);
    else
        pkt->dp_secs = 0;

    pkt->dp_flags= 0; /* set to unicast */

    /* Fill in the client's hardware address. */
    memcpy(pkt->dp_chaddr, ds_ptr->dhcp_mac_addr, DADDLEN);

    len = sizeof(pkt->dp_op) +
      sizeof(pkt->dp_htype) +
      sizeof(pkt->dp_hlen) +
      sizeof(pkt->dp_hops) +
      sizeof(pkt->dp_xid) +
      sizeof(pkt->dp_secs) +
      sizeof(pkt->dp_flags) +
      sizeof(pkt->dp_ciaddr) +
      sizeof(pkt->dp_yiaddr) +
      sizeof(pkt->dp_siaddr) +
      sizeof(pkt->dp_giaddr) +
      sizeof(pkt->dp_chaddr) +
      sizeof(pkt->dp_sname) +
      sizeof(pkt->dp_file);

    pt = pkt->dp_vend;
    PUT32(pt, 0, DHCP_COOKIE);
    pt += 4;
    *pt++ = DHCP_MSG_TYPE;
    *pt++ = 1;
    *pt++ = msg_type;
    *pt++ = DHCP_CLIENT_CLASS_ID;

    /* Determine the length of the packet so far. */
    len += 8;

#if (INCLUDE_DHCP_RFC4361_CLIENT_ID == NU_FALSE)

    *pt++ = 7;
    *pt++ = HARDWARE_TYPE;
    *pt++ = ds_ptr->dhcp_mac_addr[0];
    *pt++ = ds_ptr->dhcp_mac_addr[1];
    *pt++ = ds_ptr->dhcp_mac_addr[2];
    *pt++ = ds_ptr->dhcp_mac_addr[3];
    *pt++ = ds_ptr->dhcp_mac_addr[4];
    *pt++ = ds_ptr->dhcp_mac_addr[5];
    len += 8;

#else

    /* If using the Client ID defined in RFC 4361, the Client ID option
     * should look like the following:
     *
     *  Code  Len  Type  IAID                DUID
     *  +----+----+-----+----+----+----+----+----+----+---
     *  | 61 | n  | 255 | i1 | i2 | i3 | i4 | d1 | d2 |...
     *  +----+----+-----+----+----+----+----+----+----+---
     */

    /* Determine the length of the packet. */
    if (DHCP_Duid.duid_type == DHCP_DUID_EN)
    {
        /* 1-byte Type field, 4-byte IAID, 4-byte Enterprise Number and
         * variable length ID Number.
         */
        *pt++ = (UINT8)(1 + 4 + 4 + DHCP_Duid.duid_id_no_len);
    }

    else
    {
        /* 1-byte Type field, 4-byte IAID, 2-byte Hardware Type field
         * and 6-byte Link-Layer Address.
         */
        *pt++ = 1 + 4 + 2 + DADDLEN;
    }

    /* Set the type. */
    *pt++ = DHCP_RFC4361_CLIENT_ID;

    /* Insert the IAID and increment the buffer pointer. */
    PUT32(pt, 0, device->dev_dhcp_iaid);

    /* Increment the buffer pointer. */
    pt += 4;

    /* Add the Len, Type and IAID fields into the total length of the
     * packet.
     */
    len += 6;

    /* Determine the type of DUID being used and set it accordingly. */
    if (DHCP_Duid.duid_type == DHCP_DUID_EN)
    {
        /* Add the Enterprise Number to the packet. */
        PUT32(pt, 0, DHCP_Duid.duid_ent_no);

        /* Increment the buffer pointer. */
        pt +=4;

        /* Add the Identifier to the packet. */
        NU_BLOCK_COPY(pt, DHCP_Duid.duid_id, DHCP_Duid.duid_id_no_len);

        /* Increment the buffer pointer. */
        pt += DHCP_Duid.duid_id_no_len;

        /* Increment the length of the packet. */
        len += (4 + DHCP_Duid.duid_id_no_len);
    }

    else
    {
        /* Add the Hardware Type to the packet. */
        PUT16(pt, 0, DHCP_Duid.duid_hw_type);

        /* Increment the buffer pointer. */
        pt += 2;

        /* Add the Link-Layer address to the packet. */
        NU_BLOCK_COPY(pt, DHCP_Duid.duid_ll_addr, DADDLEN);

        /* Increment the buffer pointer. */
        pt += DADDLEN;

        /* Increment the length of the packet. */
        len += (2 + DADDLEN);
    }

#endif

    if (msg_type == DHCPDISCOVER) /* Building a Discover packet. */
    {
        /* Add any user requested options to the Discover packet */
        for (opts_ptr = ds_ptr->dhcp_opts, i = 0;
             (UINT8)i < ds_ptr->dhcp_opts_len;
             i += (DHCP_OPTION_HDR_LENGTH + opts_ptr[DHCP_OPTION_LENGTH]),
             opts_ptr += (DHCP_OPTION_HDR_LENGTH + opts_ptr[DHCP_OPTION_LENGTH]))
        {
            /* RFC 2131 - Section 4.4.1 - Table 5 - Must not include
             * Server Identifier and Should Not include Message option in
             * a DHCPDISCOVER packet.
             */
            if ( (opts_ptr[DHCP_OPTION_CODE] != DHCP_SERVER_ID) &&
                 (opts_ptr[DHCP_OPTION_CODE] != DHCP_MESSAGE) )
            {
                /* Copy the options into the packet */
                memcpy(pt, opts_ptr,
                       DHCP_OPTION_HDR_LENGTH + opts_ptr[DHCP_OPTION_LENGTH]);

                /* Increment the destination buffer by the amount of data
                 * just copied into it.
                 */
                pt +=
                    (DHCP_OPTION_HDR_LENGTH + opts_ptr[DHCP_OPTION_LENGTH]);

                /* Increment the total length by the length of this option */
                len +=
                    (DHCP_OPTION_HDR_LENGTH + opts_ptr[DHCP_OPTION_LENGTH]);
            }
        }
    }

    else if (msg_type == DHCPRELEASE) /* Building a release packet. */
    {
        /* Fill in ciaddr. */
        PUT32(pkt->dp_ciaddr, 0, dev_addr_entry->dev_entry_ip_addr);
    }
    else if (msg_type == DHCPDECLINE)
    {
        /* Send the server the IP address that we are declining as a requested IP
            option tag. */
        *pt++ = DHCP_REQUEST_IP;
        *pt++ = 4;
        memcpy(pt, ds_ptr->dhcp_yiaddr, IP_ADDR_LEN);
        pt += IP_ADDR_LEN;
        len += 6;

        /* The DECLINE message is only meant for the server that provided us with the IP
            address that is in use by another client. */
        *pt++ = DHCP_SERVER_ID;
        *pt++ = 4;
        *pt++ = ds_ptr->dhcp_siaddr[0];
        *pt++ = ds_ptr->dhcp_siaddr[1];
        *pt++ = ds_ptr->dhcp_siaddr[2];
        *pt++ = ds_ptr->dhcp_siaddr[3];
        len += 6;
    }
    else  /* Must be building a request packet. */
    {
        /* Only fill in ciaddr if in the renewing or rebinding state. */
        if ( (device->dev_addr.dev_dhcp_state == DHCP_RENEW_STATE) ||
             (device->dev_addr.dev_dhcp_state == DHCP_REBIND_STATE) )
              PUT32(pkt->dp_ciaddr, 0, dev_addr_entry->dev_entry_ip_addr);

        /* Only send the DHCP_Requested IP address during the INIT state. */
        if (device->dev_addr.dev_dhcp_state == DHCP_REQUESTING_STATE)
        {
            *pt++ = DHCP_REQUEST_IP;
            *pt++ = 4;
            *pt++ = ds_ptr->dhcp_yiaddr[0];
            *pt++ = ds_ptr->dhcp_yiaddr[1];
            *pt++ = ds_ptr->dhcp_yiaddr[2];
            *pt++ = ds_ptr->dhcp_yiaddr[3];
            len += 6;
        }

        /* Only send the server ID option if in the INIT state. */
        if (device->dev_addr.dev_dhcp_state == DHCP_REQUESTING_STATE)
        {
            /* tell all servers that responded to the DHCPDISCOVER broadcast */
            /* that the following server has been selected. */
            *pt++ = DHCP_SERVER_ID;
            *pt++ = 4;
            *pt++ = ds_ptr->dhcp_siaddr[0];
            *pt++ = ds_ptr->dhcp_siaddr[1];
            *pt++ = ds_ptr->dhcp_siaddr[2];
            *pt++ = ds_ptr->dhcp_siaddr[3];
            len += 6;
        }

        if (ds_ptr->dhcp_opts)
        {
            /* The user wishes to make some specific requests as to which
               options the server should return. */
            /* Copy the application specified options. */
            memcpy(pt, ds_ptr->dhcp_opts, ds_ptr->dhcp_opts_len);
            pt += ds_ptr->dhcp_opts_len;
            len += ds_ptr->dhcp_opts_len;
        }
    }

    /* Mark the end of the options list. */
    *pt = DHCP_END;
    len++;

    /* Some DHCP servers (SUN Solaris) reject DHCP messages that are less than
       300 bytes in length. Pad the message if necessary. */
    if (len < 300)
        len = 300;

    status = DHCP_Pkt_Copy((UINT8 *)pkt, buf_ptr, len, device);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    if (NU_Deallocate_Memory(pkt) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif

    if (status != NU_SUCCESS)
        return (NU_DHCP_INIT_FAILED);
    else
        return (NU_SUCCESS);

} /* DHCP_Init_DHCP_Layer */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Pkt_Copy
*
*   DESCRIPTION
*
*       Copy a DHCP message into a Nucleus NET buffer chain.
*
*   INPUTS
*
*       *buffer                 A pointer to the packet to copy
*       *buf_ptr                A pointer to where the packet is copied
*                               from the buffer
*       length                  The length of the packet
*       *int_face               A pointer to the interface out which the
*                               packet will be transmitted.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success.
*       DHCP_INIT_FAILURE       DHCP initialization failure
*
******************************************************************************/
STATIC STATUS DHCP_Pkt_Copy(UINT8 *buffer, NET_BUFFER *buf_ptr, INT length,
                            const DV_DEVICE_ENTRY *int_face)
{
    INT         current_size, total_size = length;
    NET_BUFFER  *next_buf;

    /* Point to where the DHCP packet should begin. */
    buf_ptr->data_ptr =
        buf_ptr->mem_parent_packet + (NET_MAX_UDP_HEADER_SIZE + int_face->dev_hdrlen);

    /*  Chain the DHCP Request Packet */
    if (total_size > NET_PARENT_BUFFER_SIZE)
    {
        current_size =
            NET_PARENT_BUFFER_SIZE - (NET_MAX_UDP_HEADER_SIZE + int_face->dev_hdrlen);

        total_size = total_size - current_size;
    }
    else
    {
       current_size =  total_size;
       total_size = 0;
    }

    /*  Copy DHCP Packet into first Buffer */
    memcpy(buf_ptr->data_ptr, buffer, (unsigned int)current_size);
    buf_ptr->data_len = (UINT32)current_size;
    buf_ptr->mem_total_data_len = (UINT32)length;

    /* Point to fresh data. */
    buffer += current_size;

    /* Point to the next buffer is there is one.  */
    next_buf = buf_ptr->next_buffer;

    /*  Check to make sure there is data to store in the mem_packet */
    while (total_size && next_buf)
    {
        /* Determine how much data can be copied to the next buffer. */
        if (total_size > (INT)NET_MAX_BUFFER_SIZE)
            current_size = NET_MAX_BUFFER_SIZE;
        else
            current_size = total_size;

        total_size -= current_size;

        /*  Copy the remaining data in the chaining packets  */
        memcpy(next_buf->mem_packet, buffer, (unsigned int)current_size);

        /*  Set the Data pointer to the remainder of the packets.  */
        next_buf->data_ptr = next_buf->mem_packet;
        next_buf->next = NU_NULL;
        next_buf->data_len = (UINT32)current_size;

        /* Point to the next buffer is there is one.  */
        buffer = buffer + current_size;

        /* Point to the next buffer in the chain. */
        next_buf = next_buf->next_buffer;
    }

    /* When the original buffer chain was allocated it was done so assuming
       that a max DHCP message would be built. However, this is unlikely.
       If there are any unused buffers in the chain then free them. The main
       reason for doing this is in case the driver is not designed to handle a
       buffer chain that contains empty buffers. */
    for (next_buf = buf_ptr; next_buf; next_buf = next_buf->next_buffer)
    {
        /* If there is a buffer after the current one and that buffer contains
           no data then free that buffer and any that follow it. */
        if (next_buf->next_buffer && (next_buf->next_buffer->data_len == 0))
        {
            MEM_One_Buffer_Chain_Free (next_buf->next_buffer,
                                       &MEM_Buffer_Freelist);
            next_buf->next_buffer = NU_NULL;
        }
    }

    /* At this point total_size should be 0. If it is not 0 then something
       has gone wrong. */
    if (total_size)
        return (NU_DHCP_INIT_FAILED);
    else
        return (NU_SUCCESS);

} /* DHCP_Pkt_Copy */

/******************************************************************************
*
*   FUNCTION
*
*       DHCP_Send
*
*   DESCRIPTION
*
*       Send a DHCP Message.
*
*   INPUTS
*
*       *device                 A pointer to the device entry
*       *dev_addr_entry         Pointer to the device address structure that
*                               is being filled in via DHCP.
*       *ds_ptr                 A pointer to the DHCP structure
*       msg_type                The message type
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_DHCP_INIT_FAILED     DHCP Initialization failed
*       NU_NO_BUFFERS           DHCP_Build_Message has no buffers to use
*       NU_INVALID_PARM         There is no device associated with dv_name
*                               in the system.
*       -1                      Output function for the device failed
*
******************************************************************************/
STATIC STATUS DHCP_Send(const CHAR *dv_name, const DHCP_STRUCT *ds_ptr,
                        UINT8 msg_type)
{
    STATUS              status;
    UINT32              flags;
    SCK_SOCKADDR_IP     sa;
    NET_BUFFER          *buf_ptr;
    IPLAYER   HUGE      *ip_ptr;
    RTAB_ROUTE          ro;
    RTAB_ROUTE          *route_for_dhcp = NU_NULL;
    DV_DEVICE_ENTRY     *device;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        device = DEV_Get_Dev_By_Name(dv_name);

        if (device)
        {
            /* Get a pointer to the address structure being obtained via DHCP */
            dev_addr_entry =
                DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

            if (dev_addr_entry)
            {
                /* build the first DHCP discovery message */
                status = DHCP_Build_Message(&buf_ptr, ds_ptr, device, dev_addr_entry,
                                            msg_type);

                if (status == NU_SUCCESS)
                {
                    ip_ptr = (IPLAYER *)(buf_ptr->data_ptr);

                    /* Initialize the structure to all zeros so unused members
                     * get initialized.
                     */
                    memset(&sa, 0, sizeof(SCK_SOCKADDR_IP));

                    /*  Set up MAC layer  */
                    sa.sck_family = SK_FAM_IP;
                    sa.sck_len = sizeof (sa);
                    sa.sck_addr = GET32(ip_ptr, IP_DEST_OFFSET);

                    if (device->dev_addr.dev_dhcp_state == DHCP_RENEW_STATE)
                    {
                        /* Check whether DHCP server is in same subnet. */
                        if ((dev_addr_entry->dev_entry_ip_addr &
                             dev_addr_entry->dev_entry_netmask) !=
                            (sa.sck_addr & dev_addr_entry->dev_entry_netmask))
                        {
                            route_for_dhcp = &ro;
                            UTL_Zero((CHAR *)route_for_dhcp, sizeof(RTAB_ROUTE));

                            memcpy(&(route_for_dhcp->rt_ip_dest), &sa,
                                   sizeof(SCK_SOCKADDR_IP));

                            route_for_dhcp->rt_route =
                                RTAB4_Find_Route(&route_for_dhcp->rt_ip_dest,
                                                 RT_BEST_METRIC);

                            if (route_for_dhcp->rt_route == NU_NULL)
                            {
                                /* Increment the number of packets that could not be delivered. */
                                MIB2_ipOutNoRoutes_Inc;
                                route_for_dhcp = NU_NULL;
                            }
                            else
                            {
                                /* If the next hop is a gateway then set the destination
                                   ip address to the gateway. */
                                if (route_for_dhcp->rt_route->rt_entry_parms.rt_parm_flags &
                                    RT_GATEWAY)
                                    memcpy(&sa, &route_for_dhcp->rt_route->rt_gateway_v4,
                                           sizeof(SCK_SOCKADDR_IP));
                            }
                        }
                    }

                    /* The device will not send packets until an IP address is attached.
                       Temporarily trick it into thinking an IP address is attached so this
                       request can be sent.  Then set it back. */
                    flags = device->dev_flags;
                    device->dev_flags |= (DV_UP | DV_RUNNING);

                    /* Send the packet. */
                    status = (*(device->dev_output))(buf_ptr, device, &sa, route_for_dhcp);

                    /* If the packet was not successfully transmitted, free the buffer*/
                    if (status != NU_SUCCESS)
                    {
                        MEM_One_Buffer_Chain_Free (buf_ptr, &MEM_Buffer_Freelist);
                    }

                    device->dev_flags = flags;

                    /* Free the route used to send the packet */
                    if (route_for_dhcp)
                        RTAB_Free((ROUTE_ENTRY*)route_for_dhcp->rt_route, NU_FAMILY_IP);
                }
            }
        }

        /* The device has been removed while trying to perform DHCP requests. */
        else
        {
            status = NU_INVALID_PARM;

            NLOG_Error_Log("Device removed while resolving DHCP address",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* DHCP_Send */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Init_UDP_Layer
*
*   DESCRIPTION
*
*       Initialize the UDP header that encapsulates the DHCP message.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the net buffer
*       src                     The source IP address
*       dst                     The destination IP address
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
STATIC VOID DHCP_Init_UDP_Layer(NET_BUFFER *buf_ptr, UINT32 src, UINT32 dst)
{
    UDPLAYER  HUGE      *udp_pkt;
    struct pseudotcp    tcp_chk;

    udp_pkt = (UDPLAYER *)(buf_ptr->data_ptr - UDP_HEADER_LEN);

    /* Initialize the local and foreign port numbers. */
    PUT16(udp_pkt, UDP_SRC_OFFSET, DHCP_Client_Port);
    PUT16(udp_pkt, UDP_DEST_OFFSET, DHCP_Server_Port);

    /*  Set the total length. */
    PUT16(udp_pkt, UDP_LENGTH_OFFSET,
          (UINT16)(buf_ptr->mem_total_data_len + UDP_HEADER_LEN));

    /* The checksum field should be cleared to zero before the checksum is
       computed. */
    PUT16(udp_pkt, UDP_CHECK_OFFSET, 0);

    /* Point to the start of the UDP header. Update the size data length of
       the whole chain and the data length for this buffer. */
    buf_ptr->data_ptr -= UDP_HEADER_LEN;
    buf_ptr->mem_total_data_len += UDP_HEADER_LEN;
    buf_ptr->data_len += UDP_HEADER_LEN;

    /*  Calculate the UDP Checksum. */
    tcp_chk.source  = LONGSWAP(src);
    tcp_chk.dest    = LONGSWAP(dst);
    tcp_chk.z       = 0;
    tcp_chk.proto   = IP_UDP_PROT;
    tcp_chk.tcplen  = INTSWAP((UINT16)buf_ptr->mem_total_data_len);
    PUT16(udp_pkt, UDP_CHECK_OFFSET, TLS_TCP_Check( (VOID *)&tcp_chk, buf_ptr) );

    /* If a checksum of zero is computed it should be replaced with 0xffff. */
    if (GET16(udp_pkt, UDP_CHECK_OFFSET) == 0)
        PUT16(udp_pkt, UDP_CHECK_OFFSET, 0xFFFF);

} /* DHCP_Init_UDP_Layer */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Init_IP_Layer
*
*   DESCRIPTION
*
*       Initialize the IP header that encapsulates the DHCP message.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the net buffer
*       ip_src                  The IP source
*       ip_dest                 The IP destination
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
STATIC VOID DHCP_Init_IP_Layer(NET_BUFFER *buf_ptr, UINT32 ip_src, UINT32 ip_dest)
{
    IPLAYER   HUGE      *ip_ptr;
    INT16               hlen = IP_HEADER_LEN;
    UINT32              length;

    ip_ptr = (IPLAYER *)(buf_ptr->data_ptr - IP_HEADER_LEN);

    /* Set the IP header length and the IP version. */
    PUT8( ip_ptr, IP_VERSIONANDHDRLEN_OFFSET,
          (UINT8)((hlen >> 2) | (IP_VERSION << 4)) );

    /* Set to no fragments and don't fragment. */
    /* ip_ptr->frags = INTSWAP(IP_DF); */
    PUT16(ip_ptr, IP_FRAGS_OFFSET, 0);

    /* Set the IP packet ID. */
    PUT16(ip_ptr, IP_IDENT_OFFSET, 0);

    /* Set the type of service. */
    PUT8(ip_ptr, IP_SERVICE_OFFSET, 0);

    length = buf_ptr->mem_total_data_len;

    /* Set the total length (data and IP header) for this packet. */
    PUT16(ip_ptr, IP_TLEN_OFFSET, (UINT16)((UINT16)length + hlen));

    /* Set the time to live */
    PUT8(ip_ptr, IP_TTL_OFFSET, IP_Time_To_Live);

    /* Set the protocol. */
    PUT8(ip_ptr, IP_PROTOCOL_OFFSET, IP_UDP_PROT);

    /* Fill in the destination and source IP addresses. */
    PUT32(ip_ptr, IP_SRC_OFFSET, ip_src);
    PUT32(ip_ptr, IP_DEST_OFFSET, ip_dest);

    /* Compute the IP checksum. Note that the length expected by */
    /* ipcheck is the length of the header in 16 bit half-words. */
    PUT16(ip_ptr, IP_CHECK_OFFSET, 0);
    PUT16(ip_ptr, IP_CHECK_OFFSET, TLS_IP_Check ((VOID *)ip_ptr, (UINT16)(hlen >> 1)));

    /*  Set the buffer pointer to the IP Layer.  */
    buf_ptr->data_ptr -= IP_HEADER_LEN;

    /*  Add the IPLAYER to the total data length */
    buf_ptr->mem_total_data_len += IP_HEADER_LEN;

    /*  Set the data length of the current packet.  */
    buf_ptr->data_len += IP_HEADER_LEN;

} /* DHCP_Init_IP_Layer */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Validate
*
*   DESCRIPTION
*
*       Function decides if a DHCPOFFER packet should be accepted.  An
*       example is provided that verifies the response is from the expected
*       DHCP server.  It is intended that this function will need to be
*       modified for specific applications.
*
*   INPUTS
*
*       *ds_ptr                 DHCP struct pointer.
*       *dv_name                A pointer to the device name of the current
*                               network card.
*       *dp                     DHCPLAYER structure pointer of current
*                               DHCPOFFER packet.
*
*   OUTPUTS
*
*       NU_TRUE                 Current DHCPOFFER packet is valid.
*       NU_FALSE                Not valid.
*
******************************************************************************/
INT DHCP_Validate(const DHCP_STRUCT *ds_ptr, const CHAR *dv_name, const UINT8 *dp)
{
#if (DHCP_VALIDATE_CALLBACK == NU_TRUE)
    UINT8      server_ip[] = {0x86, 0x56, 0x64, 0x63};

    /* stops compiler from complaining about unused variables. */
    if (ds_ptr == 0 || dv_name == 0 || dp == 0)
        return(NU_FALSE);

    /* Other items could be checked here but the user would have to */
    /* parse the vendor options field of the DHCPOFFER packet.  See */
    /* RFC 2132. */
    /*** Note: As of RFC 2132 the vendor option field is now variable ***/
    /*** length and not 64 bytes in size. Your must look for a DHCP_END. **/

    /* Does the DHCP server IP address match the address of the "trusted"
       DHCP server. */
    if (memcmp(server_ip, dp, 4) == 0)
        return(NU_TRUE);
    else
        return(NU_FALSE);

#else

    /* Get rid of compiler warnings. These parameters probably will be used
       when a user fleshes out this function. */
    UNUSED_PARAMETER(ds_ptr);
    UNUSED_PARAMETER(dv_name);
    UNUSED_PARAMETER(dp);

    return(NU_TRUE);

#endif

} /* DHCP_Validate */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Vendor_Options
*
*   DESCRIPTION
*
*       Function handles all vendor options returned by the DHCP server.
*
*   INPUTS
*
*       *device                 Pointer to the device entry structure
*       opc                     Opcode for the vendor option.
*       len                     Length of vendor option in bytes.
*       *opt_data               Pointer to the length of vendor option
*                               in bytes.
*       *ds_ptr                 Pointer to vendor option value.
*
*   OUTPUTS
*
*       NU_SUCCESS              All vendor options processed.
*       -1                      There are still vendor options to be
*                               processed.
*
******************************************************************************/
STATIC STATUS DHCP_Vendor_Options(const CHAR *dv_name, UINT8 opc, UINT16 len,
                                  UINT8 HUGE *opt_data, DHCP_STRUCT *ds_ptr)
{
    STATUS          ret = NU_SUCCESS;
    CHAR            host[MAX_HOST_NAME_LENGTH];
    DV_DEVICE_ENTRY *device;

#if (INCLUDE_DNS == NU_TRUE)
    UINT8       ip_address[4];
#endif

    /* As of RFC2131 and RFC2132 */
    /* This switch contains all opcodes known as of the above RFC's. */

    /* none or all opcodes could be returned by the DHCP server. */

    switch( opc )
    {
        case DHCP_NETMASK:
            IP_ADDR_COPY(ds_ptr->dhcp_net_mask, opt_data);
            break;

        case DHCP_TIME_OFFSET:
            break;

        case DHCP_ROUTE:

            /* Obtain the TCP semaphore to protect the stack global variables */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Get a pointer to the interface. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                {
                    /* Set the default route. */
                    if (RTAB4_Set_Default_Route(device, GET32(opt_data, 0),
                                                (INT32)(RT_UP | RT_GATEWAY)) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to set default route", NERR_SEVERE,
                                       __FILE__, __LINE__);
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            IP_ADDR_COPY(ds_ptr->dhcp_giaddr, opt_data);

            break;

        case DHCP_TIME:
        case DHCP_NAME_SERVER:
            break;

        case DHCP_DNS:

#if (INCLUDE_DNS == NU_TRUE)

            /* We received an list of DNS server addresses. Add each in turn
               to the Nucleus NET list of DNS servers. */
            while(len)
            {
                IP_ADDR_COPY(ip_address, opt_data);

                /* When renewing the lease the list of servers received will
                   likely be the same as the first list. So first attempt to
                   delete the server. This will prevent duplicate addresses
                   from appearing in the list. */
                if (NU_Delete_DNS_Server2(ip_address, NU_FAMILY_IP) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to delete DNS server", NERR_INFORMATIONAL,
                                   __FILE__, __LINE__);

                /* Adding to the end will insure that the first one received is
                   the closest to the head of the list. */
                if (NU_Add_DNS_Server(ip_address, DNS_ADD_TO_END) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to add DNS server", NERR_SEVERE,
                                   __FILE__, __LINE__);
                len -= (UINT16)4;
                opt_data = &opt_data[4];
            }
#endif
            break;

        case DHCP_LOG_SERVER:
        case DHCP_COOKIE_SERVER:
        case DHCP_LPR_SERVER:
        case DHCP_IMPRESS_SERVER:
        case DHCP_RESOURCE_SERVER:

        	break;

        case DHCP_HOSTNAME:

            SCK_Get_Host_Name(host, MAX_HOST_NAME_LENGTH);

            /* Accept hostname only if we don't already have one. */
            if ( (*host == 0) && (len < MAX_HOST_NAME_LENGTH) )
            {
                if (SCK_Set_Host_Name((CHAR*)opt_data, len) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set Host Name", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);
                }
            }
            break;

        case DHCP_BOOT_FILE_SIZE:
        case DHCP_MERIT_DUMP_FILE:
            break;

        case DHCP_DOMAIN_NAME:

            if ( (len > 1) && (len < NET_MAX_DOMAIN_NAME_LENGTH) )
            {
                if (SCK_Set_Domain_Name((CHAR*)opt_data, len) != NU_SUCCESS)
                {
                    NLOG_Error_Log("DHCP failed to set domain name",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            break;

        case DHCP_SWAP_SERVER:
        case DHCP_ROOT_PATH:
        case DHCP_EXTENSIONS_PATH:
            break;

            /* IP Layer Parameters per Host. */
        case DHCP_IP_FORWARDING:
        case DHCP_NL_SOURCE_ROUTING:
        case DHCP_POLICY_FILTER:
        case DHCP_MAX_DATAGRAM_SIZE:
        case DHCP_IP_TIME_TO_LIVE:
        case DHCP_MTU_AGING_TIMEOUT:
        case DHCP_MTU_PLATEAU_TABLE:
            break;

            /* IP Layer Parameters per Interface. */
        case DHCP_INTERFACE_MTU:
        case DHCP_ALL_SUBNETS:
        case DHCP_BROADCAST_ADDR:
        case DHCP_MASK_DISCOVERY:
        case DHCP_MASK_SUPPLIER:
        case DHCP_ROUTER_DISCOVERY:
        case DHCP_ROUTER_SOLICI_ADDR:
        case DHCP_STATIC_ROUTE:
            break;

            /* Link Layer Parameters per Interface. */
        case DHCP_TRAILER_ENCAP:
        case DHCP_ARP_CACHE_TIMEOUT:
        case DHCP_ETHERNET_ENCAP:
            break;

            /* TCP Parameters. */
        case DHCP_TCP_DEFAULT_TTL:
        case DHCP_TCP_KEEPALIVE_TIME:
        case DHCP_TCP_KEEPALIVE_GARB:
            break;

            /* Application and Service Parameters. */
        case DHCP_NIS_DOMAIN:
        case DHCP_NIS:
        case DHCP_NTP_SERVERS:
        case DHCP_VENDOR_SPECIFIC:
        case DHCP_NetBIOS_NAME_SER:
        case DHCP_NetBIOS_DATA_SER:
        case DHCP_NetBIOS_NODE_TYPE:
        case DHCP_NetBIOS_SCOPE:
        case DHCP_X11_FONT_SERVER:
        case DHCP_X11_DISPLAY_MGR:
        case DHCP_NIS_PLUS_DOMAIN:
        case DHCP_NIS_PLUS_SERVERS:
        case DHCP_MOBILE_IP_HOME:
        case DHCP_SMTP_SERVER:
        case DHCP_POP3_SERVER:
        case DHCP_NNTP_SERVER:
        case DHCP_WWW_SERVER:
        case DHCP_FINGER_SERVER:
        case DHCP_IRC_SERVER:
        case DHCP_STREETTALK_SERVER:
        case DHCP_STDA_SERVER:
            break;

            /* DHCP Extensions */
        case DHCP_REQUEST_IP:
            break;

        case DHCP_IP_LEASE_TIME:

            /* Obtain the TCP semaphore to protect the stack global variables */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Get a pointer to the interface. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                {
                    /* Set the lease time for the acquired IP address. */
                    device->dev_addr.dev_dhcp_lease = GET32(opt_data, 0);
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            else
                ret = -1;

            break;

        case DHCP_OVERLOAD:
        case DHCP_MSG_TYPE:
            break;

        case DHCP_SERVER_ID:
            IP_ADDR_COPY(ds_ptr->dhcp_siaddr, opt_data);

            /* Obtain the TCP semaphore to protect the stack global variables */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Get a pointer to the interface. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                    device->dev_addr.dev_dhcp_server_addr = GET32(opt_data, 0);

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            else
                ret = -1;

            break;

        case DHCP_REQUEST_LIST:
        case DHCP_MESSAGE:
        case DHCP_MAX_MSG_SIZE:
            break;

        case DHCP_RENEWAL_T1:

            /* Obtain the TCP semaphore to protect the stack global variables */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Get a pointer to the interface. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                    device->dev_addr.dev_dhcp_renew = GET32(opt_data, 0);

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            else
                ret = -1;

            break;

        case DHCP_REBINDING_T2:

            /* Obtain the TCP semaphore to protect the stack global variables */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Get a pointer to the interface. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                    device->dev_addr.dev_dhcp_rebind = GET32(opt_data, 0);

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            else
                ret = -1;

            break;

        case DHCP_VENDOR_CLASS_ID:
        case DHCP_CLIENT_CLASS_ID:
            break;

        default:
            break;
    }

    return (ret);

} /* DHCP_Vendor_Options */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Message_Type
*
*   DESCRIPTION
*
*       Given a list of options from a DHCP message, find and return the
*       message type.
*
*   INPUTS
*
*       *opt                    Pointer to DHCP option list.
*
*   OUTPUTS
*
*      DHCP message type        Success
*      0xff                     Failure.
*
******************************************************************************/
STATIC UINT8 DHCP_Message_Type(const UINT8 *opt)
{
    UINT8  type = 0xff;

    /* Search the complete option list looking for an option of type. */
    while (*opt != DHCP_END)
    {
        /* If this a message type option then return the packet type. */
        if (*opt == DHCP_MSG_TYPE)
        {
            /* Step past the option type and option length. */
            opt += 2;

            /* Return the type of the DHCP message. */
            type = *opt;
            break;
        }
        /* If it is a PAD option just step past it. */
        else if (*opt == DHCP_PAD)     /* move past PAD bytes */
        {
            opt++;
            continue;
        }
        /* Handle all other options. */
        else
        {
            /* First step past the option type. */
            opt++;
            /* Now step past the length of the option and the value. */
            opt += (*opt + 1);
        }
    }

    return (type);

} /* DHCP_Message_Type */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Init_Timers
*
*   DESCRIPTION
*
*       This function sets up the timers for lease renewal/rebinding when
*       the lease is initially acquired.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to set up
*                               timers
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
STATIC VOID DHCP_Init_Timers(DV_DEVICE_ENTRY *device)
{
    /* If the lease time is not infinite then set up the timers. */
    if (device->dev_addr.dev_dhcp_lease != 0xFFFFFFFFuL)
    {
        /* If the DHCP server did not return times for when the lease should
           be renewed or rebound, then set them both to the default values as
           recommended by RFC 2131. */
        if ( (device->dev_addr.dev_dhcp_renew == 0) ||
             (device->dev_addr.dev_dhcp_rebind == 0) )
        {
            /* RFC 2132 recommends a renewal time of 1/2 the lease duration. */
            device->dev_addr.dev_dhcp_renew =
                device->dev_addr.dev_dhcp_lease >> 1;

            /* RFC 2132 recommends a rebinding time of
               (.875 * the lease duration). Shift value right 3 places arriving
               at a value (.125 * lease duration) and subtract that from lease
               duration. */
            device->dev_addr.dev_dhcp_rebind =
                device->dev_addr.dev_dhcp_lease -
                (device->dev_addr.dev_dhcp_lease >> 3);
        }

        /* Create the two timer events. The first is the time at which we
           will attempt to renew the lease. The second is the time at which
           we will attempt to rebind to any address. */
        if (TQ_Timerset(DHCP_Renew, (UNSIGNED)device->dev_index,
                        device->dev_addr.dev_dhcp_renew * SCK_Ticks_Per_Second,
                        0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set DHCP_Renew timer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (TQ_Timerset(DHCP_Rebind, (UNSIGNED)device->dev_index,
                        device->dev_addr.dev_dhcp_rebind * SCK_Ticks_Per_Second,
                        0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set DHCP_Rebind timer",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* DHCP_Init_Timers */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Update_Timers
*
*   DESCRIPTION
*
*       This function updates the times for the renewal or rebinding of the
*       IP address in the event that the first attempt fails.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to update
*                               the timers.
*       event                   The event, either DHCP_Renew or DHCP_Rebind
*       ticks                   The time in clock ticks at which the renewal
*                               or rebinding began
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       -1                      Failure
*
******************************************************************************/
STATIC STATUS DHCP_Update_Timers(const CHAR *dv_name, TQ_EVENT event,
                                 UINT32 ticks)
{
    UINT32          seconds;
    STATUS          status;
    DV_DEVICE_ENTRY *device;

    /* Get the current time so the amount of time elapsed since the
       renewing/rebinding was initiated can be computed. The clock ticks are
       treated as signed to compensate for the potential wrapping of the clock.
    */
    ticks = NU_Retrieve_Clock() - ticks;

    /* How many seconds elapsed. */
    seconds = ticks / SCK_Ticks_Per_Second;

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        device = DEV_Get_Dev_By_Name(dv_name);

        if (device)
        {
            if (event == DHCP_Renew)
            {
                /* RFC 2131 specifies that if the renewal attempt fails then a new
                   attempt should be made at half the remaining time between
                   the present and rebinding. This should be done down to a minimum of
                   60 seconds. */

                /* Compute the time between now and rebinding, then half it. */
                device->dev_addr.dev_dhcp_renew += seconds;

                if (device->dev_addr.dev_dhcp_renew < device->dev_addr.dev_dhcp_rebind)
                {
                    /* Half the time between now and rebinding. */
                    seconds = (device->dev_addr.dev_dhcp_rebind -
                               device->dev_addr.dev_dhcp_renew) >> 1;

                    /* If there is at least 60 seconds before rebinding is attempted,
                       then schedule another renewal attempt. */
                    if (seconds >= 60)
                    {
                        /* Update the renewal time in case we have to do this again. */
                        device->dev_addr.dev_dhcp_renew += seconds;

                        if (TQ_Timerset(DHCP_Renew, (UNSIGNED)device->dev_index,
                                        seconds * SCK_Ticks_Per_Second,
                                        0) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to set DHCP_Renew timer",
                                           NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
            else if (event == DHCP_Rebind)
            {
                /* RFC 2131 specifies that if the rebinding attempt fails then a new
                   attempt should be made at half the remaining time between
                   the present and lease expiration. This should be done down to a
                   minimum of 60 seconds. */

                /* Compute the time between now and lease expiration, then half it. */
                device->dev_addr.dev_dhcp_rebind += seconds;

                if (device->dev_addr.dev_dhcp_rebind < device->dev_addr.dev_dhcp_lease)
                {
                    seconds = (device->dev_addr.dev_dhcp_lease -
                               device->dev_addr.dev_dhcp_rebind) >> 1;

                    /* Update the rebinding time in case we have to do this again. */
                    device->dev_addr.dev_dhcp_rebind += seconds;

                    /* If there is at least 60 seconds remaining before lease
                     * expiration, then schedule another rebinding attempt.
                     */
                    if (seconds >= 60)
                    {
                        if (TQ_Timerset(DHCP_Rebind, (UNSIGNED)device->dev_index,
                                        seconds * SCK_Ticks_Per_Second,
                                        0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to set DHCP_Rebind timer",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }

                    /* Otherwise, continue to use the address for the remaining amount
                     * of the lease, and then release it when the lease expires.
                     */
                    else
                    {
                        if (TQ_Timerset(DHCP_Release, (UNSIGNED)device->dev_index,
                                        (device->dev_addr.dev_dhcp_lease -
                                        device->dev_addr.dev_dhcp_rebind) *
                                        SCK_Ticks_Per_Second,
                                        0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to set DHCP_Release timer",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }
                }
                else
                    status = -1;
            }
        }

        else
            status = -1;

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    return (status);

} /* DHCP_Update_Timers */

/******************************************************************************
*
*   FUNCTION
*
*       DHCP_Event_Handler
*
*   DESCRIPTION
*
*       This function is the task entry point for the task that handles DHCP
*       events. Specifically the renewing and/or rebinding of the IP address
*       lease. This was made a task because it is necessary to suspend. The
*       events dispatcher can not be allowed to suspend.
*
*   INPUTS
*
*       argc                    Unused
*       *argv                   Unused
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
STATIC VOID DHCP_Event_Handler(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    INT                 socketd = 0;
    DHCP_STRUCT         *ds_ptr;
    struct addr_struct  clientaddr;
    UNSIGNED            queue_message[DHCP_Q_MESSAGE_SIZE];
    UNSIGNED            act_size;
    TQ_EVENT            event;
    DV_DEVICE_ENTRY     *device;
    UINT32              time;
    UINT32              rand_num;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;
    CHAR                dv_name[DEV_NAME_LENGTH];
    UINT8               null_addr[] = {0, 0, 0, 0};

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Declare memory for DHCP structure */
    DHCP_STRUCT         dhcp_structure_memory;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Get rid of compiler warnings. */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Allocate memory for the DHCP structure. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ds_ptr,
                                sizeof(DHCP_STRUCT), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to alloc memory for DHCP structure", NERR_FATAL,
                            __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);

        /* Return to user mode */
        NU_USER_MODE();
        return;
    }
#else
    /* Assign memory to DHCP structure pointer */
    ds_ptr = &dhcp_structure_memory;
#endif

    for (;;)
    {
        status = NU_Receive_From_Queue(DHCP_Event_Queue, queue_message,
                                       DHCP_Q_MESSAGE_SIZE, &act_size,
                                       NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error occurred while receiving from event queue", NERR_FATAL,
                                __FILE__, __LINE__);

            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);

            break;
        }

        /* Retrieve the current time. This will be used later if it is necessary
           to reschedule the event. */
        time = NU_Retrieve_Clock();

        /* Get the event and the device pointer. */
        event = (TQ_EVENT)queue_message[0];

        /* Obtain the TCP semaphore to protect the stack global variables */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                           __FILE__, __LINE__);

            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);

            break;
        }

        /* Get a pointer to the device structure */
        device = DEV_Get_Dev_By_Index(queue_message[1]);

        /* If a matching device exists in the system */
        if (device)
        {
            /* Get a pointer to the address structure of the address obtained
             * via DHCP.
             */
            dev_addr_entry =
                DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

            /* If no address structure was found. */
            if (dev_addr_entry == NU_NULL)
            {
                /* Attach an address of all zeros so a new address structure
                 * entry will be added to the list of address structure
                 * entries for the device.
                 */
                if (DEV_Attach_IP_To_Device(device->dev_net_if_name, null_addr,
                                            null_addr) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to attach IP address", NERR_SEVERE,
                                   __FILE__, __LINE__);
                }

                /* Get a pointer to the new address structure just created. */
                dev_addr_entry =
                    DEV_Find_Target_Address(device, device->dev_addr.dev_dhcp_addr);

                /* If an address structure was still not found. */
                if (dev_addr_entry == NU_NULL)
                {
                    /* Zero out the address so the application layer can recover */
                    device->dev_addr.dev_dhcp_addr = 0;

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    /* Log an error. */
                    NLOG_Error_Log("No matching address structure for DHCP address",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    continue;
                }
            }

            /* Set the state. */
            if (event == DHCP_Renew)
                device->dev_addr.dev_dhcp_state = DHCP_RENEW_STATE;
            else
                device->dev_addr.dev_dhcp_state = DHCP_REBIND_STATE;

            UTL_Zero(ds_ptr, sizeof(*ds_ptr));

            /* Copy device MAC address */
            memcpy(ds_ptr->dhcp_mac_addr, device->dev_mac_addr, DADDLEN);

            /* Copy the the IP address for which we wish to renew the lease. */
            IP_ADDR_COPY(ds_ptr->dhcp_yiaddr, &dev_addr_entry->dev_entry_ip_addr);

            memcpy(dv_name, device->dev_net_if_name, DEV_NAME_LENGTH);

            clientaddr.name = dv_name;

            /* Initialize the transaction ID. */
            ds_ptr->dhcp_xid = UTL_Rand();

            /* No need to create a new socket and bind if we are in New Lease state */
            if ((event == DHCP_Renew) || (event == DHCP_Rebind))
            {
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                /* Create a socket and bind to it, so that we can receive packets. */
                socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, NU_NONE);

                if (socketd < 0 )
                {
                    NLOG_Error_Log("Unable to create socket", NERR_FATAL, __FILE__, __LINE__);

                    NET_DBG_Notify(socketd, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    if (DHCP_Update_Timers(dv_name, event, time) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to update DHCP timers",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    continue;
                }

                /* Build local address and port to bind to. */
                clientaddr.family = NU_FAMILY_IP;
                clientaddr.port   = DHCP_Client_Port;
                *(UINT32 *)clientaddr.id.is_ip_addrs = 0;

                /* Bind the socket. */
                status = NU_Bind(socketd, &clientaddr, 0);

                if (status != socketd)
                {
                    NLOG_Error_Log("Unable to bind to socket", NERR_FATAL, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    if (DHCP_Update_Timers(dv_name, event, time) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to update DHCP timers",
                                       NERR_SEVERE, __FILE__, __LINE__);

                    status = NU_Close_Socket(socketd);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to close DHCP socket",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    continue;
                }
            }

            if (event == DHCP_Renew)
            {
                /* Obtain the TCP semaphore to protect the stack global variables */
                status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                    break;
                }

                /* get a pointer to the interface structure. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                {
                    ds_ptr->dhcp_opts = (UINT8 *)device->dev_addr.dev_dhcp_options;
                    ds_ptr->dhcp_opts_len = (UINT8)device->dev_addr.dev_dhcp_opts_length;
                }

                else
                {
                    NLOG_Error_Log("Cannot find matching device", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    continue;
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                status = DHCP_Send_Request(ds_ptr, socketd, dv_name);
                
                if (status == NU_NO_ACTION)
                {
                    /* The DHCP Server did not respond. Log the error. */
                    NLOG_Error_Log("Error occurred - DHCP Server not responsive",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                    
                    /* Update the DHCP Renew timer. */               
                    if (DHCP_Update_Timers(dv_name, event, time) != NU_SUCCESS)
                    {
                        /* Timer could not be set, log error. This is recoverable
                          since the Rebind timer is still set. */
                        NLOG_Error_Log("Error occurred - DHCP Renew timer not set",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                          
                    }
                    
                }
                else if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Error occurred while sending a DHCPREQUEST packet",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    /* If the renew request was NAK'd by the DHCP Server,
                     * restart the DHCP state machine.
                     */
                    if (status == NU_DHCP_REQUEST_FAILED)
                    {
                        status = NU_Close_Socket(socketd);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to close socket for DHCP", NERR_SEVERE,
                                           __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }

                        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
                        {
                            /* Get a pointer to the device */
                            device = DEV_Get_Dev_By_Name(dv_name);

                            if (device)
                            {
                                /* Unset the timers */
                                TQ_Timerunset(DHCP_Renew, TQ_CLEAR_EXACT,
                                              (UNSIGNED)device->dev_index, 0);

                                TQ_Timerunset(DHCP_Rebind, TQ_CLEAR_EXACT,
                                              (UNSIGNED)device->dev_index, 0);

                                TQ_Timerunset(DHCP_New_Lease, TQ_CLEAR_EXACT,
                                              (UNSIGNED)device->dev_index, 0);

                                /* Zero out the IP address being maintained by DHCP */
                                device->dev_addr.dev_dhcp_addr = 0;

                                /* Remove the IP address from the interface. */
                                if (DEV4_Delete_IP_From_Device(device, dev_addr_entry)
                                    != NU_SUCCESS)
                                    NLOG_Error_Log("Failed to delete route",
                                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Failed to release semaphore",
                                                   NERR_SEVERE, __FILE__, __LINE__);

                                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                                   NU_Current_Task_Pointer(), NU_NULL);
                                }
                            }

                            else
                            {
                                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Failed to release semaphore",
                                                   NERR_SEVERE, __FILE__, __LINE__);

                                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                                   NU_Current_Task_Pointer(), NU_NULL);
                                }

                                NLOG_Error_Log("Cannot find matching device", NERR_FATAL,
                                               __FILE__, __LINE__);

                                continue;
                            }

                            /* Restart the DHCP State Machine */
                            status = NU_Dhcp(ds_ptr, dv_name);

                            if (status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to restart DHCP state machine",
                                               NERR_FATAL, __FILE__, __LINE__);

                                NET_DBG_Notify(status, __FILE__, __LINE__,
                                               NU_Current_Task_Pointer(), NU_NULL);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                           __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                            break;
                        }
                    }
                }
                else
                {
                    /* Obtain the TCP semaphore to protect the stack global variables */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                        break;
                    }

                    /* get a pointer to the interface structure. */
                    device = DEV_Get_Dev_By_Name(dv_name);

                    if (device)
                    {
                        /* The lease was successfully extended. Return to the
                           bound state. */
                        device->dev_addr.dev_dhcp_state = DHCP_BOUND_STATE;

                        /* Clear the timer event for rebinding. */
                        TQ_Timerunset(DHCP_Rebind, TQ_CLEAR_EXACT,
                                      (UNSIGNED)device->dev_index, 0);

                        /* Re-initialize the timers. */
                        DHCP_Init_Timers(device);
                    }

                    else
                    {
                        NLOG_Error_Log("Cannot find matching device", NERR_FATAL,
                                       __FILE__, __LINE__);
                    }

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

            }
            else if (event == DHCP_Rebind)
            {
                /* Obtain the TCP semaphore to protect the stack global variables */
                status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    break;
                }

                /* get a pointer to the interface structure. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                {
                    ds_ptr->dhcp_opts = (UINT8 *)device->dev_addr.dev_dhcp_options;
                    ds_ptr->dhcp_opts_len = (UINT8)device->dev_addr.dev_dhcp_opts_length;

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

                else
                {
                    NLOG_Error_Log("Cannot find matching device", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    continue;
                }

                status = DHCP_Send_Request(ds_ptr, socketd, dv_name);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Error occurred while sending a DHCPREQUEST packet",
                                        NERR_FATAL, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    /* If a timer event was not created, then the lease has run
                       out. Bring the interface down. */
                    if (DHCP_Update_Timers(dv_name, event, time) != NU_SUCCESS)
                    {
                        /* Grab the NET semaphore. */
                        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_FATAL,
                                                __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);

                            break;
                        }
                        else
                        {
                            /* get a pointer to the interface structure. */
                            device = DEV_Get_Dev_By_Name(dv_name);

                            if (device)
                            {
                                device->dev_addr.dev_dhcp_addr = 0;

                                /* Delete the IP address from the device */
                                if (DEV4_Delete_IP_From_Device(device, dev_addr_entry)
                                    != NU_SUCCESS)
                                    NLOG_Error_Log("Failed to delete route",
                                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                                /* Notify the sockets layer to resume all tasks
                                 * using this IP address and return an error to
                                 * the user.
                                 */
                                DEV_Resume_All_Open_Sockets();

                                if (TQ_Timerset(DHCP_New_Lease,
                                                (UNSIGNED)device->dev_index,
                                                0, 0) != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Failed to set DHCP_New_Lease timer",
                                                   NERR_SEVERE, __FILE__, __LINE__);

                                    NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                                   NU_Current_Task_Pointer(), NU_NULL);
                                }
                            }

                            else
                                NLOG_Error_Log("Cannot find matching device",
                                               NERR_SEVERE, __FILE__, __LINE__);

                            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to release semaphore",
                                               NERR_SEVERE, __FILE__, __LINE__);

                                NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                               NU_Current_Task_Pointer(), NU_NULL);
                            }
                        }
                    }
                }
                else
                {
                    /* Obtain the TCP semaphore to protect the stack global variables */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);

                        break;
                    }

                    /* get a pointer to the interface structure. */
                    device = DEV_Get_Dev_By_Name(dv_name);

                    if (device)
                    {
                        /* The lease was successfully extended. Return to the
                           bound state. */
                        device->dev_addr.dev_dhcp_state = DHCP_BOUND_STATE;

                        /* Re-initialize the timers. */
                        DHCP_Init_Timers(device);
                    }

                    else
                    {
                        NLOG_Error_Log("Cannot find matching device", NERR_SEVERE,
                                       __FILE__, __LINE__);
                    }

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

            }
            else if (event == DHCP_New_Lease)
            {
                /*  Clear out the structure.  */
                UTL_Zero(ds_ptr, sizeof(ds_ptr));

                /* Get the options specified for the original lease. */
                ds_ptr->dhcp_opts = (UINT8 *)device->dev_addr.dev_dhcp_options;
                ds_ptr->dhcp_opts_len = (UINT8)device->dev_addr.dev_dhcp_opts_length;

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                status = NU_Dhcp(ds_ptr, dv_name);

                if (status != NU_SUCCESS)
                {
                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    /* time delay for DHCP_New_Lease timer to go off in a random time
                       between 0-30 seconds */
                    rand_num = (UTL_Rand())%(SCK_Ticks_Per_Second/2);

                    /* Obtain the TCP semaphore to protect the stack global variables */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Could not grab semaphore", NERR_FATAL,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);

                        break;
                    }

                    /* get a pointer to the interface structure. */
                    device = DEV_Get_Dev_By_Name(dv_name);

                    if (device)
                    {
                        if (TQ_Timerset(DHCP_New_Lease, (UNSIGNED)device->dev_index,
                                       rand_num, 0) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to set DHCP_New_Lease timer",
                                           NERR_SEVERE, __FILE__, __LINE__);
                    }

                    else
                        NLOG_Error_Log("Cannot find matching device", NERR_SEVERE,
                                       __FILE__, __LINE__);

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }
            }

            /* Release the IP address because the lease has expired and the
             * DHCP Server would not renew the address.
             */
            else if (event == DHCP_Release)
            {
                /* Get a pointer to the interface structure. */
                device = DEV_Get_Dev_By_Name(dv_name);

                if (device)
                {
                    /* Clear the DHCP address. */
                    device->dev_addr.dev_dhcp_addr = 0;

                    /* Delete the IP address from the device */
                    if (DEV4_Delete_IP_From_Device(device, dev_addr_entry)
                        != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to delete address",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Notify the sockets layer to resume all tasks using
                     * this IP address.
                     */
                    DEV_Resume_All_Open_Sockets();

                    /* Set an event to attempt to get a new IP address. */
                    if (TQ_Timerset(DHCP_New_Lease,
                                    (UNSIGNED)device->dev_index,
                                    0, 0) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to set DHCP_New_Lease timer",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

                else
                {
                    NLOG_Error_Log("Cannot find matching device",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }
            }

            /* socket is not open for New Lease State */
            if ( ((event == DHCP_Renew) && (status != -1)) ||
                 (event == DHCP_Rebind))
            {
                status = NU_Close_Socket(socketd);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to close socket for DHCP", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }
            }
        }

        else
        {
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);

                NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }

            NLOG_Error_Log("Received invalid device index", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Deallocate DHCP structure. */
    if (ds_ptr != NU_NULL)
    {
        NU_Deallocate_Memory(ds_ptr);
    }
#endif

    /* Return to user mode */
    NU_USER_MODE();

} /* DHCP_Event_Handler */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Queue_Event
*
*   DESCRIPTION
*
*       This function simply puts a DHCP event onto a queue from which it
*       will be retrieved by DHCP_Event_Handler.
*
*   INPUTS
*
*       *device                 A pointer to the device to which this event
*                               applies.
*       event                   The type of event.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID DHCP_Queue_Event(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat)
{
    UNSIGNED            queue_message[DHCP_Q_MESSAGE_SIZE];
    STATUS              status;

    UNUSED_PARAMETER(ext_dat);

    queue_message[0] = (UNSIGNED)event;
    queue_message[1] = dat;

    status = NU_Send_To_Queue(DHCP_Event_Queue, queue_message,
                              DHCP_Q_MESSAGE_SIZE, (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Error occurred during sending DHCP message to queue", NERR_FATAL,
                            __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }

} /* DHCP_Queue_Event */

/****************************************************************************
*
*   FUNCTION
*
*       DHCP_Initialize
*
*   DESCRIPTION
*
*       DHCP_Initialize is called the first time the DHCP is invoked by an
*       application. It creates the permanent resources that will be required
*       by DHCP.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       status                  Failure
*
******************************************************************************/
STATUS DHCP_Initialize(VOID)
{
    CHAR        HUGE *memory, HUGE *ptr;
    STATUS      status;
    INT         i, j = 1;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    DHCP_Server_Port = IPPORT_DHCPS;
    DHCP_Client_Port = IPPORT_DHCPC;

    /* Allocate memory for the permanent DHCP resources. Each memory allocation
       has some overhead so allocate all of the memory at once and break it up.
       Memory is being allocated for the task control block, the task stack,
       the queue control block and the queue (size of each message * number of
       messages * sizeof(unsigned)). */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&memory,
                    sizeof (NU_TASK) + DHCP_STACK_SIZE + sizeof(NU_QUEUE) +
                    ( DHCP_Q_MESSAGE_SIZE * DHCP_Q_SIZE * sizeof(UNSIGNED)),
                    (UNSIGNED)NU_NO_SUSPEND);

    if (status == NU_SUCCESS)

#else
    /* Assign memory to permanent DHCP resources */
    memory = DHCP_Initialize_Memory;
#endif

    {
        /* Initialize the backoff array. */
        /* Store 1, 2, 4, 8, 16, 32, 64, 64, 64, ... into DHCP_Backoff[] array */
        for (i = 0; i < (DHCP_MAX_BACKOFFS + 1); i++)
        {
            DHCP_Backoff[i] = j;

            if (j != 64)
                j *= 2;
        }

        /* Clear the block of memory. */
        UTL_Zero(memory, sizeof (NU_TASK) + DHCP_STACK_SIZE + sizeof(NU_QUEUE)
                   + (DHCP_Q_MESSAGE_SIZE * DHCP_Q_SIZE * sizeof(UNSIGNED)) );

        /* Point a scratch pointer at the memory block allocated. */
        ptr = memory;

        /* Break off a chunk of memory for the Task control block. */
        DHCP_Event_Handler_Tcb = (NU_TASK *)ptr;
        ptr += sizeof(NU_TASK);

        /* Break off a chunk of memory for the Queue control block. */
        DHCP_Event_Queue = (NU_QUEUE *)ptr;
        ptr += sizeof(NU_QUEUE);

        /* Create the DHCP queue. */
        status = NU_Create_Queue(DHCP_Event_Queue, "DHCPQ", ptr,
                                 (DHCP_Q_SIZE * DHCP_Q_MESSAGE_SIZE),
                                 NU_FIXED_SIZE, DHCP_Q_MESSAGE_SIZE, NU_FIFO);
        if (status == NU_SUCCESS)
        {
            /* Point past the memory used for the queue. */
            ptr += (DHCP_Q_MESSAGE_SIZE * DHCP_Q_SIZE * sizeof(UNSIGNED));


            /* Create the DHCP task. */
            status = NU_Create_Task(DHCP_Event_Handler_Tcb, "DHCP",
                                DHCP_Event_Handler, 0, NU_NULL, ptr,
                                DHCP_STACK_SIZE, DHCP_PRIORITY, DHCP_TIME_SLICE,
                                DHCP_PREEMPT, NU_NO_START);


            /* If the Task creation failed, cleanup. */
            if (status != NU_SUCCESS)
            {
                if (NU_Delete_Queue(DHCP_Event_Queue) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to delete DHCP Event Queue",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                if (NU_Deallocate_Memory(memory) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory",  NERR_SEVERE,
                                   __FILE__, __LINE__);
#endif
            }

            else
            {
                /* Start the task */
                if (NU_Resume_Task(DHCP_Event_Handler_Tcb) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            /* Register the DHCP events. */
            if ( (EQ_Register_Event(DHCP_Queue_Event, &DHCP_Renew) == NU_SUCCESS) &&
                 (EQ_Register_Event(DHCP_Queue_Event, &DHCP_Rebind) == NU_SUCCESS) &&
                 (EQ_Register_Event(DHCP_Queue_Event, &DHCP_New_Lease) == NU_SUCCESS) &&
                 (EQ_Register_Event(DHCP_Queue_Event, &DHCP_Release) == NU_SUCCESS) )
            {
                status = NU_SUCCESS;
                NET_Initialized_Modules |= DHCP_CLIENT_MODULE;
            }

            else
                status = NU_DHCP_INIT_FAILED;
        }
        else
        {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* If the task creation failed, cleanup. */
            if (NU_Deallocate_Memory(memory) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);
#endif
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* DHCP_Initialize */

#endif
#endif
