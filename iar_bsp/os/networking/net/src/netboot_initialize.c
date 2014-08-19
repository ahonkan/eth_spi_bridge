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

/************************************************************************
*
*   FILENAME
*
*       netboot_initialize.c
*
* DESCRIPTION
*
*      This is the device generic portion of the net initialization code.
*
*      This code will initialize Nucleus Net and bring up all of the
*      network interfaces.
*
* DATA STRUCTURES
*
*       NET_IF_CHANGE_MSG
*
* FUNCTIONS
*
*       nu_os_net_stack_init
*       Register_NET_Devices_DM_Callbacks
*       Ethernet_Device_Register_Callback
*       Ethernet_Device_Unregister_Callback
*       Register_NET_Device_Discovery_Device
*       Wait_For_Net_Device_Discovery_Device
*       add_if_change_msg
*       lookup_if_change_msg
*       del_if_change_msg
*       NETBOOT_Link_Status_Task
*       NU_Ethernet_Link_Up
*       NETBOOT_Wait_For_LinkLocal_Addr
*       NU_Ethernet_Link_Down
*       NU_Test_Link_Up
*       NETBOOT_Resolve_DHCP4
*       NETBOOT_Obtain_Ipv6
*       NETBOOT_Obtain_Ipv4
*       NETBOOT_Log_Ipv4_Address
*       NETBOOT_Log_Ipv6_Address
*       NETBOOT_Wait_For_Network_Up
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       netboot_query.h
*       reg_api.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_networking.h"
#include "drivers/ethernet.h"
#include "drivers/serial.h"
#include "networking/netboot_query.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"
#include "os/kernel/plus/supplement/inc/event_notification.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nud6.h"
#endif

#ifdef CFG_NU_OS_SVCS_DBG_ADV_ENABLE
#include "services/dbg_adv_extr.h"
#endif

#ifdef CFG_NU_OS_NET_WPA_SUPP_ENABLE
#define NET_LINK_TASK_STACKSIZE  CFG_NU_OS_NET_STACK_LINK_TASK_STACK_SIZE + 2500
#else
#define NET_LINK_TASK_STACKSIZE  CFG_NU_OS_NET_STACK_LINK_TASK_STACK_SIZE
#endif
#define NET_LINK_TASK_PRIORITY   (TM_PRIORITY)


NU_EVENT_GROUP                   Net_Link_Up;
NU_TASK                          NETBOOT_Link_Status_Task_TCB;


/* Net Dynamic Device Discovery Virtual Driver Device ID. */
DV_DEV_ID Net_Device_Discovery_Driver_ID;

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
/* EQM Instances for NET. */
EQM_EVENT_QUEUE NET_Eqm;
#endif

#ifdef CFG_NU_OS_NET_WPA_SUPP_ENABLE
/* EQM Instance for WLAN. */
EQM_EVENT_QUEUE NET_WL_Event_Queue;
#endif

/* these defines are used to cleanup and deallocated resources in
   the initialization routine in case something fails. */
#define COMPLETED_STEP_1          1
#define COMPLETED_STEP_2          2
#define COMPLETED_STEP_3          3
#define COMPLETED_STEP_4          4
#define COMPLETED_STEP_5          5
#define COMPLETED_STEP_6          6

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_MEMORY_POOL           System_Memory;

#if (INCLUDE_IPV6 == NU_TRUE)
extern TQ_EVENT     ICMP6_RtrSol_Event;
extern UINT8   IP6_Solicited_Node_Multi[];
#endif

CHAR net_registry_path[REG_MAX_KEY_LENGTH] = {0};

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_FALSE)
STATIC STATUS Register_NET_Device_Discovery_Device(VOID);
STATIC STATUS Wait_For_Net_Device_Discovery_Device(VOID);
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
STATIC STATUS NETBOOT_Wait_For_LinkLocal_Addr(CHAR *, UINT8 *);
#endif
STATIC void add_if_change_msg(NET_IF_CHANGE_MSG *, NET_IF_CHANGE_MSG, UINT16);
STATIC INT lookup_if_change_msg(NET_IF_CHANGE_MSG *, DV_DEV_ID, UINT16);

VOID   NETBOOT_Link_Status_Task(UNSIGNED argc, VOID *argv);
VOID   NU_Printf(CHAR *string);
STATUS NETBOOT_Obtain_Ipv4(CHAR*, UINT8*, UINT8*);
STATUS NETBOOT_Resolve_DHCP4(CHAR *dev_name, UINT8*);
STATUS NETBOOT_Obtain_Ipv6(CHAR*, UINT8*, UINT8*);
STATUS NETBOOT_Wait_For_Network_Up(UNSIGNED suspend);

STATUS NETBOOT_Log_Ipv4_Address(CHAR *dev_name, UINT8 *ipaddr);
STATUS NETBOOT_Log_Ipv6_Address(CHAR *dev_name, UINT8 *ipaddr, UINT8 prefix_len);

/* Callback routines for ETHERNET_LABEL device register/unregister event. */
STATIC STATUS Ethernet_Device_Register_Callback(DV_DEV_ID device, VOID *context);
STATIC STATUS Ethernet_Device_Unregister_Callback(DV_DEV_ID device, VOID *context);

/* Callback registration function. */
STATIC STATUS Register_NET_Devices_DM_Callbacks(VOID);

