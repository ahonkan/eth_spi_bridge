/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
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
*       mdm.c
*
*   COMPONENT
*
*       MDM - Modem Control
*
*   DESCRIPTION
*
*       This file contains the modem functions.
*
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MDM_Init
*       MDM_Receive
*       NU_Terminal_Data_Ready
*       MDM_Data_Ready
*       NU_Get_Terminal_Char
*       MDM_Get_Char
*       NU_Modem_Control_String
*       MDM_Control_String
*       MDM_Delay
*       MDM_Dial
*       MDM_Get_Modem_String
*       MDM_Hangup
*       NU_Purge_Terminal_Buffer
*       MDM_Purge_Input_Buffer
*       NU_Change_Communication_Mode
*       MDM_Change_Communication_Mode
*       MDM_Wait_For_Client
*       NU_Put_Terminal_Char
*       MDM_Put_Char
*       NU_Carrier
*       MDM_Carrier
*       NU_Reset_Modem
*       MDM_Modem_Connected
*       MDM_Rings_To_Answer
*       NU_Modem_Set_Local_Num
*       MDM_Set_Local_Num
*       NU_Modem_Get_Remote_Num
*       MDM_Get_Remote_Num
*
*   DEPENDENCIES
*
*       ppp.h
*
*************************************************************************/
#include "drivers/ppp.h"

#if MDM_DEBUG_PRINT_OK
#define PrintInfo(s)       PPP_Printf(s)
#define PrintErr(s)        PPP_Printf(s)
#else
#define PrintInfo(s)
#define PrintErr(s)
#endif

/*************************************************************************
* FUNCTION
*
*       MDM_Init
*
* DESCRIPTION
*
*       This function initializes the MODEM module.
*
* INPUTS
*
*       *dev_ptr
*
* OUTPUTS
*
*       STATUS                           Returns NU_SUCCESS if successful
*                                        initialization, else a negative
*                                        value is returned.
*
*************************************************************************/
STATUS MDM_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    MDM_LAYER   *mdm;
    LINK_LAYER  *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    STATUS      status = NU_SUCCESS;

    /* Allocate memory for the MDM layer structure. This structure will
       be pointed to by the ppp_layer structure for this device. */
    if (NU_Allocate_Memory (PPP_Memory, (VOID **)&mdm, sizeof (MDM_LAYER),
                            NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NERRS_Log_Error (TCP_FATAL, __FILE__, __LINE__);

        NLOG_Error_Log("Failed to allocate memory for MDM structure.",
            NERR_FATAL, __FILE__, __LINE__);

        return(-1);
    }

    /* Zero out the HDLC layer information. */
    UTL_Zero (mdm, sizeof (MDM_LAYER));

    /* Store the address of the HDLC layer structure. */
    link_layer->link = mdm;

    /* Allocate a block of memory for the receive buffer. Note that this 
       buffer is only used when the driver is being used in terminal mode. */
    if (NU_Allocate_Memory (PPP_Memory,
                            (VOID **)&mdm->recv_buffer.mdm_head,
                            MDM_RECV_BUFFER_SIZE, NU_NO_SUSPEND)
                            != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for MDM receive buffer.",
            NERR_FATAL, __FILE__, __LINE__);

        return(-1);
    }

    mdm->recv_buffer.mdm_read = 
        mdm->recv_buffer.mdm_write = mdm->recv_buffer.mdm_head;

    mdm->recv_buffer.mdm_tail = 
        mdm->recv_buffer.mdm_head + MDM_RECV_BUFFER_SIZE - 1;

    mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_EMPTY;

    strcpy(mdm->dial_prefix, MDM_DIAL_PREFIX);

    /* Set the default number of rings to answer the modem on. */
    mdm->num_rings = MDM_RINGS_TO_ANSWER_ON;

    link_layer->hwi.state = INITIAL;

    return (status);

} /* MDM_Init */



/*************************************************************************
* FUNCTION
*
*       MDM_Receive
*
* DESCRIPTION
*
*       This function receives characters whenever operating in
*       terminal mode.
*
* INPUTS
*
*       *dev_ptr
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID MDM_Receive(DV_DEVICE_ENTRY *dev_ptr, INT c)
{
    LINK_LAYER  *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    MDM_LAYER   *mdm = (MDM_LAYER*)link_layer->link;

#ifdef PPP_USB_ENABLE
    NU_USBH_COM_XBLOCK* xblock = (NU_USBH_COM_XBLOCK*)dev_ptr->user_defined_2;
    UINT8*  ch_ptr = (UINT8*)xblock->p_data_buf;
#endif

#ifdef PPP_USB_ENABLE
    while(xblock->transfer_length)
    {
#endif        

	    if (mdm->recv_buffer.mdm_buffer_status != MDM_BUFFER_FULL)
	    {
	        /* Get the character. */
#ifdef PPP_USB_ENABLE
	        *mdm->recv_buffer.mdm_write = *ch_ptr++;
	        xblock->transfer_length--;
#else
           *mdm->recv_buffer.mdm_write = c;
#endif
	        /* Point to the location where the next character will be written. */
	        mdm->recv_buffer.mdm_write++;

	        /* Does the write pointer need to be wrapped around to the start of the
	           ring buffer. */
	        if (mdm->recv_buffer.mdm_write > mdm->recv_buffer.mdm_tail)
	            mdm->recv_buffer.mdm_write = mdm->recv_buffer.mdm_head;

	        /* Set the status field.  We just added some data so the buffer is
	           either full or contains at least one byte of data. */
	        if(mdm->recv_buffer.mdm_write == mdm->recv_buffer.mdm_read)
	            mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_FULL;
	        else
	            mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_DATA;
	    }
	    else
	    {
	        /* There is no space for the character, so drop it. */
#ifdef PPP_USB_ENABLE
	        ch_ptr++;
	        xblock->transfer_length--;
#endif
	    }

#ifdef PPP_USB_ENABLE
    } /* End of while */
#endif

} /* MDM_Receive */


