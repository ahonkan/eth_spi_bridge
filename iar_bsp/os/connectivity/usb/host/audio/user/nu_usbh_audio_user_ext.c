/**************************************************************************
*
*               Copyright 2012  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************
***************************************************************************
*
* FILE NAME
*       nu_usbh_audio_user_ext.c
*
*
* COMPONENT
*       Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*       This file contains the external Interfaces exposed by audio user
*       driver.
*
* DATA STRUCTURES
*       None.
*
* FUNCTIONS
*       NU_USBH_AUD_USER_Create             Initializes the control block
*                                           of Audio user driver.
*       _NU_USBH_AUD_USER_Delete            Deletes an instance of Audio
*                                           user driver.
*       _NU_USBH_AUD_USER_Connect           Handles Audio Device's
*                                           connection.
*       _NU_USBH_AUD_USER_Disconnect        Handles audio device
*                                           disconnection.
*       NU_USBH_AUD_USER_Set_FU_Req         Sets the asked feature for
*                                           disconnection.
*       NU_USBH_AUD_USER_Get_FU_Req         Gets the asked feature from
*                                           disconnection.
*       NU_USBH_AUD_USER_Get_Vol_Stats      Gets the volume statistics.
*       NU_USBH_AUD_USER_Mute_Ctrl          Enables or disables mute
*                                           controls
*       NU_USBH_AUD_USER_Adjust_Vol         Increase or decreases the
*                                           volume.
*       NU_USBH_AUD_USER_Get_Aud_Funcs      Get functions supported by the
*                                           device.
*       NU_USBH_AUD_Get_Supported_Ctrls     Return supported controls
*                                           associated with a channel for a
*                                           particular Feature Unit.
*       NU_USBH_AUD_USER_Init               NMI initialization Routine.
*       NU_USBH_AUD_USER_GetHandle          Getter function for class
*                                           driver handle.
*       NU_USBH_AUD_USER_Register_Cb        Register connection and
*                                           disconnection callback function
*                                           pointers.
*
* DEPENDENCIES
*     nu_usb.h                             All USB definitions
*
**************************************************************************/

#ifndef NU_USBH_AUD_USER_EXT_C
#define NU_USBH_AUD_USER_EXT_C
/* ==============  USB Include Files ==================================  */
#include "connectivity/nu_usb.h"
#include "nu_usbh_audio_user_ext.h"