/******************************************************************************
*
*   FUNCTION
*
*       nu_os_net_stack_init
*
*   DESCRIPTION
*
*       Initializes the tasks, queues and events used by the Middleware
*
******************************************************************************/
STATUS nu_os_net_stack_init(CHAR *path, INT startstop)
{
    VOID              *pointer1;
    STATUS            status = -1;
    UINT32            completed = 0;
    NU_NET_INIT_DATA  init_struct;
    NU_MEMORY_POOL    *usys_pool_ptr;

#if (CFG_NU_OS_NET_STACK_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.net.stack */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_NET_STACK);

#endif /* CFG_NU_OS_NET_STACK_EXPORT_SYMBOLS */

#if (CFG_NU_OS_NET_IPV6_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.net.stack */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_NET_IPV6);

#endif /* CFG_NU_OS_NET_IPV6_EXPORT_SYMBOLS */

    if (path)
    {
        /* save a copy locally. */
        strcpy(net_registry_path, path);

        if (startstop == RUNLEVEL_START)
        {
            /* Initialize the interface configuration list. */
            status = Ifconfig_Init();

            if (status == NU_SUCCESS)
            {
                /* The previous step completed successfully. */
                completed = COMPLETED_STEP_1;

                /* Initialize Net */

                /* Clear initialization structure */
                memset(&init_struct, 0, sizeof(init_struct));

                /* Decide which memory pool to use. */
                if (BUFS_IN_UNCACHED_MEM == NU_TRUE)
                {
                    /* Get system uncached memory pools pointer */
                    status = NU_System_Memory_Get(NU_NULL, &usys_pool_ptr);
                }
                else
                {
                    /* Get system cached memory pools pointer */
                    status = NU_System_Memory_Get(&usys_pool_ptr, NU_NULL);
                }

                if (status == NU_SUCCESS)
                {
                    /* Set initialization structure memory pointers */
                    init_struct.net_buffered_mem = usys_pool_ptr;
                }
                else
                {
                    init_struct.net_buffered_mem = NU_NULL;
                }

                init_struct.net_internal_mem = &System_Memory;


                status = NU_Init_Net(&init_struct);

                /* Log errors. */
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log ("Error at call to NU_Init_Net().\n", NERR_FATAL, __FILE__, __LINE__);
                }
            }


            if (status == NU_SUCCESS)
            {
                /* The previous step completed successfully. */
                completed = COMPLETED_STEP_2;

                /* Create event group for Net Link Up. */
                status = NU_Create_Event_Group(&Net_Link_Up, "NetLink");

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log ("Can not create NetLink Event Group.\n", NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /*******************************/
            /* Create the link status task */
            /*******************************/

            if (status == NU_SUCCESS)
            {
                /* The previous step completed successfully. */
                completed = COMPLETED_STEP_3;

                /* Allocate a stack for the NETBOOT_Link_Status_Task task. */
                status = NU_Allocate_Memory(&System_Memory, &pointer1, NET_LINK_TASK_STACKSIZE,
                                            NU_NO_SUSPEND);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log ("Cannot create memory for NETBOOT_Link_Status_task().\n", NERR_FATAL, __FILE__, __LINE__);
                }
            }


            if (status == NU_SUCCESS)
            {
                /* The previous step completed successfully. */
                completed = COMPLETED_STEP_4;

                /* Create the NETBOOT_Link_Status_Task task. */
                status = NU_Create_Task(&NETBOOT_Link_Status_Task_TCB, "NETLINK", NETBOOT_Link_Status_Task,
                                        0, NU_NULL, pointer1, NET_LINK_TASK_STACKSIZE,
                                        NET_LINK_TASK_PRIORITY, 0, NU_PREEMPT, NU_NO_START);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log ("Cannot create NETBOOT_Link_Status_task().\n", NERR_FATAL, __FILE__, __LINE__);
                }

                if (status == NU_SUCCESS)
                {
                    /* Start the task */
                    if (NU_Resume_Task(&NETBOOT_Link_Status_Task_TCB) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                                       __FILE__, __LINE__);
                }
            }

            /**********************************************/
            /* Register the net devices callbacks with DM */
            /**********************************************/

            if (status == NU_SUCCESS)
            {
                /* The previous step completed successfully. */
                completed = COMPLETED_STEP_5;

                /* Add the callback routines for net devices register/unregister events. */
                status = Register_NET_Devices_DM_Callbacks();
            }

            if (status != NU_SUCCESS)
            {
                /* If we are here we know that initialization
                   failed.  We need to cleanup and de-allocated
                   resources.  We do this only for the ones
                   that have been allocated, and in the reverse
                   order of allocation. */
                switch(completed)
                {
                    case COMPLETED_STEP_5:
                        /* Terminate the NETBOOT_Initialization_Task task */
                        NU_Terminate_Task(&NETBOOT_Link_Status_Task_TCB);

                        /* Delete the NETBOOT_Initialization_Task task */
                        NU_Delete_Task(&NETBOOT_Link_Status_Task_TCB);

                    case COMPLETED_STEP_4:
                        /* De-allocate stack memory for NETBOOT_Link_Status_Task */
                        NU_Deallocate_Memory(pointer1);

                    case COMPLETED_STEP_3:
                        /* Delete event group for Net Link Up. */
                        NU_Delete_Event_Group(&Net_Link_Up);

                    case COMPLETED_STEP_2:
                        /* De-allocate resources which were allocated by NU_Init_Net() */

                            /* Implementation TBD. */

                    case COMPLETED_STEP_1:
                        /* De-allocate resources which were allocated by Ifconfig_Init() */
                        Ifconfig_Deinit();

                    default:
                        break;
                }
            }
        }
        else if (startstop == RUNLEVEL_STOP)
        {
            /* Should de-initialize the net stack here. */

                /* Implementation TBD. */

            /* De-initialize the interface configuration list. */
            status = Ifconfig_Deinit();
        }
    }

    /* Trace log */
    T_NET_INIT_STAT(status);

    return (status);
}   /* nu_os_net_stack_init */

/*************************************************************************
*
*   FUNCTION
*
*       Register_NET_Devices_DM_Callbacks
*
*   DESCRIPTION
*
*       Registers callbacks for devices with ETHERNET label.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS                  Function returns success
*       Non success                 status returned from called functions.
*
*************************************************************************/
STATIC STATUS Register_NET_Devices_DM_Callbacks(VOID)
{
    STATUS             status = NU_SUCCESS;
    DV_DEV_LABEL       ethernet_device_label = {ETHERNET_LABEL};
    DV_LISTENER_HANDLE listener_id;

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_FALSE)
    /* Register a "device discovery pseudo device" so that we can
       send event notifications. */
    status = Register_NET_Device_Discovery_Device();

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log ("Cannot create NET Device Discovery pseudo device.\n", NERR_FATAL, __FILE__, __LINE__);
    }
