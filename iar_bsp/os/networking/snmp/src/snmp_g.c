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
*       snmp_g.c                                                 
*
*   DESCRIPTION
*
*       This file contains functions to initialize and update SNMP.
*
*   DATA STRUCTURES
*
*       rfc1213_vars
*       Snmp_Task
*       Snmp_App_Responder
*       Snmp_Notification_Task
*
*   FUNCTIONS
*
*       nu_os_net_snmp_init
*       NU_SNMP_Init
*       NU_SNMP_System_Group_Initialize
*       NU_SNMP_Set_Host_Id
*
*   DEPENDENCIES
*
*       target.h
*       snmp_g.h
*       snmp_cr.h
*       sys.h
*       xtern.h
*       ncs_api.h
*       reg_api.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/snmp_g.h"
#include "networking/snmp_cr.h"
#include "networking/sys.h"
#include "networking/xtern.h"
#include "services/reg_api.h"
NU_TASK Snmp_Task;
NU_TASK Snmp_App_Responder;
NU_TASK Snmp_Notification_Task;
NU_TASK Snmp_Init_Task;

/* This is the global data structure where all MIB2 data is stored.  The
   definition of this structure is in 1213XXXX.H. */
rfc1213_vars_t  rfc1213_vars;

#ifdef SNMP_REL_1_5
/*  List of devices. */
extern UINT8 IP_Time_To_Live;
#endif

/* External Initialization Functions. */
extern VOID           nc_init(VOID);
extern BOOLEAN        Mib2Init(VOID);

extern UINT8 cfig_hostid[SNMP_MAX_IP_ADDRS];
extern UINT8 cfig_hosttype;

extern NU_MEMORY_POOL               System_Memory;
extern NU_EVENT_GROUP               Snmp_Events;
extern SNMP_NOTIFY_REQ_LIST         Snmp_Notification_List;
extern STATUS                       SNMP_Wait_For_FS(CHAR *mount_point,
                                                     UNSIGNED timeout);
extern VOID                         ERC_System_Error (INT);

/* String to store registry path for snmp */
CHAR                SNMP_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

/************************************************************************
* FUNCTION
*
*       nu_os_net_snmp_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus SNMP and SNMP Research
*       modules. It is the entry point of the product initialization
*       sequence.
*
* INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       NU_NO_MEMORY            Memory not available.
*       SNMP_ALREADY_RUNNING    SNMP already initialized.
*       SNMP_INVALID_PARAMS     Invalid parameter(s).
*
************************************************************************/
STATUS nu_os_net_snmp_init(CHAR *path, INT startstop)
{
    VOID              *pointer;

    STATUS            status = -1;

    if(path != NU_NULL)
    {
        /* Save a copy locally. */
        strcpy(SNMP_Registry_Path, path);
    }

    if(startstop)
    {
        /* SNMP Initialization Task */
        status = NU_Allocate_Memory(&System_Memory, &pointer,
                                    (UNSIGNED)SNMP_TASK_SIZE,
                                    (UNSIGNED)NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            pointer = TLS_Normalize_Ptr(pointer);
            status = NU_Create_Task(&Snmp_Init_Task, "SNMPIni",
                                    NU_SNMP_Init_Task,
                                    (UNSIGNED)0, NU_NULL, pointer,
                                    (UNSIGNED)SNMP_TASK_SIZE, TM_PRIORITY,
                                    (UNSIGNED)0, NU_PREEMPT, NU_START);

        }
    }
    else
    {
        /* Stop requested, for now nothing to do */
        status = NU_SUCCESS;
    }

    return (status);

} /* nu_os_net_snmp_init */

/************************************************************************
*
*   FUNCTION
*
*       NU_SNMP_Init_Task
*
*   DESCRIPTION
*
*       This task waits for all the components that SNMP depends upon
*       and then initializes SNMP.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_SNMP_Init_Task(UNSIGNED argc, VOID *argv)
{
    STATUS              status = NU_SUCCESS;
#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
    CHAR                nu_drive[3];
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
    nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Wait for maximum storage devices to be initialized. */
    status = SNMP_Wait_For_FS(nu_drive, NU_SUSPEND);
    if ((status != NU_SUCCESS) && (status != NU_TIMEOUT))
    {
        NLOG_Error_Log("Error waiting for the File System to be initialized",
                       NERR_SEVERE, __FILE__, __LINE__);
    }
#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

    if (status == NU_SUCCESS)
    {
    /* Initialize the Nucleus SNMP component. */
#if (defined(SNMP_VER) == NU_TRUE)
        status = NU_SNMP_Initialize();

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error at call to NU_SNMP_Initialize().\n",
                           NERR_FATAL, __FILE__, __LINE__);
            /* Call error handling function */
            ERC_System_Error(status);
        }
#endif

