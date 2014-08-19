/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
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
*       ethernet.c
*
*   COMPONENT
*
*       ETH - Ethernet Class Driver
*
*   DESCRIPTION
*
*       This file contains the Ethernet Class Driver specific functions.
*
*   FUNCTIONS
*
*       ETHERNET_Open
*       ETHERNET_Initialize
*       ETHERNET_Device_Open
*       ETHERNET_Transmit
*       ETHERNET_Ioctl
*       ETHERNET_Plus_Init
*       ETHERNET_Create_Name
*
*   DEPENDENCIES
*
*       string.h
*       nucleus.h
*       externs.h
*       ethernet.h
*       reg_api.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <string.h>
#include "nucleus.h"
#include "networking/externs.h"
#include "drivers/ethernet.h"
#include "services/reg_api.h"
#include "services/nu_trace_os_mark.h"

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern NU_MEMORY_POOL       System_Memory;

/*********************************/
/* EXTERNAL FUNCTION DEFINITIONS */
/*********************************/

/*********************************/
/* FUNCTION DEFINITIONS          */
/*********************************/
static STATUS        ETHERNET_Device_Open (UINT8 *ether_addr, DV_DEVICE_ENTRY *device);
static STATUS        ETHERNET_Transmit (DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr);
static STATUS        ETHERNET_Ioctl(DV_DEVICE_ENTRY *, INT, DV_REQ *);
static STATUS        ETHERNET_Plus_Init (DV_DEVICE_ENTRY *device);

/*******************************/
/* LOCAL VARIABLE DECLARATIONS */
/*******************************/


