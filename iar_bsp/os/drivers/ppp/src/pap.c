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
*       pap.c
*
*   COMPONENT
*
*       PAP - Password Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the password authentication protocol that
*       is used to log into a PPP server and authenticate a calling
*       PPP client.
*
*   DATA STRUCTURES
*
*       _passwordlist
*
*   FUNCTIONS
*
*       PAP_Interpret
*       PAP_Send_Authentication
*       PAP_Send_Authentication_Ack_Nak
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       tcp_errs.h
*       netevent.h
*       um_defs.h
*       nu_ppp.h
*       pm_extr.h
*
*************************************************************************/
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/tcp_errs.h"
#include "networking/netevent.h"
#include "networking/um_defs.h"
#include "drivers/nu_ppp.h"
#include "drivers/pm_extr.h"

#if (PAP_DEBUG_PRINT_OK == NU_TRUE)
#define PrintInfo(s)        PPP_Printf(s)
#define PrintErr(s)         PPP_Printf(s)
#else
#define PrintInfo(s)
#define PrintErr(s)
#endif

/* Import all external variables */
#if (PPP_ENABLE_UM_DATABASE == NU_FALSE)
extern PPP_USER           _passwordlist[];
#endif


/*************************************************************************
* FUNCTION
*
*       PAP_Interpret
*
* DESCRIPTION
*
*       This function processes the incoming PAP packet. This involves
*       responding to an authentication request and informing the upper
*       layer if PAP has succeed or failed.
*
* INPUTS
*
*       NET_BUFFER              *buf_ptr    Pointer to the incoming PAP
*                                           packet
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PAP_Interpret(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY         *dev_ptr;
    LCP_LAYER               *lcp;
    LINK_LAYER              *link;
    CHAR                    temp_id [PPP_MAX_ID_LENGTH];
    CHAR                    temp_pw [PPP_MAX_PW_LENGTH];
    INT                     temp_id_len, temp_pw_len, x;
    STATUS                  ret_status;
    UINT8   HUGE            *pap_pkt = buf_ptr->data_ptr;

    PrintInfo("PAP_Interpret\n");

    /* Get pointers to the link structures. */
    dev_ptr = buf_ptr->mem_buf_device;
    link    = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp     = &link->lcp;

    /* Make sure that LCP is done. */
    if (lcp->state == OPENED)
    {
        switch (pap_pkt[0])
        {
        case PAP_AUTHENTICATE_REQUEST :

            PrintInfo("req\n");

            /* Store the identifier of the packet. */
            link->authentication.auth_identifier = pap_pkt[1];

            /* We need to pull out the ID and PW, then check them
               against our internal password list. */

            /* get the id, we know where it starts in the packet. */

            /* Get the length of the value. */
            temp_id_len = pap_pkt [PAP_ID_LENGTH_OFFSET];

            /* Copy it over. */
            for (x = 0;(x < temp_id_len) &&(x < (PPP_MAX_ID_LENGTH - 1)); x++)
               temp_id [x] = pap_pkt [PAP_ID_OFFSET + x];

            /* Null terminate it. */
            temp_id [x] = (CHAR)0;

            /* store the ID */
            strcpy(link->authentication.login_name, temp_id);
            link->authentication.name_len = (UINT8)x;

            /* now get the password where it starts in the packet depends on the
               id length field. */
            temp_pw_len = pap_pkt [PAP_ID_OFFSET + temp_id_len];

            /* Copy it over. */
            for (x = 0;(x < temp_pw_len) &&(x < (PPP_MAX_PW_LENGTH - 1)); x++)
               temp_pw [x] = pap_pkt [PAP_ID_OFFSET + temp_id_len + 1 + x];

            /* Null terminate it. */
            temp_pw [x] = (CHAR)0;

            /* store the password */
            strcpy(link->authentication.login_pw, temp_pw);
            link->authentication.pw_len = (UINT8)x;

            if(link->hwi.itype & PPP_ITYPE_L2TP_LAC)
            {
                EQ_Put_Event(PPP_Event, (UNSIGNED)buf_ptr->mem_buf_device, AUTHENTICATED);
                return;
            }

            /* Validate user information via the user database */
#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
            ret_status = UM_Validate_User(temp_id, temp_pw, UM_PPP);
            if (ret_status == NU_SUCCESS)
                ret_status = NU_TRUE;
#else
            ret_status = AUTH_Check_Login(temp_id, temp_pw);
#endif
            /* Is it ok */
            if (ret_status == NU_TRUE)
            {
                /* Let the CLIENT know that everything checked out. */
                PAP_Send_Authentication_Ack_Nak(buf_ptr, PAP_AUTHENTICATE_ACK);

                /* Set the state before setting the event. */
                link->authentication.state = AUTHENTICATED;

                /* Move on to the next phase */
                EQ_Put_Event(PPP_Event,(UNSIGNED)buf_ptr->mem_buf_device, AUTHENTICATED);
            }
            else
            {
                /* tell the CLIENT that they failed to login */
                PAP_Send_Authentication_Ack_Nak(buf_ptr, PAP_AUTHENTICATE_NAK);

                /* Stop the PPP session. */
                NU_Set_Events(&link->negotiation_progression, PPP_AUTH_FAIL, NU_OR);
            }

            break;

        case PAP_AUTHENTICATE_ACK:

            PrintInfo("ack\n");

            /* Set the state before setting the event. */
            link->authentication.state = AUTHENTICATED;

            /* Clear any remaining timeout events. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT,(UNSIGNED)dev_ptr, PAP_SEND_AUTH);

            /* Notify others that the link is authenticated. */
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, AUTHENTICATED);

            break;

        case PAP_AUTHENTICATE_NAK:

            PrintInfo("nak\n");

            /* Clear any remaining timeout events. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT,(UNSIGNED)dev_ptr, PAP_SEND_AUTH);

            /* We have received a negative response from the server,
               let the upper layer know. */
            NU_Set_Events(&link->negotiation_progression, PPP_AUTH_FAIL, NU_OR);

            break;
        } /* switch */
    } /* if */

    /* Release the buffer space */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

} /* PAP_Interpret */



