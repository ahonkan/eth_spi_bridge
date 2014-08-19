/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_ms_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains implementation for different API services
*       provided by Mass Storage Class Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_MS_Create                   Initializes the Mass Storage
*                                           class driver.
*       NU_USBF_MS_Create_Task_Mode         Initializes the Mass Storage
*                                           class driver for task mode.
*       NU_USBF_MS_Bind_Interface           Binds mass storage interface
*                                           descriptor with device
*                                           configuration descriptor.
*       NU_USBF_MS_Unbind_Interface         Unbinds mass storage interface
*                                           descriptor from device
*                                           configuration descriptor.
*       _NU_USBF_MS_Class_Specific          Handles MS class specific
*                                           requests.
*       _NU_USBF_MS_Connect                 Processes a new MS device.
*       _NU_USBF_MS_Delete                  Un-initializes the MS class
*                                           driver.
*       _NU_USBF_MS_Disconnect              Processes the disconnection of
*                                           a Mass Storage device.
*       _NU_USBF_MS_New_Transfer            New transfer request
*                                           processing.
*       _NU_USBF_MS_Notify                  USB event processing.
*       _NU_USBF_MS_Set_Interface           Alternate setting change
*                                           processing.
*       NU_USBF_MS_Init                     This function initializes the
*                                           function mass storage
*                                           component.
*       NU_USBF_MS_Init_GetHandle           This function is used to
*                                           retrieve the function mass
*                                           storage class driver's
*                                           address.
*       NU_USBF_MS_Start_Cmd_Processing     Start receiving CBW from Host.
*       NU_USBF_MS_Set_Command_Direction    Retrieve the function mass
*                                           storage class driver control
*                                           block handle.
*
* DEPENDENCIES
*
*        nu_usb.h                           All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include    "connectivity/nu_usb.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"

/* =============================  Globals ============================== */
/* Following Variable is used to ensure that Mass Storage is bound
 * only once and it should be accessible in this file only */
static BOOLEAN initialized = NU_FALSE;

char USBF_MS_Configuration_String[NU_USB_MAX_STRING_LEN];
char USBF_MS_Interface_String[NU_USB_MAX_STRING_LEN];

