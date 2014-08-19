/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
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
*       nu_usbh_com_ecm_ext.c
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains the implementation for API functions provided by
*       Nucleus USB host communication class driver's ECM model.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_COM_Set_ETH_Mul_Filter      Class specific request
*                                           processing.
*       NU_USBH_COM_Set_ETH_Power_Filter    Class specific request
*                                           processing.
*       NU_USBH_COM_Get_ETH_Power_Filter    Class specific request
*                                           processing.
*       NU_USBH_COM_Set_ETH_Packet_Filter   Class specific request
*                                           processing.
*       NU_USBH_COM_Get_ETH_Static          Class specific request
*                                           processing.
*       NU_USBH_COM_Check_ECM_Func_Desc     Internal Implementation
*                                           function.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* USB Include Files */
#include "connectivity/nu_usb.h"

#ifdef INC_ECM_MDL

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_ETH_Mul_Filter
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets ethernet multiple filters.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     p_data_buf         Pointer to buffer to send multiple filters.
*     data_length        Byte length of buffer to send multiple filters.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by
*                                 Communication class driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_ETH_Mul_Filter(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       VOID*               p_data_buf,
       UINT32              data_length)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Copy contents to data IO buffer. */
        memcpy(temp_buffer, p_data_buf, data_length);

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

          /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0x21,
                                 UH_SET_ETH_MULTICAST_FILTERS,
                                 data_length/6,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    return ((status == NU_SUCCESS) ? irp_status : status);

}/* NU_USBH_COM_Set_ETH_Mul_Filter */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_ETH_Power_Filter
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets ethernet power filters.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     p_data_buf         Pointer to buffer to send power filters.
*     data_length        Byte length of buffer to send multiple power.
*     filter_num         Filter number which is to set.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by
*                                 Communication class driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_ETH_Power_Filter(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       VOID*               p_data_buf,
       UINT32              data_length,
       UINT16              filter_num)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Copy contents to data IO buffer. */
        memcpy(temp_buffer, p_data_buf, data_length);

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

          /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0x21,
                                 UH_SET_ETH_POWER_PATTERN_FILTER,
                                 filter_num,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    return ((status == NU_SUCCESS) ? irp_status : status);

}/* NU_USBH_COM_Set_ETH_Power_Filter */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Get_ETH_Power_Filter
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which get ethernet power filters.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     p_data_buf         Pointer to buffer to hold power filters.
*     data_length        Byte length of buffer to hold multiple power.
*     filter_num         Filter number which is to get.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by
*                                 Communication class driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Get_ETH_Power_Filter(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       VOID*               p_data_buf,
       UINT32              data_length,
       UINT16              filter_num)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Zero out freshly allocated buffer. */
        memset(temp_buffer, 0, data_length);

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

          /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0xA1,
                                 UH_GET_ETH_POWER_PATTERN_FILTER,
                                 filter_num,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Copy contents back to input buffer. */
        memcpy(p_data_buf, temp_buffer, data_length);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Get_ETH_Power_Filter */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_ETH_Packet_Filter
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets ethernet multiple filters.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     filter_bmp         bit pattern representing the bitmap.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication class driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_ETH_Packet_Filter (
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT16              filter_bmp)
{
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Getting the interface number. */
    NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                              &intf_num);

    /* Get and clear contents of current device control IRP block. */
    cb_ctrl_irp = pcb_curr_device->ctrl_irp;
    memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

    /* Form the control request. */
    NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                             NU_NULL,
                             NU_USBH_COM_Ctrl_IRP_Complete,
                             pcb_curr_device,
                             0x21,
                             UH_SET_ETH_PACKET_FILTER,
                             filter_bmp,
                             HOST_2_LE16 (intf_num),
                             HOST_2_LE16 (0x00));

    /* Submits the IRP. */
    NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                            (NU_USB_IRP *)cb_ctrl_irp);

    /* Wait for the the IRP to be completed. */
    NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                        UHC_CTRL_SENT,
                        NU_AND_CONSUME,
                        &ret_events,
                        NU_SUSPEND);

    /* ...and returns status. */
    NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                           &irp_status);

    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);
    return (irp_status);
} /* NU_USBH_COM_Set_ETH_Packet_Filter */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Get_ETH_Static
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets ethernet multiple filters.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     feature_selector   Feature selector number to get the details of.
*     feature            4 Bytes to hold feature details.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication class driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Get_ETH_Static (
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT16              feature_selector,
       UINT32*             feature)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 sizeof(UINT32),
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Zero out freshly allocated buffer. */
        memset(temp_buffer, 0, sizeof(UINT32));

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

        /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0xA1,
                                 UH_GET_ETH_STATISTIC,
                                 feature_selector,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (0x04));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Copy contents back to input buffer. */
        memcpy(feature, temp_buffer, sizeof(UINT32));

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    return ((status == NU_SUCCESS) ? irp_status : status);

}/* NU_USBH_COM_Get_ETH_Static */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Check_ECM_Func_Desc
*
* DESCRIPTION
*     This function gets the required functional descriptors for ECM class.
*     Then other information to form the device specific information
*     structure for user driver.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     class_desc         Pointer to start of class specific descriptors.
*     temp_length        Total length of class specific descriptors.
*
* OUTPUTS
*     None
*
**************************************************************************/

