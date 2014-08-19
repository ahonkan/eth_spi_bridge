/*************************************************************************
*
*                  Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

ahonkan   Jul 16 2014     Terabit Radios
Modifying the LWSPI device interfaces definitions and functions to allow 
both master and slave operation on SPI.



**************************************************************************
* FILE NAME
*
*       lwspi.c
*
* COMPONENT
*
*       Nucleus lightweight SPI
*
* DESCRIPTION
*
*       This file contains data structures and APIs provided with
*       Nucleus lightweitht SPI.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       nu_os_conn_lwspi_init               Entry function of lightweight 
*                                           SPI component.
*       LWSPI_Bus_Register                  SPI device register 
*                                           notification callback 
*                                           function.
*       LWSPI_Bus_Unregister                SPI device un-register 
*                                           notification callback 
*                                           function.
*       NU_SPI_Register_Device              Registers a device with SPI 
*                                           bus.
*       NU_SPI_Unregister_Device            Un-register a device with
*                                           SPI bus.
*       NU_SPI_Read                         Reads data from a specified 
*                                           SPI device.
*       NU_SPI_Write                        Writes data to a specified
*                                           SPI device.
*       NU_SPI_Write_Read                   Perform read / write from 
*                                           a specified SPI device.
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       lwspi.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"
#include    "connectivity/nu_connectivity.h"
#include    "drivers/nu_drivers.h"
#include    "os/connectivity/lwspi/inc/lwspi_defs.h"

/**********************/
/* External Variables */
/**********************/
extern  NU_MEMORY_POOL  System_Memory;

/*******************/
/* Local Variables */
/******************/
NU_SPI_BUS  SPI_Bus_CB[LWSPI_NUM_BUSES];

/*************************************************************************
* FUNCTION
*
*       nu_os_conn_lwspi_init       
*
* DESCRIPTION
*
*       This is the entry function of lightweight SPI component. It 
*       is called durint initialization from specified run level.
*
* INPUTS
*
*       key                                 Path to registry entry of 
*                                           this component.
*       compctrl                            A flag specifying desired 
*                                           operation on component. 
*                                           Its value will be one of 
*                                           these.
*                                            - RUNLEVEL_STOP
*                                            - RUNLEVEL_START
*                                            - RUNLEVEL_HIBERNATE
*                                            - RUNLEVEL_RESUME
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
STATUS nu_os_conn_lwspi_init(const CHAR * key, INT compctrl)
{
    STATUS              status;
    DV_DEV_LABEL        device_type_label[] = {{LWSPI_LABEL}}; 
    DV_LISTENER_HANDLE  listener_handle;

    if(compctrl == RUNLEVEL_START)
    {
        /* Reset global bus structure. */
        memset(&SPI_Bus_CB, 0x00, (sizeof(NU_SPI_BUS) * LWSPI_NUM_BUSES));
        
        /* Register a listener for SPI label with device manager. */
        status = DVC_Reg_Change_Notify(device_type_label,
                                    1,
                                    LWSPI_Bus_Register,
                                    LWSPI_Bus_Unregister,
                                    NU_NULL,
                                    &listener_handle);
    }
    else
    {
        /* Always return success. */
        status = NU_SUCCESS;
    }
     
    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       LWSPI_Bus_Register
*
* DESCRIPTION
*
*       This is the callback function, called by device manager when a 
*       new SPI device will register with device manager using lightweight 
*       SPI label.
*
* INPUTS
*
*       spi_dev_id                          Device ID of newly registered
*                                           SPI device.
*       context                             An option context.
*
* OUTPUTS
*
*       NU_SPI_NO_FREE_BUS_SLOT             If no free bus slot is 
*                                           found.
*       NU_SPI_INVLD_IO_PTR                 Fetched I/O pointers are 
*                                           invalid.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Bus_Register(DV_DEV_ID spi_dev_id, VOID *context)
{
    DV_DEV_LABEL        device_type_label[] = {{LWSPI_LABEL}};
    NU_SPI_BUS          *spi_bus;
    STATUS              status;
    DV_IOCTL0_STRUCT    ioctl0;
    
    /* Get a free NU_SPI_BUS control block. */
    status = LWSPI_Find_Vacant_Bus_Slot(&spi_bus);
    if(status == NU_SUCCESS)
    {
        /* Open the device */
        status =  DVC_Dev_ID_Open (spi_dev_id,
                                   device_type_label, 
                                   1,
                                   &spi_bus->dev_handle);
    }

    if(status == NU_SUCCESS)
    {
        /* Save device ID for later use. */
        spi_bus->dev_id = spi_dev_id;
        
        /* Copy lightweight SPI label for executing IOCTL 0. */
        memcpy(&ioctl0.label, device_type_label, sizeof(DV_DEV_LABEL));
        
        /* Get SPI base IOCTL ID. */
        status = DVC_Dev_Ioctl(spi_bus->dev_handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));
    }

    if(status == NU_SUCCESS)
    {
        /* Save SPI ioctl base in the SPI bus control block. */
        spi_bus->ioctl_base = ioctl0.base;
        
        /* Initialize bus lock. */
        status = NU_Create_Semaphore(&spi_bus->bus_lock,
                                    "SPI_BL",
                                    1,
                                    NU_PRIORITY_INHERIT);
    }

    if(status == NU_SUCCESS)
    {
        /* Fetch necessary information about this SPI bus. This include following.
         *  - Name of SPI device to be used as bus name. 
         *  - I/O pointers for this SPI bus.
         *  - Get device context. Value returned in this IOCTL will be passed 
         *   during I/O operations.
         */
        status = DVC_Dev_Ioctl(spi_bus->dev_handle,
                                        (spi_bus->ioctl_base + LWSPI_IOCTL_GET_BUS_INFO),
                                        spi_bus,
                                        0);
    }

