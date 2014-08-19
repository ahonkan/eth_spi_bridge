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
*     nu_usbh_audio_imp.c
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains core routines for Nucleus USB host stack's AUDIO
*     class driver component.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     NU_AUDH_CTRL_IRP_Complete    Control IRP completion callback.
*     NU_AUDH_ISO_OUT_IRP_Complete ISO OUT IRP completion callback.
*     NU_AUDH_ISO_IN_IRP_Complete  ISO IN IRP completion callback.
*     NU_AUDH_Parse_AC_Dscr        Parses Audio Control Intf descriptor.
*     NU_AUDH_Parse_Entities       Parses class specific info of an entity.
*     NU_AUDH_Parse_Terminal       Parses class specific info for a
*                                  Terminal.
*     NU_AUDH_Parse_Mixer_Unit     Parses class specific info for a Mixer
*                                  Unit.
*     NU_AUDH_Parse_Feature_Unit   Parses class specific info for a Feature
*                                  Unit.
*     NU_AUDH_Parse_Proc_Unit      Parses class specific info for a
*                                  Processing Unit.
*     NU_AUDH_Parse_Ext_Unit       Parses class specific info for an
*                                  Extension Unit.
*     NU_AUDH_Parse_Sel_Unit       Parses class specific info for a
*                                  Selector Unit.
*     NU_AUDH_Parse_Invalid_Unit   Indicates an invalid unit case.
*     NU_AUDH_Parse_Audio_Strm_Inf Parses Class specific streaming
*                                  interfaces.
*     NU_AUDH_Parse_Type_I_Dscr    Parses Class specific Type I descriptor.
*     NU_AUDH_Parse_Type_II_Dscr   Parses Class specific Type II descriptor.
*     NU_AUDH_Parse_Type_III_Dscr  Parses Class specific Type III
*                                  descriptor.
*     NU_AUDH_Updt_Vldt_Conn       Builds the connectivity information.
*     NU_AUDH_Calc_Succ_Cnt        Calculates number of successor count for
*                                  an entity.
*     NU_AUDH_Update_Succ          Updates Successor List for an entity.
*     NU_AUDH_Free_Structures      Frees memory allocated during parsing.
*     NU_AUDH_Play_Audio           Starts playing sound on an audio device.
*     NU_AUDH_Record_Audio         Start recording on an audio device.
*     NU_AUDH_Initialize_Device    Initializes control blocks of Audio
*                                  device.
*     NU_AUDH_Fill_ISO_Buffer      Packetizes the data in Type I format.
*
* DEPENDENCIES
*     nu_usb.h                     USB Definitions.
*
**************************************************************************/

/* =====================  USB Include Files  =========================== */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "nu_usbh_audio_imp.h"

/* Global variables. */
extern NU_AUDH_FMT_PARSE_FUNC
                      NU_AUDH_Fmt_Parse_Fptr[NU_AUDH_MAX_TYPE_FMT_COUNT];
extern INT            NU_Input_Pins_Count_Offset[NU_AUDH_MAX_ENTITY_COUNT];
extern INT            NU_Src_Id_Offset[NU_AUDH_MAX_ENTITY_COUNT];

extern NU_AUDH_UNIT_PARSE_FUNC
                      NU_AUDH_Unit_Parse_Fptr[NU_AUDH_MAX_ENTITY_COUNT];

/* Functions. */
/**************************************************************************
* FUNCTION
*     NU_AUDH_CTRL_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on control pipe of Audio Device
*     gets completed. It takes the context information from the IRP and
*     wakes up the task waiting on submission of IRP.
*
* INPUTS
*     pcb_pipe       Pointer to the Pipe control block.
*     pcb_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/
VOID NU_AUDH_CTRL_IRP_Complete (NU_USB_PIPE *pcb_pipe,
                                NU_USB_IRP  *pcb_irp)
{
    STATUS status;
    NU_USBH_AUD_DEV *pcb_aud_device;

    /* Gets the cookie from IRP. */
    status = NU_USB_IRP_Get_Context (pcb_irp, (VOID**)&pcb_aud_device);
    if (status == NU_SUCCESS)
    {
    /* Set the event and resume the task. */
        status = NU_Set_Events (
                    &(pcb_aud_device->trans_events),
                    (UNSIGNED)NU_AUDH_CTRL_SENT,
                    (OPTION)NU_OR);
    }
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_ISO_OUT_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on ISO OUT pipe of Audio Device
*     gets completed. It takes the context information from the IRP and
*     wakes up the task waiting on submission of IRP.
*
* INPUTS
*     iso_pipe       Pointer to the Pipe control block.
*     iso_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/