/*************************************************************************
* FUNCTION
*
*       NU_Terminal_Data_Ready
*
* DESCRIPTION
*
*       This function calls a function to check to see if there are any
*       characters in the receive buffer.  A status value is returned
*       indicating the whether characters are present in the receive
*       buffer.
*
* INPUTS
*
*       *link_name              Name of the link
*
* OUTPUTS
*
*       STATUS                  The status indicates the presence of
*                               characters.
*
*************************************************************************/
STATUS NU_Terminal_Data_Ready(CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this name. */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure that a device was found. */
    if (dev_ptr)
        status = MDM_Data_Ready(dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Terminal_Data_Ready */



/*************************************************************************
* FUNCTION
*
*       MDM_Data_Ready
*
* DESCRIPTION
*
*       This function checks to see if there are any characters in the
*       receive buffer.  A status value is returned indicating the whether
*       characters are present in the receive buffer.
*
* INPUTS
*
*       *dev_ptr                Pointer to the device.
*
* OUTPUTS
*
*       STATUS                  The status indicates the presence of
*                               characters.
*
*************************************************************************/
STATUS MDM_Data_Ready(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer;
    MDM_LAYER       *mdm;
    STATUS          status;

    /* Get the modem structure for this device. */
    link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    mdm = (MDM_LAYER*)link_layer->link;

    if((mdm->recv_buffer.mdm_buffer_status == MDM_BUFFER_DATA) ||
       (mdm->recv_buffer.mdm_buffer_status == MDM_BUFFER_FULL))
    {
        status = NU_TRUE;
    }
    else
        status = NU_FALSE;

    return status;

} /* MDM_Data_Ready */

/*************************************************************************
* FUNCTION
*
*       NU_Get_Terminal_Char
*
* DESCRIPTION
*
*       This function calls the function which returns the next character
*       that is in the receive buffer.
*
* INPUTS
*
*       *c            Pointer to a location to store the RX character.
*       *link_name    Name of the link.
*
* OUTPUTS
*
*       STATUS        NU_SUCCESS if a character is available. NU_NO_DATA
*                     if no characters are available.
*                     NU_INVALID_POINTER if c is NULL
*
*************************************************************************/
STATUS NU_Get_Terminal_Char(CHAR *c, CHAR *link_name)
{
    STATUS          status = NU_INVALID_LINK;
    DV_DEVICE_ENTRY *dev_ptr;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (c == NU_NULL)
    {
        /* Return from function in user mode. */
        NU_USER_MODE();
        return NU_INVALID_POINTER;
    }

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this name. */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure that a device was found. */
    if (dev_ptr)
        status = MDM_Get_Char(c, dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Get_Terminal_Char */


/*************************************************************************
* FUNCTION
*
*       MDM_Get_Char
*
* DESCRIPTION
*
*       This function returns the next character that is in the receive
*       buffer.
*
* INPUTS
*
*       *c            Pointer to a location to store the RX character.
*       *dev_ptr      Pointer to the device.
*
* OUTPUTS
*
*       STATUS                              NU_SUCCESS if a character is
*                                           available. NU_NO_DATA if no
*                                           characters are available.
*                                           NU_INVALID_POINTER if c is NULL
*
*************************************************************************/
STATUS MDM_Get_Char(CHAR *c, DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS          status;
    LINK_LAYER      *link_layer;
    MDM_LAYER       *mdm;

    /* Get the modem structure for this device. */
    link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    mdm = (MDM_LAYER*)link_layer->link;

    /* Is there any data available. */
    if ((mdm->recv_buffer.mdm_buffer_status == MDM_BUFFER_DATA) ||
       (mdm->recv_buffer.mdm_buffer_status == MDM_BUFFER_FULL))
    {
        /* Store the character to be returned. */
        *c = *mdm->recv_buffer.mdm_read;

        /* Point to the next character to be removed from the buffer. */
        mdm->recv_buffer.mdm_read++;

        /* Does the read pointer need to be wrapped around to the start of the
           buffer. */
        if (mdm->recv_buffer.mdm_read > mdm->recv_buffer.mdm_tail)
            mdm->recv_buffer.mdm_read = mdm->recv_buffer.mdm_head;

        /* Set the status field.  We just removed some data so the buffer is
           either empty or is at least one byte of short of FULL. */
        if(mdm->recv_buffer.mdm_write == mdm->recv_buffer.mdm_read)
            mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_EMPTY;
        else
            mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_DATA;

        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NO_DATA;
    }

    return status;

} /* MDM_Get_Char */


/*************************************************************************
* FUNCTION
*
*       NU_Modem_Control_String
*
* DESCRIPTION
*
*       This function calls a function to send control codes to the modem.
*
* INPUTS
*
*       *string            String containing the control code to be sent.
*       *link_name         Name of the link.
*
* OUTPUTS
*
*       STATUS          NU_SUCCESS          If successful
*                       NU_INVALID_POINTER  If string
*                                           pointer is null.
*
*************************************************************************/
STATUS NU_Modem_Control_String(CHAR *string, CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (string == NU_NULL)
    {
        /* Return from function in user mode. */
        NU_USER_MODE();
        return NU_INVALID_POINTER;
    }

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this name. */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure that the device was found. */
    if (dev_ptr)
    {
        status = MDM_Control_String(string, dev_ptr);
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Modem_Control_String */


/*************************************************************************
* FUNCTION
*
*       MDM_Control_String
*
* DESCRIPTION
*
*       This function is used to send control codes to the modem.
*
* INPUTS
*
*       *string                             String containing the control
*                                           code to be sent.
*       *dev_ptr                            Pointer to device.
*
* OUTPUTS
*
*       STATUS          NU_SUCCESS          If successful
*                       NU_INVALID_LINK     If device removed during wait.
*                       NU_INVALID_POINTER  If string
*                                           pointer is null.
*
*************************************************************************/
STATUS MDM_Control_String(CHAR *string, DV_DEVICE_ENTRY *dev_ptr)
{
#ifdef PPP_USB_ENABLE
    UINT8 temp = 0;
    NU_USBH_COM_MDM_DEVICE* modem = (NU_USBH_COM_MDM_DEVICE*)dev_ptr->user_defined_1;
#else
    SERIAL_SESSION *uart = ((LINK_LAYER *)dev_ptr->dev_link_layer)->uart;
#endif

    UINT32          index_save;

    while (*string) {                   /* loop for the entire string */
        switch (*string) {
            case '~':                   /* if ~, wait a half second */
                /* Save the index before releasing the semaphore. */
                index_save = dev_ptr->dev_index;

                /* Release the semaphore. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release a semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                MDM_Delay(500);

                /* Grab the semaphore again. */
                if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain a semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Check if the device has not been removed. */
                if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
                {
                    /* Device has been removed, so return from this
                       function. */
                    return (NU_INVALID_LINK);
                }

                break;
            default:
                switch (*string) {
                    case '^':           /* if ^, it's a control code */
                        if (string[1]) {     /* send the control code */
                            string++;
#ifdef PPP_USB_ENABLE
                            temp = (*string - 64);                       
                            modem->tx_buffer[modem->tx_buffer_write++] = 
                            temp;
#else
                            NU_Serial_Putchar(uart, (CHAR)(*string - 64));
#endif
                        }
                        break;
                    default:
#ifdef PPP_USB_ENABLE
                        temp = (*string);                       
                        modem->tx_buffer[modem->tx_buffer_write++] = 
                        temp;
#else
                        NU_Serial_Putchar(uart, *string); /* send the character */
#endif
                }

                /* Save the index before releasing the semaphore. */
                index_save = dev_ptr->dev_index;

                /* Release the semaphore. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release a semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                MDM_Delay(100);              /* wait 100 ms. for slow modems */

                /* Grab the semaphore again. */
                if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain a semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Check if the device has not been removed. */
                if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
                {
                    /* Device has been removed, so return from this
                       function. */
                    return (NU_INVALID_LINK);
                }
        }
        string++;                            /* bump the string pointer */

    } /* end while there are char to send */

#ifdef PPP_USB_ENABLE
        NU_USBH_COM_MDM_Send_Buffer(modem);
        NU_USBH_COM_USER_Start_Polling(&modem->user_device) ;
#endif

    return NU_SUCCESS;

} /* MDM_Control_String */



/*************************************************************************
* FUNCTION
*
*       MDM_Delay
*
* DESCRIPTION
*
*       This function is used to simply consume CPU time.
*
* INPUTS
*
*       UNSIGNED              milliseconds    Time in milliseconds to delay.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID MDM_Delay(UNSIGNED milliseconds)
{
    UNSIGNED ticks;

    /* Compute the approximate number of ticks.  This value does not have to be
       exactly on target. */
    ticks = (TICKS_PER_SECOND * milliseconds)/1000;

    /* Sleep for at least one tick. */
    if (!ticks)
        ticks = 1;

    NU_Sleep(ticks);

} /* MDM_Delay */



/*************************************************************************
* FUNCTION
*
*       MDM_Dial
*
* DESCRIPTION
*
*       This function commands the modem to dial a phone number.
*
* INPUTS
*
*       *number                             Pointer to a string that
*                                           contains the number to dial.
*       *dev_ptr                            Pointer to device.
*
* OUTPUTS
*
*       STATUS                              NU_SUCCESS is returned if
*                                           connection to remote machine
*                                           is made, else a negative value
*                                           is returned.
*
*************************************************************************/
STATUS MDM_Dial(CHAR *number, DV_DEVICE_ENTRY *dev_ptr)
{
    CHAR        dial_string[MDM_RESPONSE_SIZE],
                response[MDM_RESPONSE_SIZE],
                *baud_start;
    STATUS      status;
    LINK_LAYER  *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    MDM_LAYER   *mdm = (MDM_LAYER*)link_layer->link;
    SERIAL_SESSION *uart = link_layer->uart;
    UINT32      index_save;

    while (link_layer->hwi.state != INITIAL)
        NU_Sleep(10);

    if (number == NU_NULL)
        return(NU_INVALID_POINTER);

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
           __FILE__, __LINE__);
    }

    /* Reset the UART */
    SDC_Reset(uart);

    link_layer->hwi.state = STARTING;

    /* Clear the hangup attempts since we are starting a new session. */
    mdm->hangup_attempts = 0;

    /* Set the status of the PPP connection attempt. */
    link_layer->connection_status = NU_PPP_DIALING;

    /* Before trying to dial check for the presence of the modem. */
    if ( (MDM_Modem_Connected (dev_ptr) == NU_TRUE) )
    {
        /* Change to terminal  mode */
        MDM_Change_Communication_Mode(MDM_TERMINAL_COMMUNICATION, dev_ptr);

        /* Purge any characters that were already in the receive buffer. */
        MDM_Purge_Input_Buffer(dev_ptr);

        strcpy(dial_string, mdm->dial_prefix);
        strcpy(&dial_string[strlen(dial_string)], number);
        strcpy(&dial_string[strlen (dial_string)], "^M");

#if(PPP_ENABLE_CLI == NU_TRUE)
        /* store the number to which we are dialing to */
        strcpy(mdm->remote_num, number);
#endif

        /* dial the number */
        MDM_Control_String(dial_string, dev_ptr);

        /* The modem will return the string with a CR on the end, 0xD is
           the ASCII code for a CR. */
        strcpy(dial_string, mdm->dial_prefix);
        strcpy(&dial_string[strlen(dial_string)], number);
        dial_string [strlen(dial_string) + 1] = 0;
        dial_string [strlen(dial_string)] = 0xD;

        MDM_Get_Modem_String(response, dial_string, dev_ptr,
                             MDM_RESPONSE_SIZE);   /* get a response */

        /* get a pointer to the connect string */
        baud_start = (CHAR *) strstr(response, "CONNECT");

        /* connection was made */
        if (baud_start)
        {
            /* read the baud rate for printing purposes */
            mdm->baud_rate = NU_ATOL(baud_start + 8);

#ifdef NU_DEBUG_PPP
            PrintInfo(baud_start);
            PrintInfo("\n");
#endif

            /* Set the status to success since the modem has connected. */
            status = NU_SUCCESS;

            /* Save the index before releasing the semaphore. */
            index_save = dev_ptr->dev_index;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
            }

            /* We must wait a little to let the server begin its session. */
            NU_Sleep(TICKS_PER_SECOND * 2);

            /* Grab the semaphore again. */
            if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
            }

            /* Check if the device has not been removed. */
            if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
            {
                /* Device has been removed, so return from this
                function. */
                status = NU_INVALID_LINK;
            }
        }
        else
            if (strstr(response, "NO CARRIER") != NULL)
                status = NU_NO_CARRIER;
            else
                if (strstr(response, "BUSY") != NULL)
                    status = NU_BUSY;
                else
                    status = NU_NO_CONNECT;
    }
    else
        status = NU_NO_MODEM;

    if (status == NU_SUCCESS)
    {
        /* Switch back to network packet mode. */
        MDM_Change_Communication_Mode(MDM_NETWORK_COMMUNICATION, dev_ptr);

        link_layer->hwi.state = OPENED;
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
    }

    return (status);

} /* MDM_Dial */



/*************************************************************************
* FUNCTION
*
*       MDM_Get_Modem_String
*
* DESCRIPTION
*
*       This function retrieves the responses from the modem after the modem
*       has been issued a command.
*
* INPUTS
*
*       *response                           Pointer to a location to store
*                                           the modems response.
*       *dial_string                        Pointer to the string that was
*                                           sent to the modem.
*       *dev_ptr                            Pointer to device.
*       rsp_size                            Size of response.
*
* OUTPUTS
*
*       CHAR*                               Pointer to a location to store
*                                           the modems response.
*
*************************************************************************/
CHAR *MDM_Get_Modem_String(CHAR *response, CHAR *dial_string,
                           DV_DEVICE_ENTRY *dev_ptr, UINT8 rsp_size)
{
    CHAR            c;
    INT32           time;
    INT8            got_dial_string;
    UINT32          index_save;
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;

    *response = 0;
    got_dial_string = NU_FALSE;

    time = NU_Retrieve_Clock();

    time += 50 * TICKS_PER_SECOND;

    /* Timeout if a response is not received within the specified period of
       time.  Note the test below accounts for the wrapping of the clock. */
    while ((time - (INT32)NU_Retrieve_Clock()) > 0)
    {
        /* get a character from the port if one's there */
        if (MDM_Data_Ready(dev_ptr))
        {
            /* Retrieve the next character. */
            MDM_Get_Char(&c, dev_ptr);

            switch (c)
            {
                case 0xD:                /* CR - return the result string */

                    /* The first CR is from the dial string. If we got it
                       already then return the received string.  */
                    if ((got_dial_string) && (*response))
                        return (response);
                    else

                        /* Otherwise set the flag stating that we got
                           the CR from dialing. */
                        got_dial_string = NU_TRUE;

                default:
                    if (c != 10)
                    {   /* add char to end of string */
                        response[strlen(response) + 1] = 0;
                        response[strlen(response)] = (CHAR)c;

                        /* ignore RINGING and the dial string */
                        if ( !strcmp(response, "RINGING") ||
                            !NCL_Stricmp(response, dial_string) )
                        {
                            *response = 0;
                        }
                        else
                            /* check for a full response buffer and return
                               if it is full */
                            if ((strlen (response)) >= ((UINT16)(rsp_size - 1)))
                                return (response);
                    }
            }
        }
        else
        {
            /* Save the index before releasing the semaphore. */
            index_save = dev_ptr->dev_index;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
            }

            NU_Sleep(10);

            /* Grab the semaphore again. */
            if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
            }

            /* Check if the device has not been removed and
               also check that dialing has not been aborted. */
            if((DEV_Get_Dev_By_Index(index_save) == NU_NULL) ||
               (link_layer->connection_status == NU_PPP_WAITING_ABORTED))
            {
                /* Hangup the modem. */
                link_layer->hwi.disconnect(link_layer, NU_TRUE);

                /* Device has been removed, so set response string
                   to zero length and return from this function. */
                *response = 0;
                break;
            }
        }
    }

    return (response);

} /* MDM_Get_Modem_String */