/* ==========================  Functions =============================== */

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Create
*
* DESCRIPTION
*     Audio user driver initialization routine.
*
* INPUTS
*     pcb_user_drvr        Pointer to driver control block.
*     p_name               Name of this USB object.
*     Discon_Handler       Function pointer to call on disconnection event.
*     p_memory_pool        Memory pool to be used by user driver.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion.
*     NU_INVALID_SEMAPHORE Indicates control block is invalid.
*     NU_INVALID_GROUP     Indicates control block is invalid.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Create (NU_USBH_AUD_USER       *pcb_user_drvr,
                                CHAR                   *p_name,
                                NU_AUDH_USER_CONN       Conn_Handler,
                                NU_AUDH_USER_DISCON     Discon_Handler,
                                NU_MEMORY_POOL         *p_memory_pool)
{

    STATUS status;
    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    /* Create base component. */
    status = _NU_USBH_USER_Create(
                 (NU_USBH_USER*)pcb_user_drvr,
                 p_name,
                 p_memory_pool,
                 NU_AUDH_AC_SUBCLASS,
                 0x00,
                 (NU_USBH_AUD_USER_DISPATCH*)&usbh_audio_user_dispatch);

    /* Copies a call back routine pointer to report device's
     * disconnection.
     */
    pcb_user_drvr->Disconect_Handler = Discon_Handler;

    /* Copies a call back routine pointer to report device's
     * connection.
     */
    pcb_user_drvr->Connect_Handler = Conn_Handler;

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     _NU_USBH_AUD_USER_Delete
*
* DESCRIPTION
*     This function deletes an instance of Audio user driver.
*
* INPUTS
*     pcb_user_drvr       Pointer to the audio user driver's control block.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBH_AUD_USER_Delete (VOID* pcb_user_drvr)
{

    STATUS status;
    NU_USBH_AUD_USER* pcb_aud_usr_drvr = (NU_USBH_AUD_USER*)pcb_user_drvr;

    NU_USBH_AUD_USER_DEV* pcb_next_device;
    NU_USBH_AUD_USER_DEV* pcb_curr_device;

    /* Pointer to the first device. */
    pcb_curr_device = (NU_USBH_AUD_USER_DEV*)
            (pcb_aud_usr_drvr->pcb_first_device);
    while (pcb_curr_device)
    {
        pcb_next_device =(NU_USBH_AUD_USER_DEV*)
        (pcb_curr_device->node.cs_next);

        /* Sends disconnect event, for each audio device. */
         _NU_USBH_AUD_USER_Disconnect(
                                 (NU_USB_USER*) pcb_aud_usr_drvr,
                                 (NU_USB_DRVR*)pcb_curr_device->class_drvr,
                                 pcb_curr_device);

        /* All devices delisted from the stack. */
        if(pcb_curr_device == pcb_next_device)
        {
           break;
        }
        pcb_curr_device = pcb_next_device;
    }

    /* Deleting the base component. */
    status = _NU_USBH_DRVR_Delete (pcb_user_drvr);
    return status;

}

/**************************************************************************
* FUNCTION
*     _NU_USBH_AUD_USER_Connect
*
* DESCRIPTION
*     This function is called by the class driver whenever there is an audio
*     device connected to USB host.
*
* INPUTS
*     pcb_user           Pointer to control block of user driver.
*     pcb_drvr           Pointer to control block of class driver.
*     audio_dev          Pointer to audio device's control block.
*
* OUTPUTS
*     NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBH_AUD_USER_Connect( NU_USB_USER  *pcb_user,
                                  NU_USB_DRVR  *pcb_drvr,
                                  VOID         *audio_dev)
{

    STATUS                 status;
    NU_USBH_AUD_USER      *pcb_aud_drvr = (NU_USBH_AUD_USER*)pcb_user;
    NU_USBH_AUD_USER_DEV  *pcb_aud_usr_dev;

    /* Allocates memory for the user device. */
    status = USB_Allocate_Memory (
    		       USB_MEM_TYPE_CACHED,
    		       sizeof (NU_USBH_AUD_USER_DEV),
                  (VOID **) &pcb_aud_usr_dev);

    /* If memory allocation is successful. */
    if(status == NU_SUCCESS)
    {
        memset (pcb_aud_usr_dev,
                0x00,
                sizeof (NU_USBH_AUD_USER_DEV));

        /* Initializes different data entries to be used later. */
        pcb_aud_usr_dev->user_drvr   = pcb_user;
        pcb_aud_usr_dev->class_drvr  = pcb_drvr;
        pcb_aud_usr_dev->audio_dev  = (NU_USBH_AUD_DEV*)audio_dev;

        /* Initialize supported function values. */
        pcb_aud_usr_dev->supported_fnc.microphone = NU_FALSE;
        pcb_aud_usr_dev->supported_fnc.speaker = NU_FALSE;

        /* Finds record function, if present in device. */
        status = NU_AUDH_Find_Record_Function(
                                   pcb_aud_usr_dev->audio_dev,
                                   &(pcb_aud_usr_dev->mphone));
        if(status == NU_SUCCESS)
        {
            pcb_aud_usr_dev->supported_fnc.microphone = NU_TRUE;
        }

        /* Finds play function, if present in device. */
        status = NU_AUDH_Find_Play_Function(
                                   pcb_aud_usr_dev->audio_dev,
                                   &(pcb_aud_usr_dev->speaker));
        /* If play function is present? */
        if(status == NU_SUCCESS)
        {
            pcb_aud_usr_dev->supported_fnc.speaker = NU_TRUE;
        }

        /* Add device to the list. */
        CSC_Place_On_List ((CS_NODE **) &(pcb_aud_drvr->pcb_first_device),
                           (CS_NODE *) pcb_aud_usr_dev);

        /* Sending connection notification to application through
         * base driver.
         */
         status = _NU_USBH_USER_Connect(pcb_user,
                                        pcb_drvr,
                                        pcb_aud_usr_dev);

        if(status == NU_SUCCESS)
        {
            if(pcb_aud_drvr->Connect_Handler != NU_NULL)
            {
                /* Reporting application of connection. */
                pcb_aud_drvr->Connect_Handler(pcb_aud_usr_dev);
            }
            else
            {
                status =  NU_USB_SYS_Register_Device((void *)pcb_aud_usr_dev,
                                         NU_USBCOMPH_AUDIO);
            }
        }

    }
    return status;

}

