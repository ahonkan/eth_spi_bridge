/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*     ethernet_tgt.c
*
* COMPONENT
*
*     STM32_EMAC Ethernet Device Driver.
*
* DESCRIPTION
*
*     This file contains the source of the STM32_EMAC Ethernet driver.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*
*     nu_bsp_drvr_enet_stm32_emac_init
*     Ethernet_Tgt_Read
*     Ethernet_Tgt_Write
*     Ethernet_Tgt_Target_Initialize
*     Ethernet_Tgt_Controller_Init
*     Ethernet_Tgt_Get_Address
*     Ethernet_Tgt_Set_Address
*     Ethernet_Tgt_LISR
*     Ethernet_Tgt_RX_HISR
*     Ethernet_Tgt_Extended_Data
*     Ethernet_Tgt_Xmit_Packet
*     Ethernet_Tgt_TX_HISR
*     Ethernet_Tgt_Receive_Packet
*     Ethernet_Tgt_MII_Read
*     Ethernet_Tgt_MII_Write
*     Ethernet_Tgt_Configure
*     Ethernet_Tgt_Enable
*     Ethernet_Tgt_Disable
*
* DEPENDENCIES
*
*     nucleus.h
*     nu_kernel.h
*     nu_services.h
*     nu_networking.h
*     ethernet_tgt.h
*     phy.h
*
*************************************************************************/

/**********************************/
/* INCLUDE FILES                  */
/**********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "networking/externs.h"
#include "networking/nlog.h"
#include "networking/mii.h"
#include "bsp/drivers/ethernet/stm32_emac/ethernet_tgt.h"
#include "bsp/drivers/ethernet/stm32_emac/ethernet_tgt_power.h"
#include "bsp/drivers/ethernet/stm32_emac/phy.h"


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern VOID Ethernet_Tgt_Pwr_Default_State (VOID *inst_handle);
extern STATUS Ethernet_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);
#endif

/*********************************/
/* GLOBAL VARIABLES              */
/*********************************/

static STM32EMAC_TXDESC*        TXDescP;
static STM32EMAC_RXDESC*        RXDescP;
static UINT32                   TXBufDesIdx = 0;
static UINT32                   RXBufDesIdx = 0;

