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
*       nlog.c
*
*   COMPONENT
*
*       Nucleus NET error and protocol parameter logging component.
*
*   DESCRIPTION
*
*       This file will hold all the Nucleus routines for logging the
*       Nucleus NET errors and protocol parameters.
*
*   DATA STRUCTURES
*
*       NLOG_ENTRY
*
*   FUNCTIONS
*
*       NLOG_Comm_Task
*       NLOG_Error_Log
*       NLOG_Error_Put
*       NLOG_Init
*       NLOG_IP_Info
*       NLOG_IP_Info_Put
*       NLOG_TCP_Info
*       NLOG_TCP_Info_Put
*       NLOG_UDP_Info
*       NLOG_UDP_Info_Put
*       NLOG_IP6_Info
*       NLOG_IP6_Info_Put
*       NLOG_ARP_Info
*       NLOG_ARP_Info_Put
*       NLOG_ICMP_Info
*       NLOG_ICMP_Info_Put
*       NLOG_ICMP6_Info
*       NLOG_ICMP6_Info_Put
*       NLOG_Info_Print
*       NLOG_Time_Puts
*       NLOG_Error_Log_IS
*       NLOG_Interrupt_Safe_Logging_HISR
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"             /* Nucleus NET interface, including
                                            Sockets API, TFTP Client and RIP2. */
/* Index into array of log entries. Points to where the next entry will be stored. */
INT NLOG_Avail_Index;

/* Array to place the logged info */
NLOG_ENTRY NLOG_Entry_List[NLOG_MAX_ENTRIES];

/* Create a global counter to keep track of internal NLOG errors. */
UINT16  NLOG_Internal_Error;

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

NU_TASK         NLOG_Comm_Task_CB;
NU_QUEUE        NLOG_Msg_Queue;
NU_HISR         NLOG_IS_Logging_HISR;
NLOG_ENTRY      *NLOG_IS_Entry_List[NLOG_MAX_ENTRIES];
INT             NLOG_IS_Avail_Index;

VOID NLOG_Interrupt_Safe_Logging_HISR(VOID);

#endif

VOID NLOG_Comm_Task(UNSIGNED argc, VOID *argv);
CHAR *NLOG_Error_Put (CHAR *, STATUS, const CHAR *, INT, CHAR *);
CHAR *NLOG_IP_Info_Put(IPLAYER *, UINT8, CHAR *);
CHAR *NLOG_TCP_Info_Put(TCPLAYER *, INT16, UINT8, CHAR *);
CHAR *NLOG_UDP_Info_Put(UDPLAYER *, UINT8, CHAR *);
CHAR *NLOG_ARP_Info_Put(ARP_LAYER *, UINT8, CHAR *);
CHAR *NLOG_ICMP_Info_Put(ICMP_LAYER *, UINT8, CHAR *);
VOID NLOG_Info_Print(const CHAR *);

#if (INCLUDE_IPV6 == NU_TRUE)
CHAR *NLOG_IP6_Info_Put(IP6LAYER *, UINT8, CHAR *);
CHAR *NLOG_ICMP6_Info_Put(ICMP6_LAYER *, UINT8, CHAR *);
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for Comm task */
CHAR    NET_NLOG_Comm_Memory[NLOG_COMM_STACK_SIZE];

/* Declare memory for logging message queue */
CHAR    NET_NLOG_Logging_Memory[NLOG_MAX_ENTRIES * sizeof(UNSIGNED)];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
/* Declare memory for the HISR */
CHAR    NLOG_IS_Logging_Hisr_Memory[NET_MAX_HISR_SIZE];
#endif

#endif      /* INCLUDE_STATIC_BUILD */

/*************************************************************************
*
*   FUNCTION
*
*       NLOG_Init
*
*   DESCRIPTION
*
*       This is an initialization routine. It creates the logging
*       message queue
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_Init(VOID)
{
#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    STATUS              status;
    VOID                *pointer;
    CHAR HUGE           *ptr;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Initialize the NET info logging array indexing variable */
    NLOG_Avail_Index = 0;

    /* Initialize the NLOG internal error counter */
    NLOG_Internal_Error = 0;


#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    NLOG_IS_Avail_Index = 0;

    /* Create Comm_Task */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                NLOG_COMM_STACK_SIZE, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        /* Switch back to user mode. */
        NU_USER_MODE();

        return;
    }

#else

    /* Assign memory to the Comm Task */
    pointer = (VOID*) NET_NLOG_Comm_Memory;

#endif

    status = NU_Create_Task(&NLOG_Comm_Task_CB, "comm_tsk",
                            NLOG_Comm_Task, 0, NU_NULL, pointer,
                            NLOG_COMM_STACK_SIZE, NLOG_COMM_PRIORITY,
                            (unsigned)NLOG_COMM_TIME_SLICE,
                            NLOG_COMM_PREEMPT, NU_NO_START);

    if (status != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        /* Switch back to user mode. */
        NU_USER_MODE();

        return;
    }

    /* Start the task */
    NU_Resume_Task(&NLOG_Comm_Task_CB);

    /* Create a logging message queue.  */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                (NLOG_MAX_ENTRIES * NLOG_QUEUE_ELEMENT_SIZE *
                                sizeof(UNSIGNED)), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        /* Switch back to user mode. */
        NU_USER_MODE();

        return;
    }

#else

    /* Assign memory to the logging message queue */
    pointer = (VOID*) NET_NLOG_Logging_Memory;

#endif

    status = NU_Create_Queue(&NLOG_Msg_Queue, "NLOG_Queue", pointer,
                             (NLOG_MAX_ENTRIES * NLOG_QUEUE_ELEMENT_SIZE),
                             NU_FIXED_SIZE, NLOG_QUEUE_ELEMENT_SIZE,
                             NU_FIFO);

    if (status != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        /* Switch back to user mode. */
        NU_USER_MODE();

        return;
    }

#endif

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Allocate memory for the Interrupt-Safe Logging HISR. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ptr,
                                NLOG_IS_LOGGING_HISR_SIZE, NU_NO_SUSPEND);

    /* Did we get the memory? */
    if (status == NU_SUCCESS)
#else
    /* Assign memory to the HISR */
    ptr = NLOG_IS_Logging_Hisr_Memory;
