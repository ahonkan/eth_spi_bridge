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
*       snmp.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions specific to the conceptual SNMP
*       Engine.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Task_Entry
*       SNMP_Setup_Socket
*       SNMP_Setup_Socket6
*       SNMP_Engine_Init
*       SNMP_Engine_Config
*       SNMP_Configuration
*       SNMP_Timer_Expired
*       SNMP_Engine_Time
*       SNMP_Get_Engine_ID
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       socketd.h
*       snmp.h
*       snmp_g.h
*       sys.h
*       udp.h
*       snmp_utl.h
*       xtern.h
*       nu_sd.h
*       er_extr.h
*       ncl.h
*       pcdisk.h
*
*************************************************************************/
#include "nucleus.h"

#include "networking/target.h"
#include "networking/socketd.h"
#include "networking/snmp.h"
#include "networking/snmp_g.h"
#include "networking/sys.h"
#include "networking/snmp_udp.h"
#include "networking/snmp_utl.h"
#include "networking/xtern.h"

extern STATUS NU_Setsockopt_IPV6_V6ONLY(INT socketd, INT optval);
#if (SNMP_CONFIG_SERIAL == NU_TRUE)
#include "drivers/nu_drivers.h"
#include "networking/ncl.h"
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif

#if (INCLUDE_MIB_RMON1==NU_TRUE)
BOOL    Rmon1Init(VOID);
#endif

SNMP_ENGINE_STRUCT      Snmp_Engine;
UINT8                   Snmp_Engine_Status = SNMP_MODULE_NOTSTARTED;
snmp_stat_t             SnmpStat;
udp_stat_t              Snmp_Udp_Stat;
INT                     Snmp_Socket = -1;
#if (INCLUDE_IPV6 == NU_TRUE)
INT                     Snmp_Socket6 = -1;
#endif
extern UINT8            cfig_hostid[];
extern UINT8            cfig_hosttype;

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
NU_SERIAL_PORT          *Snmp_Serial_Port;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
STATUS SNMP_Setup_Socket6 (INT *socket);
#endif