/***********************************/
/* LOCAL FUNCTION PROTOTYPES       */
/***********************************/
static STATUS    Ethernet_Tgt_Receive_Packet (DV_DEVICE_ENTRY *device);

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_enet_stm32_emac_init (const CHAR * key, INT startstop)
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the component
*       and calls the component driver registration function.
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       Ethernet_Tgt_Dv_Register
*       Ethernet_Tgt_Dv_Unregister
*
*   INPUTS
*
*       CHAR     *key                       - Key
*       INT      startstop                  - Start or Stop flag
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_bsp_drvr_enet_stm32_emac_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID            stm32_emac_dev_id = DV_INVALID_DEV;
    STATUS                      status = NU_SUCCESS;
    NU_MEMORY_POOL           *  sys_pool_ptr;
    ETHERNET_INSTANCE_HANDLE *  inst_handle;
    CHAR                        reg_path[REG_MAX_KEY_LENGTH];
    STATUS                      reg_stat = REG_TYPE_ERROR;
    PHY_CTRL                 *  phy_ctrl;
    VOID                        (*setup_func)(VOID);

    if (startstop)
    {
        /********************************/
        /* GET A UNUSED INSTANCE HANDLE */
        /********************************/

        /* Allocate memory for the STM32_ETH_INSTANCE_HANDLE structure */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
        
        if (status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&inst_handle, sizeof (ETHERNET_INSTANCE_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (inst_handle, 0, sizeof (ETHERNET_INSTANCE_HANDLE));

            /* Save the config path in the instance handle */
            strncpy(inst_handle->config_path, key, sizeof(inst_handle->config_path));

            /******************************/
            /* SAVE DEFAULT CFG/TGT INFO  */
            /******************************/

            /* Get target info */
            status = Ethernet_Get_Target_Info(key, inst_handle);

            if (status == NU_SUCCESS)
            {
                /* PHY info */
                /* Allocate memory for the PHY CTRL structure */
                status = NU_Allocate_Memory (sys_pool_ptr, (VOID*)&phy_ctrl, sizeof (PHY_CTRL), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Zero out allocated space */
                    (VOID)memset (phy_ctrl, 0, sizeof (PHY_CTRL));

                    /* Populate the info */
                    phy_ctrl->dev_io_addr  = inst_handle->io_addr;
                    phy_ctrl->phy_dev      = 0; /* Not used.  Grepped the source tree with no results */
                    phy_ctrl->phy_io_addr  = inst_handle->io_addr;
                    phy_ctrl->mii_read     = &Ethernet_Tgt_MII_Read;
                    phy_ctrl->mii_write    = &Ethernet_Tgt_MII_Write;

                    /* Check if PHY polled link is set (phy_irq = -1) */
                    if (inst_handle->phy_irq == -1)
                    {
                        /* Yes, set flag */
                        phy_ctrl->phy_polled_link = 1;
                    }

                    /* Save pointer to the tgt_info */
                    inst_handle->phy_ctrl = phy_ctrl;
                }
            }

            /******************************/
            /* CALL DEVICE SETUP FUNCTION */
            /******************************/

            /* If there is a setup function, call it */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            strcat(reg_path, "/setup");

            if (REG_Has_Key(reg_path))
            {
                reg_stat = REG_Get_UINT32 (reg_path, (UINT32*)&setup_func);

                if (reg_stat == NU_SUCCESS)
                {
                    setup_func();
                }
            }

            /*****************************************************/
            /* Assign the target hardware manipulation functions */
            /*****************************************************/

            inst_handle->tgt_fn.Tgt_Read                 = &Ethernet_Tgt_Read;
            inst_handle->tgt_fn.Tgt_Write                = &Ethernet_Tgt_Write;
            inst_handle->tgt_fn.Tgt_Enable               = &Ethernet_Tgt_Enable;
            inst_handle->tgt_fn.Tgt_Disable              = &Ethernet_Tgt_Disable;
            inst_handle->tgt_fn.Tgt_Create_Extended_Data = &Ethernet_Tgt_Create_Extended_Data;
            inst_handle->tgt_fn.Tgt_Target_Initialize    = &Ethernet_Tgt_Target_Initialize;
            inst_handle->tgt_fn.Tgt_Controller_Init      = &Ethernet_Tgt_Controller_Init;
            inst_handle->tgt_fn.Tgt_Get_ISR_Info         = &Ethernet_Tgt_Get_ISR_Info;
            inst_handle->tgt_fn.Tgt_Set_Phy_Dev_ID       = &Ethernet_Tgt_Set_Phy_Dev_ID;
            inst_handle->tgt_fn.Tgt_Get_Link_Status      = &Ethernet_Tgt_Get_Link_Status;
            inst_handle->tgt_fn.Tgt_Update_Multicast     = &Ethernet_Tgt_Update_Multicast;
            inst_handle->tgt_fn.Tgt_Phy_Initialize       = &Ethernet_Tgt_Phy_Initialize;
            inst_handle->tgt_fn.Tgt_Notify_Status_Change = &Ethernet_Tgt_Notify_Status_Change;

            inst_handle->tgt_fn.Tgt_Get_Address = &Ethernet_Tgt_Get_Address;
            inst_handle->tgt_fn.Tgt_Set_Address = &Ethernet_Tgt_Set_Address;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            inst_handle->tgt_fn.Tgt_Pwr_Default_State  = &Ethernet_Tgt_Pwr_Default_State;
            inst_handle->tgt_fn.Tgt_Pwr_Set_State      = &Ethernet_Tgt_Pwr_Set_State;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

            inst_handle->tgt_fn.Tgt_Pwr_Hibernate_Restore = &Ethernet_Tgt_Pwr_Hibernate_Restore;

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif

            /* Call the STM32_EMAC component registration function */
            (VOID)Ethernet_Dv_Register(key, inst_handle);
        }
    }

    else
    {
        /* If we are stopping an already started device */
        if (stm32_emac_dev_id != DV_INVALID_DEV)
        {
            /* Call the component unregistration function */
            (VOID)Ethernet_Dv_Unregister (stm32_emac_dev_id);
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Read
*
*   DESCRIPTION
*
*       This function is to read data from the ethernet hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       UINT32       numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_read            - Number of bytes read
*
*   OUTPUTS
*
*       INT          status                 - NU_SUCCESS, or number of bytes
*                                             written if different than numbyte
*
*************************************************************************/
STATUS Ethernet_Tgt_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read)
{
    *bytes_read = numbyte;

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Write
*
*   DESCRIPTION
*
*       This function is to write dat on ethernet hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       UINT32       numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_written         - Number of bytes written
*
*   OUTPUTS
*
*       INT          status                 - NU_SUCCESS, or number of bytes
*                                             written if different than numbyte
*
*************************************************************************/
STATUS Ethernet_Tgt_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte,
                              OFFSET_T byte_offset, UINT32 *bytes_written)
{
    STATUS                   status = NU_SUCCESS;
    UINT32                   bytes_w = 0;
    NET_BUFFER               *buf_ptr = (NET_BUFFER *)buffer;
    ETHERNET_SESSION_HANDLE  *stm32_emac_handle = (ETHERNET_SESSION_HANDLE *)session_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_WAIT_CYCLE(stm32_emac_handle->inst_info->pmi_dev, status);
#endif

    /* Device state is now ON, so we can transmit */
    if (status == NU_SUCCESS)
    {
        /* Transmit the packet */
        bytes_w = Ethernet_Tgt_Xmit_Packet (stm32_emac_handle->device, buf_ptr);

        *bytes_written = bytes_w;
    }

    return (status);
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Get_Address
*
*   DESCRIPTION
*
*       This function returns the MAC address of the specified
*       ethernet device.
*
*       WARNING: The MAC address is hard coded within this
*       function and must be verified to make sure there are
*       no address conflicts on the network.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*       UINT8           *ether_addr         - Pointer to a buffer where the MAC address
*                                             will be stored.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service
*                                             is successful. Otherwise an
*                                             error code is returned.
*
**************************************************************************/
STATUS  Ethernet_Tgt_Get_Address (DV_DEVICE_ENTRY *device, UINT8 *ether_addr)
{
    /* Set ethernet MAC address. */
    ether_addr[0] = 0x00;
    ether_addr[1] = 0x11;

    /* Set last 4 bytes of MAC address to current hardware timer count */
    ether_addr[2] = 0x2d;//NU_HW_TIMER_COUNT_READ() & 0xFF;
    ether_addr[3] = 0x3c;//(NU_HW_TIMER_COUNT_READ() >> 8) & 0xFF;
    ether_addr[4] = 0x01;//(NU_HW_TIMER_COUNT_READ() >> 16) & 0xFF;
    ether_addr[5] = 0x4a;//(NU_HW_TIMER_COUNT_READ() >> 24) & 0xFF;

    return (NU_SUCCESS); /* This has to be replaced by an actual return. */
} /* Ethernet_Tgt_Get_Address */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Set_Address
*
*   DESCRIPTION
*
*       This function sets the MAC address of the specified ethernet
*       device.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device being opened.
*       UINT8           *ether_addr         - Pointer to a buffer where the MAC address is
*                                             stored.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service
*                                             is successful. Otherwise an
*                                             error code is returned.
*
**************************************************************************/
STATUS  Ethernet_Tgt_Set_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr)
{
    UINT32          addr_h;
    UINT32          addr_l;


    /* Convert the MAC address into register specific format. */
    addr_l = (ether_addr[3] << 24) | (ether_addr[2] << 16) | (ether_addr[1] << 8)  | (ether_addr[0] );
    addr_h = (ether_addr[5] << 8)  | (ether_addr[4] );

    /* Set register EMAC specific address registers with the specified EMAC address. */
    STM32_EMAC_OUT32 (inst_handle->io_addr, ETH_MACA0HR, addr_h);
    STM32_EMAC_OUT32 (inst_handle->io_addr, ETH_MACA0LR, addr_l);    
    
    return (NU_SUCCESS); /* This has to be replaced by an actual return. */
}   /* Ethernet_Tgt_Set_Address */

/*************************************************************************
*
*   FUNCTION
*
*     Ethernet_Tgt_Receive_Packet
*
*   DESCRIPTION
*
*       This function processes a new inbound frame in the RX DATA FIFO.
*
*   INPUTS
*
*      *device                    Device control block pointer.
*
*   OUTPUTS
*
*      None.
*
**************************************************************************/
STATUS Ethernet_Tgt_Receive_Packet(DV_DEVICE_ENTRY *device)
{
    INT                 notify_net = NU_FALSE;
    NET_BUFFER*         headP;
    NET_BUFFER*         prevP;
    NET_BUFFER*         currP;
    UINT32              trackingIdx, bufCount, pktSize;
    STM32_EMAC_XDATA*   xdata = (STM32_EMAC_XDATA *)device->user_defined_1;
    UINT32              tmp32;

    trackingIdx = bufCount = pktSize = 0;

    /* If the current RX descriptor is not owned by the host return error */
    if ((RXDescP[RXBufDesIdx].rdes0 & STM32_EMAC_RX_DESC_OWN_BIT) != 0 )
        return notify_net;

    /* Loop through all RX descriptors owned by host and process them */
    while((RXDescP[RXBufDesIdx].rdes0 & STM32_EMAC_RX_DESC_OWN_BIT) == 0)
    {
        /* Ensure this is first frame */
        if ((RXDescP[RXBufDesIdx].rdes0 & STM32_EMAC_RX_DESC_FS_BIT) != 0)
        {
            trackingIdx = RXBufDesIdx;

            /***************************************************************/
            /* Construct a complete packet in the form of NET buffer chain */
            /***************************************************************/

            /* Initialize variables */
            headP = prevP = currP = NULL;

            /* Build net buffer linked list */
            do
            {
                currP = (NET_BUFFER*)(RXDescP[trackingIdx].rdes2);

                if (prevP)
                    prevP->next_buffer = currP;

                if ((RXDescP[trackingIdx].rdes0 & STM32_EMAC_RX_DESC_LS_BIT) != 0)
                {
                    currP->next_buffer = NULL;
                    headP = (NET_BUFFER*)(RXDescP[RXBufDesIdx].rdes2);
                    break;
                }

                trackingIdx = (trackingIdx + 1) % NUM_RX_DESC;

                if ((RXDescP[trackingIdx].rdes0 & STM32_EMAC_RX_DESC_FS_BIT) != 0)
                {
                    /* Handle unexpected first frames */
                    break;
                }

                prevP = currP;

            } while((RXDescP[trackingIdx].rdes0 & STM32_EMAC_RX_DESC_OWN_BIT) == 0);

            /************************************************************/
            /* Update the net buffer chain and feed it to the NET stack */
            /************************************************************/

            if (headP)
            {
                /* Get total frame length.  Exclude 4 bytes of CRC */
                pktSize = headP->mem_total_data_len = (((RXDescP[trackingIdx].rdes0 & STM32_EMAC_RX_DESC_FL_MSK) >> 16) - 4);

                if (trackingIdx >= RXBufDesIdx)
                {
                    bufCount = (trackingIdx - RXBufDesIdx) + 1;
                }
                else
                {
                    bufCount = trackingIdx + (NUM_RX_DESC - RXBufDesIdx) + 1;
                }

                if ((MAX_BUFFERS - MEM_Buffers_Used) >= bufCount)
                {
                    currP = headP;

                    /* Traverse the NET buffer linked list */
                    while(bufCount--)
                    {
                        /**************************************************/
                        /* Update relevant elements in current NET buffer */
                        /**************************************************/

                        currP->mem_buf_device = device;
                        currP->data_ptr = currP->mem_parent_packet;

                        if (pktSize < STM32_EMAC_RX_BUF_SIZE)
                        {
                            currP->data_len = pktSize;
                        }
                        else
                        {
                            currP->data_len = STM32_EMAC_RX_BUF_SIZE;
                        }

                        pktSize -= currP->data_len;

                        currP = currP->next_buffer;

                        /***************************************/
                        /* Return current RX descriptor to DMA */
                        /***************************************/

                        /* Replace RX descriptor with new net buffer. */
                        RXDescP[RXBufDesIdx].rdes2 = (UINT32)MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

                        /* Return descriptor to DMA control. */
                        RXDescP[RXBufDesIdx].rdes0 |= STM32_EMAC_RX_DESC_OWN_BIT;

                        /* Move to next descriptor. */
                        RXBufDesIdx = (RXBufDesIdx + 1) % NUM_RX_DESC;
                    }

                    /****************************************************************/
                    /* Push the received packet to NET stack for further processing */
                    /****************************************************************/

                    /* Put head of the chain onto NET stack. */
                    MEM_Buffer_Enqueue(&MEM_Buffer_List, headP);

                    /* Set flag so NET stack gets notified of buffers being received. */
                    notify_net = NU_TRUE;

                    /* Increment count of valid packet received. */
                    xdata->emac_rx_count++;

                }
                else
                {
                    /************************************************************************/
                    /* We are out of buffers we cannot accept any more packets at this time */
                    /************************************************************************/

                    /* Record error. */
                    xdata->emac_rx_error++;

                    /* Traverse the NET buffer linked list */
                    while(bufCount--)
                    {
                        /***************************************/
                        /* Return current RX descriptor to DMA */
                        /***************************************/

                        /* Return descriptor to DMA control. */
                        RXDescP[RXBufDesIdx].rdes0 |= STM32_EMAC_RX_DESC_OWN_BIT;

                        /* Move to next descriptor. */
                        RXBufDesIdx = ((RXBufDesIdx + 1) % NUM_RX_DESC);
                    }

                    break;
                }
            }
            else
            {
                /* For the case where SW does not control the entire frame,
                   exit and wait for future interrupt. */
                if (prevP == currP)
                    break;

                /* Return 1st SOF to hw control, main loop will find next SOF. */

                /* Record error. */
                xdata->emac_rx_error++;

                /* Return descriptor to DMA control. */
                RXDescP[RXBufDesIdx].rdes0 |= STM32_EMAC_RX_DESC_OWN_BIT;

                /* Move to next descriptor. */
                RXBufDesIdx = ((RXBufDesIdx + 1) % NUM_RX_DESC);
            }
        }
        else
        {
            /* Record error. */
            xdata->emac_rx_error++;

            /* Return descriptor to DMA control. */
            RXDescP[RXBufDesIdx].rdes0 |= STM32_EMAC_RX_DESC_OWN_BIT;

            /* Move to next descriptor. */
            RXBufDesIdx = ((RXBufDesIdx + 1) % NUM_RX_DESC);
        }

    }

    /* When RX Buffer unavailable flag is set: clear it and resume reception */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_DMASR);

    /* Check for RX buffer unavailable status */
    if ((tmp32 & ETH_DMASR_RBUS) != 0)
    {
        /* Clear RBUS ETHERNET DMA flag */
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMASR, ETH_DMASR_RBUS);

        /* Resume DMA transmission */
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMARPDR, 0);
    }

    /* Return if NET needs notification of received buffers */
    return (notify_net);
}


