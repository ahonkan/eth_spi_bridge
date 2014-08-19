/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
*   FILE NAME                                                           
*                                                                      
*        nu_usb_pmg_ext.c                                 
*                                                                      
*   COMPONENT                                                            
*
*       Nucleus USB Software
*                                                                      
*   DESCRIPTION                                                          
*
*       This file provides the implementation of external interfaces of
*       power manager . 
*
*                                                                      
*   DATA STRUCTURES                                                      
*
*       None
*
*   FUNCTIONS 
*                                                           
*       NU_USB_PMG_Update_Dev_Pwr_Src   API used for updating the value of
*                                       power source in
*                                       NU_USB_DEV_PW_ATTRIB control block.
*       NU_USB_PMG_Update_Dev_U1_Enable API used for updating the value of
*                                       u1_enable capability in
*                                       NU_USB_DEV_PW_ATTRIB control block.
*       NU_USB_PMG_Update_Dev_U2_Enable API used for updating the value of
*                                       u2_enable capability in
*                                       NU_USB_DEV_PW_ATTRIB control block.
*       NU_USB_PMG_Update_Dev_LTM_Enable 
*                                       API used for updating the value of
*                                       ltm_enable capability in
*                                       NU_USB_DEV_PW_ATTRIB control
*                                       block.
*       NU_USB_PMG_Get_Dev_Pwr_Attrib   APi used to get the power 
*                                       attributes of the device.
*       NU_USB_PMG_Set_Dev_Pwr_Attrib   API used to set the power 
*                                       attributes of the device.
*       NU_USB_PMG_Update_Intf_Pwr_Attrib
*                                       This API is used for updating the
*                                       power attributes of a NU_USB_INTF.
*       NU_USB_PMG_Set_Link_State       API used to update the link state
*                                       as well as to set the hardware in
*                                       the power mode that corresponds
*                                       with the link state.
*       NU_USB_PMG_Set_Link_State       API used to update the link state
*                                       as well as to set the hardware in
*                                       the power mode that corresponds
*                                       with the link state.
*       NU_USB_PMG_Calculate_SEL        This function calcuates the system
*                                       exit latency.
*       NU_USB_PMG_Calculate_BELT_Value This API is used to calculate the
*                                       BELT value according to new link
*                                       state.
*       NU_USB_PMG_Generate_LTM         This function updates BELT value
*                                       BELT value and pass it to HW
*                                       driver for LTM generation. 
*
*   DEPENDENCIES       
*
*       nu_usb.h              All USB definitions
*
************************************************************************/

/***********************************************************************/
#include "connectivity/nu_usb.h"

/*Only include this file if stack is configured for Super Speed USB 
  (USB 3.0). */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/***********************************************************************/

/*************************************************************************
* 
*   FUNCTION                 
*
*       NU_USB_PMG_Update_Dev_Pwr_Src
*                                                                   
*   DESCRIPTION                                                          
*                                                                             
*       This API is used for updating the value of power source in 
*       NU_USB_DEV_PW_ATTRIB control block.  
*                                                                       
*   INPUTS                                                               
*
*       cb                  Pointer to device control block.
*       self_powered        Flag showing whether the device is 
*                           self_powered(1) or bus powered(0).
*
*   OUTPUTS
*                                                                      
*      NU_SUCCESS           Successful completion.  
*      NU_USB_INVLD_ARG     Any of the input argument in invalid. 
*
*************************************************************************/
STATUS NU_USB_PMG_Update_Dev_Pwr_Src ( NU_USB_DEVICE    *cb,
                                       BOOLEAN       self_powered)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->pw_attrib.self_powered = self_powered;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Update_Dev_U1_Enable
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for updating the value of U1_Enable capability 
*       in NU_USB_DEV_PW_ATTRIB control block. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to device control block. 
*       u1_enable           It has two possible values 
*                           NU_TRUE     Enable device capability to 
*                                       initiate u1 entry  
*                           NU_FALSE    Disable device capability to 
*                                       initiate u1 entry  
*                                                                      
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/

STATUS NU_USB_PMG_Update_Dev_U1_Enable (NU_USB_DEVICE   *cb,
                                        BOOLEAN         u1_enable)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
   
    cb->pw_attrib.u1_enable= u1_enable;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Update_Dev_U2_Enable
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for updating the value of U2_Enable capability 
*       in NU_USB_DEV_PW_ATTRIB control block. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to device control block. 
*       u2_enable           It has two possible values 
*                           NU_TRUE     Enable device capability to 
*                                       initiate u2 entry  
*                           NU_FALSE    Disable device capability to 
*                                       initiate u2 entry  
*                                                                      
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/