/*************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Open
*
*   DESCRIPTION
*
*       This function opens the ethernet device
*
*   INPUTS
*
*       CHAR           *regpath             - Registry path
*       DV_DEV_ID      eth_dev_id           - Ethernet device id
*       UINT32         eth_flags            - Ethernet flags
*
*   OUTPUTS
*
*       DV_DEV_HANDLE  comps_sess_hd        - Component session handle
*
*************************************************************************/
DV_DEV_HANDLE ETHERNET_Open(CHAR *regpath, DV_DEV_ID eth_dev_id, UINT32 eth_flags)
{
    STATUS               status;
    INT                  int_level;
    DV_DEV_HANDLE        comp_sess_hd;
    DV_DEV_LABEL         eth_label = {ETHERNET_LABEL};
    DV_IOCTL0_STRUCT     dev_ioctl0;
    NU_DEVICE            lan_mw;
    CHAR                 *config_options_path;
    ETHERNET_CONFIG_PATH config_path;

    /* Initialize config path */
    config_path.config_path[0] = 0;
    config_path.max_path_len   = REG_MAX_KEY_LENGTH;

    /* Disable interrupts */
    int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* Open the component driver */
    status     = DVC_Dev_ID_Open(eth_dev_id, &eth_label, 1, &comp_sess_hd);

    /* If valid */
    if (status == NU_SUCCESS)
    {
        /* Get IOCTL base address */
        dev_ioctl0.label = eth_label;
        status           = DVC_Dev_Ioctl(comp_sess_hd, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

        if (status == NU_SUCCESS)
        {
            /* Set the device handle */
            status = DVC_Dev_Ioctl(comp_sess_hd, 
                                   dev_ioctl0.base+ETHERNET_CMD_SET_DEV_HANDLE,
                                   (VOID*)&comp_sess_hd, 
                                   sizeof(DV_DEV_HANDLE));
        }

        if (status == NU_SUCCESS)
        {
            /* Get the device's configuration path . */
            status = DVC_Dev_Ioctl(comp_sess_hd, 
                                   dev_ioctl0.base+ETHERNET_CMD_GET_CONFIG_PATH,
                                   (VOID*)&config_path, 
                                   sizeof(config_path));
        }    


        /* If we get a config path back, then use it,
           if we dont, use the default */
        if (status == NU_SUCCESS)
        {
            if (strlen(config_path.config_path) > 0)
            {
                /* Get the path to the devices platform configuration. */
                config_options_path = config_path.config_path;
            }
            else
            {
                /* For now use default configuration from Net's .metadata */
                config_options_path = regpath;
            }
        }


        if (status == NU_SUCCESS)
        {
            /* Get filled NU_DEVICE structure from component */
            status = DVC_Dev_Ioctl(comp_sess_hd, 
                                   dev_ioctl0.base+ETHERNET_CMD_GET_DEV_STRUCT,
                                   (VOID*)&lan_mw, 
                                   sizeof(NU_DEVICE));

            /***********************************************************************/
            /* Set up local cache for the interface configuration list entry here. */
            /***********************************************************************/
            if (status == NU_SUCCESS)
            {
                status = NU_Ifconfig_Set_Interface_Defaults(lan_mw.dv_name, config_options_path);
            }


            if (status == NU_SUCCESS)
            {
                if (NU_Registry_Get_IPv6_Enabled(config_options_path) == NU_TRUE)
                {
                    /* Set device flags to show utilizing IPv6 */
                    eth_flags |= (DV6_IPV6 | DV6_PRIMARY_INT);
                }

                /* Set flags */
                lan_mw.dv_flags = eth_flags;

                /* Initialize this device */
                status = NU_Init_Devices(&lan_mw, 1);

            }

        }
    }

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts (int_level);

    if (status != NU_SUCCESS)
    {
        comp_sess_hd = (DV_DEV_HANDLE)status;
    }

    return (comp_sess_hd);
}


/**************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Initialize
*
*   DESCRIPTION
*
*       This function initializes the device driver and calls the
*       device open function.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the structure of the
*                                             device being initialized.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if initialization is
*                                             successful. Otherwise an error code is
*                                             returned.
*
**************************************************************************/
STATUS ETHERNET_Initialize (DV_DEVICE_ENTRY *device)
{
    STATUS           status;
    DV_IOCTL0_STRUCT dev_ioctl0;
    DV_DEV_LABEL     eth_label = {ETHERNET_LABEL};
    DV_DEV_HANDLE    dev_handle = (DV_DEV_HANDLE)device->dev_handle;

    /* Get IOCTL base address */
    dev_ioctl0.label = eth_label;
    status           = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

    if (status == NU_SUCCESS)
    {

        /* Initialize various function pointers for ethernet operations. */
        device->dev_open    = &ETHERNET_Device_Open;
        device->dev_start   = &ETHERNET_Transmit;
        device->dev_output  = &NET_Ether_Send;
        device->dev_input   = &NET_Ether_Input;
        device->dev_ioctl   = &ETHERNET_Ioctl;
        device->dev_type    = DVT_ETHER;
        device->dev_vect    = device->dev_irq;

        /* Place ETHERNET constants into device control block. */
        device->dev_addrlen = ETHERNET_MAC_ADDR_SIZE;
        device->dev_hdrlen  = ETHERNET_HDR_SIZE;

        /* MTU size */
        device->dev_mtu     = ETHERNET_MTU;

        /* Clear device NET_BUFFER queue */
        device->dev_transq.head   = NU_NULL;
        device->dev_transq_length = 0;

        /* A simplex controller. Broadcasts are allowed. */
        device->dev_flags |= (DV_SIMPLEX | DV_BROADCAST | DV_MULTICAST);

        /* Attach the controller's XDATA to the device structure */
        status = DVC_Dev_Ioctl(dev_handle, 
                               dev_ioctl0.base+ETHERNET_CMD_GET_XDATA, 
                               (VOID*)device, 
                               sizeof(DV_DEVICE_ENTRY));
        if (status == NU_SUCCESS)
        {
            /* Open the device. */
            status = (*(device->dev_open)) (device->dev_mac_addr, device);
        }
    }

    return (status);

}   /* ETHERNET_Initialize */


/**************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Device_Open
*
*   DESCRIPTION
*
*       This function completes driver initialization operations.
*
*   INPUTS
*
*       UINT8           *ether_addr         - Pointer to a buffer where the MAC
*                                             address will be stored.
*       DV_DEVICE_ENTRY *device             - Pointer to the device being opened.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
static STATUS ETHERNET_Device_Open (UINT8 *ether_addr, DV_DEVICE_ENTRY *device)
{
    STATUS           status;
    DV_IOCTL0_STRUCT dev_ioctl0;
    DV_DEV_LABEL     eth_label = {ETHERNET_LABEL};
    DV_DEV_HANDLE    dev_handle = (DV_DEV_HANDLE)device->dev_handle;

    /* Get IOCTL base address */
    dev_ioctl0.label = eth_label;
    status           = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

    if (status == NU_SUCCESS)
    {
        /* Initialize PLUS. */
        status = ETHERNET_Plus_Init (device);

        /* Ensure PLUS specific initialization successful. */
        if (status == NU_SUCCESS)
        {
            /* Initialize the Target. */
            status = DVC_Dev_Ioctl(dev_handle, 
                                   dev_ioctl0.base+ETHERNET_CMD_TARGET_INIT, 
                                   (VOID*)device, 
                                   sizeof(DV_DEVICE_ENTRY));
        }

        /* Initialize the PHY. */
        if (status == NU_SUCCESS)
        {
            status = DVC_Dev_Ioctl(dev_handle, 
                                   dev_ioctl0.base+ETHERNET_CMD_PHY_INIT, 
                                   (VOID*)device, 
                                   sizeof(DV_DEVICE_ENTRY));
        }

        /* Initialize the ETHERNET controller. */
        if (status == NU_SUCCESS)
        {
            status = DVC_Dev_Ioctl(dev_handle, 
                                   dev_ioctl0.base+ETHERNET_CMD_CTRL_INIT, 
                                   (VOID*)device, 
                                   sizeof(DV_DEVICE_ENTRY));
        }
                              
        /* Enable the controller */      
        if (status == NU_SUCCESS)
        {
            status = DVC_Dev_Ioctl(dev_handle,
                                   dev_ioctl0.base+ETHERNET_CMD_CTRL_ENABLE, 
                                   (VOID*)device, 
                                   sizeof(DV_DEVICE_ENTRY));
        }
    }

    /* Power IOCTLS may not be available yet for the  driver under development, so for now,
       convert NU_UNAVAILABLE to NU_SUCCESS */
    if (status == NU_UNAVAILABLE)
    {
        status = NU_SUCCESS;
    }

    /* Return status to caller */
    return (status);

}   /* ETHERNET_Device_Open */