VOID NU_AUDH_ISO_OUT_IRP_Complete (NU_USB_PIPE *iso_pipe,
                                   NU_USB_IRP  *iso_irp)
{
    STATUS status;
    NU_USBH_AUD_DEV *pcb_aud_device;

    /* Gets the cookie from IRP. */
    status = NU_USB_IRP_Get_Context (iso_irp, (VOID**)&pcb_aud_device);
    if (status == NU_SUCCESS)
    {
    /* Set the event and resume the task. */
        status = NU_Release_Semaphore(&pcb_aud_device->play_sem);
    }
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_ISO_IN_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on ISO IN pipe of audio gets
*     completed. It takes the context information from the IRP and wakes up
*     the task waiting on submission of IRP.
*
* INPUTS
*     iso_pipe       Pointer to the Pipe control block.
*     iso_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/
VOID NU_AUDH_ISO_IN_IRP_Complete (NU_USB_PIPE *iso_pipe,
                                  NU_USB_IRP  *iso_irp)
{
    STATUS status;
    NU_USBH_AUD_DEV *pcb_aud_device;

    /* Gets the cookie from IRP. */
    status = NU_USB_IRP_Get_Context (iso_irp, (VOID**)&pcb_aud_device);
    if (status == NU_SUCCESS)
    {
        /* Release semaphore and resume the task. */
        status = NU_Release_Semaphore(&pcb_aud_device->rec_sem);
    }
}

/**************************************************************************
*
* FUNCTION
*     NU_AUDH_Parse_AC_Dscr
*
* DESCRIPTION
*     This function Parses the header and all units and terminals related
*     to the specified Audio Control Interface.
*
* INPUTS
*     pcb_audio_dev                 Pointer to Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                    Indicates successful completion.
*     NU_AUDH_NO_HEADER             No header descriptor was found.
*     NU_AUDH_INVALID_HEADER_SIZE   Header size was invalid.
*     NU_AUDH_NO_TERM_UNITS_PRESENT No terminal or units are present.
*     NU_AUDH_MEM_FAIL              Run time memory allocation failed.
*
**************************************************************************/
STATUS NU_AUDH_Parse_AC_Dscr(NU_USBH_AUD_DEV *pcb_audio_dev)
{
    UINT8                cur_len, cur_dscr_tp;
    UINT8                cur_dscr_sub_tp;
    UINT8               *cs_descr;
    UINT16               id,tot_len;
    STATUS               status = NU_SUCCESS;

    NU_USBH_AUD_CTRL    *aud_ctrl_info;

    cs_descr       =  pcb_audio_dev->cs_descr;

    do
    {
        /* Allocate memory to Audio Control Info structure. */
        status = USB_Allocate_Memory (USB_MEM_TYPE_CACHED,
                                     (UNSIGNED)sizeof(NU_USBH_AUD_CTRL),
                                     (VOID **) &aud_ctrl_info);
        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
            break;
        }

        /* Initializing the structure with the value 0x00. */
        memset(aud_ctrl_info, 0, sizeof(NU_USBH_AUD_CTRL));

        /* Get current length of header descriptor. */
        cur_len         = cs_descr[NU_AUDH_LENGTH_OFSET];

        /* Descriptor type. */
        cur_dscr_tp     = cs_descr[NU_AUDH_DSCR_TYPE_OFSET];

        /* Descriptor sub type. */
        cur_dscr_sub_tp = cs_descr[NU_AUDH_DSCR_SUB_TYPE_OFSET];
        tot_len         = 0;

        /* Total number of bytes returned for the class specific AC
         * interface descriptor. It includes the combined length of header
         * descriptor and all Unit and Terminal followed by the header
         * descriptor.
         */
        tot_len = ((cs_descr[NU_AUDH_HDR_TOT_LEN_OFSET + 1] <<8) |
                   (cs_descr[NU_AUDH_HDR_TOT_LEN_OFSET]));

        /* Is the first class specific descriptor of header type? */
        if((cur_dscr_tp != NU_AUDH_CS_INTERFACE)||
           (cur_dscr_sub_tp != (UINT8)AUDH_HEADER))
        {
            status = NU_AUDH_NO_HEADER;
            break;
        }

        /* Check if the size of the header is appropriate w.r.t number
         * of streaming interfaces used by this ACI.
         */
        if(cur_len != (8 + cs_descr[NU_AUDH_HDR_NUM_SI_OFSET]))
        {
            status = NU_AUDH_INVALID_HEADER_SIZE;
            break;
        }

        /* Check if the total size is greater than the header size otherwise
         * it means no entities are present in this ACI.
         */
        if(tot_len <= cur_len )
        {
            /* Only Header with no terminal or unit descriptors. */
            status = NU_AUDH_NO_TERM_UNITS_PRESENT;
            break;
        }

        /* Store the header header length in Audio control info. */
        aud_ctrl_info->hdr_len    = cur_len;

        /* Total length of class specific ACI descriptor. */
        aud_ctrl_info->tot_len    = tot_len;

        /* Streaming interfaces count. */
        aud_ctrl_info->si_count   = cs_descr[NU_AUDH_HDR_NUM_SI_OFSET];

        /* Allocate memory to Streaming Interface List. */
        status = USB_Allocate_Memory (
                     USB_MEM_TYPE_CACHED,
                     (UNSIGNED)(aud_ctrl_info->si_count),
                     (VOID **) &aud_ctrl_info->si_list);

        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
            break;
        }
        /* Initializing the streaming interface list with the value 0x00.
         */
        memset(aud_ctrl_info->si_list ,
               0x00,
               aud_ctrl_info->si_count);

        /* Store streaming interface index in the list. */
        for(id = 0; id < aud_ctrl_info->si_count; id++)
        {
            aud_ctrl_info->si_list[id] =
            cs_descr[NU_AUDH_HDR_SI_INF_OFSET + id];
        }

        /* Parse all the entities related to this Audio Control
         * Interface(ACI).
         */
        status = NU_AUDH_Parse_Entities(pcb_audio_dev, aud_ctrl_info);
        if(status != NU_SUCCESS)
        {
            /* Failed to parse some entity in this ACI. */
            break;
        }

        /* Update references in Audio Device Control block structure. */
        pcb_audio_dev->ac_cs_info = aud_ctrl_info;

    }while(0);

    /* In case of failure, deallocate all the memory allocated in this
     * function so far.
     */
    if(status  != NU_SUCCESS)
    {
        if(aud_ctrl_info)
        {
            if(aud_ctrl_info->si_list)
            {
                /* Deallocate memory associated with streaming interface
                 * list.
                 */
                status = USB_Deallocate_Memory(aud_ctrl_info->si_list);
            }
            /* Deallocate memory associated with audio control info. */
            status = USB_Deallocate_Memory(aud_ctrl_info);
        }
    }
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Entities
*
* DESCRIPTION
*     This function parses all the class specific information related to
*     the entities present in ACI(Audio Control Interface).
*
* INPUTS
*     audio_dev                     Pointer to Audio Device control block.
*     ac_info                       Pointer to Audio Control information.
*
* OUTPUTS
*     NU_SUCCESS                    Indicates successful completion.
*     NU_AUDH_INVALID_CS_DSCR_TYPE  Class specific descriptor type invalid.
*     NU_AUDH_INVALID_CSD_SUB_TYPE  Class specific descriptor sub type invalid.
*     NU_AUDH_MEM_FAIL              Run time memory allocation failed.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Entities(NU_USBH_AUD_DEV   *audio_dev,
                              NU_USBH_AUD_CTRL  *ac_info)
{
    UINT8                       *cs_descr;
    UINT8                        idx,cur_len,src_id_offset;
    UINT8                        cur_dscr_tp;
    UINT8                        cur_dscr_sub_tp;
    UINT16                       processed_len;
    NU_USBH_AUD_ENTITY          *ent_info;

    STATUS                       status = NU_SUCCESS;

    /* Get class specific ACI descriptors header length. */
    processed_len = ac_info->hdr_len;

    /*Pointer to the class specific ACI descriptor. */
    cs_descr = audio_dev->cs_descr;

    /* Pointer to the first entity descriptor after ACI header. */
    cs_descr = cs_descr + processed_len;
    do
    {
        /* Entity descriptor length. */
        cur_len         = cs_descr[NU_AUDH_LENGTH_OFSET];

        /* Entity descriptor type. */
        cur_dscr_tp     = cs_descr[NU_AUDH_DSCR_TYPE_OFSET];

        /* Entity descriptor sub type. */
        cur_dscr_sub_tp = cs_descr[NU_AUDH_DSCR_SUB_TYPE_OFSET];

        /* ACI descriptor Processed so far. */
        processed_len   = processed_len + cur_len;

        /* The next descriptor exists within the total length defined by
         * the header.
         */
        if(processed_len > ac_info->tot_len)
        {
            status = NU_AUDH_ENT_SIZE_OVERFLOW;
            break;
        }

        /* Validate descriptor type. */
        if(cur_dscr_tp != NU_AUDH_CS_INTERFACE)
        {
            status = NU_AUDH_INVALID_CS_DSCR_TYPE;
            break;
        }

        /* Validate descriptor sub type. */
        if(!NU_AUDH_IS_VALID_ACI_SUB_TYPE(cur_dscr_sub_tp))
        {
            status = NU_AUDH_INVALID_CSD_SUB_TYPE;
            break;
        }

        /* Allocate Memory to entity info structure. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                   (UNSIGNED)sizeof(NU_USBH_AUD_ENTITY),
                                   (VOID**)&ent_info);
        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
            break;
        }

        /* Initializing the whole block to 0x00 value. */
        memset(ent_info,
               0x0,
               sizeof(NU_USBH_AUD_ENTITY));

        /* Fill in entity info structure. */
        ent_info->ent_id       = cs_descr[NU_AUDH_DSCR_SUB_TYPE_OFSET + 1];
        ent_info->ent_dscr_len = cur_len;
        ent_info->ent_tp       = (AUDH_ENTITY_TYPE)cur_dscr_sub_tp;

        /* Will be filled according to the type of terminal or unit.
         * Currently set following entries to null.
         */
        ent_info->ent_sub_tp = (AUDH_ENTITY_SUB_TP)0;
        ent_info->succ_count = 0;
        ent_info->succ_list  = NU_NULL;

        /* Update the max_ent_id if needed. */
        if(ent_info->ent_id > ac_info->max_ent_id)
        {
            /* Update the max entity ID with the recent one. */
            ac_info->max_ent_id = ent_info->ent_id;
        }

        /* Is it a terminal? */
        if(NU_AUDH_IS_TERMINAL(ent_info))
        {
            /* Parse Terminal (Input/Output). */
            status = NU_AUDH_Parse_Terminal( cs_descr,
                                             ent_info,
                                             audio_dev);
        }

        /* Units. */
        else
        {

             /* Number of input pins will be one in case of Feature Unit and
              * vary otherwise.
              */
             ent_info->ip_pins_count =
                    (UINT8) ((cur_dscr_sub_tp == (UINT8)AUDH_FEATURE_UNIT)?
                    1:
                    cs_descr[NU_Input_Pins_Count_Offset[cur_dscr_sub_tp]]);
        }

        /* Update the fields common to all the entities. Number of input
         * pins are zero in case of input terminal so no need to allocate
         * memory.
         */
        if(ent_info->ip_pins_count)
        {

            /* Allocate Memory to predecessor list. */
            status = USB_Allocate_Memory(
                                  USB_MEM_TYPE_CACHED,
                                  (UNSIGNED)ent_info->ip_pins_count + 1,
                                  (VOID**)&(ent_info->pred_list));

            if(status != NU_SUCCESS)
            {
                status = NU_AUDH_MEM_FAIL;
                break;
            }

            /* Initializing the whole block to 0x00 value. */
            memset(ent_info->pred_list,
                   0x0,
                   ent_info->ip_pins_count + 1);

            /* Get the first Source ID for Units and output terminal. */
            src_id_offset = NU_Src_Id_Offset[cur_dscr_sub_tp];
            for(idx = 1; idx<= ent_info->ip_pins_count; idx++)
            {
                ent_info->pred_list[idx] = cs_descr[src_id_offset + idx - 1];
            }

            if(!NU_AUDH_IS_TERMINAL(ent_info))
            {

                /* Call appropriate UNIT parsing function depending on
                 * UNIT type.
                 */
                status = NU_AUDH_Unit_Parse_Fptr[cur_dscr_sub_tp](
                                                audio_dev,
                                                cs_descr,
                                                ent_info);
                if(status != NU_SUCCESS)
                {
                     break;
                }
            }
        }

        /* Store the Entity information here. */
        ac_info->ent_id_info[ent_info->ent_id] = ent_info;

        /* Go to the parsing of the next entity descriptor. */
        cs_descr += cur_len;
    }while(processed_len != ac_info->tot_len);

    /* If not successful, Deallocate all the memory allocated so far by
     * this function.
     */
    if(status != NU_SUCCESS)
    {
        for( idx = 0; idx <= ac_info->max_ent_id; idx++)
        {
            ent_info = ac_info->ent_id_info[idx];

            /* Skip if no memory is allocated. */
            if(!(ent_info))
            {
                continue;
            }
            if(ent_info->pred_list)
            {
                /* Deallocate predecessor list. */
                status = USB_Deallocate_Memory(ent_info->pred_list);
            }
            /* Deallocate entity info structure. */
            status = USB_Deallocate_Memory(ent_info);
        }
    }

    /* Entities are parsed successfully. */
    else
    {
        /* Update and validate connection information. */
        status = NU_AUDH_Updt_Vldt_Conn(audio_dev, ac_info);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Terminal
*
* DESCRIPTION
*     This function parses the terminal(both input and output terminal)
*     descriptors.
*
* INPUTS
*     audio_dev                   Pointer to Audio Device control block.
*     cs_descr                    Pointer to the raw descriptor.
*     ent_info                    Pointer to Entity information structure.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_AUDH_INVALID_TERM_SIZE   Terminal descriptor size invalid.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Terminal(UINT8              *cs_descr,
                              NU_USBH_AUD_ENTITY *ent_info,
                              NU_USBH_AUD_DEV    *audio_dev )
{

    STATUS            status = NU_SUCCESS;

    do
    {

        /* Make sure that terminal sizes are valid one. */
        if(!NU_AUDH_IS_TERM_SIZE_VALID(ent_info->ent_tp,
                                       ent_info->ent_dscr_len))
        {
            status = NU_AUDH_INVALID_TERM_SIZE;
            break;
        }

        /* Get Terminal Type. */
        ent_info->ent_sub_tp = (AUDH_ENTITY_SUB_TP)
        ((cs_descr[NU_AUDH_TERM_TYPE_OFSET +1] <<8) |
         (cs_descr[NU_AUDH_TERM_TYPE_OFSET]));

        /* Terminal being parsed is Input Terminal or Output Terminal. */
        if(ent_info->ent_tp == AUDH_INPUT_TERMINAL)
        {
            /* Currently streaming interface associated with this terminal
             * is not known. It will be updated at the end of
             * NU_AUDH_Parse_AC_Dscr.
             */
             /* No input pin is present in case of Input Terminal. */
             ent_info->ip_pins_count = 0;
             /* No predecessor exist in case of input terminal. */
             ent_info->pred_list  = NU_NULL;
        }
        else
        {
            /* Output terminal has one Input pin. */
            ent_info->ip_pins_count  = 1;
        }
    }while(0);

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Mixer_Unit
*
* DESCRIPTION
*     This function Parses the Mixer Unit descriptor.
*
* INPUTS
*     audio_dev                 Pointer to Audio Device control block.
*     cs_descr                  Pointer to Class specific descriptor info.
*     ent_info                  Pointer to Entity information.
*
* OUTPUTS
*
*     NU_SUCCESS                Indicates successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Mixer_Unit(NU_USBH_AUD_DEV     *audio_dev,
                                UINT8               *cs_descr,
                                NU_USBH_AUD_ENTITY  *ent_info)
{
    /* Currently not supporting anything for Mixer controls. */
    return NU_SUCCESS;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Feature_Unit
*
* DESCRIPTION
*     This function Parses the Feature Unit descriptor.
*
* INPUTS
*     audio_dev                Pointer to Audio Device control block.
*     cs_descr                 Pointer to Class specific descriptor info.
*     ent_info                 Pointer to Entity information.
*
* OUTPUTS
*     NU_SUCCESS                        Indicates successful completion.
*     NU_AUDH_MEM_FAIL                  Run time memory allocation failed.
*     NU_AUDH_INVALID_FEATURE_CTRL_SIZE Feature Unit control size invalid
*
**************************************************************************/
STATUS NU_AUDH_Parse_Feature_Unit(NU_USBH_AUD_DEV    *audio_dev,
                                  UINT8              *cs_descr,
                                  NU_USBH_AUD_ENTITY *ent_info)
{
    NU_USBH_AUD_FEATURE *feature_info = NU_NULL;
    UINT8                ctrl_size;
    STATUS               status = NU_SUCCESS;

    /* Get Control size. */
    ctrl_size = cs_descr[NU_AUDH_FU_CTRL_SIZE_OFSET];

    /* Check whether the control size is supported. */
    if((ctrl_size < NU_AUDH_MIN_FEATURE_CTRL_SIZE) ||
       (ctrl_size > NU_AUDH_MAX_FEATURE_CTRL_SIZE))
    {
        status = NU_AUDH_INVALID_FU_CTRL_SIZE;
    }
    else
    {
        /* Allocate resources to hold information specific to the Feature
         * Unit.
         */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                   (UNSIGNED)sizeof(NU_USBH_AUD_FEATURE),
                                   (VOID**)&feature_info);

        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
        }
        else
        {
              /* Initializing the structure with the value 0x00. */
            memset(feature_info,
                   0x0,
                   sizeof(NU_USBH_AUD_FEATURE));

            /* Update the feature unit control size info. */
            feature_info->ctrl_size = ctrl_size;

            /* Update control list pointer from raw descriptor. */
            feature_info->ctrls_list = cs_descr +
                                       NU_AUDH_FU_CTRL_SIZE_OFSET + 1;
            /* Update the parent pointer. */
            feature_info->ent_info = ent_info;
            ent_info->ent_tp_spec_info = feature_info;
        }
    }

    /* In case of failure, Deallocate all the memory allocated by this
     * function so far.
     */
    if((status != NU_SUCCESS) && (feature_info != NU_NULL))
    {
        status = USB_Deallocate_Memory(feature_info);
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Proc_Unit
*
* DESCRIPTION
*     This function Parses the Processing Unit descriptor.
*
* INPUTS
*     audio_dev               Pointer to Audio Device control block.
*     cs_descr                Pointer to Class specific descriptor info.
*     ent_info                Pointer to Entity information.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Proc_Unit(NU_USBH_AUD_DEV     *audio_dev,
                               UINT8               *cs_descr,
                               NU_USBH_AUD_ENTITY  *ent_info)
{
    /* Currently not supporting anything for processing controls. */
    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*     NU_Parse_Ext_Unit
*
* DESCRIPTION
*     This function Parses the Extension Unit descriptor.
*
* INPUTS
*     audio_dev               Pointer to Audio Device control block.
*     cs_descr                Pointer to Class specific descriptor info.
*     ent_info                Pointer to Entity information.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Ext_Unit(NU_USBH_AUD_DEV     *audio_dev,
                              UINT8               *cs_descr,
                              NU_USBH_AUD_ENTITY  *ent_info)
{

    /* Currently not supporting anything for extension unit controls. */
    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Sel_Unit
*
* DESCRIPTION
*     This function Parses the Selector unit descriptor.
*
* INPUTS
*     audio_dev             Pointer to Audio Device control block.
*     cs_descr              Pointer to Class specific descriptor info.
*     ent_info              Pointer to Entity information.
*
* OUTPUTS
*     NU_SUCCESS                    Indicates successful completion.
*     NU_AUDH_INVALID_SEL_UNIT_SIZE Invalid selector unit size.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Sel_Unit(NU_USBH_AUD_DEV    *audio_dev,
                              UINT8              *cs_descr,
                              NU_USBH_AUD_ENTITY *ent_info)
{
    STATUS status = NU_SUCCESS;

    /* Is Selector Unit descriptor valid? */
    if(cs_descr[NU_AUDH_LENGTH_OFSET] >
       (NU_AUDH_CONST_SEL_UNIT_SIZE + ent_info->ip_pins_count))
    {
        /* Invalid selector unit size. */
        status = NU_AUDH_INVALID_SEL_UNIT_SIZE;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Parse_Invalid_Unit
*
* DESCRIPTION
*     This is a dummy function which is registered in the table for unit
*     descriptors parsing functions. This is registered for each of the
*     invalid unit descriptors
* INPUTS
*     audio_dev                  Pointer to Audio Device control block.
*     cs_descr                   Pointer to Class specific descriptor info.
*     ent_info                   Pointer to Entity information.
*
* OUTPUTS
*     NU_AUDH_INVALID_UNIT_TYPE  Unit type is invalid.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Invalid_Unit(NU_USBH_AUD_DEV    *audio_dev,
                                  UINT8              *cs_descr,
                                  NU_USBH_AUD_ENTITY *ent_info)
{
    return NU_AUDH_INVALID_UNIT_TYPE;
}

/**************************************************************************
* FUNCTION
*     NU_Aud_Parse_Audio_Strm_Infs
*
* DESCRIPTION
*     This function is responsible for parsing the Audio class specific
*     information related to all the Streaming interfaces that belongs to
*     the specified ACI.
*
* INPUTS
*     as_info                       Pointer to Audio streaming information.
*     pcb_audio_dev                 Pointer to Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                    Indicates successful completion.
*     NU_AUDH_ASID_SUB_TYPE_INVALID ASI descriptor sub type invalid.
*     NU_AUDH_ASI_DSCR_TYPE_INVALID ASI descriptor type invalid.
*     NU_AUDH_LINK_TERM_NOT_FOUND   Invalid terminal link.
*     NU_AUDH_INVALID_AUD_FMTS_TYPE Invalid format type neither type-1,2
*                                   or 3.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Audio_Strm_Infs(AUDH_CS_ASI_INFO   *as_info,
                                     NU_USBH_AUD_DEV    *pcb_audio_dev)
{
    UINT8               *cs_descr,cur_dscr_type;
    UINT8                cur_dscr_sub_type,fmt_type;
    UINT32               len_cs_descr;
    STATUS               status = NU_SUCCESS;
    NU_USBH_AUD_CTRL    *ac_info;
    NU_USBH_AUD_ENTITY  *ent_info;

    /* Get Pointer to Audio Control Interface information.*/
    ac_info  =  pcb_audio_dev->ac_cs_info;
    do
    {
        /* Get class specific Streaming Interface descriptor. */
        status = NU_USB_ALT_SETTG_Get_Class_Desc(as_info->op_alt_sttg,
                                        &cs_descr,
                                        &len_cs_descr);

        cur_dscr_type = cs_descr[NU_AUDH_DSCR_TYPE_OFSET];
        cur_dscr_sub_type = cs_descr[NU_AUDH_DSCR_SUB_TYPE_OFSET];

        /* Basic class specific descriptor validation: Check if the type
         * and sub types are valid.
         */
        if(cur_dscr_type != NU_AUDH_CS_INTERFACE)
        {
            status = NU_AUDH_ASI_DSCR_TYPE_INVALID;
            break;
        }
        if(cur_dscr_sub_type != NU_AUDH_AS_GENERAL)
        {
            status = NU_AUDH_ASID_SUB_TYPE_INVALID;
            break;
        }

        /* Terminal link related validation: Get terminal link id from
         * raw descriptor.
         */
        as_info->term_link = cs_descr[NU_AUDH_ASI_TERM_LINK_OFSET];

        /* Get entity with entity id as that of terminal link. */
        ent_info = ac_info->ent_id_info[as_info->term_link];
        as_info->term_link_tp     = ent_info->ent_tp;

        /* Entity type should be of terminal type. */
        if(!NU_AUDH_IS_TERMINAL(ent_info))
        {
            status = NU_AUDH_LINK_TERM_NOT_FOUND;
            break;
        }

        /* Format tag obtained from raw descriptor. */
        as_info->fmt_tag =
        (AUDH_FORMAT_TAG)cs_descr[NU_AUDH_ASI_FMT_TAG_OFSET];
        as_info->fmt_tag =
                          (AUDH_FORMAT_TAG)((as_info->fmt_tag) |
                          (cs_descr[NU_AUDH_ASI_FMT_TAG_OFSET+1]<< 8 ));

        /* Format descriptor follows exactly after the class specific
         * ASI info.
         */
        cs_descr = cs_descr + NU_AUDH_ASI_DSCR_SIZE;

        /* We need the Most significant Nibble of the two byte format tag
         * to find out, to which group (TYPE-I, II or III) this format
         * belongs to.
         */
        fmt_type = (UINT8)((AUDH_FORMAT_TAG)as_info->fmt_tag >> 8);
        if(fmt_type > NU_AUDH_MAX_FMT_TYPE)
        {
            /* Invalid format TYPE. It is none of TYPE-1, 2 or 3. */
            status = NU_AUDH_INVALID_AUD_FMTS_TYPE;
            break;
        }

        /* Call the respective parse function. */
        status = NU_AUDH_Fmt_Parse_Fptr[fmt_type](cs_descr,
                                                  as_info,
                                                  pcb_audio_dev);
    }while(0);

    return status;

}

/**************************************************************************
* FUNCTION
*     NU_Aud_Parse_Type_I_Dscr
*
* DESCRIPTION
*     This function parses the type-I format descriptor.
*
* INPUTS
*     asi_info                      Pointer to Audio streaming information.
*     scan                          Pointer to class specific descriptor
*                                   information.
*     pcb_audio_dev                 Pointer to Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                    Successful completion.
*     NU_AUDH_INVALID_TID_TYPE      Invalid Type ID.
*     NU_AUDH_INVALID_TID_SUB_TYPE  Invalid Sub Type.
*     NU_AUDH_TYPE_I_FMT_MATCH_FAIL Invalid Format Match.
*     NU_AUDH_TIFD_INVALID_SUB_TYPE Invalid Format:Not Type 1 Format.
*     NU_AUDH_MEM_FAIL              Run time memory allocation failed.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Type_I_Dscr(UINT8               *scan,
                                 AUDH_CS_ASI_INFO    *cs_asi_info,
                                 NU_USBH_AUD_DEV     *pcb_audio_dev)
{
    UINT8                    fmt_tag,i;
    STATUS                   status = NU_SUCCESS;
    AUDH_TYPE1_DSCR_INFO    *fmt_tp_info = NU_NULL;

    /* Reset the pointer that is to be updated after successful parsing. */
    cs_asi_info->fmt_tp_dscr = NU_NULL;
    do
    {
        /* Check the descriptor type and sub type. */
        if(scan[NU_AUDH_DSCR_TYPE_OFSET] != NU_AUDH_CS_INTERFACE)
        {
            status = NU_AUDH_INVALID_TID_TYPE;
            break;
        }
        if(scan[NU_AUDH_DSCR_SUB_TYPE_OFSET] != NU_AUDH_FORMAT_TYPE)
        {
            status = NU_AUDH_INVALID_TID_SUB_TYPE;
            break;
        }

        /* Check the format tag is matching with the format tag of the ASI.
         */
        fmt_tag = scan[NU_AUDH_FMT_TYPE_OFSET];
        if(fmt_tag == (cs_asi_info->fmt_tag >> NU_AUDH_GROUP_TYPE_POS))
        {
            status = NU_AUDH_TYPE_I_FMT_MATCH_FAIL;
            break;
        }

        /* Check the sub type of the format. */
        if(!NU_AUDH_IS_TYPE_I_FORMAT(fmt_tag))
        {
            status = NU_AUDH_TIFD_INVALID_SUB_TYPE;
            break;
        }

        /* Allocate the memory to hold the format type information. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                    (UNSIGNED)sizeof(AUDH_TYPE1_DSCR_INFO),
                                    (VOID**)&fmt_tp_info);

        if(status != NU_SUCCESS)
        {
            status = NU_AUDH_MEM_FAIL;
            break;
        }

        /* Store the format type information. */
        fmt_tp_info->chnls_count  = scan[NU_AUDH_FMT_TYPE_CHNLCNT_OFSET];
        fmt_tp_info->sub_frm_size = scan[NU_AUDH_FMT_TYPE_SUBFRM_OFSET];
        fmt_tp_info->bit_res      = scan[NU_AUDH_FMT_TYPE_BITRES_OFSET];
        fmt_tp_info->smpl_tp      = scan[NU_AUDH_FTYP1_SMPL_TP_OFSET];

        /* In case of continuous sampling frequencies. */
        if(fmt_tp_info->smpl_tp == 0)
        {
            /* Minimum sampling frequency. */
            fmt_tp_info->min_smpl_freq =(
                   (scan[NU_AUDH_FTM_SMPL_INFO_OFSET+2]<< 16)|
                   (scan[NU_AUDH_FTYP1_SMPL_TP_OFSET+1]<<8)|
                   (scan[NU_AUDH_FTYP1_SMPL_TP_OFSET]));

            /* Maximum sampling frequency. */
            fmt_tp_info->max_smpl_freq = (
                   (scan[NU_AUDH_FTM_SMPL_INFO_OFSET+5]<< 16)|
                   (scan[NU_AUDH_FTYP1_SMPL_TP_OFSET+4]<<8)|
                   (scan[NU_AUDH_FTYP1_SMPL_TP_OFSET+3]));

            /* Not valid in case of continuous frequencies. */
            fmt_tp_info->smpl_freq_count = 0;
            fmt_tp_info->smpl_freq_list  = NU_NULL;
        }

        /* In case of discrete stapling frequencies. */
        if(fmt_tp_info->smpl_tp > 0)
        {
            fmt_tp_info->smpl_freq_count   = fmt_tp_info->smpl_tp;

            /* Not valid in case of discrete frequencies, make following
             * entries NULL.
             */
            fmt_tp_info->min_smpl_freq = 0;
            fmt_tp_info->max_smpl_freq = 0;


            /* Allocate Memory for discrete sampling frequency list */
            status = USB_Allocate_Memory(
                             USB_MEM_TYPE_CACHED,
                             (UNSIGNED)(fmt_tp_info->smpl_freq_count * sizeof(UINT32)),
                             (VOID**)&fmt_tp_info->smpl_freq_list);

            if(status == NU_SUCCESS)
            {
                  /* Initializing the frequency list with the value 0x00. */
                memset(fmt_tp_info->smpl_freq_list,
                       0,
                       fmt_tp_info->smpl_freq_count * sizeof(UINT32));
            

                /* Fill sampling frequency list with discrete values. */
                for(i = 0; i < fmt_tp_info->smpl_freq_count ; i++)
                {
                    fmt_tp_info->smpl_freq_list[i] = (
                    (scan[NU_AUDH_FTM_SMPL_INFO_OFSET+2+(i*3)] << 16)|
                    (scan[NU_AUDH_FTM_SMPL_INFO_OFSET+1+(i*3)] << 8)|
                    (scan[NU_AUDH_FTM_SMPL_INFO_OFSET+(i*3)]));
                }
            }

        }
    } while(0);

    /* In case of failure, Deallocate all the memory allocated by this
     * function so far!
     */
    if(status != NU_SUCCESS)
    {
        if(fmt_tp_info != NU_NULL)
        {
            if(fmt_tp_info->smpl_freq_list)
            {
               /* Deallocate memory for sampling frequency list. */
               status = USB_Deallocate_Memory(fmt_tp_info->smpl_freq_list);
            }
            /* Deallocate memory for sampling type info. */
            status = USB_Deallocate_Memory(fmt_tp_info);
        }
    }
    else
    {
        /* Update format type information. */
        cs_asi_info->fmt_tp_dscr = fmt_tp_info;
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_Aud_Parse_Type_II_Dscr
*
* DESCRIPTION
*     This function parses the type-II format descriptors.
*
* INPUTS
*     asi_info                    Pointer to Audio streaming information.
*     scan                        Pointer to class specific information.
*     pcb_audio_dev               Pointer to Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                  Successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Type_II_Dscr(UINT8             *scan,
                                  AUDH_CS_ASI_INFO  *asi_info,
                                  NU_USBH_AUD_DEV   *pcb_audio_dev)
{
  return NU_SUCCESS;
}
/**************************************************************************
* FUNCTION
*     NU_Aud_Parse_Type_III_Dscr
*
* DESCRIPTION
*     This function parses the type-III format descriptors
*
* INPUTS
*     asi_info                   Pointer to Audio streaming information.
*     scan                       Pointer to class specific information.
*     pcb_audio_dev              Pointer to Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                 Successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Parse_Type_III_Dscr(UINT8            *scan,
                                   AUDH_CS_ASI_INFO *asi_info,
                                   NU_USBH_AUD_DEV  *pcb_audio_dev)
{
  return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Updt_Vldt_Conn
*
* DESCRIPTION
*     This function validates the source IDs of all the descriptors,
*     builds the successor information for each of the entities and then
*     validates the successor IDs. This makes sure that all the units
*     source at some input terminal and sink at some output terminal.
*
* INPUTS
*     audio_dev                 Pointer to Audio Device control block.
*     ac_info                   Pointer to Audio Control information.
*
* OUTPUTS
*     NU_SUCCESS                Indicates successful completion.
*     NU_AUDH_MEM_FAIL          Memory Allocation Failed.
*
**************************************************************************/
STATUS  NU_AUDH_Updt_Vldt_Conn(NU_USBH_AUD_DEV  *aud_dev,
                               NU_USBH_AUD_CTRL *ac_info)
{
    UINT8                   curr_id[NU_AUDH_MAX_POSSIBLE_ENTITIES],idx1;
    NU_USBH_AUD_ENTITY    **ent_arr;
    NU_USBH_AUD_ENTITY     *ent_info;
    STATUS                  status = NU_SUCCESS;


    /* Entities information data base. */
    ent_arr = ac_info->ent_id_info;

    do
    {
        /* Calculate successor count for each entity. */
        status = NU_AUDH_Calc_Succ_Cnt(ac_info);
        if(status != NU_SUCCESS)
        {
            break;
        }
        /* This ensures that all the entities source from some Input
         * Terminal only.
         */
        for(idx1 = 0; idx1 <= ac_info->max_ent_id; idx1++)
        {
            /* Successor index for every entity starts at one. This is a
             * temporary variable to keep track of the index where the
             * next successor of the entity (with ent_id idx1) need to be
             * stored.
             */
             curr_id[idx1] = 1;

            /* Is there any entity with this entity id? */
            ent_info = ent_arr[idx1];
            if(ent_info == NU_NULL)
            {
                continue;
            }
            if(ent_info->ent_tp == AUDH_OUTPUT_TERMINAL)
            {
                /* Output terminal entity doesn't contain any successors.*/
                continue;
            }
            else
            {
                /* For Input Terminal or any other unit, allocate memory
                 * for successors list.
                 */
                status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     (UNSIGNED)ent_info->succ_count+1,
                                     (VOID**)&ent_info->succ_list);
                if(status != NU_SUCCESS)
                {
                    status = NU_AUDH_MEM_FAIL;
                    break;
                }

                /* Initializing the successor list with the value 0x00. */
                memset(ent_info->succ_list,
                       0x00,
                       ent_info->succ_count+1);
            }
        }
        if(idx1 <= ac_info->max_ent_id)
        {
            /* Error in allocating memory for storing the successor list. */
            break;
        }

        /* Fill up the successor and channel list. */
        status = NU_AUDH_Update_Succ(ac_info , curr_id);
    }while(0);

    /* Deallocate all the memory allocated in this function so far, in case
     * of failure.
     */
    if(status != NU_SUCCESS)
    {
        for(idx1 = 0; idx1 <= ac_info->max_ent_id; idx1++)
        {
            ent_info = ent_arr[idx1];
            if((ent_info != NU_NULL) && (ent_info->succ_list))
            {
                /* Deallocate memory fir successor list. */
                status = USB_Deallocate_Memory(ent_info->succ_list);
            }
        }
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Calc_Succ_Cnt
*
* DESCRIPTION
*     This function calculates the successor count for each of the entity.
*
* INPUTS
*     ac_info                  Pointer to Audio Control Interface specific
*                              information.
*
* OUTPUTS
*     NU_SUCCESS               Indicates successful completion.
*     NU_AUDH_INVALID_SRC_ID   Invalid source ID.
*
**************************************************************************/
STATUS  NU_AUDH_Calc_Succ_Cnt(NU_USBH_AUD_CTRL *ac_info)
{
    UINT8                src_id;
    UINT16               idx1,idx2;
    STATUS               status = NU_SUCCESS;
    NU_USBH_AUD_ENTITY  *ent_info,*src_ent;

    /* Go through all the entities. */
    for(idx1 = 0;(idx1 <= ac_info->max_ent_id); idx1++)
    {
        if(status != NU_SUCCESS)
        {
            break;
        }
        /* Get Entity info. */
        ent_info = ac_info->ent_id_info[idx1];

        /* Is it a valid entity? */
        if(ent_info == NU_NULL)
        {
            continue;
        }
        if(ent_info->ent_tp == AUDH_INPUT_TERMINAL)
        {
            /* Input Terminal doesn't contain source IDs. No need to
             * proceed.
             */
            continue;
        }

        /* Output Terminal or Units. */
        else
         {
             /* For each Input Pin validate the source ID. */
             for(idx2 = 1; idx2 <= ent_info->ip_pins_count; idx2++)
             {
                 /* Gets source ID for respective pins. */
                 src_id = ent_info->pred_list[idx2];

                 /* Increment the successor of the source src_id. Only non
                  * Output Terminals can have the successors.
                  */
                 src_ent = ac_info->ent_id_info[src_id];
                 if(src_ent->ent_tp != AUDH_OUTPUT_TERMINAL)
                 {
                      src_ent->succ_count++;
                 }
                 else
                 {
                     status =    NU_AUDH_INVALID_SRC_ID;
                 }
            }
        }
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Update_Succ
*
* DESCRIPTION
*     This function updates successor as well as channel information .It
*     checks if there exists any entity with the entity ID equal to the
*     source ID and that entity is not the output terminal.
*
* INPUTS
*     ac_info                       Pointer to the Audio Control structure.
*     curr_id                       Array of pointers for each entity.
*
* OUTPUTS
*     NU_SUCCESS                    Indicates successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Update_Succ(
                         NU_USBH_AUD_CTRL *ac_info,
                         UINT8            *curr_id)
{
    UINT8               id,idx;
    UINT8               src_id,input_pin;
    NU_USBH_AUD_ENTITY *ent_info,**ent_arr;
    NU_USBH_AUD_ENTITY *pred_ent;

    STATUS status = NU_SUCCESS;
    /* Get entities info data base. */
    ent_arr = ac_info->ent_id_info;

    /* Update the successor list for each of the entity. */
    for(id = 0; id <= ac_info->max_ent_id; id++)
    {
        ent_info = ac_info->ent_id_info[id];

        /* Is there any entity with this entity ID. */
        if(ent_info == NU_NULL)
        {
            continue;
        }

        if(ent_info->ent_tp == AUDH_INPUT_TERMINAL)
        {

            /* No entity can have Input Terminal as successor, so skip. */
            continue;
        }

        /* For all the predecessors update the successor information with
         * current entity id.
         */
         for(input_pin = 1; input_pin <= ent_info->ip_pins_count; input_pin++)
         {
             src_id   = ent_info->pred_list[input_pin];
             idx      = curr_id[src_id];
             pred_ent = ent_arr[src_id];
             if(pred_ent->ent_tp!= AUDH_OUTPUT_TERMINAL)
             {
                 pred_ent->succ_list[idx] = ent_info->ent_id;
             }

             curr_id[src_id]++;
        }
    }
    return status;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Free_Structures
*
* DESCRIPTION
*     This function frees memory acquired by the device during parsing
*     routines.
*
* INPUTS
*     p_curr_device           Pointer to the Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS              Memory deallocation successfully completed.
*
**************************************************************************/
STATUS NU_AUDH_Free_Structures( NU_USBH_AUD_DEV *p_curr_device )
{
    STATUS                status = NU_SUCCESS;
    UINT8                 entities_count,intf_count;
    UINT8                 alt_sttg;
    AUDH_TYPE1_DSCR_INFO *tp1_dscr;
    AUDH_CS_ASI_INFO     *cs_asi_info;

    /* Deallocate the memory associated with all the entities. */
    for(entities_count = 0; entities_count < NU_AUDH_MAX_POSSIBLE_ENTITIES;
        entities_count++)
    {
        if(p_curr_device->ac_cs_info)
        {
            /* Get entity info. */
            NU_USBH_AUD_ENTITY *ent_info =
            p_curr_device->ac_cs_info->ent_id_info[entities_count];

            /* Does entity info memory needs to be deallocated? */
            if(ent_info)
            {
                /* Deallocate predecessor list memory if acquired. */
                if(ent_info->pred_list)
                {
                    status = USB_Deallocate_Memory(ent_info->pred_list);
                }
                /* Deallocate successor list memory if acquired. */
                if(ent_info->succ_list)
                {
                    status = USB_Deallocate_Memory(ent_info->succ_list);
                }
                /* Deallocate entity specific info related memory if acquired.
                 */
                if(ent_info->ent_tp_spec_info)
                {
                    status = NU_Deallocate_Memory(ent_info->ent_tp_spec_info);
                }

                /* Deallocate entity info memory. */
                status = NU_Deallocate_Memory(ent_info);
            }
        }    
    }

    /* Deallocate memory associated with audio control class specific info
     * structure.
     */

    if(p_curr_device->ac_cs_info)
    {
        if(p_curr_device->ac_cs_info->si_list)
        {
            status = USB_Deallocate_Memory(p_curr_device->ac_cs_info->si_list);
        }
        status = USB_Deallocate_Memory(p_curr_device->ac_cs_info);
    }

    /* Release memory acquired by streaming interfaces info. */
    for(intf_count= 0; intf_count < NU_AUDH_MAX_STREAMING_INFS ;
        intf_count++)
    {
        if(p_curr_device->si_info[intf_count])
        {

           /* Release memory acquired by alternate setting info of
            * streaming interfaces.
            */
           for(alt_sttg = 0; alt_sttg < NU_USB_MAX_ALT_SETTINGS ;
               alt_sttg++)
           {
               /* Get class specific streaming interface info pointer.*/
               cs_asi_info =
               p_curr_device->si_info[intf_count]->cs_asi_info[alt_sttg];
               if(cs_asi_info)
               {

                   /* Get type I descriptor info pointer. */
                   tp1_dscr = cs_asi_info->fmt_tp_dscr;
                   if(tp1_dscr)
                   {

                      /* Release memory acquired by frequency list. */
                      if(tp1_dscr->smpl_freq_list)
                      {
                          status = USB_Deallocate_Memory(
                          tp1_dscr->smpl_freq_list);
                      }
                      /* Release memory acquired by type I descriptor
                       *info.
                       */
                      status = USB_Deallocate_Memory(tp1_dscr);
                   }
                   /* Release memory acquired by Class Specific ASI info.*/
                   status = USB_Deallocate_Memory(cs_asi_info);
               }

               /* Release memory acquired by streaming interface info
                * structure.
                */
               status = NU_Deallocate_Memory(
                                    p_curr_device->si_info[intf_count]);
            }
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Play_Audio
*
* DESCRIPTION
*     This is the entry function for playing task created as a result of
*     playing session opening for an audio device by application. This
*     function starts and manages playing sound on audio device. For this
*     it creates two IRPs to submit to host driver and through its flow
*     makes sure that host is always kept fed with at least two IRPs for
*     smooth and sustained data over ISO pipe. After completely
*     transmitting the data, it reports application of completion.
*
* INPUTS
*     pcb_aud_drvr           Pointer to Class driver.
*     pcb_device             Pointer to the audio device.
*
* OUTPUTS
*     None.
*
**************************************************************************/

VOID NU_AUDH_Play_Audio(UNSIGNED  pcb_aud_drvr,
                          VOID     *pcb_device)
{
    STATUS             status = NU_SUCCESS;

    NU_USBH_AUD_DEV    *pcb_aud_device  = (NU_USBH_AUD_DEV*)pcb_device;

    NU_AUDH_TRANSFER *transfer1;
    NU_AUDH_TRANSFER *transfer2;

    UINT32      suspend_option;
    UINT32      ply_length;
    UINT32      i;
    UINT32      j;
    UINT32      irp_count;
    UINT16 *           len_in = 0;
    UINT16             actual_trans = 0;
    UINT32              remainder;
    BOOLEAN             bWait_For_2nd_Transfer = NU_FALSE;
    UNSIGNED        actual_size = 0;

    /* Get play info from audio device structure. */
    AUDH_PLAY_INFORM    *play_info = &(pcb_aud_device->cb_play_inform);

    /* Allocate memory for ISO transfer 1*/
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 sizeof(NU_AUDH_TRANSFER),
                                (VOID **)&(transfer1));
    NU_USB_ASSERT(status == NU_SUCCESS);
    if (status == NU_SUCCESS)
    {
        /* Allocate memory for ISO transfer 2. */
	    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_AUDH_TRANSFER),
                                    (VOID **)&(transfer2));
        NU_USB_ASSERT(status == NU_SUCCESS);
        if(status != NU_SUCCESS)
        {
            USB_Deallocate_Memory(transfer1);
            return;
        }
    }
    else
    {
        return;
    }
    /* Initialize transfer1,2's irp_count to 0 */
    transfer1->irp_count = 0;
    transfer2->irp_count = 0;

    /* Create 1st IRP. */

    /* Packets per frame to be sent. */
    play_info->pkt_per_frame =
                    (pcb_aud_device->bus_speed == USB_SPEED_FULL)?
                     1000:8000;

    play_info->compensator = 1;
    remainder = (play_info->data_rate)%(play_info->pkt_per_frame);

    /* Divide data rate by a factor 2 if remainder is 200 and by
     * 4, if remainder is 400.
     */
    if(remainder == 200)
    {
        play_info->data_rate  /= 2;
        play_info->compensator = 2;
    }

    if(remainder == 400)
    {
        play_info->data_rate  /= 4;
        play_info->compensator = 4;
    }

    for(i=0;(i<NU_AUDH_NUM_PLAY_TRANS) && (status == NU_SUCCESS);i++)
    {
        /*
         * Prepare irps for first transfer.
         */
        status =  NU_USB_ISO_IRP_Create(
                           &(transfer1->iso_irp[i].irp),
                           0x00,
                           transfer1->iso_irp[i].buffer_ptrs_array,
                           transfer1->iso_irp[i].pkt_len_array,
                           transfer1->iso_irp[i].pkt_len_array,
                           NU_AUDH_ISO_OUT_IRP_Complete,
                           pcb_aud_device);
    }

    for(i=0;(i<NU_AUDH_NUM_PLAY_TRANS) && (status == NU_SUCCESS);i++)
    {
        /*
         * Prepare irps for second transfer
         */
        status =  NU_USB_ISO_IRP_Create(
                           &transfer2->iso_irp[i].irp,
                           0x00,
                           transfer2->iso_irp[i].buffer_ptrs_array,
                           transfer2->iso_irp[i].pkt_len_array,
                           transfer2->iso_irp[i].pkt_len_array,
                           NU_AUDH_ISO_OUT_IRP_Complete,
                           pcb_aud_device);
    }



    if(status == NU_SUCCESS)
    {
        for (;;)
        {
            status = NU_AUDH_Get_Events(pcb_aud_device,
                                        NU_AUDH_PLAY_TASK_TERMINATE,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                break;
            }

            if(bWait_For_2nd_Transfer == NU_TRUE)
            {
                suspend_option = NU_NO_SUSPEND;
            }

            else
            {
                suspend_option = NU_SUSPEND;
            }
            
            /*
             * Retrieve information of transfer1.
             */
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_play_inform.double_buff_queue),
                   &(transfer1->p_buffer),
                   (UNSIGNED)1,
                   &actual_size,
                   suspend_option);

            if(status == NU_SUCCESS)
            {
                status = NU_Receive_From_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       &(transfer1->buffer_length),
                       (UNSIGNED)1,
                       &actual_size,
                       suspend_option);
            }

            else
            {
                transfer1->buffer_length = 0;
            }

            if(status == NU_SUCCESS)
            {
                status = NU_Receive_From_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       &(transfer1->call_back),
                       (UNSIGNED)1,
                       &actual_size,
                       suspend_option);
            }

            else
            {
                transfer1->call_back = NU_NULL;
            }
    

            if(status == NU_SUCCESS)
            {
                ply_length = 0;

                /* Copies passed parameters. */
                transfer1->p_curr_tx_buffer = transfer1->p_buffer;
                transfer1->curr_tx_length   = transfer1->buffer_length;

                irp_count=0;

                /* Loops until all of requested data is transmitted. */
                while(transfer1->curr_tx_length > 0)
                {
                    /* Fill up 1st irp. */
                    NU_AUDH_Fill_ISO_Buffer(
                        &transfer1->iso_irp[irp_count].irp,
                        &transfer1->p_curr_tx_buffer,
                        &transfer1->curr_tx_length,
                        play_info,
                        transfer1->iso_irp[irp_count].pkt_len_array,
                        transfer1->iso_irp[irp_count].buffer_ptrs_array);

                    irp_count++;
                }

                transfer1->irp_count = 0;

                /*
                 * Submit irps for transfer1.
                 */
                for(i=0;(i<irp_count) && (status == NU_SUCCESS);i++)
                {
                    status = NU_USB_PIPE_Submit_IRP (
                             pcb_aud_device->pcb_iso_out_pipe,
                             (NU_USB_IRP *)&transfer1->iso_irp[i].irp);

                    if(status == NU_SUCCESS)
                    {
                        transfer1->irp_count++;
                    }
                }
            }

            else
            {
                status = NU_SUCCESS;
            }

            if(status == NU_SUCCESS)
            {
                /*
                 * Check if irps of transfer2 have been submitted.
                 */
                if(bWait_For_2nd_Transfer == NU_TRUE)
                {
                    /*
                     * Wait for irps of transfer2 to be completed.
                     */
                    for(i=0;
                        (i<transfer2->irp_count) && (status == NU_SUCCESS);
                        i++)
                    {
                        status = NU_Obtain_Semaphore(
                                            &pcb_aud_device->play_sem,
                                            NU_SUSPEND);
                    }

                    /*
                     * Accumulate effective length of irps of transfer2.
                     */
                    if(status == NU_SUCCESS)
                    {
                        ply_length = 0;

                        for(i=0;i<transfer2->irp_count;i++)
                        {
                            NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                            &transfer2->iso_irp[i].irp,
                                            &actual_trans);

                            NU_USB_ISO_IRP_Get_Actual_Lengths(
                                            &transfer2->iso_irp[i].irp,
                                            &len_in);

                            

                            for(j=0;j<actual_trans;j++)
                            {
                                ply_length += len_in[j];
                            }
                        }
                        
                        
                        transfer2->irp_count -= i;
                        
                    
                        

                        if(transfer2->call_back != NU_NULL)
                        {
                            /* Reporting application of completion through
                             * call back.
                             */
                            transfer2->call_back(pcb_aud_device,
                                    transfer2->p_buffer,
                                    status,
                                    (ply_length/play_info->sample_size)/
                                    play_info->channels);
                        }

                        bWait_For_2nd_Transfer = NU_FALSE;
                    }
                }
            }

            
            /*
             * Retrieve information of transfer2.
             */
            if(status == NU_SUCCESS)
            {
                status = NU_Receive_From_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       &(transfer2->p_buffer),
                       (UNSIGNED)1,
                       &actual_size,
                       NU_NO_SUSPEND);
            }

            else
            {
                transfer2->p_buffer = NU_NULL;
            }
            if(status == NU_SUCCESS)
            {
                status = NU_Receive_From_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       &(transfer2->buffer_length),
                       (UNSIGNED)1,
                       &actual_size,
                       NU_NO_SUSPEND);
            }

            else
            {
                transfer2->buffer_length = 0;
            }
            if(status == NU_SUCCESS)
            {
                status = NU_Receive_From_Queue(
                       &(pcb_aud_device->cb_play_inform.double_buff_queue),
                       &(transfer2->call_back),
                       (UNSIGNED)1,
                       &actual_size,
                       NU_NO_SUSPEND);
            }
            else
            {
                transfer2->call_back = NU_NULL;
            }
            if(status == NU_SUCCESS)
            {
                ply_length = 0;

                /* Copies passed parameters. */
                transfer2->p_curr_tx_buffer = transfer2->p_buffer;
                transfer2->curr_tx_length   = transfer2->buffer_length;

                irp_count=0;

                /* Loops until all of requested data is transmitted. */
                while(transfer2->curr_tx_length > 0)
                {
                    /* Fill up irps for transfer2. */
                    NU_AUDH_Fill_ISO_Buffer(
                        &transfer2->iso_irp[irp_count].irp,
                        &transfer2->p_curr_tx_buffer,
                        &transfer2->curr_tx_length,
                        play_info,
                        transfer2->iso_irp[irp_count].pkt_len_array,
                        transfer2->iso_irp[irp_count].buffer_ptrs_array);

                    irp_count++;
                }

                transfer2->irp_count = 0;

                /*
                 * Submit irps for transfer1.
                 */
                for(i=0;(i<irp_count) && (status == NU_SUCCESS);i++)
                {
                    status = NU_USB_PIPE_Submit_IRP (
                             pcb_aud_device->pcb_iso_out_pipe,
                             (NU_USB_IRP *)&transfer2->iso_irp[i].irp);

                    if(status == NU_SUCCESS)
                    {
                        transfer2->irp_count++;
                    }
                }

                if(status == NU_SUCCESS)
                {
                    bWait_For_2nd_Transfer = NU_TRUE;
                }
            }

            else
            {
                /*
                 * There is no transfer to be retrieved so wait for
                 * completion of transfer1 and reiterate.
                 */
                status = NU_SUCCESS;
            }

            if(status == NU_SUCCESS)
            {
                /*
                 * Wait for irps of transfer1 to be completed.
                 */
                for(i=0;
                    (i<transfer1->irp_count) && (status == NU_SUCCESS);
                    i++)
                {
                    status = NU_Obtain_Semaphore(
                                        &pcb_aud_device->play_sem,
                                        NU_SUSPEND);
                }

                /*
                 * Accumulate effective length of irps of transfer1.
                 */
                if(status == NU_SUCCESS)
                {
                    ply_length = 0;

                    for(i=0;i<transfer1->irp_count;i++)
                    {
                        NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                        &transfer1->iso_irp[i].irp,
                                        &actual_trans);

                        NU_USB_ISO_IRP_Get_Actual_Lengths(
                                        &transfer1->iso_irp[i].irp,
                                        &len_in);
                                                
                                                

                        for(j=0;j<actual_trans;j++)
                        {
                            ply_length += len_in[j];
                        }
                    }

                                        
                    transfer1->irp_count -= i;
                                                                                                                        

                    if(transfer1->call_back != NU_NULL)
                    {
                        /* Reporting application of completion through
                         * call back.
                         */
                        transfer1->call_back(pcb_aud_device,
                                    transfer1->p_buffer,
                                    status,
                                    (ply_length/play_info->sample_size)/
                                    play_info->channels);
                    }
                }
            }
        }
    }
    status = USB_Deallocate_Memory(transfer1);
    NU_USB_ASSERT(status == NU_SUCCESS);

    status = USB_Deallocate_Memory(transfer2);
    NU_USB_ASSERT(status == NU_SUCCESS);

    status = NU_AUDH_Set_Events(pcb_aud_device,
                                       NU_AUDH_PLAY_TASK_TERMINATED);
    NU_USB_ASSERT(status == NU_SUCCESS);
        
    

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Record_Audio_Single_Buffer
*
* DESCRIPTION
*     This is the entry function for recording task created as a result of 
*     recording session opening for an audio device by application. This 
*     function starts and manages recording sound on audio device. For this 
*     it creates 2 IRPs to submit to host driver and through its flow makes
*     sure that host is always kept fed with at least two IRPs for smooth 
*     and sustained data over ISO pipe. After completely receiving the 
*     data, it reports application of completion.
*
* INPUTS
*     pcb_aud_drvr           Pointer to audio Class driver control block.
*     pcb_device             Pointer to the Audio Device control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/

VOID   NU_AUDH_Record_Single_Buffer(UNSIGNED  pcb_aud_drvr,
                            VOID     *pcb_device)
{
    STATUS              status;
    NU_USBH_AUD_DEV    *pcb_aud_device = (NU_USBH_AUD_DEV*)pcb_device;
    USBH_AUD_SESSION_CONTEXT iso_irp1,iso_irp2;
    UINT16             *len_in, maxp, actual_trans = 0;
    UINT8              *p_curr_rx_buffer = NU_NULL;
    UINT8              *p_prepared_buffer = NU_NULL;
    UINT32              curr_rx_length;
    NU_USB_ENDP        *endpoint;
    UINT32              rec_length,i = 0, j = 0;
    UINT8               *psrc = NU_NULL;
    UINT8             **buffer_array_out = NU_NULL;
    BOOLEAN             bWait_For_2nd_Irp = NU_FALSE;
    UNSIGNED           actual_size = 0;
    AUDH_PLAY_INFORM   *record_info = &(pcb_aud_device->cb_record_inform);

    NU_USB_PIPE_Get_Endp(pcb_aud_device->pcb_iso_in_pipe, &endpoint);
    NU_USB_ENDP_Get_Max_Packet_Size(endpoint, &maxp);

    NU_Reset_Semaphore(&(pcb_aud_device->rec_sem),0);
    /* Create 1st IRP. */
    status =  NU_USB_ISO_IRP_Create(
                                   &iso_irp1.irp,
                                    0x00,
                                    iso_irp1.buffer_ptrs_array,
                                    iso_irp1.pkt_len_array,
                                    iso_irp1.pkt_len_array,
                                    NU_AUDH_ISO_IN_IRP_Complete,
                                    pcb_aud_device);

    if(status == NU_SUCCESS)
    {
        status =  NU_USB_ISO_IRP_Create(
                                       &iso_irp2.irp,
                                        0x00,
                                        iso_irp2.buffer_ptrs_array,
                                        iso_irp2.pkt_len_array,
                                        iso_irp2.pkt_len_array,
                                        NU_AUDH_ISO_IN_IRP_Complete,
                                        pcb_aud_device);
    }
    
   for (;;)
    {
        status = NU_AUDH_Get_Events(pcb_aud_device,
                                    NU_AUDH_REC_TASK_TERMINATE,
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            break;
        }
        
        else
        {
            status = NU_SUCCESS;    
        }
        
        if (status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                 &(pcb_aud_device->cb_record_inform.double_buff_queue),
                 &(pcb_aud_device->cb_record_inform.p_buffer),
                 (UNSIGNED)1,
                 &actual_size,
                 NU_SUSPEND);
        }
          
        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                 &(pcb_aud_device->cb_record_inform.double_buff_queue),
                 &(pcb_aud_device->cb_record_inform.buffer_length),
                 (UNSIGNED)1,
                 &actual_size,
                 NU_SUSPEND);
        }
        
        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                 &(pcb_aud_device->cb_record_inform.double_buff_queue),
                 &(pcb_aud_device->cb_record_inform.call_back),
                 (UNSIGNED)1,
                 &actual_size,
                 NU_SUSPEND);
        }        
            
        record_info->p_buffer = 
                        pcb_aud_device->cb_record_inform.p_buffer;
        record_info->buffer_length = 
                        pcb_aud_device->cb_record_inform.buffer_length;
        record_info->call_back = 
                        pcb_aud_device->cb_record_inform.call_back;

        /* Copies passed parameters. */
        rec_length = 0;
        p_curr_rx_buffer  = record_info->p_buffer;
        p_prepared_buffer = record_info->p_buffer;
        curr_rx_length    = record_info->buffer_length;

        /* Loops until all of requested data is received. */
        while(curr_rx_length > 0 && status == NU_SUCCESS)
        {    
            NU_AUDH_Packetize(&iso_irp1.irp,
                              &p_curr_rx_buffer,
                              &curr_rx_length,
                              maxp,
                              iso_irp1.pkt_len_array,
                              iso_irp1.buffer_ptrs_array);

            /* Submit 1st IRP. */
            status = NU_USB_PIPE_Submit_IRP(
                                 pcb_aud_device->pcb_iso_in_pipe,
                                  (NU_USB_IRP *)&iso_irp1.irp);

            if (bWait_For_2nd_Irp == NU_TRUE && status == NU_SUCCESS)
            {   
                status = NU_Obtain_Semaphore (
                                &(pcb_aud_device->rec_sem),
                                 NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    NU_USB_IRP_Get_Status(
                                    (NU_USB_IRP*)&iso_irp2.irp, 
                                    &status);
                    NU_USB_ISO_IRP_Get_Actual_Lengths(
                                    &iso_irp2.irp,
                                    &len_in);
                    NU_USB_ISO_IRP_Get_Buffer_Array  (
                                    &iso_irp2.irp,
                                    &buffer_array_out);
                    NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                    &iso_irp2.irp,
                                    &actual_trans);
                }

                for (i = 0; i < actual_trans; i++)
                {       
                    psrc = buffer_array_out[i];
                    for(j = 0; j < len_in[i]; j++)
                    {
                        p_prepared_buffer[rec_length++] = psrc[j];
                    }
                }

                bWait_For_2nd_Irp = NU_FALSE;
            }

            if(curr_rx_length > 0 && status == NU_SUCCESS)
            {
                NU_AUDH_Packetize   (&iso_irp2.irp,
                                     &p_curr_rx_buffer,
                                     &curr_rx_length,
                                     maxp,
                                     iso_irp2.pkt_len_array,
                                     iso_irp2.buffer_ptrs_array);

                 /* Submit 2nd IRP. */
                 status = NU_USB_PIPE_Submit_IRP(
                                     pcb_aud_device->pcb_iso_in_pipe,
                                      (NU_USB_IRP *)&iso_irp2.irp);

                if(status == NU_SUCCESS)
                {
                    bWait_For_2nd_Irp = NU_TRUE;
                }
            }

            /* Wait for 1st IRP completion. */
            if(status == NU_SUCCESS)
            {
                status = NU_Obtain_Semaphore (
                                &(pcb_aud_device->rec_sem),
                                 NU_SUSPEND);
            }

            if(status == NU_SUCCESS)
            {
                NU_USB_IRP_Get_Status(
                                    (NU_USB_IRP*)&iso_irp1.irp, 
                                    &status);
                NU_USB_ISO_IRP_Get_Actual_Lengths(
                                    &iso_irp1.irp,
                                    &len_in);
                NU_USB_ISO_IRP_Get_Buffer_Array  (
                                    &iso_irp1.irp,
                                    &buffer_array_out);
                NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                    &iso_irp1.irp,
                                    &actual_trans);
            }

            for (i = 0; i < actual_trans; i++)
            {       
                psrc = buffer_array_out[i];
                for(j = 0; j < len_in[i]; j++)
                {
                    p_prepared_buffer[rec_length++] = psrc[j];
                }
            }


        }

        if (bWait_For_2nd_Irp == NU_TRUE && status == NU_SUCCESS)
        {   
                    /* Wait for 2nd IRP's completion. */
                status = NU_Obtain_Semaphore (
                                &(pcb_aud_device->rec_sem),
                                 NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    NU_USB_IRP_Get_Status(
                                    (NU_USB_IRP*)&iso_irp2.irp, 
                                    &status);
                    NU_USB_ISO_IRP_Get_Actual_Lengths(
                                        &iso_irp2.irp,
                                        &len_in);
                    NU_USB_ISO_IRP_Get_Buffer_Array  (
                                    &iso_irp2.irp,
                                    &buffer_array_out);
                    NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                    &iso_irp2.irp,
                                    &actual_trans);
                }

                for (i = 0; i < actual_trans; i++)
                {           
                    psrc = buffer_array_out[i];
                    for(j = 0; j < len_in[i]; j++)
                    {
                        p_prepared_buffer[rec_length++] = psrc[j];
                    }
                }

                bWait_For_2nd_Irp = NU_FALSE;

        }

        /* Reset internal status value. */
        record_info->service_status = NU_AUDH_AVAILBLE;

        /* Reporting application of completion through call back. */
        record_info->call_back(pcb_aud_device,
                               record_info->p_buffer,
                               status,
                               (rec_length/record_info->sample_size)/
                               record_info->channels);
    }

    status = NU_AUDH_Set_Events(pcb_aud_device,
                                NU_AUDH_REC_TASK_TERMINATED);
}
/**************************************************************************
* FUNCTION
*
*     NU_AUDH_Record_Audio
*
* DESCRIPTION
*
*     This is the entry function for recording task created as a result of
*     recording session opening for an audio device by application. This
*     function starts and manages recording sound on audio device. This 
*     function is used when simultaneously two buffers are submitted by the 
*     application. It submits IRPs for two buffers at the same time. For 
*     this it creates multiple IRPs to submit to host driver and through its
*     flow makes sure that host is always kept fed with IRPs for smooth and 
*     sustained data over ISO pipe. After completely receiving the data, it 
*     reports application of completion.
*
* INPUTS
*     pcb_aud_drvr           Pointer to audio Class driver control block.
*     pcb_device             Pointer to the Audio Device control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/

VOID   NU_AUDH_Record_Audio(UNSIGNED  pcb_aud_drvr,
                            VOID     *pcb_device)
{
    STATUS              status = NU_SUCCESS;
    NU_USBH_AUD_DEV     *pcb_aud_device = (NU_USBH_AUD_DEV*)pcb_device;    
    UINT16              *len_in, maxp, actual_trans = 0;   
    UINT8               *p_prepared_buffer = NU_NULL;
    UINT8               *p_prepared_buffer2 = NU_NULL;    
    NU_USB_ENDP         *endpoint;
    UINT32              rec_length,i = 0, j = 0, k = 0;
    UINT8               *psrc = NU_NULL;
    UINT8               **buffer_array_out = NU_NULL;   
    UNSIGNED            actual_size = NU_NULL;
    AUDH_PLAY_INFORM    *record_info = &(pcb_aud_device->cb_record_inform); 
    NU_AUDH_TRANSFER *transfer1;
    NU_AUDH_TRANSFER *transfer2;
    BOOLEAN             bWait_For_2nd_Transfer = NU_FALSE;
    UINT32              suspend_option;
    UINT32              irp_count;
	
    /* Allocate memory for ISO transfer 1*/
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                sizeof(NU_AUDH_TRANSFER),
                               (VOID **)&(transfer1));
    NU_USB_ASSERT(status == NU_SUCCESS);
    if (status == NU_SUCCESS)
    {	
        /* Allocate memory for ISO transfer 2. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                    sizeof(NU_AUDH_TRANSFER),
                                    (VOID **)&(transfer2));
        NU_USB_ASSERT(status == NU_SUCCESS);
        if(status != NU_SUCCESS)
        {
            USB_Deallocate_Memory(transfer1);
            return;
        }
    }
    else
    {
        return;
    }

    /* Initialize transfer1,2's irp_count to 0 */
    transfer1->irp_count = 0;
    transfer2->irp_count = 0;        
        

    NU_USB_PIPE_Get_Endp(pcb_aud_device->pcb_iso_in_pipe, &endpoint);
    NU_USB_ENDP_Get_Max_Packet_Size(endpoint, &maxp);

    NU_Reset_Semaphore(&(pcb_aud_device->rec_sem),0);    
    

    for(i=0;(i<NU_AUDH_NUM_PLAY_TRANS) && (status == NU_SUCCESS);i++)
    {
        /*
         * Prepare irps for first transfer.
         */
        status =  NU_USB_ISO_IRP_Create(
                           &(transfer1->iso_irp[i].irp),
                           0x00,
                           transfer1->iso_irp[i].buffer_ptrs_array,
                           transfer1->iso_irp[i].pkt_len_array,
                           transfer1->iso_irp[i].pkt_len_array,
                           NU_AUDH_ISO_IN_IRP_Complete,
                           pcb_aud_device);
    }

    for(i=0;(i<NU_AUDH_NUM_PLAY_TRANS) && (status == NU_SUCCESS);i++)
    {
        /*
         * Prepare irps for second transfer
         */
        status =  NU_USB_ISO_IRP_Create(
                           &transfer2->iso_irp[i].irp,
                           0x00,
                           transfer2->iso_irp[i].buffer_ptrs_array,
                           transfer2->iso_irp[i].pkt_len_array,
                           transfer2->iso_irp[i].pkt_len_array,
                           NU_AUDH_ISO_IN_IRP_Complete,
                           pcb_aud_device);
    }
        

    for (;;)
    {
            
        status = NU_AUDH_Get_Events(pcb_aud_device,
                                    NU_AUDH_REC_TASK_TERMINATE,
                                    NU_NO_SUSPEND);
        
        if(status == NU_SUCCESS)
        {
            break;
        }

        if(bWait_For_2nd_Transfer == NU_TRUE)
        {
            suspend_option = NU_NO_SUSPEND;
        }

        else
        {
            suspend_option = NU_SUSPEND;
        }
                
        /*
         * Retrieve information of transfer1.
         */
        status = NU_Receive_From_Queue(
               &(pcb_aud_device->cb_record_inform.double_buff_queue),
               &(transfer1->p_buffer),
               (UNSIGNED)1,
               &actual_size,
               suspend_option);

        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   &(transfer1->buffer_length),
                   (UNSIGNED)1,
                   &actual_size,
                   suspend_option);
        }

        else
        {
            transfer1->buffer_length = 0;
        }

        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   &(transfer1->call_back),
                   (UNSIGNED)1,
                   &actual_size,
                   suspend_option);
        }

        else
        {
            transfer1->call_back = NU_NULL;
        }

        if(status == NU_SUCCESS)
        {
            rec_length = 0;

            /* Copies passed parameters. */
            transfer1->p_curr_tx_buffer = transfer1->p_buffer;
            transfer1->curr_tx_length   = transfer1->buffer_length;
            p_prepared_buffer = transfer1->p_buffer;

            irp_count=0;

            /* Loops until all of requested data is transmitted. */
            while(transfer1->curr_tx_length > 0)
            {
                NU_AUDH_Packetize ( &transfer1->iso_irp[irp_count].irp,
                                    &transfer1->p_curr_tx_buffer,
                                    &transfer1->curr_tx_length,
                                    maxp,
                                    transfer1->iso_irp[irp_count].pkt_len_array,
                                    transfer1->iso_irp[irp_count].buffer_ptrs_array);
                        
                irp_count++;
            }

            transfer1->irp_count = 0;

            /*
             * Submit irps for transfer1.
             */
            for(i=0;(i<irp_count) && (status == NU_SUCCESS);i++)
            {
                status = NU_USB_PIPE_Submit_IRP (
                          pcb_aud_device->pcb_iso_in_pipe,
                         (NU_USB_IRP *)&transfer1->iso_irp[i].irp);

                if(status == NU_SUCCESS)
                {
                    transfer1->irp_count++;
                                        
                }
            }
        }

        else
        {
            status = NU_SUCCESS;
        }

        if(status == NU_SUCCESS)
        {
            /*
             * Check if irps of transfer2 have been submitted.
             */
            if(bWait_For_2nd_Transfer == NU_TRUE)
            {
                rec_length = 0;
                /*
                 * Wait for irps of transfer2 to be completed.
                 */
                if(p_prepared_buffer2 != NU_NULL)
                {
                    for(i=0;
                        (i<transfer2->irp_count) && (status == NU_SUCCESS);
                        i++)
                    {
                        status = NU_Obtain_Semaphore(
                                            &pcb_aud_device->rec_sem,
                                            NU_SUSPEND);
                                        
                        if (status == NU_SUCCESS)
                        {
                            NU_USB_IRP_Get_Status(
                                   (NU_USB_IRP *)&transfer2->iso_irp[i].irp,
                                   &status);
                            NU_USB_ISO_IRP_Get_Actual_Lengths(
                                   &transfer2->iso_irp[i].irp,
                                   &len_in);
                            NU_USB_ISO_IRP_Get_Buffer_Array  (
                                   &transfer2->iso_irp[i].irp,
                                   &buffer_array_out);
                            NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                   &transfer2->iso_irp[i].irp,
                                   &actual_trans);
                        }

                        for (k = 0; k < actual_trans; k++)
                        {
                            psrc = buffer_array_out[k];
                            for(j = 0; j < len_in[k]; j++)
                            {
                                p_prepared_buffer2[rec_length++] = psrc[j];
                            }
                        }
                    }
                    transfer2->irp_count -= i;
            	}
 



                if(transfer2->call_back != NU_NULL)
                {
                     /* Reporting application of completion through
                     * call back.
                     */
                    transfer2->call_back(pcb_aud_device,
                                p_prepared_buffer2,
                                status,
                                (rec_length/record_info->sample_size)/
                                record_info->channels);
                }

                bWait_For_2nd_Transfer = NU_FALSE;
            
            }
        }

        /*
         * Retrieve information of transfer2.
         */
        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   &(transfer2->p_buffer),
                   (UNSIGNED)1,
                   &actual_size,
                   NU_NO_SUSPEND);
        }

        else
        {
            transfer2->p_buffer = NU_NULL;
        }
        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   &(transfer2->buffer_length),
                   (UNSIGNED)1,
                   &actual_size,
                   NU_NO_SUSPEND);
        }
        else
        {
            transfer2->buffer_length = 0;
        }

        if(status == NU_SUCCESS)
        {
            status = NU_Receive_From_Queue(
                   &(pcb_aud_device->cb_record_inform.double_buff_queue),
                   &(transfer2->call_back),
                   (UNSIGNED)1,
                   &actual_size,
                   NU_NO_SUSPEND);
        }

        else
        {
            transfer2->call_back = NU_NULL;
        }
        if(status == NU_SUCCESS)
        {
            rec_length = 0;
            /* Copies passed parameters. */
            transfer2->p_curr_tx_buffer = transfer2->p_buffer;
            transfer2->curr_tx_length   = transfer2->buffer_length;
            p_prepared_buffer2 = transfer2->p_buffer;
             

            irp_count=0;

            /* Loops until all of requested data is transmitted. */
            while(transfer2->curr_tx_length > 0)
            {
                NU_AUDH_Packetize (    &transfer2->iso_irp[irp_count].irp,
                                        &transfer2->p_curr_tx_buffer,
                                        &transfer2->curr_tx_length,
                                        maxp,
                                        transfer2->iso_irp[irp_count].pkt_len_array,
                                        transfer2->iso_irp[irp_count].buffer_ptrs_array);

                irp_count++;
            }

            transfer2->irp_count = 0;

            /*
             * Submit irps for transfer2.
             */
            for(i=0;(i<irp_count) && (status == NU_SUCCESS);i++)
            {
                status = NU_USB_PIPE_Submit_IRP (
                         pcb_aud_device->pcb_iso_in_pipe,
                         (NU_USB_IRP *)&transfer2->iso_irp[i].irp);

                if(status == NU_SUCCESS)
                {
                    transfer2->irp_count++;
                                 
                }
            }

            if(status == NU_SUCCESS)
            {
                bWait_For_2nd_Transfer = NU_TRUE;
            }
        }

        else
        {
            /*
             * There is no transfer to be retrieved so wait for
             * completion of transfer1 and reiterate.
             */
            status = NU_SUCCESS;
        }

        if(status == NU_SUCCESS)
        {
            rec_length = 0;
            /*
             * Wait for irps of transfer1 to be completed.
             */  
            if(p_prepared_buffer != NU_NULL)
            {
                for(i=0;
                     (i<transfer1->irp_count) && (status == NU_SUCCESS);
                     i++)
                {
                    status = NU_Obtain_Semaphore(
                                       &pcb_aud_device->rec_sem,
                                       NU_SUSPEND);
                 
                    if (status == NU_SUCCESS)
                    {
                        NU_USB_IRP_Get_Status(
                                       (NU_USB_IRP*)&transfer1->iso_irp[i].irp,
                                       &status);
                        NU_USB_ISO_IRP_Get_Actual_Lengths(
                                       &transfer1->iso_irp[i].irp,
                                       &len_in);
                        NU_USB_ISO_IRP_Get_Buffer_Array  (
                                       &transfer1->iso_irp[i].irp,
                                       &buffer_array_out);
                        NU_USB_ISO_IRP_Get_Actual_Num_Transactions(
                                       &transfer1->iso_irp[i].irp,
                                       &actual_trans);
                    }

                    for (k = 0; k < actual_trans; k++)
                    {
                        psrc = buffer_array_out[k];
                        for(j = 0; j < len_in[k]; j++)
                        {
                            p_prepared_buffer[rec_length++] = psrc[j];
                        }
                    }
                }
                transfer1->irp_count -= i;
            }



          
            if(transfer1->call_back != NU_NULL)
            {
                /* Reporting application of completion through
                 * call back.
                 */
                 transfer1->call_back(pcb_aud_device,
                              p_prepared_buffer,
                              status,
                              (rec_length/record_info->sample_size)/
                              record_info->channels);
            }                    
        }
    }
    status = USB_Deallocate_Memory(transfer1);
    NU_USB_ASSERT(status == NU_SUCCESS);
        
    status = USB_Deallocate_Memory(transfer2);
    NU_USB_ASSERT(status == NU_SUCCESS);
        
    status = NU_AUDH_Set_Events(pcb_aud_device,
                                      NU_AUDH_REC_TASK_TERMINATED);
    NU_USB_ASSERT(status == NU_SUCCESS);
        
    
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Initialize_Device
*
* DESCRIPTION
*     This function initializes few of control blocks associated with
*     audio device's control block.
*
* INPUTS
*     pcb_aud_device             Pointer to the Audio Device control block.
*
* OUTPUTS
*     NU_SUCCESS                 Indicates successful completion.
*
**************************************************************************/
STATUS NU_AUDH_Initialize_Device(NU_USBH_AUD_DEV   *pcb_aud_device)
{
    STATUS status;

    /* Create event group responsible for USB transfers. */
    status = NU_Create_Event_Group (&(pcb_aud_device->trans_events),
                                    "TrnsDone");

    if(status == NU_SUCCESS)
    {
        memset(&(pcb_aud_device->rec_sem), 0, sizeof(NU_SEMAPHORE));

        status = NU_Create_Semaphore(&(pcb_aud_device->rec_sem),
                                "AUDHREC",
                                (UNSIGNED)0,
                                (OPTION)NU_FIFO);
    }

    if(status == NU_SUCCESS)
    {
        memset(&(pcb_aud_device->play_sem), 0, sizeof(NU_SEMAPHORE));

        status = NU_Create_Semaphore(&(pcb_aud_device->play_sem),
                                "AUDHPLAY",
                                (UNSIGNED)0,
                                (OPTION)NU_FIFO);
    }


    if(status == NU_SUCCESS)
    {

        /* Create semaphore to schedule control transfers. */
        status = NU_Create_Semaphore (&(pcb_aud_device->sm_ctrl_trans),
                                      "AUDHCTRL",
                                      (UNSIGNED)1,
                                      (OPTION)NU_FIFO);

        /* In case of failure delete event group. */
        if(status != NU_SUCCESS)
        {
            status = NU_Delete_Event_Group (
                                &(pcb_aud_device->trans_events));
        }
    }



    return status;

}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Fill_ISO_Transactions
*
* DESCRIPTION
*     Helps in Filling data irp for ISO transfer. This routine actually
*     packetizes data according to the data rate.
*
* INPUTS
*     irp                 IRP structure that need to be filled up.
*     tx_data             Pointer to the audio device Host driver.
*     tx_len              Pointer to audio control request structure.
*     play_info           Pointer to Audio Play info structure.
*     trans_len_array     Array having length of each transaction.
*     trans_ptr_array     Array of pointers, pointing to the data buffer.
*
* OUTPUTS
*     None.
*
**************************************************************************/
VOID NU_AUDH_Fill_ISO_Buffer(NU_USB_ISO_IRP    *irp,
                             UINT8            **tx_data,
                             UINT32            *tx_len,
                             AUDH_PLAY_INFORM  *play_info,
                             UINT16            *trans_len_array,
                             UINT8            **trans_ptr_array)
{
    STATUS    status;
    UINT32    tot_no_bytes = 0;
    UINT32    pkt_idx, new_tot_no_bytes;
    UINT32    data_rate, compensator;
    UINT32    pkt_per_frame = 0x00;
    UINT16    num_trans,curr_data_len;
    UINT8     poll_interval;

    /* Get saved values from play inform structure. */
    data_rate     = play_info->data_rate;
    compensator   = play_info->compensator;
    pkt_per_frame = play_info->pkt_per_frame;
    poll_interval = play_info->poll_interval;

    /* Packetizes the raw data. */
    for(pkt_idx = 0; (pkt_idx < NU_AUDH_MAX_ISO_PKT_IRP); pkt_idx++)
    {
        if(*tx_len == 0)
          {
              break;
          }

          /* Calculate total bytes count that needs to be sent. */
          new_tot_no_bytes =
          ((data_rate)*(pkt_idx + 1)*(poll_interval))/pkt_per_frame;
          curr_data_len    = (new_tot_no_bytes - tot_no_bytes)*compensator;

          /* In case data length is less than current data length. */
          if(curr_data_len > *tx_len)
          {
              trans_ptr_array[pkt_idx]   = (UINT8*)(*tx_data);
              *tx_data                  -= *tx_len;
              trans_len_array[pkt_idx]   = *tx_len ;
              *tx_len                    = 0;
              break;
          }

          /* Fill buffer array with data pointers. */
          trans_ptr_array[pkt_idx]  = (UINT8*)(*tx_data);

          /* Fill length array with calculated length. */
          trans_len_array[pkt_idx]  = curr_data_len;
          tot_no_bytes              = new_tot_no_bytes;

          /* Increment current data pointer. */
          *tx_data                 += curr_data_len;

          /* Decrement current data length. */
          *tx_len                  -= curr_data_len;

    }
    /* Number of transactions needed. */
    num_trans = pkt_idx;

    /*Fill ISO IRP structure with the help of calculated values. */
    status = NU_USB_ISO_IRP_Set_Num_Transactions(irp ,num_trans);
    if (status == NU_SUCCESS)
    {
        status = NU_USB_ISO_IRP_Set_Lengths(irp, trans_len_array);
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USB_ISO_IRP_Set_Buffer_Array(irp, trans_ptr_array);
    }
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Paketize
*
* DESCRIPTION
*     Helps in Filling data irp for ISO transfer. This routine actually
*     packetizes data according to the data rate.
*
* INPUTS
*     irp                 IRP structure that need to be filled up.
*     tx_data             Pointer to the audio device Host driver.
*     tx_len              Pointer to audio control request structure.
*     play_info           Pointer to Audio Play info structure.
*     trans_len_array     Array having length of each transaction.
*     trans_ptr_array     Array of pointers, pointing to the data buffer.
*
* OUTPUTS
*     None.
*
**************************************************************************/
VOID NU_AUDH_Packetize (NU_USB_ISO_IRP    *irp,
                        UINT8            **tx_data,
                        UINT32            *tx_len,
                        UINT16             maxp,
                        UINT16            *trans_len_array,
                        UINT8            **trans_ptr_array)
{
    STATUS    status;
    UINT32    pkt_idx;

    /* Packetizes the raw data. */
    for(pkt_idx = 0; (pkt_idx < NU_AUDH_MAX_ISO_PKT_IRP); pkt_idx++)
    {
        if(*tx_len == 0)
        {
            break;
        }

        if(maxp > *tx_len)
        {
            trans_ptr_array[pkt_idx]   = (UINT8*)(*tx_data);
            *tx_data                  -= *tx_len;
            trans_len_array[pkt_idx]   = *tx_len;
            *tx_len                    = 0;
            break;
        }

          /* Fill buffer array with data pointers. */
          trans_ptr_array[pkt_idx]  = (UINT8*)(*tx_data);

          /* Fill length array with calculated length. */
          trans_len_array[pkt_idx]  = maxp;

          /* Increment current data pointer. */
          *tx_data                 += maxp;

          /* Decrement current data length. */
          *tx_len                  -= maxp;
    }

    /*Fill ISO IRP structure with the help of calculated values. */
    status = NU_USB_ISO_IRP_Set_Num_Transactions( irp ,
                                                  (UINT16)pkt_idx);
    if (status == NU_SUCCESS)
    {
        status = NU_USB_ISO_IRP_Set_Lengths(irp, trans_len_array);
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USB_ISO_IRP_Set_Buffer_Array(irp, trans_ptr_array);
    }
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Set_Events
*
* DESCRIPTION
*     Set passed event in device's task event group.
*
* INPUTS
*     pcb_audio_device                      Pointer to audio device's
*                                           control block.
*     mask                                  Event mask to be set.
*
* OUTPUTS
*     NU_SUCCESS                            Indicates that events
*                                           corresponding to the mask have
*                                           successfully been set.
*
**************************************************************************/
STATUS NU_AUDH_Set_Events(NU_USBH_AUD_DEV * pcb_audio_device,
                          UINT32            mask)
{
    STATUS status;

    status = NU_Set_Events(&(pcb_audio_device->task_events),
                            mask,
                            NU_OR);

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_AUDH_Get_Events
*
* DESCRIPTION
*     Set passed event in device's task event group.
*
* INPUTS
*     pcb_audio_device                      Pointer to audio device's
*     mask                                  Event mask to be set.
*     suspend                               Suspension option. It can be
*                                           one of the following values:-
*                                           NU_SUSPEND, NU_NO_SUSPEND or
*                                           time-out value.
*
* OUTPUTS
*     NU_SUCCESS                            Indicates that events
*                                           corresponding to the mask are
*                                           currently set.
*
**************************************************************************/
STATUS NU_AUDH_Get_Events(NU_USBH_AUD_DEV * pcb_audio_device,
                          UINT32            mask,
                          UINT32            suspend)
{
    STATUS status;
    UINT32 ret_events;

    status = NU_Retrieve_Events(&(pcb_audio_device->task_events),
                                mask,
                                NU_AND_CONSUME,
                                &ret_events,
                                suspend);

    return status;
}


/************************* end of file ***********************************/