#endif

    if (status == NU_SUCCESS)
    {
        /* Call DM API to add callbacks for Ethernet devices register
         * and unregister events. */
        status = DVC_Reg_Change_Notify(&ethernet_device_label,
                                       DV_GET_LABEL_COUNT(ethernet_device_label),
                                       &Ethernet_Device_Register_Callback,
                                       &Ethernet_Device_Unregister_Callback,
                                       NU_NULL,
                                       &listener_id);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Cannot register callbacks with DM for NET Devices.\n", NERR_FATAL, __FILE__, __LINE__);
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Device_Register_Callback
*
*   DESCRIPTION
*
*       Callback function for a new Ethernet device addition event.
*
*   INPUT
*
*       device                      Device ID of newly registered Ethernet
*                                   device.
*       context                     Context information for this callback.
*                                   Unused (null) for this component.
*
*   OUTPUT
*
*       NU_SUCCESS                  Function returns success.
*       Non success                 status returned from called functions.
*
*************************************************************************/
STATIC STATUS Ethernet_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS            status;   /* Return value from function call. */
    NU_DEVICE         lan_mw;
    DV_DEV_LABEL      eth_label = {ETHERNET_LABEL};
    DV_DEV_HANDLE     comp_sess_hd;
    DV_IOCTL0_STRUCT  dev_ioctl0;
    NET_IF_CHANGE_MSG if_change_msg;   /* Interface device change message. */

    /*****************************/
    /* Open the ethernet device. */
    /*****************************/

    comp_sess_hd = ETHERNET_Open(net_registry_path, device, 0);

    /* Get the "Ethernet" IOCTL base address. */
    dev_ioctl0.label = eth_label;
    status = DVC_Dev_Ioctl(comp_sess_hd, DV_IOCTL0,
                            &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

    if (status == NU_SUCCESS)
    {
        /* Get the "NU_DEVICE" structure from device. */
        status = DVC_Dev_Ioctl(comp_sess_hd,
                               dev_ioctl0.base + ETHERNET_CMD_GET_DEV_STRUCT,
                               (VOID*)&lan_mw,
                               sizeof(NU_DEVICE));
    }

    if (status == NU_SUCCESS)
        if_change_msg.dev_name = lan_mw.dv_name;
    else
        if_change_msg.dev_name = NU_NULL;

    /* Use EVENT manager to send this change to the
       link-up / link-down task. */
    if_change_msg.dev_id = device;
    if_change_msg.dev_hd = comp_sess_hd;
    if_change_msg.link_state = LINK_DN;
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    if_change_msg.event_type = NET_IF_CHANGE_ADD;

    /* Send a notification to listeners to inform about Net device status change. */
    NU_EQM_Post_Event(&NET_Eqm, (EQM_EVENT*)(&if_change_msg),
                                    sizeof(if_change_msg), NU_NULL);
#else
    NU_Notification_Send(Net_Device_Discovery_Driver_ID,
                         NET_IF_CHANGE_ADD,
                         &if_change_msg,
                         sizeof(if_change_msg));
#endif

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Device_Unregister_Callback
*
*   DESCRIPTION
*
*       Callback function for previously connected Net device removal event.
*
*   INPUT
*
*       device                      Device ID of newly unregistered Ethernet
*                                   device.
*       context                     Context information for this callback.
*                                   Unused (null) for this component.
*
*   OUTPUT
*
*       NU_SUCCESS                  Function returns success.
*
*************************************************************************/
STATIC STATUS Ethernet_Device_Unregister_Callback(DV_DEV_ID device, VOID *context)
{
    NET_IF_CHANGE_MSG if_change_msg;   /* Interface device change message. */

    if_change_msg.dev_id = device;
    if_change_msg.dev_hd = 0;

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    if_change_msg.event_type = NET_IF_CHANGE_DEL;

    /* Send a notification to listeners to inform about Net device status change. */
    NU_EQM_Post_Event(&NET_Eqm, (EQM_EVENT*)(&if_change_msg),
                                    sizeof(if_change_msg), NU_NULL);
#else
    /* Use EVENT manager to send this change. */
    NU_Notification_Send(Net_Device_Discovery_Driver_ID,
                         NET_IF_CHANGE_DEL,
                         &if_change_msg,
                         sizeof(if_change_msg));
#endif

    /* I.e., a device was removed. For instance a USB Device or a SDIO device. */
    NLOG_Error_Log ("A device wants to be unregistered.\n", NERR_INFORMATIONAL, __FILE__, __LINE__);

    return NU_SUCCESS;
}

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_FALSE)
/*************************************************************************
*
*   FUNCTION
*
*       Register_NET_Device_Discovery_Device
*
*   DESCRIPTION
*
*       This function registers Net's Device Discovery Service
*       with the DM so we can use the notification system
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS               Function returns success
*       NU_EN_INVALID_INPUT_PARAMS Invalid input parameters
*
*************************************************************************/
STATIC STATUS Register_NET_Device_Discovery_Device(VOID)
{
    STATUS status;
    DV_DEV_LABEL system_labels = {NET_DD_NOTIFICATIONS_LABEL};


    status = NU_Notification_Register(system_labels, &Net_Device_Discovery_Driver_ID);
    if (status == NU_SUCCESS)
    {
        if (Net_Device_Discovery_Driver_ID < 0)
        {
            status = -1;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Wait_For_Net_Device_Discovery_Device
*
*   DESCRIPTION
*
*       Wait for the Net dynamic device discovery pseudo driver to be
*       registered
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS               Function returns success
*       Non success status retuned from called functions.
*
*************************************************************************/
STATIC STATUS Wait_For_Net_Device_Discovery_Device(VOID)
{
    STATUS           status;
    DV_DEV_LABEL     dvfs_class_id = {NET_DD_NOTIFICATIONS_LABEL};

    /* Parameters for DVC_Reg_Change_Wait */
    INT              dev_reg_count   = 0;
    INT              dev_unreg_count = 0;
    DV_DEV_ID        dev_reg_list[CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED];
    DV_DEV_ID        dev_unreg_list[CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED];
    DV_DEV_ID        known_dev_id[CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED];
    INT              known_id_count  = 0;
    DV_APP_REGISTRY_CHANGE dev_state_info;


    memset(&dev_state_info, 0, sizeof(dev_state_info));


    /* Set up the device state info structure */
    dev_state_info.dev_label_list_ptr = &dvfs_class_id;
    dev_state_info.dev_label_cnt      = DV_GET_LABEL_COUNT(dvfs_class_id);
    dev_state_info.known_id_list_ptr  = known_dev_id;
    dev_state_info.known_id_cnt_ptr   = &known_id_count;
    dev_state_info.max_id_cnt         = 1;
    dev_state_info.reg_id_list_ptr    = dev_reg_list;
    dev_state_info.reg_id_cnt_ptr     = &dev_reg_count;
    dev_state_info.unreg_id_list_ptr  = dev_unreg_list;
    dev_state_info.unreg_id_cnt_ptr   = &dev_unreg_count;


    /* Wait for the Net dynamic device discovery pseudo driver to be registered */
    status = DVC_Reg_Change_Check(&dev_state_info, NU_SUSPEND);

    return status;
}
#endif /* #if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_FALSE) */


/*************************************************************************
*
*   FUNCTION
*
*       add_if_change_msg
*
*   DESCRIPTION
*
*       The link-up / link-down task needs to keep a local storage of the
*       device IDs and associated session handles, so it can perform IOCTLs
*       on the device.  IOCTLs require a session handle but the link-up /
*       link-down notification only provides the device ID.
*
*       This helper function adds a device ID, device handle pair to an
*       array of such pairs, limited by the max_msg parameter.
*
*       if it can't find room it just does nothing.
*
*   INPUT
*
*       if_change_msgs           base address of an array to store
*                                device ID / session handle pairs.
*       new_if_change_msg        new device IDs / session handle pair
*                                to add to the array.
*       max_msg                  size of the array.
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATIC void add_if_change_msg(NET_IF_CHANGE_MSG *if_change_msgs,
                       NET_IF_CHANGE_MSG new_if_change_msg, UINT16 max_msg)
{
    INT idx;

    for (idx = 0; idx < max_msg; idx++)
    {
        if (if_change_msgs[idx].dev_id == ~0)
        {
            if_change_msgs[idx] = new_if_change_msg;
            break;
        }
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       lookup_if_change_msg
*
*   DESCRIPTION
*
*       The link-up / link-down task needs to keep a local storage of the
*       device IDs and associated session handles, so it can perform IOCTLs
*       on the device.  IOCTLs require a session handle but the link-up /
*       link-down notification only provides the device ID.
*
*       This helper function looks up a device ID, device handle pair by
*       device ID and returns the index of the found pair or -1.
*
*
*   INPUT
*
*       if_change_msgs           base address of an array of device ID
*                                / session handle pairs.
*       dev_id                   Device ID to lookup
*       max_msg                  size of the array.
*
*   OUTPUT
*
*       Index of device ID / session handle in array if found,
*       otherwise -1.
*
*************************************************************************/
STATIC INT lookup_if_change_msg(NET_IF_CHANGE_MSG *if_change_msgs,
                                          DV_DEV_ID dev_id, UINT16 max_msg)
{
    INT idx;

    for (idx = 0; idx < max_msg; idx++)
    {
        if (if_change_msgs[idx].dev_id == dev_id)
        {
            return idx;
        }
    }

    return -1;
}


/*************************************************************************
*
*   FUNCTION
*
*       del_if_change_msg
*
*   DESCRIPTION
*
*       The link-up / link-down task needs to keep a local storage of the
*       device IDs and associated session handles, so it can perform IOCTLs
*       on the device.  IOCTLs require a session handle but the link-up /
*       link-down notification only provides the device ID.
*
*       This helper function deletes a device ID, device handle pair from
*       an array of such pairs, limited by the max_msg parameter.
*
*       The pair is looked up by dev_id and if found it is deleted.
*
*       If device ID not found, it is ignored.
*
*   INPUT
*
*       if_change_msgs           base address of an array of device ID
*                                / session handle pairs.
*       dev_id                   Device ID to lookup
*       max_msg                  size of the array.
*
*   OUTPUT
*
*       None
*
*************************************************************************/
void del_if_change_msg(NET_IF_CHANGE_MSG *if_change_msgs, DV_DEV_ID dev_id, UINT16 max_msg)
{
    INT idx;

    idx = lookup_if_change_msg(if_change_msgs, dev_id, max_msg);

    if (idx >= 0)
    {
        if_change_msgs[idx].dev_id = ~0;
        if_change_msgs[idx].dev_hd = ~0;
    }
}


/************************************************************************
*
*   FUNCTION
*
*      NETBOOT_Link_Status_Task
*
*   DESCRIPTION
*
*      This task monitors the link status of all network interfaces, and,
*      when notified, brings the specified link up or down as indicated.
*
*************************************************************************/
VOID NETBOOT_Link_Status_Task(UNSIGNED argc, VOID *argv)
{
    STATUS            status;
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    UINT32            requested_events_mask;
    EQM_EVENT_ID      recvd_event_id = 0;
    EQM_EVENT_HANDLE  recvd_event_handle;
#else
    VOID              *listen_handle;
    UINT8             notification_msg_len;
#endif
    DV_DEV_ID         notification_sender;
    UINT32            notification_type;
    VOID              *notification_msg;
    DV_DEV_LABEL      eth_label = {ETHERNET_LABEL};
    DV_IOCTL0_STRUCT  dev_ioctl0;

    /* Locally store all interface device change messages. */
    NET_IF_CHANGE_MSG if_change_msgs[CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED];

    /* Interface device change message. */
    NET_IF_CHANGE_MSG if_change_msg;
    INT           if_change_msgs_size;
    INT           idx;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);


    /* get the number of elements in the interface change message array. */
    if_change_msgs_size = sizeof(if_change_msgs)/sizeof(NET_IF_CHANGE_MSG);


    /* Initialize this array to all 1's.  This is because 0s are valid data. */
    memset(if_change_msgs, ~0, sizeof(if_change_msgs));

#ifdef CFG_NU_OS_NET_WPA_SUPP_ENABLE
    /* Create the WLAN Event Queue. */
    status = NU_EQM_Create(&NET_WL_Event_Queue, NET_WL_QUEUE_SIZE,
                            NET_WL_MAX_EVENT_DATA_SIZE, &System_Memory);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create WLAN Event Queue.",
                        NERR_FATAL, __FILE__, __LINE__);
    }
#endif

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    /* Create a queue for all messages */
    status = NU_EQM_Create(&NET_Eqm, NET_EQM_QUEUE_SIZE, MAX_EVENT_DATA_SIZE, &System_Memory);

    if (status == NU_SUCCESS)
    {
        status = NU_Allocate_Memory(&System_Memory, &notification_msg,
                                    sizeof(NET_IF_CHANGE_MSG), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Update the requested event mask, which are the events this task will monitor */
            requested_events_mask = NET_IF_CHANGE_ADD | NET_IF_CHANGE_DEL |
                                                                LINK_CHANGE_STATE;

            /* Loop here waiting for, getting and processing event notifications. */
            for (;;)
            {
                /* Wait for an event */
                NU_EQM_Wait_Event(&NET_Eqm, requested_events_mask,&notification_type,
                                            &recvd_event_id,&recvd_event_handle);

                /* Retrieve the event data sent */
                status = NU_EQM_Get_Event_Data(&NET_Eqm, recvd_event_id,
                                recvd_event_handle,(EQM_EVENT *)(notification_msg));

                if (status == NU_SUCCESS)
                {
                    notification_sender = ((NET_IF_CHANGE_MSG*)notification_msg)->dev_id;

                    switch (notification_type)
                    {
                        /* An interface has been added to the system, start listening for link-up / link-down events. */
                        case NET_IF_CHANGE_ADD:

                            memcpy(&if_change_msg, notification_msg, sizeof(NET_IF_CHANGE_MSG));

                            add_if_change_msg(if_change_msgs, if_change_msg, if_change_msgs_size);

                            /* First, get the IOCTL base. */
                            dev_ioctl0.label = eth_label;
                            status = DVC_Dev_Ioctl(if_change_msg.dev_hd, DV_IOCTL0,
                                                   &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

                            if (status == NU_SUCCESS)
                            {
                                /* Then, request a link status message. */
                                status = DVC_Dev_Ioctl(if_change_msg.dev_hd,
                                                       dev_ioctl0.base + ETHERNET_CMD_SEND_LINK_STATUS,
                                                       &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));
                            }
                            break;

                        /* An interface has been removed from the system, stop listening for link-up / link-down events. */
                        case NET_IF_CHANGE_DEL:

                            memcpy(&if_change_msg, notification_msg, sizeof(NET_IF_CHANGE_MSG));

                            /* Lookup the saved session handle by the device ID */
                            idx = lookup_if_change_msg(if_change_msgs, if_change_msg.dev_id, if_change_msgs_size);
                            if ((idx >= 0) && (idx < if_change_msgs_size))
                            {
                                /* If the link was up. */
                                if (if_change_msgs[idx].link_state == LINK_UP)
                                {
                                    /* If we found a session handle, use it to bring the link down. */
                                    status = NU_Ethernet_Link_Down(if_change_msgs[idx].dev_name);
                                    if (status == NU_SUCCESS)
                                    {
                                        /* We think the link is down. */
                                        if_change_msgs[idx].link_state = LINK_DN;
                                    }
                                }

                                /* Disassociate this device from the middleware. */
                                status = NU_Remove_Device(if_change_msgs[idx].dev_name, 0);
                                if (status != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Error at call to NU_Remove_Device().\n",
                                                    NERR_FATAL, __FILE__, __LINE__);
                                }
                            }

                            /* Clear the "link up" event. */
                            NU_Set_Events (&Net_Link_Up, 0, NU_AND);

                            /****************************************************************/
                            /* Remove this interface from the interface configuration list. */
                            /****************************************************************/
                            if ((idx >= 0) && (idx < if_change_msgs_size))
                            {
                                NU_Ifconfig_Delete_Interface(if_change_msgs[idx].dev_name);
                            }

                            del_if_change_msg(if_change_msgs, if_change_msg.dev_id, if_change_msgs_size);
                            break;

                        /* Process link-up / link-down events here. */
                        case LINK_CHANGE_STATE:

                            /***********/
                            /* Link Up */
                            /***********/
                            if (strcmp(((NET_IF_LINK_STATE_MSG*)notification_msg)->msg, "LINK UP") == 0)
                            {
                                /* Lookup the saved session handle by the device ID */
                                idx = lookup_if_change_msg(if_change_msgs, notification_sender, if_change_msgs_size);
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    /* If the link was down */
                                    if (if_change_msgs[idx].link_state == LINK_DN)
                                    {
                                        /* If we found a session handle, use it to bring the link up. */
                                        status = NU_Ethernet_Link_Up(if_change_msgs[idx].dev_name);
                                        if (status == NU_SUCCESS)
                                        {
                                            /* We think the link is up. */
                                            if_change_msgs[idx].link_state = LINK_UP;
                                        }
                                    }
                                }
                            }

                            /*************/
                            /* Link Down */
                            /*************/
                            else if (strcmp(((NET_IF_LINK_STATE_MSG*)notification_msg)->msg, "LINK DOWN") == 0)
                            {
                                /* Lookup the saved session handle by the device ID */
                                idx = lookup_if_change_msg(if_change_msgs, notification_sender, if_change_msgs_size);
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    /* If the link was up */
                                    if (if_change_msgs[idx].link_state == LINK_UP)
                                    {
                                        /* If we found a session handle, use it to bring the link down. */
                                        status = NU_Ethernet_Link_Down(if_change_msgs[idx].dev_name);
                                        if (status == NU_SUCCESS)
                                        {
                                            /* We think the link is down. */
                                            if_change_msgs[idx].link_state = LINK_DN;
                                        }
                                    }
                                }
                            }
                            break;

                    } /* end switch */
                }
            } /* end for */
        }
    }