/*************************************************************************
* FUNCTION
*
*       PAP_Send_Authentication
*
* DESCRIPTION
*
*       This function sends an authentication request.
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
VOID PAP_Send_Authentication(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER              *link;
    AUTHENTICATION_LAYER    *auth;
    NET_BUFFER              *buf_ptr;
    LCP_FRAME               *out_frame;
    UINT16                  len = 0;

    PrintInfo("PAP_Send_Authentication\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, PAP_AUTHENTICATE_REQUEST, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Map an LCP data access frame to the outgoing packet. */
    out_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Get a pointer to the authentication structure. */
    link = (LINK_LAYER*)dev_ptr->dev_link_layer;
    auth = &link->authentication;

    /* Add the length of login name. */
    out_frame->data[len++] = auth->name_len;

    /* Copy the login name into the packet. */
    memcpy(&out_frame->data[len], auth->login_name, auth->name_len);
    len = len + auth->name_len;

    /* Add the length of the password. */
    out_frame->data[len++] = auth->pw_len;

    /* Copy the password into the packet. */
    memcpy(&out_frame->data[len], auth->login_pw, auth->pw_len);
    len = len + auth->pw_len;

    /* Send the packet. */
    out_frame->len = out_frame->len + len;

    if(link->hwi.itype & PPP_ITYPE_L2TP_LNS)
    {
        buf_ptr->mem_buf_device = dev_ptr;
        PAP_Interpret(buf_ptr);
    }
    else
    {
        LCP_Send(dev_ptr, buf_ptr, PPP_PAP_PROTOCOL);

        /* Set up a timeout event for PAP. */
        TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, PAP_SEND_AUTH);
    }

} /* PAP_Send_Authentication */



/*************************************************************************
* FUNCTION
*
*       PAP_Send_Authentication_Ack_Nak
*
* DESCRIPTION
*
*       Sends a PAP ack or nak packet
*
* INPUTS
*
*       NET_BUFFER                  *in_buf_ptr Pointer to the incoming PAP
*                                               packet
*       UINT8                       pkt_type    Which type to send - ACK or NAK
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PAP_Send_Authentication_Ack_Nak(NET_BUFFER *in_buf_ptr, UINT8 pkt_type)
{
    DV_DEVICE_ENTRY         *dev_ptr;
    NET_BUFFER              *buf_ptr;
    LCP_FRAME               *in_frame;

    if (pkt_type == PAP_AUTHENTICATE_ACK)
        PrintInfo("PAP send authentication ACK\n");
    else
        PrintInfo("PAP send authentication NAK\n");

    dev_ptr = in_buf_ptr->mem_buf_device;

    /* Get a pointer to the data part of the CHAP packet. */
    in_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    if (pkt_type == PAP_AUTHENTICATE_ACK)
        buf_ptr = LCP_New_Buffer(in_buf_ptr->mem_buf_device, PAP_AUTHENTICATE_ACK, in_frame->id);
    else
        buf_ptr = LCP_New_Buffer(in_buf_ptr->mem_buf_device, PAP_AUTHENTICATE_NAK, in_frame->id);

    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Nothing else goes into the packet, so just send it. */
    LCP_Send(dev_ptr, buf_ptr, PPP_PAP_PROTOCOL);

} /* PAP_Send_Authentication_Ack_Nak */