#if (CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE == NU_TRUE)
    if(status == NU_SUCCESS)
    {
        /* Check if retrieved bus information is valid. */
        if((spi_bus->dev_context == NU_NULL)        ||
            (spi_bus->io_ptrs.configure == NU_NULL) ||
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
            (spi_bus->io_ptrs.isr_read == NU_NULL)  ||
            (spi_bus->io_ptrs.isr_write == NU_NULL) ||
            (spi_bus->io_ptrs.isr_write_read == NU_NULL)  ||
#endif
            (spi_bus->io_ptrs.read == NU_NULL)      ||
            (spi_bus->io_ptrs.write == NU_NULL)     ||
            (spi_bus->io_ptrs.write_read == NU_NULL))
        {
            /* If retrieved bus information is not correct then 
             * do clean-up befor exiting.
             *  - Close device.
             *  - Detelete bus lock.
             *  - Free bus immediately .
             */
            DVC_Dev_Close(spi_bus->dev_handle);
            
            NU_Delete_Semaphore(&spi_bus->bus_lock);
                        
            spi_bus->is_used = NU_FALSE;
                        
            /* Return invalid pointers status. */
            status = NU_SPI_INVLD_IO_PTR;
        }
    }
#endif
  
    return (status);
}
/*************************************************************************
* FUNCTION
*
*       LWSPI_Bus_Unregister
*
* DESCRIPTION
*
*       This is the callback function, called by device manager when a 
*       device with lightweight SPI label un-registers itself with device 
*       manager.
*
* INPUTS
*
*       spi_dev_id                          Device ID of un-registered 
*                                           SPI device.
*       context                             An optional context.
*
* OUTPUTS
*
*       NU_SPI_BUS_SLOT_NOT_FOUND           Bus slot with specified ID 
*                                           is not found.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Bus_Unregister(DV_DEV_ID spi_dev_id, VOID *context)
{
    UINT8           device_idx;
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    STATUS          status;
    
    /* Search SPI bus identified by 'spi_dev_id'. */
    status = LWSPI_Find_Bus_Slot_By_ID(spi_dev_id, &spi_bus);
    if(status == NU_SUCCESS)
    {
        /* Close SPI device. */
        DVC_Dev_Close(spi_bus->dev_handle);
        
        /* Unregister all devices. */
        for(device_idx=0; device_idx<LWSPI_NUM_DEVICES; device_idx++)
        {
            spi_device = spi_bus->spi_devices[device_idx];
            if(spi_device != NU_NULL)
            {
                NU_SPI_Unregister(spi_device->handle);
            }
        }
        
        /* Delete bus lock. */
        NU_Delete_Semaphore(&spi_bus->bus_lock);

        /* Mark bus slot free for later use. */
        spi_bus->is_used = NU_FALSE;
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       LWSPI_Find_Vacant_Bus_Slot
*
* DESCRIPTION
*
*       This function finds a vacant bus slot. If a vacant slot is 
*       available then its pointer is returned in output argument 
*       spi_bus.
*
* INPUTS
*
*       spi_bus                             An output argument containing 
*                                           pointer to the vacant bus slot 
*                                           if avaialble, otherwise 
*                                           NU_NULL.
*
* OUTPUTS
*
*       NU_SPI_NO_FREE_BUS_SLOT             No free bus slot is available.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Find_Vacant_Bus_Slot(NU_SPI_BUS **spi_bus)
{
    UINT8   bus_idx;
    STATUS  status;
    
    /* Initialize local variables. */
    *spi_bus    = NU_NULL;
    status      = NU_SPI_NO_FREE_BUS_SLOT;

    /* Look for a vacant bus slot. */
    for(bus_idx=0; bus_idx<LWSPI_NUM_BUSES; bus_idx++)
    {
        if(!SPI_Bus_CB[bus_idx].is_used)
        {
            *spi_bus            = &SPI_Bus_CB[bus_idx];
            (*spi_bus)->is_used = NU_TRUE;
            (*spi_bus)->index   = bus_idx;
            status              = NU_SUCCESS;
            break;
        }
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       LWSPI_Find_Vacant_Device_Slot
*
* DESCRIPTION
*
*       This function finds a vacant device slot. If a vacant slot is 
*       available then its pointer is returned in output argument 
*       spi_device.
*
* INPUTS
*
*       spi_bus                             Pointer to SPI bus with which
*                                           device is associated.
*       spi_device                          An output argument containing 
*                                           address to SPI device when 
*                                           function returns.
*                                           If no vacant slot is available
*                                           then this will contain NU_NULL.
*
* OUTPUTS
*
*       NU_SPI_NO_FREE_DEVICE_SLOT          No free device slot is 
*                                           available.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Find_Vacant_Device_Slot( NU_SPI_BUS      *spi_bus,
                                            NU_SPI_DEVICE   **spi_device)
{
    UINT8   device_idx;
    STATUS  status;
    
    /* Initialize local variables. */
    *spi_device  = NU_NULL;
    status      = NU_SPI_NO_FREE_DEVICE_SLOT;

    /* Look for a vacant device slot. */
    for(device_idx=0; device_idx<LWSPI_NUM_DEVICES; device_idx++)
    {
        if(spi_bus->spi_devices[device_idx] == NU_NULL)
        {
            /* Allocate memory for device. */
            status = NU_Allocate_Memory(&System_Memory,
                                        (VOID**)spi_device,
                                        sizeof(NU_SPI_DEVICE),
                                        NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                /* Reset contents of newly allocated SPI device. */
                memset(*(spi_device), 0x00, sizeof(NU_SPI_DEVICE));

                (*spi_device)->index         = device_idx;
                spi_bus->spi_devices[device_idx]  = *spi_device;
            }
            break;
        }
    }
    
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       LWSPI_Find_Bus_Slot_By_Name
*
* DESCRIPTION
*
*       This function finds a bus slot by name. If specified bus slot is 
*       available then its pointer is returned in output argument spi_bus.
*
* INPUTS
*
*       name                                Name of bus.
*       spi_bus                             An output argument containing 
*                                           address to SPI bus wehen 
*                                           function returns.
*                                           If no bus with specified name 
*                                           exists then it contains 
*                                           NU_NULL.
*
* OUTPUTS
*
*       NU_SPI_BUS_SLOT_NOT_FOUND           No bus with specified name was
*                                           found.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Find_Bus_Slot_By_Name(CHAR *name, NU_SPI_BUS **spi_bus)
{
    STATUS  status;
    UINT8   bus_index;
    
    /* Initialize local variables. */
    status      = NU_SPI_BUS_SLOT_NOT_FOUND;
    *spi_bus    = NU_NULL;
    
    /* Look for a bus slot with specified name. */
    for(bus_index=0; bus_index<LWSPI_NUM_BUSES; bus_index++)
    {
        if(!strncmp(SPI_Bus_CB[bus_index].name,
                    name,
                    NU_SPI_BUS_NAME_LEN))
        {
            *spi_bus    = &SPI_Bus_CB[bus_index];
            status      = NU_SUCCESS;
            break;
        }
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       LWSPI_Find_Bus_Slot_By_ID
*
* DESCRIPTION
*
*       This function finds a bus slot by ID. If specified bus slot is 
*       available then its pointer is returned in output argument spi_bus.
*
* INPUTS
*
*       id                                  SPI bus ID.
*       spi_bus                             An output argument containing 
*                                           address to SPI bus wehen 
*                                           function returns.
*                                           If no bus with specified ID 
*                                           exists then it contains 
*                                           NU_NULL.
*
* OUTPUTS
*
*       NU_SPI_BUS_SLOT_NOT_FOUND           No bus with specified name was
*                                           found.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Find_Bus_Slot_By_ID(DV_DEV_ID id, NU_SPI_BUS **spi_bus)
{
    STATUS  status;
    UINT8   bus_index;
    
    /* Initialize local variables. */
    status      = NU_SPI_BUS_SLOT_NOT_FOUND;
    *spi_bus    = NU_NULL;
    
    /* Look for a bus slot with specified device ID. */
    for(bus_index=0; bus_index<LWSPI_NUM_BUSES; bus_index++)
    {
        if(SPI_Bus_CB[bus_index].dev_id == id)
        {
            *spi_bus    = &SPI_Bus_CB[bus_index];
            status      = NU_SUCCESS;
            break;
        }
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       LWSPI_Get_Params_From_Handle
*
* DESCRIPTION
*
*       This function decodes the input handle and returns SPI bus and 
*       parameters as output arguments.
*
* INPUTS
*
*       handle                              Input handle of device.
*       spi_bus                             An output argument containing 
*                                           address to SPI bus wehen 
*                                           function returns.
*       spi_device                          An output argument containing 
*                                           address to SPI device when 
*                                           function returns.
*
* OUTPUTS
*
*       NU_SPI_INVLD_ARG                    If an input argument is 
*                                           invalid.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
static STATUS LWSPI_Get_Params_From_Handle( NU_SPI_HANDLE   handle, 
                                            NU_SPI_BUS      **spi_bus,
                                            NU_SPI_DEVICE   **spi_device)
{
    UINT8   bus_idx, device_idx;
    STATUS  status;

    /* Initialize status with an error case. */
    status = NU_SPI_INVLD_ARG;
    
#if (CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check for valid handle. */
    if(NU_SPI_VALIDATE_HANDLE(handle))
#endif
    {
        /* Decode handle to get index of SPI bus and device. */
        NU_SPI_DECODE_HANDLE(handle, &bus_idx, &device_idx);
#if (CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE == NU_TRUE)
        if((bus_idx < LWSPI_NUM_BUSES) && (device_idx < LWSPI_NUM_DEVICES))
#endif
        {
            /* Get pointer to SPI bus. */
            *spi_bus = &SPI_Bus_CB[bus_idx];
            
#if (CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE == NU_TRUE)
            /* Check validity of requested bus. */
            if((*spi_bus)->is_used)
#endif
            {
                /* Get pointer to SPI device. */
                *spi_device = (*spi_bus)->spi_devices[device_idx];
                
#if (CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE == NU_TRUE)
                /* Check validity of requested SPI device. */
                if(*spi_device)
#endif
                {
                    status = NU_SUCCESS;
                }
            }
        }
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_SPI_Register
*
* DESCRIPTION
*
*       Application should call this function to register a SPI  
*       device with a SPI bus. Only after registering a device 
*       with a bus, application can perform I/O with this device.
*
* INPUTS
*
*       bus_name                            Name of the SPI bus.
*       baud_rate                           Operational baud rate of
*                                           device. Applicable only in slave
*                                           mode
*       transfer_size                       Bit transfer size of the spi device.
*       spi_config                          Bitwise OR of the following 
*                                           parameters:
*                                           * Definitions for slave select polarity. *
*                                              SPI_SS_POL_LO                           
*                                              SPI_SS_POL_HI                           
*                                              SPI_SS_POL_SW_CONTROL                    
*
*                                            * Definitions for bit order. *
*                                              SPI_BO_LSB_FIRST                        
*                                              SPI_BO_MSB_FIRST                        
*
*                                            * Defines for SPI Clock Polarity *
*                                              SPI_MODE_POL_LO                         
*                                              SPI_MODE_POL_HI                         
*
*                                            * Defines for SPI Clock Phase *
*                                              SPI_MODE_PHA_FIRST_EDGE                      
*                                              SPI_MODE_PHA_SECOND_EDGE                     
*
*                                            * Defines for SPI MasterSlave *
*                                              SPI_DEV_MASTER                          
*                                              SPI_DEV_SLAVE                           
*       handle                              An output argument containing 
*                                           handle to newly registered 
*                                           device when function returns.
*
* OUTPUTS
*
*       NU_SPI_INVLD_ARG                    Invalid configuration 
*       NU_SPI_BUS_SLOT_NOT_FOUND           SPI bus with specified name 
*                                           is not found.
*       NU_SPI_NO_FREE_DEVICE_SLOT          No free device slot is found.
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
STATUS NU_SPI_Register(CHAR           *bus_name, 
                       UINT32          baud_rate, 
                       UINT32          transfer_size, 
                       UINT32          spi_config,
                       NU_SPI_HANDLE   *handle)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    STATUS          status;
    
    /* Find bus index, identified byt 'bus_name'. */
    status = LWSPI_Find_Bus_Slot_By_Name(bus_name, &spi_bus);
    if(status == NU_SUCCESS)
    {
        /* Find a vacant device slot. */
        status = LWSPI_Find_Vacant_Device_Slot(spi_bus, &spi_device);
        if(status == NU_SUCCESS)
        {
            /* Validate SPI configuration */
            if ((spi_config & SPI_CFG_BO_ERROR)       == SPI_CFG_BO_ERROR)        return NU_SPI_INVLD_ARG;
            if ((spi_config & SPI_CFG_MODE_POL_ERROR) == SPI_CFG_MODE_POL_ERROR)  return NU_SPI_INVLD_ARG;
            if ((spi_config & SPI_CFG_MODE_PHA_ERROR) == SPI_CFG_MODE_PHA_ERROR)  return NU_SPI_INVLD_ARG;
            if ((spi_config & SPI_CFG_DEV_ERROR)      == SPI_CFG_DEV_ERROR)       return NU_SPI_INVLD_ARG;

            spi_device->baud_rate            = baud_rate;
            spi_device->transfer_size        = transfer_size;
            spi_device->handle               = NU_SPI_ENCODE_HANDLE(spi_bus->index,spi_device->index);
            spi_device->spi_config           = spi_config;
            spi_device->ss_index             = 0;
            spi_device->bus                  = spi_bus;

            /* Create asynchronous I/O lock for this device. */
            status = NU_Create_Semaphore(&spi_device->async_io_lock,
                                        "SPI_DL",
                                        1,
                                        NU_FIFO);
            if(status == NU_SUCCESS)
            {
#if (LWSPI_NUM_DEVICES == 1)
                /* If there is only one device in system then configure 
                 * hardware once.
                 */
                /* Configure hardware parameters. */
                spi_bus->io_ptrs.configure(spi_bus->dev_context,
                                            spi_device);
#endif

                /* Return device handle to the caller. */
                *handle = spi_device->handle;
            }
        }
    }
    
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_SPI_Unregister
*
* DESCRIPTION
*
*       Application may call this function to un-register a spi device.
*       Once un-registered no further I/O can be performed on this device.
*
* INPUTS
*
*       handle                              Handle to SPI device obtained 
*                                           during registeration.
*
* OUTPUTS
*
*       NU_SPI_INVLD_ARG                    Handle is invalid.
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS NU_SPI_Unregister(NU_SPI_HANDLE handle)
{
    UINT8           bus_idx, device_idx;
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    STATUS          status;
    
    /* Initialize status to an invalid value. */
    status = NU_SPI_INVLD_ARG;
    
    /* Check for valid handle. */
    if(NU_SPI_VALIDATE_HANDLE(handle))
    {
        NU_SPI_DECODE_HANDLE(handle, &bus_idx, &device_idx);
        if((bus_idx < LWSPI_NUM_BUSES) && (device_idx < LWSPI_NUM_DEVICES))
        {
            spi_bus = &SPI_Bus_CB[bus_idx];
            spi_device = spi_bus->spi_devices[device_idx];
            
            if(spi_device != NU_NULL)
            {
                /* Delete async I/O lock of this device. */
                status = NU_Delete_Semaphore(&spi_device->async_io_lock);
                
                /* Deallocate device. */
                status |= NU_Deallocate_Memory(spi_device);
                
                /* Mark device slot as empty. */
                spi_bus->spi_devices[device_idx] = NU_NULL;
            }
        }
    }
    
    return (status);

}

/*************************************************************************
* FUNCTION
*
*       NU_SPI_DMA_Setup
*
*       ahonkan Terabit Radios 
*       Aug 10 2014
*
* DESCRIPTION
*
*       Code initializes DMA device driver and prepares SPI for DMA operation.
*
* INPUTS
*
*       handle                              Handle of SPI device.
*
* OUTPUTS
*
*       NU_SUCCESS                          DMA engine successfully
*                                           initialized.
*       NU_SPI_INVALID_HANDLE               Invalid SPI device specified.
*
*************************************************************************/
#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE
STATUS NU_SPI_DMA_Setup(NU_SPI_HANDLE   handle)
{
    UINT8           bus_idx, device_idx;
    NU_SPI_BUS      *spi_bus;
    STATUS          status;
    
    /* Initialize status to an invalid value. */
    status = NU_SPI_INVLD_ARG;
    
    /* Check for valid handle. */
    if(NU_SPI_VALIDATE_HANDLE(handle))
    {
        NU_SPI_DECODE_HANDLE(handle, &bus_idx, &device_idx);
        if((bus_idx < LWSPI_NUM_BUSES) && (device_idx < LWSPI_NUM_DEVICES))
        {
            spi_bus = &SPI_Bus_CB[bus_idx];
      
            /* 
             * Using context info, open the dma channels
             * if specified.
             */
      
            LWSPI_INSTANCE_HANDLE   *inst_ptr = (LWSPI_INSTANCE_HANDLE*)spi_bus->dev_context;
      
            if (inst_ptr->dma_intf.dma_enable == 1)
            {
              status = NU_DMA_Open(inst_ptr->dma_intf.tx_dma_dev_id,  &(spi_bus->dma_tx_handle));
              if (status != NU_SUCCESS)
                return status;
                
              status = NU_DMA_Open(inst_ptr->dma_intf.rx_dma_dev_id,  &(spi_bus->dma_rx_handle));
              if (status != NU_SUCCESS)
                return status;
      
              status = NU_DMA_Acquire_Channel(spi_bus->dma_tx_handle, &(spi_bus->chan_tx_handle), 
                                              inst_ptr->dma_intf.tx_dma_dev_id,
                                              inst_ptr->dma_intf.tx_dma_peri_id,
                                              NULL);
              if (status != NU_SUCCESS)
                return status;
      
              status = NU_DMA_Acquire_Channel(spi_bus->dma_rx_handle, &(spi_bus->chan_rx_handle), 
                                              inst_ptr->dma_intf.rx_dma_dev_id,
                                              inst_ptr->dma_intf.rx_dma_peri_id,
                                              NULL);
              if (status != NU_SUCCESS)
                return status;
      
            }
          }

    }
    else
    {
      status = NU_SPI_INVALID_HANDLE;
    }

  return status;    

}
#endif  /* CFG_NU_OS_DRVR_DMA_ENABLE */


/*************************************************************************
* FUNCTION
*
*       NU_SPI_DMA_Transfer
*
*       ahonkan Terabit Radios 
*       Aug 10 2014
*
* DESCRIPTION
*
*       Start the SPI DMA data transfer and block thread for transfer 
*       completion.  
*
*       The DMA transfer routines need to be revisited so that the transfer
*       can start and an asynch test for completion.
*
* INPUTS
*
*       handle                              Handle of SPI device.
*       dma_tx_data_ptr                     pointer to data to be transmitted
*       dma_rx_data_ptr                     pointer to data to be received
*       data_len                            number of elements to transfer
*
* OUTPUTS
*
*       NU_SUCCESS                          DMA engine successfully
*                                           tranferred data
*
*************************************************************************/
#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE
STATUS NU_SPI_DMA_Transfer(NU_SPI_HANDLE handle,
                           VOID* dma_tx_data_ptr,
                           VOID* dma_rx_data_ptr,
                           UINT16 data_len)
{
    UINT8           bus_idx, device_idx;
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    STATUS          status;
    
    /* Initialize status to an invalid value. */
    status = NU_SPI_INVLD_ARG;

    /* Check for valid handle. */
    if(NU_SPI_VALIDATE_HANDLE(handle))
    {
        NU_SPI_DECODE_HANDLE(handle, &bus_idx, &device_idx);
        if((bus_idx < LWSPI_NUM_BUSES) && (device_idx < LWSPI_NUM_DEVICES))
        {
            spi_bus = &SPI_Bus_CB[bus_idx];
            spi_device = spi_bus->spi_devices[device_idx];
      
            /* 
             * Using context info, access the dma channels
             */
      
            if (spi_device->transfer_size == SPI_CFG_16Bit)
            	data_len >>= 1;

            /* Populate DMA Request structures with necessary data for DMA */
            spi_device->dma_tx_req.src_ptr = dma_tx_data_ptr;
            spi_device->dma_tx_req.dst_ptr = 0;       /* To be filled by target */
            spi_device->dma_tx_req.length = data_len;
            spi_device->dma_tx_req.src_add_type = DMA_ADDRESS_INCR;
            spi_device->dma_tx_req.dst_add_type = DMA_ADDRESS_FIXED;

            spi_device->dma_rx_req.src_ptr = 0;       /* To be filled by target */
            spi_device->dma_rx_req.dst_ptr = dma_rx_data_ptr;
            spi_device->dma_rx_req.length = data_len;
            spi_device->dma_rx_req.src_add_type = DMA_ADDRESS_FIXED;
            spi_device->dma_rx_req.dst_add_type = DMA_ADDRESS_INCR;


            /* Perform IOCTL call to enable the target device dma */
            status = DVC_Dev_Ioctl(spi_bus->dev_handle,
                                   (spi_bus->ioctl_base + LWSPI_IOCTL_PREP_DMA),
                                    spi_device,
                                    0);

            /* Initiate DMA transfers.  
             * Initiate the rx in non-block and then tx in block to allow
             * both master and slave op.
             */
            
            status = NU_DMA_Data_Transfer(spi_bus->chan_rx_handle,
                                          &(spi_device->dma_rx_req),
                                          1,
                                          NU_FALSE,
                                          DMA_ASYNC_RECEIVE,
                                          NU_SUSPEND);

            if (status != NU_SUCCESS)
              return status;                    

            status = NU_DMA_Data_Transfer(spi_bus->chan_tx_handle,
                                          &(spi_device->dma_tx_req),
                                          1,
                                          NU_FALSE,
                                          DMA_SYNC_SEND,
                                          NU_SUSPEND);

        }
    }
    else
      status = NU_SPI_INVALID_HANDLE;              


  return status;    

}
#endif  /* CFG_NU_OS_DRVR_DMA_ENABLE */



/*************************************************************************
* FUNCTION
*
*       NU_SPI_Read
*
*       ahonkan Terabit Radios 
*       Jul 16 2014
*
*       Modified code for SPI blocking/non-blocking access to allow calling
*       thread to continue with other operations.
*
*
* DESCRIPTION
*
*       Application calls this function to read data from specified SPI 
*       device. The behavior if this function varies depending upon the 
*       value of 'io_type' argument.
*
*           - If caller passes 'io_type' as SPI_POLLED_IO then driver will 
*             complete read operation in polling mode. This way all 
*             data will be read within the calling context.
*
*           - If caller passes 'io_type' as SPI_INTERRUPT_IO then driver
*             will carry interrupt driven read operation. This way all 
*             data will be read within the context of SPI master controller 
*             driver HISR.  The API will return to calling context to allow
*             further code execution while the current SPI operation is in 
*             progress.  Further operations on the interface will 
*             return NU_SPI_BUSY if the current SPI operation isn't completed.
*             The NU_SPI_Check_Complete can be called to either block
*             on completion or return completion status if the calling 
*             context needs to know the status of the operation. 
*
*       Note: If the device selected is a slave device and it's slave select
*       is configured as SPI_SS_POL_SW_CONTROL, then the calling thread needs 
*       to assert the Slave Select line before calling
*       this routine.  It's also up to the caller to de-assert the SS line
*       when the transaction is completed.  This could be by either blocking 
*       the thread or polling using the NU_SPI_Check_Complete function.
*
* INPUTS
*
*       handle                              Handle of SPI device.
*       io_type                             Specifies if polling or 
*                                           interrupt driven I/O should 
*                                           be used. value of this argument
*                                           should be one of these.
*                                            - SPI_POLLED_IO
*                                            - SPI_INTERRUPT_IO
*       buffer                              Pointer to buffer where data 
*                                           is to be read.
*       length                              length of data to be read.
*
* OUTPUTS
*
*       NU_SUCCESS                          Transfer initiated if in 
*                                           SPI_INTERRUPT_IO or service 
*                                           completed successfully if in
*                                           SPI_POLLED_IO.
*       NU_SPI_BUSY                         SPI device is currently in the 
*                                           middle of a transaction
*
*************************************************************************/
STATUS NU_SPI_Read( NU_SPI_HANDLE   handle, 
                    UNSIGNED        io_type, 
                    VOID            *buffer, 
                    UINT32          length)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    NU_SPI_IRP      *irp;
    STATUS          status;
    
    /* Get SPI bus and device parameters from handle. */
    status = LWSPI_Get_Params_From_Handle(handle, &spi_bus, &spi_device);
    if(status == NU_SUCCESS)
    {
        /* Check the status of the current spi transaction.  If the device
         * is locked, it means data is being transferred.  The semaphore
         * is locked when the spi transaction is initiated in SPI_INTERRUPT_IO
         * mode.  The semaphore is unlocked at the end of the io operation where
         * all the data in the buffer is sent.
         *
         * If the semaphore is unlocked then it means the transactions are over and 
         * this code can release the bus which it will claim again because a SPI
         * transaction is requested again.
         */  

        status = NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                     NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;
            
        if(status == NU_SUCCESS)
        {
            /* Release SPI bus. */
            NU_Release_Semaphore(&spi_bus->bus_lock);
            NU_Release_Semaphore(&spi_device->async_io_lock);
        }

        /* Claim access on bus. */
        status = NU_Obtain_Semaphore(&spi_bus->bus_lock, 
                                    NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;

        if(status == NU_SUCCESS)
        {
            irp = &(spi_device->rx_irp);
            
            irp->device         = spi_device;
            irp->buffer         = buffer;
            irp->length         = length;
            irp->actual_length  = 0;
            
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Check if device power is on. If device power is off then driver 
            * may block the call until device power is on.
            */
            if (spi_bus->io_ptrs.check_power_on != NU_NULL)
            {
                spi_bus->io_ptrs.check_power_on(spi_bus->dev_context);
            }
#endif
            
#if (LWSPI_NUM_DEVICES > 1)
            /* Re-configure hardware only if current I/O operation is 
             * targeted for a different device than the previous one. 
             */
            if(spi_bus->current_device != spi_device)
            {
                /* Configure hardware parameters. */
                spi_bus->io_ptrs.configure(spi_bus->dev_context,
                                            spi_device);

                /* Save pointer to SPI device for later use. */
                spi_bus->current_device = spi_device;
            }
#endif
        
            /* Call target driver read function. */
            status = spi_bus->io_ptrs.read(spi_bus->dev_context, 
                                    (io_type == SPI_POLLED_IO ? NU_TRUE : NU_FALSE), 
                                    irp);

            if ((status == NU_SUCCESS) && (io_type == SPI_INTERRUPT_IO))
            {
                NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                    NU_SUSPEND);
            }
            
            /* Release SPI bus. */
            /* Handle the relese in the NU_SPI_Check_Complete function. */

        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_SPI_Write
*
*       ahonkan Terabit Radios 
*       Jul 16 2014
*
*       Modified code for SPI blocking/non-blocking access to allow calling
*       thread to continue with other operations.
*
*
* DESCRIPTION
*
*       Application calls this function to write data from specified SPI 
*       device. The behavior if this function varies depending upon the 
*       value of 'io_type' argument.
*
*           - If caller passes 'io_type' as SPI_POLLED_IO then driver will 
*             complete write operation in polling mode. This way all 
*             data will be written within the calling context.
*
*           - If caller passes 'io_type' as SPI_INTERRUPT_IO then driver
*             will carry interrupt driven write operation. This way all 
*             data will be written within the context of SPI master controller 
*             driver HISR.  The API will return to calling context to allow
*             further code execution while the current SPI operation is in 
*             progress.  Further operations on the interface will 
*             return NU_SPI_BUSY if the current SPI operation isn't completed.
*             The NU_SPI_Check_Complete can be called to either block
*             on completion or return completion status if the calling 
*             context needs to know the status of the operation. 
*
*       Note: If the device selected is a slave device and it's slave select
*       is configured as SPI_SS_POL_SW_CONTROL, then the calling thread needs 
*       to assert the Slave Select line before calling
*       this routine.  It's also up to the caller to de-assert the SS line
*       when the transaction is completed.  This could be by either blocking 
*       the thread or polling using the NU_SPI_Check_Complete function.
*
* INPUTS
*
*       handle                              Handle of SPI device.
*       io_type                             Specifies if polling or 
*                                           interrupt driven I/O should 
*                                           be used. value of this argument
*                                           should be one of these.
*                                            - SPI_POLLED_IO
*                                            - SPI_INTERRUPT_IO
*       buffer                              Pointer to buffer where data 
*                                           is to be written.
*       length                              length of data to be written.
*
* OUTPUTS
*
*       NU_SUCCESS                          Transfer initiated if in 
*                                           SPI_INTERRUPT_IO or service 
*                                           completed successfully if in
*                                           SPI_POLLED_IO.
*       NU_SPI_BUSY                         SPI device is currently in the 
*                                           middle of a transaction
*
*************************************************************************/
STATUS NU_SPI_Write(NU_SPI_HANDLE   handle, 
                    UNSIGNED        io_type, 
                    VOID            *buffer, 
                    UINT32          length)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    NU_SPI_IRP      *irp;
    STATUS          status;
    
    /* Get SPI bus and device parameters from handle. */
    status = LWSPI_Get_Params_From_Handle(handle, &spi_bus, &spi_device);
    if(status == NU_SUCCESS)
    {
        /* Check the status of the current spi transaction.  If the device
         * is locked, it means data is being transferred.  The semaphore
         * is locked when the spi transaction is initiated in SPI_INTERRUPT_IO
         * mode.  The semaphore is unlocked at the end of the io operation where
         * all the data in the buffer is sent.
         *
         * If the semaphore is unlocked then it means the transactions are over and 
         * this code can release the bus which it will claim again because a SPI
         * transaction is requested again.
         */  

        status = NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                     NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;
            
        if(status == NU_SUCCESS)
        {
            /* Release SPI bus. */
            NU_Release_Semaphore(&spi_bus->bus_lock);
            NU_Release_Semaphore(&spi_device->async_io_lock);
        }

        /* Claim access on bus. */
        status = NU_Obtain_Semaphore(&spi_bus->bus_lock, 
                                    NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;

        if(status == NU_SUCCESS)
        {
            irp = &(spi_device->tx_irp);
            
            irp->device         = spi_device;
            irp->buffer         = buffer;
            irp->length         = length;
            irp->actual_length  = 0;
            
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Check if device power is on. If device power is off then driver 
            * may block the call until device power is on.
            */
            if (spi_bus->io_ptrs.check_power_on != NU_NULL)
            {
                spi_bus->io_ptrs.check_power_on(spi_bus->dev_context);
            }
#endif
            
#if (LWSPI_NUM_DEVICES > 1)
            /* Re-configure hardware only if current I/O operation is 
             * targeted for a different device than the previous one. 
             */
            if(spi_bus->current_device != spi_device)
            {
                /* Configure hardware parameters. */
                spi_bus->io_ptrs.configure(spi_bus->dev_context,
                                            spi_device);

                /* Save pointer to SPI device for later use. */
                spi_bus->current_device = spi_device;
            }
#endif

            /* Call target driver read function. */
            status = spi_bus->io_ptrs.write(spi_bus->dev_context, 
                                    (io_type == SPI_POLLED_IO ? NU_TRUE : NU_FALSE),
                                    irp);
            if ((status == NU_SUCCESS) && (io_type == SPI_INTERRUPT_IO))
            {
                NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                    NU_SUSPEND);
            }
            
            
            /* Release SPI bus. */
            /* Handle the relese in the NU_SPI_Check_Complete function. */

        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_SPI_Write_Read
*
*       ahonkan Terabit Radios 
*       Jul 16 2014
*
*       Modified code for SPI blocking/non-blocking access to allow calling
*       thread to continue with other operations.
*
* DESCRIPTION
*
*       Application calls this function to perform a write/read operation
*       on the spi bus. If the device connected to this processor is configured 
*       as a slave, then the write operation is initiated (on MOSI) and the 
*       data is read in on the MISO line.  If the device connected to this 
*       processor is configured as a master, the operation then is a read 
*       on MOSI with the MISO being the write operation.
*         
*       The behavior of this function varies depending upon the 
*       value of 'io_type' argument.
*
*           - If caller passes 'io_type' as SPI_POLLED_IO then driver will 
*             complete write/read operation in polling mode. This way all 
*             data will be written within the calling context.
*
*           - If caller passes 'io_type' as SPI_INTERRUPT_IO then driver
*             will carry interrupt driven write/read operation. This way all 
*             data will be written/read within the context of SPI master controller 
*             driver HISR.  The API will return to calling context to allow
*             further code execution while the current SPI operation is in 
*             progress.  Further operations on the interface will 
*             return NU_SPI_BUSY if the current SPI operation isn't completed.
*             The NU_SPI_Check_Complete can be called to either block
*             on completion or return completion status if the calling 
*             context needs to know the status of the operation. 
*
*       Note: If the device selected is a slave device and it's slave select
*       is configured as SPI_SS_POL_SW_CONTROL, then the calling thread needs 
*       to assert the Slave Select line before calling
*       this routine.  It's also up to the caller to de-assert the SS line
*       when the transaction is completed.  This could be by either blocking 
*       the thread or polling using the NU_SPI_Check_Complete function.
*
*       handle                              Handle of SPI device.
*       io_type                             Specifies if polling or 
*                                           interrupt driven I/O should 
*                                           be used. value of this argument
*                                           should be one of these.
*                                            - SPI_POLLED_IO
*                                            - SPI_INTERRUPT_IO
*       buffer_tx                           Pointer to buffer containing 
*                                           data to be written.
*       buffer_rx                           Pointer to buffer where data 
*                                           is to be read.
*       io_length                           Length of data to be written/read.
*
* OUTPUTS
*
*       NU_SUCCESS                          Transfer initiated if in 
*                                           SPI_INTERRUPT_IO or service 
*                                           completed successfully if in
*                                           SPI_POLLED_IO.
*       NU_SPI_BUSY                         SPI device is currently in the 
*                                           middle of a transaction
*
* 
*************************************************************************/
STATUS NU_SPI_Write_Read(NU_SPI_HANDLE  handle,
                         UNSIGNED       io_type,
                         VOID          *buffer_tx,
                         VOID          *buffer_rx,
                         UINT32         io_length)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    NU_SPI_IRP      *tx_irp, *rx_irp;
    STATUS          status;
    
    /* Get SPI bus and device parameters from handle. */
    status = LWSPI_Get_Params_From_Handle(handle, &spi_bus, &spi_device);
    if(status == NU_SUCCESS)
    {
        /* Check the status of the current spi transaction.  If the device
         * is locked, it means data is being transferred.  The semaphore
         * is locked when the spi transaction is initiated in SPI_INTERRUPT_IO
         * mode.  The semaphore is unlocked at the end of the io operation where
         * all the data in the buffer is sent.
         *
         * If the semaphore is unlocked then it means the transactions are over and 
         * this code can release the bus which it will claim again because a SPI
         * transaction is requested again.
         */  

        status = NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                     NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;
            
        if(status == NU_SUCCESS)
        {
            /* Release SPI bus. */
            NU_Release_Semaphore(&spi_bus->bus_lock);
            NU_Release_Semaphore(&spi_device->async_io_lock);
        }

        /* Claim access on bus. */
        status = NU_Obtain_Semaphore(&spi_bus->bus_lock, 
                                    NU_NO_SUSPEND);

        if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;

        if(status == NU_SUCCESS)
        {
            tx_irp = &(spi_device->tx_irp);
        
            /* Construct Tx IRP. */
            tx_irp->device         = spi_device;
            tx_irp->buffer         = buffer_tx;
            tx_irp->length         = io_length;
            tx_irp->actual_length  = 0;
            
            rx_irp = &(spi_device->rx_irp);
            
            /* Construct Rx IRP. */
            rx_irp->device         = spi_device;
            rx_irp->buffer         = buffer_rx;
            rx_irp->length         = io_length;
            rx_irp->actual_length  = 0;
            
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Check if device power is on. If device power is off then driver 
            * may block the call until device power is on.
            */
            if (spi_bus->io_ptrs.check_power_on != NU_NULL)
            {
                spi_bus->io_ptrs.check_power_on(spi_bus->dev_context);
            }
#endif
        
#if (LWSPI_NUM_DEVICES > 1)
            /* Re-configure hardware only if current I/O operation is 
             * targeted for a different device than the previous one. 
             */
            if(spi_bus->current_device != spi_device)
            {
                /* Configure hardware parameters. */
                spi_bus->io_ptrs.configure(spi_bus->dev_context,
                                            spi_device);

                /* Save pointer to SPI device for later use. */
                spi_bus->current_device = spi_device;
            }
#endif

            /* Call target driver read function. */
            status = spi_bus->io_ptrs.write_read(spi_bus->dev_context, 
                                        (io_type == SPI_POLLED_IO ? NU_TRUE : NU_FALSE),
                                        tx_irp, 
                                        rx_irp);
            if ((status == NU_SUCCESS) && (io_type == SPI_INTERRUPT_IO))
            {
                NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                    NU_SUSPEND);
            }
            
            /* Release SPI bus. */
            NU_Release_Semaphore(&spi_bus->bus_lock);
        }
    }

    return (status);
}



/*************************************************************************
* FUNCTION
*
*       NU_SPI_Check_Complete
*
*       ahonkan Terabit Radios 
*       Jul 16 2014
*
*       Modified code for SPI blocking/non-blocking access to allow calling
*       thread to continue with other operations.
*
* DESCRIPTION
*
*       Application calls this function to monitor the status of the semaphore
*       that protects the spi transaction.  The routine is called as part of 
*       a polling or monitoring routine where the calling thread could be doing
*       some other activity while waiting for the spi to complete.
*
*       This routine should be called at least once after the transaction is over
*       to 
*
*       handle                              Handle of SPI device.
*       sem_param                           Semaphore parameter from the
*                                           kernel documentation.  Can be
*                                           NU_NO_SUSPEND
*                                           NU_SUSPEND
*                                           or tick count from 1 - 0xFFFFFFFD
*
* OUTPUTS
*
*       NU_SUCCESS                          Transfer initiated if in 
*                                           SPI_INTERRUPT_IO or service 
*                                           completed successfully if in
*                                           SPI_POLLED_IO.
*       NU_SPI_BUSY                         SPI device is currently in the 
*                                           middle of a transaction
*       NU_SPI_TIMEOUT                      SPI device check timed out
*       NU_SPI_SEM_ERROR                    Some semaphore error, debug with
*                                           debugger.
* 
*************************************************************************/
STATUS NU_SPI_Check_Complete(NU_SPI_HANDLE handle, UINT32 sem_param)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_device;
    STATUS          status;
    
    /* Get SPI bus and device parameters from handle. */
    status = LWSPI_Get_Params_From_Handle(handle, &spi_bus, &spi_device);
    if(status == NU_SUCCESS)
    {
        /* Check the status of the current spi transaction.  If the device
         * is locked, it means data is being transferred.  The semaphore
         * is locked when the spi transaction is initiated in SPI_INTERRUPT_IO
         * mode.  The semaphore is unlocked at the end of the io operation where
         * all the data in the buffer is sent.
         *
         * If the semaphore is unlocked then it means the transactions are over and 
         * this code can release the bus which it will claim again because a SPI
         * transaction is requested again.
         */  

        status = NU_Obtain_Semaphore(&spi_device->async_io_lock, 
                                     sem_param);

        if(status == NU_SUCCESS)
        {
            /* Release SPI bus. */
            status = NU_Release_Semaphore(&spi_bus->bus_lock);
            NU_Release_Semaphore(&spi_device->async_io_lock);

            return NU_SUCCESS;
        }
        else if (status == NU_UNAVAILABLE) return NU_SPI_BUSY;
        else if (status == NU_TIMEOUT) return NU_SPI_TIMEOUT;
        else return NU_SPI_SEM_ERROR;   

    }

    return status;
}



#if (CFG_NU_OS_CONN_LWSPI_EXTENDED_API_ENABLE == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*       NU_SPI_Set_Slave_Select_Index
*
* DESCRIPTION
*
*       Application should call this function if it needs to set slave 
*       select index for a particular slave. This is required in case 
*       when there are multiple slave selects in hardware.
*
* INPUTS
*
*       handle                              Handle of SPI device.
*       ss_index                            Slave select index.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed 
*                                           successfully.
*
*************************************************************************/
STATUS NU_SPI_Set_Slave_Select_Index(NU_SPI_HANDLE handle, UINT8 ss_index)
{
    NU_SPI_BUS      *spi_bus;
    NU_SPI_DEVICE   *spi_slave;
    STATUS          status;
    
    /* Get SPI bus and slave parameters from handle. */
    status = LWSPI_Get_Params_From_Handle(handle, &spi_bus, &spi_slave);
    if(status == NU_SUCCESS)
    {
        /* Claim exclusive access on bus. */
        status = NU_Obtain_Semaphore(&spi_bus->bus_lock, 
                                    NU_SUSPEND);
        if(status == NU_SUCCESS)
        {
            /* Save assigned slave select index. */
            spi_slave->ss_index = ss_index;
    
#if (LWSPI_NUM_SLAVES == 1)
            /* If there is only one slave in system then re-configure 
             * hardware here.
             */
            /* Configure hardware parameters. */
            spi_bus->io_ptrs.configure(spi_bus->dev_context,
                                        spi_slave);
#else
            /* Invalidate current slave so that bus is reconfigured at 
             * next I/O operation. 
             */
            spi_bus->current_slave = NU_NULL;
#endif
            /* Release SPI bus. */
            NU_Release_Semaphore(&spi_bus->bus_lock);
        }
    }
    
    return (status);
}
#endif /* (CFG_NU_OS_CONN_LWSPI_EXTENDED_API_ENABLE == NU_TRUE) */