STATUS NU_USB_PMG_Update_Dev_U2_Enable (NU_USB_DEVICE   *cb,
                                        BOOLEAN         u2_enable)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->pw_attrib.u2_enable = u2_enable;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Update_Dev_LTM_Enable
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for updating the value of LTM_Enable capability 
*       in NU_USB_DEV_PW_ATTRIB control block. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to device control block. 
*       ltm_enable          It has two possible values 
*                           NU_TRUE     Enable LTM capability of the
*                                       device.                                       
*                           NU_FALSE    Disable LTM capability of the
*                                       device.  
*                                                                      
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/

STATUS NU_USB_PMG_Update_Dev_LTM_Enable (NU_USB_DEVICE   *cb,
                                        BOOLEAN         ltm_enable)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->pw_attrib.ltm_enable = ltm_enable;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS ); 
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Get_Dev_Pwr_Attrib
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for retrieving the power attributes of a 
*       NU_USB_DEVICE. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to device control block. 
*       pwr_attrib          Double pointer to the NU_USB_PW_ATTRIB 
*                           structure. It will be updated to the pointer
*                           of NU_USB_PWR_ATTRIB structure defined in 
*                           NU_USB_DEVICE control block.                                           
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/
STATUS NU_USB_PMG_Get_Dev_Pwr_Attrib (  NU_USB_DEVICE *cb,
                                        NU_USB_DEV_PW_ATTRIB **pwr_attrib)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pwr_attrib);
    
    *pwr_attrib = &cb->pw_attrib; 

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS ); 
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Set_Dev_Pwr_Attrib
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for setting the power attributes of a 
*       NU_USB_DEVICE. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to NU_USB_DEVICE control block. 
*       pwr_attrib          Pointer to the NU_USB_PW_ATTRIB 
*                           structure. 
*
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/
STATUS NU_USB_PMG_Set_Dev_Pwr_Attrib (  NU_USB_DEVICE *cb,
                                        NU_USB_DEV_PW_ATTRIB *pwr_attrib)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pwr_attrib);

    cb->pw_attrib.ltm_enable    = pwr_attrib->ltm_enable;
    cb->pw_attrib.remote_wakeup = pwr_attrib->remote_wakeup;
    cb->pw_attrib.self_powered  = pwr_attrib->self_powered;
    cb->pw_attrib.u1_enable     = pwr_attrib->u1_enable;
    cb->pw_attrib.u2_enable     = pwr_attrib->u2_enable;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS ); 
}

/*************************************************************************
*
*   FUNCTION                 
*
*       NU_USB_PMG_Update_Intf_Pwr_Attrib
*                                                                      
*   DESCRIPTION                                                          
*    
*       This API is used for updating the power attributes of 
*       NU_USB_INTF. 
*    
*   INPUTS                                                               
*                                                                            
*       cb                  Pointer to device control block. 
*       intf_number         Interface number.
*       pwr_attrib          Pointer to the NU_USB_INTF_PW_ATTRIB 
*                           structure. 
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/

STATUS NU_USB_PMG_Update_Intf_Pwr_Attrib ( NU_USB_DEVICE   *cb,
                                           UINT8           intf_number,
                                    NU_USB_INTF_PW_ATTRIB  *pwr_attrib)
{
    NU_USB_INTF *intf;
    NU_USB_CFG  *active_cfg;
    UINT8       active_cfg_num;
    STATUS      status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pwr_attrib);
    
    intf       = NU_NULL;
    active_cfg = NU_NULL;

    /* Get configuration number of current active configuration. */
    status = NU_USB_DEVICE_Get_Active_Cfg_Num(cb, &active_cfg_num);
    if ( status == NU_SUCCESS )
    {
        /* Get pointer to current active configuration. */
        status = NU_USB_DEVICE_Get_Cfg(cb, active_cfg_num, &active_cfg);
        if ( status == NU_SUCCESS )
        {
            /* Get pointer to desired interface. */
            status = NU_USB_CFG_Get_Intf(active_cfg, intf_number, &intf);
            if ( status == NU_SUCCESS )
            {
                intf->pw_attrib.function_rw_capable = 
                    pwr_attrib->function_rw_capable;
                intf->pw_attrib.function_rw_state = 
                    pwr_attrib->function_rw_state;
                intf->pw_attrib.function_suspend_state = 
                    pwr_attrib->function_suspend_state;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status ); 
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_PMG_Set_Link_State
*                                                                      
*   DESCRIPTION
*
*       API used to update the link state as well as to set the hardware
*       in the power mode that corresponds with the link state.
*       
*   INPUTS                                                               
*                                                                            
*       device              Pointer to NU_USB_DEVICE control block.
*       link_state          Variable containing current link state to be
*                           saved.
*
*   OUTPUTS
*        
*       NU_SUCCESS          Successful operation.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid. 
*
*************************************************************************/
STATUS NU_USB_PMG_Set_Link_State (  NU_USB_DEVICE   *device,
                                    UINT8           link_state  )
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(device);

    /* Update the link state in the Control Block. */
    status = NU_USB_DEVICE_Set_Link_State (device, link_state);
    if ( status == NU_SUCCESS)
	{
        /* Change the power mode of hardware with the corresponding
           link state. */
        status = NU_USB_HW_Update_Power_Mode (device->hw, link_state);
	}

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status ); 
}

