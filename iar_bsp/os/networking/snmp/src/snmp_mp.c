/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_mp.c                                                
*
*   DESCRIPTION
*
*       This file contains definitions of all the functions
*       required by the SNMP Message Processing Subsystem.
*
*   DATA STRUCTURES
*
*       SnmpCnv
*
*   FUNCTIONS
*
*       SNMP_Decode
*       SNMP_Notify
*       SNMP_Encode
*       SNMP_Mp_Init
*       SNMP_Mp_Config
*       SNMP_Init_ReqList
*       SNMP_Add_ReqList
*       SNMP_Remove_ReqList
*       SNMP_Syn2TagCls
*       SNMP_TagCls2Syn
*
*   DEPENDENCIES
*
*       snmp_cfg.h
*       snmp_dis.h
*       snmp_mp.h
*
************************************************************************/

#include "networking/snmp_cfg.h"
#include "networking/snmp_dis.h"
#include "networking/snmp_mp.h"

NU_PROTECT              Snmp_Protect_ReqList;
extern snmp_stat_t      SnmpStat;
extern SNMP_MP_STRUCT   Snmp_Mp_Models[SNMP_MP_MODELS_NO];
extern SNMP_MPD_MIB_STRUCT  Snmp_Mpd_Mib;

/* Class and Tag Mapping to the Syntax. */
static SNMP_CNV_STRUCT SnmpCnv[] =
{
    {ASN1_UNI, ASN1_NUL, SNMP_NULL},
    {ASN1_CTX, ASN1_NSO, SNMP_NOSUCHOBJECT},
    {ASN1_CTX, ASN1_NSI, SNMP_NOSUCHINSTANCE},
    {ASN1_CTX, ASN1_EMV, SNMP_ENDOFMIBVIEW},
    {ASN1_UNI, ASN1_INT, SNMP_INTEGER},
    {ASN1_UNI, ASN1_OTS, SNMP_OCTETSTR},
    {ASN1_UNI, ASN1_OTS, SNMP_DISPLAYSTR},
    {ASN1_UNI, ASN1_OJI, SNMP_OBJECTID},
    {ASN1_APL, SNMP_IPA, SNMP_IPADDR},
    {ASN1_APL, SNMP_CNT, SNMP_COUNTER},
    {ASN1_APL, SNMP_GGE, SNMP_GAUGE},
    {ASN1_APL, SNMP_TIT, SNMP_TIMETICKS},
    {ASN1_APL, SNMP_OPQ, SNMP_OPAQUE},
    {ASN1_APL, SNMP_C64, SNMP_COUNTER64},
    {0, 0, -1}
};

