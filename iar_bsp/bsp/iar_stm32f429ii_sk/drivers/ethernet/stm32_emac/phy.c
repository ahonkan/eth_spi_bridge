/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       phy.c
*
*   COMPONENT
*
*       PHY - Ethernet PHY Driver
*
*   DESCRIPTION
*
*       This file contains the PHY Class Driver functions.
*
*   FUNCTIONS
*
*       PHY_Initialize
*       PHY_Get_Link_Status
*       PHY_Auto_Negotiate
*       PHY_Read
*       PHY_Write
*       PHY_LISR
*       PHY_HISR
*       PHY_Notify_Status_Change
*       PHY_Poll_Expire
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       externs.h
*       mii.h
*       phy.h
*       stm32_emac.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "networking/nu_networking.h"
#include "networking/mii.h"
#include "bsp/drivers/ethernet/stm32_emac/phy.h"
#include "bsp/drivers/ethernet/stm32_emac/ethernet_tgt.h"

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern NU_MEMORY_POOL        System_Memory;

/*********************************/
/* EXTERNAL FUNCTION DEFINITIONS */
/*********************************/

/*********************************/
/* FUNCTION DEFINITIONS          */
/*********************************/
static VOID PHY_Poll_Expire (UNSIGNED id); 

/*******************************/
/* LOCAL VARIABLE DECLARATIONS */
/*******************************/