#endif

    {
        /* Create the HISR. */
        NU_Create_HISR(&NLOG_IS_Logging_HISR, "nlog_is",
                       NLOG_Interrupt_Safe_Logging_HISR,
                       2, ptr, NLOG_IS_LOGGING_HISR_SIZE);
    }
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return;

} /* NLOG_Init */

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_Comm_Task
*
*   DESCRIPTION
*
*
*   INPUTS
*
*       *message                Pointer to the char string message about
*                                   the error that occurred
*       stat                    Status flag to store for error severity.
*       *file                   Pointer to the current filename in which
*                               the error happened.
*       line                    Line number in the file where the error
*                               happened.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_Comm_Task(UNSIGNED argc, VOID *argv)
{
    INT                 socketd;
    struct addr_struct  servaddr;
    STATUS              status;
    INT32               bytes_sent;
    INT16               unused_parm = 0;

    UINT16              msg_size;
    UNSIGNED            actsize;
    CHAR                *msg_string;
    UINT32              first_message, message_count;

    /* No compilation warnings allowed.  */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* open a connection via the socket interface */
    socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, 0);

    if (socketd >= 0)
    {
        servaddr.id.is_ip_addrs[0] = NLOG_CLIENT_IP_0;
        servaddr.id.is_ip_addrs[1] = NLOG_CLIENT_IP_1;
        servaddr.id.is_ip_addrs[2] = NLOG_CLIENT_IP_2;
        servaddr.id.is_ip_addrs[3] = NLOG_CLIENT_IP_3;

        /* fill in a structure with the server address */
        servaddr.family     = NU_FAMILY_IP;
        servaddr.port       = NLOG_PORT;
        servaddr.name       = "NLOG_Serv";

        /* Get the initial time value. */
        first_message = NU_Retrieve_Clock();

        /* Initialize the message count to 0. */
        message_count = 0;

        for (;;)
        {
            /* If there is a message on the message queue, send it out
             * to the specified client.  If no messages are present,
             * suspend until there is one.
             */
            status = NU_Receive_From_Queue(&NLOG_Msg_Queue,
                                           (VOID *)&msg_string, 1,
                                           &actsize, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* If enough time has passed since the first message
                 * in this timed count of messages was transmitted,
                 * reset the message timer and the message count.
                 */
                if (TQ_Check_Duetime(first_message + NLOG_TIME_THRESH) <= 0)
                {
                    message_count = 0;
                    first_message = NU_Retrieve_Clock();
                }

                /* If we have not exceeded the number of messages that
                 * can be transmitted during this time period.
                 */
                if (message_count < NLOG_COUNT_THRESH)
                {
                    /* Increment the count of the number of messages
                     * that have been attempted to be transmitted.  Do
                     * not wait to check the status of the transmission
                     * since the purpose of this is to prevent infinite
                     * messages being sent when NU_Send_To generates
                     * error messages.
                     */
                    message_count++;

                    /* Determine the size of the string to send out */
                    msg_size = (UINT16)(strlen(msg_string));

                    /* Send the data out to the specified client */
                    bytes_sent = NU_Send_To(socketd, msg_string, msg_size,
                                            0, &servaddr, unused_parm);

                    if ( (bytes_sent < 0) ||
                        ((UINT16)bytes_sent != msg_size) )
                    {
                        /* Increment the internal error counter */
                        NLOG_Internal_Error++;
                    }
                }
            }

            else
            {
                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }
    } /* end successful NU_Socket */

} /* NLOG_Comm_Task */

#endif