/***********************************************************************
* FUNCTION
*
*   Ethernet_Tgt_Xmit_Packet
*
* DESCRIPTION
*
*   TX Packet function for LM3S_ETH Ethernet driver. This function accepts
*   the chained NET buffer frame to be transmitted and inserts the data
*   into FIFO by traversing buffer by buffer in the Net Buffer chain.
*
* INPUTS
*
*   device  - Pointer to software DEVICE structure.
*   buf_ptr - Chained NET Buffer pointer.
*
* OUTPUTS
*
* STATUS       Returns NU_SUCCESS if service
*              is successful .Otherwise an
*              error code is returned.
*
***********************************************************************/
STATUS Ethernet_Tgt_Xmit_Packet(DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr)
{
    INT               bytes_copied = 0;
    UINT32            firstFrmIdx, lastFrmIdx;
    STM32_EMAC_XDATA  *xdata = (STM32_EMAC_XDATA *)device->user_defined_1;

    /* If pointer to NET_BUFFER is NULL or current TX descriptor is unavailable, bail */
    if ((buf_ptr == NULL) || ((TXDescP[TXBufDesIdx].tdes0 & STM32_EMAC_TX_DESC_OWN_BIT) != 0))
        return 0;

    /* This is first frame for this pay load set first segment bit */
    TXDescP[TXBufDesIdx].tdes0 |= STM32_EMAC_TX_DESC_FS_BIT;

    /* Set first frame index to current available index */
    firstFrmIdx = TXBufDesIdx;

    /* Loop through and transmit all frames for the pay load */
    do
    {

        /* Update TX descriptor with buffer address, size and mark it owned by DMA */
        TXDescP[TXBufDesIdx].tdes2 = (UINT32)buf_ptr->data_ptr;
        TXDescP[TXBufDesIdx].tdes1 = buf_ptr->data_len;

        /* Clear previously set LS and IC bits for this index */
        TXDescP[TXBufDesIdx].tdes0 &= ~(STM32_EMAC_TX_DESC_LS_BIT | STM32_EMAC_TX_DESC_IC_BIT);

        /* Intermediate descriptors */
        if (firstFrmIdx != TXBufDesIdx)
        {
            /* Clear previously set FS bit for the intermediate descriptors */
            TXDescP[TXBufDesIdx].tdes0 &= ~(STM32_EMAC_TX_DESC_FS_BIT);

            /* Set OWN bit */
            TXDescP[TXBufDesIdx].tdes0 |= STM32_EMAC_TX_DESC_OWN_BIT;

        }

        /* Add net buffer size to number of bytes copied. */
        bytes_copied += buf_ptr->data_len;

        /* Move to next buffer in the list. */
        buf_ptr = buf_ptr->next_buffer;

        /* Save last descriptor index before incrementing */
        lastFrmIdx = TXBufDesIdx;

        /* Increment TX Descriptor */
        TXBufDesIdx = ((TXBufDesIdx + 1) % NUM_TX_DESC);

    } while(buf_ptr && buf_ptr->data_len != 0);

    /* Set last segment and interrupt on completion to last descriptor */
    TXDescP[lastFrmIdx].tdes0 |= (STM32_EMAC_TX_DESC_LS_BIT | STM32_EMAC_TX_DESC_IC_BIT);

    /* Set OWN bit for the first frame */
    TXDescP[firstFrmIdx].tdes0 |= STM32_EMAC_TX_DESC_OWN_BIT;

    /* Resume TX DMA engine */
    if ((STM32_EMAC_IN32 (device->dev_io_addr, ETH_DMASR) & ETH_DMASR_TBUS) != 0)
    {
        /* Clear TBUS Ethernet DMA flag */
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMASR, ETH_DMASR_TBUS);

        /* Resume DMA transmission */
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMATPDR, 0);

    }

    /* DMA should have updated errors if any, check them */
    if ((TXDescP[firstFrmIdx].tdes0 & STM32_EMAC_TX_DESC_ES_BIT) != 0)
    {
        xdata->emac_tx_underrun += TXDescP[firstFrmIdx].tdes0 & STM32_EMAC_TX_DESC_UF_BIT;

        /* Collision count is valid only if Excessive Collision bit is not set */
        if ((TXDescP[firstFrmIdx].tdes0 & STM32_EMAC_TX_DESC_EC_BIT) == 0)
            xdata->emac_tx_collision += TXDescP[firstFrmIdx].tdes0 & STM32_EMAC_TX_DESC_CC_MSK;

    }

    return (bytes_copied);

}    /* End Ethernet_Tgt_Xmit_Packet. */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_MII_Read
*
*   DESCRIPTION
*
*       This is a utility function used to read a half-word from
*       the specified register of the specified PHY on the MII bus.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*       INT             phy_addr            - Address of PHY on MII bus.
*       INT             reg_addr            - Address of PHY register to read.
*       UINT15          *data               - On return, contains the data read.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS Ethernet_Tgt_MII_Read (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 *data)
{
    STATUS      status;
    INT         old_level;

    /* Disable interrupts. */
    old_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* Call the STM32_EMAC driver PHY read function. */
    status = PHY_Read (device->dev_io_addr, (UINT16) phy_addr, (UINT16) reg_addr, (UINT16*) data);

    /* Enable interrupts. */
    NU_Local_Control_Interrupts (old_level);

    return (status);

}   /* Ethernet_Tgt_MII_Read */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_MII_Write
*
*   DESCRIPTION
*
*       This is a utility function used to write a half-word to
*       the specified register of the specified PHY on the MII bus.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*       INT             phy_addr            - Address of PHY on MII bus.
*       INT             reg_addr            - Address of PHY register to read.
*       UINT16          data                - Data to be written to the MII.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS Ethernet_Tgt_MII_Write (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 data)
{
    STATUS      status;
    INT         old_level;

    /* Disable interrupts. */
    old_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* Call the STM32_EMAC driver PHY write function. */
    status = PHY_Write (device->dev_io_addr, (UINT16) phy_addr, (UINT16) reg_addr, (UINT16) data);

    /* Enable interrupts. */
    NU_Local_Control_Interrupts (old_level);

    return (status);

}   /* Ethernet_Tgt_MII_Write */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_RX_HISR
*
*   DESCRIPTION
*
*       This function is responsible for informing the NET that received
*       packets are ready for processing.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID Ethernet_Tgt_RX_HISR (VOID)
{
    DV_DEVICE_ENTRY *device;
    NU_HISR         *hcb;
    UINT32          dmaier;

    /* Get the device associated with this HISR from the HISR's control block */

    /* Get the HISR control block */
    hcb = NU_Current_HISR_Pointer();

    /* Get the device pointer from the HCB */
    device = (DV_DEVICE_ENTRY*)hcb->tc_app_reserved_1;

    /* Receive packets and see if NET notification is necessary */
    if (Ethernet_Tgt_Receive_Packet (device) == NU_TRUE)
    {
        /* Set NET notification event to show at least 1 frame was successfully received */
        NU_Set_Events (&Buffers_Available, (UNSIGNED)2, NU_OR);
    }

    /* Re-enable receive interrupts */
    dmaier = STM32_EMAC_IN32 (device->dev_io_addr, ETH_DMAIER);
    dmaier |= ETH_DMAIER_RIE; 
    STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMAIER, dmaier);

}   /* Ethernet_Tgt_RX_HISR */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_TX_HISR
*
*   DESCRIPTION
*
*       This HISR is activated when a TX LISR event occurs.  Both normal
*       and error conditions are processed by this HISR. The NET STACK
*       transqueue is checked for any new outbound packets and restarts
*       the TX Packet function again if needed.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID Ethernet_Tgt_TX_HISR (VOID)
{
    DV_DEVICE_ENTRY *device; 
    INT             old_level;
    NU_HISR         *hcb;

    /* Get the device associated with this HISR from the HISR's control block */

    /* Get the HISR control block */
    hcb = NU_Current_HISR_Pointer();

    /* Get the device pointer from the HCB */
    device = (DV_DEVICE_ENTRY*)hcb->tc_app_reserved_1;

    /* Check the transqueue for any new outbound packets */
    /* Disable interrupts. */
    old_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    if (device->dev_transq.head)
    {
        /* Give the buffers held by this packet back to the stack. */
        DEV_Recover_TX_Buffers (device);

        /* If there is another item on the list, transmit it. */
        if (device->dev_transq.head)
        {
            /* Restore interrupts. */
            NU_Local_Control_Interrupts (old_level);

            /* Transmit the next packet. */
            (VOID)Ethernet_Tgt_Xmit_Packet (device, device->dev_transq.head);
        }
        else
        {
            /* Restore interrupts. */
            NU_Local_Control_Interrupts(old_level);
        }
    }
    else
    {
        /* Restore interrupts. */
        NU_Local_Control_Interrupts(old_level);
    }
}   /* Ethernet_Tgt_TX_HISR */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_LISR
*
*   DESCRIPTION
*
*       This function is the Low Level Interrupt Routine for the LM3S_ETH driver.
*
*   INPUTS
*
*       INT vector                          - The LM3S_ETH interrupt vector.
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID Ethernet_Tgt_LISR (INT vector)
{
    UINT32              tmp32_sr;
    UINT32              tmp32_ier;
    DV_DEVICE_ENTRY     *device = ESAL_GE_ISR_VECTOR_DATA_GET (vector);
    STM32_EMAC_XDATA    *xdata = (STM32_EMAC_XDATA *)device->user_defined_1;


    /* Read EMAC interrupt status register (ISR) */
    tmp32_sr = STM32_EMAC_IN32 (device->dev_io_addr, ETH_DMASR);

    /* Test for errors */
    if ((tmp32_sr & ETH_DMASR_FBES) != 0)
    {
        /* Test for TX errors */
        if ((tmp32_sr & ETH_DMASR_EBS_TXDMA) != 0)
        {
            xdata->emac_tx_data_xfer_error++;
            xdata->emac_tx_read_xfer_error += (((tmp32_sr & ETH_DMASR_EBS_RD) == 1) ? 1:0);
            xdata->emac_tx_write_xfer_error += (((tmp32_sr & ETH_DMASR_EBS_RD) == 0) ? 1:0);
            xdata->emac_tx_desc_access_error += (((tmp32_sr & ETH_DMASR_EBS_DA) == 1) ? 1:0);
            xdata->emac_tx_data_buffer_access_error += (((tmp32_sr & ETH_DMASR_EBS_DA) == 0) ? 1:0);
        }
        else
        {
            xdata->emac_rx_data_xfer_error++;
            xdata->emac_rx_read_xfer_error += (((tmp32_sr & ETH_DMASR_EBS_RD) == 1) ? 1:0);
            xdata->emac_rx_write_xfer_error += (((tmp32_sr & ETH_DMASR_EBS_RD) == 0) ? 1:0);
            xdata->emac_rx_desc_access_error += (((tmp32_sr & ETH_DMASR_EBS_DA) == 1) ? 1:0);
            xdata->emac_rx_data_buffer_access_error += (((tmp32_sr & ETH_DMASR_EBS_DA) == 0) ? 1:0);
        }
    }

    /* Test for RX Interrupts first */
    if ((tmp32_sr & ETH_DMASR_RS) != 0)
    {
        /* Disable receive interrupt */
        tmp32_ier = STM32_EMAC_IN32 (device->dev_io_addr, ETH_DMAIER);
        tmp32_ier &= ~ETH_DMAIER_RIE;
        STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMAIER, tmp32_ier);

        /* Clear pending interrupts */
        if ((tmp32_sr & ETH_DMASR_ROS) != 0)
        {
            STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_ROS);
            if ((tmp32_sr & ETH_DMASR_NIS) != 0)
                STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_NIS);
        }

        /* Clear RX interrupt */
        STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_RS);
        if ((tmp32_sr & ETH_DMASR_NIS) != 0)
            STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_NIS);

        /* Activate the Receive HISR. */
        NU_Activate_HISR (&xdata->rx_hisr_cb);
    }

    /* Test for TX interrupts */
    if ((tmp32_sr & ETH_DMASR_TS) != 0)
    {
        /* Clear pending interrupts */
        if ((tmp32_sr & ETH_DMASR_ETS) != 0)
        {
            STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_ETS);
            if ((tmp32_sr & ETH_DMASR_NIS) != 0)
                STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_NIS);
        }

        /* Clear TX interrupt */
        STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_TS);
        if ((tmp32_sr & ETH_DMASR_NIS) != 0)
            STM32_EMAC_OUT32 (device->dev_io_addr, ETH_DMASR, ETH_DMASR_NIS);

        /* Activate the Transmit HISR. */
        NU_Activate_HISR (&xdata->tx_hisr_cb);
    }

}   /* Ethernet_Tgt_LISR */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Allocate_Descriptor
*
*   DESCRIPTION
*
*       This function allocates TX Single Large Buffer (SLB),
*       and RX NET buffer memory regions.
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
static STATUS  Ethernet_Tgt_Allocate_Descriptor (DV_DEVICE_ENTRY *device)
{
    STATUS           status;
    UINT32           *pointer;
    INT              i;

    /******************************/
    /* Set up RX descriptor chain */
    /******************************/

    /* Allocate memory for RX descriptors */
    status = NU_Allocate_Memory(&System_Memory, (VOID**)&pointer,
                                (NUM_RX_DESC * sizeof(STM32EMAC_RXDESC)), NU_NO_SUSPEND);
                                
    /* Set up RX descriptors */
    if (status == NU_SUCCESS)
    {
        /* Zero out allocated memory for RX descriptors */
        (VOID)memset (pointer, 0, NUM_RX_DESC * sizeof(STM32EMAC_RXDESC));

        /* Initialize global RX descriptor pointer */
        RXDescP = (STM32EMAC_RXDESC*)pointer;

        /* Initialize the list of descriptors */
        for (i = 0; i < NUM_RX_DESC; i++)
        {
            /* Set the OWN bit in rdes0 to indicate the buffer belongs to DMA */
            RXDescP[i].rdes0 |= STM32_EMAC_RX_DESC_OWN_BIT;

            /* Indicate rdes3 is next descriptor address */
            RXDescP[i].rdes1 |= (STM32_EMAC_RX_DESC_RCH_BIT | STM32_EMAC_RX_BUF_SIZE);

            /* Dequeue a NET buffer and assign it to this descriptor */
            RXDescP[i].rdes2 = (UINT32) MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

            /* Set next descriptor address */
            if (i == (NUM_RX_DESC-1))
            {
                /* Wrap around RX buffer descriptor ring */
                RXDescP[i].rdes3 = (UINT32)&RXDescP[0];
            }
            else
            {
                RXDescP[i].rdes3 = (UINT32)&RXDescP[i+1];
            }
        }

        /* Initialize RX buffer descriptor index */
        RXBufDesIdx = 0;

        /* Set start address of RX descriptor list */
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMARDLAR, (UINT32)RXDescP);
           
    /******************************/
    /* Set up TX descriptor chain */
    /******************************/

    /* Allocate memory for TX Buffer descriptors */
    status = NU_Allocate_Memory(&System_Memory, (VOID**)&pointer,
             (NUM_TX_DESC * sizeof(STM32EMAC_TXDESC)), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
            /* Zero allocated descriptor memory */
            (VOID)memset (pointer, 0, NUM_TX_DESC * sizeof(STM32EMAC_TXDESC));

            /* Initialize global RX descriptor pointer */
            TXDescP = (STM32EMAC_TXDESC*)pointer;

            /* Initialize the list of descriptors */
            for (i = 0; i < NUM_TX_DESC; i++)
            {
                /* Set flag to indicate 3rd descriptor address is next descriptor address */
                TXDescP[i].tdes0 = STM32_EMAC_TX_DESC_TCH_BIT; 

                /* Fields tdes1 and tdes2 will be filled in the EMAC_Xmit_Packet routine */

                /* Set next descriptor address */
                if (i == (NUM_TX_DESC-1))
                {
                    /* Wrap around TX buffer descriptor ring */
                    TXDescP[i].tdes3 = (UINT32)&TXDescP[0];
                }
                else
                {
                    TXDescP[i].tdes3 = (UINT32)&TXDescP[i+1];
                }
            }

            /* Initialize RX buffer descriptor index */
            TXBufDesIdx = 0;

            /* Set start address of TX descriptor list */
            STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMATDLAR, (UINT32)TXDescP);
        }
    }

    return (status);

}   /* Ethernet_Tgt_Allocate_Descriptor */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Calculate_Hash
*
*   DESCRIPTION
*
*       This function calculates a 6-bit address into a hash table for a
*       given multicast address.
*
*   INPUTS
*
*       UINT8  *multi_addr                  - Multicast address used to compute hash
*                                             address.
*
*   OUTPUTS
*
*       UINT32 crc                          - 6-bit address into 64-bit hash table.
*
**************************************************************************/
static UINT32 Ethernet_Tgt_Calculate_Hash (UINT8 *multi_addr)
{
    UINT8       i, j;
    UINT32      crc = 0xFFFFFFFF;
    UINT32      carry;
    UINT8       maddr[6];

    /* Put the ethernet address into a local array */
    NU_BLOCK_COPY (maddr, multi_addr, 6);

    /* Cycle through each character of the address */
    for (i = 0; i < 6; ++i)
    {
        /* Cycle through each bit of this character */
        for (j = 0; j < 8; ++j)
        {
            /* Update the CRC for this address */
            carry = ( (crc & 0x80000000) ? 0x01 : 0x00) ^ (maddr[i] & 0x01);
            crc <<= 1;
            maddr[i] >>= 1;

            if (carry)
            {
                crc = ((crc ^ CRC_POLYNOMIAL) | carry);
            }
        }
    }

    /* Return the upper 6 bits of the CRC. */
    crc >>= 26;

    return (crc);

}   /* Ethernet_Tgt_Calculate_Hash */