#if (INCLUDE_SR_SNMP == NU_TRUE)
        SR_SNMP_Initialize();
#endif
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_SNMP_Init
*
*   DESCRIPTION
*
*       This function initializes the SNMP.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              SNMP initialized successfully.
*       SNMP_BAD_PARAMETER      One of the initialization routines
*                               received a bad parameter.
*       SNMP_NO_MEMORY          Memory allocation failed.
*       SNMP_BAD_POINTER        Insufficient resources.
*       NU_INVALID_SEMAPHORE    Invalid semaphore pointer.
*       NU_INVALID_SUSPEND      Suspension requested from non-task.
*
*************************************************************************/
STATUS NU_SNMP_Init (VOID)
{
    SNMP_System_Group   sys_group;
    VOID                *pointer;
    STATUS              status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Clear the SNMP data structure. */
    UTL_Zero((char *)&rfc1213_vars, sizeof(rfc1213_vars_t));

#if (RFC1213_SYS_INCLUDE == NU_TRUE)
    /* SNMP database initialization. */
    strcpy((char *)sys_group.sysDescr, MIBII_SYSDESCRIPTION);
    strcpy((char *)sys_group.sysObjectID, MIBII_OBJECTID);
    strcpy((char *)sys_group.sysContact, MIBII_SYSCONTACT);
    strcpy((char *)sys_group.sysLocation, MIBII_SYSLOCATION);
    sys_group.sysServices = MIBII_SERVICES;
#endif

    sc_init();
    if(!x_timerinit())
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize timer",
                       NERR_SEVERE,__FILE__, __LINE__);
        return SNMP_NO_MEMORY;
    }
    nc_init();
    status = snmp_init();
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize xsnmp variables",
                       NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    MacInit();

    status = SNMP_Mib_Init();
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize MIB", NERR_SEVERE,
                        __FILE__, __LINE__);
        return status;
    }

    sys_group.sysUpTime = 0;

    status = NU_SNMP_System_Group_Initialize(&sys_group);
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize 1213 system group components",
                       NERR_SEVERE,__FILE__, __LINE__);
    }
    Mib2Init();

    /* Initialize the SNMP Engine. */
    status = SNMP_Engine_Init();
        if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize SNMP Engine",
                        NERR_SEVERE,__FILE__, __LINE__);
        return status;
    }

    /* Initializing the SNMP Message Processing Models. */
    status = SNMP_Mp_Init();
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize Message Processing Models",
                       NERR_SEVERE,__FILE__, __LINE__);
        return status;
    }

    /* Initialize the SNMP Security Models. */
    status = SNMP_Ss_Init();
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize Security Models\n",
                       NERR_SEVERE,__FILE__, __LINE__);
    }


    /* Initialize the SNMP Access Control Model. */
    status = VACM_Init();
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to initialize Access Control Model\n",
                       NERR_SEVERE,__FILE__, __LINE__);
    }


    /* Initialize the SNMP Notification Model. Effectively, this step only
     * reads and initializes Target Address and Params tables from file.
     */
    Notification_Init();

    /* Initialize the event group to be used by SNMP. */
    status = NU_Create_Event_Group(&Snmp_Events, "SNMPEv");
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init NU_Create_Event_Group failed",
                        NERR_SEVERE, __FILE__, __LINE__);


        NU_USER_MODE();
        return (SNMP_BAD_PARAMETER);
    }    

    /* Create Notification Originator Task */
    status = NU_Allocate_Memory(&System_Memory, &pointer,
                                (UNSIGNED)SNMP_TASK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to allocate memory for Notification task",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_NO_MEMORY);
    }

    pointer = TLS_Normalize_Ptr(pointer);
    status = NU_Create_Task(&Snmp_Notification_Task, "SNMPNo",
                            SNMP_Notification_Task_Entry,
                            (UNSIGNED)0, NU_NULL, pointer,
                            (UNSIGNED)SNMP_TASK_SIZE, TM_PRIORITY,
                            (UNSIGNED)0, NU_PREEMPT, NU_NO_START);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init NU_Create_Task failed",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_BAD_PARAMETER);
    }

    /* Create the SNMP task.  */
    status = NU_Allocate_Memory(&System_Memory, &pointer,
                               (UNSIGNED)SNMP_TASK_SIZE,
                               (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to allocate memory for SNMP task",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_NO_MEMORY);
    }

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Task(&Snmp_Task, "SNMPTask",
                            SNMP_Task_Entry, (UNSIGNED)0, NU_NULL,
                            pointer, (UNSIGNED)SNMP_TASK_SIZE,
                            TM_PRIORITY, (UNSIGNED)0, NU_PREEMPT,
                            NU_NO_START);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init NU_Create_Task failed",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_BAD_PARAMETER);
    }

    NU_Resume_Task(&Snmp_Task);

    /* Create the Application Responder Task.  */
    status = NU_Allocate_Memory(&System_Memory, &pointer,
                                (UNSIGNED)SNMP_TASK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init Failed to allocate memory for App. responder task",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_NO_MEMORY);
    }

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Task(&Snmp_App_Responder, "SNMPCr",
                            SNMP_AppResponder_Task_Entry, (UNSIGNED)0,
                            NU_NULL, pointer,
                            (UNSIGNED)SNMP_TASK_SIZE, TM_PRIORITY,
                            (UNSIGNED)0, NU_PREEMPT, NU_NO_START);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_SNMP_Init NU_Create_Task failed",
                        NERR_SEVERE, __FILE__, __LINE__);

        NU_USER_MODE();    /* return to user mode */
        return (SNMP_BAD_PARAMETER);
    }

    NU_Resume_Task(&Snmp_App_Responder);

    SNMP_Configuration();

    /* return to user mode */
    NU_USER_MODE();
    return (NU_SUCCESS);

} /* NU_SNMP_Init */