#else
    Wait_For_Net_Device_Discovery_Device();

    /* Create a queue for all messages */
    listen_handle = NU_Notification_Queue_Create();

    if (listen_handle != NU_NULL)
    {

        /* Start Listening to link state change notifications */
        status = NU_Notification_Listen_Start(listen_handle, Net_Device_Discovery_Driver_ID,
                                              0, 0);

        if (status == NU_SUCCESS)
        {

            /* Allocate space for the notification message */
            status = NU_Allocate_Memory(&System_Memory, &notification_msg, EN_MAX_MSG_LEN, NU_NO_SUSPEND);

            /* Loop here waiting for, getting and processing event notifications. */
            for (;;)
            {
                /* Get notifications, reset the max size on each iteration */
                notification_msg_len = EN_MAX_MSG_LEN;
                status = NU_Notification_Get(listen_handle, &notification_sender, &notification_type,
                                             notification_msg, &notification_msg_len, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {

                    switch (notification_type)
                    {

                        /* An interface has been added to the system, start listening for link-up / link-down events. */
                        case NET_IF_CHANGE_ADD:
                            if (notification_msg_len == sizeof(NET_IF_CHANGE_MSG))
                            {
                                memcpy(&if_change_msg, notification_msg, sizeof(NET_IF_CHANGE_MSG));

                                add_if_change_msg(if_change_msgs, if_change_msg, if_change_msgs_size);

                                /* Start Listening to link state change notifications */
                                status = NU_Notification_Listen_Start(listen_handle, if_change_msg.dev_id,
                                                                      LINK_CHANGE_STATE, LINK_CHANGE_STATE);

                                /* Tell the driver to send us its link status message. */
                                if (status == NU_SUCCESS)
                                {
                                    /* First, get the IOCTL base. */
                                    dev_ioctl0.label = eth_label;
                                    status = DVC_Dev_Ioctl(if_change_msg.dev_hd, DV_IOCTL0,
                                                           &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));
                                }

                                if (status == NU_SUCCESS)
                                {
                                    /* Then, request a link status message. */
                                    status = DVC_Dev_Ioctl(if_change_msg.dev_hd,
                                                           dev_ioctl0.base + ETHERNET_CMD_SEND_LINK_STATUS, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));
                                }

                            }
                            break;

                        /* An interface has been removed from the system, stop listening for link-up / link-down events. */
                        case NET_IF_CHANGE_DEL:
                            if (notification_msg_len == sizeof(NET_IF_CHANGE_MSG))
                            {
                                memcpy(&if_change_msg, notification_msg, sizeof(NET_IF_CHANGE_MSG));

                                /* Lookup the saved session handle by the device ID */
                                idx = lookup_if_change_msg(if_change_msgs, if_change_msg.dev_id, if_change_msgs_size);
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    /* If the link was up. */
                                    if (if_change_msgs[idx].link_state == LINK_UP)
                                    {
                                        /* If we found a session handle, use it to bring the link down. */
                                        status = NU_Ethernet_Link_Down(if_change_msgs[idx].dev_name);
                                        if (status == NU_SUCCESS)
                                        {
                                            /* We think the link is down. */
                                            if_change_msgs[idx].link_state = LINK_DN;
                                        }
                                    }

                                    /* Disassociate this device from the middleware. */
                                    status = NU_Remove_Device(if_change_msgs[idx].dev_name, 0);
                                    if (status != NU_SUCCESS)
                                    {
                                        NLOG_Error_Log("Error at call to NU_Remove_Device().\n",
                                                        NERR_FATAL, __FILE__, __LINE__);
                                    }
                                }

                                /* Stop Listening to link state change notifications */
                                status = NU_Notification_Listen_Stop (listen_handle, if_change_msg.dev_id);

                                /* Clear the "link up" event. */
                                NU_Set_Events (&Net_Link_Up, 0, NU_AND);


                                /****************************************************************/
                                /* Remove this interface from the interface configuration list. */
                                /****************************************************************/
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    NU_Ifconfig_Delete_Interface(if_change_msgs[idx].dev_name);
                                }

                                del_if_change_msg(if_change_msgs, if_change_msg.dev_id, if_change_msgs_size);
                            }
                            break;

                        /* Process link-up / link-down events here. */
                        case LINK_CHANGE_STATE:

                            /***********/
                            /* Link Up */
                            /***********/
                            if (strcmp(notification_msg, "LINK UP") == 0)
                            {
                                /* Lookup the saved session handle by the device ID */
                                idx = lookup_if_change_msg(if_change_msgs, notification_sender, if_change_msgs_size);
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    /* If the link was down */
                                    if (if_change_msgs[idx].link_state == LINK_DN)
                                    {
                                        /* If we found a session handle, use it to bring the link up. */
                                        status = NU_Ethernet_Link_Up(if_change_msgs[idx].dev_name);
                                        if (status == NU_SUCCESS)
                                        {
                                            /* We think the link is up. */
                                            if_change_msgs[idx].link_state = LINK_UP;
                                        }
                                    }
                                }
                            }


                            /*************/
                            /* Link Down */
                            /*************/
                            if (strcmp(notification_msg, "LINK DOWN") == 0)
                            {
                                /* Lookup the saved session handle by the device ID */
                                idx = lookup_if_change_msg(if_change_msgs, notification_sender, if_change_msgs_size);
                                if ((idx >= 0) && (idx < if_change_msgs_size))
                                {
                                    /* If the link was up */
                                    if (if_change_msgs[idx].link_state == LINK_UP)
                                    {
                                        /* If we found a session handle, use it to bring the link down. */
                                        status = NU_Ethernet_Link_Down(if_change_msgs[idx].dev_name);
                                        if (status == NU_SUCCESS)
                                        {
                                            /* We think the link is down. */
                                            if_change_msgs[idx].link_state = LINK_DN;
                                        }
                                    }
                                }
                            }
                            break;

                    } /* end switch */
                }
            } /* end for */
        }
    }