/**************************************************************************
* FUNCTION
*     _NU_USBH_AUD_USER_Disconnect
*
* DESCRIPTION
*     This function is called by the class driver whenever audio device
*     is disconnected from USB host.
*
* INPUTS
*     pcb_user_drvr      Pointer to control block of user driver.
*     pcb_audio_drvr     Pointer to control block of class driver.
*     pcb_device         Pointer to disconnected device.
*
* OUTPUTS
*     NU_SUCCESS         Indicates successful completion.
*     NU_USB_INVLD_ARG   Some pointer became stale before call
*                        completion.
*
**************************************************************************/
STATUS _NU_USBH_AUD_USER_Disconnect(NU_USB_USER    *pcb_user_drvr,
                                    NU_USB_DRVR    *pcb_audio_drvr,
                                    VOID           *pcb_device)
{

    NU_USBH_AUD_USER     *pcb_aud_drvr = (NU_USBH_AUD_USER*)pcb_user_drvr;
    NU_USBH_AUD_USER_DEV *pcb_curr_device = pcb_aud_drvr->pcb_first_device;

    STATUS status = NU_USB_INVLD_ARG;

    /* Scan the List of devices and Cleanup the associated one. */
    while (pcb_curr_device)
    {

        /* If this device disconnected. */
        if (pcb_curr_device->audio_dev == pcb_device)
        {
            if(pcb_aud_drvr->Disconect_Handler != NU_NULL)
            {
                /* Reporting application of disconnection. */
                pcb_aud_drvr->Disconect_Handler(pcb_curr_device);
            }
            else
            {
                status = NU_USB_SYS_DeRegister_Device(pcb_curr_device,
                                                      NU_USBCOMPH_AUDIO);
            }

            /* Sends disconnection interrupt to base driver. */
            _NU_USBH_USER_Disconnect ((NU_USB_USER*)pcb_user_drvr,
                        (NU_USB_DRVR*)pcb_audio_drvr,
                         pcb_device);


            /* Remove the Audio Structure from the List... */
            CSC_Remove_From_List(
                           (CS_NODE **) & pcb_aud_drvr->pcb_first_device,
                           (CS_NODE *) pcb_curr_device);

            /* Deallocate audio user device structure. */
            status = NU_Deallocate_Memory (pcb_curr_device);
            status = NU_SUCCESS;
            break;
        }
        else
        {
            /* Search for the device present in the list. */
            if(pcb_curr_device == (NU_USBH_AUD_USER_DEV*)
                      pcb_curr_device->node.cs_next)
            {
                status = NU_USB_INVLD_ARG;
                break;
            }
            pcb_curr_device = (NU_USBH_AUD_USER_DEV*)
            pcb_curr_device->node.cs_next;
        }
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Set_FU_Req
*
* DESCRIPTION
*     This function sends the Feature Unit " SET " request for a
*     particular feature type(volume, mute etc.) and attribute(max, min etc.)
*
* INPUTS
*     aud_usr_dev         Pointer to Audio device.
*     function            Function operation i.e; speaker or microphone.
*                         NU_AUDH_USR_SPKR_FUNCTION:Speaker.
*                         NU_AUDH_USR_MIC_FUNCTION :Microphone.
*     attribute           Attribute Selection of a control like MAX,MIN,
*                         CURR or RES.
*     ch_type             Channel type like left, right or master channel.
*     set_value           Control value to set.
*
*     feature_ctrl        Feature control selection type like Volume,
*                         Base etc.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*     NU_USB_INVLD_ARG    Indicates that one or more args passed to this
*                         function are invalid.
*     NU_NOT_PRESENT      Indicates that the pipe is invalid.
*     NU_NO_MEMORY        Indicates failure of memory allocation.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Set_FU_Req(NU_USBH_AUD_USER_DEV *aud_usr_dev,
                                   UINT8                 function,
                                   UINT8                 ch_type,
                                   UINT16                set_value,
                                   UINT8                 attribute,
                                   UINT16                feature_ctrl)
{

    STATUS status;

    NU_USBH_AUD_CTRL_REQ  ac_req;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    /* Fill up audio control request structure. */
    ac_req.channel    = ch_type;

    /* Control selector value*/
    ac_req.ctrl_slctr = feature_ctrl;

    /* Request code for SET request. */
    ac_req.req        = (AUDH_CS_REQ_CODE)(0x07 & attribute);

    /* Value to be set. */
    ac_req.trans_buff = (VOID*)(&set_value);
    /* Id of the Feature Unit to which request is being sent. */
    ac_req.id         = NU_AUDH_INVALID_ID;

    /* Get feature unit id. */
    NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                           &ac_req.id,
                           function);
    if(ac_req.id != NU_AUDH_INVALID_ID)
    {
        /* Sending class specific Feature Unit "SET" request. */
        status = NU_USBH_AUD_Feature_Unit_Req( aud_usr_dev->audio_dev,
                                               &ac_req);
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Get_FU_Req
*
* DESCRIPTION
*     This function sends the Feature Unit " GET " request against a
*     particular feature type(volume, mute etc.) and attribute(max, min ,
*     curr, res).
*
* INPUTS
*     aud_usr_dev         Pointer to the Audio device.
*     function            Function operation i.e speaker or microphone.
*                         NU_AUDH_USR_SPKR_FUNCTION:Speaker.
*                         NU_AUDH_USR_MIC_FUNCTION :Microphone.
*     ch_type             Channel index.
*     get_value           Variable to hold feature value.
*     attribute           Selection of MAX,MIN or CURR value of the feature.
*     feature_ctrl        Feature selection identifier.e.g. Volume, Base.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Get_FU_Req(NU_USBH_AUD_USER_DEV *aud_usr_dev,
                                   UINT8                 function,
                                   UINT8                 ch_type,
                                   UINT16               *get_value,
                                   UINT8                 attribute,
                                   UINT16                feature_ctrl)
{

    STATUS      status;

    NU_USBH_AUD_CTRL_REQ ac_req;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    ac_req.channel    = ch_type;
    ac_req.ctrl_slctr = feature_ctrl;
    ac_req.req        =(AUDH_CS_REQ_CODE)(0x87 & attribute);
    *get_value        =  0x0000;
    ac_req.trans_buff = (UINT8*)get_value;
    ac_req.id         = NU_AUDH_INVALID_ID;

    /* Get feature unit id. */
    NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                           &ac_req.id,
                           function);

    if( ac_req.id != NU_AUDH_INVALID_ID)
    {
        /* Sending class specific Feature unit "GET" request. */
        status = NU_USBH_AUD_Feature_Unit_Req(aud_usr_dev->audio_dev,
                                              &ac_req);
        if(status == NU_SUCCESS)
        {
            /* Converting data into a 16 bit variable. */
            *get_value   = HOST_2_LE16(*((UINT16*)ac_req.trans_buff));

        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Get_Vol_Statistics
*
* DESCRIPTION
*     This function returns the volume attributes related to a particular
*     feature unit. It returns min, max, curr and resolution of volume in
*     vol_info structure.
*
* INPUTS
*     aud_usr_dev       Pointer to audio device.
*     function          Target functionality of microphone or speaker.
*                       NU_AUDH_USR_SPKR_FUNCTION:Speaker.
*                       NU_AUDH_USR_MIC_FUNCTION :Microphone.
*     ch_type           Channel number for which volume is inquired.
*     vol_info          Complete volume info for one particular channel.
*
* OUTPUTS
*     NU_SUCCESS        Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Get_Vol_Stats(
            NU_USBH_AUD_USER_DEV          *aud_usr_dev,
            UINT8                          function,
            UINT8                          ch_type,
            NU_USBH_AUD_USR_FEATURE_INFO  *vol_info)
{

    STATUS      status;

    NU_USBH_AUD_CTRL_REQ ac_req;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    ac_req.channel    = ch_type;
    ac_req.ctrl_slctr = NU_AUDH_VOLUME_CONTROL;
    ac_req.id = NU_AUDH_INVALID_ID;
        
    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                sizeof(UINT16),
                                (VOID **) &(ac_req.trans_buff));

    if (status == NU_SUCCESS)
    {
        /* Get feature unit id. */
        NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                              &ac_req.id,
                              function);

        if(ac_req.id != NU_AUDH_INVALID_ID)
        {
            /* Sending class specific request for minimum volume. */
            ac_req.req  = AUDH_GET_MIN;
            status = NU_USBH_AUD_Feature_Unit_Req( aud_usr_dev->audio_dev,
                                                   &ac_req);
            if(status == NU_SUCCESS)
            {
                vol_info->min_attribute =
                LE16_2_HOST(*((UINT16*)ac_req.trans_buff));

                /* Sending class specific request for maximum volume. */
                ac_req.req  = AUDH_GET_MAX;
                status = NU_USBH_AUD_Feature_Unit_Req( aud_usr_dev->audio_dev,
                                                       &ac_req);
                if(status == NU_SUCCESS)
                {
                    vol_info->max_attribute = LE16_2_HOST(*((UINT16*)ac_req.trans_buff));

                    /* Sending class specific request for current volume. */
                    ac_req.req  = AUDH_GET_CUR;
                    status = NU_USBH_AUD_Feature_Unit_Req(
                                                       aud_usr_dev->audio_dev,
                                                       &ac_req);
                    if(status == NU_SUCCESS)
                    {
                        vol_info->curr_attribute = LE16_2_HOST(*((UINT16*)ac_req.trans_buff));

                        /* Sending class specific request for volume resolution. */
                        ac_req.req  = AUDH_GET_RES;
                        status = NU_USBH_AUD_Feature_Unit_Req(
                                                        aud_usr_dev->audio_dev,
                                                       &ac_req);
                        if(status == NU_SUCCESS)
                        {
                            vol_info->res_attribute = LE16_2_HOST(*((UINT16*)ac_req.trans_buff));
                        }
                    }
                }
            }
        }
        status = USB_Deallocate_Memory(ac_req.trans_buff);
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Mute_Ctrl
*
* DESCRIPTION
*     This function sends enable or disable mute control request for
*     a particular Feature Unit.
*
* INPUTS
*     aud_usr_dev         Pointer to audio device.
*     function            Speaker or microphone functionality sector.
*                         NU_AUDH_USR_SPKR_FUNCTION:Speaker.
*                         NU_AUDH_USR_MIC_FUNCTION :Microphone.
*     ch_type             Channel index thats mute control needs to be
*                         enabled or disabled.
*     mute                NU_TRUE : Enable mute control.
*                         NU_FALSE: Disable mute control.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Mute_Ctrl(
           NU_USBH_AUD_USER_DEV *aud_usr_dev,
           UINT8                 function,
           UINT8                 ch_type,
           BOOLEAN               mute)
{

    STATUS      status;

    NU_USBH_AUD_CTRL_REQ ac_req;

    UINT8  bmute;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    bmute = mute;

    ac_req.channel    = ch_type;
    ac_req.ctrl_slctr = NU_AUDH_MUTE_CONTROL;
    ac_req.req        = AUDH_SET_CUR;
    ac_req.trans_buff = &bmute;
    ac_req.id         = NU_AUDH_INVALID_ID;

    /* Get feature unit id. */
    NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                           &ac_req.id,
                           function);
    /* Sending class specific request Feature Unit Request for Mute
     * Control.
     */
    if(ac_req.id != NU_AUDH_INVALID_ID)
    {
        status = NU_USBH_AUD_Feature_Unit_Req(aud_usr_dev->audio_dev,
                                              &ac_req);
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Adjust_Vol
*
* DESCRIPTION
*     This function sets current volume of the desired Feature Unit in an
*     Audio device.
*
* INPUTS
*     aud_usr_dev         Pointer to audio device.
*     function            Speaker or microphone functionality sector.
*                         NU_AUDH_USR_SPKR_FUNCTION:Speaker.
*                         NU_AUDH_USR_MIC_FUNCTION :Microphone.
*     ch_type             Channel type thats mute control needs to be
*                         enabled or disabled.
*     vol_value           Volume level to set.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Adjust_Vol(NU_USBH_AUD_USER_DEV *aud_usr_dev,
                                   UINT8                 function,
                                   UINT8                 ch_type,
                                   UINT16                vol_value)
{

    STATUS status;

    NU_USBH_AUD_CTRL_REQ  ac_req;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    /* Control Selector. */
    ac_req.ctrl_slctr = NU_AUDH_VOLUME_CONTROL;

    /* Master, Left or Right channel. */
    ac_req.channel    = ch_type;
    ac_req.req        = AUDH_SET_CUR;
    ac_req.id         = NU_AUDH_INVALID_ID;
    vol_value         = HOST_2_LE16(vol_value);

    /* Get feature unit id. */
    NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                           &ac_req.id,
                           function);

    ac_req.trans_buff = (VOID*)(&vol_value);

    /* Sending class specific Feature Unit Request for Volume
     * Control.
     */
    if(ac_req.id != NU_AUDH_INVALID_ID)
    {
        status = NU_USBH_AUD_Feature_Unit_Req( aud_usr_dev->audio_dev,
                                               &ac_req);
    }
    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_USER_Get_Aud_Funcs
*
* DESCRIPTION
*     This function informs the user about functions present in Audio device.
*     Device can support speaker and microphone.
*
* INPUTS
*     aud_usr_dev             Pointer to audio user device control block.
*     function                Pointer to structure having info about
*                             supported functions.
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Get_Aud_Funcs(
           NU_USBH_AUD_USER_DEV      *aud_usr_dev,
           NU_USBH_AUD_USR_FUNCTIONS *functions)
{
    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    functions->microphone = aud_usr_dev->supported_fnc.microphone;
    functions->speaker    = aud_usr_dev->supported_fnc.speaker;

    /* Revert to user mode. */
    NU_USER_MODE();
    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Get_Supported_Features
*
* DESCRIPTION
*     This function returns supported features against a particular channel
*     index in 'bitmap'.
*
* INPUTS
*     aud_usr_dev             Pointer to the audio device Host driver.
*     function                Speaker function or Microphone function.
*     channel                 Channel number.
*     bitmap                  Bitmap representing which controls are
*                             available.
*
*  OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_Get_Supported_Ctrls(
           NU_USBH_AUD_USER_DEV *aud_usr_dev,
           UINT8                 function,
           UINT8                 channel,
           UINT16               *bitmap)
{

    STATUS               status;
    UINT8                ctrl_size;
    UINT8               fu_id;
    UINT8               *avail_ctrls;
    NU_USBH_AUD_ENTITY  *ent_info;
    NU_USBH_AUD_FEATURE *feature_info;
    NU_USBH_AUD_DEV     *audio_dev;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_AUDH_FU_NOT_PRESENT;

    fu_id = NU_AUDH_INVALID_ID;

    audio_dev = aud_usr_dev->audio_dev;

    /* Get feature unit id. */
    NU_AUDH_USER_Get_FU_Id(aud_usr_dev,
                           &fu_id,
                           function);

    if( fu_id != NU_AUDH_INVALID_ID )
    {
        /* Get Entity info. */
        ent_info = audio_dev->ac_cs_info->ent_id_info[fu_id];

        /* Get Feature Unit specific info. */
        feature_info = (NU_USBH_AUD_FEATURE*)ent_info->ent_tp_spec_info;

        /* Control size. */
        ctrl_size = feature_info->ctrl_size;

        /* Fill in available controls in bitmap. */
        if(ctrl_size == 1)
        {
            /* List of available controls. */
            avail_ctrls = (UINT8*)feature_info->ctrls_list + channel;
            *bitmap     = (UINT16)(*avail_ctrls);
        }
        else
        {

            bitmap = (UINT16*)feature_info->ctrls_list + channel * ctrl_size;
        }
        status = NU_SUCCESS;
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
*   FUNCTION
*
*       NU_USBH_AUD_USER_Init
*
*   DESCRIPTION
*
*       This function initializes the audio User driver component.
*
*   INPUTS
*
*       pSystem_Memory                           Cached Memory Pool.
*       pUncached_System_Memory             Uncached Memory Pool.
*
*
*
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_Init(NU_MEMORY_POOL* pSystem_Memory, NU_MEMORY_POOL* pUncached_System_Memory)
{
    STATUS  status;
    VOID *usbh_audio_handle;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(pSystem_Memory);
    NU_USB_MEMPOOLCHK_RETURN(pUncached_System_Memory);

    /* Allocate memory for audio user driver control block. */
    status = USB_Allocate_Object(sizeof(NU_USBH_AUD_USER),
                                 (VOID **)&NU_USBH_AUD_USER_Cb_Pt);

    if(status == NU_SUCCESS)
    {
        /* Zero out allocated block. */
        memset(NU_USBH_AUD_USER_Cb_Pt, 0, sizeof(NU_USBH_AUD_USER));
        /* Create the device subsystem. */
        status = NU_USBH_AUD_USER_Create (NU_USBH_AUD_USER_Cb_Pt,
                                          "aud_usr",
                                          NU_NULL,
                                          NU_NULL,
                                          pSystem_Memory);
    }

    /*  Get the host audio class driver handle. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_AUD_GetHandle(&usbh_audio_handle);
    }

    /* Register the user driver to the class driver. */
    if (status == NU_SUCCESS)
    {
        status =
        NU_USBH_AUD_Register_User (
                                  (NU_USBH_AUDIO *)usbh_audio_handle,
                                  (NU_USBH_USER *)
                                   NU_USBH_AUD_USER_Cb_Pt);
    }
    if (status != NU_SUCCESS)
    {
        /* Clean up */
        if (NU_USBH_AUD_USER_Cb_Pt)
        {
            status = USB_Deallocate_Memory(NU_USBH_AUD_USER_Cb_Pt);
            NU_USBH_AUD_USER_Cb_Pt = NU_NULL;
        }
    }
  
    /* Switch back to user mode. */
    NU_USER_MODE();
    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_AUD_USER_DeInit
*
*   DESCRIPTION
*
*       This function de-initializes the Audio User driver component via cleanup.
*
*   INPUTS
*
*       cb                VOID pointer to audio user driver control block.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS NU_USBH_AUD_USER_DeInit(VOID *cb)
{
    STATUS  status;
    VOID *usbh_aud_handle;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = NU_USBH_AUD_GetHandle(&usbh_aud_handle);

    if (status == NU_SUCCESS)
    {
        NU_USB_DRVR_Deregister_User (usbh_aud_handle,(NU_USB_USER*)(cb));
        
        _NU_USBH_AUD_USER_Delete((NU_USB_USER*)(cb));

        USB_Deallocate_Memory(NU_USBH_AUD_USER_Cb_Pt);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*   FUNCTION
*
*       NU_USBH_AUD_USER_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host audio
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host audio
*                           user driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
**************************************************************************/
STATUS NU_USBH_AUD_USER_GetHandle(VOID  **handle)
{
    STATUS status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    *handle = NU_USBH_AUD_USER_Cb_Pt;
    if (NU_USBH_AUD_USER_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
*   FUNCTION
*
*       NU_USBH_AUD_USER_Register_Cb
*
*   DESCRIPTION
*
*       This function is called to register connection and disconnection
*       callback functions with audio host user driver.
*
*   INPUTS
*
*       Conn_Handler                        Pointer to connection callback
*                                           function.
*       Discon_Handler                      Pointer to disconnection
*                                           callback function.
*
*   OUTPUTS
*
*       NU_SUCCESS                          Indicates there exists a host
*                                           audio user driver.
*       NU_INVALID_POINTER                  Indicate that driver has not
*                                           been initialized.
*
**************************************************************************/

STATUS   NU_USBH_AUD_USER_Register_Cb (NU_USBH_AUD_USER *cb,
                                       NU_AUDH_USER_CONN Conn_Handler,
                                       NU_AUDH_USER_DISCON Discon_Handler)
{
    STATUS status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if(cb == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }

    if(status == NU_SUCCESS)
    {
        /*
         * Copies a call back routine pointer to report device's
         * disconnection.
         */
        cb->Disconect_Handler = Discon_Handler;

        /*
         * Copies a call back routine pointer to report device's
         * disconnection.
         */
        cb->Connect_Handler = Conn_Handler;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_host_audio_user_init
*
*   DESCRIPTION
*
*       This function initializes the audio user drivers.
*
*   INPUTS
*
*       key                                 Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_host_audio_user_init (const CHAR * key, int startstop)
{
    STATUS status;

    if(startstop)
    {
        /* In following API call, passing memory pool ptr parameters
         * NU_NULL because in ReadyStart memory in USB system is
         * allocated through USB specific memory APIs, not directly
         * with any given memory pool pointer. These parameter remain
         * only for backwards code compatibility. */

        status = NU_USBH_AUD_USER_Init(NU_NULL, NU_NULL);
    }
    else
    {
        if (NU_USBH_AUD_USER_Cb_Pt)
        {
            status = NU_USBH_AUD_USER_DeInit((VOID *)NU_USBH_AUD_USER_Cb_Pt);
        }

        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_AUDIO_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the audio device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS                          
*
*
**************************************************************************/
STATUS  NU_USBH_AUDIO_DM_Open (VOID *dev_handle)
{
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_AUDIO_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the audio device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_AUDIO_DM_Close (VOID* dev_handle)
{
    STATUS            status = NU_SUCCESS;
    NU_USBH_AUD_DEV*  pcb_aud_dev;
    NU_USBH_AUD_USER* pcb_aud_drvr;

    NU_USB_PTRCHK(dev_handle);

    pcb_aud_dev = (NU_USBH_AUD_DEV*)dev_handle;
    pcb_aud_drvr = (NU_USBH_AUD_USER *) pcb_aud_dev->pcb_user_drvr;

    /* Update application about disconnection. */
    if(pcb_aud_drvr->Disconect_Handler)
    {
        pcb_aud_drvr->Disconect_Handler(pcb_aud_dev);        
    }

    return (status);

}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_AUDIO_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       session_handle   Pointer to the audio device passed as context.
*       buffer                Pointer to memory location where to put the read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain offset in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_AUDIO_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr)
{
    
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_AUDIO_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       session_handle   Pointer to the audio device passed as context.
*       buffer                Pointer to memory location where data to be written is available.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_AUDIO_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr)
{
    
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_AUDIO_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform
*       a control operation on the device.
*
* INPUTS
*
*       session_handle Pointer to the audio device passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       STATUS        status               
*
*************************************************************************/
STATUS  NU_USBH_AUDIO_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length)
{
    NU_USBH_AUD_USER_DEV           *device_ptr;
    STATUS                         status = NU_SUCCESS;
    NU_USBH_AUD_Data_Callback      callback;
    NU_AUDIO_OP_PARAM              *params;
    NU_AUDIO_DEVICE_SETTINGS       *device_settings;
    NU_AUDIO_USER_DEVICE_SETTINGS  *user_device_settings;
    NU_USBH_AUDIO_NOTIFY_CALLBACKS *notify_callbacks;
	
    NU_USB_PTRCHK(session_handle);
    device_ptr = (NU_USBH_AUD_USER_DEV*) session_handle;

    switch(cmd)
    {
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_OPEN_PLAY_SESSION):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Open_Play_Session(device_settings->aud_dev, device_settings->channels, device_settings->sample_size, device_settings->sampling_rate);
        break;
		
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_CLOSE_PLAY_SESSION):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Close_Play_Session(device_settings->aud_dev);
        break;

        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_OPEN_RECORD_SESSION):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Open_Record_Session(device_settings->aud_dev, device_settings->channels, device_settings->sample_size, device_settings->sampling_rate, device_settings->double_buffering);
        break;

        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_CLOSE_RECORD_SESSION):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Close_Record_Session(device_settings->aud_dev);
        break;

        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_REGISTER_RECORD_CALLBACK):
            callback = (NU_USBH_AUD_Data_Callback)data;
            device_ptr->audio_dev->cb_record_inform.call_back = callback;
        break;
	   
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_REGISTER_PLAY_CALLBACK):
            callback = (NU_USBH_AUD_Data_Callback)data;
            device_ptr->audio_dev->cb_play_inform.call_back = callback;
        break;
		
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_PLAY_SOUND):
            params = (NU_AUDIO_OP_PARAM*)data;
            status = NU_USBH_AUD_Play_Sound(params->aud_dev, params->buffer, params->samples, device_ptr->audio_dev->cb_play_inform.call_back); 
        break;
		
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_RECORD_SOUND):
            params = (NU_AUDIO_OP_PARAM*)data;
            status = NU_USBH_AUD_Record_Sound(params->aud_dev, params->buffer, params->max_samples, device_ptr->audio_dev->cb_record_inform.call_back); 
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_GET_FUNC_COUNT):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Get_Function_Count(device_settings->aud_dev, device_settings->function, &(device_settings->func_count)); 
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_GET_FREQ_COUNT):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Get_Freqs_Count(device_settings->aud_dev, device_settings->function, device_settings->index, &(device_settings->freq_count)); 
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_GET_FUNC_SETTINGS):
            device_settings = (NU_AUDIO_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Get_Func_Settings(device_settings->aud_dev, 
                                                   device_settings->function, 
                                                   device_settings->index, 
                                                   &(device_settings->freq_type), 
                                                   device_settings->freq_list, 
                                                   &(device_settings->freq_count), 
                                                   &(device_settings->channels),
                                                   &(device_settings->sample_size),
                                                   &(device_settings->lock_delay),
                                                   &(device_settings->reserved_bits) ); 
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_REGISTER_NOTIFICATION_CALLBACKS):
            notify_callbacks= (NU_USBH_AUDIO_NOTIFY_CALLBACKS*)data;
            status = NU_USBH_AUD_USER_Register_Cb((NU_USBH_AUD_USER *)(device_ptr->user_drvr), notify_callbacks->connect_callback, notify_callbacks->disconnect_callback);
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_GET_VOL_STATS):
            user_device_settings = (NU_AUDIO_USER_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_USER_Get_Vol_Stats(user_device_settings->aud_user_dev,
        		                                    user_device_settings->function, 
        		                                    user_device_settings->ch_type,
        		                                    user_device_settings->vol_info); 
        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_MUTE_CTRL):
            user_device_settings= (NU_AUDIO_USER_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_USER_Mute_Ctrl(user_device_settings->aud_user_dev,
        		                                    user_device_settings->function, 
        		                                    user_device_settings->ch_type,
        		                                    user_device_settings->mute); 


        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_ADJUST_VOL):
            user_device_settings= (NU_AUDIO_USER_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_USER_Adjust_Vol(user_device_settings->aud_user_dev,
        		                                    user_device_settings->function, 
        		                                    user_device_settings->ch_type,
        		                                    user_device_settings->vol_val); 


        break;

        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_GET_AUD_FUNCS):
            user_device_settings= (NU_AUDIO_USER_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_USER_Get_Aud_Funcs(user_device_settings->aud_user_dev,
        		                                    user_device_settings->functions); 


        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_GET_SUPPORTED_CTRLS):
            user_device_settings= (NU_AUDIO_USER_DEVICE_SETTINGS*)data;
            status = NU_USBH_AUD_Get_Supported_Ctrls(user_device_settings->aud_user_dev,
                                                    user_device_settings->function,
                                                    user_device_settings->ch_type,
                                                    user_device_settings->bitmap); 



        break;
        case (USB_AUDIO_IOCTL_BASE + USBH_AUDIO_USER_GET_DEV_CB):
            *(void **)data = (VOID *)device_ptr;
            status = NU_SUCCESS;
        break;

        default:
            status = DV_IOCTL_INVALID_CMD;
        break;
    }

    return (status);
}


#endif /*_NU_USBH_AUD_USER_EXT_C*/
/* ======================  End Of File  ===============================. */
