/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       keypad_ge.c
*
*   COMPONENT
*
*       Keypad Driver
*
*   DESCRIPTION
*
*       This file contains the keypad driver generic functions.
*
*   DATA STRUCTURES
*
*       KP_Driver_HISR
*       KP_Driver_Timer
*
*   FUNCTIONS
*
*       KP_Device_Mgr
*       KP_Open
*       KP_Close
*       KP_Driver_HISR_Entry
*       KP_Driver_Task_Entry
*       KP_Driver_Timer_Func
*       KP_Register_Callbacks
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "ui/nu_ui.h"
#include    "services/nu_services.h"
#include    "drivers/nu_drivers.h"


/* System memory pool. */
extern      NU_MEMORY_POOL  System_Memory;
extern      INT32   EVENTH_StoreEvent(rsEvent *argEV);

/* Keypad driver HISR control block. */
NU_HISR     KP_Driver_HISR;

#if         (KP_TASK_REQUIRED == NU_TRUE)

/* Keypad driver task. */
static      NU_TASK             KP_Driver_Task;

/* Keypad driver event group. */
static      NU_EVENT_GROUP      KP_Driver_Events;

#else

/* Keypad driver timer. */
static      NU_TIMER            KP_Driver_Timer;

#endif      /* (KP_TASK_REQUIRED == NU_TRUE) */

/* Functions with local scope. */

/* Keypad driver HISR entry function. */
static      VOID    KP_Driver_HISR_Entry(VOID);

#if         (KP_TASK_REQUIRED == NU_TRUE)

/* Keypad driver task entry function. */
static      VOID    KP_Driver_Task_Entry(UNSIGNED argc, VOID *argv);

#else

/* Keypad driver timer function. */
static      VOID    KP_Driver_Timer_Func(UINT32 id);

#endif      /* (KP_TASK_REQUIRED == NU_TRUE) */

static      STATUS  KP_Open(mouseRcd *rcd);
static      STATUS  KP_Close(mouseRcd *rcd);

#if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

static      BOOLEAN     KP_Released = NU_TRUE;

#endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

/* Keypad controller session handle. */
static      DV_DEV_HANDLE   keypad_ctrl_sess_hd = NU_NULL;

/* Event structure to store keypad event. */
static      rsEvent         keypad_events;

/* Keypad callback structure. */
static      KEYPAD_CALLBACKS  Keypad_Callback;


/*************************************************************************
*
*   FUNCTION
*
*       KP_Device_Mgr
*
*   DESCRIPTION
*
*       This function provides device control interface to Grafix RS input
*       management for keypad driver.
*
*   INPUTS
*
*       rcd                                 Mouse record, this parameter
*                                           is not used
*
*       md                                  Command: IMOPEN, IMCLOSE
*
*   OUTPUTS
*
*       status                              Return status of the open or
*                                           close function
*       NU_INVALID_COMMAND                  Invalid command
*
*************************************************************************/
INT32 KP_Device_Mgr(mouseRcd *rcd, INT16 md)
{
    STATUS   status;

    /* Check if open command is specified. */
    if (md == IMOPEN)
    {
        /* Call the keypad driver open service. */
        status = KP_Open(rcd);
    }

    /* Check if close command is specified. */
    else if (md == IMCLOSE)
    {
        /* Call the keypad driver close service. */
        status = KP_Close(rcd);
    }

    /* Otherwise an invalid command is specified. */
    else
    {
        /* Set status to indicate that invalid command was specified. */
        status = NU_INVALID_COMMAND;
    }

    /* Return the completion status of the service. */
    return ((INT32)status);
}