#endif /* #if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)*/
    /* Switch back to user mode. */
    NU_USER_MODE();

} /* NETBOOT_Link_Status_Task */


/*************************************************************************
*
*   FUNCTION
*
*       NU_Ethernet_Link_Up
*
*   DESCRIPTION
*
*       Bring the link up on the specified interface device.
*
*   INPUTS
*
*       CHAR           *dev_name            - Ethernet device name
*
*   RETURNS
*
*        NU_SUCCESS                The link was successfully brought down.
*       NU_INVALID_PARM            dev_name is NULL or does not refer to
*                               a valid interface in the system.
*
*************************************************************************/
STATUS NU_Ethernet_Link_Up(CHAR *dev_name)
{
    STATUS            status = NU_SUCCESS, local_status;
    UINT8             ip_addr[MAX_ADDRESS_SIZE] = {0};
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8             subnet[IP_ADDR_LEN] = {0,0,0,0};
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8             prefix_len;
    DV_DEVICE_ENTRY   *dev_ptr = NU_NULL;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* We MUST have a valid name to continue. */
    if ( (dev_name == NU_NULL) || (strlen(dev_name) == 0) ||
         (NU_Ifconfig_Validate_Interface(dev_name) == NU_FALSE) )
    {
        NLOG_Error_Log("Error in NU_Ethernet_Link_Up device has no name.\n",
                        NERR_FATAL, __FILE__, __LINE__);

        status = NU_INVALID_PARM;
    }

    /**********************************/
    /* Configure the ethernet device. */
    /**********************************/

    /***********************************/
    /* Obtain or Install IPv4 address. */
    /***********************************/
#if (INCLUDE_IPV4 == NU_TRUE)
    if (status == NU_SUCCESS)
    {
        /* Is this interface configured for IPv4? */
        if (NU_Ifconfig_Get_IPv4_Enabled(dev_name) == NU_TRUE)
        {
            /* If IPv4 is included in the build and the interface is configured
             * to use IPv4, then an IPv4 address is REQUIRED.  This address can
             * be obtained either via DHCP and/or through configuration setting in
             * the registry.
             */

            /* Obtain the manually configured IPv4 address.  This should be done even if
             * DHCP is being used since the user could have configured a manual address
             * on the interface.
             */
            local_status = NU_Ifconfig_Get_Ipv4_Address(dev_name, ip_addr, subnet);

            /* An IP address may not have been returned, because no address is configured
             * on the interface.  This is not a fatal error and should not affect
             * forward progress through this function.
             */
            while (local_status == NU_SUCCESS)
            {
                /* Attach the IP address to this device. */
                local_status = NU_Attach_IP_To_Device(dev_name, ip_addr, subnet);

                /* The address was successfully attached. */
                if (local_status == NU_SUCCESS)
                {
                    /* Log the IP address */
                    NETBOOT_Log_Ipv4_Address(dev_name, ip_addr);
                }

                else
                {
                    NLOG_Error_Log("Configured address could not be attached to interface.\n",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    break;
                }

                /* Get the next address on the interface. */
                local_status = NU_Ifconfig_Get_Ipv4_Address(dev_name, ip_addr, subnet);
            }

            /* Clear the address information. */
            memset(ip_addr, 0, MAX_ADDRESS_SIZE);
            memset(subnet, 0, IP_ADDR_LEN);

#if CFG_NU_OS_NET_STACK_INCLUDE_DHCP
            /* Is the interface configured to use DHCP? */
            if (NU_Ifconfig_Get_DHCP4_Enabled(dev_name) == NU_TRUE)
            {
                /* Obtain IPv4 address from DHCP server */
                local_status = NETBOOT_Resolve_DHCP4(dev_name, ip_addr);

                /* If we are here and status is success we have
                   configured an IPv4 interface. */
                if (local_status == NU_SUCCESS)
                {
                    /* Log the IP address */
                    NETBOOT_Log_Ipv4_Address(dev_name, ip_addr);
                }
                /* Log errors. */
                else
                {
                    NLOG_Error_Log("Error at call to NETBOOT_Resolve_DHCP4().\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }

                /* Trace log */
                T_DEV_IPv4_DHCP_IP(dev_name, ip_addr, status, 4);

                /* Clear the address information. */
                memset(ip_addr, 0, MAX_ADDRESS_SIZE);
            }

#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP */
        }
    }
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

    /******************************************/
    /* Initialize Nucleus IPv6 if configured */
    /******************************************/
#if (INCLUDE_IPV6 == NU_TRUE)
    if (status == NU_SUCCESS)
    {
        /* Is this interface configured for IPv6? */
        if (NU_Ifconfig_Get_IPv6_Enabled(dev_name) == NU_TRUE)
        {
            /* If IPv6 is included in the build and the interface is configured
             * to use IPv6, then an IPv6 link-local address will be generated
             * automatically when the device first boots up.  In addition to the
             * link-local address, a global address can be obtained either via
             * stateless address autoconfiguration, DHCP6, or through a
             * configuration setting in the registry.
             */

            /* Clear the address information. */
            memset(ip_addr, 0, MAX_ADDRESS_SIZE);

            local_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            if (local_status == NU_SUCCESS)
            {
                dev_ptr = DEV_Get_Dev_By_Name(dev_name);

                if (dev_ptr != NU_NULL)
                {
                    /* Create a link-local address and invoke DAD.  Also, invoke
                     * stateless address autoconfiguration to obtain a globally
                     * unique IPv6 address.
                     */
                    local_status = DEV6_AutoConfigure_Device(dev_ptr);
                    if (local_status == NU_SUCCESS)
                    {
                        NUD6_Init(dev_ptr);
                    }
                }

                else
                {
                    local_status = -1;
                }

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
                }

                if (local_status == NU_SUCCESS)
                {
                    /* Wait for the link-local address to be configured. */
                    local_status = NETBOOT_Wait_For_LinkLocal_Addr(dev_name, ip_addr);
                }
            }

            /* If we are here and status is success we have configured an IPv6 address. */
            if (local_status == NU_SUCCESS)
            {
                /* Log the IP address. Prefix-length is always 64. */
                NETBOOT_Log_Ipv6_Address(dev_name, ip_addr, 64);
            }

            else
            {
                /* Log errors. */
                NLOG_Error_Log ("Could not configure link-local address.\n", NERR_FATAL, __FILE__, __LINE__);
            }

#if CFG_NU_OS_NET_IPV6_INCLUDE_DHCP6
            /* Is the interface configured to use DHCP? */
            if (NU_Ifconfig_Get_DHCP6_Enabled(dev_name) == NU_TRUE)
            {
                local_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                if (local_status == NU_SUCCESS)
                {
                    dev_ptr = DEV_Get_Dev_By_Name(dev_name);

                    /* Set the event to invoke stateful configuration in the DHCPv6
                     * client module.
                     */
                    if (EQ_Put_Event(DHCP6_Stateful_Config_Event, dev_ptr->dev_index,
                                     0) != NU_SUCCESS)
                    {
                            NLOG_Error_Log("Failed to set event to invoke stateful configuration",
                                NERR_SEVERE, __FILE__, __LINE__);
                    }

                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                       __FILE__, __LINE__);
                    }
                }
            }

#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP6 */

            /* Clear the address information. */
            memset(ip_addr, 0, MAX_ADDRESS_SIZE);

            /* Check if there is a globally unique IPv6 address assigned to the
             * interface via stateless address autoconfiguration or DHCPv6.
             */
            local_status = NETBOOT_Obtain_Ipv6(dev_name, ip_addr, &prefix_len);

            /* If a global address is not yet configured. */
            if ( (local_status != NU_SUCCESS) || (IPV6_IS_ADDR_LINKLOCAL(ip_addr)) )
            {
                /* Let autoconfiguration complete. */
                NU_Sleep(IP6_MAX_RTR_SOLICITATION_DELAY * 2);

                /* Check for the address again.  If DHCP was not used to obtain a
                 * global address, autoconfiguration should be complete by now.
                 */
                local_status = NETBOOT_Obtain_Ipv6(dev_name, ip_addr, &prefix_len);
            }

            /* If a global IPv6 address was autoconfigured or obtained via DHCP. */
            if ( (local_status == NU_SUCCESS) && (!(IPV6_IS_ADDR_LINKLOCAL(ip_addr))) )
            {
                /* Log the IP address. */
                NETBOOT_Log_Ipv6_Address(dev_name, ip_addr, prefix_len);
            }

            /* Clear the address information. */
            memset(ip_addr, 0, MAX_ADDRESS_SIZE);

            /* If there is a manually configured IPv6 address, get it. */
            local_status = NU_Ifconfig_Get_Ipv6_Address(dev_name, ip_addr, &prefix_len);

            if (local_status == NU_SUCCESS)
            {
                /* Log the IP address */
                NETBOOT_Log_Ipv6_Address(dev_name, ip_addr, prefix_len);

                /* Configure the IPv6 address on the interface. */
                local_status = NU_Add_IP_To_Device(dev_name, ip_addr, prefix_len,
                                                   0xffffffff, 0xffffffff);
                /* Log errors. */
                if (local_status == NU_SUCCESS)
                {
                    NETBOOT_Log_Ipv6_Address(dev_name, ip_addr, prefix_len);
                }

                else
                {
                    NLOG_Error_Log ("Error at call to NU_Add_IP_To_Device().\n", NERR_FATAL, __FILE__, __LINE__);
                }
            }
        }
    }
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

    /************************************************/
    /* Indicate that NET initialization is complete */
    /************************************************/
    if (status == NU_SUCCESS)
    {
        NU_Set_Events(&Net_Link_Up, NU_TRUE, NU_OR);
    }

    /* Trace log */
    T_DEV_LINK_UP(dev_name, status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Ethernet_Link_Up */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       NETBOOT_Wait_For_LinkLocal_Addr
*
*   DESCRIPTION
*
*       Checks if link-local autoconfiguration has been invoked on the
*       interface, and if so, waits for the address to be validated or
*       marked as a duplicate.
*
*   INPUTS
*
*       dev_name                Name of the interface.
*       addr                    Pointer to be filled in with the address.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATIC STATUS NETBOOT_Wait_For_LinkLocal_Addr(CHAR *dev_name, UINT8 *addr)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_ptr = NU_NULL;
    DEV6_IF_ADDRESS     *dev_addr = NU_NULL;

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        dev_ptr = DEV_Get_Dev_By_Name(dev_name);

        if (dev_ptr != NU_NULL)
        {
            /* Get the first address in the list of addresses for the device */
            dev_addr = dev_ptr->dev6_addr_list.dv_head;

            /* Search through the list of addresses for the link-local address. */
            while (dev_addr)
            {
                /* If this is the link-local address. */
                if (IPV6_IS_ADDR_LINKLOCAL(dev_addr->dev6_ip_addr))
                {
                    /* Return the address even if an error is returned. */
                    memcpy(addr, dev_addr->dev6_ip_addr, MAX_ADDRESS_SIZE);

                    /* If it is still tentative, wait for duplicate address
                     * detection to complete.
                     */
                    while (dev_addr->dev6_addr_state & DV6_TENTATIVE)
                    {
                        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                           __FILE__, __LINE__);
                        }

                        NU_Sleep(1 * TICKS_PER_SECOND);

                        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

                        if (status != NU_SUCCESS)
                        {
                            break;
                        }
                    }

                    /* If this is a duplicate address, set an error. */
                    if (dev_addr->dev6_addr_state & DV6_DUPLICATED)
                    {
                        status = -1;
                    }

                    break;
                }

                dev_addr = dev_addr->dev6_next;
            }

            /* No link-local address was found. */
            if (!dev_addr)
            {
                status = -1;
            }
        }

        else
        {
            status = -1;
        }

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    return (status);

} /* NETBOOT_Wait_For_LinkLocal_Addr */
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ethernet_Link_Down
*
*   DESCRIPTION
*
*       Bring the link down on the specified interface device.
*
*   INPUTS
*
*       CHAR           *dev_name            - Ethernet device name
*
*   RETURNS
*
*        NU_SUCCESS                The link was successfully brought down.
*       NU_INVALID_PARM            dev_name is NULL or does not refer to
*                               a valid interface in the system.
*
*************************************************************************/
STATUS NU_Ethernet_Link_Down(CHAR *dev_name)
{
    STATUS           status   = NU_SUCCESS;
#if CFG_NU_OS_NET_STACK_INCLUDE_DHCP
    DV_DEVICE_ENTRY  *dev_ptr = NU_NULL;
    UINT32           dhcp_addr = 0;
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* We MUST have a name to continue. */
    if ((dev_name == NU_NULL) || (strlen(dev_name) == 0))
    {
        NLOG_Error_Log("Error in NU_Ethernet_Link_Down device has no name.\n",
                        NERR_FATAL, __FILE__, __LINE__);

        status = NU_INVALID_PARM;
    }

#if ( (CFG_NU_OS_NET_STACK_INCLUDE_DHCP) || ((INCLUDE_IPV6) && (CFG_NU_OS_NET_IPV6_INCLUDE_DHCP6)) )
    /* Note that currently dev_ptr is only needed for DHCP so
       this block is conditionally compiled, but it could be
       needed by other logic in the future */

    /* Get a pointer to the device structure by name.
       This requires obtaining the semaphore. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get the device by name */
            dev_ptr = DEV_Get_Dev_By_Name(dev_name);

            if (dev_ptr)
            {
#if (INCLUDE_IPV6 == NU_TRUE)
#if CFG_NU_OS_NET_IPV6_INCLUDE_DHCP6
                /* Set the flag indicating that no DHCPv6 release message should be sent. */
                dev_ptr->dev6_flags |= DV6_NO_DHCP_RELEASE;
#endif
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP6 */

                /* Store the value of the 32-bit IPv4 DHCP address. */
                dhcp_addr = dev_ptr->dev_addr.dev_dhcp_addr;
            }

            else
            {
                status = NU_INVALID_PARM;
                NLOG_Error_Log("No matching interface found", NERR_INFORMATIONAL,
                               __FILE__, __LINE__);
            }

            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }
    }
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP */

    if (status == NU_SUCCESS)
    {

#if CFG_NU_OS_NET_STACK_INCLUDE_DHCP
#if (INCLUDE_IPV4 == NU_TRUE)
        /* NU_DHCP_Release must be used to delete an address
         * obtained via DHCP from an interface.
         */
        if (dhcp_addr != 0)
        {
            /* Disable the given interface by detaching the DHCP
             * IP address of the interface. */
            status = DHCP_Release_Address(NU_NULL, dev_name, NU_FALSE);
        }
#endif
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP */

        /* We must grab the NET semaphore */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Disable the given interface by detaching all manually configured
             * IP addresses from the interface.  Do not use the API routine since
             * the API routine will also remove the cached addresses.
             */
            status = DEV_Detach_IP_From_Device(dev_name);

#if (INCLUDE_IPV6 == NU_TRUE)
#if CFG_NU_OS_NET_IPV6_INCLUDE_DHCP6
            /* Get the device by name */
            dev_ptr = DEV_Get_Dev_By_Name(dev_name);

            if (dev_ptr)
            {
                /* Clear the flag that was previously set. */
                dev_ptr->dev6_flags &= ~DV6_NO_DHCP_RELEASE;
            }
#endif
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP6 */

            /* Release the semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                            __FILE__, __LINE__);
        }
    }

    /* Clear the "link up" event. */
    NU_Set_Events (&Net_Link_Up, 0, NU_AND);

    /* Trace log */
    T_DEV_LINK_DOWN(dev_name, status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Test_Link_Up
*
*   DESCRIPTION
*
*       Return TRUE if the link associated with the device session handle is up.
*
*   INPUTS
*
*       CHAR           *dev_name            - Ethernet device name
*
*   OUTPUTS
*
*       UINT32         *state               - Link state: 1=up, 0=down
*
*   RETURNS
*
*        NU_SUCCESS                The link was successfully brought down.
*       NU_INVALID_PARM            dev_name is NULL or does not refer to
*                               a valid interface in the system.
*
*************************************************************************/
STATUS NU_Test_Link_Up(CHAR *dev_name, UINT32 *state)
{
    STATUS            status    = NU_SUCCESS;
    DV_DEVICE_ENTRY   *dev_ptr  = NU_NULL;
    DV_DEV_LABEL      eth_label = {ETHERNET_LABEL};
    DV_IOCTL0_STRUCT  dev_ioctl0;
    UINT32            value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* We MUST have a name to continue. */
    if ((dev_name == NU_NULL) || (strlen(dev_name) == 0) || (state == NU_NULL))
    {
        NLOG_Error_Log("Error in NU_Test_Link_Up device has no name.\n",
                        NERR_FATAL, __FILE__, __LINE__);

        status = NU_INVALID_PARM;
    }

    /* Get a pointer to the device structure by name.
       This requires obtaining the semaphore. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get the device by name */
            dev_ptr = DEV_Get_Dev_By_Name(dev_name);

            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            if (dev_ptr == NU_NULL)
            {
                status = NU_INVALID_PARM;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Get the IOCTL base. */
        dev_ioctl0.label = eth_label;
        status = DVC_Dev_Ioctl(dev_ptr->dev_handle,
                               DV_IOCTL0,
                               &dev_ioctl0,
                               sizeof(DV_IOCTL0_STRUCT));
    }

    if (status == NU_SUCCESS)
    {
        /* Request the link status. */
        status = DVC_Dev_Ioctl(dev_ptr->dev_handle,
                               dev_ioctl0.base + ETHERNET_CMD_GET_LINK_STATUS,
                               (VOID *)&value,
                               sizeof(value));
    }

    if (status == NU_SUCCESS)
    {
        /* Set the link state return value. */
        *state = value;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

   return status;
} /* NU_Test_Link_Up */

/******************************************************************************
* FUNCTION
*
*      NETBOOT_Resolve_DHCP4
*
* DESCRIPTION
*
*      Obtain IPv4 address from a remote DHCP server.
*
******************************************************************************/
#if CFG_NU_OS_NET_STACK_INCLUDE_DHCP
STATUS NETBOOT_Resolve_DHCP4(CHAR *dev_name, UINT8 *local)
{
    STATUS              status;
    IFCONFIG_NODE       *ifconfig_ptr;

    /* These are the dhcp options desired. Each option is specified by three
       items, an option id, an option length, and an option value. The following
       specifies two options. The first option is to request that the DHCP
       server return some specific parameters. There are three such parameters
       (the length). The three requested are DHCP_MASK, DHCP_ROUTE, and
       DHCP_DNS.  The second option, DHCP_HOSTNAME, is to specify a name that
       will be sent to the server. The string length and host name will be
       filled dynamically. */

    UINT8 dhcp_options[7 + MAX_HOST_NAME_LENGTH + 1] =
          {DHCP_REQUEST_LIST, 3, DHCP_NETMASK, DHCP_ROUTE, DHCP_DNS, DHCP_HOSTNAME, 0};

    /* Size of dhcp_options array before hostname entered. */
    UINT8 dhcp_options_length = 7;

    /* Get the host name. */
    status = NU_Get_Host_Name((CHAR *)&dhcp_options[7], MAX_HOST_NAME_LENGTH);

    if (status != NU_SUCCESS)
    {
        memcpy(&dhcp_options[7], "DHCPCLIENT", strlen("DHCPCLIENT") + 1);
    }
    else
    {
        /* Add null character manually because NU_Get_Host_Name might not
           add a null character . Adding null character at the end of the
           dhcp_options array is not a bug in the code. */
        dhcp_options[7 + MAX_HOST_NAME_LENGTH ] = '\0';
    }

    /* Fill the string length of hostname. Adding 1 because strlen does
       not count the null character. */
    dhcp_options[6] = strlen((CHAR *)&dhcp_options[7]) + 1;

    /* Total size of dhcp_options array. */
    dhcp_options_length += dhcp_options[6];

    /* Get a pointer to the cached interface structure. */
    status = Ifconfig_Find_Interface(dev_name, &ifconfig_ptr);

    if ( (status == NU_SUCCESS) && (ifconfig_ptr) )
    {
        /* Initialize DHCP fields to zero value */
        memset(&ifconfig_ptr->ifconfig_dhcp, 0, sizeof(DHCP_STRUCT));

        /* Specify the DHCP options desired. */
        ifconfig_ptr->ifconfig_dhcp.dhcp_opts = dhcp_options;
        ifconfig_ptr->ifconfig_dhcp.dhcp_opts_len = dhcp_options_length;

        /* dhcp_ptr struct above is left blank unless the caller wanted to pass
         * requests to the DHCP server.
         */

        status = NU_Dhcp(&ifconfig_ptr->ifconfig_dhcp, dev_name);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Error at call to NU_Dhcp().\n", NERR_FATAL, __FILE__, __LINE__);
        }

        if ( status == NU_SUCCESS)
        {
            /* Copy the address obtained via DHCP into the local variable. */
            memcpy(local, ifconfig_ptr->ifconfig_dhcp.dhcp_yiaddr, IP_ADDR_LEN);
        }

        /* Do not maintain a pointer to this local memory. */
        ifconfig_ptr->ifconfig_dhcp.dhcp_opts = NU_NULL;
        ifconfig_ptr->ifconfig_dhcp.dhcp_opts_len = 0;
    }

    return status;

} /* NETBOOT_Resolve_DHCP4 */
#endif /* CFG_NU_OS_NET_STACK_INCLUDE_DHCP */

#if (INCLUDE_IPV6 == NU_TRUE)
/******************************************************************************
* FUNCTION
*
*      NETBOOT_Obtain_Ipv6
*
* DESCRIPTION
*
*      Obtain the local and remote IPv6 addresses.
*
******************************************************************************/
STATUS NETBOOT_Obtain_Ipv6(CHAR *dev_name, UINT8 *local, UINT8 *prefix_len)
{
    STATUS              status;
    SCK_IOCTL_OPTION    option;

    /* Get the local address from the device structure. */
    option.s_optval = (UINT8*)dev_name;

    status = NU_Ioctl_SIOCGIFADDR_IN6(&option, sizeof(SCK_IOCTL_OPTION));

    if (status == NU_SUCCESS)
    {
        /* Return the prefix length. */
        *prefix_len = option.s_optval_octet;

        /* Copy it to the caller's location. */
        memcpy(local, option.s_ret.s_ipaddr, 16);
    }

    else
    {
        NLOG_Error_Log ("NETBOOT_Obtain_Ipv6: Failed to obtain local IPv6 address.\n", NERR_FATAL, __FILE__, __LINE__);
    }

    return status;
}
#endif  /* (INCLUDE_IPV6 == NU_TRUE) */


/******************************************************************************
* FUNCTION
*
*      NETBOOT_Obtain_Ipv4
*
* DESCRIPTION
*
*      Obtain the local and remote IPv4 addresses.
*
******************************************************************************/
STATUS NETBOOT_Obtain_Ipv4(CHAR *dev_name, UINT8 *local, UINT8 *remote)
{
    STATUS              status;
    SCK_IOCTL_OPTION    option;

    /* Get the local address from the device structure. */
    option.s_optval = (UINT8*)dev_name;

    status = NU_Ioctl_SIOCGIFADDR(&option, sizeof(SCK_IOCTL_OPTION));

    if (status == NU_SUCCESS)
    {
        /* Copy it to the caller's location. */
        memcpy(local, option.s_ret.s_ipaddr, 4);
    }

    else
    {
        NLOG_Error_Log ("NETBOOT_Obtain_Ipv4: Failed to obtain local IPv4 address.\n", NERR_FATAL, __FILE__, __LINE__);
    }

    return status;
}



/******************************************************************************
* FUNCTION
*
*      NETBOOT_Log_Ipv4_Address
*
* DESCRIPTION
*
*      Log the IPv4 addresses.
*
******************************************************************************/
STATUS NETBOOT_Log_Ipv4_Address(CHAR *dev_name, UINT8 *ipaddr)
{
    STATUS status = -1;
    CHAR   buf[80];
    UINT16 room;
    UINT16 size;

    if (dev_name && ipaddr)
    {
        buf[0] = 0;
        size = sizeof(buf) - 1;
        strcat(buf, "Device ");
        room = size - strlen(buf);
        if (strlen(dev_name) < room)
        {
            strcat(buf, dev_name);
            room = size - strlen(buf);
        }

        if(room >= 15)
        {
            strcat(buf, "  IPv4 address ");
            room = size - strlen(buf);
        }

        if (room >= 16)
        {
            /* Convert the destination IP addr to ASCII */
            status = NU_Inet_NTOP(NU_FAMILY_IP, ipaddr, &buf[strlen(buf)], 16);
            room = size - strlen(buf);
        }

        if (status == NU_SUCCESS)
        {
            NLOG_Error_Log (buf, NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

#ifdef CFG_NU_OS_SVCS_DBG_ADV_ENABLE
        /* If we are using the Agent debugging network advertiser, send it
           our IP address. */
        (VOID) DBG_ADV_Send_To_Queue(ipaddr, 4);
#endif
    }

    return status;
}



#if (INCLUDE_IPV6 == NU_TRUE)
/******************************************************************************
* FUNCTION
*
*      NETBOOT_Log_Ipv6_Address
*
* DESCRIPTION
*
*      Log the IPv6 addresses.
*
******************************************************************************/
STATUS NETBOOT_Log_Ipv6_Address(CHAR *dev_name, UINT8 *ipaddr, UINT8 prefix_len)
{
    STATUS status = -1;
    CHAR   buf[96];
    UINT16 room;
    UINT16 size;

    if (dev_name && ipaddr)
    {
        buf[0] = 0;
        size = sizeof(buf) - 1;
        strcat(buf, "Device ");
        room = size - strlen(buf);
        if (strlen(dev_name) < room)
        {
            strcat(buf, dev_name);
            room = size - strlen(buf);
        }

        if (room >= 15)
        {
            strcat(buf, "  IPv6 address ");
            room = size - strlen(buf);
        }

        if (room >= 40)
        {
            /* Convert the destination IP addr to ASCII */
            status = NU_Inet_NTOP(NU_FAMILY_IP6, ipaddr, &buf[strlen(buf)], 40);
            room = size - strlen(buf);
            strcat(buf, "/");
            room = size - strlen(buf);
            snprintf(&buf[strlen(buf)], room, "%d", prefix_len);
        }

        if (status == NU_SUCCESS)
        {
            NLOG_Error_Log (buf, NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    return status;
}
#endif  /* (INCLUDE_IPV6 == NU_TRUE) */



/******************************************************************************
* FUNCTION
*
*      NETBOOT_Wait_For_Network_Up
*
* DESCRIPTION
*
*      Provides an API that the application can use to wait until the network
*      stack is initialized.
*
*   INPUTS
*
*       suspend                             Suspension option
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If successful completion
*           NU_TIMEOUT                      If timeout on suspension
*           NU_NOT_PRESENT                  If event flags are not
*                                           present
*
******************************************************************************/
STATUS NETBOOT_Wait_For_Network_Up(UNSIGNED suspend)
{
    UNSIGNED            ret_events;
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Wait until the NET stack is initialized and the driver
    is up and running. */

    status = NU_Retrieve_Events(&Net_Link_Up, NU_TRUE,
                                NU_AND, &ret_events, suspend);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return(status);
}

/*************************************************************************
* FUNCTION
*
*      NU_Printf
*
* DESCRIPTION
*
*      Custom print routine, for debugging purposes if necessary.
*
******************************************************************************/
VOID NU_Printf(CHAR *string)
{
    UNUSED_PARAMETER(string);

    /* Nothing to do. */
}
