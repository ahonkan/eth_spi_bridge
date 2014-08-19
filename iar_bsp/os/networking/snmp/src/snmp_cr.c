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
*       snmp_cr.c                                                
*
*   DESCRIPTION
*
*       This file contains definitions of the functions
*       used by the Command Responder.
*
*   DATA STRUCTURES
*
*       Snmp_Session_List
*       Snmp_Events
*
*   FUNCTIONS
*
*       SNMP_AppResponder_Task_Entry
*       SNMP_Get_Request_Ptr
*       SNMP_Request_Ready
*       SNMP_Retrieve_Request
*       SNMP_Process_Request
*
*   DEPENDENCIES
*
*       snmp_cr.h
*       mib.h
*
************************************************************************/

#include "networking/snmp_cr.h"
#include "networking/mib.h"

/* Global Structures */
STATIC SNMP_SESSION_STRUCT  Snmp_Session_List[SNMP_CR_QSIZE];
NU_EVENT_GROUP              Snmp_Events;

extern NU_TASK              Snmp_App_Responder;
extern NU_MEMORY_POOL System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_AppResponder_Task_Entry
*
*   DESCRIPTION
*
*       This function contains the Command Responder
*       Task Entry which includes creating a queue and receiving the
*       incoming requests from that queue.
*
*   INPUTS
*
*       argc            Unused variable
*       *argv           Unused pointer variable
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SNMP_AppResponder_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    SNMP_SESSION_STRUCT *snmp_session;
    SNMP_MESSAGE_STRUCT snmp_response;
    UINT8               *snmp_buff;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Initialize snmp_response, which will be used to respond to the
     * manager.
     */
    if (NU_Allocate_Memory(&System_Memory, (VOID**)&(snmp_buff),
                           (SNMP_BUFSIZE + SNMP_CIPHER_PAD_SIZE),
                           NU_NO_SUSPEND) != NU_SUCCESS)
    {
         /* Suspend Task */
         NU_Suspend_Task(&Snmp_App_Responder);
    }

    /* Will wait for the requests to be added to the queue */
    for(;;)
    {
        /* Reset snmp_buffer in snmp_response. */
        snmp_response.snmp_buffer = snmp_buff;

        /* Get the request from the queue. */
        status = SNMP_Retrieve_Request(&snmp_session);

        if (status != NU_SUCCESS)
            break;

        /* There is no need to process a packet which doesn't contain any
         * object list unless its an SNMPv3 message for which we might have
         * to send an acknowledgement or a report message. */
        if ((snmp_session->snmp_object_list_len > 0) || ((snmp_session->snmp_sm - 1) <= 2)) 
        {
            /* This function is called for processing requests */
            status = SNMP_Process_Request(snmp_session);
            if(status == NU_SUCCESS)
            {
                /* The execution went okay. Encode and Send the result. */
                SNMP_Send_Response(&snmp_response, snmp_session);
            }
        }

        /* Clear element. */
        snmp_session->snmp_in_use = NU_FALSE;

    }   /* End of while loop    */

    /* Deallocate the response buffer. */
    if (NU_Deallocate_Memory(snmp_buff) != NU_SUCCESS)
    {
        NLOG_Error_Log("SNMP: Memory deallocation failed",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Suspend Task */
    NU_Suspend_Task(&Snmp_App_Responder);

    /* we only need this statement to avoid compile warning */
    NU_USER_MODE();

} /* SNMP_AppResponder_Task_Entry */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Get_Request_Ptr
*
*   DESCRIPTION
*
*       Gets a pointer to an empty request location.
*
*   INPUTS
*
*       **request           A pointer to a request which is to be
*                           processed.
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Get_Request_Ptr(SNMP_SESSION_STRUCT **request)
{
    STATUS          status = SNMP_ERROR;
    UINT8           i;

    /* Find an empty location. */
    for(i = 0; i < SNMP_CR_QSIZE; i++)
    {
        /* Is the location empty?. */
        if(Snmp_Session_List[i].snmp_in_use == NU_FALSE)
        {
            /* We have found an empty location, prepare the location and
               give it away. */
            Snmp_Session_List[i].snmp_in_use = SNMP_TAKEN;
            *request = &(Snmp_Session_List[i]);
            status = NU_SUCCESS;

            break;
        }

    }

    return (status);

} /* SNMP_Get_Request_Ptr */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Request_Ready
*
*   DESCRIPTION
*
*       This function adds the incoming request to the queue.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Request_Ready()
{
    STATUS          status;

    status = NU_Set_Events(&Snmp_Events, SNMP_REQUEST_ARRIVED, NU_OR);

    return (status);

} /* SNMP_Request_Ready */


/************************************************************************
*
*   FUNCTION
*
*       SNMP_Retrieve_Request
*
*   DESCRIPTION
*
*       This function retrieves a request from the queue.
*
*   INPUTS
*
*       **snmp_session         Pointer to session information.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Retrieve_Request (SNMP_SESSION_STRUCT **snmp_session)
{
    STATUS          status = SNMP_ERROR;
    UNSIGNED        retrieved_events;
    UINT8           one_loop = NU_FALSE;
    static UINT8    i = 0;

    /* Loop until we finally get a request. */
    while(status != NU_SUCCESS)
    {
        /* Check whether there are request which need to be processed. */
        for(; i < SNMP_CR_QSIZE; i++)
        {
            if(Snmp_Session_List[i].snmp_in_use == SNMP_READY)
            {
                /* A request was found. */
                *snmp_session = &Snmp_Session_List[i];
                status = NU_SUCCESS;
                break;
            }

            if((i == (SNMP_CR_QSIZE - 1)) && (one_loop == NU_FALSE))
            {
                i = 0;
                one_loop = NU_TRUE;
            }
        }

        /* We have not found any request. Wait for a request. */
        if(status != NU_SUCCESS)
        {
            /* Wait for an event to indicate that a request has
             * arrived.
             */
            status = NU_Retrieve_Events(&Snmp_Events,
                                        SNMP_REQUEST_ARRIVED,
                                        NU_OR_CONSUME,
                                        &retrieved_events,
                                        NU_SUSPEND);

            /* Return the error if one occurred. */
            if(status != NU_SUCCESS)
                return (status);
            else
            {
                status = SNMP_ERROR;

                /* Loop through the complete list. */
                i = 0;
            }
        }

    }

    return (status);

} /* SNMP_Retrieve_Request */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Process_Request
*
*   DESCRIPTION
*
*       This function processes the request.
*
*   INPUTS
*
*       *snmp_session           Pointer to SNMP_SESSION_STRUCT, which
*                               contains information about the current
*                               request.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Process_Request(SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS              status = NU_SUCCESS;
    UINT16              i;
    UINT16              error_index;

    /* Ensure that object list is not greater than maximum allocated
	 * size.
	 */
	if ( snmp_session->snmp_object_list_len > AGENT_LIST_SIZE)
	{
		status = SNMP_NO_MEMORY;
	}
	
	else
	{
 	   /* For each object, set the PDU operation to be executed. */
    	for(i = 0; i < snmp_session->snmp_object_list_len; i++)
    	{
        	snmp_session->snmp_object_list[i].Request =
                                    snmp_session->snmp_pdu.Request.Type;
    	}

    	/* Check the PDU type. */
    	switch(snmp_session->snmp_pdu_version)
    	{

#if (INCLUDE_SNMPv1 == NU_TRUE)
    		case SNMP_PDU_V1:

        	/* Call MibRequest to process SNMP V1 Protocol. */
        	snmp_session->snmp_pdu.Request.ErrorStatus =
        		(UINT32)MibRequest(snmp_session->snmp_object_list_len,
                   snmp_session->snmp_object_list,
                   &error_index,
                   snmp_session->snmp_sm,
                   snmp_session->snmp_security_level,
                   snmp_session->snmp_security_name,
                   snmp_session->snmp_context_name);

        	/* Set the PDU type to response. */
        	snmp_session->snmp_pdu.Request.Type = SNMP_PDU_RESPONSE;

        	/* Set the error index to if there is an error. */
        	if(snmp_session->snmp_pdu.Request.ErrorStatus != SNMP_NOERROR)
        	{
            	/* If the error returned is SNMP_ERROR, then convert the error
               	 * to SNMP_GENERROR.
				 */
            	if(snmp_session->snmp_pdu.Request.ErrorStatus == SNMP_ERROR)
            	{
                	snmp_session->snmp_pdu.Request.ErrorStatus =
                                                        SNMP_GENERROR;
            	}

            	else if(snmp_session->snmp_pdu.Request.ErrorStatus == SNMP_WRONGTYPE)
            	{
                	snmp_session->snmp_pdu.Request.ErrorStatus =
                    SNMP_WRONGVALUE;
            	}

            	snmp_session->snmp_pdu.Request.ErrorIndex = error_index;
            	status = NU_SUCCESS;
        	}

        	break;

#endif /* (INCLUDE_SNMPv1 == NU_TRUE) */

#if ( INCLUDE_SNMPv2 || INCLUDE_SNMPv3 )
    		case SNMP_PDU_V2:

        	snmp_session->snmp_pdu.Request.ErrorStatus =
        		(UINT32)MIB_V2_Request(&snmp_session->snmp_object_list_len,
                       snmp_session->snmp_object_list,
                       &error_index,
                       &(snmp_session->snmp_pdu),
                       snmp_session->snmp_sm,
                       snmp_session->snmp_security_level,
                       snmp_session->snmp_security_name,
                       snmp_session->snmp_context_name
                        );

        	/* Set the PDU type to response. */
        	snmp_session->snmp_pdu.Request.Type = SNMP_PDU_RESPONSE;

        	/* Set the error index to if there is an error. */
        	if(snmp_session->snmp_pdu.Request.ErrorStatus != SNMP_NOERROR)
        	{
            	/* If the error returned is SNMP_ERROR, then convert the
				 * error to SNMP_GENERROR.
				 */
            	if(snmp_session->snmp_pdu.Request.ErrorStatus == SNMP_ERROR)
            	{
                	snmp_session->snmp_pdu.Request.ErrorStatus =
                                                            SNMP_GENERROR;
            	}
            	status = NU_SUCCESS;
        	}

        	snmp_session->snmp_pdu.Request.ErrorIndex = error_index;

        	break;
#endif
    		default:

        	status = SNMP_ERROR;
        	break;
    	}
	}

    return (status);

} /* SNMP_Process_Request */