/*************************************************************************
*
*   NOTE:
*   All functions listed below this point are used by the Generic Ethernet
*   driver layer. The function names and parameters must remain the same,
*   however the function content will be specific to this device.
*
*************************************************************************/

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Enable
*
*   DESCRIPTION
*
*       This function enables the LM3S_ETH hardware
*
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Ethernet_Tgt_Enable (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    UINT32         tmp32;

    if (inst_handle != NU_NULL)
    {
        /* Enable MAC transmission */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_MACCR);
        tmp32 |= ETH_MACCR_TE;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_MACCR, tmp32);

        /* Flush TX FIFO */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_DMAOMR);
        tmp32 |= ETH_DMAOMR_FTF;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAOMR, tmp32);

        /* Enable MAC reception */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_MACCR);
        tmp32 |= ETH_MACCR_RE;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_MACCR, tmp32);

        /* Enable DMA transmission */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_DMAOMR);
        tmp32 |= ETH_DMAOMR_ST;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAOMR, tmp32);

        /* Enable DMA reception */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_DMAOMR);
        tmp32 |= ETH_DMAOMR_SR;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAOMR, tmp32);

        /* Enable DMA transmit and receive interrupts */
        tmp32 = STM32_EMAC_IN32 (inst_handle->io_addr, ETH_DMAIER);
        tmp32 |= (ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE);
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAIER, tmp32);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Disable
*
*   DESCRIPTION
*
*       This function disables the LM3S_ETH hardware
*
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Ethernet_Tgt_Disable (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    UINT32         tmp32;

    if (inst_handle != NU_NULL)
    {
        /* Disable DMA interrupts */
        tmp32 = STM32_EMAC_IN32 (inst_handle->io_addr, ETH_DMAIER);
        tmp32 &= ~ETH_DMAIER_NISE;
        tmp32 &= ~ETH_DMAIER_RIE;
        tmp32 &= ~ETH_DMAIER_TIE;
        STM32_EMAC_OUT32 (inst_handle->io_addr, ETH_DMAIER, tmp32);

        /* Disable DMA transmission */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_DMAOMR);
        tmp32 &= ~ETH_DMAOMR_ST;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAOMR, tmp32);

        /* Disable DMA reception */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_DMAOMR);
        tmp32 &= ~ETH_DMAOMR_SR;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_DMAOMR, tmp32);

        /* Disable MAC transmission */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_MACCR);
        tmp32 &= ~ETH_MACCR_TE;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_MACCR, tmp32);

        /* Disable MAC reception */
        tmp32 = STM32_EMAC_IN32(inst_handle->io_addr, ETH_MACCR);
        tmp32 &= ~ETH_MACCR_RE;
        STM32_EMAC_OUT32(inst_handle->io_addr, ETH_MACCR, tmp32);
    }
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Create_Extended_Data
*
*   DESCRIPTION
*
*       This function creates the LM3S_ETH extended data structure then attaches
*       it to the device.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the structure of the
*                                             device.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS or error code
*
**************************************************************************/
STATUS Ethernet_Tgt_Create_Extended_Data (DV_DEVICE_ENTRY *device)
{
    STATUS          status = NU_SUCCESS;
    VOID            *pointer;
    NU_MEMORY_POOL  *sys_pool_ptr;

    /* Allocate the memory required for the ethernet data. */
    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
    status = NU_Allocate_Memory (sys_pool_ptr, &pointer, sizeof(STM32_EMAC_XDATA), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Report error if memory allocation failed. */
        NLOG_Error_Log ("Failed to allocate memory for ethernet driver",
                        NERR_FATAL, __FILE__, __LINE__);
    }

    else
    {
        /* Clear the data. */
        UTL_Zero (pointer, sizeof (STM32_EMAC_XDATA));

        /* Use the user field in the device control block to store the
           extended device data. */
        device->user_defined_1 = (UINT32) pointer;

    }

    return (status);
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Get_ISR_Info
*
*   DESCRIPTION
*
*       This function returns the ISR information associated with this driver.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the structure of the
*                                             device.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS or error code
*
**************************************************************************/
VOID Ethernet_Tgt_Get_ISR_Info (ETHERNET_SESSION_HANDLE *ses_handle, ETHERNET_ISR_INFO *isr_info)
{
    STM32_EMAC_XDATA  *xdata;

    xdata = (STM32_EMAC_XDATA *)ses_handle->device->user_defined_1;

    /* Populate the info */
    isr_info->tx_hisr_cb    = &xdata->tx_hisr_cb;
    isr_info->tx_hisr_func  = &Ethernet_Tgt_TX_HISR;
    isr_info->rx_hisr_cb    = &xdata->rx_hisr_cb;
    isr_info->rx_hisr_func  = &Ethernet_Tgt_RX_HISR;
    isr_info->phy_hisr_cb   = &xdata->phy_hisr_cb;
    isr_info->phy_hisr_func = &PHY_HISR;

    isr_info->tx_irq        = (INT)ses_handle->inst_info->irq;
    isr_info->tx_lisr_func  = &Ethernet_Tgt_LISR;
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Target_Initialize
*
*   DESCRIPTION
*
*       This functions performs target specific initialization. This
*       function will initialize the LM3S_ETH Interrupt on advanced interrupt
*       controller, enable the clock and initialize the parallel
*       input/output controller.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID Ethernet_Tgt_Target_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device)
{

    /* Program Advance Interrupt Controller (AIC) to enable and connect the LM3S_ETH interrupt.
       Enable the lm3s_eth interrupt */
    (VOID) ESAL_GE_INT_Enable (inst_handle->irq, inst_handle->irq_type, inst_handle->irq_priority);

    /* Register the lm3s_eth data structure with this vector id */
    ESAL_GE_ISR_VECTOR_DATA_SET (inst_handle->irq, (VOID*) device);

}   /* Ethernet_Tgt_Target_Initialize */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Controller_Init
*
*   DESCRIPTION
*
*       This function is responsible for initializing and setting up the
*       LM3S_ETH controller.
*
*   INPUTS
*
*       UINT8           *ether_addr         - Pointer to the ethernet address.
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS Ethernet_Tgt_Controller_Init (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device, UINT8 *ether_addr)
{
    STATUS          status;
    STATUS          phy_status;
    INT             is_100_mbps    = PHY_NEGOT_100MBPS;
    INT             is_full_duplex = PHY_NEGOT_FULL_DUPLEX;
    PHY_CTRL        *phy_ctrl       = (PHY_CTRL *)inst_handle->phy_ctrl;
    UINT32          tmp32;

    /****************/
    /* Setup ETHMAC */
    /****************/
#define __SYSCFG_CLK_ENABLE()  (RCC->APB2ENR |= (RCC_APB2ENR_SYSCFGEN))

    /* Enable SYSCFG Clock */
    __SYSCFG_CLK_ENABLE();

    /* Confgure for RMII */
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;

    /* Reset */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_DMABMR);
    tmp32 |= ETH_DMABMR_SR;
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMABMR, tmp32);

    /* Wait for reset to complete */
    while((STM32_EMAC_IN32(device->dev_io_addr, ETH_DMABMR) & ETH_DMABMR_SR) != 0);

    /* Disable all interrupts */
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMAIER, 0);

    /* Ethernet MACMIIAR configuration */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_MACMIIAR);
    tmp32 |= ETH_MACMIIAR_CR_PRE102;
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACMIIAR, tmp32);


    /* Set up MAC configuration register */
    /* CRC stripping - disabled
     * Watchdog - enabled
     * Jabber - disabled
     * Inter frame gap - 96 bit times
     * Carrier sense - enabled
     * Fast ethernet speed - disabled (default)
     * Receive own - enabled
     * Loop back mode - disabled
     * Duplex mode - half duplex mode (default)
     * IPv4 checksum offload - disabled
     * Retry transmission - enabled
     * Automatic CRC stripping and padding - disabled
     * Back off limit - 10
     * Deferral clock - disabled
     * Transmitter enable - disabled (default)
     * Receiver enable - disabled (default)
     */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_MACCR);
    tmp32 |= (ETH_MACCR_IFG(0)| ETH_MACCR_RD);
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACCR, tmp32);

    /* Set up MAC frame filter configuration */
    /* Receive all - disabled
     * Hash or perfect filter - enabled
     * Source address filter - disabled
     * Pass control frames - block all
     * Broadcast frame reception - disabled
     * Destination address filter - normal
     * Promiscuous mode - disabled
     * Multicast frame filter -  perfect
     * Unicast frame filter -  perfect
     */
    tmp32 = (ETH_MACFFR_RX_ALL | ETH_MACFFR_HM | ETH_MACFFR_HU);
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACFFR, tmp32);

    /* Init MACHTHR configuration */
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACHTHR, 0);

    /* Init MACHTLR configuration */
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACHTLR, 0);

    /* Set up MACFCR configuration */
    /* Pause time - zero
     * Zero quanta phase - disabled
     * Pause low threshold - -4
     * Uni-cast pause frame detect - disabled
     * Receive flow control - disabled
     * Transmit flow control - disabled
     * back pressure activate - 0
     */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_MACFCR);
    tmp32 |= (ETH_MACFCR_ZQPD);
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACFCR, tmp32);

    /* Set up MACVLANTR */
    /* VTAG comparison - 16 bit
     * VTAG identifier = 0
     */
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACVLANTR, 0);

    /* DMA configuration - DMABMR */
    /* Mixed burst - disable
     * Address aligned beats - enable
     * 4xPBL mode - disable
     * Use separate PBL - disable
     * RX DMA burst length - 1 Beat
     * Fixed Burst - disable
     * RX TX priority ratio - 1:1
     * TX DMA burst length - 1 Beat
     * Enhanced descriptor format - disable
     * Descriptor skip length - 0
     * DMA arbitration - round robin
     * Software reset - off
     */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_DMABMR);
    tmp32 |= (ETH_DMABMR_AAB | ETH_DMABMR_USP | ETH_DMABMR_RDP(32) | ETH_DMABMR_FB | ETH_DMABMR_PBL(32) | ETH_DMABMR_DA);
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMABMR, tmp32);

    /* DMA configuration - DMAOMR */
    /* Dropping of TCPIP/checksum error frames - disabled
     * Receive store and forward - enabled
     * Flushing of received frames - disabled
     * Transmit store and forward - enabled
     * Flush transmit FIFO - disabled
     * Transmit threshold control - 64 bytes
     * Start/stop transmission - disabled
     * Forward error frames - disabled
     * Forward under sized good frames - disable
     * Receive threshold control - 64 bytes
     * Operate on second frame - disable
     * Start/stop receive - disable
     */
    tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_DMAOMR);
    tmp32 |= (ETH_DMAOMR_DTCEFD | ETH_DMAOMR_RSF | ETH_DMAOMR_TSF); 
    STM32_EMAC_OUT32(device->dev_io_addr, ETH_DMAOMR, tmp32);

    /* Allocate receive and transmit descriptor lists */
    status = Ethernet_Tgt_Allocate_Descriptor (device);

    /******************************/
    /* Set MAC address for device */
    /******************************/
    if (status == NU_SUCCESS)
    {
        /* Get the Ethernet MAC address. */
        Ethernet_Tgt_Get_Address (device, ether_addr);

        /* Set the Ethernet MAC address to the controller registers. */
        Ethernet_Tgt_Set_Address (inst_handle, ether_addr);
    }

    /**********************************************/
    /* PHY link status check and auto-negotiation */
    /**********************************************/
    if (status == NU_SUCCESS)
    {
        /* Check if the link is up */
        phy_status = PHY_Get_Link_Status (phy_ctrl);

        if ((phy_status == NU_SUCCESS) && phy_ctrl->phy_link_status == PHY_LINK_UP)
        {
            /* PHY auto negotiation */
            (VOID)PHY_Auto_Negotiate (phy_ctrl, device, PHY_AUTONEGOT_TRIES,
                                 &is_full_duplex, &is_100_mbps);
        }
        else
        {
            /* Set PHY speed and mode to 10MB and HALF DUPLEX */
            phy_ctrl->phy_speed = __10_MBPS;
            phy_ctrl->phy_mode  = HALF_DUPLEX;
        }
    }

    /* Set the speed bit in EMAC configuration register. */
    if ((phy_ctrl)->phy_speed == __100_MBPS)
    {
        /* Setup MAC for 100mbps */
        tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_MACCR);
        tmp32 |= ETH_MACCR_FES;
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACCR, tmp32);
    }

    /* Set the duplex bit in EMAC configuration register. */
    if (phy_ctrl->phy_mode  == FULL_DUPLEX)
    {
        /* Set up MAC for full duplex communications */
        tmp32 = STM32_EMAC_IN32(device->dev_io_addr, ETH_MACCR);
        tmp32 |= ETH_MACCR_DM;
        STM32_EMAC_OUT32(device->dev_io_addr, ETH_MACCR, tmp32);
    }

    /* Return status */
    return (status);
}   /* Ethernet_Tgt_Controller_Init */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Set_Phy_Dev_ID
*
*   DESCRIPTION
*
*       This function puts the Ethernet device ID into the PHY structure
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       NONE
*
**************************************************************************/
VOID Ethernet_Tgt_Set_Phy_Dev_ID (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    PHY_CTRL    *stm32_phy_ctrl = (PHY_CTRL*)inst_handle->phy_ctrl;

    /* Copy the Ethernet device ID from the instance handle into the PHY structure */
    stm32_phy_ctrl->dev_id = (DV_DEV_ID)inst_handle->dev_id;
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Get_Link_Status
*
*   DESCRIPTION
*
*       This function calls the PHY specific function to get link status
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*       INT             *link_status            - Pointer to where link status
*                                                 will be returned
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS Ethernet_Tgt_Get_Link_Status (ETHERNET_INSTANCE_HANDLE *inst_handle, INT *link_status)
{
    STATUS      status;
    PHY_CTRL    *stm32_phy_ctrl = (PHY_CTRL *)inst_handle->phy_ctrl;

    status = PHY_Get_Link_Status (stm32_phy_ctrl);

    if (status == NU_SUCCESS)
    {
        *link_status = stm32_phy_ctrl->phy_link_status;
    }

    return(status);
}

/**************************************************************************
*
*   FUNCTION
*
*       EMAC_Update_Multicast
*
*   DESCRIPTION
*
*       This function synchronizes the multicast list of the
*       specified device with the underlying EMAC hardware.
*       All addresses are removed and added again.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID    Ethernet_Tgt_Update_Multicast (DV_DEVICE_ENTRY *device)
{
    UINT32          index;
    UINT32          reg_num;
    UINT32          multicast_filter[2];
    INT             old_level;
    NET_MULTI       *net_multi;
    volatile UINT32 emac_hrt_reg;
    volatile UINT32 emac_hrb_reg;
    UINT32          maccr;


    /* Lock out interrupts. interrupts need to be disabled here. */
    old_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* First, disable the EMAC transmitter and receiver.  */
    maccr =  STM32_EMAC_IN32 (device->dev_io_addr, ETH_MACCR);
    maccr &= ~ETH_MACCR_TE;
    maccr &= ~ETH_MACCR_RE;
    STM32_EMAC_OUT32 (device->dev_io_addr, ETH_MACCR, maccr);

    /* Initialize the multicast address filter with zero. */
    multicast_filter[0] = 0;
    multicast_filter[1] = 0;

    /* Check if multicasting is enabled in the device options. */
    if (device->dev_flags & DV_MULTICAST)
    {
        /* Traverse the multicast addresses list. */
        for (net_multi = device->dev_ethermulti;
            net_multi != NU_NULL;
            net_multi = net_multi->nm_next)
        {
            /* Calculate the 6-bit index for this address */
            index = Ethernet_Tgt_Calculate_Hash (net_multi->nm_addr);
            reg_num = index >> 5;
            multicast_filter[reg_num] = multicast_filter[reg_num] | (1<<(index&31));
        }

        /* First read EMAC multicast hash registers. */
        emac_hrt_reg = STM32_EMAC_IN32 (device->dev_io_addr, ETH_MACHTHR);
        emac_hrb_reg = STM32_EMAC_IN32 (device->dev_io_addr, ETH_MACHTLR);

        /* Add the current multicast address. */
        emac_hrt_reg = (emac_hrt_reg | multicast_filter[1]);
        emac_hrb_reg = (emac_hrb_reg | multicast_filter[0]);

        /* Set the address to the EMAC multicast hash registers. */
        STM32_EMAC_OUT32 (device->dev_io_addr, ETH_MACHTHR,
                   (emac_hrt_reg | STM32_EMAC_MCAST_ENABLE));
        STM32_EMAC_OUT32 (device->dev_io_addr, ETH_MACHTLR,
                   (emac_hrb_reg | STM32_EMAC_MCAST_ENABLE));

    }

    /* Re enable the EMAC i.e. receiver and transmitters. */
    maccr =  STM32_EMAC_IN32 (device->dev_io_addr, ETH_MACCR);
    maccr |= ETH_MACCR_TE;
    maccr |= ETH_MACCR_RE;
    STM32_EMAC_OUT32 (device->dev_io_addr, ETH_MACCR, maccr);

    /* Restore the interrupts. */
    NU_Local_Control_Interrupts (old_level);

}   /* EMAC_Update_Multicast */

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Phy_Initialize
*
*   DESCRIPTION
*
*       This function calls the PHY specific initialization function
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*       DV_DEVICE_ENTRY *device                 - Pointer to the device
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS Ethernet_Tgt_Phy_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device)
{
    STATUS      status;
    PHY_CTRL    *stm32_phy_ctrl = (PHY_CTRL *)inst_handle->phy_ctrl;

    status = PHY_Initialize (stm32_phy_ctrl, device);

    return(status);
}

/**************************************************************************
*
*   FUNCTION
*
*       Ethernet_Tgt_Notify_Status_Change
*
*   DESCRIPTION
*
*       This function calls the specific PHY link status change function
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*
*   OUTPUTS
*
*       NONE
*
**************************************************************************/
VOID Ethernet_Tgt_Notify_Status_Change (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    PHY_CTRL    *stm32_phy_ctrl = (PHY_CTRL *)inst_handle->phy_ctrl;

    PHY_Notify_Status_Change (stm32_phy_ctrl);
}