/*************************************************************************
* FUNCTION
*
*       MDM_Hangup
*
* DESCRIPTION
*
*       This function does exactly what you would think, hangs up the modem.
*
* INPUTS
*
*       LINK_LAYER      *link_ptr       Pointer to the PPP link to hangup
*       UINT8           wait_for_modem  Should the routine wait for the
*                                       modem to respond
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS MDM_Hangup(LINK_LAYER *link_ptr, UINT8 wait_for_modem)
{
    CHAR      mdm_response[3];
    INT       mdm_index = 0;
    INT       mdm_hangup = NU_FALSE;
    MDM_LAYER *mdm = (MDM_LAYER*)link_ptr->link;
    UINT32    index_save;

    /* No need to do anything if the modem is already disconnected. */
    if (link_ptr->hwi.state == INITIAL)
        return NU_SUCCESS;

    /* First things first. Mark the device as not up. This will stop
       IP packets from trying to be transmitted. */
    link_ptr->hwi.dev_ptr->dev_flags &= ~DV_UP;
    link_ptr->hwi.state = STOPPING;

    /* Set the status of the PPP connection attempt. */
    link_ptr->connection_status = NU_PPP_HANGING_UP;

    /* Is this the first time we have tried to hangup this device and
       we do not want to wait on the modem. */
    if ((mdm->hangup_attempts == 0) && !wait_for_modem)
    {
        /* Switch to terminal mode so that we can communicate with
           the modem. */
        MDM_Change_Communication_Mode(MDM_TERMINAL_COMMUNICATION, link_ptr->hwi.dev_ptr);

        /* Send the AT hangup command to the modem. */
        MDM_Control_String(MDM_HANGUP_STRING,
                                    link_ptr->hwi.dev_ptr);

        /* Bump the number of hangup attempts. */
        mdm->hangup_attempts++;

        /* Set an event to check the status of the modem. We do not
           want to wait in this routine for the modem to hangup. */
        TQ_Timerset(PPP_Event, (UNSIGNED) link_ptr->hwi.dev_ptr,
                    TICKS_PER_SECOND, MDM_HANGUP);
    }
    else
    {
        do
        {

            /* Bump the number of hangup attempts. */
            mdm->hangup_attempts++;

            /* If we are waiting for the modem then send the hangup command
               here. */
            if (wait_for_modem)
            {
                /* Switch to terminal mode so that we can communicate with
                   the modem. */
                MDM_Change_Communication_Mode (MDM_TERMINAL_COMMUNICATION,
                                link_ptr->hwi.dev_ptr);

                /* Send the AT hangup command to the modem. */
                MDM_Control_String(MDM_HANGUP_STRING,
                                    link_ptr->hwi.dev_ptr);

                /* Store the index of the device before releasing the
                semaphore. */
                index_save = link_ptr->hwi.dev_ptr->dev_index;

                /* Release the semaphore. */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
                }

                /* Wait for a second for the modem to hangup */
                NU_Sleep (TICKS_PER_SECOND);

                /* Grab the semaphore again. */
                if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
                }

                /* Check if the device has not been removed. */
                if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
                {
                    /* Device has been removed, so return from this
                    function. */
                    return NU_INVALID_LINK;
                }
            }

            /* We have already sent the hangup command. So check the modem
               buffer for an OK response from the modem. */
            while ((!mdm_hangup) &&
                    (MDM_Data_Ready (link_ptr->hwi.dev_ptr)))
            {
                /* Read in one char. */
                MDM_Get_Char (&mdm_response[mdm_index++],
                                    link_ptr->hwi.dev_ptr);

                /* Make sure the OK does not get split between the last
                   byte and the first byte. If we just placed a byte in the
                   last position then move it to the first. */
                if (mdm_index > 2)
                {
                    /* Move the byte. */
                    mdm_response[0] = mdm_response[2];

                    /* Set the index past the one we just moved. */
                    mdm_index = 1;
                }

                /* Check to see if we got an OK back from the modem. */
                if (strstr (mdm_response, "OK"))

                    /* The modem did return an OK. Set the hangup flag to true. */
                    mdm_hangup = NU_TRUE;
            }

        } while ((wait_for_modem && !mdm_hangup) &&
                        (mdm->hangup_attempts <= MDM_MAX_HANGUP_ATTEMPTS));

        /* If the modem did not hangup and we are not out of attempts
           then we need to try again. */
        if ((!mdm_hangup) &&
            (mdm->hangup_attempts <= MDM_MAX_HANGUP_ATTEMPTS))
        {
            /* Send the AT hangup command to the modem again. */
            MDM_Control_String(MDM_HANGUP_STRING,
                                link_ptr->hwi.dev_ptr);

            /* Set an event to check the status of the modem. */
            TQ_Timerset(PPP_Event, (UNSIGNED) link_ptr->hwi.dev_ptr,
                        TICKS_PER_SECOND, MDM_HANGUP);
        }
        else if (!wait_for_modem)
        {
            /* Change to the next state after the modem has hungup.
               This is the only spot where the other layers and the
               modem layer are mixed. NOTE that at this point the
               echo counter is not being used and was previously set
               by the original call to this function. */
            link_ptr->hwi.state = INITIAL;

            /* Clear any remaining modem events. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)link_ptr->hwi.dev_ptr, MDM_HANGUP);
        }

    }

    return NU_SUCCESS;
} /* MDM_Hangup */



/*************************************************************************
* FUNCTION
*
*       NU_Purge_Terminal_Buffer
*
* DESCRIPTION
*
*       This function calls a function to reset the receive buffer,
*       purging all characters that still remain in the buffer.
*
* INPUTS
*
*       CHAR        *link_name      Pointer to the link for which to clear
*                                   the buffer
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Purge_Terminal_Buffer(CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this name. */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure that a device was found. */
    if (dev_ptr)
    {
        status = MDM_Purge_Input_Buffer(dev_ptr);
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Purge_Terminal_Buffer */


/*************************************************************************
* FUNCTION
*
*       MDM_Purge_Input_Buffer
*
* DESCRIPTION
*
*       This function resets the receive buffer, purging all characters that
*       still remain in the buffer.
*
* INPUTS
*
*       *dev_ptr          Pointer to the link for which to clear the buffer*
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS MDM_Purge_Input_Buffer(DV_DEVICE_ENTRY *dev_ptr)
{
    INT             prev_value;
    LINK_LAYER      *link_layer;
    MDM_LAYER       *mdm;

    /* Get the modem structure for this device. */
    link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    mdm = (MDM_LAYER*)link_layer->link;

    prev_value = NU_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Reset the read and write pointers. */
    mdm->recv_buffer.mdm_read =
        mdm->recv_buffer.mdm_write =
        mdm->recv_buffer.mdm_head;

    /* Update the buffer status to empty. */
    mdm->recv_buffer.mdm_buffer_status = MDM_BUFFER_EMPTY;

    NU_Control_Interrupts(prev_value);

    /* Return the status */
    return NU_SUCCESS;

} /* MDM_Purge_Input_Buffer */


/*************************************************************************
* FUNCTION
*
*       NU_Change_Communication_Mode
*
* DESCRIPTION
*
*       This function calls a function to change the communication mode
*       that the UART is operating in.  There are two modes.  Terminal
*       communication mode might be used to log into a dial up router.
*       Network mode is for exchanging IP packets
*
* INPUTS
*
*       mode                 The mode of operation desired.
*       *link_name           Name of the link
*
* OUTPUTS
*
*       STATUS              Indicates success or failure of the operation.
*
*************************************************************************/
STATUS NU_Change_Communication_Mode(INT mode, CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Is the mode valid. */
    if( (mode != MDM_NETWORK_COMMUNICATION) &&
        (mode != MDM_TERMINAL_COMMUNICATION) )
    {
        /* Return from function in user mode. */
        NU_USER_MODE();
        return NU_INVALID_MODE;
    }

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    if(dev_ptr)
        status = MDM_Change_Communication_Mode(mode, dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Change_Communication_Mode */


/*************************************************************************
* FUNCTION
*
*       MDM_Change_Communication_Mode
*
* DESCRIPTION
*
*       This function changes the communication mode that the UART is operating
*       in.  There are two modes.  Terminal communication mode might be used
*       to log into a dial-up router. Network mode is for exchanging IP packets
*
* INPUTS
*
*       mode                 The mode of operation desired.
*       *dev_ptr             Pointer to the device.
*
* OUTPUTS
*
*       STATUS              Indicates success or failure of the operation.
*
*************************************************************************/
STATUS MDM_Change_Communication_Mode(INT mode, DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declare variables. */
    LINK_LAYER      *link_layer;

    /* Clear the input buffer for this device. */
    MDM_Purge_Input_Buffer(dev_ptr);

    /* Switch the mode in the PPP layer structure as well. */
    link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
 
    link_layer->comm_mode = mode;

    return NU_SUCCESS;

} /* MDM_Change_Comm_Mode */


/*************************************************************************
* FUNCTION
*
*       MDM_Wait_For_Client
*
* DESCRIPTION
*
*       This function waits for a client to call. If only returns once a
*       successful modem connection is made.
*
* INPUTS
*
*       DV_DEVICE_ENTRY     *dev_ptr    Pointer to the device from which
*                                       to accept a PPP client.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS MDM_Wait_For_Client (DV_DEVICE_ENTRY *dev_ptr)
{
    CHAR        mdm_string [81];
    CHAR        rx_char;
    INT         i, index, connected;
    LINK_LAYER *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    MDM_LAYER  *mdm = (MDM_LAYER*)link_layer->link;
    SERIAL_SESSION *uart = link_layer->uart;
    STATUS      status = ~NU_SUCCESS;
    UINT32      index_save;

#if(PPP_ENABLE_CLI == NU_TRUE)
    UINT8       cli_found = NU_FALSE, cli_index = 0;
#endif

#ifdef NU_DEBUG_PPP
    PrintInfo("\nwaiting for client ");
#endif

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
           __FILE__, __LINE__);
    }

    /* Reset the UART */
    SDC_Reset(uart);

    link_layer->hwi.state = STARTING;

    /* Clear the hangup attempts since we are starting a new session. */
    mdm->hangup_attempts = 0;

    /* We are not currently connected. */
    connected = NU_FALSE;
    index     = 0;

    /* Save the index before releasing the semaphore. */
    index_save = dev_ptr->dev_index;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
    }

    /* Wait for modem */
    NU_Sleep (TICKS_PER_SECOND);

    /* Grab the semaphore again. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
           __FILE__, __LINE__);
    }

    /* Check if the device has not been removed. */
    if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
    {
        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
        }

        /* Device has been removed, so return from this
        function. */
        return NU_INVALID_LINK;
    }

    /* Change to terminal mode so we can talk to the modem. */
    MDM_Change_Communication_Mode (MDM_TERMINAL_COMMUNICATION, dev_ptr);

    /* Create the modem string to send to modem. */
    strcpy(mdm_string, MDM_ACCEPT_CALL);

    /* Add number of rings to the string. */
    i = strlen(mdm_string);
    NU_ITOA((int)mdm->num_rings, &mdm_string[i], 10);
    strcat(mdm_string, MDM_STRING_TERMINATOR);

    /* Tell the modem to accept a caller. */
    MDM_Control_String (mdm_string, dev_ptr);

    /* Clear out the modem buffer. */
    MDM_Purge_Input_Buffer(dev_ptr);

    /* Wait for a caller. */
    while ((!connected) && (link_layer->connection_status != NU_PPP_WAITING_ABORTED))
    {
        /* Save the index before releasing the semaphore. */
        index_save = dev_ptr->dev_index;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
        }

        /* Wait for modem */
        NU_Sleep (TICKS_PER_SECOND);

        /* Grab the semaphore again. */
        if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
               __FILE__, __LINE__);
        }

        /* Check if the device has not been removed. */
        if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
        {

            /* Device has been removed, so return from this
            function. */
            status = NU_INVALID_LINK;
            break;
        }

        while (MDM_Data_Ready(dev_ptr))
        {
            /* Get the char that came in */
            if (MDM_Get_Char (&rx_char, dev_ptr) == NU_SUCCESS)
            {

#if(PPP_ENABLE_CLI == NU_TRUE)
                /* See if we got a caller line identification. */
                if (strstr (mdm_string, MDM_NUM_PREFIX) && (cli_found == NU_FALSE))
                {
                    /* Check if we have got the caller's complete number. */
                    if(rx_char == MDM_NUM_TERMINATOR)
                    {
                        /* CLI has been stored. */
                        cli_found = NU_TRUE;
                    }
                    else
                    {
                        /* Store the remote client line identification. */
                        mdm->remote_num[cli_index++] = rx_char;
                        mdm->remote_num[cli_index] = 0;

                        if(cli_index + 1 >= MDM_CLI_MAX)
                        {
                            /* Reset. */
                            cli_index = 0;
                        }
                    }
                }
#endif

                if ((index + 1) >= 80)
                    index = 0;

                /* Put it in our buffer so we can look at it. */
                mdm_string [index++] = rx_char;

                /* Null the end. */
                mdm_string [index]   = 0;

                /* See if we got a connection. */
                if (strstr (mdm_string, "CONNECT"))
                {
                    /* Wait for the baud rate. */
                    NU_Sleep(TICKS_PER_SECOND / 18);

                    index = 0;

                    /* read in the baud rate */
                    while (MDM_Data_Ready(dev_ptr) &&
                                        (index < 80))
                    {
                        MDM_Get_Char (&rx_char, dev_ptr);
                        mdm_string[index++] = rx_char;
                        mdm_string[index] = 0;
                    }

                    /* convert the baud rate to a number and store it */
                    mdm->baud_rate = NU_ATOL(mdm_string);

#ifdef NU_DEBUG_PPP
                    PrintInfo("\n***MODEM OUTPUT***\n");
                    PrintInfo(mdm_string);
                    PrintInfo("\n");
#endif

                    /* Set this flag to true so that we will fall out of
                       the loop and exit this function. */
                    connected = NU_TRUE;
                    status = NU_SUCCESS;
                }
            }
        }
    }

    if (link_layer->connection_status == NU_PPP_WAITING_ABORTED)
    {
        /* Hangup the modem. */
        link_layer->hwi.disconnect(link_layer, NU_TRUE);

        status = NU_PPP_ATTEMPT_ABORTED;
    }

    else if (status == NU_SUCCESS)
    {
        /* Switch back to network packet mode. */
        MDM_Change_Communication_Mode (MDM_NETWORK_COMMUNICATION, dev_ptr);

        link_layer->hwi.state = OPENED;
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
    }

    return status;
} /* MDM_Wait_For_Client */