/**************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Transmit
*
*   DESCRIPTION
*
*       This function places a packet onto the wire.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device               - Pointer to the device
*       NET_BUFFER      *buf_ptr              - Pointer to packet being sent.
*
*   OUTPUTS
*
*       STATUS          status                - NU_SUCCESS
*
**************************************************************************/
static STATUS ETHERNET_Transmit (DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr)
{
    STATUS         status;
    UINT32         bytes_written;
    DV_DEV_HANDLE  dev_handle = (DV_DEV_HANDLE)device->dev_handle;
    
    /* Trace log */
    T_DEV_TX_LAT_START(device->dev_net_if_name);
    
    /* Transmit the NET buffer */
    status = DVC_Dev_Write (dev_handle, (VOID*)buf_ptr, buf_ptr->mem_total_data_len, 0, &bytes_written);

    /* Trace log */
    T_DEV_TX_LAT_STOP(device->dev_net_if_name, buf_ptr->mem_total_data_len, bytes_written, status);
    
    return (status);

}   /* ETHERNET_Transmit */


/*************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Ioctl
*
*   DESCRIPTION
*
*       This function uses ioctl command to communicate with the ethernet 
*       hardware
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*       INT             option              - Option to be used.
*       DV_REQ          *d_req              - Pointer to the request.
*
*   OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*
*************************************************************************/
static STATUS  ETHERNET_Ioctl (DV_DEVICE_ENTRY *device, INT option, DV_REQ *d_req)
{
    STATUS           status;
    DV_IOCTL0_STRUCT dev_ioctl0;
    DV_DEV_LABEL     net_label = {NETWORKING_LABEL};
    DV_DEV_HANDLE    dev_handle = (DV_DEV_HANDLE)device->dev_handle;

    /* Get IOCTL base address */
    dev_ioctl0.label = net_label;
    status           = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

    if (status == NU_SUCCESS)
    {
        switch (option)
        {
            case DEV_ADDMULTI:
                /* Join the ethernet multicast group. */
                status = NET_Add_Multi (device, d_req);

                /* A status of NU_RESET means the operation was a success and
                 * as a result the ethernet chip must be reset.*/
                if (status == NU_RESET)
                {
                    /* Reset chip so Multicast Hash table can be updated. */
                    status = DVC_Dev_Ioctl(dev_handle, 
                                           dev_ioctl0.base+ETHERNET_CMD_DEV_ADDMULTI, 
                                           (VOID*)device, 
                                           sizeof(DV_DEVICE_ENTRY));
                }
                break;

            case DEV_DELMULTI:
                /* Leave the ethernet multicast group. */
                status = NET_Del_Multi (device, d_req);

                /* A status of NU_RESET means the operation was a success and
                 * as a result the ethernet chip must be reset.*/
                if (status == NU_RESET)
                {
                    /* Reset chip so Multicast Hash table can be updated. */
                    status = DVC_Dev_Ioctl(dev_handle, 
                                           dev_ioctl0.base+ETHERNET_CMD_DEV_DELMULTI, 
                                           (VOID*)device, 
                                           sizeof(DV_DEVICE_ENTRY));
                }
                break;

            /* Enable Hardware Offloading Capabilities */
            case DEV_HW_OFFLOAD_CTRL:

                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_OFFLOAD_CTRL, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Report Hardware offloading capabilities */
            case DEV_GET_HW_OFLD_CAP:

                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_OFFLOAD_CAP, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Uninitialize the Ethernet driver */
            case DEV_REMDEV:
 
                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_REMDEV, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Set VLAN tag */
            case DEV_SET_VLAN_TAG:

                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_SET_VLAN_TAG, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Set VLAN RX mode */
            case DEV_SET_VLAN_RX_MODE:

                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_SET_VLAN_RX_MODE, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Get VLAN tag */
            case DEV_GET_VLAN_TAG:

                status = DVC_Dev_Ioctl(dev_handle, 
                                       dev_ioctl0.base+ETHERNET_CMD_GET_VLAN_TAG, 
                                       (VOID*)device, 
                                       sizeof(DV_DEVICE_ENTRY));
                break;

            /* Set Physical Address of the Hardware. */
            case DEV_SET_HW_ADDR:

                status = DVC_Dev_Ioctl(dev_handle,
                                       dev_ioctl0.base + ETHERNET_CMD_SET_HW_ADDR,
                                       (VOID*) d_req, sizeof(DV_REQ));
                break;

            /* Gets the Physical Address of the Hardware.*/
            case DEV_GET_HW_ADDR:

                status = DVC_Dev_Ioctl(dev_handle,
                                       dev_ioctl0.base + ETHERNET_CMD_GET_HW_ADDR,
                                       (VOID*) d_req, sizeof(DV_REQ));
                break;

            default:
                /* Return error if option is unrecognized. */
                status = NU_INVAL;
                break;
        }
    }

    return (status);
} 

