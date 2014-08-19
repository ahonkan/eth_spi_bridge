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
* FILE NAME
*     nu_usbh_audio_ext.c
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains the implementation for different API services
*     provided by audio class driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*       nu_os_conn_usb_host_audio_user_init Registry Initialization
*                                           function.
*       NU_USBH_AUD_Create                  Creates Host AUDIO Class
*                                           Driver.
*       _NU_USBH_AUD_Delete                 Deletes an AUDIO Class Driver.
*       _NU_USBH_AUD_Initialize_Intf        Connect callback routine.
*       _NU_USBH_AUD_Disconnect             Disconnect callback routine.
*       NU_USBH_AUD_Is_Device_Capable       Device capable of playing/
*                                           recording.
*       NU_USBH_AUD_Feature_Unit_Req        Feature Unit Control request.
*       NU_USBH_AUD_Sample_Freq_Req         SET/GET Sampling Frequency
*                                           Request.
*       NU_USBH_AUD_Pitch_Ctrl_Req          SET/GET Pitch Control Request.
*       NU_USBH_AUD_Open_Play_Session       Creates environment for OUT
*                                           transfer scheduling.
*       NU_USBH_AUD_Close_Play_Session      Deletes the session created for
*                                           NU_USBH_AUD_Open_Play_Session
*                                           API.
*       NU_USBH_AUD_Open_Record_Session     Creates environment for IN
*                                           transfer scheduling.
*       NU_USBH_AUD_Close_Record_Session    Deletes the session created for
*                                           NU_USBH_AUD_Open_Record_Session
*                                           API.
*       NU_USBH_AUD_Play_Sound              Plays sound on attached Device.
*       NU_USBH_AUD_Record_Sound            Records sound from Audio
*                                           Device.
*       NU_AUDH_Find_Record_Function        Finds play function, if it is
*                                           present in an audio device.
*       NU_AUDH_Find_Play_Function          Finds recoding function, if it
*                                           is present in an audio device.
*       NU_USBH_AUD_Register_User           Registers the user to class
*                                           driver.
*       NU_USBH_AUD_GetHandle               Getter function for class
*                                           driver handle.
*       NU_USBH_AUD_Get_Function_Count      Getter function for number of
*                                           playback or recording
*                                           functions available in audio
*                                           device.
*       NU_USBH_AUD_Get_Freqs_Count         Getter function for number of
*                                           supported frequencies a
*                                           particular playback or
*                                           recording function of audio
*                                           device.
*       NU_USBH_AUD_Get_Func_Settings       Getter function for settings
*                                           supported by a particular
*                                           playback or recording function
*                                           of an audio device.
*
* DEPENDENCIES
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef NU_USBH_AUD_EXT_C
#define NU_USBH_AUD_EXT_C

/* ====================  USB Include Files  ============================ */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"
#include "nu_usbh_audio_ext.h"

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_USBH_AUDIO *NU_USBH_Audio_Cb_Pt;
extern STATUS nu_os_conn_usb_host_audio_user_init (const CHAR * key, int startstop);