/*************************************************************************
* FUNCTION
*
*       NU_Put_Terminal_Char
*
* DESCRIPTION
*
*       This function calls a function which sends one byte to the UART.
*
* INPUTS
*
*       CHAR            c           The character to send
*       CHAR*           link_name   Pointer to the name of the link
*                                   to send the character to
*
* OUTPUTS
*
*       STATUS                      NU_SUCCESS if the character was sent.
*                                   Otherwise NU_INVALID_LINK is returned.
*
*************************************************************************/
STATUS NU_Put_Terminal_Char (CHAR c, CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure the link was found. */
    if (dev_ptr)
        status = MDM_Put_Char (c, dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Put_Terminal_Char */




/*************************************************************************
* FUNCTION
*
*       MDM_Put_Char
*
* DESCRIPTION
*
*       This function sends one byte to the UART.
*
* INPUTS
*
*       c           The character to send
*       *dev_ptr    Pointer to the device to send the character to
*
*
* OUTPUTS
*
*       STATUS      NU_SUCCESS if the character was sent.
*
*************************************************************************/
STATUS MDM_Put_Char (CHAR c, DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;

    SERIAL_SESSION  *uart = link_layer->uart;

    /* Send the byte */
    NU_Serial_Putchar (uart, c);

    /* Return the status */
    return NU_SUCCESS;

} /* MDM_Put_Char */


/*************************************************************************
* FUNCTION
*
*       NU_Carrier
*
* DESCRIPTION
*
*       This function checks for the presence of a carrier from the UART.
*
* INPUTS
*
*       CHAR*       link_name   Pointer to the link for which to check
*                               for a carrier
*
* OUTPUTS
*
*       STATUS                  The state of the carrier or NU_INVALID_LINK
*
*************************************************************************/
STATUS NU_Carrier (CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure the link was found. */
    if (dev_ptr)
        status = MDM_Carrier(dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Carrier */


/*************************************************************************
* FUNCTION
*
*       NU_Reset_Modem
*
* DESCRIPTION
*
*       Reset and Initialize the Modem before dialing out.
*
* INPUTS
*
*       CHAR* if_name         Name of the interface
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID NU_Reset_Modem (CHAR *if_name)
{
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (if_name)
    {
        /* Reset the modem. */
        NU_Modem_Control_String("ATZ^M", if_name);

        /* Wait for the modem. */
        NU_Sleep(2 * TICKS_PER_SECOND);

        /* Modem Initialization string. */
        NU_Modem_Control_String("ATm1S0=0V1X4&K0^M", if_name);

        /* Wait for the modem. */
        NU_Sleep(2 * TICKS_PER_SECOND);
    }
    else
    {
        NLOG_Error_Log("Invalid Interface name, failed to Reset the Modem",
                       NERR_FATAL, __FILE__, __LINE__);
    }

    NU_USER_MODE();

} /* NU_Reset_Modem */



/*************************************************************************
* FUNCTION
*
*       MDM_Carrier
*
* DESCRIPTION
*
*       This function checks for the presence of a carrier from the UART.
*
* INPUTS
*
*       *dev_ptr    Pointer to the device for which to check for a carrier
*
* OUTPUTS
*
*       STATUS      The state of the carrier
*
*************************************************************************/
STATUS MDM_Carrier (DV_DEVICE_ENTRY *dev_ptr)
{
    /* In old serial driver, it was stub also. */
    return NU_TRUE;

} /* MDM_Carrier */



/*************************************************************************
* FUNCTION
*
*       MDM_Modem_Connected
*
* DESCRIPTION
*
*       This function checks for the presence of a modem by
*       querying the modem to respond to the AT command.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device structure
*                                        for this device, i.e. the modem
*                                        to query.
* OUTPUTS
*
*       STATUS                          NU_TRUE or NU_FALSE
*
*************************************************************************/
STATUS MDM_Modem_Connected (DV_DEVICE_ENTRY *dev_ptr)
{
    CHAR     mdm_response[3];
    INT      mdm_index = 0, mdm_there = NU_FALSE;
    INT      sleep_loops;
    UINT32   index_save;

    /* Change to terminal  mode */
    MDM_Change_Communication_Mode(MDM_TERMINAL_COMMUNICATION, dev_ptr);

    /* Purge any characters that were already in the receive buffer. */
    MDM_Purge_Input_Buffer(dev_ptr);

    /* Send the AT command to the modem. */
    MDM_Control_String(MDM_ATTENTION, dev_ptr);

    /* We will only wait MDM_MAX_DETECTION_WAIT_LOOPS seconds for the modem to respond.
       Each loop we sleep for one second. */
    sleep_loops = MDM_MAX_DETECTION_WAIT_LOOPS;

    /* Loop until the modem responds or we timeout. */
    while ( (mdm_there == NU_FALSE) && (sleep_loops > 0) )
    {
        /* Save the index before releasing the semaphore. */
        index_save = dev_ptr->dev_index;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
        }

        /* Wait for the response. */
        NU_Sleep (SCK_Ticks_Per_Second);

        /* Grab the semaphore again. */
        if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
               __FILE__, __LINE__);
        }

        /* Check if the device has not been removed. */
        if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
        {
            /* Device has been removed, so return from this
            function. */
            return NU_INVALID_LINK;
        }

        /* Decrement the number of times to sleep. */
        --sleep_loops;

        /* We have already sent the AT command. So check the modem
           buffer for an OK response from the modem. */
        while ( (MDM_Data_Ready (dev_ptr) == NU_TRUE)
                 && (mdm_there == NU_FALSE) )
        {
            /* Read in one char. */
            MDM_Get_Char (&mdm_response[mdm_index++], dev_ptr);

            /* Make sure the the OK does not get split between the last
               byte and the first byte. If we just placed a byte in the
               last position then move it to the first. */
            if (mdm_index > 2)
            {
                /* Move the byte. */
                mdm_response[0] = mdm_response[2];

                /* Set the index past the one we just moved. */
                mdm_index = 1;
            }

            /* Check to see if we got an OK back from the modem. */
            if (strstr (mdm_response, "OK"))

                /* The modem did return an OK. Set the flag to true. */
                mdm_there = NU_TRUE;
        }
    }

    return (mdm_there);

} /* MDM_Modem_Connected */