extern STATUS           NU_Get_Remaining_Time(NU_TIMER *, UNSIGNED *);
extern NU_MEMORY_POOL   System_Memory;
extern NU_TASK          Snmp_Notification_Task;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Task_Entry
*
*   DESCRIPTION
*
*       This is the task entry point for the main SNMP function that
*       initializes all agent parameters and receives and processes
*       requests.
*
*   INPUTS
*
*       argc        Unused parameter
*       *argv       Unused parameter
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SNMP_Task_Entry(UNSIGNED argc, VOID *argv)
{
    FD_SET                  readfs, writefs, exceptfs;
    INT                     max_socket;
    INT16                   clilen;
    INT32                   n;
    DV_DEVICE_ENTRY         *current_device;
    SNMP_MESSAGE_STRUCT     snmp_request;
    SNMP_NOTIFY_REQ_STRUCT  *snmp_notification;
    UINT32                  cold_start_oid[] = SNMP_COLD_START_TRAP;
    UINT8                   cold_start_sent = NU_FALSE;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    current_device = DEV_Table.dv_head;

    /* Wait until a device has been registered and is UP before
     * going any further.
     */
    for (;;)
    {
        if (current_device != NU_NULL)
        {
            if ( (current_device->dev_flags & DV_UP) &&
                 (current_device->dev_flags & DV_RUNNING) &&
                 (current_device->dev_type != DVT_LOOP) )
                break;

            current_device = current_device->dev_next;
        }

        if (current_device == NU_NULL)
            current_device = DEV_Table.dv_head;

        NU_Sleep(TICKS_PER_SECOND);
    }

    /* Initialize the SNMP Message Buffer. */
    if (NU_Allocate_Memory(&System_Memory,
                           (VOID**)&(snmp_request.snmp_buffer),
                           (SNMP_BUFSIZE + SNMP_CIPHER_PAD_SIZE),
                           NU_NO_SUSPEND) == NU_SUCCESS)
    {

        snmp_request.snmp_buffer =
                            TLS_Normalize_Ptr(snmp_request.snmp_buffer);

#if (INCLUDE_MIB_RMON1==NU_TRUE)
        Rmon1Init();
#endif
        for (;;)
        {
            /* Setup the socket. */
            if(SNMP_Setup_Socket(&Snmp_Socket) != NU_SUCCESS)
            {
                NU_Sleep(TICKS_PER_SECOND);
                continue;
            }

#if (INCLUDE_IPV6 == NU_TRUE)
            if (SNMP_Setup_Socket6(&Snmp_Socket6) != NU_SUCCESS)
            {
                NU_Sleep(TICKS_PER_SECOND);
                continue;
            }
#endif

            if ((snmp_cfg.coldtrap_enable == NU_TRUE) &&
                (cold_start_sent == NU_FALSE))
            {
                cold_start_sent = NU_TRUE;

                /* Get notification structure to send starting trap. */
                if(SNMP_Get_Notification_Ptr(&snmp_notification)
                                                            == NU_SUCCESS)
                {
                    /* Copy the cold start OID. */
                    NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      cold_start_oid, sizeof(UINT32) * SNMP_TRAP_OID_LEN);

                    snmp_notification->OID.oid_len = SNMP_TRAP_OID_LEN;

                    /* The request is ready. */
                    SNMP_Notification_Ready(snmp_notification);
                }
            }

            /* This is the UDP Transport. */
            snmp_request.snmp_transport_domain = 1;

            for (;;)
            {
                if (Snmp_Socket < 0)
                    break;

                /*Make sure all bits are initially set to 0 */
                NU_FD_Init(&readfs);
                NU_FD_Init(&writefs);
                NU_FD_Init(&exceptfs);

                NU_FD_Set(Snmp_Socket, &readfs);

#if (INCLUDE_IPV6 == NU_TRUE)
                if (Snmp_Socket6 < 0)
                    break;

                NU_FD_Set(Snmp_Socket6, &readfs);

                max_socket = ((Snmp_Socket6 < Snmp_Socket) ?
                                    Snmp_Socket : Snmp_Socket6);
#else
                max_socket = Snmp_Socket;
#endif

                if (NU_Select(max_socket + 1, &readfs, &writefs,
                              &exceptfs, NU_SUSPEND) != NU_SUCCESS)
                {
                    break;
                }

                if (NU_FD_Check(Snmp_Socket, &readfs) == NU_TRUE)
                {
                    n = NU_Recv_From(Snmp_Socket,
                                 ((CHAR *)snmp_request.snmp_buffer),
                                 SNMP_BUFSIZE, 0,
                                 &(snmp_request.snmp_transport_address),
                                 &clilen);

                    if (n < 0)
                        break;
                    else
                    {
                        /* Storing the length of the packet received. */
                        snmp_request.snmp_buffer_len = (UINT32)n;

                        /* Call the execute function for processing the
                         * request.
                         */
                        status = SNMP_Execute_Request(&snmp_request);

                        /* Was the execution of the request successful? */
                        if(status == NU_SUCCESS)
                            Snmp_Udp_Stat.inDatagrams++;
                        else
                            Snmp_Udp_Stat.inErrors++;
                    }
                }

#if (INCLUDE_IPV6 == NU_TRUE)
                if (NU_FD_Check(Snmp_Socket6, &readfs) == NU_TRUE)
                {
                    n = NU_Recv_From(Snmp_Socket6,
                                ((CHAR *)snmp_request.snmp_buffer),
                                SNMP_BUFSIZE, 0,
                                &(snmp_request.snmp_transport_address),
                                &clilen);
                    if (n < 0)
                        break;
                    else
                    {
                        /* storing the length of the packet received */
                        snmp_request.snmp_buffer_len = (UINT32)n;

                        /* Call the execute function for processing the
                         * request.
                         */
                        status = SNMP_Execute_Request(&snmp_request);

                        /* Was the execution of the request successful? */
                        if(status == NU_SUCCESS)
                            Snmp_Udp_Stat.inDatagrams++;
                        else
                            Snmp_Udp_Stat.inErrors++;
                    }
                }
#endif
            }
        }
    }

    /* We only need this statement to avoid compile warning. */
    NU_USER_MODE();

} /* SNMP_Task_Entry */