#if ( (INCLUDE_NET_ERROR_LOGGING == NU_TRUE) || (PRINT_NET_ERROR_MSG == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_Error_Log
*
*   DESCRIPTION
*
*       This routine will handle storing the current error number into the
*       error structure.  The current system time will also be stored into
*       the structure.  This routine will handle searching for the next
*       available location, set the next location to avail, and then set
*       its own location to TRUE for being used.  The data will then be
*       stored into the structure.
*
*       Load passed in and calculated information into the
*       NLOG_Error_List array, and also will update the value of the
*       NLOG_Avail_Index.
*
*   INPUTS
*
*       *message                Pointer to the char string message about
*                                   the error that occurred
*       stat                    Status flag to store for error severity.
*       *file                   Pointer to the current filename in which
*                               the error happened.
*       line                    Line number in the file where the error
*                               happened.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_Error_Log(CHAR *message, STATUS stat, const CHAR *file, INT line)
{
    NLOG_ENTRY      *err_list_ptr;
    CHAR            *err_str,
                    error_buffer[NLOG_MAX_BUFFER_SIZE];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&error_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Convert the error info into an ASCII string */
    err_str = NLOG_Error_Put(message, stat, file, line, &error_buffer[0]);

    /* load the needed information into the current array location */
    err_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(err_list_ptr->log_msg, err_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&err_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#if (PRINT_NET_ERROR_MSG == NU_TRUE)
    /* Print the info */
    NLOG_Info_Print (&error_buffer[0]);
#endif

} /* NLOG_Error_Log */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_Error_Put
*
*   DESCRIPTION
*
*       Prints out an error message along with a status, file name, and
*       line number the error occurred on.
*
*   INPUTS
*
*       *message                A pointer to a text message
*       status                  Status code, found in NLOG.H
*       *file                   __FILE__ macro supplied by tools
*       line                    __LINE__ macro supplied by tools
*       *err_buff               A pointer to where the string will be
*                               written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_Error_Put(CHAR *message, STATUS status, const CHAR *file,
                     INT line, CHAR *err_buff)
{
    CHAR            char_buff[6],
                    *char_ptr,
                    name[NU_MAX_NAME];
    UNSIGNED        schd_count,
                    time_slice,
                    stack_size,
                    min_stack;
    NU_TASK         *task;
    STATUS          ret_stat;
    VOID            *stack_base;
    OPTION       priority,
                    preempt;
    DATA_ELEMENT    task_stat;
    INT             message_length;

    NLOG_Time_Puts(err_buff);

    strcat(err_buff, "***ERROR***: \r\n");

    /* Get the current task pointer */
    task = NU_Current_Task_Pointer();

    memset(name, 0, sizeof(name));

    /* If the task pointer is not NU_NULL, get the task name. */
    if (task != NU_NULL)
    {
        /* Get the task info */
        ret_stat = NU_Task_Information(task, &name[0], &task_stat, &schd_count,
                                       &priority, &preempt, &time_slice,
                                       &stack_base, &stack_size, &min_stack);
    }

    /* Otherwise, this routine was called from a LISR or HISR.
     * Hardcode the task name to "ISR."
     */
    else
    {
        strcpy(name, "ISR");
        ret_stat = NU_SUCCESS;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Copy the task name into the buffer */
        strcat(err_buff, &name[0]);
    }

    /* Copy the file name into the buffer */
    strcat(err_buff, " File:");

    strcat(err_buff, (CHAR *)file);

    /* Copy the line number into the buffer */
    strcat(err_buff, " (");

    char_ptr = NU_ITOA(line, &char_buff[0], 10);

    strcat(err_buff, char_ptr);

    strcat(err_buff, ")\r\n stat: ");

    /* Copy the type of error that occurred into the buffer */
    switch (status)
    {
        case NERR_INFORMATIONAL:
        {
            strcat(err_buff, "Informational");
            break;
        }

        case NERR_RECOVERABLE:
        {
            strcat(err_buff, "Recoverable");
            break;
        }

        case NERR_SEVERE:
        {
            strcat(err_buff, "Severe");
            break;
        }

        case NERR_FATAL:
        {
            strcat(err_buff, "Fatal");
            break;
        }

        default:
            break;
    }

    /* Append the string, "Msg:" to the buffer. */
    strcat(err_buff, ". Msg:");

    /* Determine the amount of data left in the buffer, including the
     * NULL termination required at the end of the message.
     */
    message_length = NLOG_MAX_BUFFER_SIZE - strlen(err_buff) - 3;

    /* If there is room in the buffer for the message, add the message. */
    if (message_length > 0)
    {
        /* If the message is too big to fit in the buffer. */
        if (strlen(message) > message_length)
        {
            /* Truncate the message so it does not overrun the buffer
             * length.
             */
            message[message_length] = 0;
        }

        /* Copy the error message into the buffer */
        strcat(err_buff, message);
        strcat(err_buff, "\r\n");
    }

    return (err_buff);

} /* NLOG_Error_Put */
#endif

#if ( ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_IP_Info
*
*
*   INPUTS
*
*       *ip_info                Pointer to the IP layer information
*       flags                   Information about the IP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_IP_Info(IPLAYER *ip_info, UINT8 flags)
{
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if (INCLUDE_IP_INFO_LOGGING == NU_TRUE)
    NLOG_ENTRY      *ip_list_ptr;
#endif  /* #if (INCLUDE_IP_INFO_LOGGING */

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif  /* #if (NLOG_INFO_SEND */

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the IP header info so that it can be stored and/or
     * printed.
     */
    info_str = NLOG_IP_Info_Put(ip_info, flags, &info_buffer[0]);

#if (INCLUDE_IP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    ip_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(ip_list_ptr->log_msg, info_str);

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&ip_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif /* #if (NLOG_INFO_SEND */
#endif /* #if (INCLUDE_IP_INFO_LOGGING */

#if (PRINT_IP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif  /* #if (PRINT_IP_MSG */

} /* NLOG_IP_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_IP_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *ip_info                A pointer to the IP header info
*       flags                   Information about the IP packet
*       *ip_buff                A pointer to the buffer where the string
*                               will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_IP_Info_Put(IPLAYER *ip_ptr, UINT8 flags, CHAR *ip_buff)
{
    CHAR        char_buff[16],  /* Max length of an IPv4 address, including
                                   delimiter and null terminator */
                *char_ptr;
    STATUS      status;

    NLOG_Time_Puts(ip_buff);

    /* Signify that this is the IP header */
    strcat(ip_buff, "IP Header:");

    /* Signify whether or not the packet was received or transmitted */
    if ((flags & NLOG_RX_PACK) != 0)
        strcat(ip_buff, "Rx");

    else if ((flags & NLOG_TX_PACK) != 0)
        strcat(ip_buff, "Tx");

    strcat(ip_buff, "\r\n");

    /* IP version */
    strcat(ip_buff, " Ver: ");

    /* Convert the version number to ASCII */
    char_ptr = NU_ITOA((ip_ptr->ip_versionandhdrlen >> 4), &char_buff[0], 10);

    /* Copy the version number into the buffer */
    strcat(ip_buff, char_ptr);

    /* Type of service */
    strcat(ip_buff, " TOS: ");

    /* Convert the service number to ASCII */
    char_ptr = NU_ITOA(ip_ptr->ip_service, &char_buff[0], 10);

    /* Copy the service number into the buffer */
    strcat(ip_buff, char_ptr);

    /* Total length */
    strcat(ip_buff, " TLen: ");

    /* Convert the total length into ASCII */
    char_ptr = NU_ITOA((GET16(ip_ptr, IP_TLEN_OFFSET)), &char_buff[0], 10);

    /* Copy the total length into the buffer */
    strcat(ip_buff, char_ptr);

    /* Identifier */
    strcat(ip_buff, " Ident: ");

    /* Convert the identifier into ASCII */
    char_ptr = NU_ITOA((GET16(ip_ptr, IP_IDENT_OFFSET)), &char_buff[0], 10);

    /* Copy the identifier into the buffer */
    strcat(ip_buff, char_ptr);

    /* Time to life */
    strcat(ip_buff, " TTL: ");

    /* Convert the TTL to ASCII */
    char_ptr = NU_ITOA(ip_ptr->ip_ttl, &char_buff[0], 10);

    /* Copy the TTL into the buffer */
    strcat(ip_buff, char_ptr);

    /* Protocol */
    strcat(ip_buff, "\r\n Prot: ");

    /* Convert the protocol number into ASCII */
    char_ptr = NU_ITOA(ip_ptr->ip_protocol, &char_buff[0], 10);

    /* Copy the protocol number into the buffer */
    strcat(ip_buff, char_ptr);

    /* Provide the name of the protocol */
    switch (ip_ptr->ip_protocol)
    {
        case IP_UDP_PROT:
            strcat(ip_buff, "(UDP)");
            break;

        case IP_TCP_PROT:
            strcat (ip_buff, "(TCP)");
            break;

        case IP_ICMP_PROT:
            strcat (ip_buff, "(ICMP)");
            break;

        case IP_IGMP_PROT:
            strcat(ip_buff, "(IGMP)");
            break;

        case IPPROTO_IPV6:
            strcat(ip_buff, "(IPv6)");
            break;

        default:
            break;
    }

    /* Destination IP addr */
    strcat(ip_buff, " DestIP: ");

    /* Convert the destination IP addr to ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP, &ip_ptr->ip_dest, &char_buff[0], 16);

    /* Append the destination IP to the string */
    if (status == NU_SUCCESS)
    {
        strcat(ip_buff, &char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    /* Source IP addr */
    strcat(ip_buff, "\r\n SrcIP: ");

    /* Convert the source IP addr to ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP, &ip_ptr->ip_src, &char_buff[0], 16);

    if (status == NU_SUCCESS)
    {
        strcat(ip_buff,&char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    return (ip_buff);

} /* NLOG_IP_Info_Put */
#endif  /* #if ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE)) */

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
/*************************************************************************
*
*   FUNCTION
*
*       NLog_TCP_Info
*
*
*   INPUTS
*
*       *tcp_info               Pointer to the TCP layer information
*       tcp_len                 Length of the data contained in the
*                               packet
*       flags                   Information about the TCP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_TCP_Info(TCPLAYER *tcp_info, INT16 tcp_len, UINT8 flags)
{
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if (INCLUDE_TCP_INFO_LOGGING == NU_TRUE)
    NLOG_ENTRY      *tcp_list_ptr;
#endif

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the TCP header info for storing and printing */
    info_str = NLOG_TCP_Info_Put(tcp_info, tcp_len, flags, &info_buffer[0]);

#if (INCLUDE_TCP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    tcp_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(tcp_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&tcp_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_TCP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

} /* NLOG_TCP_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_TCP_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *tcp_info                A pointer to the TCP header info
*       tcp_len                  total TCP data length, including header
*       flags                    Information about the TCP packet
*       *tcp_buff                Buffer where the string will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_TCP_Info_Put(TCPLAYER *tcp_info, INT16 tcp_len, UINT8 flags,
                        CHAR *tcp_buff)
{
    CHAR    char_buff[15], *char_ptr;

    NLOG_Time_Puts(tcp_buff);

    strcat(tcp_buff, "TCP Header:");

    /* Signify whether or not the packet was received or transmitted */
    if ((flags & NLOG_RX_PACK) != 0)
        strcat(tcp_buff, "Rx");

    else if ((flags & NLOG_TX_PACK) != 0)
        strcat(tcp_buff, "Tx");

    else if ((flags & NLOG_RETX_PACK) != 0)
        strcat(tcp_buff, "ReTx");

    strcat(tcp_buff, "\r\n");

    /* Source port */
    strcat(tcp_buff, " Src Prt: ");

    /* Convert the source port to ASCII */
    char_ptr = NU_ITOA((GET16(tcp_info, TCP_SRC_OFFSET)), &char_buff[0], 10);

    /* Copy the source port into the buffer */
    strcat(tcp_buff, char_ptr);

    /* Destination port */
    strcat(tcp_buff, " Dest Prt: ");

    /* Convert the destination port to ASCII */
    char_ptr = NU_ITOA((GET16(tcp_info, TCP_DEST_OFFSET)), &char_buff[0], 10);

    /* Copy the destination port into the buffer */
    strcat(tcp_buff, char_ptr);

    /* Sequence Number */
    strcat(tcp_buff, " Seq Num: ");

    /* Convert the sequence number to ASCII */
    char_ptr = NU_ULTOA((GET32(tcp_info, TCP_SEQ_OFFSET)), &char_buff[0], 10);

    /* Copy the sequence number into the buffer */
    strcat(tcp_buff, char_ptr);

    /* ACK Number */
    strcat(tcp_buff, " ACK Num: ");

    /* Convert the ACK number to ASCII */
    char_ptr = NU_ULTOA((GET32(tcp_info, TCP_ACK_OFFSET)), &char_buff[0], 10);

    /* Copy the ACK number into the buffer */
    strcat(tcp_buff, char_ptr);

    /* Data Length */
    strcat(tcp_buff, "\r\n TCP Len: ");

    /* Convert the data length to ASCII */
    char_ptr = NU_ITOA(tcp_len, &char_buff[0], 10);

    /* Copy the data length into the buffer */
    strcat(tcp_buff, char_ptr);

    /* Flags */
    strcat(tcp_buff, " Flags: ");

    /* Convert the flags to ASCII */
    NU_ITOA(tcp_info->tcp_flags, &char_buff[0], 10);

    if (tcp_info->tcp_flags & TACK)
        strcat(tcp_buff, "ACK");

    if (tcp_info->tcp_flags & TSYN)
        strcat(tcp_buff, "SYN");

    if (tcp_info->tcp_flags & TPUSH)
        strcat(tcp_buff, "PUSH");

    if (tcp_info->tcp_flags & TRESET)
        strcat(tcp_buff, "RESET");

    if (tcp_info->tcp_flags & TFIN)
        strcat(tcp_buff, "FIN");

    /* Window size */
    strcat(tcp_buff, " Win Size: ");

    /* Convert the window size to ASCII */
    char_ptr = NU_ITOA((GET16(tcp_info, TCP_WINDOW_OFFSET)), &char_buff[0], 10);

    /* Copy the window size into the buffer */
    strcat(tcp_buff, char_ptr);

    return (tcp_buff);

} /* NLOG_TCP_Info_Put */
#endif /*#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE)) */

#if ( (INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_UDP_Info
*
*
*   INPUTS
*
*       *udp_info                Pointer to the UDP layer information
*       flags                    Information about the UDP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_UDP_Info(UDPLAYER *udp_info, UINT8 flags)
{
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if (INCLUDE_UDP_INFO_LOGGING == NU_TRUE)
    NLOG_ENTRY      *udp_list_ptr;
#endif

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the TCP header info for storing and printing */
    info_str = NLOG_UDP_Info_Put(udp_info, flags, &info_buffer[0]);

#if (INCLUDE_UDP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    udp_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(udp_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&udp_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_UDP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

}  /* NLOG_UDP_Info */


/***************************************************************************
*
*   FUNCTION
*
*       NLOG_UDP_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *udp_info                A pointer to the UDP header info
*       flags                    Information about the UDP packet
*       *udp_buff                ptr to the buffer where the string will be
*                                copied
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_UDP_Info_Put(UDPLAYER *udp_info, UINT8 flags, CHAR *udp_buff)
{
    CHAR        char_buff[15],
                *char_ptr;

    NLOG_Time_Puts(udp_buff);

    strcat(udp_buff, "UDP Header:");

    /* Signify whether or not the packet was received or transmitted */
    if ((flags & NLOG_RX_PACK) != 0)
        strcat(udp_buff, "Rx");

    else if ((flags & NLOG_TX_PACK) != 0)
        strcat(udp_buff, "Tx");

    strcat(udp_buff, "\r\n");

    /* Source port */
    strcat(udp_buff, " Src Port: ");

    /* Convert the source port to ASCII */
    char_ptr = NU_ITOA((GET16(udp_info, UDP_SRC_OFFSET)), &char_buff[0], 10);

    /* Copy the source port into the buffer */
    strcat(udp_buff, char_ptr);

    /* Destination port */
    strcat(udp_buff, " Dest Port: ");

    /* Convert the destination port to ASCII */
    char_ptr = NU_ITOA((GET16(udp_info, UDP_DEST_OFFSET)), &char_buff[0], 10);

    /* Copy the destination port into the buffer */
    strcat(udp_buff, char_ptr);

    /* Data Length */
    strcat(udp_buff, " UDP Len: ");

    /* Convert the data length to ASCII */
    char_ptr = NU_ITOA((GET16(udp_info, UDP_LENGTH_OFFSET)), &char_buff[0], 10);

    /* Copy the data length into the buffer */
    strcat(udp_buff, char_ptr);

    return (udp_buff);

} /* NLOG_UDP_Info_Put */
#endif /* #if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE)) */