/*************************************************************************
*
*   FUNCTION
*
*       PHY_Initialize
*
*   DESCRIPTION
*
*       This function initializes the PHY hardware
*
*   INPUTS
*
*       PHY_CTRL        *phy_ctrl           - PHY control
*       DV_DEVICE_ENTRY *device             - Pointer of the device struct
*
*   OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*
*************************************************************************/
STATUS  PHY_Initialize(PHY_CTRL *phy_ctrl, DV_DEVICE_ENTRY *device) 
{
    STATUS           status = STM32_EMAC_FAILURE;
    STM32_EMAC_XDATA *xdata = (STM32_EMAC_XDATA *) device->user_defined_1;

    if (phy_ctrl != NU_NULL)
    {
        /* Locate the PHY address. */
        for (phy_ctrl->phy_addr=0; phy_ctrl->phy_addr < PHY_MAX_PHY_ADDR; phy_ctrl->phy_addr++)
        {
            /* First check to see whether PHY is connected or not. */
            status = PHY_Get_Link_Status (phy_ctrl);

            if (status == NU_SUCCESS)
            {
                /* Address found so abort search. */
                status = NU_SUCCESS;
                break;
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Check if this PHY is using polled mode */
            if (phy_ctrl->phy_polled_link)
            {
                /* Allocate memory for the PHY Timer Control Block */
                status = NU_Allocate_Memory(&System_Memory, (VOID *)&phy_ctrl->phy_timer,
                                            sizeof (NU_TIMER), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Zero out allocated space */
                    (VOID)memset(phy_ctrl->phy_timer, 0, sizeof(NU_TIMER));
 
                    /* Create timer for link status polling */
                    status = NU_Create_Timer (phy_ctrl->phy_timer,
                                              "EMACPHY",
                                              PHY_Poll_Expire,
                                              (UNSIGNED)&xdata->phy_hisr_cb,
                                              PHY_POLL_INTERVAL,
                                              PHY_POLL_INTERVAL,
                                              NU_ENABLE_TIMER);
                }

            }
            else
            {
               /* Not supported for this target */

            }
        }
    }

    /* Return status */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PHY_Get_Link_Status
*
*   DESCRIPTION
*
*       This function gets the PHY link status.
*
*   INPUTS
*
*       PHY_CTRL *phy_ctrl                  - PHY control structure
*
*   OUTPUTS
*
*       STATUS   status                     - PHY status
*
*************************************************************************/
INT     PHY_Get_Link_Status(PHY_CTRL *phy_ctrl) 
{
    INT      status = STM32_EMAC_FAILURE;
    UINT16   phy_data1 = 0;
    UINT16   phy_data2 = 0;

    /* Read  PHY id registers. */
    PHY_Read (phy_ctrl->phy_io_addr, phy_ctrl->phy_addr, PHY_PHYIDR1, &phy_data1);
    PHY_Read (phy_ctrl->phy_io_addr, phy_ctrl->phy_addr, PHY_PHYIDR2, &phy_data2);

    if (((phy_data1 ) != 0) &&
        ((phy_data2 ) != 0) &&
        ((phy_data1 ) != 0xFFFF) &&
        ((phy_data2 ) != 0xFFFF))
    {
        status = NU_SUCCESS;

        /* Read the PHY status register */
        PHY_Read (phy_ctrl->phy_io_addr, phy_ctrl->phy_addr, PHY_BMSR, &phy_data1);
 
        if (phy_data1 & PHY_BMSR_LINK_STATUS)
        {
            phy_ctrl->phy_link_status = PHY_LINK_UP;
        }
        else
        {
            phy_ctrl->phy_link_status = PHY_LINK_DOWN;
        }

        /* Update link speed and duplex mode */
        if (phy_data1 & PHY_BMSR_100MBPS_FULLD)
        {
            phy_ctrl->phy_speed = __100_MBPS;
            phy_ctrl->phy_mode  = FULL_DUPLEX;
        }
        else if (phy_data1 & PHY_BMSR_100MBPS_HALFD)
        {
            phy_ctrl->phy_speed = __100_MBPS;
            phy_ctrl->phy_mode  = HALF_DUPLEX;
        }
        else if (phy_data1 & PHY_BMSR_10MBPS_FULLD)
        {
            phy_ctrl->phy_speed = __10_MBPS;
            phy_ctrl->phy_mode  = FULL_DUPLEX;
        }
        else if (phy_data1 & PHY_BMSR_10MBPS_HALFD)
        {
            phy_ctrl->phy_speed = __10_MBPS;
            phy_ctrl->phy_mode  = HALF_DUPLEX;
        }
    }
    else
    {
        status = STM32_EMAC_FAILURE;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PHY_Auto_Negotiate
*
*   DESCRIPTION
*
*       This function does PHY auto-negotiation to resolve the mode and
*       speed given the Ethernet device ID and number of retries. 
*
*   INPUTS
*
*       PHY_CTRL        *phy_ctrl           - PHY control structure
*       DV_DEVICE_ENTRY *device             - Pointer to the device struct
*       UINT32          retries             - Number of retries
*       INT             *mode               - Duplex/Half-duplex
*       INT             *speed              - Connection speed 
*
*   OUTPUTS
*
*       STATUS          status              - PHY status
*
*************************************************************************/
STATUS  PHY_Auto_Negotiate (PHY_CTRL *phy_ctrl, DV_DEVICE_ENTRY *device, 
                            UINT32 retries, INT *mode, INT *speed)
{
    INT       status;

    /* Call the MII auto-negotiation function. */
    status = MII_AutoNeg (device, phy_ctrl->phy_addr, retries, mode, speed,
                          phy_ctrl->mii_read, phy_ctrl->mii_write);

    return (status);    
}

  
/*************************************************************************
*
*   FUNCTION
*
*       PHY_Read
*
*   DESCRIPTION
*
*       This function reads a PHY location associated with the reg_id
*       given the Ethernet device ID.  Data is stored in phy_data.
*
*   INPUTS
*
*       UINT32   phy_io_addr                - PHY base address
*       UINT16   phy_addr                   - PHY address
*       UINT16   reg_id                     - PHY register ID
*       UINT16   *phy_data                  - PHY data read 
*
*   OUTPUTS
*
*       STATUS   status                     - PHY status
*
*************************************************************************/
STATUS  PHY_Read(UINT32 phy_io_addr, UINT16 phy_addr, UINT16 reg_id, UINT16 *phy_data)
{
    INT     status = NU_SUCCESS;
    UINT32  temp32;

    /* Get Busy bit */
    status = STM32_EMAC_IN32(phy_io_addr, ETH_MACMIIAR) & ETH_MACMIIAR_MII_BUSY;

    /* Proceed only if the Busy bit is 0 */
    if (status == NU_SUCCESS)
    {
        /* Calculate the value */
        temp32  = BIT_FLD_SET(11, phy_addr);
        temp32 |= BIT_FLD_SET(6, reg_id);
        temp32 |= ETH_MACMIIAR_CR(0x01); /* 100 Mhz */
        temp32 |= ETH_MACMIIAR_MII_READ; 
        temp32 |= ETH_MACMIIAR_MII_BUSY; 

        /* Perform MII write */   
        STM32_EMAC_OUT32(phy_io_addr, ETH_MACMIIAR, temp32);

        /* Wait until the Busy bit is clear */
        while ((STM32_EMAC_IN32(phy_io_addr, ETH_MACMIIAR) & ETH_MACMIIAR_MII_BUSY));

        /* Read data from ETH_MACMIIDR */
        *phy_data = (UINT16)STM32_EMAC_IN32(phy_io_addr, ETH_MACMIIDR);
    }

    return (status);    
}

/*************************************************************************
*
*   FUNCTION
*
*       PHY_Write
*
*   DESCRIPTION
*
*       This function writes phy_data to a PHY location associated with the 
*       reg_id given the Ethernet device ID. 
*
*   INPUTS
*
*       UINT32   phy_io_addr                - PHY base address
*       UINT16   phy_addr                   - PHY address
*       UINT16   reg_id                     - PHY register ID
*       UINT16   phy_data                   - PHY data written 
*
*   OUTPUTS
*
*       STATUS   status                     - PHY status
*
*************************************************************************/
STATUS  PHY_Write(UINT32 phy_io_addr, UINT16 phy_addr, UINT16 reg_id, UINT16 phy_data) 
{
    INT     status = NU_SUCCESS;
    UINT32  temp32;

    /* Get Busy bit */
    status = STM32_EMAC_IN32(phy_io_addr, ETH_MACMIIAR) & ETH_MACMIIAR_MII_BUSY;

    /* Proceed only if the Busy bit is 0 */
    if (status == 0)
    {
        /* Write data to ETH_MACMIIDR */
        STM32_EMAC_OUT32(phy_io_addr, ETH_MACMIIDR, phy_data);

        /* Calculate the value */
        temp32  = BIT_FLD_SET(11, phy_addr);
        temp32 |= BIT_FLD_SET(6, reg_id);
        temp32 |= ETH_MACMIIAR_CR(1); /* 100 MHz */
        temp32 |= ETH_MACMIIAR_MII_WRITE; 
        temp32 |= ETH_MACMIIAR_MII_BUSY; 

        /* Perform MII write */   
        STM32_EMAC_OUT32(phy_io_addr, ETH_MACMIIAR, temp32);

        /* Wait until the Busy bit is clear */
        while ((STM32_EMAC_IN32(phy_io_addr, ETH_MACMIIAR) & ETH_MACMIIAR_MII_BUSY));
    }  

    return (status);    
}

/**************************************************************************
*
*   FUNCTION
*
*       PHY_LISR
*
*   DESCRIPTION
*
*       This function is the Low Level Interrupt Routine for the PHY 
*       interrupt.
*
*   INPUTS
*
*       INT      vector                     - The PHY interrupt vector.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID PHY_LISR(INT vector)
{

}   /* PHY_LISR */

/**************************************************************************
*
*   FUNCTION
*
*       PHY_HISR
*
*   DESCRIPTION
*
*       This function is responsible for informing the NET that link status 
*       has been changed.
* 
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID PHY_HISR(VOID)
{
    INT                             old_level;
    NU_HISR *                       hcb;
    DV_DEVICE_ENTRY *               device;
    ETHERNET_INSTANCE_HANDLE *      stm32_inst_handle; 
    PHY_CTRL *                      phy_ctrl;
    PHY_CTRL                        tmp_phy_ctrl;
    INT                             is_100_mbps    = PHY_NEGOT_100MBPS;
    INT                             is_full_duplex = PHY_NEGOT_FULL_DUPLEX;

    /* Get the HISR control block */
    hcb = (NU_HISR *)NU_Current_HISR_Pointer();

    /* Get the device pointer from the HCB */
    device = (DV_DEVICE_ENTRY *)hcb->tc_app_reserved_1;

    /* Get the ETHERNET_INSTANCE_HANDLE structure */ 
    stm32_inst_handle = (ETHERNET_INSTANCE_HANDLE *) (device->dev_driver_options);

    /* Get the PHY_CTRL structure */
    phy_ctrl = (PHY_CTRL *)stm32_inst_handle->phy_ctrl;

    /* Disable interrupts. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Copy this PHY control structure */
    tmp_phy_ctrl = *phy_ctrl;

    /* Get link status */
    (VOID)PHY_Get_Link_Status(&tmp_phy_ctrl); 

    /* Auto-negotiate if the link was down */
    if (tmp_phy_ctrl.phy_link_status == PHY_LINK_UP)
    {
        if (phy_ctrl->phy_link_status == PHY_LINK_DOWN)
        {
            /* Try to auto negotiate */    
            (VOID)PHY_Auto_Negotiate (phy_ctrl, device, 
                                      PHY_AUTONEGOT_TRIES, &is_full_duplex, &is_100_mbps);
        }
    }

    /* Check if the link status has changed */ 
    if (phy_ctrl->phy_link_status != tmp_phy_ctrl.phy_link_status)
    {
        /* Update the current link status */
        phy_ctrl->phy_link_status = tmp_phy_ctrl.phy_link_status;

        /* Notify the link change */
        PHY_Notify_Status_Change (phy_ctrl);
    }

    /* Restore interrupts. */
    NU_Local_Control_Interrupts(old_level);   
}

/**************************************************************************
*
*   FUNCTION
*
*       PHY_Notify_Status_Change
*
*   DESCRIPTION
*
*       This function sends a notification message if the link status 
*       has changed.
* 
*   INPUTS
*
*       PHY_CTRL *phy_ctrl                  - PHY control structure
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID PHY_Notify_Status_Change (PHY_CTRL *phy_ctrl)
{
    NET_IF_LINK_STATE_MSG   if_status_change_msg;

    /* Generate message based on link status */
    if (phy_ctrl->phy_link_status == PHY_LINK_UP)
    {
        strcpy(if_status_change_msg.msg, "LINK UP");
    }

    else
    {
        strcpy(if_status_change_msg.msg, "LINK DOWN");
    }
#ifdef CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED 
    if_status_change_msg.dev_id = phy_ctrl->dev_id;
    if_status_change_msg.event_type = LINK_CHANGE_STATE;
    NU_EQM_Post_Event(&NET_Eqm, (EQM_EVENT*)(&if_status_change_msg),
                                sizeof(if_status_change_msg), NU_NULL);
#else
    /* Use EVENT manager to send this change */
    (VOID)ENC_Notification_Send (phy_ctrl->dev_id,
                                 LINK_CHANGE_STATE,
                                 if_status_change_msg.msg,
                                 strlen(if_status_change_msg.msg)+1);
#endif
}

/**************************************************************************
*
*   FUNCTION
*
*       PHY_Poll_Expire
*
*   DESCRIPTION
*
*       This function is the expiration routine of the EMAC driver
*       timer in polling mode. At each expiration of the timer, the link 
*       status is polled and checked to notify NET if there was any change.
* 
*   INPUTS
*
*       UNSIGNED id                         - Timer expiration ID which is
*                                               tied to the HISR CB.             
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
static VOID PHY_Poll_Expire (UNSIGNED id)
{
    /* Activate the PHY HISR. */
    NU_Activate_HISR((NU_HISR *)id);
}