VOID NU_USBH_COM_Check_ECM_Func_Desc(
     NU_USBH_COM_DEVICE* pcb_curr_device,
     UINT8*              class_desc,
     UINT32              temp_length)
{
    UINT16  MAC_address[0x20] = {0};
    UINT8   PAC_address[12]   = {0};
    UINT8*  func_desc;
    UINT32  var_32 = 0x00;
    UINT8   i;
    UINT8*  ptr_8;
    NU_USBH_COM_ECM_INFORM *pcb_inform;

    /* Allocating a structure to hold ECM specific information. */

    USB_Allocate_Object(sizeof(NU_USBH_COM_ECM_INFORM),
                        (VOID**)&(pcb_curr_device->model_spc_inform));

    if(pcb_curr_device->model_spc_inform == NU_NULL)
    {
        return;
    }

    memset(pcb_curr_device->model_spc_inform,
           0x00,
           sizeof(NU_USBH_COM_ECM_INFORM));

    pcb_inform =
    (NU_USBH_COM_ECM_INFORM*)(pcb_curr_device->model_spc_inform);

    /* Determining the string index from class specific descriptors. */

    func_desc = NU_USBH_COM_Parse_Strings(class_desc,
                                          temp_length,
                                          UH_ETH_NET_FD);
    if (func_desc)
        /* Fetching the string descriptor. */
        NU_USBH_COM_Get_String(pcb_curr_device,
                           (UINT8*)MAC_address,
                           0x40,
                           *func_desc);

    /* Calculating the MAC address from unicode string. */
    for(i=1; i<13;i++)
    {
        PAC_address[i-1] = (UINT8)LE16_2_HOST(MAC_address[i]);
    }

    for(i=0; i<12;i++)
    {
        PAC_address[i] -= 0x30;
        if((PAC_address[i] >= 0x11)&& (PAC_address[i] < 0x17))
        {
           PAC_address[i] -= 0x07;
        }
        else if((PAC_address[i] >= 0x21)&& (PAC_address[i] < 0x27))
        {
           PAC_address[i] -= 0x17;
        }
    }

    ptr_8 = pcb_inform->MAC_addr;
    for(i=0; i<6;i++)
    {
        PAC_address[2*i] = (PAC_address[2*i]<<4);
        *(ptr_8++)  = (UINT8)(PAC_address[2*i] + PAC_address[(2*i)+1]);
    }

    /* Calculating the ethernet stats. */
    func_desc++;
    var_32 = (var_32 | (UINT32)*func_desc);
    func_desc++;
    var_32 = (var_32 | (UINT32)(*func_desc<<8));
    func_desc++;
    var_32 = (var_32 | (UINT32)(*func_desc<<16));
    func_desc++;
    var_32 = (var_32 | (UINT32)(*func_desc<<24));

    pcb_inform->stats         = var_32;

    /* Calculating Maximum segment size. */
    var_32 = 0x00;
    func_desc++;
    var_32 = (var_32 | (UINT32)*func_desc);
    func_desc++;
    var_32 = (var_32 | (UINT32)(*func_desc<<8));
    pcb_inform->segment_size  = var_32;

    /* Calculating the multicast filters */
    var_32 = 0x00;
    func_desc++;
    var_32 = (var_32 | (UINT32)*func_desc);
    func_desc++;
    var_32 = (var_32 | (UINT32)(*func_desc<<8));
    pcb_inform->MC_filters    = var_32;

    func_desc++;
    pcb_inform->power_filters = *(func_desc);
} /* NU_USBH_COM_Check_ECM_Func_Desc */

#endif /* INC_ECM_MDL */