#if (INCLUDE_IPV6 == NU_TRUE)
#if ( (INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_IP6_Info
*
*
*   INPUTS
*
*       *ip_info                Pointer to the IP layer information
*       flags                   Information about the IP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_IP6_Info(IP6LAYER *ip_info, UINT8 flags)
{
    NLOG_ENTRY      *ip_list_ptr;
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the IP header info so that it can be stored and/or printed */
    info_str = NLOG_IP6_Info_Put(ip_info, flags, &info_buffer[0]);

#if (INCLUDE_IP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    ip_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(ip_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&ip_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_IP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

} /* NLOG_IP6_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_IP6_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *ip_info                A pointer to the IP header info
*       flags                   Information about the IP packet
*       *ip_buff                A pointer to the buffer where the string
*                               will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_IP6_Info_Put(IP6LAYER *ip6_ptr, UINT8 flags, CHAR *ip6_buff)
{
    CHAR        char_buff[40],  /* Max IPv6 address size, including
                                   delimiters and null terminator */
                *char_ptr;
    UINT8       ip6_array[MAX_ADDRESS_SIZE],
                i;
    STATUS      status;

    NLOG_Time_Puts(ip6_buff);

    /* Signify that this is the IPv6 header */
    strcat(ip6_buff, "IPv6 Header:");

    /* Signify whether or not the packet was received or transmitted */
    if ((flags & NLOG_RX_PACK) != 0)
        strcat(ip6_buff, "Rx");

    else if ((flags & NLOG_TX_PACK) != 0)
        strcat(ip6_buff, "Tx");

    strcat(ip6_buff, "\r\n");

    PUT32(ip6_array, 0, GET32(ip6_ptr,IP6_XXX_OFFSET));

    /* IP version */
    strcat(ip6_buff, " Ver: ");

    /* Convert the version number to ASCII */
    char_ptr = NU_ITOA((ip6_array[0] >> 4), &char_buff[0], 10);

    /* Copy the version number into the buffer */
    strcat(ip6_buff, char_ptr);

    /* Traffic Class */
    strcat(ip6_buff, " Traf Class: ");

    /* Convert the traffic class to ASCII */
    char_ptr = NU_ITOA(ip6_array[0], &char_buff[0], 10);

    /* Copy the traffic class into the buffer */
    strcat(ip6_buff, char_ptr);

    /* Flow Label */
    strcat(ip6_buff, " Flow Label: ");

    for (i = 1; i < 6; i++)
    {
        /* Convert the flow label into ASCII */
        char_ptr = NU_ITOA(ip6_array[i], &char_buff[i - 1], 10);

        /* Copy the flow label into the buffer */
        strcat(ip6_buff, char_ptr);
    }

    /* Payload Length */
    strcat(ip6_buff, " Payload Len: ");

    /* Convert the payload length into ASCII */
    char_ptr = NU_ITOA((GET16(ip6_ptr, IP6_PAYLEN_OFFSET)), &char_buff[0], 10);

    /* Copy the payload length into the buffer */
    strcat(ip6_buff, char_ptr);

    /* Hop Limit */
    strcat(ip6_buff, " Hop Limit: ");

    /* Convert the hop limit to ASCII */
    char_ptr = NU_ITOA((ip6_ptr->ip6_hop >> 4), &char_buff[0], 10);

    /* Copy the hop limit into the buffer */
    strcat(ip6_buff, char_ptr);

    /* Source IP Address */
    strcat(ip6_buff, "\r\n SrcIP: ");

    /* Convert the source address into ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP6, ip6_ptr->ip6_src, &char_buff[0], 40);

    if (status == NU_SUCCESS)
    {
        strcat(ip6_buff, &char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    /* Destination IP Address */
    strcat(ip6_buff, "\r\n DestIP: ");

    /* Convert the destination IP addr to ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP6, ip6_ptr->ip6_dest, &char_buff[0], 40);

    if (status == NU_SUCCESS)
    {
        strcat(ip6_buff, &char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    return(ip6_buff);

} /* NLOG_IP6_Info_Put */
#endif /* #if ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE)) */