extern NU_MEMORY_POOL System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Decode
*
*   DESCRIPTION
*
*       This function calls the decoding function according to the Message
*       processing model.
*
*   INPUTS
*
*       *snmp_request         The request message.
*       *snmp_session         The session structure which will contain
*                             information decode from the message.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Decode(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    UINT8                   i;
    STATUS                  status = SNMP_ERROR;

    /* Checking whether the version is supported or not */
    for(i=0; i<SNMP_MP_MODELS_NO; i++)
    {
        /* Matching the MP Model from MP Map with the index */
        if(snmp_session->snmp_mp == Snmp_Mp_Models[i].snmp_mp_model)
        {
            /* Calling the callback function from MP Model */
            if(Snmp_Mp_Models[i].snmp_dec_request_cb != NU_NULL)
            {
                status = Snmp_Mp_Models[i].snmp_dec_request_cb(
                                            snmp_request,snmp_session);

                if ((status != SNMP_NOERROR) && (status != ASN1_ERR_DEC_BADVALUE))
                {
                    Snmp_Mpd_Mib.snmp_invalid_msg++;
                }
            }

            return (status);
        }
    }

    if ((i == SNMP_MP_MODELS_NO) && (status == SNMP_ERROR))
    {
        if (snmp_session->snmp_mp > Snmp_Mp_Models[i-1].snmp_mp_model) 
        {
            SnmpStat.InBadVersions++;

            if (snmp_session->snmp_mp > 0x7FFFFFFF)
            {
                SnmpStat.InASNParseErrs++;
            }

            return (status);
        }
    }

    SnmpStat.InBadVersions++;
    return (status);

} /* SNMP_Decode */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Notify
*
*   DESCRIPTION
*
*       This function calls the encoding function according to the Message
*       processing model.
*
*   INPUTS
*
*       *snmp_notification  Pointer to SNMP_MESSAGE_STRUCT which holds the
*                           notification.
*       *snmp_session       Pointer to SNMP_SESSION_STRUCT which holds the
*                           values to be encoded for the notification.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Notify(SNMP_MESSAGE_STRUCT *snmp_notification,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    UINT8                   i;
    STATUS                  status = SNMP_ERROR;

    /* Checking whether the version is supported or not */
    for(i=0; i<SNMP_MP_MODELS_NO; i++)
    {
        /* Matching the MP Model from MP Map with the index */
        if(snmp_session->snmp_mp == Snmp_Mp_Models[i].snmp_mp_model)
        {
            /* Calling the callback function from MP Model */
            if(Snmp_Mp_Models[i].snmp_enc_notify_callback != NU_NULL)
                status = Snmp_Mp_Models[i].snmp_enc_notify_callback(
                                        snmp_notification, snmp_session);

            return (status);
        }
    }

    return (status);

} /* SNMP_Notify */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Encode
*
*   DESCRIPTION
*
*       This function calls the encoding function according to the Message
*       processing model.
*
*   INPUTS
*
*       *snmp_response      Pointer to SNMP_MESSAGE_STRUCT which holds the
*                           response message.
*       *snmp_session       Pointer to SNMP_SESSION_STRUCT which holds the
*                           values decoded from the SNMP Message and then
*                           processed by the command responder
*                           application.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Encode(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    UINT8                   i;
    STATUS                  status = SNMP_ERROR;

    /* Checking whether the version is supported or not */
    for(i=0; i<SNMP_MP_MODELS_NO; i++)
    {
        /* Matching the MP Model from MP Map with the index */
        if(snmp_session->snmp_mp == Snmp_Mp_Models[i].snmp_mp_model)
        {
            /* Calling the callback function from MP Model */
            if(Snmp_Mp_Models[i].snmp_enc_response_cb != NU_NULL)
                status = Snmp_Mp_Models[i].snmp_enc_response_cb(
                                            snmp_response, snmp_session);

            return (status);
        }
    }

    return (status);

} /* SNMP_Encode */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Mp_Init
*
*   DESCRIPTION
*
*       This function initializes the message processing model. This is
*       done every time SNMP starts.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_INVALID_PARAMETER
*       SNMP_NO_MEMORY
*
*************************************************************************/
STATUS SNMP_Mp_Init(VOID)
{
    UINT8       i;
    STATUS      status = NU_SUCCESS;

    for(i=0; i<SNMP_MP_MODELS_NO; i++)
    {
        /* Calling the Initializing function of corresponding SNMP Model*/
        if(Snmp_Mp_Models[i].snmp_init_cb != NU_NULL)  
        {
            status =Snmp_Mp_Models[i].snmp_init_cb();
        }
        else
        {
            return SNMP_BAD_PARAMETER;
        }
    }

    return (status);

} /* SNMP_Mp_Init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Mp_Config
*
*   DESCRIPTION
*
*       This function configures the message processing models. This is
*       executed when SNMP needs to be (re)configured.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Mp_Config(VOID)
{
    UINT8       i;
    STATUS      status = NU_SUCCESS;

    for(i=0; i<SNMP_MP_MODELS_NO; i++)
    {
        /* Calling the Initializing function of corresponding SNMP Model*/
        if((Snmp_Mp_Models[i].snmp_config_cb != NU_NULL) &&
           ((Snmp_Mp_Models[i].snmp_config_cb()) != NU_SUCCESS))
        {
            status = SNMP_ERROR;
        }
    }

    return (status);

} /* SNMP_Mp_Config */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Init_ReqList
*
*   DESCRIPTION
*
*       This function initializes an SNMP_REQLIST_STRUCT, and allocates
*       list_size bytes to it. SNMP_REQLIST_STRUCT is a list of requests
*       which are currently being processed.
*
*   INPUTS
*
*       req_list             Pointer to SNMP_REQLIST_STRUCT.
*       list_size            The size to be allocated to the list.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_NO_MEMORY
*
*************************************************************************/
STATUS SNMP_Init_ReqList(SNMP_REQLIST_STRUCT *req_list, UINT32 list_size)
{
    STATUS                  status;
    SNMP_FREEBUFF_STRUCT    *free_buff;

    /* Initially, there is only one free memory block in the list. Create
     * that block.
     */
    if((status = NU_Allocate_Memory(&System_Memory, (VOID**)&free_buff,
                                list_size, NU_NO_SUSPEND))== NU_SUCCESS)
    {
        NU_Protect(&Snmp_Protect_ReqList);

        free_buff = TLS_Normalize_Ptr(free_buff);

        /* There are no buffers in the list. */
        req_list->snmp_buff_list.buff_list_head = NU_NULL;
        req_list->snmp_buff_list.buff_list_tail = NU_NULL;
        req_list->snmp_free_list.free_list_head = NU_NULL;
        req_list->snmp_free_list.free_list_tail = NU_NULL;

        /* The memory currently available is the complete memory allocated
         * minus the sizeof SNMP_FREEBUFF_STRUCT.
         */
        free_buff->size = list_size - sizeof(SNMP_FREEBUFF_STRUCT);

        /* Enqueue in to the free list. */
        DLL_Enqueue(&(req_list->snmp_free_list), free_buff);

        NU_Unprotect();
    }
    else
    {
        return SNMP_NO_MEMORY;
    }
    return (status);

} /* SNMP_Init_ReqList */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Add_ReqList
*
*   DESCRIPTION
*
*       This function copies the SNMP_MESSAGE_STRUCT, to the
*       SNMP_REQLIST_STRUCT.
*
*   INPUTS
*
*       request_list          Pointer to SNMP_REQLIST_STRUCT.
*       snmp_request          Pointer to SNMP_MESSAGE_STRUCT.
*
*   OUTPUTS
*
*       Pointer to where SNMP_MESSAGE_STRUCT has been placed.
*       NU_NULL, if an error occurred.
*
*************************************************************************/
SNMP_MESSAGE_STRUCT *SNMP_Add_ReqList(SNMP_REQLIST_STRUCT *request_list,
                                  const SNMP_MESSAGE_STRUCT *snmp_request)
{
    UINT32                  reqSize;
    SNMP_FREEBUFF_STRUCT    *freeBuff;
    SNMP_FREEBUFF_STRUCT    *smallestBuff = NU_NULL;
    SNMP_BUFF_STRUCT        *theBuffer;
    UINT32                  buffer_len = snmp_request->snmp_buffer_len;

    NU_Protect(&Snmp_Protect_ReqList);

    /* Make sure that length of incoming packet is multiple of 4. To
       ensure word-alignment. */
    if(buffer_len & 0x3)
    {
        /*if it is not multiple of 4 then make it*/
        buffer_len = (((UINT32)buffer_len >> 2) << 2) + 4;
    }

    /* Calculate the request size. This is the size of SNMP_MESSAGE_STRUCT
     * plus the size of SNMP_BUFF_STRUCT and the size of the buffer.
     */
    reqSize = sizeof(SNMP_BUFF_STRUCT) + sizeof(SNMP_MESSAGE_STRUCT) +
              buffer_len;

    /* Find the smallest possible free memory in which snmp_request can
     * fit in.
     */
    freeBuff = request_list->snmp_free_list.free_list_head;

    while(freeBuff != NU_NULL)
    {
        /* Check if this buffer has a memory greater than reqSize and less
         * than smallestBuff.
         */
        if((freeBuff->size >= reqSize) &&
           ( (smallestBuff == NU_NULL) ||
             (freeBuff->size < smallestBuff->size) ))

            /* If so, then this is the new smallestBuff. */
            smallestBuff = freeBuff;

        /* Go to the next free memory. */
        freeBuff = freeBuff->snmp_next_link;
    }

    /* If we have found a suitable place to put the request, then place
     * the request there.
     */
    if(smallestBuff != NU_NULL)
    {
        /* If there is memory left in smallestBuff then update the
         * smallestBuff.
         */
        if(smallestBuff->size > reqSize)
        {
            /* Set theBuffer to use the end of the free location. */
            theBuffer = (SNMP_BUFF_STRUCT *)(((UINT8 *)(smallestBuff + 1))
                        + smallestBuff->size - reqSize);

            /* Update the free memory that is left. */
            smallestBuff->size -= reqSize;

            /* Set theBuffer Size. */
            theBuffer->size = reqSize;
        }
        else
        {
            /* Otherwise, remove smallestBuff from the list of Free
             * Buffers since the complete memory has been taken.
             */
            DLL_Remove(&(request_list->snmp_free_list), smallestBuff);

            /* Assign theBuffer to the smallest Buff. */
            theBuffer = (SNMP_BUFF_STRUCT *)smallestBuff;

            /* Set theBuffer Size. */
            theBuffer->size = smallestBuff->size +
                              sizeof(SNMP_FREEBUFF_STRUCT);
        }

        /* Set the snmp_request pointer in the buffer. */
        theBuffer->snmp_request = (SNMP_MESSAGE_STRUCT *)(theBuffer + 1);

        /* Copy the SNMP buffer. */
        theBuffer->snmp_request->snmp_buffer =
                                (UINT8 *)(theBuffer->snmp_request + 1);

        NU_BLOCK_COPY(theBuffer->snmp_request->snmp_buffer,
                      snmp_request->snmp_buffer,
                      (unsigned int)snmp_request->snmp_buffer_len);

        theBuffer->snmp_request->snmp_buffer_len =
                                            snmp_request->snmp_buffer_len;

        /* Copy the transport domain and address. */
        NU_BLOCK_COPY(&theBuffer->snmp_request->snmp_transport_address,
                      &snmp_request->snmp_transport_address,
                      sizeof(struct addr_struct));

        theBuffer->snmp_request->snmp_transport_domain =
                                    snmp_request->snmp_transport_domain;

        /* Add the new request to the Buffer list. */
        DLL_Enqueue(&(request_list->snmp_buff_list), theBuffer);

        NU_Unprotect();
        
        /* Return the request pointer. */
        return (theBuffer->snmp_request);
    }

    NU_Unprotect();

    /* Could not find a free location to place the request. */
    return (NU_NULL);

} /* SNMP_Add_ReqList */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Remove_ReqList
*
*   DESCRIPTION
*
*       This function removes the SNMP_MESSAGE_STRUCT from the
*       SNMP_REQLIST_STRUCT.
*
*   INPUTS
*
*       request_list          Pointer to SNMP_REQLIST_STRUCT.
*       snmp_request          Pointer to SNMP_MESSAGE_STRUCT.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Remove_ReqList(SNMP_REQLIST_STRUCT *request_list,
                           const SNMP_MESSAGE_STRUCT *snmp_request)
{
    SNMP_FREEBUFF_STRUCT       *previous_free = NU_NULL;
    SNMP_FREEBUFF_STRUCT       *next_free = NU_NULL;
    SNMP_FREEBUFF_STRUCT       *freeBuff;
    SNMP_BUFF_STRUCT           *theBuffer;

    NU_Protect(&Snmp_Protect_ReqList);
    
    /* Find the Buffer with the given request. */
    theBuffer = request_list->snmp_buff_list.buff_list_head;

    while(theBuffer != NU_NULL)
    {
        if(theBuffer->snmp_request == snmp_request)
            break;

        /* Go to the next buffer. */
        theBuffer = theBuffer->snmp_next_link;
    }

    /* If we find such a buffer. */
    if(theBuffer != NU_NULL)
    {
        /* Check if the memory being released, has free buffers on either
         * side.
         */
        freeBuff = request_list->snmp_free_list.free_list_head;

        while(freeBuff != NU_NULL)
        {
            /* Does the start of the buffer coincide with the end of a
             * free memory.
             */
            if(theBuffer == (SNMP_BUFF_STRUCT *)(((UINT8 *)(freeBuff + 1))
                            + freeBuff->size))
            {
                previous_free = freeBuff;
            }

            /* Does the end of the buffer coincide with the start of a
             * free memory.
             */
            else if((SNMP_FREEBUFF_STRUCT *)(((UINT8 *)theBuffer)
                                        + theBuffer->size) == freeBuff)
            {
                next_free = freeBuff;
            }

            /* Go to the next buffer. */
            freeBuff = freeBuff->snmp_next_link;
        }

        if(previous_free != NU_NULL && next_free != NU_NULL)
        {
            /* There are three consecutive free buffer. We need to merge
             * them.
             */

            /* Remove the next_free buffer, we do not need this anymore.
             */
            DLL_Remove(&request_list->snmp_free_list, next_free);

            /* Update the size of the previous buffer. */
            previous_free->size += theBuffer->size +
                        sizeof(SNMP_FREEBUFF_STRUCT) + next_free->size;

            /* Remove from snmp_buff_list. */
            DLL_Remove(&request_list->snmp_buff_list, theBuffer);
        }
        else if(previous_free != NU_NULL)
        {
            /* Merge this buffer with the previous one. */
            previous_free->size += theBuffer->size;

            /* Remove from snmp_buff_list. */
            DLL_Remove(&request_list->snmp_buff_list, theBuffer);
        }
        else if(next_free != NU_NULL)
        {
            /* Remove the next_free buffer. */
            DLL_Remove(&request_list->snmp_free_list, next_free);

            /* The new free buffer will start from theBuffer. */
            previous_free = (SNMP_FREEBUFF_STRUCT *)theBuffer;

            /* Update the size. */
            previous_free->size = next_free->size + theBuffer->size;

            /* Remove from snmp_buff_list. */
            DLL_Remove(&request_list->snmp_buff_list, theBuffer);

            /* Add the new free buffer to the free list. */
            DLL_Enqueue(&request_list->snmp_free_list, previous_free);
        }
        else
        {
            /* There are no adjacent free buffers around. Create a new
             * free buffer.
             */
            freeBuff = (SNMP_FREEBUFF_STRUCT *)theBuffer;

            /* Remove theBuffer from snmp_buff_list.*/
            DLL_Remove(&request_list->snmp_buff_list, theBuffer);

            freeBuff->size = theBuffer->size -
                                            sizeof(SNMP_FREEBUFF_STRUCT);

            /* Add to the List. */
            DLL_Enqueue(&request_list->snmp_free_list, freeBuff);
        }

        NU_Unprotect();

        return (NU_SUCCESS);
    }

    NU_Unprotect();

    return (SNMP_ERROR);

} /* SNMP_Remove_ReqList */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Syn2TagCls
*
*   DESCRIPTION
*
*       This function finds the entry in the global snmp_cnv_t data
*       structure SnmpCnv with the syntax of syn and copies the Tag
*       and Class values of that entry into the *tag and *cls variables
*       passed in to the function.
*
*   INPUTS
*
*       *tag        A pointer to the tag.
*       *cls        A pointer to the class.
*       syn         The syntax.
*
*   OUTPUTS
*
*       NU_TRUE     There is a corresponding entry in SnmpCnv.
*       NU_FALSE    There is not a corresponding entry in SnmpCnv.
*
*************************************************************************/
BOOLEAN SNMP_Syn2TagCls (UINT32 *tag, UINT32 *cls, UINT32 syn)
{
    BOOLEAN             success = NU_FALSE;
    SNMP_CNV_STRUCT  *cnv;

    cnv = SnmpCnv;

    while (cnv->snmp_syntax != -1)
    {
        if (cnv->snmp_syntax == (INT32)(syn))
        {
            *tag = cnv->snmp_tag;
            *cls = cnv->snmp_class;
            success = NU_TRUE;
            break;
        }
        cnv++;
    }

    if (success == NU_FALSE)
    {
        SnmpStat.OutBadValues++;
    }

    return (success);

} /* SNMP_Syn2TagCls */
/************************************************************************
*
*   FUNCTION
*
*       SNMP_TagCls2Syn
*
*   DESCRIPTION
*
*       This function finds the entry in the global snmp_cnv_t data
*       structure SnmpCnv with the tag of tag and the class of cls and
*       copies the syntax into syn.
*
*   INPUTS
*
*       tag         The tag value of the entry to find.
*       cls         The class value of the entry to find.
*       *syn        A pointer to the buffer into which to store the
*                   syntax value of the found entry.
*
*   OUTPUTS
*
*       NU_TRUE     There is a corresponding entry in SnmpCnv.
*       NU_FALSE    There is not a corresponding entry in SnmpCnv.
*
*************************************************************************/
BOOLEAN SNMP_TagCls2Syn (UINT32 tag, UINT32 cls, UINT32 *syn)
{
    BOOLEAN                success = NU_FALSE;
    SNMP_CNV_STRUCT     *cnv;

    cnv = SnmpCnv;

    while (cnv->snmp_syntax != -1)
    {
        if (cnv->snmp_tag == tag && cnv->snmp_class == cls)
        {
            *syn = (UINT32)cnv->snmp_syntax;
            success = NU_TRUE;
            break;
        }
        cnv++;
    }

    if (success == NU_FALSE)
    {
        SnmpStat.OutBadValues++;
    }

    return (success);

} /* SNMP_TagCls2Syn */



