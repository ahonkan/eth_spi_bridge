/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   nu_usbf_dfu_ext.c
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver
*
* DESCRIPTION
*
*   This file contains the external Interfaces exposed by  DFU Class
*   Driver.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*   nu_os_conn_usb_func_dfu_init    This function initializes the 
*                                   device firmware upgrade component.
*   NU_USBF_DFU_Create              Creates an instance of DFU class 
*                                   driver.
*   _NU_USBF_DFU_Delete             Deletes an instance of DFU class 
*                                   driver.
*   NU_USBF_DFU_Register            This API provides and interface to 
*                                   the upper layer user modules, like 
*                                   firmware handler, to be registered 
*                                   with the DFU class diver.
*   NU_USBF_DFU_Unregister          This API provides an interface to 
*                                   deregister the registered user driver.
*   NU_USBF_DFU_Set_Status          Set the status of the DFU from the 
*                                   user driver.
*   NU_USBF_DFU_Get_State           This API provides an interface to the 
*                                   user driver to get the DFU state.
*   NU_USBF_DFU_Get_Status          This API provides an interface to the 
*                                   user driver to get the DFU status.
*   NU_USBF_DFU_DM_Open             This function is called by an 
*                                   application through DM when it want
*                                   to bind DFU interface with device.
*   NU_USBF_DFU_DM_Close            This funciton is called by an 
*                                   application through DM when it want
*                                   to unbind DFU interface with device.
*   NU_USBF_DFU_DM_Read             This function is called by an 
*                                   applicaiton through DM when it want 
*                                   to read data from device.
*   NU_USBF_DFU_DM_Write            This funciton is called by an 
*                                   application through DM when it want 
*                                   to write data to device.
*   NU_USBF_DFU_DM_IOCTL            This funciton is called by an
*                                   application through DM for performing
*                                   I/O control operations.
* DEPENDENCIES
*
*   nu_usb.h
*
**************************************************************************/

/*======================  USB Include Files ============================*/
#include "connectivity/nu_usb.h"
#include "nu_usbf_dfu_ext.h"
#include "services/runlevel_init.h"

/* ===================== DFU Runtime Device Desc ====================== */

static UINT8 dfu_rtm_intf_desc[] = {

    /* DFU Interface Descriptor */

    DFU_SPEC_RTM_INT_DES_LEN,                /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE            */
    0,                                       /* bInterfaceNumber     */
    DFU_SPEC_RTM_INT_DES_ALTR_STNG,          /* bAlternateSetting    */
    DFU_SPEC_RTM_INT_DES_NUM_EPS,            /* bNumEndpoints        */
    DFU_SPEC_RTM_INT_DES_INT_CLS,            /* bInterfaceClass      */
    DFU_SPEC_RTM_INT_DES_INT_SUBCLS,         /* bInterfaceSubClass   */
    DFU_SPEC_RTM_INT_DES_INT_PRTCL,          /* bInterfaceProtocol   */
    00,                                      /* iInterface           */
        
    /* DFU Functional Descriptor */
    DFU_SPEC_RTM_FUN_DES_LEN,                /* bLength              */
    DFU_SPEC_RTM_FUN_DES_TYPE,               /* bDescriptorType      */
    DFUF_RTM_FUN_DES_BMATTRIBUTES,           /* bmAttributes         */
    DFUF_WDETACH_TMOUT_IN_MS,                /* wDetachTimeOutLSB    */
    00,                                      /* wDetachTimeOutMSB    */
    0,                                       /* wTransferSizeLSB     */
    1,                                       /* wTransferSizeMSB     */
    DFU_RELEASE_SPEC_BCDVER,                 /* bcdDFUVersion        */
    0
};

/* =================== DFU Standalone Device Desc ===================== */