#endif /* INCLUDE_IPV6 */

#if ( ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_ARP_Info
*
*
*   INPUTS
*
*       *arp_info               Pointer to the ARP layer information
*       flags                   Information about the ARP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_ARP_Info(ARP_LAYER *arp_info, UINT8 flags)
{
    NLOG_ENTRY      *arp_list_ptr;
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the IP header info so that it can be stored and/or printed */
    info_str = NLOG_ARP_Info_Put(arp_info, flags, &info_buffer[0]);

#if (INCLUDE_ARP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    arp_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(arp_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&arp_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_ARP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

} /* NLOG_ARP_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_ARP_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *arp_info               A pointer to the ARP header info
*       flags                   Information about the ARP packet
*       *arp_buff               A pointer to the buffer where the string
*                               will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_ARP_Info_Put(ARP_LAYER *arp_ptr, UINT8 flags, CHAR *arp_buff)
{
    CHAR        char_buff[16],
                *char_ptr;
    UINT8       i;
    STATUS      status;

    NLOG_Time_Puts(arp_buff);

    /* Signify that this is the IP header */
    strcat(arp_buff, "ARP Header:");

    /* Signify whether or not the packet was received or transmitted */
    if (flags & NLOG_RX_PACK)
        strcat(arp_buff, "Rx");

    else if (flags & NLOG_TX_PACK)
        strcat(arp_buff, "Tx");

    strcat(arp_buff, "\r\n");

    /* Hardware Type */
    strcat(arp_buff, " HW Typ: ");

    /* Convert the hardware type to ASCII */
    char_ptr = NU_ITOA((GET16(arp_ptr, ARP_HRD_OFFSET)), &char_buff[0], 10);

    /* Copy the hardware type number into the buffer */
    strcat(arp_buff, char_ptr);

    /* Protocol */
    strcat(arp_buff, " Protocol: ");

    /* Convert the protocol number to ASCII */
    char_ptr = NU_ITOA((GET16(arp_ptr, ARP_PRO_OFFSET)), &char_buff[0], 10);

    /* Copy the protocol number into the buffer */
    strcat(arp_buff, char_ptr);

    /* Provide the name of the protocol */
    switch (INTSWAP(arp_ptr->arp_pro))
    {
        case ARPPRO:
            strcat(arp_buff, "(IP)");
            break;

        default:
            break;
    }

    /* Operation */
    strcat(arp_buff, " Operation: ");

    /* Convert the Operation number into ASCII */
    char_ptr = NU_ITOA((GET16(arp_ptr, ARP_OP_OFFSET)), &char_buff[0], 10);

    /* Copy the operation number into the buffer */
    strcat(arp_buff, char_ptr);

    /* Provide the name of the operation */
    switch (GET16(arp_ptr, ARP_OP_OFFSET))
    {
        case ARPREQ:
            strcat(arp_buff, "(ARP Request)");
            break;

        case ARPREP:
            strcat(arp_buff, "(ARP Response)");
            break;

        case RARPQ:
            strcat(arp_buff, "(RARP Request)");
            break;

        case RARPR:
            strcat(arp_buff, "(RARP Response)");
            break;

        default:
            break;
    }

    /* Sender Hardware Address */
    strcat(arp_buff, " \r\n Sender HW Addr: ");

    for (i = 0; i < arp_ptr->arp_hln; i++)
    {
        /* Convert the sender hardware address into ASCII */
        char_ptr = NU_ITOA(arp_ptr->arp_sha[i], &char_buff[0], 10);

        /* Copy the sender hardware address into the buffer */
        strcat(arp_buff, char_ptr);

        /* Don't need to place a colon at the end of the hardware address */
        if (i < (arp_ptr->arp_hln - 1))
            strcat(arp_buff, ":");
    }

    /* Sender IP Address */
    strcat(arp_buff, " Sender IP Addr: ");

    /* Convert the sender IP address to ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP, arp_ptr->arp_spa, &char_buff[0], 16);

    if (status == NU_SUCCESS)
    {
        strcat(arp_buff, &char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    /* Target Hardware Address */
    strcat(arp_buff, "\r\n Target HW Addr: ");

    for (i = 0; i < arp_ptr->arp_hln; i++)
    {
        /* Convert the target hardware address into ASCII */
        char_ptr = NU_ITOA(arp_ptr->arp_tha[i], &char_buff[0], 10);

        /* Copy the target hardware address into the buffer */
        strcat(arp_buff, char_ptr);

        /* Don't need to place a colon at the end of the hardware address */
        if (i < (arp_ptr->arp_hln - 1))
            strcat(arp_buff, ":");
    }

    /* Target IP Address */
    strcat(arp_buff, " Target IP Addr: ");

    /* Convert the target IP address to ASCII */
    status = NU_Inet_NTOP(NU_FAMILY_IP, arp_ptr->arp_tpa, &char_buff[0], 16);

    if (status == NU_SUCCESS)
    {
        strcat(arp_buff, &char_buff[0]);
    }

    else
        NLOG_Internal_Error++;

    return (arp_buff);

} /* NLOG_ARP_Info_Put */
#endif /* #if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE)) */

#if ( ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_ICMP_Info
*
*
*   INPUTS
*
*       *arp_info               Pointer to the ARP layer information
*       flags                   Information about the ARP packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_ICMP_Info(ICMP_LAYER *icmp_info, UINT8 flags)
{
    NLOG_ENTRY      *icmp_list_ptr;
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the IP header info so that it can be stored and/or printed */
    info_str = NLOG_ICMP_Info_Put(icmp_info, flags, &info_buffer[0]);

#if (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    icmp_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(icmp_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&icmp_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_ICMP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

} /* NLOG_ICMP_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_ICMP_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *arp_info               A pointer to the ARP header info
*       flags                   Information about the ARP packet
*       *arp_buff               A pointer to the buffer where the string
*                               will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_ICMP_Info_Put(ICMP_LAYER *icmp_ptr, UINT8 flags, CHAR *icmp_buff)
{
    CHAR        char_buff[15],
                *char_ptr;

    NLOG_Time_Puts(icmp_buff);

    /* Signify that this is the IP header */
    strcat(icmp_buff, "ICMP Header:");

    /* Signify whether or not the packet was received or transmitted */
    if (flags & NLOG_RX_PACK)
        strcat(icmp_buff, "Rx");

    else if (flags & NLOG_TX_PACK)
        strcat(icmp_buff, "Tx");

    strcat(icmp_buff, "\r\n");

    /* ICMP Type */
    strcat(icmp_buff, " ICMP Type: ");

    /* Convert the type to ASCII */
    char_ptr = NU_ITOA(icmp_ptr->icmp_type, &char_buff[0], 10);

    /* Copy the type number into the buffer */
    strcat(icmp_buff, char_ptr);

    /* Provide a name for the ICMP type */
    switch (icmp_ptr->icmp_type)
    {
        case ICMP_ECHOREPLY:
            strcat(icmp_buff, "(Echo Reply)");
            break;

        case ICMP_ECHO:
            strcat(icmp_buff, "(Echo Request)");
            break;

        case ICMP_UNREACH:
            strcat(icmp_buff, "(Unreachable)");
            break;

        case ICMP_REDIRECT:
            strcat(icmp_buff, "(Redirect)");
            break;

        default:
            break;
    }

    /* Code */
    strcat(icmp_buff, " Code: ");

    /* Convert the code number to ASCII */
    char_ptr = NU_ITOA(icmp_ptr->icmp_code, &char_buff[0], 10);

    /* Copy the code number into the buffer */
    strcat(icmp_buff, char_ptr);

    /* Identifier */
    strcat(icmp_buff, " Identifier: ");

    /* Convert the identifier into ASCII */
    char_ptr = NU_ITOA((GET16(icmp_ptr, ICMP_ID_OFFSET)), &char_buff[0], 10);

    /* Copy the identifier into the buffer */
    strcat(icmp_buff, char_ptr);

    /* Sequence Number */
    strcat(icmp_buff, " Seq Number: ");

    /* Convert the sequence number into ASCII */
    char_ptr = NU_ITOA((GET16(icmp_ptr, ICMP_SEQ_OFFSET)), &char_buff[0], 10);

    /* Copy the sequence into the buffer */
    strcat(icmp_buff, char_ptr);

    return (icmp_buff);

} /* NLOG_ICMP_Info_Put */
#endif /* #if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE)) */