/**************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Plus_Init
*
*   DESCRIPTION
*
*       This function registers all LISRs with the kernel and allocates
*       memory for all HISRs if available.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device 
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
static STATUS ETHERNET_Plus_Init (DV_DEVICE_ENTRY *device)
{
    STATUS            status = NU_SUCCESS;
    VOID              *pointer = NU_NULL;
    VOID              (*ETHERNET_old_vect_routine) (INT);
    DV_IOCTL0_STRUCT  dev_ioctl0;
    DV_DEV_LABEL      eth_label = {ETHERNET_LABEL};
    DV_DEV_HANDLE     dev_handle = (DV_DEV_HANDLE)device->dev_handle;
    ETHERNET_ISR_INFO *isr_info;

    /* Get IOCTL base address */
    dev_ioctl0.label = eth_label;
    status           = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for ISR_INFO. */
        status = NU_Allocate_Memory (&System_Memory, (VOID*)&isr_info, sizeof (ETHERNET_ISR_INFO), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Clear data */
            (VOID)memset (isr_info, 0, sizeof (ETHERNET_ISR_INFO));

            /* Get ISR info */
            status = DVC_Dev_Ioctl(dev_handle, 
                                   dev_ioctl0.base+ETHERNET_CMD_GET_ISR_INFO, 
                                   (VOID*)isr_info, 
                                   sizeof(ETHERNET_ISR_INFO));
        }
    }

    /* Register the TX LISR. */
    if ((status == NU_SUCCESS) && (isr_info->tx_lisr_func != NU_NULL))
    {
        status = NU_Register_LISR (isr_info->tx_irq, 
                                   isr_info->tx_lisr_func,
                                   &ETHERNET_old_vect_routine);

        /* Set the data for this vector to point to device structure */
        ESAL_GE_ISR_VECTOR_DATA_SET(isr_info->tx_irq, (VOID *) device);
    }

    /* Register the RX LISR. */
    if ((status == NU_SUCCESS) && (isr_info->rx_lisr_func != NU_NULL))
    {
        status = NU_Register_LISR (isr_info->rx_irq, 
                                   isr_info->rx_lisr_func,
                                   &ETHERNET_old_vect_routine);

        /* Set the data for this vector to point to device structure */
        ESAL_GE_ISR_VECTOR_DATA_SET(isr_info->rx_irq, (VOID *) device);
    }

    /* Register the ER LISR. */
    if ((status == NU_SUCCESS) && (isr_info->er_lisr_func != NU_NULL))
    {
        status = NU_Register_LISR (isr_info->er_irq, 
                                   isr_info->er_lisr_func,
                                   &ETHERNET_old_vect_routine);

        /* Set the data for this vector to point to device structure */
        ESAL_GE_ISR_VECTOR_DATA_SET(isr_info->er_irq, (VOID *) device);
    }

    /* Register the PHY LISR. */
    if ((status == NU_SUCCESS) && (isr_info->phy_lisr_func != NU_NULL))
    {
        status = NU_Register_LISR (isr_info->phy_irq, 
                                   isr_info->phy_lisr_func,
                                   &ETHERNET_old_vect_routine);

        /* Set the data for this vector to point to device structure */
        ESAL_GE_ISR_VECTOR_DATA_SET(isr_info->phy_irq, (VOID *) device);
    }


    /* Allocate HISR stack space if we support any of the HISRs */
    if ((status == NU_SUCCESS) && 
        ((isr_info->rx_hisr_func != NU_NULL) || (isr_info->tx_hisr_func != NU_NULL) ||
         (isr_info->er_hisr_func != NU_NULL) || (isr_info->phy_hisr_func != NU_NULL)))
    {
        /* Allocate memory for RX_HISR, TX_HISR, ER_HISR and PHY HISR. */
        status = NU_Allocate_Memory (&System_Memory, &pointer, ETHERNET_HISR_MEM_SIZE,
                                         NU_NO_SUSPEND);

        /* Check memory allocation status. */
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to allocate memory for RX HISR's",
                             NERR_FATAL, __FILE__, __LINE__);
        }
        else
        {
            /* Normalize the pointer. */
            pointer = TLS_Normalize_Ptr (pointer);
        }
    }

    /* Create RX HISR. */
    if ((status == NU_SUCCESS) && (isr_info->rx_hisr_func != NU_NULL))
    {
        /* Create HISR for the receive interrupt. */
        status = NU_Create_HISR (isr_info->rx_hisr_cb, "RXHISR",
                                 isr_info->rx_hisr_func, ETHERNET_HISR_PRIORITY,
                                 pointer, ETHERNET_HISR_MEM_SIZE);

        /* Check HISR creation return status. */
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to create RX HISR", NERR_FATAL,
                              __FILE__, __LINE__);
        }

        /* Place a pointer to the device in the HISR control block in order for the
           RX HISR to find the ethernet device that caused the TX interrupt. */
        isr_info->rx_hisr_cb->tc_app_reserved_1 = (UNSIGNED)device;
    }

    /* Create TX HISR. */
    if ((status == NU_SUCCESS) && (isr_info->tx_hisr_func != NU_NULL))
    {
        
        /* Create HISR for the transmit interrupt. */
        status = NU_Create_HISR (isr_info->tx_hisr_cb, "TXHISR",
                                 isr_info->tx_hisr_func, ETHERNET_HISR_PRIORITY, 
                                 pointer, ETHERNET_HISR_MEM_SIZE);

        /* Check HISR creation return status. */
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to create TX HISR",
                             NERR_FATAL, __FILE__, __LINE__);
        }

        /* Place a pointer to the device in the HISR control block in order for the
           TX HISR to find the ethernet device that caused the TX interrupt. */
        isr_info->tx_hisr_cb->tc_app_reserved_1 = (UNSIGNED)device;
    } 

    /* Create ER HISR. */
    if ((status == NU_SUCCESS) && (isr_info->er_hisr_func != NU_NULL))
    {
        /* Create HISR for the transmit interrupt. */
        status = NU_Create_HISR (isr_info->er_hisr_cb, "ERHISR",
                                 isr_info->er_hisr_func, ETHERNET_HISR_PRIORITY, 
                                 pointer, ETHERNET_HISR_MEM_SIZE);

        /* Check HISR creation return status. */
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to create ER HISR",
                              NERR_FATAL, __FILE__, __LINE__);
        }

        /* Place a pointer to the device in the HISR control block in order for the
           ER HISR to find the ethernet device that caused the TX interrupt. */
        isr_info->er_hisr_cb->tc_app_reserved_1 = (UNSIGNED)device;
    }

    /* Create PHY HISR. */
    if ((status == NU_SUCCESS) && (isr_info->phy_hisr_func != NU_NULL))
    {

        /* Create HISR for the transmit interrupt. */
        status = NU_Create_HISR (isr_info->phy_hisr_cb, "PHYHISR",
                                 isr_info->phy_hisr_func, ETHERNET_HISR_PRIORITY, 
                                 pointer, ETHERNET_HISR_MEM_SIZE);

        /* Check HISR creation return status. */
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to create PHY HISR",
                              NERR_FATAL, __FILE__, __LINE__);
        }

        /* Place a pointer to the device in the HISR control block in order for the
           ER HISR to find the ethernet device that caused the TX interrupt. */
        isr_info->phy_hisr_cb->tc_app_reserved_1 = (UNSIGNED)device;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       ETHERNET_Create_Name
*
*   DESCRIPTION
*
*       This function creates a unique name for each ethernet device
*       that calls in. The name is appended with a string number starting 
*       at 0. 
*
*   INPUTS
*
*       CHAR           *name                - Modifiable name
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID  ETHERNET_Create_Name (CHAR *name)
{
    static INT eth_number = 0;
    INT        length;

    /* Create the name only if the array is non null */
    if (name != NU_NULL)
    {
        /* First copy the common part */
        strcpy (name, "net");

        length = strlen(name);

        /* Append the string number */
        name[length] = (CHAR)(0x30 + eth_number++);
        name[length + 1] = (CHAR)'\0';
    }

}