/*************************************************************************
*
* FUNCTION
*     NU_USB_PMG_Calculate_SEL
*
* DESCRIPTION
*     This function calculates the System Exit Latency (SEL).
*
* INPUTS
*     dev                pointer to device control block.
*     sel                pointer to device system exit latency
*                        control block.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.  
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*
*************************************************************************/
STATUS NU_USB_PMG_Calculate_SEL (  NU_USB_DEVICE     *dev,
                                   NU_USB_DEVICE_SEL *sel  )
{
    STATUS          status              = NU_SUCCESS;
    NU_USB_DEVICE   *device             = NU_NULL;
    NU_USB_BOS      *bos                = NU_NULL;
    UINT8           u1_dev_lat          = 0;
    UINT8           hub_count           = 0;
    UINT8           u1_max_latency      = 0;
    UINT16          u2_dev_lat          = 0;
    UINT16          u2_max_latency      = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(dev);
    NU_USB_PTRCHK(sel);

    /* Make local copy of device pointer as it will be used  in the
     * subsequent while loop.
     */
    device = dev;

    if(device->parent != NU_NULL)
    {
        /* Get the BOS descriptor handle. */
        status = NU_USB_DEVICE_Get_BOS(device, &bos);

        /* Get the U1 and U2 exit latencies of device from the SuperSpeed
         * device capability descriptor.
         */
        if (status == NU_SUCCESS)
        {
            status = NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat(bos, &u1_dev_lat);
        }

        if (status == NU_SUCCESS)
        {
            status = NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat(bos, &u2_dev_lat);
        }

        /* In order to calculate the system exit latency we have to
           consider two scenarios:
         * 1. Device is directly connected to roothub.
         * 2. Device is connected to roothub via other hubs.
         * 3. For more details on calculating system exit latency in these
         *    codition refer to appendix C, sections C1.5 and C.2 of specs.
         */
        if (status == NU_SUCCESS)
        {
            /* The algorithm works as follow :
             *
             * If device is directly connected to root hub then there
             *    no delay is incurred due to hubs therfore U1 end to end
             *    exit latency is larger of device and root port U1 exit
             *    latency same holds for U2 exit latencies.
             * Else
             * 1. Start from the device and move up in the tiers untill root
             *    hub is reached.
             * 2. Keep record of intervening hubs in the variable hub_count.
             * 3. At each iteration of while loop compare the bU1DevExitLat
             *    and wU2DevExitLat values of device and its parent and save
             *    maximum of each in the u1_tmp_latency and u2_tmp_latency
             *    respectively.
             * 4. Use the values found in the step4 to calculate U1SEL,U1PEL
             *     ,U2SEL and U2PEL.
             */
            u1_max_latency = u1_dev_lat;
            u2_max_latency = u2_dev_lat;

            while ((device->parent != NU_NULL) &&
                   (status == NU_SUCCESS))
            {
                status = NU_USB_DEVICE_Get_BOS(device->parent, &bos);

                if (status == NU_SUCCESS)
                {
                    status = NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat(bos,
                                                          &u1_dev_lat);
                }

                if (status == NU_SUCCESS)
                {
                    status = NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat(bos,
                                                          &u2_dev_lat);
                }

                /* After a latency of tHubPort2PortExitLat(1us),the
                 * time it takes hub to know that one of its downstream
                 * port is awakening, hub initiates LFPS on upstream
                 * link.This constant tHubPort2PortExitLat adds in the
                 * exit latencies of all the intervening hubs except
                 * for the hub directly connected to the device.
                 */
                if (hub_count != 0)
                {
                     u1_dev_lat += tHubPort2PortExitLat;
                     u2_dev_lat += tHubPort2PortExitLat;
                }
                
                if (u1_dev_lat > u1_max_latency)
                {
                    u1_max_latency = u1_dev_lat;
                }

                if (u2_dev_lat > u2_max_latency)
                {
                    u2_max_latency = u2_dev_lat;
                }

                ++hub_count;
                device = device->parent;
            }

            if (status == NU_SUCCESS)
            {
                if ( hub_count - 1 )
				{
					sel->u1_sel = u1_max_latency +
	                                 USB_PMG_CALC_SEL_PARAMETER(hub_count-1);
	                sel->u2_sel = u2_max_latency +
	                                 USB_PMG_CALC_SEL_PARAMETER(hub_count-1);
				}
				else
				{
					sel->u1_sel = u1_max_latency + USB_PMG_MAX_T3_LATENCY;
					sel->u2_sel = u2_max_latency + USB_PMG_MAX_T3_LATENCY;
				}
				sel->u1_pel = u1_max_latency;
                sel->u2_pel = u2_max_latency;
            }

        }
    }

    return ( status );
}