/*************************************************************************
* FUNCTION
*
*       MDM_Rings_To_Answer
*
* DESCRIPTION
*
*       This function allows the user to change the number of rings the
*       modem will answer on.
*
* INPUTS
*
*       CHAR  *link_name         name of the PPP device
*       UINT8 num_rings          number of rings (0-255)
*
* OUTPUTS
*
*       STATUS                   NU_SUCCESS or NU_INVALID_LINK
*
*************************************************************************/
STATUS MDM_Rings_To_Answer(CHAR *link_name, UINT8 num_rings)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_SUCCESS;
    LINK_LAYER      *link_layer;
    MDM_LAYER       *mdm;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure the link was found. */
    if (dev_ptr)
    {
        /* Get the address of the modem structure for this device. */
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
        mdm = (MDM_LAYER*)link_layer->link;

        mdm->num_rings = num_rings;
    }
    else
        /* Set the error status. */
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* MDM_Rings_To_Answer */

/*************************************************************************
* FUNCTION
*
*       NU_Modem_Set_Local_Num
*
* DESCRIPTION
*
*       This function calls a function which specifies the dialing number
*       attached to the modem. Remote clients dial to this number.
*
* INPUTS
*
*       *link_name               Name of the PPP device
*       *local_num               Number which is attached to a modem
*
* OUTPUTS
*
*       STATUS                   NU_SUCCESS or NU_INVALID_LINK
*
*************************************************************************/
STATUS NU_Modem_Set_Local_Num(CHAR *link_name, CHAR *local_num)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure the link was found. */
    if (dev_ptr)
        status = MDM_Set_Local_Num(dev_ptr, local_num);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return status;

} /* MDM_Set_Local_Num */