#if (INCLUDE_IPV6 == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       SNMP_Setup_Socket6
*
*   DESCRIPTION
*
*       This function will close the current socket if it greater than
*       zero, then get a new socket descriptor.  The socket being passed
*       in is a global variable.
*
*   INPUTS
*
*       socket      The socket descriptor to be closed and reassigned.
*
*   OUTPUTS
*
*       NU_SUCCESS          The socket was successfully set up.
*       NU_NO_SOCKETS       There are no sockets available.
*       NU_INVALID_SOCKET   The bind failed.
*       NU_INVALID_PARM     The bind failed.
*       NU_INVALID_ADDRESS  The bind failed.
*
*************************************************************************/
STATUS SNMP_Setup_Socket6(INT *socket)
{
    STATUS  status;
    struct  addr_struct addr;

    if (*socket >= 0)
        NU_Close_Socket((*socket));

    (*socket) = NU_Socket(NU_FAMILY_IP6, NU_TYPE_DGRAM, 0);

    if ((*socket) < 0)
        status = NU_NO_SOCKETS;

    else
    {
        NU_Fcntl((*socket), NU_SETFLAG, NU_BLOCK);

        addr.family = NU_FAMILY_IP6;
        memset(&addr.id.is_ip_addrs[0], 0, 16);
        addr.name = "SNMP Receive";
        addr.port = SNMP_PORT;

        NU_Setsockopt_IPV6_V6ONLY(*socket, NU_TRUE);

        if ((status = NU_Bind((*socket), &addr, 0)) < 0)
            NU_Close_Socket((*socket));
        else
            status = NU_SUCCESS;
    }

    return (status);

} /* SNMP_Setup_Socket6 */