static UINT8 dfu_intf_desc[] =
{
    /* DFU Interface Descriptor */
    DFU_SPEC_INT_DES_LEN,                    /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE    */
    0,                                       /* bInterfaceNumber     */
    DFU_SPEC_RTM_INT_DES_ALTR_STNG,          /* bAlternateSetting    */
    DFU_SPEC_INT_DES_NUM_EPS,                /* bNumEndpoints        */
    DFU_SPEC_INT_DES_INT_CLS,                /* bInterfaceClass      */
    DFU_SPEC_INT_DES_INT_SUBCLS,             /* bInterfaceSubClass   */
    DFU_SPEC_INT_DES_INT_PRTCL,              /* bInterfaceProtocol   */
    0x00,                                    /* iInterface           */

    /* DFU Functional Descriptor */
    DFU_SPEC_RTM_FUN_DES_LEN,                /* bLength              */
    DFU_SPEC_RTM_FUN_DES_TYPE,               /* bDescriptorType      */
    DFUF_RTM_FUN_DES_BMATTRIBUTES,           /* bmAttributes         */
    DFUF_WDETACH_TMOUT_IN_MS,                /* wDetachTimeOutLSB    */
    00,                                      /* wDetachTimeOutMSB    */
    0,                                       /* wTransferSizeLSB     */
    1,                                       /* wTransferSizeMSB     */
    DFU_RELEASE_SPEC_BCDVER,                 /* bcdDFUVersion        */
    0
};