/*************************************************************************
*
*   FUNCTION
*
*       KP_Open
*
*   DESCRIPTION
*
*       This function initializes the keypad and allocates required
*       resources.
*
*   INPUTS
*
*       rcd                                 Mouse record, this parameter
*                                           is not needed
*
*   OUTPUTS
*
*       NU_SUCCESS                          Device opened successfully
*       [error code]                        Error code, otherwise
*
*************************************************************************/
static STATUS KP_Open(mouseRcd *rcd)
{
    STATUS         status;
    VOID          *pointer;
    DV_DEV_LABEL   labels[] = {{KEYPAD_LABEL}};
    INT            label_cnt = 1;
    DV_DEV_ID      keypad_dev_id_list[CFG_NU_OS_DRVR_KEYPAD_MAX_DEVS_SUPPORTED];
    INT            dev_id_cnt = CFG_NU_OS_DRVR_KEYPAD_MAX_DEVS_SUPPORTED;

    /* Suppress harmless compiler warning. */
    NU_UNUSED_PARAM(rcd);

    /* Allocate memory for keypad driver HISR. */
    status = NU_Allocate_Memory(&System_Memory, &pointer,
                                KP_HISR_STACK_SIZE, NU_NO_SUSPEND);

    /* Check if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Create the HISR. */
        status = NU_Create_HISR(&KP_Driver_HISR, "KP_HISR",
                                KP_Driver_HISR_Entry, KP_HISR_PRIORITY,
                                pointer, KP_HISR_STACK_SIZE);
    }

#if         (KP_TASK_REQUIRED == NU_TRUE)

    /* Check to see if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Create timer for release event. */
        status = NU_Create_Event_Group(&KP_Driver_Events, "KP_EVNT");

    }

    /* Check to see if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Allocate memory for keypad driver task. */
        status = NU_Allocate_Memory(&System_Memory, &pointer,
                                     KP_TASK_STACK_SIZE, NU_NO_SUSPEND);
    }

    /* Check to see if previous operation was successful */
    if (status == NU_SUCCESS)
    {
        /* Create the keypad driver task. */
        status = NU_Create_Task(&KP_Driver_Task, "KP_TASK",
                                 KP_Driver_Task_Entry,
                                 0, NU_NULL, pointer, KP_TASK_STACK_SIZE,
                                 KP_TASK_PRIORITY, 0, NU_PREEMPT, NU_START);
    }

#else

    /* Check to see if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Create keypad driver timer. */
        status = NU_Create_Timer(&KP_Driver_Timer, "KP_TMR",
                                 KP_Driver_Timer_Func,
                                 1, KP_SAMPLING_INTERVAL, 0,
                                 NU_DISABLE_TIMER);
    }

#endif      /* (KP_TASK_REQUIRED == NU_TRUE) */

    /* Check to see if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Call target specific initialize routine to initialize the keypad
           hardware. */

        /* Get all devices with these labels; <KEYPAD, name>  */
        status = DVC_Dev_ID_Get (labels, label_cnt, keypad_dev_id_list,
                                    &dev_id_cnt);

        if ((status == NU_SUCCESS) && (dev_id_cnt > 0))
        {
            /* Hardware initialization. Open keypad controller.
             * As there is support for only one keypad controller
             * so pass first id only. */
            status = DVC_Dev_ID_Open(keypad_dev_id_list[0], labels, 1,
                                    &keypad_ctrl_sess_hd);
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       KP_Close
*
*   DESCRIPTION
*
*       This function closes the keypad and releases the allocated OS
*       resources.
*
*   INPUTS
*
*       rcd                                 Mouse record, this parameter
*                                           is not used
*
*   OUTPUTS
*
*       NU_SUCCESS                          Device closed successfully
*       [error code]                        Error code, otherwise
*
*************************************************************************/
static STATUS KP_Close(mouseRcd *rcd)
{
    STATUS  status;

    /* Suppress harmless compiler warning. */
    NU_UNUSED_PARAM(rcd);

    /* Call the target sepcific shutdown service for any target sepcific
       operation required. */
    if (keypad_ctrl_sess_hd)
    {
        (VOID)DVC_Dev_Close(keypad_ctrl_sess_hd);
    }

    /* Deallocate the memory allocated to driver HISR stack. */
    status = NU_Deallocate_Memory(KP_Driver_HISR.tc_stack_start);

    /* Check if HISR stack memory was successfully deallocated. */
    if (status == NU_SUCCESS)
    {
        /* Delete the keypad driver HISR. */
        status = NU_Delete_HISR(&KP_Driver_HISR);
    }

#if         (KP_TASK_REQUIRED == NU_FALSE)

    /* Check if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Delete the keypad driver timer. */
        status = NU_Delete_Timer(&KP_Driver_Timer);
    }

#else

    /* Check if previous operation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Deallocate the memory allocated to driver task stack. */
        status = NU_Deallocate_Memory(KP_Driver_Task.tc_stack_start);
    }

    /* Check if task stack memory was successfully deallocated. */
    if (status == NU_SUCCESS)
    {
        /* Delete the keypad driver task. */
        status = NU_Delete_Task(&KP_Driver_Task);
    }

    /* Check if task was successfully deleted. */
    if (status == NU_SUCCESS)
    {
        /* Delete the keypad event group. */
        status = NU_Delete_Event_Group(&KP_Driver_Events);
    }