/*************************************************************************
* FUNCTION
*
*       MDM_Set_Local_Num
*
* DESCRIPTION
*
*       This function specifies the dialing number attached to the modem.
*       Remote clients dial to this number.
*
* INPUTS
*
*       *dev_ptr                 Pointer to PPP device
*       *local_num               Number which is attached to a modem
*
* OUTPUTS
*
*       STATUS                   NU_SUCCESS
*
*************************************************************************/
STATUS MDM_Set_Local_Num(DV_DEVICE_ENTRY *dev_ptr, CHAR *local_num)
{
    LINK_LAYER      *link_layer;
    MDM_LAYER       *mdm;

    /* Get the address of the modem structure for this device. */
    link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    mdm = (MDM_LAYER*)link_layer->link;

    /* store the number */
    strcpy(mdm->local_num, local_num);

    return NU_SUCCESS;

} /* MDM_Set_Local_Num */

/*************************************************************************
* FUNCTION
*
*       NU_Modem_Get_Remote_Num
*
* DESCRIPTION
*
*       This function calls a function which returns the calling number or
*       called number of a link
*
* INPUTS
*
*       *link_name              Name of the PPP device
*       *remote_num             Number which is attached to a modem
*
* OUTPUTS
*
*       STATUS                   NU_SUCCESS or NU_INVALID_LINK
*
*************************************************************************/
STATUS NU_Modem_Get_Remote_Num(CHAR *link_name, CHAR *remote_num)
{
    /* Declaring Variables. */
    STATUS          status = NU_INVALID_LINK;
    DV_DEVICE_ENTRY *dev_ptr;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link */
    dev_ptr = DEV_Get_Dev_By_Name (link_name);

    /* Make sure the link was found. */
    if (dev_ptr)
        status = MDM_Get_Remote_Num(dev_ptr, remote_num);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return status;

} /* NU_Modem_Get_Remote_Num */


/*************************************************************************
* FUNCTION
*
*       MDM_Get_Remote_Num
*
* DESCRIPTION
*
*       This function returns the calling number or called number of a
*       link.
*
* INPUTS
*
*       *dev_ptr                PPP device
*       *remote_num             Number which is attached to a modem
*
* OUTPUTS
*
*       STATUS                   NU_SUCCESS or NU_INVALID_LINK
*
*************************************************************************/
STATUS MDM_Get_Remote_Num(DV_DEVICE_ENTRY *dev_ptr, CHAR *remote_num)
{

#if(PPP_ENABLE_CLI == NU_TRUE)
    MDM_LAYER       *mdm;

    /* Get the address of the modem structure for this device. */
    mdm = (MDM_LAYER*)((LINK_LAYER *)dev_ptr->dev_link_layer)->link;

    /* Return the number */
    strcpy(remote_num, mdm->remote_num);
#else

    /* Remove warnings. */
    UNUSED_PARAMETER(dev_ptr);

    /* Return empty string. */
    strcpy(remote_num, " ");
#endif

    return NU_SUCCESS;

} /* MDM_Get_Remote_Num */