static char *status_str_desc[16] = {
    "No error condition is present",
    "File is not targeted for use by this device",
    "File is for this device but fails some vendor-specific verification test",
    "Device is unable to write memory",
    "Memory erased function failed",
    "Memory erase check failed",
    "Program memory function failed",
    "Programmed memory failed verification",
    "Cannot Program Memory due to received address that is out of range",
    "Received DFU_DNLOAD with wLength = 0 but device does not think it has all of the data yet",
    "Device firmware is corrupt. It cannot return to run-time operations",
    "iString indicates a vendor-specific error",
    "Device detected unexpected USB reset signaling",
    "Device detected unexpected power on reset",
    "Something wrong but device does not know what it was",
    "Device stalled an unexpected request"
};
/*===============================  Functions ===========================*/
/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_func_dfu_init
*
*   DESCRIPTION
*
*       This function initializes the Device Firmware Upgrade Component.
*
*   INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_func_dfu_init(CHAR *path, INT startstop)
{
    STATUS status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    VOID *stack_handle = NU_NULL;
    UINT8 rollback = 0;

    if (startstop == RUNLEVEL_START)
    {
        /* Allocate Memory for DFU Class Driver. */
        status = USB_Allocate_Object( sizeof(NU_USBF_DFU),
                                     (VOID **) &NU_USBF_DFU_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /* Zero Out the Allocated Block */
            memset(NU_USBF_DFU_Cb_Pt, 0x00, sizeof(NU_USBF_DFU));

            status = NU_USBF_DFU_Create( NU_USBF_DFU_Cb_Pt,
                                         "USBF-DFU");

            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        if (!rollback)
        {
            status = NU_USBF_Init_GetHandle(&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if (!rollback)
        {
            status = NU_USB_STACK_Register_Drvr ( (NU_USB_STACK *) stack_handle,
                                                 (NU_USB_DRVR *) NU_USBF_DFU_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register USB Function DFU with DM. */
        if(!rollback)
        {
            status = NU_USB_SYS_Register_Device( (VOID *) NU_USBF_DFU_Cb_Pt,
                                                 NU_USBCOMPF_DFU);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Clean Up in case if Error Occurs */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBF_DFU_Delete((VOID *) NU_USBF_DFU_Cb_Pt);
                NU_USB_ASSERT(internal_sts == NU_SUCCESS);

            case 2:
                if (NU_USBF_DFU_Cb_Pt)
                {
                    internal_sts = USB_Deallocate_Memory((VOID *) NU_USBF_DFU_Cb_Pt);
                    NU_USB_ASSERT(internal_sts == NU_SUCCESS);
                    NU_USBF_DFU_Cb_Pt = NU_NULL;
                }
            case 1:
            case 0:
                break;
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        if (NU_USBF_DFU_Cb_Pt)
        {
            status = NU_USBF_Init_GetHandle(&stack_handle);
            NU_USB_ASSERT(status == NU_SUCCESS);

            status = NU_USB_STACK_Deregister_Drvr( (NU_USB_STACK *) stack_handle, (NU_USB_DRVR *) NU_USBF_DFU_Cb_Pt);
            NU_USB_ASSERT(status == NU_SUCCESS);

            status = _NU_USBF_DFU_Delete(NU_USBF_DFU_Cb_Pt);
            NU_USB_ASSERT(status == NU_SUCCESS);

            status = USB_Deallocate_Memory(NU_USBF_DFU_Cb_Pt);
            NU_USB_ASSERT(status == NU_SUCCESS);
        }
        else
        {
            status = NU_INVALID_POINTER;
        }
    }

    /* internal_sts is not used after this. So to remove
    * KW and PC-Lint warning set it as unused variable.
    */
    NU_UNUSED_PARAM(internal_sts);

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_DFU_Get_Handle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function Still Image
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host IMG
*                           class driver.
*       NU_NOT_PRESENT      Indicates there exists no class driver.
*
*************************************************************************/
STATUS NU_USBF_DFU_Get_Handle(VOID  **handle)
{
    STATUS status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_DFU_Cb_Pt;

    if (NU_USBF_DFU_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Create
*
* DESCRIPTION
*
*   DFU Class Driver initialization routine. It allocates the resources,
*   required by the class driver to function properly. For e.g., it creates
*   the timers and initializes the global variables.
*
* INPUTS
*
*   cb              Pointer to DFU class driver control block.
*   name            Name of this USB object.
*   mem_pool        Pointer to the memory pool for class driver.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS           Successful initialization of DFU.
*   NU_USB_INVLD_ARG     Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Create (NU_USBF_DFU     *dfu_cb,
                           CHAR            *name)
{
    STATUS status, internal_sts;
    NU_USBF_DFU* cb;
    UINT8 index;
    UINT8 rollback;

    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    cb              = dfu_cb;
    rollback        = 0;
    internal_sts    = NU_SUCCESS;

    /* Check if the arguments are valid */
    if((NU_NULL == cb) || (NU_NULL == name))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        do
        {
            /* Initialize the DFU state to the application idle state. */
            cb->dfu_state = DFU_SPEC_STATE_APPL_IDLE;

            /* Allocate memory for the DFU user callback structure pointer
               * variable which holds the pointers to the callback functions of
               * the firmware handler.
               */
            status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                    sizeof(NU_USBF_DFU_USR_CALLBK),
                                    (VOID **) &(cb->dfu_usr_callback) );
            if((NU_SUCCESS != status) || (!cb->dfu_usr_callback))
            {
                rollback = 1;
                break;
            }

            memset(cb->dfu_usr_callback, 0, sizeof(NU_USBF_DFU_USR_CALLBK));

            /* Call the base driver's create function with DFU
               * class code and match class match flags.
               */
            status = _NU_USBF_DRVR_Create (&cb->parent, name,
                                   USB_MATCH_CLASS, 0, 0, 0, 0,
                                   DFU_SPEC_INT_DES_INT_CLS,
                                   DFU_SPEC_INT_DES_INT_SUBCLS,
                                   DFU_SPEC_INT_DES_INT_PRTCL,
                                   &DFUF_Usbf_Dispatch);
            if(NU_SUCCESS != status)
            {
                rollback = 2;
                break;
            }
            
            /* Create the Detach timer */
            status = NU_Create_Timer(&cb->dfu_detach_timer,"DTCHTMR",
                                        DFU_Detach_Tmr, (UNSIGNED)cb,
                                        DFUF_WDETACH_TMOUT_IN_TICKS,
                                        DFU_RESET, NU_DISABLE_TIMER);
            if(NU_SUCCESS != status)
            {
                rollback = 3;
                break;
            }

            /* Initialize the timer status as IDLE. */
            cb->detach_tmr_status = DFU_DETACH_TMR_IDLE;

            /* Create the Download busy timer. */
            status = NU_Create_Timer(&cb->dfu_dnload_busy_timer,
                                "DBSYTMR",
                                DFU_Dnload_Bsy_Tmr, (UNSIGNED)cb,
                                DFUF_POLL_TM_OUT,
                                DFU_RESET, NU_DISABLE_TIMER);
            if(NU_SUCCESS != status)
            {
                rollback = 4;
                break;
            }

            /* Create the Download busy timer. */
            status = NU_Create_Timer(&cb->dfu_mnfst_sync_timer,
                                            "MSYNTMR",
                                            DFU_Mnfst_Sync_Tmr,
                                            (UNSIGNED)cb,
                                            DFUF_POLL_TM_OUT,
                                            DFU_RESET,
                                            NU_DISABLE_TIMER);
            if(NU_SUCCESS != status)
            {
                rollback = 5;
                break;
            }   

            cb->blk_num = DFU_SET;

            /* Add DFU Function for runtime mode */
            status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                                dfu_rtm_intf_desc,
                                                sizeof(dfu_rtm_intf_desc),
                                                dfu_rtm_intf_desc,
                                                sizeof(dfu_rtm_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                                NU_NULL,
                                                sizeof(0),
#endif
                                                &dfu_cb->dfu_runtime);
            if (NU_SUCCESS != status)
            {
                rollback = 6;
                break;
            }
            
            /* Add DFU Function for standalone mode */
            status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                                dfu_intf_desc,
                                                sizeof(dfu_intf_desc),
                                                dfu_intf_desc,
                                                sizeof(dfu_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                                NU_NULL,
                                                sizeof(0),
#endif
                                                &dfu_cb->dfu_standalone);
            if (NU_SUCCESS != status)
            {
                rollback = 7;
                break;
            }

            /* Bind String Descriptors with standalone device */
            for (index = 0; index < 16; index++)
            {
                status = USBF_DEVCFG_Add_String(dfu_cb->dfu_standalone->device, status_str_desc[index]);
                if(NU_SUCCESS != status)
                    break;
            }
            
            if(NU_SUCCESS != status)
            {
                rollback = 8;
                break;
            }   
        }while(0);
    }
    
    /* Perform cleanup if there was a problem. */
    switch(rollback)
    {
        case 8:
            internal_sts = USBF_DEVCFG_Delete_Function(dfu_cb->dfu_standalone);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 7:
            internal_sts = USBF_DEVCFG_Delete_Function(dfu_cb->dfu_runtime);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 6:
            internal_sts = NU_Delete_Timer(&cb->dfu_mnfst_sync_timer);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 5:
            internal_sts = NU_Delete_Timer(&cb->dfu_dnload_busy_timer);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 4:
            internal_sts = NU_Delete_Timer(&cb->dfu_detach_timer);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 3:
            internal_sts = _NU_USBF_DRVR_Delete (&cb->parent);
            NU_USB_ASSERT(NU_SUCCESS == internal_sts);
        case 2:
            if(cb->dfu_usr_callback)
            {
                USB_Deallocate_Memory(cb->dfu_usr_callback);
                cb->dfu_usr_callback = NU_NULL;
            }
        default:
            break;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();
    
    /* internal_sts is not used after this. So to remove
     * KW and PC-Lint warning set it as unused variable.
     */
    NU_UNUSED_PARAM(internal_sts);

    return (status);
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Delete
*
* DESCRIPTION
*
*   This function deletes an instance of DFU class driver and de-allocates
*   all the resources allocated in the initialization routine.
*
* INPUTS
*
*   cb         Pointer to DFU class driver control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS          Successful deletion of DFU.
*   NU_USB_INVLD_ARG    Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS _NU_USBF_DFU_Delete (VOID *cb)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_DFU *dfu;

    /* Check if the arguments are valid */
    if(NU_NULL == cb)
    {
        status = NU_USB_INVLD_ARG;
    }

    if(NU_SUCCESS == status)
    {
        dfu = (NU_USBF_DFU *)cb;

        /* Deallocate memory for the DFU user call back pointer. */
        status = USB_Deallocate_Memory((VOID *)dfu->dfu_usr_callback);
        if(NU_SUCCESS == status)
        {
            /* Call the base driver's delete function. */
            status =  _NU_USBF_DRVR_Delete(cb);

            /* Delete the detach timer created. */
            NU_Delete_Timer(&dfu->dfu_detach_timer);

            /* Delete the download sync timer created. */
            NU_Delete_Timer(&dfu->dfu_dnload_busy_timer);

            /* Delete the manifestation sync timer created. */
            NU_Delete_Timer(&dfu->dfu_mnfst_sync_timer);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Register
*
* DESCRIPTION
*
*   This API provides an interface to the user driver to be registered with
*   the DFU class driver.
*
* INPUTS
*
*   cb                  Pointer to DFU class driver control block.
*   dfu_usr_callbk      This is a user call back structure pointer variable
*                       holding the call backs to the user driver.
*   dfu_fwh_data        Pointer to the firmware handler control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS         Registration with the DFU class driver is successful
*   NU_USB_INVLD_ARG   Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Register(NU_USBF_DFU *cb,
                            NU_USBF_DFU_USR_CALLBK *dfu_usr_callbk,
                            VOID *dfu_fwh_data)
{
    STATUS status;

    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check if the arguments are valid */
    if(NU_NULL == cb || NU_NULL == dfu_usr_callbk)
    {
        return NU_USB_INVLD_ARG;
    }

    cb->dfu_usr_callback->DFU_Data_Xfer_Callbk =
                                dfu_usr_callbk->DFU_Data_Xfer_Callbk;
    cb->dfu_usr_callback->DFU_Event_Callbk =
                                dfu_usr_callbk->DFU_Event_Callbk;
    cb->dfu_usr_callback->DFU_Status_Update_Callbk =
                                dfu_usr_callbk->DFU_Status_Update_Callbk;
    cb->dfu_usr_callback->DFU_Request_Update_Callbk =
                                dfu_usr_callbk->DFU_Request_Update_Callbk;
    cb->dfu_usr_callback->DFU_Get_Poll_Tm_Out =
                                dfu_usr_callbk->DFU_Get_Poll_Tm_Out;

    cb->user_context = dfu_fwh_data;

    /* Allocate Memory for DFU Class Driver. */
    status = USB_Allocate_Object( DFUF_WTRANSFER_SIZE,
                                 (VOID **) &(cb->data_buffer));


    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Unregister
*
* DESCRIPTION
*
*   This API deregisters the user driver with the DFU class driver by
*   un-initializing the callback pointers to the firmware handler.
*
* INPUTS
*
*   cb                  Pointer to DFU class driver control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS          Un-registering of the user driver with the DFU class
*                       driver is successful.
*   NU_USB_INVLD_ARG    Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Unregister(NU_USBF_DFU *cb)
{

    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check if the DFU handle is valid */
    if(NU_NULL == cb)
    {
        return NU_USB_INVLD_ARG;
    }

    memset(cb->dfu_usr_callback, DFU_RESET, sizeof(NU_USBF_DFU_USR_CALLBK));

    /* Revert back to user mode. */
    NU_USER_MODE();

    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Set_Status
*
* DESCRIPTION
*
*   This API provides an interface to the user driver to keep the status of
*   the DFU updated while its processing firmware upload or firmware
*   download.
*
* INPUTS
*
*   cb              Pointer to DFU class driver control block.
*   dfu_status      This API maintains the status of the firmware
*                   download/upload.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS          DFU set status is successful
*   NU_USB_INVLD_ARG    Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Set_Status(NU_USBF_DFU *cb,
                                                UINT8 dfu_status)
{
    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check if the DFU handle is valid */
    if(NU_NULL == cb)
    {
        return NU_USB_INVLD_ARG;
    }

    cb->dfu_status_str_index = dfu_status;
    cb->dfu_status = dfu_status;

    /* Revert back to user mode. */
    NU_USER_MODE();

    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Get_State
*
* DESCRIPTION
*
*   This API provides an interface to the user driver to get the DFU state.
*
* INPUTS
*
*   cb              Pointer to DFU class driver control block.
*
* OUTPUTS
*
*   dfu_state       DFU class driver returns the state of the DFU to the
*                   calling function.
*
* RETURNS
*
*   NU_SUCCESS          On successful, DFU state is returned to the upper
*                       layer.
*   NU_USB_INVLD_ARG    Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Get_State(NU_USBF_DFU *cb, UINT8 *dfu_state)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check if the input arguments are valid */
    if((NU_NULL != cb) && (NU_NULL != dfu_state))
    {
        *dfu_state = cb->dfu_state;
        status = NU_SUCCESS;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*   NU_USBF_DFU_Get_Status
*
* DESCRIPTION
*
*   This API provides an interface to the user driver to get the DFU
*   status.
*
* INPUTS
*
*   cb              Pointer to DFU class driver control block.
*
* OUTPUTS
*
*   dfu_status      DFU class driver returns the status of the DFU to the
*                   calling function.
*
* RETURNS
*
*   NU_SUCCESS          On successful, DFU status is returned to the upper
*                       layer.
*   NU_USB_INVLD_ARG    Indicates that one or more parameters is invalid
*
**************************************************************************/
STATUS NU_USBF_DFU_Get_Status(NU_USBF_DFU *cb,
                              UINT8       *dfu_status)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Declaring variables and switching to SV mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check if the input arguments are valid */
    if((NU_NULL != cb) && (NU_NULL != dfu_status))
    {
        *dfu_status = cb->dfu_status;
        status = NU_SUCCESS;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_Bind_Interface
*
* DESCRIPTION
*
*       This function binds DFU interface with USB device configuration
*
* INPUTS
*
*       dfu                     Dfu usb function pointer.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that keyboard function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_DFU_Bind_Interface(USBF_FUNCTION *dfu_func)
{
    /* Local Variables */
    STATUS status;

    status = NU_USB_INVLD_ARG;
    if (dfu_func != NU_NULL)
    {
        /* Enable DFU Function */
        status = USBF_DEVCFG_Enable_Function(dfu_func);
        if (status == NU_SUCCESS)
        {
            /* Bind DFU Function */
            status = USBF_DEVCFG_Bind_Function(dfu_func);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_Unbind_Interface
*
* DESCRIPTION
*
*       This function Unbinds DFU interface with USB device configuration
*
* INPUTS
*
*       dfu                     Dfu usb function pointer.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that keyboard function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_DFU_Unbind_Interface(USBF_FUNCTION *dfu_func)
{
    STATUS status;

    status = NU_USB_INVLD_ARG;
    if (dfu_func != NU_NULL)
    {
        /* Disable DFU Function */
        status = USBF_DEVCFG_Disable_Function(dfu_func);
        if (status == NU_SUCCESS)
        {
            /* Unbind DFU Function */
            status = USBF_DEVCFG_Unbind_Function(dfu_func);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the DFU cb passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_DFU_DM_Open (VOID* instance_handle, DV_DEV_LABEL label_list[],
                          INT label_cnt, VOID** session_handle)
{
    STATUS status = NU_USB_INVLD_ARG;
    DV_DEV_LABEL DFU_RTM_LABEL = {USBF_DFU_RTM_LABEL};
    DV_DEV_LABEL DFU_STDA_LABEL = {USBF_DFU_STDA_LABEL};
    NU_USBF_DFU *dfu_cb;

    dfu_cb = (NU_USBF_DFU *) instance_handle;

    if (label_cnt == 1)
    {
        if (DV_COMPARE_LABELS( &label_list[0], &DFU_RTM_LABEL))
        {
            status = NU_USBF_DFU_Bind_Interface( dfu_cb -> dfu_runtime);
        }
        else if (DV_COMPARE_LABELS( &label_list[0], &DFU_STDA_LABEL))
        {
            status = NU_USBF_DFU_Bind_Interface( dfu_cb -> dfu_standalone);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close
*       a device which it has already been opened for read/write/ioctl 
*       operations.
*
* INPUTS
*
*       dev_handle         Pointer to DFU cb passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_DFU_DM_Close(VOID* instance_handle)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBF_DFU *dfu_cb;

    dfu_cb = (NU_USBF_DFU *) instance_handle;

    if (dfu_cb -> dfu_runtime -> is_enabled == NU_TRUE)
    {
        status = NU_USBF_DFU_Unbind_Interface( dfu_cb -> dfu_runtime);
    }

    if (dfu_cb -> dfu_standalone -> is_enabled == NU_TRUE)
    {
        status = NU_USBF_DFU_Unbind_Interface( dfu_cb -> dfu_standalone);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the DFU cb passed as context.
*       buffer             Pointer to memory location where to put the 
*                          read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain 
*                          offset in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_DFU_DM_Read(
                           VOID*    dev_handle,
                           VOID*    buffer,
                           UINT32   numbyte,
                           OFFSET_T byte_offset,
                           UINT32   *bytes_read_ptr)
{
    return(NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the DFU driver passed as context.
*       buffer             Pointer to memory location where data to be 
*                          written is available.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset 
*                          in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_DFU_DM_Write(
                            VOID*        dev_handle,
                            const VOID*  buffer,
                            UINT32       numbyte,
                            OFFSET_T     byte_offset,
                            UINT32       *bytes_written_ptr)
{
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_DFU_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to 
*       perform a control operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the DFU cb passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_DFU_DM_IOCTL (
                             VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len)
{
    STATUS status;
    NU_USBF_DFU *dfu_cb = (NU_USBF_DFU *) dev_handle;
    USBF_DFU_APP_CALLBACK *dfu_struct_ptr;
    DV_IOCTL0_STRUCT        *ioctl0;
    UINT8 dfu_value;
    UINT8 *dfu_ptr;

    switch (ioctl_num)
    {
        case DV_IOCTL0:
        {
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0          = (DV_IOCTL0_STRUCT *) ioctl_data;
                ioctl0->base    = USB_DFU_IOCTL_BASE;
                status          = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }

        case( USB_DFU_IOCTL_BASE + USBF_DFU_IOCTL_SET_STATUS):
        {
            dfu_value = *( (UINT8 *) ioctl_data );
            status = NU_USBF_DFU_Set_Status( dfu_cb, dfu_value );
            break;
        }

        case( USB_DFU_IOCTL_BASE + USBF_DFU_IOCTL_GET_STATE):
        {
            dfu_ptr = (UINT8 *) ioctl_data;
            status = NU_USBF_DFU_Get_State( dfu_cb, dfu_ptr );
            break;
        }

        case( USB_DFU_IOCTL_BASE + USBF_DFU_IOCTL_GET_STATUS):
        {
            dfu_ptr = (UINT8 *) ioctl_data;
            status = NU_USBF_DFU_Get_Status( dfu_cb, dfu_ptr );
            break;
        }

        case( USB_DFU_IOCTL_BASE + USBF_DFU_IOCTL_REG_CALLBACK):
        {
           dfu_struct_ptr = (USBF_DFU_APP_CALLBACK *) ioctl_data;
           status = NU_USBF_DFU_Register
                                ( dfu_cb,
                                &(dfu_struct_ptr->dfu_user_callbk),
                                dfu_struct_ptr->dfu_data_context);
            break;
        }

        case( USB_DFU_IOCTL_BASE + USBF_DFU_IOCTL_UNREG_CALLBACK):
        {
            status = NU_USBF_DFU_Unregister( dfu_cb );
            break;
        }

        default:
        {
            status = NU_USB_NOT_SUPPORTED;
            break;
        }
    }

    return (status);
}
/*============================  End Of File  ===========================*/
