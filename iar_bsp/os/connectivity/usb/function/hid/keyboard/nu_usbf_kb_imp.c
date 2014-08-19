/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_kb_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software : HID User Driver.
*
* DESCRIPTION
*
*       This file contains the internal implementations for the HID KB
*       User Driver.
*
* DATA STRUCTURES
*
*       None.
*
*
* FUNCTIONS
*
*       NU_USBF_KB_Process_Current_Request  Function to
*                                           handle class specific requests.
*
*       NU_USBF_KB_Timer_expiration_routine Timer expiration routine.
*
*       NU_USBF_KB_Fillkeydata              This function fills the usage
*                                           data for the corresponding key
*                                           pressed.
*
*       NU_USBF_KB_Fill_Modifier_Byte       This function fills the usage
*                                           data for the corresponding
*                                           modifier key pressed.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files ===================================*/
#include    "connectivity/nu_usb.h"

/* ==========================  Functions ==============================*/

/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Process_Current_Request
*
* DESCRIPTION
*       This function handles the class specific requests for the class
*       driver.
*
* INPUTS
*
*       hid_user        Pointer to user driver's control block.
*       cmd             Pointer to received control pipe command.
*       data_out        Pointer to location holding for transfer buffer.
*       data_len_out    Pointer to location holding for transfer length.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the command has been
*                               processed successfully
*       NU_USB_INVLD_ARG        Indicates a malformed command block
*
*************************************************************************/
STATUS NU_USBF_KB_Process_Current_Request(NU_USBF_KB *hid_user,
                                          NU_USBF_HID_CMD *cmd,
                                          UINT8 **data_out,
                                          UINT32 *data_len_out)
{

    STATUS status = NU_SUCCESS;
    UINT32 Command;
    UINT16 wValue = cmd->cmd_Value;
    UINT8 TimerTicks = wValue && 0x00FF;    /* Obtain Idle rate */

    /* Check input argument */
    if((hid_user == NULL))
        status = NU_USB_INVLD_ARG;
    else
    {
        Command = cmd->command;

        /* Pass out the report descriptor pointer and size*/
        if(Command == HIDF_GET_DESCRIPTOR)
        {
            *data_out = (UINT8*)hid_user->kb_report_descriptor;
            *data_len_out = USBF_KB_REPORT_DESCRIPTOR_LEN;
        }
        else if(Command == HIDF_SET_IDLE)
        {

            if(TimerTicks != 0)
            {
                /* Reset the timer according to the idle rate received
                 in SET_IDLE request */
                status = NU_Reset_Timer(&hid_user->Idle_Timer,
                                    NU_USBF_KB_Timer_expiration_routine,
                                    TimerTicks,
                                    TimerTicks,
                                    NU_ENABLE_TIMER);

            }

        }
        else if(Command == HIDF_SET_REPORT)
        {
            if(hid_user->rx_callback != NU_NULL)
            {
                /* Pass the data received from the host in the callback
                function defined by the application */
                hid_user->rx_callback(cmd->data_len,cmd->cmd_data);
            }

        }


    }

    return status;
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Timer_expiration_routine
*
*
* DESCRIPTION
*       This function is invoked each time the timer expires.
*
*
* INPUTS
*
*       ID         Timer ID.
*
*
*************************************************************************/
void NU_USBF_KB_Timer_expiration_routine(UNSIGNED ID)
{

    STATUS status;
    NU_USBF_KB_IN_REPORT *keyboardreport = NU_NULL;
    UINT32 event;
    NU_USBF_KB  *cb = (NU_USBF_KB  *)ID;
    
    /* Allocate memory to keyboard report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                sizeof(NU_USBF_KB_IN_REPORT),
                                (void**)&keyboardreport);
    if ( status == NU_SUCCESS )
    {
        /* First initialize the value of all the fields of the data structure
         to be 0*/
        memset(keyboardreport,0,sizeof(NU_USBF_KB_IN_REPORT));
    
        /* Send report data to the class driver*/
        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                        (UINT8*)keyboardreport,
                                        sizeof(NU_USBF_KB_IN_REPORT),
                                        cb->handle);
    
        if(status == NU_SUCCESS)
        {
            /* Wait for the completion of data transfer. */
            status = NU_Retrieve_Events(&cb->device_connect_event,
                                        HIDF_DATA_SENT,
                                        NU_AND_CONSUME,
                                        &event,
                                        NU_SUSPEND);
    
        }
    }
    
    if ( keyboardreport )
    {
        USB_Deallocate_Memory(keyboardreport);
        keyboardreport = NU_NULL;
    }
}
/*************************************************************************
* FUNCTION
*
*   NU_USBF_KB_Fill_Key_Data
*
* DESCRIPTION
*   This function fills the usage data for the corresponding key pressed.
*
*
* INPUTS
*
*   Keypressed      Pointer to the character mentioning the key pressed.
*   KeyCode         Pointer to the Key data to be filled up by the
*                   function.
*
*
*************************************************************************/
void NU_USBF_KB_Fill_Key_Data(CHAR *KeyPressed,UINT8 *KeyCode)
{
    INT i;


    for(i=0;i<USBF_KB_ASCII_TABLE_SIZE;i++)
    {
        if(nu_usbf_kb_keysasciitable[i] == *KeyPressed)
        {
           *KeyCode = nu_usbf_kb_usageidtable[i];
            break;
        }
    }


}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Fill_Modifier_Byte
*
* DESCRIPTION
*       This function fills the usage data for the corresponding modifier
*       key pressed.
*
*
* INPUTS
*
*       modifierbyte            Pointer to the modifier byte of report
*                               data.
*       modifierbuffer          Pointer to the modifier Key data.
*       buffersize              Size of the buffer holding modifier keys.
*
*
*************************************************************************/
void NU_USBF_KB_Fill_Modifier_Byte(UINT8 *modifierByte,
                                   UINT8 *modifierkeybuffer,
                                   UINT8 buffersize)
{
    INT i;

    for(i=0;i<buffersize;i++)
        *modifierByte = (*modifierByte) | (modifierkeybuffer[i]);
}

/* ======================  End Of File  =============================== */