#endif      /* (KP_TASK_REQUIRED == NU_FALSE) */

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       KP_Driver_HISR_Entry
*
*   DESCRIPTION
*
*       Keypad driver HISR entry function. This function processes the
*       keypad interrupt and determines the key which has been pressed or
*       released and generates a corresponding event.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID KP_Driver_HISR_Entry(VOID)
{
#if     (KP_TASK_REQUIRED == NU_TRUE)

    #if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

    /* Check if currently no key is pressed. */
    if (KP_Released)
    {

    #endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

        /* Send the key down event to keypad driver task. */
        (VOID)NU_Set_Events(&KP_Driver_Events, KP_KEY_DOWN_EVENT, NU_OR);

    #if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

        /* Mark the flag to signify that a key is in pressed state. */
        KP_Released = NU_FALSE;
    }

    /* A key is already in pressed state so this should be a key release
       interrupt */
    else
    {
        /* Send the key up event to keypad driver task. */
        (VOID)NU_Set_Events(&KP_Driver_Events, KP_KEY_UP_EVENT, NU_OR);

        /* Mark the flag to signify that no key is in pressed state. */
        KP_Released = NU_TRUE;
    }

    #endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

#else

    UINT16   key = KP_NO_KEY;

    #if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

    /* Check if currently no key is pressed. */
    if (KP_Released)
    {

    #endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

        /* Scan the keypad. */
        if (Keypad_Callback.kp_scan)
        {
            key = Keypad_Callback.kp_scan();
        }        

        /* Check if a valid key press is detected. */
        if (key != KP_NO_KEY)
        {
            /* Set the event scan and type within the event structure. */

            keypad_events.eventScan = 0;
            keypad_events.eventType = evntKEYDN;
            keypad_events.eventChar = key;

            /* Store the event. */
            EVENTH_StoreEvent(&keypad_events);

            /* Start the timer for debouncing delay. */
            NU_Reset_Timer(&KP_Driver_Timer, KP_Driver_Timer_Func,
                           KP_INITIAL_INTERVAL, 0, NU_ENABLE_TIMER);

    #if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

            /* Mark the flag to signify that a key is in pressed state. */
            KP_Released = NU_FALSE;

    #endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

        }

        /* Otherwise, ignore the interrupt. */
        else
        {
            /* Enable key down (press) interrupt(s). */
            if (Keypad_Callback.kp_enable_key_down_interrupts)
            {
                Keypad_Callback.kp_enable_key_down_interrupts();
            }
        }

    #if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

    }
    else
    {
        /* Mark the flag to signify that no key is in pressed state. */
        KP_Released = NU_TRUE;

        /* Disable key up (release) interrupt(s). */
        if (Keypad_Callback.kp_disable_key_up_interrupts)
        {
            Keypad_Callback.kp_disable_key_up_interrupts();
        }

        /* Set the event scan and type within the event structure. */

        keypad_events.eventScan = 0;
        keypad_events.eventType = evntKEYUP;

        /* Store the event. */
        EVENTH_StoreEvent(&keypad_events);

        /* Enable key down (press) interrupt(s). */
        if (Keypad_Callback.kp_enable_key_down_interrupts)
        {
            Keypad_Callback.kp_enable_key_down_interrupts();
        }
    }

    #endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

#endif      /* (KP_TASK_REQUIRED == NU_TRUE) */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
    /* Reset the watchdog */
    (VOID)PMI_Reset_Watchdog((Keypad_Callback.kp_pmi_dev));
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */
}