#if (RFC1213_SYS_INCLUDE == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       NU_SNMP_System_Group_Initialize
*
*   DESCRIPTION
*
*       This function should be called by applications to initialize the
*       MIB2 system group.
*
*   INPUTS
*
*       *sys_group      A pointer to the data structure containing the
*                       values of the rfc1213_sys components.
*
*   OUTPUTS
*
*       NU_SUCCESS              The parameters were successfully
*                               initialized.
*       SMNP_BAD_PARAMETER  One of the parameters in the sys_group
*                               data structures was invalid.
*       SNMP_BAD_POINTER    Insufficient resources.
*
*************************************************************************/
STATUS NU_SNMP_System_Group_Initialize (const rfc1213_sys_t *sys_group)
{
    static  INT     initialized = 0;
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (initialized == 0)
    {
        if (sys_group == NU_NULL)
            status = SNMP_BAD_PARAMETER;

        /* Perform error checking to make sure the parameters are
         * valid.
         */
        else if ((strlen((const char *)sys_group->sysDescr) >=
                                                    MAX_1213_STRSIZE) ||
                 (strlen((const char *)sys_group->sysObjectID) >
                                                    MAX_1213_STRSIZE) ||
                 (strlen((const char *)sys_group->sysContact) >=
                                                    MAX_1213_STRSIZE) ||
                 (strlen((const char *)sys_group->sysLocation) >=
                                                    MAX_1213_STRSIZE))
        {
            status = SNMP_BAD_PARAMETER;
        }

        else if (sys_group->sysServices > 127)
            status = SNMP_BAD_PARAMETER;

        else
        {
            /* Initialize the System Group. */
            SNMP_sysDescr(sys_group->sysDescr);
            SNMP_sysObjectID(sys_group->sysObjectID);
            SNMP_sysContact(sys_group->sysContact);
            SNMP_sysLocation(sys_group->sysLocation);
            SNMP_sysServices(sys_group->sysServices);
        }

        initialized = 1;
    }
    else
        status = NU_NO_ACTION;

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* NU_SNMP_System_Group_Initialize */
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_SNMP_Set_Host_Id
*
*   DESCRIPTION
*
*       Sets the local IPv4 address for engine id resolution.
*
*   INPUTS
*
*       addr                    Pointer to local IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was set successfully.
*       NU_INVALID_PARM         IP address parameter is null.
*
*************************************************************************/
STATUS NU_SNMP_Set_Host_Id(const UINT8 *addr)
{
    NU_SUPERV_USER_VARIABLES

    if (addr == NU_NULL)
        return NU_INVALID_PARM;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set the local address into cfig_hostid. */
    cfig_hosttype = 1;
    NU_BLOCK_COPY(cfig_hostid, addr, IP_ADDR_LEN);

    /* return to user mode */
    NU_USER_MODE();

    return NU_SUCCESS;

} /* NU_SNMP_Set_Host_Id */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       NU_SNMP_Set_Host_Id6
*
*   DESCRIPTION
*
*       Sets the local IPv6 address for engine id resolution.
*
*   INPUTS
*
*       addr                    Pointer to local IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was set successfully.
*       NU_INVALID_PARM         IP address parameter is null.
*
*************************************************************************/
STATUS NU_SNMP_Set_Host_Id6(UINT8 *addr)
{
    NU_SUPERV_USER_VARIABLES

    if (addr == NU_NULL)
        return NU_INVALID_PARM;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set the local address into cfig_hostid. */
    cfig_hosttype = 2;
    NU_BLOCK_COPY(cfig_hostid, addr, IP6_ADDR_LEN);

    /* return to user mode */
    NU_USER_MODE();

    return NU_SUCCESS;

} /* NU_SNMP_Set_Host_Id6 */

#endif /* (INCLUDE_IPV6 == NU_TRUE) */
