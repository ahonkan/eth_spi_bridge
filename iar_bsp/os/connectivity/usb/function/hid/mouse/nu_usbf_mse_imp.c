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
*       nu_usbf_mse_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software: HID Mouse User Driver.
*
* DESCRIPTION
*
*       This file contains the internal implementations for the HID Mouse
*       User Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_USBF_MSE_Process_Request          Function to handle class
*                                            specific requests.
*       NU_USBF_MSE_Timer_expiration_routine Timer expiration routine.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files ===================================*/
#include    "connectivity/nu_usb.h"

/* ==========================  Functions ==============================**/


/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_Process_Current_Request
*
* DESCRIPTION
*
*       This function handles the class specific requests for the class
*       driver.
*
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
STATUS NU_USBF_MSE_Process_Current_Request(NU_USBF_MSE *hid_user,
                                           NU_USBF_HID_CMD *cmd,
                                           UINT8 **data_out,
                                           UINT32 *data_len_out)
{
    STATUS status = NU_SUCCESS;
    UINT32 Command;
    UINT16 wValue = cmd->cmd_Value;
    UINT8 TimerTicks = wValue && 0x00FF;    /* Get Timer Ticks */

    if(hid_user == NU_NULL)
        status = NU_USB_INVLD_ARG;
    else
    {
        /* Store command in the local variable */
        Command = cmd->command;

        /* Pass the descriptor data and its size as output*/
        if(Command == HIDF_GET_DESCRIPTOR)
        {
            *data_out = (UINT8*)hid_user->mse_report_descriptor;
            *data_len_out = MUS_REPORT_DESCRIPTOR_SIZE;

        }
        else if(Command == HIDF_SET_IDLE)
        {
            /* Set the idle rate of the timer. Timer expires at this
            idle rate */
            if(TimerTicks != 0)
            {
                status = NU_Control_Timer(&hid_user->Idle_Timer,
                                          NU_ENABLE_TIMER);

                if(status == NU_SUCCESS)
                {
                    status = NU_Reset_Timer(&hid_user->Idle_Timer,
                                    NU_USBF_MSE_Timer_expiration_routine,
                                    TimerTicks,
                                    TimerTicks,
                                    NU_ENABLE_TIMER);
                }
            }
        }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_Timer_expiration_routine()
*
*
* DESCRIPTION
*
*       This function is invoked each time the timer expires.
*
*
* INPUTS
*
*       ID         Timer ID.
*
*
*************************************************************************/
VOID NU_USBF_MSE_Timer_expiration_routine(UNSIGNED ID)
{

    STATUS status;
    NU_USBF_MSE_REPORT *mousereport = NU_NULL;
    UINT32 event;
    NU_USBF_MSE  *cb = (NU_USBF_MSE  *)ID;

    /* Allocate memory for mouse report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 MSE_REPORT_LEN,
                                 (VOID**)&(mousereport));
    if ( status == NU_SUCCESS )
    {
        /* First initialize the value of all the fields of the data structure
         to be 0*/
        memset(mousereport, 0, sizeof(NU_USBF_MSE_REPORT));

        /* Send report data to the class driver*/
        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                        (UINT8*)mousereport,
                                        sizeof(NU_USBF_MSE_REPORT),
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

    if ( mousereport )
    {
        USB_Deallocate_Memory(mousereport);
        mousereport = NU_NULL;
    }
}

/* ======================  End Of File  =============================== */