#if         (KP_TASK_REQUIRED == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       KP_Driver_Task_Entry
*
*   DESCRIPTION
*
*       Entry function of keypad driver task. This task is responsible
*       for processing the keypad events. It waits on the key down
*       event to be received from the HISR. After the event is received,
*       it polls the keypad to detect key release. After that it again
*       starts waiting for the key down event.
*
*   INPUTS
*
*       argc                                Number of arguments to the
*                                           task
*
*       argv                                Array of arguments
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID KP_Driver_Task_Entry(UNSIGNED argc, VOID *argv)
{
    UNSIGNED    ret_events;
    STATUS      status;

    /* Suppress harmless compiler warnings. */

    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    /* Keep on processing the keypad events. */
    for (;;)
    {
        /* Wait for the key down event from the HISR. */
        status = NU_Retrieve_Events(&KP_Driver_Events, KP_KEY_DOWN_EVENT,
                                    NU_OR_CONSUME, &ret_events, NU_SUSPEND);

        /* Check if event is received successfully. */
        if (status == NU_SUCCESS)
        {
            UINT16   key = KP_NO_KEY;

            /* Scan the keypad. */
            if (Keypad_Callback.kp_scan)
            {
                key = Keypad_Callback.kp_scan();
            }

            /* Check if a valid key press is detected. */
            if (key != KP_NO_KEY)
            {
                /* Set the event scan and type within the event structure. */

                keypad_events.eventScan = 0;
                keypad_events.eventType = evntKEYDN;
                keypad_events.eventChar = key;

                /* Store the event. */
                EVENTH_StoreEvent(&keypad_events);

                /* Wait for a while for debouncing. */
                NU_Sleep(KP_INITIAL_INTERVAL);

#if         (KP_USE_RELEASE_INTERRUPT == NU_FALSE)

                /* Polling loop until key is released. */
                for (;;)
                {
                    /* Scan the keypad. */
                    if (Keypad_Callback.kp_scan)
                    {
                        key = Keypad_Callback.kp_scan();
                    }

                    /* Check if the key has not been released yet. */
                    if (key != KP_NO_KEY)
                    {
                        /* Wait before polling the keypad again. */
                        NU_Sleep(KP_SAMPLING_INTERVAL);
                    }

                    /* Key is released. */
                    else
                    {
                        /* Set the event scan and type within the event structure. */

                        keypad_events.eventScan = 0;
                        keypad_events.eventType = evntKEYUP;

                        /* Store the event. */
                        EVENTH_StoreEvent(&keypad_events);

                        /* Enable key down (press) interrupt(s). */
                        if (Keypad_Callback.kp_enable_key_down_interrupts)
                        {
                            Keypad_Callback.kp_enable_key_down_interrupts();
                        }

                        /* Get out of the polling loop. */
                        break;
                    }
                }

#else

                /* Enable key up (release) interrupt(s). */
                if (Keypad_Callback.kp_enable_key_up_interrupts)
                {
                    Keypad_Callback.kp_enable_key_up_interrupts();
                }

                /* Wait for the key up event from the HISR. */
                status = NU_Retrieve_Events(&KP_Driver_Events, KP_KEY_UP_EVENT,
                                            NU_OR_CONSUME, &ret_events, NU_SUSPEND);

                /* Check if event is received successfully. */
                if (status == NU_SUCCESS)
                {
                    /* Set the event scan and type within the event structure. */

                    keypad_events.eventScan = 0;
                    keypad_events.eventType = evntKEYUP;

                    /* Store the event. */
                    EVENTH_StoreEvent(&keypad_events);

                    /* Enable key down (press) interrupt(s). */
                    if (Keypad_Callback.kp_enable_key_down_interrupts)
                    {
                        Keypad_Callback.kp_enable_key_down_interrupts();
                    }
                }

#endif      /* (KP_USE_RELEASE_INTERRUPT == NU_FALSE) */

            }

            /* Otherwise, ignore the interrupt. */
            else
            {

#if         (KP_USE_RELEASE_INTERRUPT == NU_TRUE)

                KP_Released = NU_TRUE;

#endif      /* (KP_USE_RELEASE_INTERRUPT == NU_TRUE) */

                /* Enable key down interrupt(s). */
                if (Keypad_Callback.kp_enable_key_down_interrupts)
                {
                    Keypad_Callback.kp_enable_key_down_interrupts();
                }
            }
        }
    }
}

