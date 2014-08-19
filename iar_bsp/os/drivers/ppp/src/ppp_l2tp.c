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
*       ppp_l2tp.c
*
*   COMPONENT
*
*       L2TP - Layer Two Tunneling Protocol
*
*   DESCRIPTION
*
*       This file contains support required from PPP by Nucleus L2TP
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PPP_L2TP_Init
*       PPP_L2TP_Get_Opt
*       PPP_L2TP_Set_Opt
*       PPP_L2TP_LNS_Do_Proxy
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       ppp_l2tp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"
#include "drivers/ppp_l2tp.h"

/*************************************************************************
* FUNCTION
*
*       PPP_L2TP_Init
*
* DESCRIPTION
*
*       This function initializes the PPP specific data for the L2TP.
*
* INPUTS
*
*       dev_index            Index of the device.
*       l2tp_type            Type of this L2TP entity i.e LNS, LAC.
*
* OUTPUTS
*
*       NU_SUCCESS           If the call is successful.
*       NU_NOT_FOUND         The device passed in doesn't exist.
*
*************************************************************************/
STATUS PPP_L2TP_Init(UINT32 dev_index, UINT32 l2tp_type)
{
    /* Declaring Variables. */
    DV_DEVICE_ENTRY         *dev_ptr;
    LINK_LAYER              *link_layer;
    LCP_LAYER               *lcp;
    AUTHENTICATION_LAYER    *auth;

    /* Get the pointer to the device. */
    dev_ptr = DEV_Get_Dev_By_Index (dev_index);

    if (dev_ptr)
    {
        /* Get hold of the different PPP layers we'll be using later. */
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
        lcp        = &(link_layer->lcp);
        auth       = &(link_layer->authentication);

        /* Allocate memory for initial configuration request. */
        if (NU_Allocate_Memory(PPP_Memory,
                               (VOID **)&lcp->options.remote.init_cfg_req,
                               sizeof(LCP_FRAME),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Get out */
            return (NU_NO_MEMORY);
        }

        /* Initialize the length, */
        ((LCP_FRAME *)lcp->options.remote.init_cfg_req)->len = 0;

        /* Allocate memory for the last configuration request received. */
        if (NU_Allocate_Memory(PPP_Memory,
                               (VOID **)&lcp->options.remote.last_cfg_req,
                               sizeof(LCP_FRAME), NU_NO_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Get out */
            return (NU_NO_MEMORY);
        }

        /* Initialize the length. */
        ((LCP_FRAME *)lcp->options.remote.last_cfg_req)->len = 0;

        /* Allocate the memory for last configuration request sent. */
        if (NU_Allocate_Memory(PPP_Memory,
                               (VOID **)&lcp->options.local.last_cfg_req,
                               sizeof(LCP_FRAME),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Get out */
            return (NU_NO_MEMORY);
        }

        /* Initialize the length. */
        ((LCP_FRAME *)lcp->options.local.last_cfg_req)->len = 0;

        /* Allocate memory for proxy authentication data. */
        if (NU_Allocate_Memory(PPP_Memory,
                               (VOID **)&auth->chap.chap_ext,
                               sizeof(LCP_FRAME),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Get out */
            return (NU_NO_MEMORY);
        }

        /* This device would now be used for L2TP proxy authentication. */
        link_layer->hwi.itype = (UINT16)(link_layer->hwi.itype | l2tp_type);
    }

    /* Return status. */
    return (NU_SUCCESS);

} /* PPP_L2TP_Init */

/*************************************************************************
* FUNCTION
*
*       PPP_L2TP_Get_Opt
*
* DESCRIPTION
*
*       This function gets the PPP specific data for the L2TP.
*
* INPUTS
*
*       dev_index            Index of the device.
*       *data_ptr            Pointer to the place where to write data.
*       *len                 Size of the space available for writing data
*                            and when returning from this function it
*                            specifies the size of the data actually
*                            written.
*       option               Specifies which data to get.
*
* OUTPUTS
*
*       NU_SUCCESS           Data successfully gotten.
*       NU_NOT_FOUND         The option specified/device doesn't exist.
*
*************************************************************************/
STATUS PPP_L2TP_Get_Opt(UINT32 dev_index, UINT8 *data_ptr, UINT16 *len,
                        UINT8 option)
{
    /* Declaring Variables. */
    DV_DEVICE_ENTRY         *dev_ptr;
    LINK_LAYER              *link_layer;
    LCP_LAYER               *lcp;
    MDM_LAYER               *mdm;
    AUTHENTICATION_LAYER    *auth;
    STATUS                  status = NU_SUCCESS;
    UINT16                  size = 0;
    PPP_L2TP_PROXY_GET      *proxy;

    /* Get the pointer to the device. */
    dev_ptr = DEV_Get_Dev_By_Index (dev_index);

    if (dev_ptr)
    {
        /* Get hold of the different PPP layers we'll be using later. */
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
        mdm        = (MDM_LAYER *)link_layer->link;
        lcp        = &(link_layer->lcp);
        auth       = &(link_layer->authentication);

        switch(option)
        {

        /* Number which is called. */
        case PPP_L2TP_CALLED_NUM:

            /* Get the size of the number. */
            size = (UINT16)strlen(mdm->local_num);

            /* Check if enough space is available. */
            if (*len >= size)
            {
                /* Copy the number. */
                strcpy((CHAR *)data_ptr, (CHAR *)mdm->local_num);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;

#if(PPP_ENABLE_CLI == NU_TRUE)

        /* Number from which a call is made */
        case PPP_L2TP_CALLING_NUM:

            /* Get the size of the number. */
            size = strlen(mdm->remote_num);

            /* Check if enough space is available. */
            if (*len >= size)
            {
                /* Copy the number. */
                strcpy((CHAR *)data_ptr, (CHAR *)mdm->remote_num);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;
#endif

        /* Baud rates of the PPP link. */
        case PPP_L2TP_TX_SPEED:
        case PPP_L2TP_RX_SPEED:

            size = sizeof(UINT32);

            /* Check if enough space is available. */
            if (*len >= size)
            {
                /* Copy the data rate. */
                PUT32(data_ptr, 0, mdm->baud_rate);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;

        case PPP_L2TP_PROXY_DATA:

            status = NU_SUCCESS;

            /* Point to the proxy structure passed in. */
            proxy = (PPP_L2TP_PROXY_GET *)data_ptr;

            size = sizeof(PPP_L2TP_PROXY_GET);

            /* Check if we have adequate space. */
            if (*len < size)
                break;

            /** Initial LCP configuration received from the peer. **/

            if (lcp->options.remote.init_cfg_req == NU_NULL)
            {
                /* A zero length will indicate that a PPP LCP never
                 * happened. */
                (*len) = 0;
                break;
            }

            /* Get the size of the data to be written. */
            proxy->init_rx_lcp_len =
                ((LCP_FRAME *)(lcp->options.remote.init_cfg_req))->len -
                LCP_HEADER_LEN;

            /* Copy the data over. */
            NU_BLOCK_COPY(proxy->init_rx_lcp,
                          ((LCP_FRAME *)lcp->options.remote.init_cfg_req)->data,
                          proxy->init_rx_lcp_len);

            /** Last LCP request sent by us. **/
            if (lcp->options.local.last_cfg_req == NU_NULL)
            {
                /* A zero length will indicate that a PPP LCP never
                 * happened. */
                (*len) = 0;
                break;
            }

            /* Get the size of the data to be written. */
            proxy->last_tx_lcp_len =
                ((LCP_FRAME *)(lcp->options.local.last_cfg_req))->len -
                LCP_HEADER_LEN;

            /* Copy the data. */
            NU_BLOCK_COPY(proxy->last_tx_lcp,
                         ((LCP_FRAME *)(lcp->options.local.last_cfg_req))->data,
                         proxy->last_tx_lcp_len);

            /** Last LCP request received. **/

            if (lcp->options.remote.last_cfg_req == NU_NULL)
            {
                /* A zero length will indicate that a PPP LCP never
                 * happened. */
                (*len) = 0;
                break;
            }

            /* Get the size of the data to be written. */
            proxy->last_rx_lcp_len =
                ((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->len -
                LCP_HEADER_LEN;

            /* Copy the data. */
            NU_BLOCK_COPY(proxy->last_rx_lcp,
                          ((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->data,
                          proxy->last_rx_lcp_len);

            /** Proxy Authentication Name. **/

            /* Get the size of the data to be written. */
            proxy->auth_name_len = (UINT8)strlen(auth->login_name);

            /* Copy the authentication name. */
            strcpy((CHAR *)proxy->auth_name, (CHAR *)auth->login_name);

            switch (lcp->options.local.chap_protocol)
            {
            /* MSCHAP v2 case. */
            case LCP_CHAP_MS2:

#if(PPP_USE_CHAP_MS2 == NU_TRUE)
                /* Authentication type. */
                proxy->auth_type[0] = 0;
                proxy->auth_type[1] = 6;

                /* Proxy Authentication Challenge length. */
                proxy->auth_chal_len = CHAP_MS2_CHALLENGE_VALUE_SIZE;

                /* Proxy Authentication Challenge. */
                NU_BLOCK_COPY(proxy->auth_chal,
                              auth->chap.challenge_value_ms_authenticator,
                              proxy->auth_chal_len);

                /* Proxy authentication id. */
                proxy->auth_id[0] = 0;
                proxy->auth_id[1] = auth->auth_identifier;

                /* Authentication response length. */
                proxy->auth_resp_len  = CHAP_MS_VALUE_SIZE;

                /* Copy the response as it was received. */
                NU_BLOCK_COPY(&(proxy->auth_resp),
                    auth->chap.chap_ext, CHAP_MS_VALUE_SIZE);
#endif

                break;

            /* MSCHAPv1 case. */
            case LCP_CHAP_MS1:

#if(PPP_USE_CHAP_MS1 == NU_TRUE)
                /* Authentication type. */
                proxy->auth_type[0] = 0;
                proxy->auth_type[1] = 5;

                /* Proxy Authentication Challenge length. */
                proxy->auth_chal_len = CHAP_MS1_CHALLENGE_VALUE_SIZE;

                /* Proxy Authentication Challenge. */
                NU_BLOCK_COPY(proxy->auth_chal,
                              auth->chap.challenge_value_ms_authenticator,
                              proxy->auth_chal_len);

                /* Authentication id. */
                proxy->auth_id[0] = 0;
                proxy->auth_id[1] = auth->auth_identifier;

                /* Get the size of the data to be written. */
                proxy->auth_resp_len = CHAP_MS_VALUE_SIZE;

                /* Copy the response as it was received. */
                NU_BLOCK_COPY(&(proxy->auth_resp), auth->chap.chap_ext,
                              CHAP_MS_VALUE_SIZE);
#endif
                break;

            /* Simple CHAP case. */
            case LCP_CHAP_MD5:

                /* Authentication type. */
                proxy->auth_type[0] = 0;
                proxy->auth_type[1] = 2;

                /* Proxy Authentication Challenge length. */
                proxy->auth_chal_len = CHAP_CHALLENGE_VALUE_SIZE;

                /* Proxy Authentication Challenge. */
                PUT32(proxy->auth_chal, 0, auth->chap.challenge_value);

                /* Authentication id. */
                proxy->auth_id[0] = 0;
                proxy->auth_id[1] = auth->auth_identifier;

                /* Get the size of the data to be written. */
                proxy->auth_resp_len = CHAP_MD5_VALUE_SIZE;

                /* Copy the response as it was received. */
                NU_BLOCK_COPY(proxy->auth_resp, auth->chap.chap_ext,
                              proxy->auth_resp_len);

                break;

            /* PAP case. */
            default:

                /* Authentication type. */
                proxy->auth_type[0] = 0;
                proxy->auth_type[1] = 3;

                /* Copy the authentication id. */
                proxy->auth_id[0] = 0;
                proxy->auth_id[1] = auth->auth_identifier;

                /* Get the size of the data to be written. */
                proxy->auth_resp_len = auth->pw_len;

                /* Copy the data. */
                NU_BLOCK_COPY(proxy->auth_resp, auth->login_pw,
                              proxy->auth_resp_len);

                break;
            }

            break;

        case PPP_L2TP_ACCM:

            /* Set the size of the data to be written. */
            size = 8;

            /* Check if enough space is available. */
            if (*len >= size)
            {
                /* Send ACCM */
                PUT32(&data_ptr[0], 0, lcp->options.remote.accm);

                /* Receive ACCM */
                PUT32(&data_ptr[4], 0, lcp->options.local.accm);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;

        case PPP_L2TP_CRC:
        case PPP_L2TP_HARDWARE_OVRNS:
        case PPP_L2TP_BUFFER_OVRNS:
        case PPP_L2TP_TIMOUT_ERRORS:
        case PPP_L2TP_ALIGNMENT_ERRORS:
        case PPP_L2TP_RESULT_CODE:
        case PPP_L2TP_CAUSE_CODE:
        case PPP_L2TP_CALL_SERIAL_NUM:
        default:
            status = NU_NOT_FOUND;
            break;
        }
    }

    /* Device not found. */
    else
    {
        /* set the status. */
        status = NU_NOT_FOUND;
    }

    if (status == NU_SUCCESS)
    {
        /* Set the number of bytes we have written. */
        *len = size;
    }

    /* Return status. */
    return (status);

} /* PPP_L2TP_Get_Opt */

/*************************************************************************
* FUNCTION
*
*       PPP_L2TP_Set_Opt
*
* DESCRIPTION
*
*       This function sets the PPP specific data for the L2TP.
*
* INPUTS
*
*       dev_index            Index of the device.
*       *data_ptr            Pointer to the data to be set.
*       len                  Length of the data.
*       option               Specifies which data to set.
*
* OUTPUTS
*
*       NU_SUCCESS           Data successfully set.
*       NU_NO_MEMORY         Data size is too big.
*       NU_NOT_FOUND         The option specified doesn't exist.
*       NU_NOT_FOUND         The device passed in doesn't exist.
*
*************************************************************************/
STATUS PPP_L2TP_Set_Opt(UINT32 dev_index, UINT8 *data_ptr, UINT16 len,
                        UINT8 option)
{
    /* Declaring Variables. */
    DV_DEVICE_ENTRY         *dev_ptr;
    LINK_LAYER              *link_layer;
    LCP_LAYER               *lcp;
    MDM_LAYER               *mdm;
    AUTHENTICATION_LAYER    *auth;
    STATUS                  status = NU_SUCCESS;
    UINT16                  size;
    PPP_L2TP_PROXY_SAVE     *proxy;

    /* Get the pointer to the device. */
    dev_ptr = DEV_Get_Dev_By_Index (dev_index);

    if (dev_ptr)
    {
        /* Get hold of the different PPP layers we'll be using later. */
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
        mdm        = (MDM_LAYER *)link_layer->link;
        lcp        = &(link_layer->lcp);
        auth       = &(link_layer->authentication);

        switch (option)
        {
        /* Number which is called. */
        case PPP_L2TP_CALLED_NUM:

            /* Check how much space is available. */
            size = sizeof(mdm->local_num);

            /* Check if enough space is available. */
            if (len <= size)
            {
                /* Copy the number. */
                strcpy((CHAR *)mdm->local_num, (CHAR *)data_ptr);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;

#if(PPP_ENABLE_CLI == NU_TRUE)

        /* Number from which a call is made */
        case PPP_L2TP_CALLING_NUM:

            /* Check how much space is available. */
            size = sizeof(mdm->remote_num);

            /* Check if enough space is available. */
            if (len <= size)
            {
                /* Copy the number. */
                strcpy((CHAR *)mdm->remote_num, (CHAR *)data_ptr);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;
#endif

        /* Baud rates of the PPP link. */
        case PPP_L2TP_TX_SPEED:
        case PPP_L2TP_RX_SPEED:

            size = sizeof(UINT32);

            /* Check if enough space is available. */
            if (len <= size)
            {
                /* Copy the data rate. */
                mdm->baud_rate = GET32(data_ptr, 0);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            break;

        case PPP_L2TP_PROXY_DATA:

            /* Point to the structure passed in. */
            proxy = (PPP_L2TP_PROXY_SAVE *)data_ptr;

            if (proxy->init_rx_lcp_len)
            {
                /* Initial LCP configuration received from the remote peer. */
                NU_BLOCK_COPY(((LCP_FRAME *)(lcp->options.remote.init_cfg_req))->data,
                              proxy->init_rx_lcp,
                              proxy->init_rx_lcp_len);

                ((LCP_FRAME *)(lcp->options.remote.init_cfg_req))->len =
                    proxy->init_rx_lcp_len;
            }

            if (proxy->last_tx_lcp_len)
            {
                /* Last LCP request sent by LAC. */
                NU_BLOCK_COPY(((LCP_FRAME *)(lcp->options.local.last_cfg_req))->data,
                              proxy->last_tx_lcp,
                              proxy->last_tx_lcp_len);

                ((LCP_FRAME *)(lcp->options.local.last_cfg_req))->len =
                    proxy->last_tx_lcp_len;
            }

            if (proxy->last_rx_lcp_len)
            {
                /* Last LCP request received by LAC. */
                NU_BLOCK_COPY(((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->data,
                              proxy->last_rx_lcp,
                              proxy->last_rx_lcp_len);

                ((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->len =
                    proxy->last_rx_lcp_len;
            }

            /* Copy the authentication name. */
            if (proxy->auth_name_len)
            {
                strcpy((CHAR *)auth->login_name, (CHAR *)proxy->auth_name);
                auth->name_len = proxy->auth_name_len;
            }

            auth->auth_identifier = (UINT8)proxy->auth_id;

            /* Proxy authentication type. */
            switch (proxy->auth_type)
            {
#if(PPP_USE_CHAP_MS2 == NU_TRUE)

            /* MSCHAPv2 Case. */
            case 6:
                lcp->options.remote.flags |= PPP_FLAG_CHAP_MS2;

                lcp->options.remote.flags |=
                    ~(PPP_FLAG_CHAP_MS1 | PPP_FLAG_CHAP | PPP_FLAG_PAP);

                /* Authentication challenge. */
                if (proxy->auth_chal_len == CHAP_MS2_CHALLENGE_VALUE_SIZE)
                    NU_BLOCK_COPY(auth->chap.challenge_value_ms_authenticator,
                                  proxy->auth_chal, proxy->auth_chal_len);

                /* Authentication response len. */
                auth->chap.chap_ext[0] = proxy->auth_resp_len;

                /* Authentication response. */
                NU_BLOCK_COPY(&auth->chap.chap_ext[1], proxy->auth_resp,
                              proxy->auth_resp_len);

                lcp->options.local.chap_protocol = LCP_CHAP_MS2;
                lcp->options.remote.chap_protocol = LCP_CHAP_MS2;

                break;
#endif

#if(PPP_USE_CHAP_MS1 == NU_TRUE)

            /* MSCHAPv1 Case. */
            case 5:
                lcp->options.remote.flags |= PPP_FLAG_CHAP_MS1;

                lcp->options.remote.flags |=
                    ~(PPP_FLAG_CHAP_MS2 | PPP_FLAG_CHAP | PPP_FLAG_PAP);

                /* Authentication challenge. */
                if (proxy->auth_chal_len == CHAP_MS1_CHALLENGE_VALUE_SIZE)
                    NU_BLOCK_COPY(auth->chap.challenge_value_ms_authenticator,
                                  proxy->auth_chal, proxy->auth_chal_len);

                /* Authentication response len. */
                auth->chap.chap_ext[0] = proxy->auth_resp_len;

                /* Authentication response. */
                NU_BLOCK_COPY(&auth->chap.chap_ext[1], proxy->auth_resp,
                              proxy->auth_resp_len);

                lcp->options.local.chap_protocol = LCP_CHAP_MS1;
                lcp->options.remote.chap_protocol = LCP_CHAP_MS1;

                break;
#endif

            /* Simple CHAP Case. */
            case 2:
                lcp->options.remote.flags |= PPP_FLAG_CHAP;

                lcp->options.remote.flags |=
                    ~(PPP_FLAG_CHAP_MS2 | PPP_FLAG_CHAP_MS1 | PPP_FLAG_PAP);

                if (proxy->auth_chal_len == CHAP_CHALLENGE_VALUE_SIZE)
                    auth->chap.challenge_value = GET32(proxy->auth_chal, 0);

                /* Authentication response len. */
                auth->chap.chap_ext[0] = proxy->auth_resp_len;

                /* Authentication response. */
                NU_BLOCK_COPY(&auth->chap.chap_ext[1], proxy->auth_resp,
                              proxy->auth_resp_len);

                lcp->options.local.chap_protocol = LCP_CHAP_MD5;
                lcp->options.remote.chap_protocol = LCP_CHAP_MD5;

                break;

            /* PAP Case. */
            case 3:
                lcp->options.remote.flags |= PPP_FLAG_PAP;

                lcp->options.remote.flags |=
                    ~(PPP_FLAG_CHAP_MS2 | PPP_FLAG_CHAP_MS1 | PPP_FLAG_CHAP);

                /* Authentication response. */
                NU_BLOCK_COPY(auth->login_pw, proxy->auth_resp,
                              proxy->auth_resp_len);

                auth->pw_len = proxy->auth_resp_len;

                break;

            /* No authentication. */
            case 7:
                lcp->options.remote.flags |=
                    ~(PPP_FLAG_CHAP_MS2 | PPP_FLAG_CHAP_MS1 |
                      PPP_FLAG_CHAP | PPP_FLAG_PAP);

                break;

            default:
                break;
            }

        case PPP_L2TP_ACCM:

            /* Send ACCM */
            lcp->options.remote.accm = GET32(&data_ptr[0], 0);

            /* Receive ACCM */
            lcp->options.remote.accm = GET32(&data_ptr[4], 0);

            /* Update the status. */
            status = NU_SUCCESS;

            break;

        default:
            status = NU_NOT_FOUND;
            break;

        }
    }

    /* Device not found. */
    else
    {
        /* set the status. */
        status = NU_NOT_FOUND;
    }

    /* Return status. */
    return (status);

} /* PPP_L2TP_Set_Opt */

/*************************************************************************
* FUNCTION
*
*       PPP_L2TP_LNS_Do_Proxy
*
* DESCRIPTION
*
*       This function does the proxy LCP and proxy authentication
*       at LNS.
*
* INPUTS
*
*       *dev_ptr             Pointer to the device.
*
* OUTPUTS
*
*       NU_SUCCESS           Proxy LCP and authentication successful.
*       NU_NOT_FOUND         Proxy LCP and authentication unsuccessful.
*
*************************************************************************/
STATUS PPP_L2TP_LNS_Do_Proxy(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variables. */
    LINK_LAYER              *link_layer;
    LCP_LAYER               *lcp;
    STATUS                  status;
    NET_BUFFER              *buf_ptr, *ack_buf;

    /* Get pointers to various layers. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    /* If proxy L2TP AVPs were not received then return. */
    if ((((LCP_FRAME *)(lcp->options.local.last_cfg_req))->len == 0) &&
        (((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->len == 0) )
       return (NU_NOT_FOUND);

    /* If proxy lcp has been done, then do not do it again if any pending
     * LCP requests are encountered. */
    if (lcp->state == OPENED) 
        return (NU_SUCCESS);

    /* Acquire a buffer chain from Free buffer list. */
    buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                       sizeof(LCP_FRAME));

	if (buf_ptr)
	{
	    /* Set the dlist to free the buffer after sending. */
	    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
	
	    /* Set the device. */
	    buf_ptr->mem_buf_device = dev_ptr;
	
	    buf_ptr->data_ptr = buf_ptr->mem_parent_packet + LCP_HEADER_LEN;
	
	    /* Copy the last configuration request into the net buffer. */
	    MEM_Copy_Data(buf_ptr,
	                  (const CHAR HUGE *)((LCP_FRAME *)
	                  (lcp->options.local.last_cfg_req))->data,
	                  ((LCP_FRAME *)(lcp->options.local.last_cfg_req))->len, 0);
	
	    /* Update the data pointer. */
	    buf_ptr->data_ptr -= LCP_HEADER_LEN;
	
	    /* Update the data len. */
	    ((LCP_FRAME*)buf_ptr->data_ptr)->len =
	        ((LCP_FRAME *)(lcp->options.local.last_cfg_req))->len;
	
	    /* Process the last lcp configuration request sent by L2TP LAC to
	     * remote client.
	     */
	    status = LCP_Process_Request(buf_ptr, &ack_buf);
	
	    /* If the request is acceptable to this LNS. */
	    if (status == PPP_FLAG_ACK)
	    {
	        /* Now put the last configuration request received from the
	         * peer into the net buffer.
	         */
	        buf_ptr->data_ptr = buf_ptr->mem_parent_packet + LCP_HEADER_LEN;
	        buf_ptr->data_len = 0;
	        buf_ptr->mem_total_data_len = 0;
	
	        /* Copy the data into the buffer. */
	        MEM_Copy_Data(buf_ptr,
	                      (const CHAR HUGE *)((LCP_FRAME *)
	                      (lcp->options.remote.last_cfg_req))->data,
	                      ((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->len, 0);
	
	        /* Update the data pointer. */
	        buf_ptr->data_ptr -= LCP_HEADER_LEN;
	
	        /* Update data length. */
	        ((LCP_FRAME*)buf_ptr->data_ptr)->len =
	            ((LCP_FRAME *)(lcp->options.remote.last_cfg_req))->len;
	
	        /* Process the last lcp configuration request received by L2TP
	         * LAC from remote client.
	         */
	        status = LCP_Process_Request(buf_ptr, &ack_buf);
	
	        /* If the request is acceptable to this LNS. */
	        if(status == PPP_FLAG_ACK)
	        {
	            /* No need to do LCP, so just set it done. */
	            lcp->state = OPENED;
	
	            /*Now do proxy authentication. */
	            if(lcp->options.remote.flags & PPP_FLAG_PAP)
	            {
	                /* If proxy lcp has been successful then also do proxy
	                 * authentication.
	                 */
	                PAP_Send_Authentication(dev_ptr);
	            }
	
#if(PPP_USE_CHAP == NU_TRUE)
	
	            else if (lcp->options.remote.flags &
	                     (PPP_FLAG_CHAP_MS2 | PPP_FLAG_CHAP_MS1 | PPP_FLAG_CHAP))
	            {
	                /* Do proxy authentication. */
	                CHAP_Respond_To_Challenge(buf_ptr);
	            }
#endif
	
	            /* Set the LCP up event to start using the link. */
	            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_LAYER_UP);
	
	            /* set the status to NU_SUCCESS. */
	            status = NU_SUCCESS;
	        }
	    }
	
	    /* Free the buffer chain of the packet sent. */
	    MEM_Multiple_Buffer_Chain_Free(buf_ptr);
	}
	
	else
	{
		status = NU_NOT_FOUND;
	}
	
    /* Return status. */
    return (status);

} /* PPP_L2TP_LNS_Do_Proxy */