#endif

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Setup_Socket
*
*   DESCRIPTION
*
*       This function will close the current socket if it greater than
*       zero, then get a new socket descriptor.  The socket being passed
*       in is a global variable.
*
*   INPUTS
*
*       socket      The socket descriptor to be closed and reassigned.
*
*   OUTPUTS
*
*       NU_SUCCESS          The socket was successfully set up.
*       NU_NO_SOCKETS       There are no sockets available.
*       NU_INVALID_SOCKET   The bind failed.
*       NU_INVALID_PARM     The bind failed.
*       NU_INVALID_ADDRESS  The bind failed.
*
*************************************************************************/
STATUS SNMP_Setup_Socket(INT *socket)
{
    STATUS  status;
    struct  addr_struct addr;

    if (*socket >= 0)
        NU_Close_Socket((*socket));

    (*socket) = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, 0);

    if ((*socket) < 0)
        status = NU_NO_SOCKETS;

    else
    {
        NU_Fcntl((*socket), NU_SETFLAG, NU_BLOCK);

        addr.family = NU_FAMILY_IP;
        NU_BLOCK_COPY(&addr.id.is_ip_addrs[0], "\0\0\0\0", 4);
        addr.name = "SNMP Receive";
        addr.port = SNMP_PORT;

        NU_Setsockopt_IP_RECVIFADDR(*socket, NU_TRUE);

        if ((status = NU_Bind((*socket), &addr, 0)) < 0)
            NU_Close_Socket((*socket));
        else
            status = NU_SUCCESS;
    }

    return (status);

} /* SNMP_Setup_Socket */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Engine_Init
*
*   DESCRIPTION
*
*       This function initializes the SNMP Engine.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS              The initialization was successful.
*       SNMP_BAD_PARAMETER      There was an error during initialization.
*
*************************************************************************/
STATUS SNMP_Engine_Init(VOID)
{
#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
    INT        snmp_file;
    CHAR       nu_drive[3];
#endif

    STATUS     status;

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
    /* Get the Serial Port. */
    Snmp_Serial_Port = NU_SIO_Get_Port();
#endif

    /* Set the maximum SNMP Message size which can be processed. */
    Snmp_Engine.snmp_max_message_size = SNMP_BUFSIZE;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Register this task to access the file system. */
    status = NU_Become_File_User();

    if (status == NU_SUCCESS)
    {
        /* Open disk for accessing */
        status = NU_Open_Disk (nu_drive);

        /* If the disk is already open, return success and leave the current
         * directory where it is.
         */
        if (status == NUF_NO_ACTION)
            status = NU_SUCCESS;

        if (status == NU_SUCCESS)
        {
            /* Set the default drive */
            status = NU_Set_Default_Drive(SNMP_DRIVE);

            if (status == NU_SUCCESS)
            {
                /*  Set Current Directory to SNMP_DIR */
                status = NU_Set_Current_Dir(SNMP_DIR);

                if (status == NUF_NOFILE)
                {
                    /* If the directory does not exist, create it in the
                     * initialization phase
                     */
                    status = NU_Make_Dir(SNMP_DIR);
                }
                else if (status != NU_SUCCESS)
                {
                    /* The SNMP Engine was not initialized. */
                    Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

                    NLOG_Error_Log("SNMP_Engine_Init cannot set Current Directory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Continue if all is well */
                if(status == NU_SUCCESS)
                {
                    /* Open the file */
                    snmp_file = NU_Open(SNMP_FILE, (PO_RDWR | PO_BINARY),
                                        PS_IREAD);

                    if (snmp_file >= 0)
                    {
                        /* Read the value for snmpEngineBoots and SNMP engine id. */
                        if(NU_Read(snmp_file, (CHAR *)(&Snmp_Engine.snmp_engine_boots),
                                   4) != 4)
                        {
                            /* The SNMP Engine was not initialized. */
                            Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;
                        }

                        else if(NU_Read(snmp_file, (CHAR *)(&Snmp_Engine.snmp_engine_id_len),
                                        4) != 4)
                        {
                            /* The SNMP Engine was not initialized. */
                            Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;
                        }

                        else if(NU_Read(snmp_file, (CHAR *)Snmp_Engine.snmp_engine_id,
                                        (UINT16)Snmp_Engine.snmp_engine_id_len)
                                != Snmp_Engine.snmp_engine_id_len)
                        {
                            /* The SNMP Engine was not initialized. */
                            Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;
                        }
                        else
                        {
                            /* Since we have restarted again, increment the value of
                             * snmpEngineBoots. */
                            Snmp_Engine.snmp_engine_boots++;

                            NU_Seek(snmp_file, 0, 0);

                            if(NU_Write(snmp_file, (CHAR *)(&Snmp_Engine.snmp_engine_boots),
                                        4) == 4)
                            {
                                Snmp_Engine_Status = SNMP_MODULE_INITIALIZED;
                            }
                            else
                            {
                                /* The SNMP Engine was not initialized. */
                                Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;
                            }
                        }

                        /* Close the file. */
                        NU_Close(snmp_file);
                    }

                    else
                    {
                        /* The SNMP Engine was not initialized. */
                        Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

                        NLOG_Error_Log("SNMP_Engine_Init failed to open the file for reading.",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    /* The SNMP Engine was not initialized. */
                    Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

                    NLOG_Error_Log("SNMP Engine failed to initialize",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                /* The SNMP Engine was not initialized. */
                Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

                NLOG_Error_Log("SNMP_Engine_Init cannot set Default Drive",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Close the Disk */
            NU_Close_Disk (nu_drive);
        }

        else
        {
            /* The SNMP Engine was not initialized. */
            Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

            NLOG_Error_Log("SNMP_Engine_Init cannot Open Disk",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* We are done using the file system. */
        NU_Release_File_User();
    }

    else
    {
        /* The SNMP Engine was not initialized. */
        Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

        NLOG_Error_Log("SNMP_Engine_Init Task cannot register as Nucleus FILE User",
                       NERR_SEVERE, __FILE__, __LINE__);
    }


#else /* (SNMP_ENABLE_FILE_STORAGE == NU_FALSE) */

    /* The SNMP Engine was not initialized. */
    Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;

    NLOG_Error_Log("SNMP Engine failed to initialize",
                   NERR_SEVERE, __FILE__, __LINE__);

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

    /* Create the Timer. Keep it disabled. We will enable it later. */
    if((status = NU_Create_Timer(&(Snmp_Engine.snmp_engine_timer),
                                 (CHAR *)"SNMPTime", SNMP_Timer_Expired,
                                 1, 0xFFFFFFFF, 0xFFFFFFFF,
                                 NU_DISABLE_TIMER)) != NU_SUCCESS)
       status = SNMP_BAD_PARAMETER;

    return (status);

} /* SNMP_Engine_Init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Engine_Config
*
*   DESCRIPTION
*
*       This function configures the SNMP Engine.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS  The initialization was successful.
*
*************************************************************************/
STATUS SNMP_Engine_Config(VOID)
{
    STATUS status;

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
    CHAR   snmp_boots_str[13];
    UINT8  i;
    CHAR   input[2];
    UINT8  temp;
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
    INT        file;
    CHAR       nu_drive[3];
#endif

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if (SNMP_CONFIG_SERIAL == NU_TRUE)

    /* Null terminate. */
    input[1] = 0;

#endif

    /* Wait for the SNMP Engine to be initialized. */
    while(Snmp_Engine_Status == SNMP_MODULE_NOTSTARTED)
    {
        /* Sleep for a second. */
        NU_Sleep(NU_PLUS_Ticks_Per_Second);
    }

    /* Check whether the initialization has succeeded. */
    if(Snmp_Engine_Status == SNMP_MODULE_NOTINITIALIZED)
    {
        /* The initialization did not succeed. Manual configuration is
         * now required.
         */

        /* Initialize the snmpEngineID. */

        /* The first four octets are assigned the IANA number. */
        Snmp_Engine.snmp_engine_id[0] =
                           (UINT8)((SNMP_IANA_NUMBER & 0xFF000000) >> 24);
        Snmp_Engine.snmp_engine_id[1] =
                           (UINT8)((SNMP_IANA_NUMBER & 0x00FF0000) >> 16);
        Snmp_Engine.snmp_engine_id[2] =
                           (UINT8)((SNMP_IANA_NUMBER & 0x0000FF00) >> 8);
        Snmp_Engine.snmp_engine_id[3] =
                           (UINT8)(SNMP_IANA_NUMBER & 0x000000FF);

        /* The first bit is set to 1. */
        Snmp_Engine.snmp_engine_id[0] =
                            (UINT8)(Snmp_Engine.snmp_engine_id[0] | 0x80);

        /* Use the IPv4 address for identification. */
        Snmp_Engine.snmp_engine_id[4] = cfig_hosttype;

        NU_BLOCK_COPY(&Snmp_Engine.snmp_engine_id[5], cfig_hostid,
                      SNMP_MAX_IP_ADDRS);

        if (cfig_hosttype == 1)
        {
            /* Set the SNMP Engine ID length. */
            Snmp_Engine.snmp_engine_id_len = 5 + IP_ADDR_LEN;
        }

#if (INCLUDE_IPV6 == NU_TRUE)
        else
        {
            /* Set the SNMP Engine ID length. */
            Snmp_Engine.snmp_engine_id_len = 5 + IP6_ADDR_LEN;
        }
#endif

        /* Set the initial values for snmpEngineBoots. */
        Snmp_Engine.snmp_engine_boots = 0xFFFFFFFF;

        /* Using the hard coded configuration. */

        /* This hard coded configuration just restarts the SNMP Engine
         * Timer. Note that this can be a security hazard. */

        /* Enable the timer. */
        if((status = NU_Control_Timer(&Snmp_Engine.snmp_engine_timer,
                                      NU_ENABLE_TIMER)) == NU_SUCCESS)
        {
            Snmp_Engine.snmp_engine_boots = 1;
            Snmp_Engine.snmp_timer_resets = 0;
            Snmp_Engine_Status = SNMP_MODULE_INITIALIZED;
        }

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
        /* Use the Serial Interface. */

        for(;;)
        {
            /* This is the SNMP Engine Configuration Wizard. */
            NU_SD_Put_String("*************************************",
                              Snmp_Serial_Port);
            NU_SD_Put_String("*************************************\n\r",
                             Snmp_Serial_Port);
            NU_SD_Put_String("Copyright (c) 1998 - 2007 MGC - Nucleus ",
                             Snmp_Serial_Port);
            NU_SD_Put_String("SNMP - Engine Configuration Wizard",
                             Snmp_Serial_Port);
            NU_SD_Put_String("\n\r", Snmp_Serial_Port);
            NU_SD_Put_String("*************************************",
                             Snmp_Serial_Port);
            NU_SD_Put_String("*************************************\n\r",
                             Snmp_Serial_Port);

            /* Output the current SNMP Engine ID. */
            NU_SD_Put_String("1. Set SNMP Engine ID - Current Value = ",
                             Snmp_Serial_Port);

            for(i = 0; i < Snmp_Engine.snmp_engine_id_len; i++)
            {
                NU_SD_Put_Char(UTL_Hex_To_Char((UINT8)
                    ((Snmp_Engine.snmp_engine_id[i] >> 4))),
                    Snmp_Serial_Port, NU_FALSE);
                NU_SD_Put_Char(UTL_Hex_To_Char((UINT8)((Snmp_Engine.
                        snmp_engine_id[i] & 0x0F))), Snmp_Serial_Port, NU_FALSE);
            }

            NU_SD_Put_String("\n\r", Snmp_Serial_Port);

            /* Output the current SNMP Engine Boots. */
            NU_ULTOA(Snmp_Engine.snmp_engine_boots, snmp_boots_str, 10);
            NU_SD_Put_String("2. Set SNMP Engine Boots - Current Value = "
                             , Snmp_Serial_Port);
            NU_SD_Put_String(snmp_boots_str, Snmp_Serial_Port);
            NU_SD_Put_String("\n\r", Snmp_Serial_Port);

            NU_SD_Put_String("3. Quit Wizard", Snmp_Serial_Port);
            NU_SD_Put_String("\n\n\r", Snmp_Serial_Port);

            NU_SD_Put_String("Enter Choice: ", Snmp_Serial_Port);

            /* Wait for an input. */
            while (!NU_SD_Data_Ready(Snmp_Serial_Port));

            input[0] = SDC_Get_Char(Snmp_Serial_Port);
            NU_SD_Put_Char(input[0], Snmp_Serial_Port, NU_FALSE);
            NU_SD_Put_String("\n\n\r", Snmp_Serial_Port);

            /* What does the user want to do? */
            if(input[0] == '1')
            {
                /* Set the SNMP Engine ID. */
                NU_SD_Put_String("New SNMP Engine ID = ",
                                 Snmp_Serial_Port);

                i = 0;
                Snmp_Engine.snmp_engine_id_len = 0;

                while(Snmp_Engine.snmp_engine_id_len <
                                                SNMP_SIZE_SMALLOBJECTID)
                {
                    /* Wait for an input. */
                    while (!NU_SD_Data_Ready(Snmp_Serial_Port));

                    /* Get a hexadecimal digit. */
                    input[0] = SDC_Get_Char(Snmp_Serial_Port);

                    if(input[0] != '\r')
                    {
                        /* convert from hexadecimal to digit. */
                        temp = (UINT8)NU_AHTOI(input);

                        if(temp <= 0x0F)
                        {
                            /* If the input is correct put it in the right
                             * place. i=0 means the higher 4-bits of the
                             * byte and i=1 means the lower 4-bits of the
                             * byte.
                             */
                            NU_SD_Put_Char(input[0], Snmp_Serial_Port, NU_FALSE);

                            if(i == 0)
                            {
                                Snmp_Engine.snmp_engine_id[Snmp_Engine.
                                          snmp_engine_id_len] =
                                                       (UINT8)(temp << 4);
                                i++;
                            }
                            else
                            {
                                Snmp_Engine.snmp_engine_id[Snmp_Engine.
                                             snmp_engine_id_len] |= temp;
                                i = 0;
                                Snmp_Engine.snmp_engine_id_len++;
                            }
                        }
                    }
                    else
                    {
                        /* User has finished with the input. */

                        /* If we have an unfinished byte, complete it. */
                        if(i == 1)
                        {
                            Snmp_Engine.snmp_engine_id_len++;
                            Snmp_Engine.snmp_engine_id[Snmp_Engine.
                                              snmp_engine_id_len] |= 0x0;
                        }

                        /* Break out of the loop. */
                        break;
                    }
                }
            }
            else if(input[0] == '2')
            {
                /* Set the SNMP Engine Boots. */
                NU_SD_Put_String("New SNMP Engine Boots value = ",
                                 Snmp_Serial_Port);

                /* i is the length of the integer. */
                i = 0;

                while(i < 10)
                {
                    /* Wait for an input. */
                    while (!NU_SD_Data_Ready(Snmp_Serial_Port));

                    /* Get an integer value. */
                    input[0] = SDC_Get_Char(Snmp_Serial_Port);

                    if(input[0] != '\r')
                    {
                        temp = (UINT8)NU_AHTOI(input);

                        /* If the digit entered is valid. */
                        if(temp <= 0x09)
                        {
                            snmp_boots_str[i] = input[0];
                            NU_SD_Put_Char(input[0], Snmp_Serial_Port, NU_FALSE);
                            i++;
                        }
                    }
                    else
                    {
                        /* Break out of the loop. */
                        break;
                    }
                }

                /* Terminate the string. */
                snmp_boots_str[i] = NU_NULL;

                /* convert from string to integer. */
                Snmp_Engine.snmp_engine_boots = NU_ATOI(snmp_boots_str);
            }

            else if(input[0] == '3')
            {
                /* Quit Wizard. */

                /* Enable the timer. */
                if((status = NU_Control_Timer(&Snmp_Engine.
                       snmp_engine_timer, NU_ENABLE_TIMER)) != NU_SUCCESS)
                {
                    /* The timer did not start! The configuration failed.
                     * Set Snmp_Engine.snmp_engine_boots to 0xFFFFFFFF.
                     */
                    Snmp_Engine.snmp_engine_boots = 0xFFFFFFFF;
                    NU_SD_Put_String("Unable to enable timer. SNMP Engine"
                                   " not configured!", Snmp_Serial_Port);

                    NU_SD_Put_String("\n\r", Snmp_Serial_Port);
                    NU_SD_Put_String("Quitting Configuration Wizard",
                                     Snmp_Serial_Port);

                    NU_SD_Put_String("\n\n\n\r", Snmp_Serial_Port);
                }
                else
                {
                    /* The timer was successfully enabled. The
                     * configuration was successful.
                     */
                    Snmp_Engine.snmp_timer_resets = 0;
                    Snmp_Engine_Status = SNMP_MODULE_INITIALIZED;
                    NU_SD_Put_String("SNMP Engine successfully"
                                     " configured!", Snmp_Serial_Port);
                    NU_SD_Put_String("\n\r", Snmp_Serial_Port);
                    NU_SD_Put_String("Quitting Configuration Wizard",
                                     Snmp_Serial_Port);
                    NU_SD_Put_String("\n\n\n\r", Snmp_Serial_Port);
                }

                break;
            }

            NU_SD_Put_String("\n\n\n\r", Snmp_Serial_Port);
       }

#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

        nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
        nu_drive[1] = ':';
        nu_drive[2] = '\0';

        /* Register this task to access the file system. */
        status = NU_Become_File_User();

        if (status == NU_SUCCESS)
        {
            /* Open disk for accessing */
            status = NU_Open_Disk (nu_drive);

            /* If the disk is already open, return success and leave the current
             * directory where it is.
             */
            if (status == NUF_NO_ACTION)
                status = NU_SUCCESS;

            if (status == NU_SUCCESS)
            {
                /* Set the default drive */
                status = NU_Set_Default_Drive(SNMP_DRIVE);

                if (status == NU_SUCCESS)
                {
                    /*  Set Current Directory to SNMP_DIR */
                    status = NU_Set_Current_Dir(SNMP_DIR);
                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP_Engine_Config cannot set Current Directory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("SNMP_Engine_Config cannot set Default Drive",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* If all is well, open the file. */
                if(status == NU_SUCCESS)
                {
                    /* Delete the file if it already exists. */
                    NU_Delete(SNMP_FILE);

                    /* Create a new file and open it. */
                    file = NU_Open(SNMP_FILE,
                                PO_RDWR | PO_CREAT | PO_BINARY, PS_IWRITE);

                    /* If the file was successfully created and opened. */
                    if(file >= 0)
                    {
                        /* Write the SNMP engine boots to file. */
                        NU_Write(file, (CHAR *)(&Snmp_Engine.snmp_engine_boots), 4);

                        /* Write the SNMP engine ID to file. */
                        NU_Write(file, (CHAR *)(&Snmp_Engine.snmp_engine_id_len), 4);
                        NU_Write(file, (CHAR *)Snmp_Engine.snmp_engine_id,
                                 (UINT16)Snmp_Engine.snmp_engine_id_len);

                        /* Close the file. */
                        NU_Close(file);
                    }
                    else
                    {
                        /* If an error occurred, make a log entry. */
                        NLOG_Error_Log("SNMP_Engine_Config failed to create Engine MIB file",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                /* Close the Disk */
                NU_Close_Disk (nu_drive);
            }

            else
            {
                NLOG_Error_Log("SNMP_Engine_Config cannot Open Disk",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* We are done using the file system. */
            NU_Release_File_User();
        }

        else
        {
            NLOG_Error_Log("SNMP_Engine_Config cannot register as Nucleus FILE User",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
    }
    else
    {
        /* The initializations succeeded. Enable the timer. */
        if((status = NU_Control_Timer(&Snmp_Engine.snmp_engine_timer,
                                         NU_ENABLE_TIMER)) != NU_SUCCESS)
        {
            /* The timer did not start! The configuration failed. Set
             * Snmp_Engine.snmp_engine_boots to 0xFFFFFFFF. */
            Snmp_Engine_Status = SNMP_MODULE_NOTINITIALIZED;
            Snmp_Engine.snmp_engine_boots = 0xFFFFFFFF;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Engine_Config */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Configuration
*
*   DESCRIPTION
*
*       This function configures all the components of Nucleus SNMP.
*
*   INPUTS
*
*   OUTPUTS
*
*************************************************************************/
VOID SNMP_Configuration(VOID)
{
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Configure the SNMP Engine. */
    SNMP_Engine_Config();

    /* Configure the Message Processing Models. */
    SNMP_Mp_Config();

    /* Configure the Security Models. */
    SNMP_Ss_Config();

    /* Configure the Access Control Model. */
    VACM_Config();

    /* Configure the Notification Originator application. */
    SNMP_Notification_Config();

    /* Resume suspended Notification task. */
    NU_Resume_Task(&Snmp_Notification_Task);

    NU_USER_MODE();

} /* SNMP_Configuration */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Timer_Expired
*
*   DESCRIPTION
*
*       This function is executed when the timer expires. At this point we
*       we update the concept of time.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS                  The initialization was successful.
*
*************************************************************************/
VOID SNMP_Timer_Expired(UNSIGNED id)
{
#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
    STATUS      status;
    INT         fd;
    CHAR        nu_drive[3];
#endif

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(id);

    /* Increment the number of timer resets. */
    Snmp_Engine.snmp_timer_resets++;

    /* If this is equal to NU_PLUS_Ticks_Per_Second then the number of
     * seconds has reached 4294967295 and we need to increment
     * Snmp_Engine.snmp_engine_boots.
     */
    if(Snmp_Engine.snmp_timer_resets == NU_PLUS_Ticks_Per_Second)
    {
        Snmp_Engine.snmp_timer_resets = 0;
        Snmp_Engine.snmp_engine_boots++;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

        /* Save the new value to file. */

        /* Create SNMP engine directory if it does not exist
          put snmp_engine_boots value = 1. id=current id*/

        nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
        nu_drive[1] = ':';
        nu_drive[2] = '\0';

        /* Register this task to access the file system. */
        status = NU_Become_File_User();

        if (status == NU_SUCCESS)
        {
            /* Open disk for accessing */
            status = NU_Open_Disk (nu_drive);

            /* If the disk is already open, return success and leave the current
             * directory where it is.
             */
            if (status == NUF_NO_ACTION)
                status = NU_SUCCESS;

            if (status == NU_SUCCESS)
            {
                /* Set the default drive */
                status = NU_Set_Default_Drive(SNMP_DRIVE);

                if (status == NU_SUCCESS)
                {
                    /*  Set Current Directory to SNMP_DIR */
                    status = NU_Set_Current_Dir(SNMP_DIR);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP_Timer_Expired cannot set Current Directory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("SNMP_Timer_Expired cannot set Default Drive",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                if (status == NU_SUCCESS)
                {
                    /* Open the file to write */
                    fd = NU_Open(SNMP_FILE, PO_RDWR|PO_BINARY, PS_IWRITE);

                    if (fd >= 0)
                    {
                        /* If successfully opened, write to the file */
                        if (NU_Write(fd, (CHAR *)(&Snmp_Engine.snmp_engine_boots), 4) < 0)
                        {
                            NLOG_Error_Log("SNMP_Timer_Expired failed to write to file.",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        /* Close the file once done writing. */
                        NU_Close(fd);
                    }
                    else
                    {
                        NLOG_Error_Log("SNMP_Timer_Expired failed to open the file.",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                /* Close the Disk */
                NU_Close_Disk (nu_drive);
            }

            else
            {
                NLOG_Error_Log("SNMP_Timer_Expired cannot Open Disk",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* We are done using the file system. */
            NU_Release_File_User();
        }

        else
        {
            NLOG_Error_Log("SNMP_Timer_Expired cannot register as Nucleus FILE User",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
    }

} /* SNMP_Timer_Expired */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Engine_Time
*
*   DESCRIPTION
*
*       This function retrieves the SNMP Engine Time.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS  The time has been retrieved.
*       Error Indication otherwise.
*
*************************************************************************/
STATUS SNMP_Engine_Time(UINT32 *snmp_engine_time)
{
    STATUS status;

    status = NU_Get_Remaining_Time(&Snmp_Engine.snmp_engine_timer,
                                   (UNSIGNED *)snmp_engine_time);

    if(status == NU_SUCCESS)
        (*snmp_engine_time) = ((0xFFFFFFFF - (*snmp_engine_time)) /
                                             NU_PLUS_Ticks_Per_Second) +
                (Snmp_Engine.snmp_timer_resets * (0xFFFFFFFF/
                                               NU_PLUS_Ticks_Per_Second));

    return (status);

} /* SNMP_Engine_Time */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Get_Engine_ID
*
*   DESCRIPTION
*
*       This function retrieves the SNMP Engine ID.
*
*   INPUTS
*       engine_id           pointer to array containing engine id
*       engine_id_len       pointer to engine id length
*
*   OUTPUTS
*
*       NU_SUCCESS  The id has been retrieved.
*       Error Indication otherwise.
*
*************************************************************************/
STATUS SNMP_Get_Engine_ID(UINT8 *engine_id, UINT32 *engine_id_len)
{
    STATUS status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (engine_id != NU_NULL) && (engine_id_len != NU_NULL))
    {
        *engine_id_len = Snmp_Engine.snmp_engine_id_len;
        NU_BLOCK_COPY(engine_id, Snmp_Engine.snmp_engine_id,
                      Snmp_Engine.snmp_engine_id_len);
    }

    else
    {
        status = SNMP_ERROR;
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* SNMP_Get_Engine_ID */