UINT8 fs_hs_ms_intf_desc[] =
{
    /* Interface Descriptor         */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x02,                                   /* bNumEndpoints        */
    USB_SPEC_MSF_CLASS,                     /* bInterfaceClass      */
    USB_SPEC_MSF_SUBCLASS_SCSI,             /* bInterfaceSubClass   */
    USB_SPEC_MSF_PROTOCOL_BOT,              /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Endpoint Descriptors         */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_OUT,                            /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Descriptors         */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00                                    /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
UINT8 ss_ms_intf_desc[] =
{
    /* Interface Descriptor         */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x02,                                   /* bNumEndpoints        */
    USB_SPEC_MSF_CLASS,                     /* bInterfaceClass      */
    USB_SPEC_MSF_SUBCLASS_SCSI,             /* bInterfaceSubClass   */
    USB_SPEC_MSF_PROTOCOL_BOT,              /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Endpoint Descriptors         */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_OUT,                            /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0,

    /* Endpoint Descriptors         */
    7,                                      /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0
};
#endif

#ifdef  NU_USBF_MS_TASK_MODE
/**************************************************************************
* FUNCTION
*
*        NU_USBF_MS_Create_Task_Mode
*
* DESCRIPTION
*
*        This function is used only when we need to process commands in
*        task mode. This function initializes the mass storage class driver
*        All the internal data structures are initialized. Subsequently,
*        users can be registered with this class driver. Once, initialized
*        successfully, this class driver must be registered with a stack.
*        Together with a registered user, this enables the mass storage
*        functionality of the USB device. Mass storage class driver starts
*        responding to the Host requests to communicate with the mass
*        storage device.
*
* INPUTS
*
*        pcb_ms_drvr         Pointer to Nucleus USB Mass Storage class
*                            driver.
*        p_name              Name of the class driver.
*        p_mem_pool       Pointer to memory pool.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful initialization.
*        NU_USB_INVLD_ARG    Indicates incorrect parameters.
*        NU_USB_NOT_PRESENT  Indicates a configuration problem because of
*                            which no more USB objects could be created.
*
**************************************************************************/
STATUS NU_USBF_MS_Create_Task_Mode (NU_USBF_MS        *pcb_ms_drvr,
                                    CHAR              *p_name,
                                    NU_MEMORY_POOL    *p_mem_pool)
{

    /* Local variables. */
    STATUS      status;
    UINT8       idx;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(p_mem_pool);

    /* Validate the arguments. */
    status = NU_SUCCESS;
    if((!pcb_ms_drvr) || (!p_name) || (!pcb_ms_drvr)
                      || ((NU_USB *) pcb_ms_drvr)->subsys != 0)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        memset (pcb_ms_drvr, 0, sizeof(NU_USBF_MS));
        
        /* Allocate memory for CBW and CSW in each device for later use. */
        for(idx = 0; idx < NU_USBF_MS_MAX_INTERFACES; idx++ )
        {
            /* Allocate memory for CBW in each device control block
             * Exit from loop if there was an error.
             */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(MSF_BOT_CBW),
                                        (VOID**)&(pcb_ms_drvr->devices[idx].cbw));
            if ( status != NU_SUCCESS )
            {
                break;
            }
            
            /* Allocate memory for CSW in each device control block
             * Exit from loop if there was an error. 
             */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(MSF_BOT_CSW),
                                        (VOID**)&(pcb_ms_drvr->devices[idx].csw));
            if ( status != NU_SUCCESS )
            {
                break;
            }
            
            /* Allocate memory to hold value of Maximum LUN. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(UINT8),
                                        (void**)&(pcb_ms_drvr->devices[idx].max_lun))
            if ( status != NU_SUCCESS )
            {
                break;
            }
        }

        if ( status == NU_SUCCESS )
        {
            /* Creating the base class component for Mass Storage Interface. */
            status = _NU_USBF_DRVR_Create (&pcb_ms_drvr->parent,
                                           p_name,
                                           (UNSIGNED)(USB_MATCH_CLASS),
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT8)(USB_SPEC_MSF_CLASS),
                                           (UINT8)0,
                                           (UINT8)0,
                                           &usbf_ms_dispatch);
            if ( status == NU_SUCCESS )
            {
                pcb_ms_drvr->mem_pool = p_mem_pool;
    
                /* Add mass storage function with USB device configuration layer.
                     */
                /* Add descriptor for FULL, HIGH and SUPER speed. */
                status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                                 fs_hs_ms_intf_desc,
                                                 sizeof(fs_hs_ms_intf_desc),
                                                 fs_hs_ms_intf_desc,
                                                 sizeof(fs_hs_ms_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                                 ss_ms_intf_desc,
                                                 sizeof(ss_ms_intf_desc),
#endif
                                                 &pcb_ms_drvr->ms_function);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

#else
/**************************************************************************
* FUNCTION
*
*        NU_USBF_MS_Create
*
* DESCRIPTION
*
*        This function initializes the mass storage class driver. All the
*        internal data structures are initialized. Subsequently, users can
*        be registered with this class driver. Once, initialized
*        successfully, this class driver must be registered with a stack.
*        Together with a registered user, this enables the mass storage
*        functionality of the USB device. Mass storage class driver starts
*        responding to the Host requests to communicate with the mass
*        storage device.
*
* INPUTS
*
*        pcb_ms_drvr         Pointer to Nucleus USB Mass Storage class
*                            driver.
*        p_name              Name of the class driver.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful initialization.
*        NU_USB_INVLD_ARG    Indicates incorrect parameters.
*        NU_USB_NOT_PRESENT  Indicates a configuration problem because of
*                            which no more USB objects could be created.
*
**************************************************************************/
STATUS NU_USBF_MS_Create (NU_USBF_MS        *pcb_ms_drvr,
                          CHAR              *p_name)
{

    /* Local variables. */
    STATUS      status;
    UINT8       idx;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Validate the arguments. */
    if((!pcb_ms_drvr) || (!p_name))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Initialize the control block. */
        memset ((VOID *)pcb_ms_drvr, 0, sizeof(NU_USBF_MS));

        /* Allocate memory for CBW and CSW in each device for later use. */
        for(idx = 0; idx < NU_USBF_MS_MAX_INTERFACES; idx++ )
        {
            /* Allocate memory for CBW in each device control block
             * Exit from loop if there was an error.
             */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(MSF_BOT_CBW),
                                        (VOID**)&(pcb_ms_drvr->devices[idx].cbw));
            if ( status != NU_SUCCESS )
            {
                break;
            }
            
            /* Allocate memory for CSW in each device control block
             * Exit from loop if there was an error. 
             */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(MSF_BOT_CSW),
                                        (VOID**)&(pcb_ms_drvr->devices[idx].csw));
            if ( status != NU_SUCCESS )
            {
                break;
            }
            
            /* Allocate memory to hold value of Maximum LUN. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(UINT8),
                                        (void**)&(pcb_ms_drvr->devices[idx].max_lun));
            if ( status != NU_SUCCESS )
            {
                break;
            }
        }

        if ( status == NU_SUCCESS )
        {
            /* Creating the base class component for Mass Storage Interface. */
            status = _NU_USBF_DRVR_Create (&pcb_ms_drvr->parent,
                                           p_name,
                                           (UNSIGNED)(USB_MATCH_CLASS),
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT16)0,
                                           (UINT8)(USB_SPEC_MSF_CLASS),
                                           (UINT8)0,
                                           (UINT8)0,
                                           &usbf_ms_dispatch);
            if ( status == NU_SUCCESS )
            {
                /* Add mass storage function with USB device configuration layer.
                     */
                /* Add descriptor for FULL, HIGH and SUPER speed. */
                status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                                 fs_hs_ms_intf_desc,
                                                 sizeof(fs_hs_ms_intf_desc),
                                                 fs_hs_ms_intf_desc,
                                                 sizeof(fs_hs_ms_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                                 ss_ms_intf_desc,
                                                 sizeof(ss_ms_intf_desc),
#endif
                                                 &pcb_ms_drvr->ms_function);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}
#endif