/* Functions. */
/**************************************************************************
*
* FUNCTION
*
*       nu_os_conn_usb_host_audio_class_init
*
* DESCRIPTION
*
*       Audio Driver initialization routine.
*
* INPUTS
*
*       path                                Registry path of component.
*       compctrl                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization.
*       NU_USB_INVLD_ARG                    Indicates that parameters are
*                                           NU_NULL.
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid.
*       NU_INVALID_SIZE                     Indicates the size is larger
*                                           than the pool.
*       NU_NO_MEMORY                        Memory not available.
*
**************************************************************************/
STATUS nu_os_conn_usb_host_audio_class_init(CHAR *path, INT compctrl)
{
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8   rollback = 0;
    NU_USB_STACK * stack_handle;

    if (compctrl == RUNLEVEL_START)
    {        
        /* Allocate memory for USB host audio. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                    sizeof(NU_USBH_AUDIO),(void **)&NU_USBH_Audio_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBH_Audio_Cb_Pt, 0, sizeof(NU_USBH_AUDIO));
            status = NU_USBH_AUD_Create (NU_USBH_Audio_Cb_Pt,
                                        "AUD",
                                         NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the host stack handle */
        if(!rollback)
        {
            status = NU_USBH_Init_GetHandle ((VOID*)&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if(!rollback)
        {
            status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) stack_handle,
                                                 (NU_USB_DRVR *) NU_USBH_Audio_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }
        if (status == NU_SUCCESS)
        {
            /* Create & Register user. */
            internal_sts = nu_os_conn_usb_host_audio_user_init(path, compctrl);
        }
        else
        {
            switch (rollback)
            {
                case 0x03:
                    internal_sts = _NU_USBH_AUD_Delete((void*) NU_USBH_Audio_Cb_Pt);
                    NU_USB_ASSERT( internal_sts == NU_SUCCESS );

                case 0x02:
                    if(NU_USBH_Audio_Cb_Pt)
                    {
                        internal_sts = USB_Deallocate_Memory(NU_USBH_Audio_Cb_Pt);
                        NU_USB_ASSERT( internal_sts == NU_SUCCESS );
                        NU_USBH_Audio_Cb_Pt = NU_NULL;
                    }
                
                case 1:
                case 0:
                    NU_UNUSED_PARAM(internal_sts);

            }
        }
    }
    else if(compctrl == RUNLEVEL_STOP)
    {
        nu_os_conn_usb_host_audio_user_init(path, compctrl);

        status = NU_USBH_Init_GetHandle ((VOID*)&stack_handle);

        if (status == NU_SUCCESS)
        {
            _NU_USB_STACK_Deregister_Drvr ( stack_handle,
                                            (NU_USB_DRVR *)NU_USBH_Audio_Cb_Pt);
        }
        status = _NU_USBH_AUD_Delete((void*)NU_USBH_Audio_Cb_Pt);
                
        if (status == NU_SUCCESS)
        {
            status = USB_Deallocate_Memory(NU_USBH_Audio_Cb_Pt);
        }
    }

    return (internal_sts != NU_SUCCESS ? internal_sts : status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Create
*
* DESCRIPTION
*     This function is the Nucleus USB AUDIO Class Driver initialization
*     routine for host side. It initializes the AUDIO Class Driver and
*     makes it ready to  be registered with the host stack.
*
* INPUTS
*     pcb_audio_drvr       Pointer to Nucleus USB AUDIO Class Driver's
*                          control block.
*     p_name               Name of Nucleus USB AUDIO  Class Driver.
*     p_mem_pool           Pointer to Memory Pool available to the driver.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion of the service.
*     NU_USB_INVLD_ARG     Indicates that the match_flag is not well formed.
*     NU_USB_NOT_PRESENT   Indicates that the maximum number of control
*                          blocks that could be created in the sub system
*                          has exceeded.
*     NU_INVALID_SEMAPHORE Indicates that the internally created semaphore
*                          pointer is invalid.
*     NU_SEMAPHORE_DELETED Indicates that the internally created semaphore
*                          was deleted while the task was suspended.
*     NU_UNAVAILABLE       Indicates that the internally created semaphore
*                          is unavailable.
*     NU_INVALID_SUSPEND   Indicates that this API is called from
*                          a non-task thread.
*
**************************************************************************/
STATUS NU_USBH_AUD_Create (NU_USBH_AUDIO    *pcb_audio_drvr,
                           CHAR             *p_name,
                           NU_MEMORY_POOL   *p_mem_pool)
{
    STATUS status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(pcb_audio_drvr);
    NU_USB_PTRCHK_RETURN(p_name);
    NU_USB_MEMPOOLCHK_RETURN(p_mem_pool);

    /* Initializing the Audio control block to 0x00 value. */
    memset(pcb_audio_drvr,
           0x00,
           sizeof(NU_USBH_AUDIO));

    /* Creating the base class component for AUDIO Interface. */
    status = _NU_USBH_DRVR_Create (
              (NU_USB_DRVR *)pcb_audio_drvr,
              p_name,
              (UINT32)(USB_MATCH_CLASS | USB_MATCH_SUB_CLASS),
              (UINT16)0,
              (UINT16)0,
              (UINT16)0,
              (UINT16)0,
              (UINT8)NU_AUDH_CLASS,
              (UINT8)NU_AUDH_AC_SUBCLASS,
              (UINT8)0,
              &usbh_audio_dispatch);
    if(status == NU_SUCCESS)
    {
         /* Assign memory pool pointer to the memory pool pointer for memory
          * related operations.
        */
        pcb_audio_drvr->memory_pool = p_mem_pool;
    }

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*    _NU_USBH_AUD_Delete
*
* DESCRIPTION
*     This function deletes a Nucleus USB AUDIO Class Driver. All the
*     interfaces claimed by this driver are given disconnect callback and
*     interfaces are released. Driver is also deregistered from the stack
*     it was registered before deletion. Note that this function does not
*     free the memory associated with the audio Driver control block.
*
* INPUTS
*     cb             Pointer to the audio class Host driver.
*
* OUTPUTS
*     NU_SUCCESS     Indicates successful completion.
*
**************************************************************************/
STATUS _NU_USBH_AUD_Delete (VOID *cb)
{
    STATUS status;

      /* Type cast the Control Block to Audio Class Driver. */
    NU_USBH_AUDIO   *pcb_audio_drvr = (NU_USBH_AUDIO*) cb;
    NU_USBH_AUD_DEV *p_next_device,
                    *p_curr_device = pcb_audio_drvr->pcb_first_device;

    /* For each connected Audio device. */
    while (p_curr_device)
    {
        p_next_device =(NU_USBH_AUD_DEV*) p_curr_device->node.cs_next;

        /* Send disconnect event, for each Audio interface. */
        status =_NU_USBH_AUD_Disconnect (
                                    (NU_USB_DRVR *) pcb_audio_drvr,
                                   p_curr_device->pcb_stack,
                                   p_curr_device->pcb_device);
        if (status == NU_SUCCESS)
        {
        /* All devices delisted from the stack. */
        if(p_curr_device == p_next_device)
        {
            break;
        }
        }
        /* Select next device. */
        p_curr_device = p_next_device;
    }

    /* Delete the instance of semaphore inside control block. */
    status = NU_Delete_Semaphore (&(pcb_audio_drvr->sm_aud_lock));

    /* Call Base Behavior. */
    return (_NU_USBH_DRVR_Delete (cb));

}

/**************************************************************************
* FUNCTION
*      _NU_USBH_AUD_Initialize_Intf
*
* DESCRIPTION
*     Connect callback function invoked by stack when a interface with
*     AUDIO class is found on a device.
*
* INPUTS
*     pcb_drvr             Pointer to Class Driver control block.
*     pcb_stack            Pointer to Stack control block of the calling
*                          stack.
*     pcb_device           Pointer to Device control block of the device
*                          found.
*     pcb_intf             Pointer to Interface control Block to be served
*                          by this class driver.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion.
*     NU_NOT_PRESENT       Indicates that no matching pipe could be found.
*     NU_USB_INVLD_ARG     Indicates that the control block is invalid.
*     NU_AUDH_MEM_FAIL     Memory allocation failed.
*
**************************************************************************/
STATUS _NU_USBH_AUD_Initialize_Intf(NU_USB_DRVR     *pcb_drvr,
                                    NU_USB_STACK    *pcb_stack,
                                    NU_USB_DEVICE   *pcb_device,
                                    NU_USB_INTF     *pcb_intf)
{

    UINT8              i, ep_count;
    UINT8              si_count;
    UINT8              alt_sttg, actual_alt_sttg;
    UINT8              intf_index, roll_back = NU_AUDH_NO_ROLLBACK;
    UINT8             *si_list;
    NU_USB_INTF       *as_intf;
    NU_USB_ALT_SETTG  *as_alt_sttg;
    AUDH_ASI_INFO     *asi_info;
    AUDH_CS_ASI_INFO  *cs_asi_info;
    NU_USBH_AUD_DEV   *pcb_curr_device;

    /* Pointer to the audio control block. */
    NU_USBH_AUDIO     *pcb_audio_drvr = (NU_USBH_AUDIO*) pcb_drvr;
    STATUS             status = NU_SUCCESS;
    STATUS             internal_sts = NU_SUCCESS;

    do
    {

        /* Allocate memory to Audio device control block structure. */
        status = USB_Allocate_Object (sizeof(NU_USBH_AUD_DEV),
                                      (VOID **) &pcb_curr_device);
        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
            break;
        }

        /* Initializing the Audio Device control block to 0x00 value. */
        memset(pcb_curr_device,0,sizeof(NU_USBH_AUD_DEV));

        /* Allocate memory for Control IRP. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USBH_CTRL_IRP),
                                    (VOID **)&(pcb_curr_device->ctrl_irp));
        if (status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_DEV_STRUCT;
            break;
        }

        /* Store in Audio Device control block. */
        pcb_curr_device->pcb_ac_intf  = pcb_intf;
        pcb_curr_device->pcb_aud_drvr = pcb_drvr;
        pcb_curr_device->pcb_stack    = pcb_stack;
        pcb_curr_device->pcb_device   = pcb_device;
        /* Find Alternate Setting for Audio Control Interface. */
        status = NU_USB_INTF_Find_Alt_Setting(
                               pcb_intf,
                               (UINT32)USB_MATCH_CLASS|USB_MATCH_SUB_CLASS,
                               (UINT8)0x00,
                               (UINT8)NU_AUDH_CLASS,
                               (UINT8)NU_AUDH_AC_SUBCLASS,
                               (UINT8)0x00,
                               &(pcb_curr_device->pcb_ac_alt_settg));
        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_DEV_STRUCT;
            break;
        }

        /* Setting the current Audio Control Interface as active. */
        status = NU_USB_ALT_SETTG_Set_Active(
                     pcb_curr_device->pcb_ac_alt_settg);

        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_DEV_STRUCT;
            break;
        }

        /* Find the Control pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe(
                                        pcb_curr_device->pcb_ac_alt_settg,
                                        (UINT32)USB_MATCH_EP_ADDRESS,
                                        (UINT8)0,
                                        (UINT8)0,
                                        (UINT8)0,
                                        &(pcb_curr_device->pcb_ctrl_pipe));
       if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_DEV_STRUCT;
            break;
        }
        /* Get Class specific descriptors for ACI. */
        status = NU_USB_ALT_SETTG_Get_Class_Desc(
                                     pcb_curr_device->pcb_ac_alt_settg,
                                     (UINT8**)&(pcb_curr_device->cs_descr),
                                     &(pcb_curr_device->len_cs_descr));

        if (status == NU_SUCCESS)
        {
        /* Parse all descriptors associated with Audio Control Interface.
         */
        status =  NU_AUDH_Parse_AC_Dscr(pcb_curr_device);
        }
        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_DEV_STRUCT;
            break;
        }

        si_count = pcb_curr_device->ac_cs_info->si_count;
        si_list = pcb_curr_device->ac_cs_info->si_list;

        /* Get current configuration. */
        status = NU_USB_INTF_Get_Cfg(pcb_intf,&pcb_curr_device->pcb_cfg);

        /* For Parsing Audio Streaming Interfaces: */
        for(i = 0x00; i< si_count ; i++)
        {
            if(roll_back != NU_AUDH_NO_ROLLBACK)
            {
                break;
            }

            /* Get Audio Streaming interface index from streaming
             * interface list.
             */
            intf_index = si_list[i];

            /* Get Pointer to the Audio streaming interface descriptor. */
            status = NU_USB_CFG_Get_Intf(pcb_curr_device->pcb_cfg,
                                intf_index,
                               &(as_intf));

            /* Go through all the alternate settings for this interface. */
            for(alt_sttg = 0; alt_sttg < NU_USB_MAX_ALT_SETTINGS;
                alt_sttg++)
            {
                /* Find ASI alternate setting. */
                status = NU_USB_INTF_Find_Alt_Setting(
                             as_intf,
                             (UINT32)USB_MATCH_CLASS|USB_MATCH_SUB_CLASS,
                             (UINT8)alt_sttg,
                             (UINT8)NU_AUDH_CLASS,
                             (UINT8)NU_AUDH_AS_SUBCLASS,
                             (UINT8)0x00,
                             &as_alt_sttg);
                if(status != NU_SUCCESS)
                {
                    continue;
                }
                /* Get bAlternateSetting field from interface descriptor.
                 */
                status = NU_USB_ALT_SETTG_Get_bAlternateSetting(
                                             as_alt_sttg,
                                             &actual_alt_sttg);

                /* This check is introduced because there are Audio devices
                 * those have alternate settings not in sequential order
                 * for a given interface. For example an interface has
                 * alternate 0 followed by alternate 2 and 4 instead of 1,2,
                 * 3,4.
                 */
                if(actual_alt_sttg != alt_sttg)
                {
                    continue;
                }

                /* Allocate memory for streaming interface list, if not
                 * yet allocated.
                 */
                if(!pcb_curr_device->si_info[intf_index])
                {
                    /* Allocate memory to Streaming interfaces info list. */
                    status = USB_Allocate_Memory(
                             USB_MEM_TYPE_CACHED,
                             (UNSIGNED)sizeof(AUDH_ASI_INFO),
                             (VOID**)&pcb_curr_device->si_info[intf_index]);
                    if(status != NU_SUCCESS)
                    {
                        status = NU_AUDH_MEM_FAIL;
                        roll_back = NU_AUDH_RELEASE_PARSE_MEM;
                        break;
                    }

                    memset(pcb_curr_device->si_info[intf_index],
                           0x00,
                           sizeof(AUDH_ASI_INFO));
                }
                /* Save this streaming interface info in local variable.*/
                asi_info = pcb_curr_device->si_info[intf_index];

                /* Get number of endpoint for this alternate setting. */
                status = NU_USB_ALT_SETTG_Get_Num_Endps(as_alt_sttg,
                                               &ep_count);

                /* No endpoint, means zero band width alternate setting. */
                if(ep_count == 0)
                {
                    asi_info->zbw_alt_sttg = as_alt_sttg;
                    asi_info->strm_inf     = as_intf;

                    /* Set ZBW alternate setting as active alternate
                     * setting.
                     */
                    status = NU_USB_ALT_SETTG_Set_Active (as_alt_sttg);
                    if(status != NU_SUCCESS)
                    {
                        roll_back = NU_AUDH_DEL_SI_INFO;
                        break;
                    }
                    continue;
                }

                /* Alternate Setting having some bandwidth. */
                asi_info->strm_inf  = as_intf;

                /* Allocate Memory to Class specific Audio Streaming
                 * Interface structure.
                 */
                status = USB_Allocate_Memory(
                         USB_MEM_TYPE_CACHED,
                         (UNSIGNED)(sizeof(AUDH_CS_ASI_INFO)),     
                                         (VOID**)&cs_asi_info);
                if(status != NU_SUCCESS)
                {
                   roll_back = NU_AUDH_DEL_SI_INFO;
                   break;
                }

                /* Save operational alternate setting. */
                cs_asi_info->op_alt_sttg  = as_alt_sttg;

                /* Save audio streaming info pointer. */
                cs_asi_info->asi_info     = asi_info;

                /* Parse Audio streaming interface descriptors. */
                status = NU_AUDH_Parse_Audio_Strm_Infs(cs_asi_info,
                                                       pcb_curr_device);

                /* If not successful deallocate class specific streaming
                 * info structure along with memory allocated so far.
                 */
                if(status != NU_SUCCESS)
                {
                    status = NU_AUDH_PARSE_SI_FAILED;
                    roll_back = NU_AUDH_DEL_CS_ASI;
                    break;
                }

                if(cs_asi_info->term_link_tp ==  AUDH_INPUT_TERMINAL)
                {
                    /* The terminal link type is Input Terminal,it means
                     * this alternate must have ISO OUT endpoint.
                     */
                    status = NU_USB_ALT_SETTG_Find_Pipe (
                                       as_alt_sttg,
                                       (UINT32) (USB_MATCH_EP_TYPE |
                                       USB_MATCH_EP_DIRECTION),
                                       (UINT8)0,
                                       (UINT8)0,
                                       (UINT8)1,
                                       &(cs_asi_info->pcb_iso_pipe ));
                     if(status != NU_SUCCESS)
                     {
                         roll_back = NU_AUDH_DEL_CS_ASI;
                         break;
                     }
                }
                if( cs_asi_info->term_link_tp == AUDH_OUTPUT_TERMINAL)
                {
                    /* The terminal link type is Output Terminal it means
                     * this alternate must have ISO IN endpoint.
                     */
                     status = NU_USB_ALT_SETTG_Find_Pipe (
                                   as_alt_sttg,
                                   (UINT32) (USB_MATCH_EP_TYPE |
                                   USB_MATCH_EP_DIRECTION),
                                   (UINT8)0,
                                   (UINT8)0x80,
                                   (UINT8)1,
                                   &(cs_asi_info->pcb_iso_pipe));
                     if(status != NU_SUCCESS)
                     {
                         roll_back = NU_AUDH_DEL_CS_ASI;
                         break;
                     }
                }

                asi_info->cs_asi_info[alt_sttg]      = cs_asi_info;
                pcb_curr_device->si_info[intf_index] = asi_info;

            }
        }

        if(roll_back != NU_AUDH_NO_ROLLBACK)
        {
            break;
        }

        /* High speed or Full speed device. */
        pcb_curr_device->bus_speed      = pcb_device->speed;

        /* Get user from the list of registered users. */
        pcb_curr_device->pcb_user_drvr  =
                                (NU_USB_USER*)pcb_audio_drvr->pcb_user_drvr;

        /* Create semaphore and event for USB transfers. */
        status = NU_AUDH_Initialize_Device(pcb_curr_device);
        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_CS_ASI;
            break;
        }

        /* Claim this interface. */
        status = NU_USB_INTF_Claim(pcb_intf, pcb_drvr);
        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_EVENTS;
            break;
        }

        /* Give user connect notification. */
        status = NU_USB_USER_Connect (
                     (NU_USB_USER*)pcb_audio_drvr->pcb_user_drvr,
                     (NU_USB_DRVR*)pcb_audio_drvr,
                      pcb_curr_device);
        if(status != NU_SUCCESS)
        {
            roll_back = NU_AUDH_DEL_EVENTS;
            break;
        }

        if(status == NU_SUCCESS)
        {
            status = NU_Create_Event_Group(&(pcb_curr_device->task_events),
                                            "AUDHTEVN");
        }

        /* Make a link list for Audio Device control block. */
        CSC_Place_On_List (
                (CS_NODE**)  &pcb_audio_drvr->pcb_first_device,
                (CS_NODE*)   pcb_curr_device);

    } while(0);

    switch (roll_back)
    {

        case NU_AUDH_DEL_EVENTS:

            /* Delete event group and semaphore created by Initialize
             * Device.
             */
            internal_sts = NU_Delete_Event_Group (
                                    &(pcb_curr_device->trans_events));
            internal_sts = NU_Delete_Semaphore (
                                    &(pcb_curr_device->sm_ctrl_trans));
            NU_USB_ASSERT( internal_sts == NU_SUCCESS );

        case NU_AUDH_DEL_CS_ASI:

            /* Release memory related to Class specific ASI info. */
             internal_sts = USB_Deallocate_Memory (&cs_asi_info);
             NU_USB_ASSERT( internal_sts == NU_SUCCESS );

        case NU_AUDH_DEL_SI_INFO:

            /* Release all the memory associated with si_info structure. */
            for( i =0; i< NU_AUDH_MAX_STREAMING_INFS; i++)
            {
                if(pcb_curr_device->si_info[i])
                {
                    internal_sts = USB_Deallocate_Memory(
                                    pcb_curr_device->si_info[i]);
                    NU_USB_ASSERT( internal_sts == NU_SUCCESS );
                }
            }
        case NU_AUDH_RELEASE_PARSE_MEM:

               /* Free all memory allocated by parsing routines. */
               internal_sts = NU_AUDH_Free_Structures(pcb_curr_device);
               NU_USB_ASSERT( internal_sts == NU_SUCCESS );

        case NU_AUDH_DEL_DEV_STRUCT:

            if(pcb_curr_device->ctrl_irp != NU_NULL)
            {
                internal_sts = USB_Deallocate_Memory (pcb_curr_device->ctrl_irp);
                NU_USB_ASSERT( internal_sts == NU_SUCCESS );
            }
          
            /* Release all the memory associated with Audio device
             * control block.
             */
            internal_sts = USB_Deallocate_Memory (pcb_curr_device);
            NU_USB_ASSERT( internal_sts == NU_SUCCESS );

        default:
            NU_UNUSED_PARAM(internal_sts);
    }
    return (internal_sts != NU_SUCCESS ? internal_sts : status);
}