/*************************************************************************
*
* FUNCTION
*     NU_USB_PMG_Calculate_BELT_Value
*
* DESCRIPTION
*     This function calculates BELT value which is used by LTMs.
*
* INPUTS
*     device             pointer to device control block.
*     belt_value         pointer hold to belt value.
*     new_link_state     new state where link is intended to be
*                        transitioned.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.  
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*
*************************************************************************/
STATUS NU_USB_PMG_Calculate_BELT_Value(NU_USB_DEVICE *device,
                                       UINT16        *belt_value,
                                       UINT8          new_link_state)
{
    STATUS               status      = NU_SUCCESS;
    NU_USB_DEV_PW_ATTRIB *pwr_attrib = NU_NULL;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(belt_value);

    /* Get the power attributes of the device. */
    status = NU_USB_PMG_Get_Dev_Pwr_Attrib(device, &pwr_attrib);
    if (status == NU_SUCCESS)
    {
        /* Here we update the BELT value depending upon the link 
         * transition state.
         *
         * 1.If link is going to be in active state then BELT value is
         *   calculated by subtracting u1_sel from worst case U1SEL value.
         * 2.If link is going to be in low power state then BELT value is
         *   calculated by subtracting u2_sel from worst case U2SEL value.
         *
         * For details refer to USB 3.0 specs, appendix C section C.4.Here
         * LT_Active state is reprsented by the active link state and
         * LT_Idle state is representd by low power state. The state is
         * passed as parameter.
         */

        if (new_link_state == USB_LINK_STATE_U0)
        {
            *belt_value = USB_PMG_MAX_U1_LATENCY - pwr_attrib->sel.u1_sel;
        }
        else 
        {
            *belt_value = USB_PMG_MAX_U2_LATENCY - pwr_attrib->sel.u2_sel;
        }
    }

    return ( status );
}

/*************************************************************************
*
* FUNCTION
*     NU_USB_PMG_Generate_LTM
*
* DESCRIPTION
*     This function updates BELT value and pass it to controller
*     driver for LTM generation.
*
* INPUTS
*     device             pointer to device control block.
*     new_link_state     new state where link is intended to be
*                        transitioned.
*
* OUTPUTS
*
*     NU_SUCCESS                 Successful completion.
*     NU_USB_INVLD_ARG           Any of the input arguments is in invalid.
*     NU_USB_DEVICE_LTM_DISABLED LTM generation feature is disabled.
*
*************************************************************************/
STATUS NU_USB_PMG_Generate_LTM(NU_USB_DEVICE *device,
                               UINT16        new_link_state)
{
    STATUS               status      = NU_SUCCESS;
    NU_USB_DEV_PW_ATTRIB *pwr_attrib = NU_NULL;
    UINT16               belt_value  = 0;

    NU_USB_PTRCHK(device);

    status = NU_USB_PMG_Get_Dev_Pwr_Attrib(device, &pwr_attrib);
    if (status == NU_SUCCESS)
    {
        /* We need to generate LTM only if device is LTM enabled. */
        if (pwr_attrib->ltm_enable)
        {
            /* Calculate the BELT value for the new link state. */
            status = NU_USB_PMG_Calculate_BELT_Value(device,
                                                     &belt_value,
                                                     new_link_state);
            if (status == NU_SUCCESS)
            {
                /* Pass updated BELT value to the contorller driver. */
                status = NU_USB_HW_Update_BELT_Value(device->hw,
                                                     belt_value);
            }
        }

        else
        {
            status = NU_USB_DEVICE_LTM_DISABLED;
        }
    }

    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/*************************** end of file ********************************/