/**************************************************************************
* FUNCTION
*
*        NU_USBF_MS_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds mass storage function with USB device
*       configuration. It is necessary that mass storage function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       pcb_ms_drvr             Pointer to mass storage function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that mass storage function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_MS_Bind_Interface (NU_USBF_MS *pcb_ms_drvr)
{

    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Mass storage interface is allowed to be bound only once. */
    if ( initialized == NU_FALSE )
    {
        status = NU_USB_INVLD_ARG;
        if ( pcb_ms_drvr != NU_NULL )
        {
            status = NU_NOT_PRESENT;
            if ( pcb_ms_drvr->ms_function != NU_NULL )
            {
                /* Enable mass storage function. */
                status = USBF_DEVCFG_Enable_Function(pcb_ms_drvr->ms_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind mass storage function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(pcb_ms_drvr->ms_function);
                    if ( status == NU_SUCCESS )
                    {
                        initialized = NU_TRUE;
                    }
                }
            }
        }
    }
    else
    {
        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_MS_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds mass storage function with USB device
*       configuration. It is necessary that mass storage function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       pcb_ms_drvr             Pointer to mass storage function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that mass storage function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_MS_Unbind_Interface (NU_USBF_MS *pcb_ms_drvr)
{

    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( pcb_ms_drvr != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( pcb_ms_drvr->ms_function != NU_NULL )
        {
            /* Bind mass storage function with USB device configuration. */
            status = USBF_DEVCFG_Unbind_Function(pcb_ms_drvr->ms_function);

            if ( status == NU_SUCCESS )
            {
                initialized = NU_FALSE;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Delete
*
* DESCRIPTION
*
*        Un-initializes the Mass storage class driver. It disowns all the
*        interfaces being served by this driver. Once uninitialized the
*        mass storage functionality of the device will be lost and the
*        device stops responding to any of the Host requests to communicate
*        with the device.
*
* INPUTS
*
*        pcb               Pointer to the mass storage class
*                          function driver.
*
* OUTPUTS
*
*        NU_SUCCESS        Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBF_MS_Delete (VOID *pcb)
{

    /* Local variables. */
    NU_USBF_MS *ms_ptr;
    UINT8       dev_index;
    STATUS      status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    if(pcb)
    {
        /* Initialize local variables. */
        ms_ptr = (NU_USBF_MS *) pcb;
        status = NU_SUCCESS;

        /* If there are pending devices, then release them all. */
        for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES;
                                        dev_index++)
        {
            if(ms_ptr->devices[dev_index].in_use != NU_FALSE)
            {
                /* Inform the stack that we disown the interface, we have
                 * been serving so far.
                 */
                status = NU_USB_INTF_Release (
                                        ms_ptr->devices[dev_index].intf,
                                             (NU_USB_DRVR *) ms_ptr);
                /* On failure exit loop and return status. */
                if(status != NU_SUCCESS)
                {
                    break;
                }

                /* Mark the device as free. */
                ms_ptr->devices[dev_index].in_use = NU_FALSE;
            }
            
            /* Deallocate memory for CBW and CSW control blocks. */
            if ( ms_ptr->devices[dev_index].cbw )
            {
                USB_Deallocate_Memory(ms_ptr->devices[dev_index].cbw);
                ms_ptr->devices[dev_index].cbw = NU_NULL;
            }
            if ( ms_ptr->devices[dev_index].csw )
            {
                USB_Deallocate_Memory(ms_ptr->devices[dev_index].csw);
                ms_ptr->devices[dev_index].csw = NU_NULL;
            }
            
            /* Deallocatge memory for Maximum LUN. */
            if ( ms_ptr->devices[dev_index].max_lun )
            {
                USB_Deallocate_Memory(ms_ptr->devices[dev_index].max_lun);
                ms_ptr->devices[dev_index].max_lun = NU_NULL;
            }
        }

        /* Successfully released all the interfaces. */
        if(status == NU_SUCCESS)
        {
            /* Call parent's delete. */
            status = _NU_USBF_DRVR_Delete (pcb);
            memset(pcb, 0, sizeof(NU_USBF_MS));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Set_Interface
*
* DESCRIPTION
*
*        Notifies driver of the change in alternate setting.
*        Alternate setting change callback. This function informs
*        associated client of the alternate setting change.
*
* INPUTS
*
*        pcb_drvr                Pointer to class driver control block.
*        pcb_stack               Pointer to the stack control block.
*        pcb_device              Pointer to the USB device on which this
*                                event has happened.
*        pcb_intf                Pointer to the interface control block.
*        pcb_alt_settg           Pointer to the new alternate setting for
*                                the interface.
* OUTPUTS
*
*        NU_SUCCESS              Indicates that the class driver could
*                                process the event successfully.
*        NU_USB_INVLD_ARG        Indicates the one of the input parameters
*                                has been incorrect AND/OR the event could
*                                not be processed without an error.
*        NU_NOT_PRESENT          Some required resource is not found.
*
**************************************************************************/
STATUS _NU_USBF_MS_Set_Interface (NU_USB_DRVR      *pcb_drvr,
                                  NU_USB_STACK     *pcb_stack,
                                  NU_USB_DEVICE    *pcb_device,
                                  NU_USB_INTF      *pcb_intf,
                                  NU_USB_ALT_SETTG *pcb_alt_settg)
{
    /* Local variables. */
    BOOLEAN            isclaimed;
    NU_USB_DRVR       *usb_drvr_ptr;
    NU_USB_USER       *prev_user_ptr;
    NU_USBF_MS_DEVICE *ms_device_ptr;
    UINT8              dev_index;
    NU_USBF_MS        *ms_ptr;
    STATUS             status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Initialize local variables. */
    ms_ptr = (NU_USBF_MS *) pcb_drvr;
    status = NU_SUCCESS;

    /* Dummy loop. It would be helpful in returning the function status by
     * breaking the loop as soon as any error occurs.
     */
    do
    {
        /* Look for an existing slot(device entry in devices array) and use
         * it.
         */
        for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES;
                                dev_index++)
        {
            if((ms_ptr->devices[dev_index].in_use == NU_TRUE) &&
               (ms_ptr->devices[dev_index].dev    == pcb_device) &&
               (ms_ptr->devices[dev_index].intf   == pcb_intf))
            {
                /* We have found the existing slot, exit loop. */
                break;
            }
        }

        if(dev_index == NU_USBF_MS_MAX_INTERFACES)
        {
            /* Device array does not contain any entry for this device. */
            status = NU_USB_INVLD_ARG;
            break;                          /* Exit loop. */
        }

        /* Store device entries from device array to local variables. This
         * helpful in accessing the device easily.
         */
        ms_device_ptr            = &ms_ptr->devices[dev_index];
        ms_device_ptr->dev       = (NU_USB_DEVICE*)pcb_device;
        ms_device_ptr->intf      = pcb_intf;
        ms_device_ptr->alt_settg = pcb_alt_settg;
        ms_device_ptr->usb_drvr  = pcb_drvr;

        /* Get previous user of this device. */
        prev_user_ptr = ms_device_ptr->user;

        /* If some user from previous connection is still associated with
         * this device.
         */
        if(prev_user_ptr != NU_NULL)
        {
            /* This case should never occur actually. Because on every
             * disconnection, we always issue user disconnect call back for
             * every user that is not NULL. But some hardware drivers don't
             * give disconnect notification to the class driver layer, so
             * on disconnection event NU_USB_USER_Disconnect API never gets
             * called and on second connection we find an associated user
             * that needs to be disconnected. This will help in running
             * mass storage demo (with out FILE) running even in the
             * absence of disconnection notification. This check may be
             * omitted in future when all hardware drivers have capability
             * to report disconnection event notification to the class
             * driver layer.
             */

            /* Mark the device free if we are no longer using. */
            if(ms_ptr->slot_no != dev_index)
            {
                ms_ptr->devices[ms_ptr->slot_no].in_use = NU_FALSE;
            }

            /* Call mass storage disconnection behavior. */
            status = _NU_USBF_MS_Disconnect (pcb_drvr,
                                             pcb_stack,
                                             pcb_device);

            /* Mark this device as not free because mass storage
             * disconnection functionality would mark it free.
             */
            ms_ptr->devices[dev_index].in_use = NU_TRUE;

        }

        /* On failure exit loop and return status. */
        if (status != NU_SUCCESS)
        {
            break;                           /* Exit loop. */
        }

        /* Release the interface if already claimed. */
        status = NU_USB_INTF_Get_Is_Claimed(pcb_intf,
                                            &isclaimed,
                                            &usb_drvr_ptr);

        if (status != NU_SUCCESS)
        {
            break;
        }

        /* If the interface is claimed already? */
        if(isclaimed)
        {
            /* Release already claimed interface. */
            status = NU_USB_INTF_Release(pcb_intf, usb_drvr_ptr);
        }

        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Create necessary environment for communication over USB, such as
         * creating IRPs and finding Bulk IN, Bulk OUT and control pipes.
         */
        status = NU_MSF_Initialize_Device(ms_device_ptr);

        /* If we fail to create the USB environment? */
        if(status != NU_SUCCESS)
        {
            break;                         /* Exit loop. */
        }

        /* Claim the interface. */
        status = NU_USB_INTF_Claim (pcb_intf, pcb_drvr);

        /* Did the driver successfully own the interface? */
        if (status != NU_SUCCESS)
        {
            break;
        }

        status = NU_MSF_Process_Connect_event (ms_device_ptr);
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Submit CBW request. */
        status = NU_MSF_Submit_Command_IRP(ms_device_ptr);

    }   while(0);                           /* End of dummy loop. */

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Connect
*
* DESCRIPTION
*
*        Set configuration callback. This function is invoked by the stack
*        when the driver is given an opportunity to own an interface.
*
* INPUTS
*
*        pcb_drvr            Pointer to the driver control block.s
*        pcb_stack           Pointer to stack control block.
*        pcb_device          Pointer to USB device that got connected.
*        pcb_intf            pointer to Interface control block.
*
* OUTPUTS
*
*        NU_SUCCESS          If the new ms device could be accommodated
*                            successfully.
*        NU_USB_INVLD_ARG    Invalid arguments.
*
**************************************************************************/
STATUS _NU_USBF_MS_Connect (NU_USB_DRVR   *pcb_drvr,
                            NU_USB_STACK  *pcb_stack,
                            NU_USB_DEVICE *pcb_device,
                            NU_USB_INTF   *pcb_intf)
{
    /* Local variables. */
    NU_USBF_MS *ms_ptr;
    STATUS      status;
    UINT8       dev_index;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_drvr) || (!pcb_stack) || (!pcb_device) || (!pcb_intf))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        if(status == NU_SUCCESS)
        {
            /* Initialize the local variables. */
            ms_ptr = (NU_USBF_MS *)pcb_drvr;
            status = NU_SUCCESS;

            /* Look for an available slot. */
            for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES; dev_index++)
            {
                if(ms_ptr->devices[dev_index].in_use == NU_FALSE ||
                   ms_ptr->devices[dev_index].intf == pcb_intf)
                {
                    /* Slot found. */
                    break;
                }
            }

            /* Was the devices array full? */
            if(dev_index == NU_USBF_MS_MAX_INTERFACES)
            {
                status = NU_USB_INVLD_ARG;
            }

            if(status == NU_SUCCESS)
            {

            /* Store device,interface,stack,user and class driver control block
             * in devices array at an index that is free.
                 */
                ms_ptr->devices[dev_index].dev    = pcb_device;
                ms_ptr->devices[dev_index].intf   = pcb_intf;
                ms_ptr->devices[dev_index].stack  = pcb_stack;
                ms_ptr->devices[dev_index].user   = NU_NULL;
                ms_ptr->devices[dev_index].drvr   = (NU_USBF_DRVR *)pcb_drvr;
                ms_ptr->devices[dev_index].in_use = NU_TRUE;
                ms_ptr->slot_no                   = dev_index;

                /* Call Set Interface routine to make the required interface as
                 * active.
                 */
                status = _NU_USBF_MS_Set_Interface (pcb_drvr,
                                                    pcb_stack,
                                                    pcb_device,
                                                    pcb_intf,
                                                    &pcb_intf->alt_settg[0]);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Disconnect
*
* DESCRIPTION
*
*        This function updates the control block and informs all associated
*        clients of this event.
*
* INPUTS
*
*        pcb_drvr            Pointer to Class Driver control block.
*        pcb_stack           Pointer to Stack control block.
*        pcb_device          Pointer to the USB device that got
*                            disconnected.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Invalid arguments.
*
**************************************************************************/
STATUS _NU_USBF_MS_Disconnect (NU_USB_DRVR   *pcb_drvr,
                               NU_USB_STACK  *pcb_stack,
                               NU_USB_DEVICE *pcb_device)
{
    /* Local variables. */
    UINT8              dev_index;
    NU_USBF_MS        *ms_ptr;
    NU_USBF_MS_DEVICE *ms_device_ptr;
    STATUS             status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_drvr) || (!pcb_stack) || (!pcb_device))
    {
        status = NU_USB_INVLD_ARG;
    }

    if(status == NU_SUCCESS)
    {
        /* Initialize local variables. */
        ms_ptr = (NU_USBF_MS *)pcb_drvr;

        /*  For each of the users associated with the mass storage device,
         *  1. Issue a disconnect callback.
         *  2. Remove device from device list.
         */
        for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES; dev_index++)
        {
            /* Get Mass Storage device from the device list. */
            ms_device_ptr = &ms_ptr->devices[dev_index];

            /* If the device is not free and its device pointer matches
             * with that of the caller.
             */
            if((ms_device_ptr->in_use == NU_TRUE) &&
               (ms_device_ptr->dev == pcb_device))
            {

                if(ms_device_ptr->user != NU_NULL)
                {
                    /* There is a user associated with this Mass Storage
                     * device Issue disconnect callback to the associated
                     * user.
                     */
                    status = NU_USB_USER_Disconnect (ms_device_ptr->user,
                                                     pcb_drvr,
                                                 (VOID *)ms_device_ptr);

                    /* If Disconnect returned error? */
                    if (status != NU_SUCCESS)
                    {
                      /* Exit the loop. */
                      break;
                    }
                }
                if(status != NU_SUCCESS)
                {
                    break;
                }

                /* Remove the device from the device array by marking the
                 * device as free one.
                 */

                ms_device_ptr->user         = NU_NULL;
                ms_device_ptr->in_use       = NU_FALSE;

                /* Reset the state variables. */
                ms_device_ptr->invalid_cbw  = NU_FALSE;
                ms_device_ptr->phase_error  = NU_FALSE;
                ms_device_ptr->cmd_failed   = NU_FALSE;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Class_Request
*
* DESCRIPTION
*
*       Processes a new class specific SETUP packet from the Host.
*
*       The format of the setup packet is defined by the corresponding
*       class specification / custom protocol. The setup packet is
*       validated by this function and processed further as per the class
*       specification.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status.
*
* INPUTS
*
*        pcb_drvr                   Pointer to Class Driver control block.
*        pcb_stack                  Pointer to Stack control block of the
*                                   calling stack.
*        pcb_device                 USB device on which the Event occurred.
*        pcb_setup                  Setup packet that has been received.
*
* OUTPUTS
*
*        NU_SUCCESS                 If the command is processed
*                                   successfully.
*        NU_USB_INVLD_ARG           On invalid command.
*
**************************************************************************/
STATUS _NU_USBF_MS_Class_Specific (NU_USB_DRVR      *pcb_drvr,
                                   NU_USB_STACK     *pcb_stack,
                                   NU_USB_DEVICE    *pcb_device,
                                   NU_USB_SETUP_PKT *pcb_setup)
{
    /* Local variables. */
    UINT8              dev_index;
    NU_USBF_MS_DEVICE *ms_device_ptr;
    NU_USB_INTF       *intf_ptr;
    NU_USB_IRP        *irp_ptr;
    NU_USB_CFG        *cfg_ptr;
    STATUS             status;
    NU_USBF_MS        *ms_ptr;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    cfg_ptr = NU_NULL;
    ms_ptr = NU_NULL;
    intf_ptr = NU_NULL;
    ms_device_ptr = NU_NULL;

    status = NU_SUCCESS;

    if((!pcb_drvr) || (!pcb_stack) || (!pcb_device) || (!pcb_setup))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        if(status == NU_SUCCESS)
        {
            /* Initialize local variables. */
            ms_ptr = (NU_USBF_MS *) pcb_drvr;
            
           /* Get active configuration pointer. */
           status = NU_USB_DEVICE_Get_Active_Cfg((NU_USB_DEVICE*)pcb_device,
                                                 &cfg_ptr);
        }

        if(status == NU_SUCCESS)
        {
            /* Get interface descriptor from configuration descriptor. */
            status = NU_USB_CFG_Get_Intf (cfg_ptr,
                                         (UINT8)pcb_setup->wIndex,
                                         &intf_ptr);
        }

        if(status == NU_SUCCESS)
        {
            ms_device_ptr = NU_NULL;        /* Reset device pointer. */

            for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES;
                                                    dev_index++)
            {
                /* Get device form the list of devices. */
                ms_device_ptr = &ms_ptr->devices[dev_index];

                if((ms_device_ptr->in_use == NU_TRUE) &&
                (ms_device_ptr->dev    == pcb_device) &&
                (ms_device_ptr->intf   == intf_ptr))
                {
                    /* Required device control block found from devices
                     * list.
                     */
                     break;
                }
            }

            /* If we found the device or loop terminating condition
             * arrived.
             */
            if((dev_index == NU_USBF_MS_MAX_INTERFACES) ||
                                    (ms_device_ptr->user == NU_NULL))
            {
                status = NU_USB_INVLD_ARG;
            }
        }

        if(status == NU_SUCCESS)
        {
            /* Parse the setup token. */
            switch (pcb_setup->bRequest)
            {

                /* Mass Storage Class Specific Reset request. */
                case USB_SPEC_MSF_CLASS_RESET:

                    /* Validate setup package */
                    if ((pcb_setup->wLength != 0) ||
                        (pcb_setup->wValue != 0) ||
                        (pcb_setup->wIndex != ms_device_ptr->intf->intf_num))
                    {
                        /* Invalid arguments stall control pipe.*/
                        status = NU_USB_INVLD_ARG;
                        break;
                    }

                    /* Stop stalling the BULK end points. */
                    ms_device_ptr->invalid_cbw = NU_FALSE;
                    /* If pipes are already unstall/Flushed some controllers
                     * might set return status as invalid argument so not
                     * checking the return status. Unstall endpoints if
                     * stalled.
                     */
                    status = NU_MSF_Unstall_Pipes((VOID *)ms_device_ptr);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }

                    /* Cancel pending transfer over Bulk IN pipe. */
                    status = NU_USB_PIPE_Flush (ms_device_ptr->in_pipe);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }

                    /* Cancel pending transfer over Bulk OUT pipe. */
                    status = NU_USB_PIPE_Flush (ms_device_ptr->out_pipe);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }

                    /* Reset the state variables. */
                    ms_device_ptr->data_direction = 0;
                    ms_device_ptr->cmd_failed     = 0;
                    ms_device_ptr->phase_error    = 0;

                    /* Reset the client. */
                    status = NU_USBF_USER_MS_Reset (
                                                    ms_device_ptr->user,
                                                    pcb_drvr,
                                                    (VOID *)ms_device_ptr);

                    if (status == NU_SUCCESS)
                    {
                        /* Submit the CBW request. */
                        status = NU_MSF_Submit_Command_IRP(ms_device_ptr);
                    }

                    break;

                case USB_SPEC_MSF_CLASS_GET_MAX_LUN:

                    /* Validate setup package */
                    if ((pcb_setup->wLength != 1) ||
                        (pcb_setup->wValue != 0) ||
                        (pcb_setup->wIndex != ms_device_ptr->intf->intf_num))
                    {

                        /* Return invalid arguments.*/
                        status = NU_USB_INVLD_ARG;
                        break;
                    }

                    /* Get Max LUN command arrived. */
                    status = NU_USBF_USER_MS_Get_Max_LUN (
                               ms_device_ptr->user,
                               pcb_drvr,
                               (VOID *)ms_device_ptr,
                               (ms_device_ptr->max_lun));

                    if (status == NU_SUCCESS)
                    {

                        /* Fill in IRP length, buffer , status and
                         * submit this IRP.
                         */
                        irp_ptr         = &(ms_device_ptr->ctrl_irp);
                        irp_ptr->buffer = (ms_device_ptr->max_lun);

                        irp_ptr->length = 1;
                        irp_ptr->status = NU_SUCCESS;

                        /* Submit IRP over control pipe. */
                        status = NU_USB_PIPE_Submit_IRP (
                                              ms_device_ptr->ctrl_pipe,
                                              irp_ptr);
                    }

                    break;

                    /* This case should never occur. This indicates
                     * invalid setup request.
                     */
    
                    default:
                        status = NU_USB_INVLD_ARG;
                        break;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_New_Transfer
*
* DESCRIPTION
*
*        This routine only returns successful status. This function is
*        called when an IRP is not submitted over BULK OUT and data arrives.
*        It is controller driver responsibility to retain data until IRP is
*        submitted by the class driver.
*
* INPUTS
*
*        pcb_drvr             Pointer to Class Driver control block.
*        pcb_stack            Stack control block.
*        pcb_device           USB device on which the Event occurred.
*        pcb_pipe             Pipe that has received new transfer.
*
* OUTPUTS
*
*        NU_SUCCESS           Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBF_MS_New_Transfer (NU_USB_DRVR   *pcb_drvr,
                                 NU_USB_STACK  *pcb_stack,
                                 NU_USB_DEVICE *pcb_device,
                                 NU_USB_PIPE   *pcb_pipe)
{
    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_MS_Notify
*
* DESCRIPTION
*
*        This function notifies all client drivers of the event occurred.
*
* INPUTS
*
*        pcb_drvr                Pointer to Class Driver control block.
*        pcb_stack               Stack Control block.
*        pcb_device              USB device on which the Event occurred.
*        event                   Event that has occurred.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBF_MS_Notify (NU_USB_DRVR   *pcb_drvr,
                           NU_USB_STACK  *pcb_stack,
                           NU_USB_DEVICE *pcb_device,
                           UINT32         event)
{

    /* Local variables. */
    UINT8              dev_index;
    NU_USBF_USER      *user_ptr;
    NU_USBF_MS        *ms_ptr;
    NU_USBF_MS_DEVICE *ms_device_ptr;

    STATUS        status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_drvr) || (!pcb_stack) || (!pcb_device))
    {
        status = NU_USB_INVLD_ARG;
    }

    if(status == NU_SUCCESS)
    {
        ms_ptr = (NU_USBF_MS*)pcb_drvr;

        /* For each of the Mass Storage devices, check if mass storage
         * device is present.
         */
        for (dev_index = 0; dev_index < NU_USBF_MS_MAX_INTERFACES;
                                        dev_index++)
        {
            if((ms_ptr->devices[dev_index].in_use == NU_TRUE) &&
               (ms_ptr->devices[dev_index].dev == pcb_device))
            {

                /* If device is matching, notify the corresponding user of
                 * this event?
                 */
                user_ptr = (ms_ptr->devices[dev_index].user);
                if(user_ptr != NU_NULL)
                {

                    ms_device_ptr = &ms_ptr->devices[dev_index];

                    /* Notify the user about the occurred event. */
                    status = NU_USBF_USER_Notify (user_ptr,
                                                  pcb_drvr,
                                                  (VOID *)ms_device_ptr,
                                                  event);

                    /* Was the notification sent to the user successfully?
                     */
                    if(status != NU_SUCCESS)
                    {
                        break;                  /* Exit loop. */
                    }

                    /* Identify and process the event accordingly. */
                    switch(event)
                    {

                        /* Reset event occurred. */
                        case  USBF_EVENT_RESET:
                            break;

                        /* Clear Feature event occurred. */
                        case USBF_EVENT_CLEAR_HALTENDPOINT:

#ifdef  NU_USBF_MS_TASK_MODE
                            /* Set event to process clear feature event in
                             * task mode.
                             */
                            status = NU_Set_Events (&ms_device_ptr->event_group,
                            NU_USBF_MS_CLEAR_FEATURE,
                            NU_OR);
#else
                            /* Do appropriate actions on clear feature. */
                            status =
                              NU_MSF_Clear_Feature_Callback(ms_device_ptr);
#endif

                            break;

                        /* On disconnection notification call mass storage
                         * disconnection behavior.
                         */
                        case  USBF_EVENT_DISCONNECT:

                            /* NU_USBF_MS_Connect callback is called only
                             * once in case of hardware that handle set
                             * configuration request automatically(sh3dsp
                             * with sh7727r2 function controller) and will
                             * never trigger again on second/third... time
                             * when device is connected with the host. If
                             * we disconnect the user here(for such cases)
                             * pipes would be flushed, resources allocated
                             * would be lost. For such hardware since
                             * NU_USBF_MS_Connect would never be called
                             * again. Mass Storage device would never
                             * enumerate after 1st connection. Do not give
                             * disconnection notification if the hardware
                             * is capable of handling set configuration
                             * request.
                             */

                            if( !( (ms_ptr->hw_capability) &
                                   (0x01 << USB_SET_CONFIGURATION) ) )
                            {
                                status = _NU_USBF_MS_Disconnect (pcb_drvr,
                                                               pcb_stack,
                                                               pcb_device);
                            }

                            break;

                        /* Some other event other than Reset and
                         * disconnect.
                         */
                        default:
                            break;               /* Do nothing. */
                    }

                    /* Exit loop in case of error status. */
                    if(status != NU_SUCCESS)
                    {
                        break;
                    }
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

#ifdef NU_USBF_MS_TASK_MODE
/**************************************************************************
* FUNCTION
*
*        NU_USBF_MS_Start_Cmd_Processing
*
* DESCRIPTION
*
*        This function would enable device to accept command from the Host
*        side.
*
* INPUTS
*
*        pcb_ms_device                Pointer to device control block.
*
* OUTPUTS
*
*        NU_SUCCESS                   Indicates successful completion.
*        NU_USB_INVLD_ARG             Invalid arguments.
*
**************************************************************************/
STATUS NU_USBF_MS_Start_Cmd_Processing (
                                         NU_USBF_MS_DEVICE *pcb_ms_device)
{

    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    if(pcb_ms_device)
    {
        /* Submit IRP for CBW. */
        status = NU_MSF_Submit_Command_IRP (pcb_ms_device);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}
#endif

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_MS_Set_Command_Direction
*
*   DESCRIPTION
*
*       This function is called to retrieve the function mass storage
*       class driver control block handle.
*
*   INPUTS
*
*       handle              Pointer used to retrieve the class
*                           driver's address.
*       direction           To be set up by the user.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function mass storage
*                           class driver.
*       NU_USB_INVLD_ARG    Invalid arguments.
*
*************************************************************************/
STATUS NU_USBF_MS_Set_Command_Direction(VOID  *handle, UINT8 direction)
{

    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    if ( handle != (VOID *)0)
    {
        ((NU_USBF_MS_DEVICE *)handle)->data_direction = direction;

        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_MS_Init
*
*   DESCRIPTION
*
*       This function initializes the Mass Storage Component.
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
STATUS nu_os_conn_usb_func_ms_class_init(CHAR *path, INT startstop)
{
    VOID   *stack_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS, reg_status;
    CHAR    usb_func_ms_path[80];

    usb_func_ms_path[0] = '\0';
    strcat(usb_func_ms_path, path);

    /* Save registry settings of USB Function Mass Storage. */
    strcat(usb_func_ms_path, "/configstring");
    reg_status = REG_Get_String(usb_func_ms_path, USBF_MS_Configuration_String, NU_USB_MAX_STRING_LEN);
    if(reg_status == NU_SUCCESS)
    {
        usb_func_ms_path[0] = '\0';
        strcat(usb_func_ms_path, path);
        strcat(usb_func_ms_path, "/interfacestring");
        reg_status = REG_Get_String(usb_func_ms_path, USBF_MS_Interface_String, NU_USB_MAX_STRING_LEN);
    }

    if (startstop == RUNLEVEL_START)
    {
        /* Allocate memory for USB function mass storage. */
        status = USB_Allocate_Object(sizeof(NU_USBF_MS), (VOID **)&NU_USBF_MS_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset((VOID *)NU_USBF_MS_Cb_Pt, 0, sizeof(NU_USBF_MS));

#ifdef  NU_USBF_MS_TASK_MODE
            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBF_MS_Create_Task_Mode (NU_USBF_MS_Cb_Pt, "USBF-MS",
                                                  NU_NULL);
#else
            status = NU_USBF_MS_Create (NU_USBF_MS_Cb_Pt, "USBF-MS");
#endif
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the function stack handle */
        if(!rollback)
        {
            status = NU_USBF_Init_GetHandle (&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

         /* Register to the stack */
        if(!rollback)
        {
            status = NU_USBF_STACK_Register_Drvr ((NU_USBF_STACK *) stack_handle,
                                                 (NU_USB_DRVR *) NU_USBF_MS_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if(!rollback)
        {
            internal_sts = nu_os_conn_usb_func_ms_user_init(path, startstop);
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBF_MS_Delete ((VOID *) NU_USBF_MS_Cb_Pt);

            case 2:
                if (NU_USBF_MS_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_MS_Cb_Pt);
                    NU_USBF_MS_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused variable.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        if (NU_USBF_MS_Cb_Pt)
        {
            do
            {
                status = nu_os_conn_usb_func_ms_user_init(usb_func_ms_path, startstop);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = NU_USBF_Init_GetHandle (&stack_handle);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = NU_USB_STACK_Deregister_Drvr((NU_USB_STACK *) stack_handle,(NU_USB_DRVR *) NU_USBF_MS_Cb_Pt);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = _NU_USBF_MS_Delete(NU_USBF_MS_Cb_Pt);
                if ( status != NU_SUCCESS )
                {
                    break;
                }
                status = USB_Deallocate_Memory(NU_USBF_MS_Cb_Pt);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

            } while(0);
        }
        else
        {
            status = NU_INVALID_POINTER;
        }
    }

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_MS_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function mass storage
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function mass storage
*                           class driver.
*       NU_NOT_PRESENT      Indicate there exists .
*
*************************************************************************/
STATUS NU_USBF_MS_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if(handle)
    {
        status = NU_SUCCESS;
        *handle = (VOID *)NU_USBF_MS_Cb_Pt;
        if (NU_USBF_MS_Cb_Pt == NU_NULL)
        {
            status = NU_NOT_PRESENT;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_MS_RW
*
*   DESCRIPTION
*
*         Function called by user driver to send/recv data
*
*
*   INPUTS
*
*   handle                    MS device handle
*   buffer                    Pointer to data to be sent OR Data recv pointer
*   numbytes                  Number of bytes to transfer in data
*   byte_offset               Bytes offset
*
*
*   OUTPUTS
*
*
*   NU_SUCCESS                Successful operation
*   NU_USB_INVLD_ARG          When MS BOT device is not in data stage
*
*
*************************************************************************/
STATUS NU_USBF_MS_RW(VOID*      handle,
                    VOID*       buffer,
                    UINT32      numbyte,
                    OFFSET_T    byte_offset)
{
    STATUS              status;
    NU_USB_PIPE         *pipe_ptr;
    UINT8               cbw_flags;
    NU_USBF_MS_DEVICE   *pcb_ms_device;
    MSF_BOT_CBW         *cbw_ptr;

    pcb_ms_device   = (NU_USBF_MS_DEVICE *)handle;
    cbw_ptr         = (pcb_ms_device->cbw);

    /* BOT not in data stage. Ignore RW request. */
    if( pcb_ms_device->bot_stage != USBF_MS_BOT_STAGE_DATA )
    { 
        return NU_USB_INVLD_ARG;
    } 

    status    = NU_USB_INVLD_ARG;
    cbw_flags = cbw_ptr->bCBWFlags & USB_DIR_IN;
    pipe_ptr  = (cbw_flags == USB_DIR_IN)?
                 pcb_ms_device->in_pipe : pcb_ms_device->out_pipe;

    /* Case # 6. */
    if( numbyte <= cbw_ptr->dCBWTransferLength )
    {
        /* case # 9 and 4. */
        if(!numbyte)
        {
            pcb_ms_device->cmd_failed = NU_TRUE;
            status = NU_USB_PIPE_Stall(pipe_ptr);
        }
        
        else
        {
            /* Case # 5 & 11 -- case # 6 & 12. Prepare IRP to submit data.
             */
            status = NU_USB_IRP_Create(&pcb_ms_device->bulk_in_irp,
                                       numbyte,
                                       (UINT8*)buffer,
                                       NU_FALSE,
                                       NU_FALSE,
                                       NU_MSF_Data_IRP_Complete,
                                       (VOID *)pcb_ms_device,
                                       0);
            if(status == NU_SUCCESS)
            {

                /* Update the residue. */
                pcb_ms_device->residue -= pcb_ms_device->bulk_in_irp.length;

                /* Submit the transfer. */
                status = NU_USB_PIPE_Submit_IRP (pipe_ptr,
                                                 &pcb_ms_device->bulk_in_irp);
            }
        }    
    }

    /* Following cases are separated from the above as they are not
     * likely to occur every time as these are error cases. We do not
     * need to check them in normal flow.
     */
    else
    {
        /* Case # 13 & 7 */
        pcb_ms_device->phase_error = NU_TRUE;
        status = NU_USB_PIPE_Stall(pipe_ptr);
    }

    return ( status );
}
/************************* End Of File ***********************************/