/**************************************************************************
* FUNCTION
*      _NU_USBH_AUD_Disconnect
*
* DESCRIPTION
*     Disconnect callback function, invoked by stack when an interface
*     with AUDIO class is removed from the BUS.
*
* INPUTS
*     pcb_drvr       Pointer to Audio Class Driver control block claimed by
*                    this Interface.
*     stack          Pointer to Stack control block.
*     pcb_device     Pointer to NU_USB_DEVICE control block disconnected.
*
* OUTPUTS
*     NU_SUCCESS     Indicates successful completion of the service.
*
**************************************************************************/
STATUS _NU_USBH_AUD_Disconnect (NU_USB_DRVR   *pcb_drvr,
                                NU_USB_STACK  *stack,
                                NU_USB_DEVICE *pcb_device)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_AUDIO   *pcb_audio_drvr = (NU_USBH_AUDIO*) pcb_drvr;
    NU_USBH_AUD_DEV *p_next_device,
                    *p_curr_device = pcb_audio_drvr->pcb_first_device;

    /* For each connected Audio device. */
    while (p_curr_device)
    {
          /* Get next device from the list. */
        p_next_device = (NU_USBH_AUD_DEV*) p_curr_device->node.cs_next;
        if(p_curr_device->pcb_device == pcb_device)
        {
            /* Deallocate all the memory allocated by descriptor parsing
             * routines.
             */
            status = NU_AUDH_Free_Structures(p_curr_device);

            /* Send disconnection notification to user. */
            status = NU_USB_USER_Disconnect (
                                    p_curr_device->pcb_user_drvr,
                                    p_curr_device->pcb_aud_drvr,
                                    p_curr_device);

            /* Delete transfer event group. */
            status = NU_Delete_Event_Group (
                                    &(p_curr_device->trans_events));

            /* Delete semaphore to schedule control transfers. */
            status = NU_Delete_Semaphore (
                                    &(p_curr_device->sm_ctrl_trans));

            /* Remove device from the link list. */
            CSC_Remove_From_List(
                (CS_NODE **) & pcb_audio_drvr->pcb_first_device,
                (CS_NODE *) p_curr_device);

            /* Deallocate Audio device structure. */
            status = USB_Deallocate_Memory(p_curr_device);
            status = NU_SUCCESS;
        }

        /* If current device is the only device. */
        if(p_curr_device == p_next_device)
        {
            break;
        }
        else
        {
              /* Assign current device to the pointer of the next. */
            p_curr_device = p_next_device;
        }
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Is_Device_Compatible
*
* DESCRIPTION
*     This function verifies if the attached audio device is capable of
*     playing/recording Audio clip on given frequency, channel type and
*     sample size.
*
* INPUTS
*     pcb_aud_device        Pointer to the control block of Audio Device.
*     function              Microphone or Speaker function.
*     freq                  This frequency value is supported or not.
*     ch_type               channel type(MONO or STEREO) in an Audio Clip.
*                           1:Mono
*                           2:Stereo
*     sample_size           Sample size in bytes etc.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_NOT_PRESENT        Indicates that required compatibility not
*                           found.
*
**************************************************************************/
STATUS NU_USBH_AUD_Is_Device_Compatible(NU_USBH_AUD_DEV *pcb_aud_device,
                                        UINT8            function,
                                        UINT32           freq,
                                        UINT8            ch_type,
                                        UINT8            sample_size)
{
    STATUS                status = NU_NOT_PRESENT;
    AUDH_ASI_INFO        *as_info;
    AUDH_CS_ASI_INFO     *cs_asi_info = NU_NULL;
    NU_USBH_AUD_CTRL     *audio_ctrl;
    AUDH_TYPE1_DSCR_INFO *tp1;
    UINT8                 count, i;
    UINT8                 terminal_type = (UINT8)AUDH_AC_DSCR_UNDEF;
    UINT8                 intf_index;
    UINT8                 alt_sttg;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if(function == NU_AUDH_SPKR_FUNCTION)
    {
        /* Helps in finding Audio Steaming Interfaces that are attached
         * with Input Terminal.
         */
        terminal_type = (UINT8) AUDH_INPUT_TERMINAL;
    }
    if(function == NU_AUDH_MIC_FUNCTION)
    {
        /* Helps in finding Audio Steaming Interfaces that are attached
         * with Output Terminal.
         */
        terminal_type = (UINT8) AUDH_OUTPUT_TERMINAL;
    }
    audio_ctrl = pcb_aud_device->ac_cs_info;

    /* Check every Audio Streaming interface. */
    for(count = 0; count < audio_ctrl->si_count; count++)
    {
        if(status == NU_SUCCESS)
        {
            break;
        }

        /* Get Streaming Interface index from Steaming Interface List. */
        intf_index = audio_ctrl->si_list[count];

        /* Get Audio Streaming Interface info from intf index calculated
         * above.
         */
        as_info = pcb_aud_device->si_info[intf_index];

        /* Is it a valid information? */
        if(!as_info)
        {
            continue;
        }
        /* Go through all the alternate settings of this Audio Streaming
         * Interface.
         */
        for(alt_sttg = 0; alt_sttg < NU_USB_MAX_ALT_SETTINGS; alt_sttg++)
        {
            if(status == NU_SUCCESS)
            {
                break;
            }

            /* Get class specific ASI info structure for each alternate. */
            cs_asi_info = as_info->cs_asi_info[alt_sttg];

            /* Is it a valid information? */
            if(!cs_asi_info)
            {
                continue;
            }
            if(cs_asi_info->term_link_tp == terminal_type)
            {
                /* Get TYPE-1 class specific descriptor info from class
                 * specific Audio Streaming interface structure.
                 */

                tp1 = (AUDH_TYPE1_DSCR_INFO*)(cs_asi_info->fmt_tp_dscr);

                /* Sampling Frequency is continuous. */
                if(tp1->smpl_tp == (UINT8) AUDH_CONTINEOUS)
                {
                    /* If the input frequency is within the max and min
                     * frequency range?
                     */
                    if((freq >= tp1->min_smpl_freq) &&
                       (freq <= tp1->max_smpl_freq))
                    {
                        if((tp1->chnls_count == ch_type) &&
                           (tp1->sub_frm_size == sample_size))
                        {
                            /* Frequency, channels type and sample size
                             * matched.
                             */
                            status = NU_SUCCESS;
                            break;
                        }
                    }
                }

                /* Sampling frequency is Discrete. */
                if(tp1->smpl_tp > 0)
                {
                    for(i =0; i < tp1->smpl_freq_count; i++)
                    {
                        /* Frequency matched in sampling frequency list. */
                        if(freq == tp1->smpl_freq_list[i])
                        {
                            if((tp1->chnls_count == ch_type) &&
                               (tp1->bit_res == sample_size*8))
                            {
                                /* Frequency, Channels type and sample
                                 * size matched.
                                 */
                                status = NU_SUCCESS;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if(status == NU_SUCCESS)
    {
        if(function == NU_AUDH_SPKR_FUNCTION)
        {
            /* Update class specific streaming info to play Audio clip. */
            pcb_aud_device->cb_play_inform.cs_asi_info = cs_asi_info;
        }
        if(function == NU_AUDH_MIC_FUNCTION)
        {
           /* Update class specific streaming info to record Audio clip. */
           pcb_aud_device->cb_record_inform.cs_asi_info = cs_asi_info;
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Feature_Unit_Req
*
* DESCRIPTION
*    This request is used to SET or GET an attribute of an audio control
*    for given channel type inside a Feature Unit.
*
* INPUTS
*     pcb_aud_device        Pointer to the control block of Audio Device.
*     ac_req                Pointer to Audio Control request structure.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Indicates that one or more args passed to this
*                           function are invalid.
*     NU_NOT_PRESENT        Indicates that the pipe is invalid.
*     NU_NO_MEMORY          Indicates failure of memory allocation.
*
**************************************************************************/
STATUS NU_USBH_AUD_Feature_Unit_Req(NU_USBH_AUD_DEV       *pcb_aud_device,
                                    NU_USBH_AUD_CTRL_REQ  *ac_req)
{
    STATUS             status;
    UINT8              intf, bmrequest;
    UINT8              buf_len;
    UINT16             id_intf;
    NU_USBH_CTRL_IRP   *ctrl_irp;
    UNSIGNED           ret_events;
    UINT32             buffer_length;

    /* Control Selector value. */
    UINT16             cs_cn = ac_req->ctrl_slctr;
    UINT8 NU_AUDH_Ctrl_Data_len[11] = {0, 1, 2, 1, 1, 1, 0, 1, 2, 1, 1};

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    ctrl_irp = pcb_aud_device->ctrl_irp;
    memset(ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

    /* Obtain semaphore to schedule control transfers. */
    status = NU_Obtain_Semaphore (
                            &(pcb_aud_device->sm_ctrl_trans),
                            (UNSIGNED) NU_SUSPEND);

    /* Prepare Feature Unit request. */

    /* Lower byte contains Control Selector while Higher byte contains
     * Channel Type.
     */
    cs_cn = (cs_cn << 8)|(ac_req->channel);
    status = NU_USB_INTF_Get_Intf_Num(pcb_aud_device->pcb_ac_intf,&intf);
    id_intf = ac_req->id;

    /* Lower byte contains Interface Number and Higher byte contains
     * Feature Unit Id.
     */
    id_intf = (id_intf << 8)| intf;

    /* Set or Get request. */
    bmrequest =
        ((UINT8)(ac_req->req) & (0x80) ? (UINT8)NU_AUDH_GET_INF_REQUEST :
        (UINT8)NU_AUDH_SET_INF_REQUEST);
    buf_len   = NU_AUDH_Ctrl_Data_len[ac_req->ctrl_slctr];

    /* Feature Unit request. */
    status = NU_USBH_CTRL_IRP_Create (ctrl_irp,
                                      (UINT8*)ac_req->trans_buff,
                                      NU_AUDH_CTRL_IRP_Complete,
                                      pcb_aud_device,
                                      bmrequest,
                                      (UINT8) (ac_req->req),
                                      HOST_2_LE16(cs_cn),
                                      HOST_2_LE16(id_intf),
                                      HOST_2_LE16(buf_len));
    if(status == NU_SUCCESS)
    {
        /* Submits the IRP. */
        status = NU_USB_PIPE_Submit_IRP (pcb_aud_device->pcb_ctrl_pipe,
                                        (NU_USB_IRP *)ctrl_irp);
        if(status == NU_SUCCESS)
        {
            /* Wait for IRP completion. */
            status = NU_Retrieve_Events (&(pcb_aud_device->trans_events),
                                         (UNSIGNED)NU_AUDH_CTRL_SENT,
                                         (OPTION)NU_AND_CONSUME,
                                         &ret_events,
                                         (UNSIGNED)NU_SUSPEND);

            /* Retrieves the status for IRP completion. */
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP *)ctrl_irp,
                                    &status);
            /* If it is a GET request return buffer length. */
            if(status == NU_SUCCESS)
            {
                  /* Get actual length of the submitted IRP. */
                status = NU_USB_IRP_Get_Actual_Length((NU_USB_IRP*)(ctrl_irp),
                                            (UINT32*)(&buffer_length));
            }

            if(status == NU_SUCCESS)
            {
                ac_req->buf_len = (UINT16)buffer_length;
            }
        }
    }

    /* Release the acquired semaphore. */
    status = NU_Release_Semaphore (&(pcb_aud_device->sm_ctrl_trans));
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Sample_Freq_Req
*
* DESCRIPTION
*     This function sends SET or GET Sampling Frequency Request. The
*     Sampling Frequency Control is used to set the initial sampling
*     frequency for an isochronous audio data endpoint.
*
* INPUTS
*     pcb_aud_device    Pointer to Audio Device control block.
*     ep_req            Pointer to Endpoint Control request structure.
*     function          Microphone or Speaker function.
*                       NU_AUDH_SPKR_FUNCTION :Speaker Function.
*                       NU_AUDH_MIC_FUNCTION  :Microphone Function.
*
* OUTPUTS
*     NU_SUCCESS        Indicates successful completion of the service.
*     NU_USB_INVLD_ARG  Indicates that one or more args passed to this
*                       function are invalid.
*     NU_NOT_PRESENT    Indicates that the pipe is invalid.
*     NU_NO_MEMORY      Indicates failure of memory allocation.
*
**************************************************************************/
STATUS  NU_USBH_AUD_Sample_Freq_Req(NU_USBH_AUD_DEV     *pcb_aud_device,
                                    NU_USBH_AUD_EP_REQ  *ep_req,
                                    UINT8                function)
{

    STATUS            status;
    UINT8             bmrequest;
    UINT16            wlength,ep_address;
    UINT16            ctrl_slctr;
    UNSIGNED          ret_events;
    NU_USB_ENDP      *endp;
    NU_USBH_CTRL_IRP *ctrl_irp;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    ctrl_irp = pcb_aud_device->ctrl_irp;
    memset(ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

    /* Acquire semaphore to schedule control transfers. */
    status = NU_Obtain_Semaphore (
                                    &(pcb_aud_device->sm_ctrl_trans),
                                    (UNSIGNED)NU_SUSPEND);

    if (status == NU_SUCCESS )
    {
        if(function == NU_AUDH_SPKR_FUNCTION)
        {

            /* In Speaker mode,
                      * Endpoint address in control IRP must be for ISO OUT end point.
                      */
            status = NU_USB_PIPE_Get_Endp(
                                        pcb_aud_device->pcb_iso_out_pipe,
                                        &endp);
        }
        else if(function == NU_AUDH_MIC_FUNCTION)
        {
            /* In Recording mode,
                     * Endpoint address in control IRP must be for ISO IN end point.
                     */
            status = NU_USB_PIPE_Get_Endp(
                                        pcb_aud_device->pcb_iso_in_pipe,
                                        &endp);
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }
        if(status == NU_SUCCESS)
        {
            ep_address = endp->desc->bEndpointAddress;

            /* Is it a Set or Get Request? */
            bmrequest = ((UINT8)(ep_req->req) & (0x80) ?
                        (UINT8)NU_AUDH_GET_EP_REQUEST :
                        (UINT8)NU_AUDH_SET_EP_REQUEST);
            /* Fill control selector value.*/
            ctrl_slctr = ep_req->ctrl_slctr;
            ctrl_slctr = (ctrl_slctr << 8)|0;
            wlength   = 0x03;

            /* Fill Control IRP structure for Sampling Frequency Request. */
            status = NU_USBH_CTRL_IRP_Create (ctrl_irp,
                                          ep_req->trans_buff,
                                          NU_AUDH_CTRL_IRP_Complete,
                                          pcb_aud_device,
                                          bmrequest,
                                          (UINT8)(ep_req->req),
                                          HOST_2_LE16(ctrl_slctr),
                                          HOST_2_LE16(ep_address),
                                          HOST_2_LE16(wlength));
            if(status == NU_SUCCESS)
            {
                /* Submits the IRP. */
                status = NU_USB_PIPE_Submit_IRP(pcb_aud_device->pcb_ctrl_pipe,
                                           (NU_USB_IRP *)ctrl_irp);
                if(status == NU_SUCCESS)
                {
                    /* Wait for IRP completion. */
                    status = NU_Retrieve_Events (
                                    &(pcb_aud_device->trans_events),
                                    (UNSIGNED)NU_AUDH_CTRL_SENT,
                                    (OPTION)NU_AND_CONSUME,
                                    &ret_events,
                                    (UNSIGNED)NU_SUSPEND);
                }
            }
        }
                
        /* Release the acquired semaphore. */
        status = NU_Release_Semaphore (&(pcb_aud_device->sm_ctrl_trans));
        /* Revert to user mode. */
        NU_USER_MODE();
    }
        
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Pitch_Ctrl
*
* DESCRIPTION
*     This function sends SET or GET Pitch Control Request. These SET and
*     Get requests are endpoint request intended for ISO OUT or ISO IN
*     endpoint.
*
* INPUTS
*     pcb_aud_device            Pointer to control block of Audio device.
*     ep_req                    Pointer to Endpoint Control request struct.
*     function                  Microphone or Speaker function.
*
* OUTPUTS
*     NU_SUCCESS                Indicates successful completion of the
*                               service.
*     NU_USB_INVLD_ARG          Indicates that one or more args passed to
*                               this function are invalid.
*     NU_NOT_PRESENT            Indicates that the pipe is invalid.
*     NU_NO_MEMORY              Indicates failure of memory allocation.
*
**************************************************************************/
STATUS  NU_USBH_AUD_Pitch_Ctrl_Req(NU_USBH_AUD_DEV       *pcb_aud_device,
                                   NU_USBH_AUD_EP_REQ    *ep_req,
                                   UINT8                  function)
{

    STATUS            status;
    UINT8             bmrequest;
    UINT16            wlength, ctrl_slctr;
    UINT16            ep_address;
    NU_USBH_CTRL_IRP *ctrl_irp;
    UNSIGNED          ret_events;
    NU_USB_ENDP      *endp;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    ctrl_irp = pcb_aud_device->ctrl_irp;
    memset(ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));
    /* Obtain semaphore to schedule control transfers. */
    status = NU_Obtain_Semaphore (
                        &(pcb_aud_device->sm_ctrl_trans),
                         NU_SUSPEND);
    if(status == NU_SUCCESS)
    {
            
        if(function == NU_AUDH_SPKR_FUNCTION)
        {
            /* In Speaker mode,
             * Endpoint address in control IRP must be for ISO OUT end point.
             */
            status = NU_USB_PIPE_Get_Endp(
                            pcb_aud_device->pcb_iso_out_pipe,
                            &endp);
        }
        else if(function == NU_AUDH_MIC_FUNCTION)
        {
            /* In Recording mode,
             * Endpoint address in control IRP must be for ISO IN end point.
             */
            status = NU_USB_PIPE_Get_Endp(
                            pcb_aud_device->pcb_iso_in_pipe,
                            &endp);
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }
        if(status == NU_SUCCESS)
        {
            ep_address = endp->desc->bEndpointAddress;

            /* Set or Get Request. */
            bmrequest = ((UINT8)(ep_req->req) & (0x80) ?
                         (UINT8)NU_AUDH_GET_EP_REQUEST :
                         (UINT8)NU_AUDH_SET_EP_REQUEST);
            ctrl_slctr = ep_req->ctrl_slctr;
            ctrl_slctr = (ctrl_slctr << 8)|0;
            wlength   = 0x01;

            /* Fill Control IRP structure for Pitch control request. */
            status = NU_USBH_CTRL_IRP_Create (ctrl_irp,
                                              ep_req->trans_buff,
                                              NU_AUDH_CTRL_IRP_Complete,
                                              pcb_aud_device,
                                              bmrequest,
                                             (UINT8)ep_req->req,
                                              HOST_2_LE16(ctrl_slctr),
                                              HOST_2_LE16(ep_address),
                                              HOST_2_LE16(wlength));
            if(status == NU_SUCCESS)
            {

                /* Submits the IRP. */
                status = NU_USB_PIPE_Submit_IRP(pcb_aud_device->pcb_ctrl_pipe,
                                               (NU_USB_IRP *)ctrl_irp);
                if(status == NU_SUCCESS)
                {
                    /* Wait for IRP completion. */
                     status = NU_Retrieve_Events (
                                            &(pcb_aud_device->trans_events),
                                            (UNSIGNED)NU_AUDH_CTRL_SENT,
                                            (OPTION)NU_AND_CONSUME,
                                            &ret_events,
                                            (UNSIGNED)NU_SUSPEND);
                }
            }
        }

        /* Release the acquired semaphore. */
        status = NU_Release_Semaphore (&(pcb_aud_device->sm_ctrl_trans));
    
        /* Revert to user mode. */
        NU_USER_MODE();
    }


    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Open_Play_Session
*
* DESCRIPTION
*     This function creates a session to schedule the out transfers to an
*     audio device. Data transfer would start on calling the Play Sound
*     routine, It just creates the necessary environment for that.
*     Application must call this function, if it requires to play sound on
*     an attached device.
*
* INPUTS
*     pcb_aud_device             Pointer to the audio device structure.
*     ch_type                    Channel type(MONO or STEREO) in an Audio
*                                Clip.
*                                1:Mono
*                                2:Stereo
*     sample_size                Sample size in bytes.
*     sample_rate                Audio Sample rate in Hz.
*
* OUTPUTS
*     NU_SUCCESS                 Indicates successful completion.
*     NU_AUDH_SERVICE_UNAVAILBLE Service is unavailable.
*     NU_AUDH_AVAILBLE           Service is available.
*
**************************************************************************/

STATUS NU_USBH_AUD_Open_Play_Session(NU_USBH_AUD_DEV  *pcb_aud_device,
                                     UINT8             ch_type,
                                     UINT8             sample_size,
                                     UINT32            sample_rate)
{
    STATUS                   status;
    NU_USB_ENDP             *endp_out;
    NU_USBH_AUD_EP_REQ       cb_ep_req;
    UINT8                    set_frq[4]= {0x00};
    UINT8                    poll_interval;
    AUDH_CS_ASI_INFO        *cs_asi_info;

    /* Calculate data rate. */
    UINT32                   data_rate =
                             ch_type * (sample_size) * sample_rate;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if a playing session is already in progress for the device. */
    if(pcb_aud_device->cb_play_inform.pcb_task)
    {
        status = NU_AUDH_SERVICE_UNAVAILBLE;
    }
    else
    {
        /* Is it possible to play data? */
        status = NU_USBH_AUD_Is_Device_Compatible(
                                            pcb_aud_device,
                                            (UINT8)NU_AUDH_SPKR_FUNCTION,
                                             sample_rate,
                                             ch_type,
                                             sample_size);
        if(status == NU_SUCCESS)
        {
            /* Get Class specific ASI ifo. */
            cs_asi_info = pcb_aud_device->cb_play_inform.cs_asi_info;
            pcb_aud_device->pcb_iso_out_pipe = cs_asi_info->pcb_iso_pipe;
            status = NU_USB_PIPE_Get_Endp(  cs_asi_info->pcb_iso_pipe,
                                            &endp_out);
            status = NU_USB_ENDP_Get_Interval(endp_out , &poll_interval);

            /* Store polling interval for OUT end point. This will be used
             * in ISO packetizing routine.
             */
            pcb_aud_device->cb_play_inform.poll_interval =
                                                 0x01 << (poll_interval-1);
            /* Activate alternate setting that supports given frequency,
             * channel type, sample size and sample rate.
             */
            status = NU_USB_ALT_SETTG_Set_Active(cs_asi_info->op_alt_sttg);

            if(status == NU_SUCCESS)
            {
                status = NU_Set_Events(&(pcb_aud_device->task_events),
                                        ~(NU_AUDH_PLAY_TASK_TERMINATE|
                                        NU_AUDH_PLAY_TASK_TERMINATED),
                                        NU_AND);
            }

            if(status == NU_SUCCESS)
            {
                /* Allocating memory for the queue and creating queue for
                 * double buffering.
                 */
                status = USB_Allocate_Memory (
                  USB_MEM_TYPE_CACHED,
                  (UNSIGNED)AUDH_QUEUE_SIZE*sizeof(UNSIGNED),
                  (VOID **)&(pcb_aud_device->cb_play_inform.queue_pointer));
            }

            if(status == NU_SUCCESS)
            {
                status = NU_Create_Queue(
                    &(pcb_aud_device->cb_play_inform.double_buff_queue),
                    "PLAY-Q",
                    (VOID *) pcb_aud_device->cb_play_inform.queue_pointer,
                    (UNSIGNED) AUDH_QUEUE_SIZE,
                    (OPTION) NU_FIXED_SIZE,
                    (UNSIGNED) 1,
                    (OPTION) NU_FIFO);
            }

            if(status == NU_SUCCESS)
            {
                /* Allocating memory required for audio play task's
                 * block.
                 */
                status = USB_Allocate_Memory (
                         USB_MEM_TYPE_CACHED,
                         (UNSIGNED)(sizeof (NU_TASK)),
                         (VOID**)&pcb_aud_device->cb_play_inform.pcb_task);
            }

            if(status == NU_SUCCESS)
            {
                /* Initializing the whole block to 0x00 value. */
                memset (pcb_aud_device->cb_play_inform.pcb_task,
                        0,
                        sizeof (NU_TASK));

                     /* Allocating memory required for audio play task's
                      * stack.
                      */
                     status = USB_Allocate_Memory (
                                 USB_MEM_TYPE_CACHED,
                                 (UNSIGNED)NU_AUDH_PLAY_STACK_SIZE,
                                 &(pcb_aud_device->cb_play_inform.p_stack));


                /* Checking if memory allocation is successful or not. */
                if(status == NU_SUCCESS)
                {
                    /* Preparing SET Sampling Frequency Request to send to
                     * Audio device.
                     */
                    cb_ep_req.req         = AUDH_SET_CUR;
                    cb_ep_req.ctrl_slctr  = NU_AUDH_SAMPLING_FREQ_CONTROL;

                    set_frq[0] = (UINT8)sample_rate;
                    set_frq[1] = (UINT8)(sample_rate>>8);
                    set_frq[2] = (UINT8)(sample_rate>>16);

                    cb_ep_req.trans_buff =  set_frq;

                    /* Send Sampling Frequency Request to the device. */
                    status = NU_USBH_AUD_Sample_Freq_Req(
                                            pcb_aud_device,
                                            &cb_ep_req,
                                            (UINT8)NU_AUDH_SPKR_FUNCTION);

                    /* Set internal values required by audio play task. */

                    pcb_aud_device->cb_play_inform.sample_size =
                                        sample_size;
                    pcb_aud_device->cb_play_inform.data_rate  = data_rate;
                    pcb_aud_device->cb_play_inform.channels  = ch_type;
                    pcb_aud_device->cb_play_inform.service_status =
                                                          NU_AUDH_AVAILBLE;

                    /* Creating Audio Play Task. */
                    status = NU_Create_Task(
                                   pcb_aud_device->cb_play_inform.pcb_task,
                                   "MAIN_PLY",
                                   NU_AUDH_Play_Audio,
                                   (UNSIGNED)0x00,
                                   pcb_aud_device,
                                   pcb_aud_device->cb_play_inform.p_stack,
                                   (UNSIGNED)NU_AUDH_PLAY_STACK_SIZE,
                                   (OPTION)NU_AUDH_PLAY_TASK_PRIORITY,
                                   (UNSIGNED)NU_AUDH_PLAY_TIME_SLICE,
                                   (OPTION)NU_PREEMPT,
                                   (OPTION)NU_START);
                }
            }
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Close_Play_Session
*
* DESCRIPTION
*     This function deletes a session created earlier to schedule the out
*     transfers to an audio device. No Data transfer should be in progress
*     while calling this routine. On session closing it also sets zero
*     band width alternate setting active.
*
* INPUTS
*     pcb_aud_device        Pointer to the Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_Close_Play_Session(NU_USBH_AUD_DEV  *pcb_aud_device)
{
    STATUS status;


    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /*
     * Send terminate signal to player task.
     */
    status = NU_AUDH_Set_Events(pcb_aud_device,
                                NU_AUDH_PLAY_TASK_TERMINATE);

    /* Flush OUT pipe to cancel out any pending IRPs for the pipe. */
    if(status == NU_SUCCESS)
    {
        status = NU_USB_PIPE_Flush(pcb_aud_device->pcb_iso_out_pipe);
    }

    /*
     * Clear out play transfers queue.
     */
    if(status == NU_SUCCESS)
    {
        status = NU_Reset_Queue(
                      &(pcb_aud_device->cb_play_inform.double_buff_queue));
    }

    /*
     * Wait for terminated signal from player task.
     */
    if(status == NU_SUCCESS)
    {            
        status = NU_AUDH_Get_Events(pcb_aud_device,
                                    NU_AUDH_PLAY_TASK_TERMINATED,
                                    NU_SUSPEND);
    }

    /* Terminates the Playing task created while session opening. */
    if(status == NU_SUCCESS)
    {
        status = NU_Terminate_Task(pcb_aud_device->cb_play_inform.pcb_task);
    }

    /* Deletes the Playing task created while session opening. */
    if(status == NU_SUCCESS)
    {
        status = NU_Delete_Task(pcb_aud_device->cb_play_inform.pcb_task);
    }

    /* Deallocating the task and stacks memory. */
    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory (
                                  pcb_aud_device->cb_play_inform.pcb_task);
    }

    /* Delete the Doubly Buffering Queue used while playing */
    if(status == NU_SUCCESS)
    {
        status = NU_Delete_Queue(
                      &(pcb_aud_device->cb_play_inform.double_buff_queue));
    }

    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory(
                             pcb_aud_device->cb_play_inform.queue_pointer);
    }

    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory (
                                   pcb_aud_device->cb_play_inform.p_stack);
    }

    /* Resting the value in device block. */
    pcb_aud_device->cb_play_inform.pcb_task = NU_NULL;

    /* Set Zero Band Width alternate setting as active one. */
    status = NU_USB_ALT_SETTG_Set_Active(
       pcb_aud_device->cb_play_inform.cs_asi_info->asi_info->zbw_alt_sttg);

    pcb_aud_device->cb_play_inform.service_status = NU_AUDH_UNAVAILBLE;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Open_Record_Session
*
* DESCRIPTION
*     This function creates a session to schedule the IN transfers to an
*     Audio device. Data transfer would start on calling the Record Sound
*     routine. It just creates the necessary environment for that.
*
* INPUTS
*     pcb_aud_device        Pointer to the Audio device control block.
*     ch_type               Channel type(MONO or STEREO) in an Audio Clip.
*                           1:Mono.
*                           2:Stereo .
*     sample_size           Sample size in bytes.
*     sample_rate           Audio Sample rate in Hz.
*     double_buffering      Flag shows whether single buffering or double 
*                           buffering is used for recording.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_Open_Record_Session(NU_USBH_AUD_DEV  *pcb_aud_device,
                                       UINT8             ch_type,
                                       UINT8             sample_size,
                                       UINT32            sample_rate,
                                       BOOLEAN           double_buffering)
{
    STATUS                 status;
    NU_USBH_AUDIO         *pcb_aud_drvr;
    NU_USBH_AUD_EP_REQ     cb_ep_req;
    UINT8                  set_frq[4]= {0x00};
    AUDH_CS_ASI_INFO      *cs_asi_info;
    UINT8                  poll_interval;
    NU_USB_ENDP           *endp_out;

    /* Calculate data rate. */
    UINT32                 data_rate =
                           ch_type * (sample_size) * sample_rate;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Extracting class driver block from audio device. */
    pcb_aud_drvr = (NU_USBH_AUDIO*)pcb_aud_device->pcb_aud_drvr;

    /* Check if a recording session is already in progress for the device.*/
    if(pcb_aud_device->cb_record_inform.pcb_task)
    {
        status = NU_AUDH_SERVICE_UNAVAILBLE;
    }

    else
    {
        /* Is it possible to record data? */
        status = NU_USBH_AUD_Is_Device_Compatible(
                                        pcb_aud_device,
                                        (UINT8)NU_AUDH_MIC_FUNCTION,
                                        sample_rate,
                                        ch_type,
                                        sample_size);
        if( status == NU_SUCCESS)
        {
            /* Get Class specific ASI ifo. */
            cs_asi_info = pcb_aud_device->cb_record_inform.cs_asi_info;
            pcb_aud_device->pcb_iso_in_pipe = cs_asi_info->pcb_iso_pipe;
            status = NU_USB_PIPE_Get_Endp(  cs_asi_info->pcb_iso_pipe,
                                            &endp_out);
            status = NU_USB_ENDP_Get_Interval(endp_out ,
                                            &poll_interval);

            /* Store polling interval for IN end point. This will be used
             * in ISO packetizing routine.
             */
            pcb_aud_device->cb_record_inform.poll_interval =
                                                 0x01 << (poll_interval-1);

            /* Activate alternate setting that support given frequency,
             * channel type, sample size and sample rate.
             */
            status = NU_USB_ALT_SETTG_Set_Active(cs_asi_info->op_alt_sttg);

            if(status == NU_SUCCESS)
            {
                /* Allocating memory for the queue and creating queue for
                 * double buffering.
                 */
                status = USB_Allocate_Memory (
                USB_MEM_TYPE_CACHED,
                (UNSIGNED)AUDH_QUEUE_SIZE*sizeof(UNSIGNED),
                (VOID **)&(pcb_aud_device->cb_record_inform.queue_pointer));
            }

            if(status == NU_SUCCESS)
            {
                status = NU_Create_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   "REC-Q",
                   (VOID *) pcb_aud_device->cb_record_inform.queue_pointer,
                   (UNSIGNED) AUDH_QUEUE_SIZE,
                   (OPTION) NU_FIXED_SIZE,
                   (UNSIGNED) 1,
                   (OPTION) NU_FIFO);
            }

            /* Allocating memory required for audio recording task's
             * block.
             */
            status = USB_Allocate_Memory (
                        USB_MEM_TYPE_CACHED,
                        (UNSIGNED)sizeof (NU_TASK),
                        (VOID**)&pcb_aud_device->cb_record_inform.pcb_task);

            if(status == NU_SUCCESS)
            {
                /* Initializing the whole block to 0x00 value. */
                memset (pcb_aud_device->cb_record_inform.pcb_task,
                        0,
                        sizeof (NU_TASK));

                /* Allocating memory required for audio recording task's
                 * stack.
                 */
                status = USB_Allocate_Memory (
                         USB_MEM_TYPE_CACHED,
                         (UNSIGNED)NU_AUDH_PLAY_STACK_SIZE,
                         &(pcb_aud_device->cb_record_inform.p_stack));


                if(status == NU_SUCCESS)
                {
                    status = NU_Set_Events(&(pcb_aud_device->task_events),
                                            ~(NU_AUDH_REC_TASK_TERMINATE|
                                            NU_AUDH_REC_TASK_TERMINATED),
                                            NU_AND);
                }

                /* Checking if memory allocation is successful or not. */
                if(status == NU_SUCCESS)
                {

                    /* Preparing SET Sampling Frequency Request to send to
                     * device.
                     */
                    cb_ep_req.req         = AUDH_SET_CUR;
                    cb_ep_req.ctrl_slctr  = NU_AUDH_SAMPLING_FREQ_CONTROL;

                    set_frq[0] = (UINT8)sample_rate;
                    set_frq[1] = (UINT8)(sample_rate>>8);
                    set_frq[2] = (UINT8)(sample_rate>>16);

                    cb_ep_req.trans_buff =  set_frq;

                    /* Send Sampling Frequency Request for recording.*/
                    status = NU_USBH_AUD_Sample_Freq_Req(
                                            pcb_aud_device,
                                            &cb_ep_req,
                                            (UINT8)NU_AUDH_MIC_FUNCTION);

                    /* Set internal values required by audio record task.
                     */
                    pcb_aud_device->cb_record_inform.sample_size = sample_size;
                    pcb_aud_device->cb_record_inform.data_rate = data_rate;
                    pcb_aud_device->cb_record_inform.channels  = ch_type;
                    pcb_aud_device->cb_record_inform.service_status =
                                                          NU_AUDH_AVAILBLE;
                    if(double_buffering)
                    {
                        /* Create Audio Record Task. */
                        status = NU_Create_Task(
                                pcb_aud_device->cb_record_inform.pcb_task,
                                "MAIN_REC",                                
                                NU_AUDH_Record_Audio,                                                                                               
                                (UINT32)pcb_aud_drvr,
                                pcb_aud_device,
                                pcb_aud_device->cb_record_inform.p_stack,
                                (UNSIGNED)NU_AUDH_RECORD_STACK_SIZE,
                                (OPTION)NU_AUDH_RECORD_TASK_PRIORITY,
                                (UNSIGNED)NU_AUDH_RECORD_TIME_SLICE,
                                (OPTION)NU_PREEMPT,
                                (OPTION)NU_START);
                    }        
                    else
                    {                    
                        /* Create Audio Record Task. */
                        status = NU_Create_Task(
                                pcb_aud_device->cb_record_inform.pcb_task,
                                "MAIN_REC",                                
                                NU_AUDH_Record_Single_Buffer,
                                (UINT32)pcb_aud_drvr,
                                pcb_aud_device,
                                pcb_aud_device->cb_record_inform.p_stack,
                                (UNSIGNED)NU_AUDH_RECORD_STACK_SIZE,
                                (OPTION)NU_AUDH_RECORD_TASK_PRIORITY,
                                (UNSIGNED)NU_AUDH_RECORD_TIME_SLICE,
                                (OPTION)NU_PREEMPT,
                                (OPTION)NU_START);
                    } 
                }
            }
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Close_Record_Session
*
* DESCRIPTION
*     This function deletes a session created earlier to schedule the in
*     transfers to an audio device. No Data transfer should be in progress
*     while calling this routine. On session closing it also sets zero
*     band width alternate setting as active.
*
* INPUTS
*     pcb_aud_device        Pointer to the Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBH_AUD_Close_Record_Session(NU_USBH_AUD_DEV *pcb_aud_device)
{

    STATUS status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /*
     * Send terminate signal to record task.
     */
    status = NU_AUDH_Set_Events(pcb_aud_device,
                                NU_AUDH_REC_TASK_TERMINATE);

    /* Flush IN pipe to cancel out any pending IRPs for the pipe, */
    if(status == NU_SUCCESS)
    {
        status = NU_USB_PIPE_Flush(pcb_aud_device->pcb_iso_in_pipe);
    }

    /*
     * Clear out record transfers queue.
     */
    if(status == NU_SUCCESS)
    {
        status = NU_Reset_Queue(
                      &(pcb_aud_device->cb_record_inform.double_buff_queue));
    }

    /*
     * Wait for terminated signal from record task.
     */
    if(status == NU_SUCCESS)
    {

        status = NU_AUDH_Get_Events(pcb_aud_device,
                                    NU_AUDH_REC_TASK_TERMINATED,
                                    NU_SUSPEND);
    }

    /* Terminates the recording task created while session opening. */
    if(status == NU_SUCCESS)
    {
        status = NU_Terminate_Task(
                                pcb_aud_device->cb_record_inform.pcb_task);
    }

    /* Deletes the recording task created while session opening. */
    if(status == NU_SUCCESS)
    {
        status = NU_Delete_Task(pcb_aud_device->cb_record_inform.pcb_task);
    }

    /* Deallocating the task and stacks memory. */
    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory(
                                pcb_aud_device->cb_record_inform.pcb_task);
        
    }

    /* Delete the Doubly Buffering Queue used while recording */
    if(status == NU_SUCCESS)
    {
        status = NU_Delete_Queue(
                    &(pcb_aud_device->cb_record_inform.double_buff_queue));
    }

    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory(
                           pcb_aud_device->cb_record_inform.queue_pointer);
    }

    if(status == NU_SUCCESS)
    {
        status = USB_Deallocate_Memory(
                                 pcb_aud_device->cb_record_inform.p_stack);
    }

    /* Resting the value in device block. */
    pcb_aud_device->cb_record_inform.pcb_task = NU_NULL;

    /* Set Zero Band Width alternate setting as active one. */
    status = NU_USB_ALT_SETTG_Set_Active(
    pcb_aud_device->cb_record_inform.cs_asi_info->asi_info->zbw_alt_sttg);
    pcb_aud_device->cb_record_inform.service_status = NU_AUDH_UNAVAILBLE;
    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Play_Sound
*
* DESCRIPTION
*     This function starts playing sound on an audio device with data rate
*     specified earlier in Playing Session.
*
* INPUTS
*     pcb_aud_device        Pointer to the control block of Audio Device.
*     tx_data               Pointer to the start of Audio Clip.
*     tx_len                Length of Audio Clip in terms of audio samples.
*                           Duration of sound to be played should not
*                           exceed 300 milliseconds. Only sounds of 300
*                           milliseconds duration or less can be played.
*                           This restriction applies on both stereo and
*                           mono sounds.
*                           For a mono sound, recorded using 16 bit
*                           samples, an audio sample means a 16 bit signed
*                           value.
*                           For a stereo sound, recorded using 16 bit
*                           samples, an audio sample means a pair of 16 bit
*                           signed value.
*     call_back             Function Pointer of NU_AUDH_Data_Callback type.
*
* OUTPUTS
*     NU_SUCCESS            Indicates sound has been successfully queued
*                           up for playing.
*     NU_AUDH_NOT_AVAILBLE  Playing service is not available.
*     NU_INVALID_SIZE       Length of data specified is not supported.
*
**************************************************************************/
STATUS NU_USBH_AUD_Play_Sound(NU_USBH_AUD_DEV            *pcb_aud_device,
                              UINT8                      *tx_data,
                              UINT32                      tx_len,
                              NU_USBH_AUD_Data_Callback   call_back)
{
    STATUS status = NU_SUCCESS;
    UINT32      message;
    UINT32      allowed_length;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if a session has been opened for this. */
    if((pcb_aud_device->cb_play_inform.service_status) ==
        NU_AUDH_AVAILBLE )
    {
          allowed_length = (pcb_aud_device->cb_play_inform.data_rate)*
           (NU_AUDH_MAX_ISO_PKT_IRP*NU_AUDH_NUM_PLAY_TRANS)/1000;


        tx_len *= pcb_aud_device->cb_play_inform.sample_size;
        tx_len *= pcb_aud_device->cb_play_inform.channels;

        if(tx_len > allowed_length)
        {
            status = NU_INVALID_SIZE;
        }

        if(status == NU_SUCCESS)
        {
            message = (UINT32) tx_data;
            status = NU_Send_To_Queue(
                      &(pcb_aud_device->cb_play_inform.double_buff_queue),
                      (VOID *)&message,
                      (UNSIGNED) 1,
                      (UNSIGNED) NU_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            message = (UINT32) tx_len;
            status = NU_Send_To_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       (VOID *)&message,
                       (UNSIGNED) 1,
                       (UNSIGNED) NU_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            message = (UINT32) call_back;
            status = NU_Send_To_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       (VOID *)&message,
                       (UNSIGNED) 1,
                       (UNSIGNED) NU_SUSPEND);
        }
    }

    else
    {
        status = NU_AUDH_NOT_AVAILBLE;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_AUD_Record_Sound
*
* DESCRIPTION
*     This function starts recording sound from an audio device on data
*     rate specified earlier in Recording Session.
*
* INPUTS
*     pcb_aud_device        Pointer to the control block of Audio Device.
*     rx_data               Pointer to the start of Audio Clip.
*     rx_len                Length of Audio Clip.
*     call_back             Function Pointer of NU_AUDH_Data_Callback type.
*
* OUTPUTS
*     NU_SUCCESS            Indicates recording request has successfully
*                           been queued up.
*     NU_AUDH_NOT_AVAILBLE  Recording service is not available.
*
**************************************************************************/
STATUS NU_USBH_AUD_Record_Sound(NU_USBH_AUD_DEV           *pcb_aud_device,
                                UINT8                     *rx_data,
                                UINT32                     rx_len,
                                NU_USBH_AUD_Data_Callback  call_back)
{
    UINT32 message;
    STATUS  status = NU_AUDH_NOT_AVAILBLE;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if a session has been opened for this. */
    if((pcb_aud_device->cb_record_inform.service_status) ==
        NU_AUDH_AVAILBLE )
    {
        message = (UINT32) rx_data;
         status = NU_Send_To_Queue(
                    &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   (VOID *)&message,
                   (UNSIGNED) 1,
                   (UNSIGNED) NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            message = (UINT32) (rx_len*
                           (pcb_aud_device->cb_record_inform.sample_size)*
                           (pcb_aud_device->cb_record_inform.channels));

            status = NU_Send_To_Queue(
                    &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   (VOID *)&message,
                   (UNSIGNED) 1,
                   (UNSIGNED) NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            message = (UINT32) call_back;
            status = NU_Send_To_Queue(
                    &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   (VOID *)&message,
                   (UNSIGNED) 1,
                   (UNSIGNED) NU_NO_SUSPEND);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Find_Play_Function
*
* DESCRIPTION
*     This function finds play function in an audio device. If available,
*     it initializes NU_USBH_AUD_FUNC_INFO structure accordingly.
*
* INPUTS
*     pcb_aud_device          Pointer to the control block of Audio Device.
*     function_info           Structure that needs to be initialized if
*                             playing function is present.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*     NU_AUDH_NOT_AVAILBLE    Playing function not found
*
**************************************************************************/
STATUS NU_AUDH_Find_Play_Function(NU_USBH_AUD_DEV       *pcb_aud_device,
                                  NU_USBH_AUD_FUNC_INFO *function_info)
{

    STATUS                status = NU_AUDH_NOT_AVAILBLE;
    UINT8                 id;
    UINT8                 succ_count, op_pred_id;
    NU_USBH_AUD_ENTITY   *ent_info, *ent_info_pred;
    NU_USBH_AUD_ENTITY  **ent_array;
    NU_USBH_AUD_ENTITY   *op_term_info;
    NU_USBH_AUD_ENTITY   *succ_ent_info;

    /* Get audio control info. */
    NU_USBH_AUD_CTRL    *ac_info = pcb_aud_device->ac_cs_info;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Pointer to entities info data base. */
    ent_array = ac_info->ent_id_info;

    /* Go through all entities present. */
    for(id = 0; id <= ac_info->max_ent_id; id++)
    {

        /* Playing function found. */
        if( status == NU_SUCCESS)
        {
            break;
        }

        /* Get entity info based on id. */
        ent_info = ent_array[id];

        /* No need to go further if entity info is not valid. */
        if(ent_info == NU_NULL)
        {
            continue;
        }

        /* If Input Terminal with sub type streaming found? */
        if((ent_info->ent_tp == AUDH_INPUT_TERMINAL)&&
           (ent_info->ent_sub_tp == AUDH_STREAMING))
        {

            for(succ_count = 1; succ_count <= ent_info->succ_count;
                succ_count++)
            {

                /* Get successor entity info. */
                succ_ent_info =
                ac_info->ent_id_info[ent_info->succ_list[succ_count]];

                /* Find Output terminal attached with this Input
                 * Terminal.
                 */
                while( succ_ent_info->ent_tp != AUDH_OUTPUT_TERMINAL )
                {
                    succ_ent_info = ent_array[succ_ent_info->succ_list[1]];
                }

                /* Output Terminal found. */
                op_term_info = succ_ent_info;

                 /* Check if Output Terminal is of speaker type? */
                if(op_term_info->ent_sub_tp != AUDH_STREAMING)
                {
                    /* Get predecessor id of output terminal .*/
                    op_pred_id = op_term_info->pred_list[1];

                    /* Get predecessor entity info. */
                    ent_info_pred = ent_array[op_pred_id];

                    /* If Feature Unit is attached with this Output
                     * Terminal?
                     */
                    if(ent_info_pred->ent_tp == AUDH_FEATURE_UNIT)
                    {
                        /* Save feature unit id that is attached with
                         * Output Terminal.
                         */
                        function_info->feature_unit = ent_info_pred->ent_id;

                        /* Save Output terminal id. */
                        function_info->output_term  = op_term_info->ent_id;

                        /* Save Input Terminal id. */
                        function_info->input_term   = ent_info->ent_id;

                        /* Playing function found. */
                        status = NU_SUCCESS;
                        break;
                    }

                    /* If no Feature Unit is attached with this Output
                     * Terminal.
                     */
                    else
                    {
                        /* No feature unit, so Feature Unit id is
                         * invalid.
                         */
                        function_info->feature_unit = NU_AUDH_INVALID_ID;

                        /* Save Output Terminal id. */
                        function_info->output_term  = op_term_info->ent_id;

                        /* Save Input Terminal id. */
                        function_info->input_term   = ent_info->ent_id;

                        /* Playing function found. */
                        status = NU_SUCCESS;
                        break;
                    }

                }
            }
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Find_Record_Function
*
* DESCRIPTION
*     This function finds recording function in an audio device. On
*     availability, it initializes NU_USBH_AUD_FUNC_INFO structure
*     accordingly.
*
* INPUTS
*     pcb_aud_device           Pointer to the control block of Audio Device.
*     function_info            Structure that needs to be initialized if
*                              recording function is present.
*
* OUTPUTS
*     NU_SUCCESS               Indicates successful completion.
*     NU_AUDH_NOT_AVAILBLE     Recording function not found.
*
**************************************************************************/
STATUS NU_AUDH_Find_Record_Function(NU_USBH_AUD_DEV       *pcb_aud_device,
                                    NU_USBH_AUD_FUNC_INFO *function_info)
{

    STATUS                 status = NU_AUDH_NOT_AVAILBLE;
    UINT8                  id;
    UINT8                  succ_count;
    NU_USBH_AUD_ENTITY    *ent_info, *succ_ent_info;
    NU_USBH_AUD_ENTITY    *ip_succ_ent_info;
    NU_USBH_AUD_ENTITY    *op_term_info;
    NU_USBH_AUD_ENTITY   **ent_array;

    /* Audio control Information. */
    NU_USBH_AUD_CTRL    *ac_info = pcb_aud_device->ac_cs_info;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Pointer to the entity info data base. */
    ent_array = ac_info->ent_id_info;

    /* Go through all entities present. */
    for(id = 0; id <= ac_info->max_ent_id; id++)
    {

        /* Playing function found. */
        if(status == NU_SUCCESS)
        {
            break;
        }
        ent_info = ac_info->ent_id_info[id];

        /* No need to go further if entity info is not valid. */
        if(ent_info == NU_NULL)
        {
            continue;
        }

        /* If input terminal with sub type microphone found? */
        if((ent_info->ent_tp == AUDH_INPUT_TERMINAL)&&
           (ent_info->ent_sub_tp != AUDH_STREAMING ))
        {

            for(succ_count = 1; succ_count <= ent_info->succ_count ;
                succ_count++)
             {
                 /* Get successor entity info. */
                 succ_ent_info = ent_array[ent_info->succ_list[succ_count]];
                 /* Immediate successor to input terminal. */
                 ip_succ_ent_info = succ_ent_info;

                 /* Find Output terminal attached with this Input
                  * Terminal.
                  */
                 while( succ_ent_info->ent_tp != AUDH_OUTPUT_TERMINAL )
                 {
                     succ_ent_info =
                     ac_info->ent_id_info[succ_ent_info->succ_list[1]];
                 }

                 /* Output Terminal found. */
                 op_term_info = succ_ent_info;

                 /* Check if Output Terminal is of streaming type? */
                 if(op_term_info->ent_sub_tp == AUDH_STREAMING)
                 {

                     /* If Feature Unit is attached with this Input
                      * Terminal?
                      */
                     if(ip_succ_ent_info->ent_tp == AUDH_FEATURE_UNIT)
                     {
                         /* Save Feature Unit id. */
                         function_info->feature_unit =
                                                  ip_succ_ent_info->ent_id;
                         /* Save Output terminal id. */
                         function_info->output_term  =
                                                  op_term_info->ent_id;
                         /* Save Input terminal id. */
                         function_info->input_term   =
                                                  ent_info->ent_id;
                         /* Recording function found. */
                         status = NU_SUCCESS;
                         break;
                     }

                     /* If no Feature Unit is attached with this Output
                      * Terminal.
                      */
                     else
                     {
                         /* No Feature Unit hence id is invalid. */
                         function_info->feature_unit = NU_AUDH_INVALID_ID;

                         /* Save Output Terminal id. */
                         function_info->output_term  = op_term_info->ent_id;

                         /* Save Input Terminal id. */
                         function_info->input_term   = ent_info->ent_id;

                         /* Recording function found. */
                         status = NU_SUCCESS;
                         break;
                     }
                 }
             }
         }
    }
    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*       NU_USBH_AUD_Register_User
*
* DESCRIPTION
*       This function registers a user to the class driver.
*
* INPUTS
*       pcb_aud_drvr                        Pointer to the control block of
*                                           Audio Device.
*       pcb_user_drvr                       Pointer to user driver control
*                                           block.
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Indicates invalid pointer to
*                                           audio driver control block.
*
**************************************************************************/
STATUS NU_USBH_AUD_Register_User(NU_USBH_AUDIO  *pcb_aud_drvr,
                                 NU_USBH_USER   *pcb_user_drvr )
{
    STATUS status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if(pcb_aud_drvr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }

    if(status == NU_SUCCESS)
    {
        pcb_aud_drvr->pcb_user_drvr = pcb_user_drvr;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}



/*************************************************************************
*   FUNCTION
*
*       NU_USBH_AUD_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host audio
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a host audio
*                           class driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
*************************************************************************/
STATUS NU_USBH_AUD_GetHandle(VOID  **handle)
{
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_Audio_Cb_Pt;
    if (NU_USBH_Audio_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*       NU_USBH_AUD_Get_Function_Count
*
* DESCRIPTION
*       This function counts the number of playback or recording functions
*       available in the device and return it to the caller.
*
* INPUTS
*       pcb_aud_device                      Pointer to the control block of
*                                           audio device.
*       function                            Microphone or Speaker function.
*       count                               Number of functions available
*                                           in the device are returned
*                                           to the caller by updating this
*                                           parameter.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Indicates that an invalid
*                                           pointer has been passed as
*                                           a parameter.
*
**************************************************************************/
STATUS NU_USBH_AUD_Get_Function_Count(NU_USBH_AUD_DEV *pcb_aud_device,
                                      UINT8            function,
                                      UINT32 *         pCount)
{
    STATUS                status;
    AUDH_ASI_INFO        *as_info;
    AUDH_CS_ASI_INFO     *cs_asi_info = NU_NULL;
    NU_USBH_AUD_CTRL     *audio_ctrl;
    UINT8                 count;
    UINT8                 terminal_type = (UINT8)AUDH_AC_DSCR_UNDEF;
    UINT8                 intf_index;
    UINT8                 alt_sttg;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    (*pCount) = 0;

    if(pcb_aud_device == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }

    else
    {
        status = NU_SUCCESS;
    }

    if(status == NU_SUCCESS)
    {
        if(function == NU_AUDH_SPKR_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Input Terminal.
             */
            terminal_type = (UINT8) AUDH_INPUT_TERMINAL;
        }
        if(function == NU_AUDH_MIC_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Output Terminal.
             */
            terminal_type = (UINT8) AUDH_OUTPUT_TERMINAL;
        }

        audio_ctrl = pcb_aud_device->ac_cs_info;

        if(audio_ctrl == NU_NULL)
        {
            status = NU_INVALID_POINTER;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Check every Audio Streaming interface. */
        for(count = 0; count < audio_ctrl->si_count; count++)
        {
            /*
             * Get Streaming Interface index from Steaming Interface List.
             */
            intf_index = audio_ctrl->si_list[count];

            /* Get Audio Streaming Interface info from intf index
             * calculated above.
             */
            as_info = pcb_aud_device->si_info[intf_index];

            /* Is it a valid information? */
            if(!as_info)
            {
                continue;
            }
            /* Go through all the alternate settings of this Audio
             * Streaming Interface.
             */
            for(alt_sttg = 0;
                alt_sttg < NU_USB_MAX_ALT_SETTINGS;
                alt_sttg++)
            {
                /*
                 * Get class specific ASI info structure for each
                 * alternate.
                 */
                cs_asi_info = as_info->cs_asi_info[alt_sttg];

                /* Is it a valid information? */
                if(cs_asi_info != NU_NULL)
                {
                    if(cs_asi_info->term_link_tp == terminal_type)
                    {
                        (*pCount)++;
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*       NU_USBH_AUD_Get_Freqs_Count
*
* DESCRIPTION
*       This function counts the number of sampling frequencies supported
*       by passed playback or recording function and return it to the
*       caller.
*
* INPUTS
*       pcb_aud_device                      Pointer to the control block of
*                                           audio device.
*       function                            Microphone or Speaker function.
*       function_index                      Zero based index of the
*                                           function whose number of
*                                           supported sampling frequencies
*                                           is to be counted.
*       count                               Number of supported sampling
*                                           frequencies is returned to the
*                                           caller by updating this
*                                           parameter.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Indicates that an invalid
*                                           pointer has been passed as
*                                           a parameter.
*       NU_INVALID_ENTRY                    Indicates that an invalid
*                                           function entry has been
*                                           encountered.
*
**************************************************************************/
STATUS NU_USBH_AUD_Get_Freqs_Count(NU_USBH_AUD_DEV *pcb_aud_device,
                                   UINT8            function,
                                   UINT32           function_index,
                                   UINT32 *         pCount)
{
    STATUS                status;
    AUDH_ASI_INFO        *as_info;
    AUDH_CS_ASI_INFO     *cs_asi_info = NU_NULL;
    NU_USBH_AUD_CTRL     *audio_ctrl;
    AUDH_TYPE1_DSCR_INFO *tp1;
    UINT8                 count;
    UINT8                 terminal_type = (UINT8)AUDH_AC_DSCR_UNDEF;
    UINT8                 intf_index;
    UINT8                 alt_sttg;
    UINT32                i = 0;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    (*pCount) = 0;

    if(pcb_aud_device == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }

    else
    {
        status = NU_SUCCESS;
    }

    if(status == NU_SUCCESS)
    {
        if(function == NU_AUDH_SPKR_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Input Terminal.
             */
            terminal_type = (UINT8) AUDH_INPUT_TERMINAL;
        }
        if(function == NU_AUDH_MIC_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Output Terminal.
             */
            terminal_type = (UINT8) AUDH_OUTPUT_TERMINAL;
        }

        audio_ctrl = pcb_aud_device->ac_cs_info;

        if(audio_ctrl == NU_NULL)
        {
            status = NU_INVALID_POINTER;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Check every Audio Streaming interface. */
        for(count = 0;
            (count < audio_ctrl->si_count) &&
            ((*pCount) == 0) &&
            (status == NU_SUCCESS);
            count++)
        {
            /*
             * Get Streaming Interface index from Steaming Interface List.
             */
            intf_index = audio_ctrl->si_list[count];

            /* Get Audio Streaming Interface info from intf index
             * calculated above.
             */
            as_info = pcb_aud_device->si_info[intf_index];

            /* Is it a valid information? */
            if(!as_info)
            {
                continue;
            }
            /* Go through all the alternate settings of this Audio
             * Streaming Interface.
             */
            for(alt_sttg = 0;
                (alt_sttg < NU_USB_MAX_ALT_SETTINGS) &&
                ((*pCount) == 0) &&
                (status == NU_SUCCESS);
                alt_sttg++)
            {
                /*
                 * Get class specific ASI info structure for each
                 * alternate.
                 */
                cs_asi_info = as_info->cs_asi_info[alt_sttg];

                /* Is it a valid information? */
                if(cs_asi_info != NU_NULL)
                {
                    if(cs_asi_info->term_link_tp == terminal_type)
                    {
                        if(i == function_index)
                        {
                            tp1 = (AUDH_TYPE1_DSCR_INFO*)(cs_asi_info->fmt_tp_dscr);

                            if(tp1 != NU_NULL)
                            {
                                if(tp1->smpl_tp == (UINT8) AUDH_CONTINEOUS)
                                {
                                    (*pCount) = 2;
                                }

                                else
                                {
                                    (*pCount) = tp1->smpl_freq_count;
                                }
                            }

                            else
                            {
                                status = NU_INVALID_POINTER;
                            }
                        }

                        else
                        {
                            i++;
                        }
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

/**************************************************************************
* FUNCTION
*       NU_USBH_AUD_Get_Func_Settings
*
* DESCRIPTION
*       This function returns the playback or recording settings supported
*       by passed playback or recording function and return it to the
*       caller.
*
* INPUTS
*       pcb_aud_device                      Pointer to the control block of
*                                           audio device.
*       function                            Microphone or Speaker function.
*       function_index                      Zero based index of the
*                                           function whose number of
*                                           supported sampling frequencies
*                                           is to be counted.
*       pFreqType                           Number of supported sampling
*                                           frequencies is returned to the
*                                           caller by updating this
*                                           parameter.
*                                           NU_TRUE = Discrete Frequency
*                                           NU_FALSE = Continuous Frequency
*       pFreqList                           List of supported frequencies.
*       pFreqCount                          Count of supported frequencies.
*       pChnlCount                          Count of number of channels.
*       pSmplSize                           Sample size for each channel.
*       pLockDelay                          Lock delay.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Indicates that an invalid
*                                           pointer has been passed as
*                                           a parameter.
*       NU_INVALID_ENTRY                    Indicates that an invalid
*                                           function entry has been
*                                           encountered.
*
**************************************************************************/
STATUS NU_USBH_AUD_Get_Func_Settings(NU_USBH_AUD_DEV *pcb_aud_device,
                                   UINT8            function,
                                   UINT32           function_index,
                                   BOOLEAN *        pFreqType,
                                   UINT32 *         pFreqList,
                                   UINT32 *         pFreqCount,
                                   UINT8 *          pChnlCount,
                                   UINT8 *          pSmplSize,
                                   UINT16 *         pLockDelay,
                                   UINT8 *          pResBits)
{
    STATUS                status;
    AUDH_ASI_INFO        *as_info;
    AUDH_CS_ASI_INFO     *cs_asi_info = NU_NULL;
    NU_USBH_AUD_CTRL     *audio_ctrl;
    AUDH_TYPE1_DSCR_INFO *tp1;
    UINT8                 count;
    UINT8                 terminal_type = (UINT8)AUDH_AC_DSCR_UNDEF;
    UINT8                 intf_index;
    UINT8                 alt_sttg;
    UINT32                i = 0;
    UINT32                j;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if((pcb_aud_device == NU_NULL) ||
        (pFreqType == NU_NULL) ||
        (pFreqList == NU_NULL) ||
        (pFreqCount == NU_NULL) ||
        (pChnlCount == NU_NULL) ||
        (pSmplSize == NU_NULL) ||
        (pLockDelay == NU_NULL) ||
        (pResBits == NU_NULL))
    {
        status = NU_INVALID_POINTER;
    }

    else
    {
        status = NU_SUCCESS;
    }

    if(status == NU_SUCCESS)
    {
        (*pFreqCount) = 0;
        if(function == NU_AUDH_SPKR_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Input Terminal.
             */
            terminal_type = (UINT8) AUDH_INPUT_TERMINAL;
        }
        if(function == NU_AUDH_MIC_FUNCTION)
        {
            /* Helps in finding Audio Steaming Interfaces that are attached
             * with Output Terminal.
             */
            terminal_type = (UINT8) AUDH_OUTPUT_TERMINAL;
        }

        audio_ctrl = pcb_aud_device->ac_cs_info;

        if(audio_ctrl == NU_NULL)
        {
            status = NU_INVALID_POINTER;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Check every Audio Streaming interface. */
        for(count = 0;
            (count < audio_ctrl->si_count) &&
            ((*pFreqCount) == 0) &&
            (status == NU_SUCCESS);
            count++)
        {
            /*
             * Get Streaming Interface index from Steaming Interface List.
             */
            intf_index = audio_ctrl->si_list[count];

            /* Get Audio Streaming Interface info from intf index
             * calculated above.
             */
            as_info = pcb_aud_device->si_info[intf_index];

            /* Is it a valid information? */
            if(!as_info)
            {
                continue;
            }
            /* Go through all the alternate settings of this Audio
             * Streaming Interface.
             */
            for(alt_sttg = 0;
                (alt_sttg < NU_USB_MAX_ALT_SETTINGS) &&
                ((*pFreqCount) == 0) &&
                (status == NU_SUCCESS);
                alt_sttg++)
            {
                /*
                 * Get class specific ASI info structure for each
                 * alternate.
                 */
                cs_asi_info = as_info->cs_asi_info[alt_sttg];

                /* Is it a valid information? */
                if(cs_asi_info != NU_NULL)
                {
                    if(cs_asi_info->term_link_tp == terminal_type)
                    {
                        if(i == function_index)
                        {
                            tp1 = (AUDH_TYPE1_DSCR_INFO*)(cs_asi_info->fmt_tp_dscr);

                            if(tp1 != NU_NULL)
                            {
                                if(tp1->smpl_tp == (UINT8) AUDH_CONTINEOUS)
                                {
                                    (*pFreqType) = NU_FALSE;
                                    (*pFreqCount) = 2;
                                    pFreqList[0] = tp1->min_smpl_freq;
                                    pFreqList[1] = tp1->max_smpl_freq;
                                }

                                else
                                {
                                    (*pFreqType) = NU_TRUE;
                                    (*pFreqCount) = tp1->smpl_freq_count;
                                    for(j=0; j<tp1->smpl_freq_count; j++)
                                    {
                                        pFreqList[j] =
                                                    tp1->smpl_freq_list[j];
                                    }
                                }

                                (*pLockDelay) = tp1->loc_del;
                                (*pChnlCount) = tp1->chnls_count;
                                (*pSmplSize) = tp1->sub_frm_size;
                                (*pResBits) = tp1->bit_res;
                            }

                            else
                            {
                                status = NU_INVALID_POINTER;
                            }
                        }

                        else
                        {
                            i++;
                        }
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();
    return status;
}

#endif
/************************* end of file ***********************************/