#if (INCLUDE_IPV6 == NU_TRUE)
#if ( (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE) )
/*************************************************************************
*
*   FUNCTION
*
*       NLOG_ICMP6_Info
*
*
*   INPUTS
*
*       *icmp6_info             Pointer to the ICMPv6 layer information
*       flags                   Information about the ICMPv6 packet
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NLOG_ICMP6_Info(ICMP6_LAYER *icmp6_info, UINT8 flags)
{
    NLOG_ENTRY      *icmp6_list_ptr;
    CHAR            *info_str,
                    info_buffer[NLOG_MAX_BUFFER_SIZE];

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    NU_TASK         *current_task;
    STATUS          ret_stat;

    /* If this message is from our logging task, just ignore it. */

    /* Get a pointer to the task that called us */
    current_task = NU_Current_Task_Pointer();

    /* If the current task pointer is the logging comm task pointer,
     * get out.
     */
    if (current_task == &NLOG_Comm_Task_CB)
        return;
#endif

    /* Initialize the buffer */
    UTL_Zero(&info_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Format the IP header info so that it can be stored and/or printed */
    info_str = NLOG_ICMP6_Info_Put(icmp6_info, flags, &info_buffer[0]);

#if (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE)

    /* load the needed information into the current array location */
    icmp6_list_ptr = &NLOG_Entry_List [NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(icmp6_list_ptr->log_msg, info_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Place a pointer to the message on the NLOG message queue */
    ret_stat = NU_Send_To_Queue(&NLOG_Msg_Queue, (VOID *)&icmp6_list_ptr,
                                1, NU_NO_SUSPEND);

    if (ret_stat != NU_SUCCESS)
    {
        /*Increment the internal error counter */
        NLOG_Internal_Error++;

        NET_DBG_Notify(ret_stat, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

#if (PRINT_ICMP_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&info_buffer[0]);

#endif

} /* NLOG_ICMP6_Info */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_ICMP6_Info_Put
*
*   DESCRIPTION
*
*       Put message, time, parameter, and task pointer.
*
*   INPUTS
*
*       *icmp6_info             A pointer to the ICMPv6 header info
*       flags                   Information about the ICMPv6 packet
*       *icmp6_buff             A pointer to the buffer where the string
*                               will be written
*
*   OUTPUTS
*
*       CHAR *                  Pointer to the buffer where the string is
*                               written.
*
****************************************************************************/
CHAR *NLOG_ICMP6_Info_Put(ICMP6_LAYER *icmp6_ptr, UINT8 flags, CHAR *icmp6_buff)
{
    CHAR        char_buff[40],
                *char_ptr;
    INT         i;
    UINT8       *ip_addr;
    STATUS      status;

    NLOG_Time_Puts(icmp6_buff);

    /* Signify that this is the IP header */
    strcat(icmp6_buff, "ICMPv6 Header:");

    /* Signify whether or not the packet was received or transmitted */
    if (flags & NLOG_RX_PACK)
        strcat(icmp6_buff, "Rx");

    else if (flags & NLOG_TX_PACK)
        strcat(icmp6_buff, "Tx");

    strcat(icmp6_buff, "\r\n");

    /* ICMP Type */
    strcat(icmp6_buff, " ICMPv6 Type: ");

    /* Convert the type to ASCII */
    char_ptr = NU_ITOA(icmp6_ptr->icmp6_type, &char_buff[0], 10);

    /* Copy the type number into the buffer */
    strcat(icmp6_buff, char_ptr);

    /* Provide a name for the ICMP type */
    switch (icmp6_ptr->icmp6_type)
    {
        case ICMP6_ECHO_REQUEST:
            strcat(icmp6_buff, "(Echo Request)");
            break;

        case ICMP6_ECHO_REPLY:
            strcat(icmp6_buff, "(Echo Reply)");
            break;

        case ICMP6_RTR_SOL:
            strcat(icmp6_buff, "(Router Sol)");
            break;

        case ICMP6_RTR_ADV:
            strcat(icmp6_buff, "(Router Advrt)");
            break;

        case ICMP6_NEIGH_SOL:
            strcat(icmp6_buff, "(Neighbor Sol)");
            break;

        case ICMP6_NEIGH_ADV:
            strcat(icmp6_buff, "(Neighbor Advrt)");
            break;

        case ICMP6_REDIRECT:
            strcat(icmp6_buff, "(Redirect)");
            break;

        case ICMP6_DST_UNREACH:
            strcat(icmp6_buff, "(Dest Unreachable)");
            break;

        case ICMP6_PACKET_TOO_BIG:
            strcat(icmp6_buff, "(Packet too big)");
            break;

        case ICMP6_TIME_EXCEEDED:
            strcat(icmp6_buff, "(Time exceeded)");
            break;

        case ICMP6_PARAM_PROB:
            strcat(icmp6_buff, "(Param Problem)");
            break;

        case ICMP6_WRUREQUEST:
            strcat(icmp6_buff, "(Who R U Request)");
            break;

        case ICMP6_WRUREPLY:
            strcat(icmp6_buff, "(Who R U Reply)");
            break;

        case ICMP6_ROUTER_RENUMBERING:
            strcat(icmp6_buff, "(Router Renum)");
            break;

        default:
            break;
    }

    /* Code */
    strcat(icmp6_buff, " Code: ");

    /* Convert the code number to ASCII */
    char_ptr = NU_ITOA(icmp6_ptr->icmp6_code, &char_buff[0], 10);

    /* Copy the code number into the buffer */
    strcat(icmp6_buff, char_ptr);

    /* Provide a name for the ICMP code */
    if (icmp6_ptr->icmp6_type == ICMP6_DST_UNREACH)
    {
        switch (icmp6_ptr->icmp6_type)
        {
            case ICMP6_DST_UNREACH_NOROUTE:
                strcat(icmp6_buff, "(No Route)");
                break;

            case ICMP6_DST_UNREACH_ADMIN:
                strcat(icmp6_buff, "(Administration)");
                break;

            case ICMP6_DST_UNREACH_ADDRESS:
                strcat(icmp6_buff, "(Unreachable Addr)");
                break;

            case ICMP6_DST_UNREACH_PORT:
                strcat(icmp6_buff, "(Unreachable Port)");
                break;

            default:
                break;
        }
    }

    else if (icmp6_ptr->icmp6_type == ICMP6_TIME_EXCEEDED)
    {
        switch (icmp6_ptr->icmp6_type)
        {
            case ICMP6_TIME_EXCD_HPLMT:
                strcat(icmp6_buff, "(Hop Limit)");
                break;

            case ICMP6_TIME_EXCD_REASM:
                strcat(icmp6_buff, "(Reassembly)");
                break;

            default:
                break;
        }
    }

    else if (icmp6_ptr->icmp6_type == ICMP6_PARAM_PROB)
    {
        switch (icmp6_ptr->icmp6_type)
        {
            case ICMP6_PARM_PROB_HEADER:
                strcat(icmp6_buff, "(Header)");
                break;

            case ICMP6_PARM_PROB_NEXT_HDR:
                strcat(icmp6_buff, "(Next Header)");
                break;

            case ICMP6_PARM_PROB_OPTION:
                strcat(icmp6_buff, "(Option)");
                break;

            default:
                break;
        }
    }

    /* If this is a echo request or response, print the identifier and
        sequence number */
    if ( (icmp6_ptr->icmp6_type == ICMP6_ECHO_REQUEST) ||
         (icmp6_ptr->icmp6_type == ICMP6_ECHO_REPLY) )
    {
        /* Identifier */
        strcat(icmp6_buff, " Identifier: ");

        /* Convert the identifier into ASCII */
        char_ptr = NU_ITOA(icmp6_ptr->icmp6_id, &char_buff[0], 10);

        /* Copy the identifier into the buffer */
        strcat(icmp6_buff, char_ptr);

        /* Sequence Number */
        strcat(icmp6_buff, " Seq Number: ");

        /* Convert the sequence number into ASCII */
        char_ptr = NU_ITOA(icmp6_ptr->icmp6_seq, &char_buff[0], 10);

        /* Copy the sequence number into the buffer */
        strcat(icmp6_buff, char_ptr);
    }

    /* If this is a neighbor solicitation, print the target addr and the
     * source link-layer addr.
     */
    if (icmp6_ptr->icmp6_type == ICMP6_NEIGH_SOL)
    {
        /* Target Address */
        strcat(icmp6_buff, "\r\n Target Addr: ");

        /* Set a pointer to the beginning of the IP address */
        ip_addr =
            (UINT8 *)(&icmp6_ptr->icmp6_type +
                      IP6_ICMP_NEIGH_SOL_TRGT_ADDRS_OFFSET);

        /* Convert the target address into ASCII */
        status = NU_Inet_NTOP(NU_FAMILY_IP6, ip_addr, &char_buff[0], 40);

        if (status == NU_SUCCESS)
        {
            strcat(icmp6_buff, &char_buff[0]);
        }

        else
            NLOG_Internal_Error++;
    }

    /* If this is a neighbor solicitation or router solicitation, print
     * the source link-layer address.
     */
    if ( (icmp6_ptr->icmp6_type == ICMP6_NEIGH_SOL) ||
         (icmp6_ptr->icmp6_type == ICMP6_RTR_SOL) )
    {
        /* Source link-layer address */
        strcat(icmp6_buff, "\r\n Src Link-Layer Addr: ");

        for (i = 0; i < DADDLEN; i++)
        {
            /* Convert the source hardware address into ASCII */
            char_ptr = NU_ITOA((*(&icmp6_ptr->icmp6_type +
                               IP6_ICMP_NEIGH_SOL_OPTIONS_OFFSET +
                               IP6_ICMP_LL_OPTION_ADDRESS_OFFSET + i)),
                               &char_buff[0], 10);

            /* Copy the sender hardware address into the buffer */
            strcat(icmp6_buff, char_ptr);

            /* Don't need to place a colon at the end of the hardware
             * address
             */
            if (i < (DADDLEN - 1))
                strcat(icmp6_buff, ":");
        }
    }

    /* If this is a neighbor advertisement, then copy the target IP
     * address and link-layer address
     */
    if (icmp6_ptr->icmp6_type == ICMP6_NEIGH_ADV)
    {
        /* Target Address */
        strcat(icmp6_buff, " \r\n Target Addr: ");

        /* Set a pointer to the beginning of the IP address */
        ip_addr =
            (UINT8 *)(&icmp6_ptr->icmp6_type +
                      IP6_ICMP_NEIGH_ADV_TRGT_ADDRS_OFFSET);

        /* Convert the target address into ASCII */
        status = NU_Inet_NTOP(NU_FAMILY_IP6, ip_addr, &char_buff[0], 40);

        if (status == NU_SUCCESS)
        {
            strcat(icmp6_buff, &char_buff[0]);
        }

        else
            NLOG_Internal_Error++;

        /* Target link-layer address */
        strcat(icmp6_buff, "\r\n Target Link-Layer Addr: ");

        for (i = 0; i < DADDLEN; i++)
        {
            /* Convert the target hardware address into ASCII */
            char_ptr = NU_ITOA((*(&icmp6_ptr->icmp6_type +
                               IP6_ICMP_NEIGH_ADV_OPTIONS_OFFSET +
                               IP6_ICMP_LL_OPTION_ADDRESS_OFFSET + i)),
                               &char_buff[0], 10);

            /* Copy the sender hardware address into the buffer */
            strcat(icmp6_buff, char_ptr);

            /* Don't need to place a colon at the end of the hardware address */
            if (i < (DADDLEN - 1))
                strcat(icmp6_buff, ":");
        }
    }

    /* If this is a router advertisement, then copy the current hop limit,
     * router lifetime, reachable time and retransmit timer values
     */
    if (icmp6_ptr->icmp6_type == ICMP6_RTR_ADV)
    {
        /* Current Hop Limit */
        strcat(icmp6_buff, " Curr Hop Limit: ");

        /* Convert the hop limit to ASCII */
        char_ptr = NU_ITOA(((icmp6_ptr->icmp6_type +
                           IP6_ICMP_RTR_ADV_CURHOPLMT_OFFSET) >> 4),
                           &char_buff[0], 10);

        /* Copy the hop limit into the buffer */
        strcat(icmp6_buff, char_ptr);

        /* Router Lifetime */
        strcat(icmp6_buff, "\r\n Router Lifetime: ");

        /* Convert the router lifetime to ASCII */
        char_ptr = NU_ITOA((icmp6_ptr->icmp6_type +
                           IP6_ICMP_RTR_ADV_RTR_LIFETIME_OFFSET),
                           &char_buff[0], 10);

        /* Copy the router lifetime into the buffer */
        strcat(icmp6_buff, char_ptr);

        /* Reachable Time */
        strcat(icmp6_buff, " Reachable Time: ");

        /* Convert the reachable time to ASCII */
        char_ptr = NU_ULTOA((UINT32)(icmp6_ptr->icmp6_type +
                            IP6_ICMP_RTR_ADV_RCHBL_TIME_OFFSET),
                            &char_buff[0], 10);

        /* Copy the reachable time into the buffer */
        strcat(icmp6_buff, char_ptr);


        /* Retransmit Timer */
        strcat(icmp6_buff, " Retrans Timer: ");

        /* Convert the retransmit timer to ASCII */
        char_ptr = NU_ULTOA((UINT32)(icmp6_ptr->icmp6_type +
                            IP6_ICMP_RTR_ADV_RTRNS_TMR_OFFSET),
                            &char_buff[0], 10);

        /* Copy the retransmit timer into the buffer */
        strcat(icmp6_buff, char_ptr);
    }

    return (icmp6_buff);

} /* NLOG_ICMP6_Info_Put */
#endif /* #if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE)) */

#endif /* INCLUDE_IPV6 */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_Info_Print
*
*   DESCRIPTION
*
*       Send the message to the user supplied output function.  If the
*       string is too long, break it up.
*
*   INPUTS
*
*       *info_string             A pointer to the data to be displayed
*
*   OUTPUTS
*
*       None.
*
****************************************************************************/
VOID NLOG_Info_Print(const CHAR *info_string)
{
    CHAR    char_buff[128];

    PUTS ("\r\n\r\n");

    /* Test to see if the string is too long.  If so, break it up. */
    while ((strlen(info_string)) > 128)
    {
        /* Copy 127 bytes into a buffer.  Must allow room for null term. */
        strncpy(char_buff, info_string, 127);

        /* Ensure that the null character has been placed at the end of the
         * string
         */
        if (char_buff[127] != 0)
            char_buff[127] = 0;

        /* Set the pointer to the location of the next byte to be printed */
        info_string += 127;

        /* Print the first part of the string. */
        PUTS (char_buff);
    }

    /* Just print the whole string */
    PUTS (info_string);

} /* NLOG_Info_Print */

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_Time_Puts
*
*   DESCRIPTION
*
*       Put current time from boot
*
*   INPUTS
*
*       *str_ptr        Pointer to where the time will be written.
*
*   OUTPUTS
*
*       None.
*
****************************************************************************/
VOID NLOG_Time_Puts(CHAR *str_ptr)
{
    INT        i;
    unsigned long  nnum = 0;
    INT     base = 1;
    CHAR    char_buff[6],
            time_char;
    unsigned long number = (NU_Retrieve_Clock() * (100 / NU_PLUS_Ticks_Per_Second));

    /*
     *  Disp format
     *  000:00:00.00
     */

    for (i=0; i < 12; i++) /* Timer buf size must be 13 */
    {
        switch (i)
        {
            case 0:
                nnum = number % 360000UL;
                number /= 360000UL;
                base = 100;

                break;

            case 3:
                number = nnum/6000;
                nnum = nnum % 6000;
                base =10;
                strcat(str_ptr, ":");
                i++;

                break;

            case 6:
                number = nnum/100;
                nnum %= 100;
                base =10;
                strcat(str_ptr, ":");
                i++;

                break;

            case 9:
                strcat(str_ptr, ".");
                i++;
                number = nnum;
                base = 10;

                break;

            default:

                break;
        }

        /* Calculate the actual digit.  */
        time_char = (CHAR)(0x30 + ((unsigned char) (number/base)));

        PUT8(&char_buff[0], 0, (UINT8)time_char);

        /* Place a null character behind the character */
        PUT8(&char_buff[1], 0, 0);

        strcat(str_ptr, &char_buff[0]);
        number %=base;
        base /=10;
    }

    strcat(str_ptr, " ");

} /* NLOG_Time_Puts */

#if ( (INCLUDE_NET_ERROR_LOGGING == NU_TRUE) || (PRINT_NET_ERROR_MSG == NU_TRUE) )

/***************************************************************************
*
*   FUNCTION
*
*       NLOG_Error_Log_IS
*
*   DESCRIPTION
*
*       This routine is the only interrupt-safe logging routine for Nucleus
*       NET. This routine will handle storing the current error number
*       into the error structure.  The current system time will also be
*       stored into the structure.  This routine will handle searching
*       for the next available location, set the next location to avail,
*       and then set its own location to TRUE for being used.  The data
*       will then be stored into the structure.
*
*       Load passed in and calculated information into the
*       NLOG_Error_List array, and also will update the value of the
*       NLOG_Avail_Index.
*
*   INPUTS
*
*       *message                Pointer to the char string message about
*                               the error that occurred
*       stat                    Status flag to store for error severity.
*       *file                   Pointer to the current filename in which
*                               the error happened.
*       line                    Line number in the file where the error
*                               happened.
*
*   OUTPUTS
*
*       None.
*
****************************************************************************/
VOID NLOG_Error_Log_IS(CHAR *message, STATUS stat, const CHAR *file,
                       INT line)
{
    NLOG_ENTRY  *err_list_ptr;
    CHAR        *err_str, error_buffer[NLOG_MAX_BUFFER_SIZE];

    /* Initialize the buffer */
    UTL_Zero(&error_buffer[0], NLOG_MAX_BUFFER_SIZE);

    /* Convert the error info into an ASCII string */
    err_str = NLOG_Error_Put(message, stat, file, line, &error_buffer[0]);

    /* load the needed information into the current array location */
    err_list_ptr = &NLOG_Entry_List[NLOG_Avail_Index];

    /* Copy the data into the logging array */
    strcpy(err_list_ptr->log_msg, err_str);

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Save a pointer to this entry */
    NLOG_IS_Entry_List[NLOG_IS_Avail_Index] = &NLOG_Entry_List[NLOG_Avail_Index];

    /* If the HISR is already processing entries, do not activate it again */
    if ( (((NLOG_IS_Avail_Index - 1) >= 0) &&
          (!(NLOG_IS_Entry_List[NLOG_IS_Avail_Index - 1]))) ||
         (((NLOG_IS_Avail_Index - 1) < 0) &&
          (!(NLOG_IS_Entry_List[NLOG_MAX_ENTRIES - 1]))) )
    {
        if (NU_Activate_HISR(&NLOG_IS_Logging_HISR) != NU_SUCCESS)
            NET_DBG_Notify(NU_INVALID_HISR, __FILE__, __LINE__, NU_NULL,
                           NU_NULL);
    }

    /* Update the index */
    NLOG_IS_Avail_Index = (NLOG_IS_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#endif

    /* increment the global index forward and handle the wrap */
    NLOG_Avail_Index = (NLOG_Avail_Index + 1) % NLOG_MAX_ENTRIES;

#if (PRINT_NET_ERROR_MSG == NU_TRUE)

    /* Print the info */
    NLOG_Info_Print (&error_buffer[0]);

#endif

} /* NLOG_Error_Log_IS */

#endif

#if ( (NLOG_INFO_SEND == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
/***************************************************************************
*
*   FUNCTION
*
*       NLOG_Interrupt_Safe_Logging_HISR
*
*   DESCRIPTION
*
*       This is the interrupt-safe logging HISR that will send a message to
*       a queue to be sent to a foreign port.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
****************************************************************************/
VOID NLOG_Interrupt_Safe_Logging_HISR(VOID)
{
    INT     i;
    STATUS  status;

    /* Look at every entry in the array */
    for (i = 0; i < NLOG_Avail_Index; i++)
    {
        /* If this entry is not NULL */
        if (NLOG_IS_Entry_List[i])
        {
            /* Place a pointer to the message on the NLOG message queue */
            status = NU_Send_To_Queue(&NLOG_Msg_Queue,
                                      (VOID *)&NLOG_IS_Entry_List[i],
                                      1, NU_NO_SUSPEND);

            if (status != NU_SUCCESS)
            {
                /*Increment the internal error counter */
                NLOG_Internal_Error++;
            }

            else
                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);

            /* NULL out the value */
            NLOG_IS_Entry_List[i] = NU_NULL;
        }
    }

} /* NLOG_Interrupt_Safe_Logging_HISR */

#endif