#else

/*************************************************************************
*
*   FUNCTION
*
*       KP_Driver_Timer_Func
*
*   DESCRIPTION
*
*       This is the expiration routine of the keypad driver timer. This
*       timer gets active after a keypress. This function checks if key
*       has been released. If key is released, a key release event is
*       generated, otherwise timer is started again to check the key
*       release after some delay.
*
*   INPUTS
*
*       id                                  The timer ID
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID KP_Driver_Timer_Func(UINT32 id)
{

#if         (KP_USE_RELEASE_INTERRUPT == NU_FALSE)

    UINT16   key = KP_NO_KEY;

    /* Scan the keypad. */
    if (Keypad_Callback.kp_scan)
    {
        key = Keypad_Callback.kp_scan();
    }

    /* Check if the key has not been released yet. */
    if (key != KP_NO_KEY)
    {
        /* Restart the timer to scan the keypad again after
           some delay. */
        NU_Reset_Timer(&KP_Driver_Timer, KP_Driver_Timer_Func,
                       KP_SAMPLING_INTERVAL, 0, NU_ENABLE_TIMER);
    }

    /* Key is released. */
    else
    {
        /* Set the event scan and type within the event structure. */

        keypad_events.eventScan = 0;
        keypad_events.eventType = evntKEYUP;

        /* Store the event. */
        EVENTH_StoreEvent(&keypad_events);

        /* Enable key down (press) interrupt(s). */
        if (Keypad_Callback.kp_enable_key_down_interrupts)
        {
            Keypad_Callback.kp_enable_key_down_interrupts();
        }
    }

#else

    /* Enable key up (release) interrupt(s). */
    if (Keypad_Callback.kp_enable_key_up_interrupts)
    {
        Keypad_Callback.kp_enable_key_up_interrupts();
    }

#endif      /* (KP_USE_RELEASE_INTERRUPT == NU_FALSE) */

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(id);
}

#endif      /* (KP_TASK_REQUIRED == NU_TRUE) */

/*************************************************************************
*
*   FUNCTION
*
*       KP_Register_Callbacks
*
*   DESCRIPTION
*
*       This function registers callback functions which are implemented 
*       by the user.
*
*   INPUTS
*
*       kp_cb                               Pointer to Keypad callback
*                                           structure
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID KP_Register_Callbacks(KEYPAD_CALLBACKS *kp_cb)
{
    /* Copy callback function pointers to global variable. */
    Keypad_Callback.kp_enable_key_down_interrupts = kp_cb->kp_enable_key_down_interrupts;
    Keypad_Callback.kp_disable_key_down_interrupts = kp_cb->kp_disable_key_down_interrupts;
    Keypad_Callback.kp_enable_key_up_interrupts = kp_cb->kp_enable_key_up_interrupts;
    Keypad_Callback.kp_disable_key_up_interrupts = kp_cb->kp_disable_key_up_interrupts;
    Keypad_Callback.kp_scan = kp_cb->kp_scan;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
    Keypad_Callback.kp_pmi_dev = (kp_cb->kp_pmi_dev);
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

}
