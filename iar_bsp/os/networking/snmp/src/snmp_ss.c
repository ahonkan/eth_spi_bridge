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
*       snmp_ss.c                                                
*
*   DESCRIPTION
*
*       This file contains definitions of all the functions
*       required by the Security Subsystem.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Ss_Init
*       SNMP_Ss_Config
*       SNMP_Secure
*       SNMP_Verify
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       snmp_dis.h
*       snmp_ss.h
*		snmp_mp.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/snmp_dis.h"
#include "networking/snmp_ss.h"
#include "networking/snmp_mp.h"

extern SNMP_SS_STRUCT                  Snmp_Ss_Models[];
extern SNMP_MP_STRUCT                  Snmp_Mp_Models[];
extern snmp_stat_t                     SnmpStat;

#if (INCLUDE_MIB_MPD == NU_TRUE)
extern SNMP_MPD_MIB_STRUCT             Snmp_Mpd_Mib;
#endif

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Ss_Init
*
*   DESCRIPTION
*
*       This function initialized the Security Subsystem each time SNMP
*       starts.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Ss_Init(VOID)
{
    UINT8       i;
    STATUS      status = NU_SUCCESS;

    for(i=0; i<SNMP_SS_MODELS_NO; i++)
    {
        /* Calling the Initializing function of corresponding SNMP Model*/
        if((Snmp_Ss_Models[i].snmp_init_cb != NU_NULL) &&
           ((Snmp_Ss_Models[i].snmp_init_cb()) != NU_SUCCESS))
        {
            status = SNMP_ERROR;
        }
    }

    return (status);

} /* SNMP_Ss_Init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Ss_Config
*
*   DESCRIPTION
*
*       This function (re)configures the Security Subsystem.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Ss_Config(VOID)
{
    UINT8       i;
    STATUS      status = NU_SUCCESS;

    for(i=0; i<SNMP_SS_MODELS_NO; i++)
    {
        /* Calling the configuration function of corresponding SNMP Model.
         */
        if((Snmp_Ss_Models[i].snmp_config_cb != NU_NULL) &&
           ((Snmp_Ss_Models[i].snmp_config_cb()) != NU_SUCCESS))
        {
            status = SNMP_ERROR;
        }
    }

    return (status);

} /* SNMP_Ss_Config */

#if(INCLUDE_SNMPv3 == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Secure
*
*   DESCRIPTION
*
*       This function calls the security function according to the
*       Security Model.
*
*   INPUTS
*
*       snmp_mp                 Message Processing Model.
*       **whole_message         The complete message to be sent.
*       *msg_len                Message Length.
*       max_message_size        Maximum message size for the Manager.
*       snmp_sm                 Security Model.
*       *security_engine_id     The authoritative Engine
*       security_engine_id_len  Length of authoritative Engine ID
*       *security_name          Security Name of the principal.
*       security_level          Required security level.
*       *scoped_pdu             Scoped PDU
*       *security_state_ref     Pointer to the security information.
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Secure(UINT32 snmp_mp, UINT8 **whole_message, UINT32 *msg_len,
                   UINT32 max_message_size, UINT32 snmp_sm,
                   UINT8 *security_engine_id,
                   UINT32 security_engine_id_len, UINT8 *security_name,
                   UINT8 security_level, UINT8 *scoped_pdu,
                   VOID *security_state_ref)
{
    UINT8                   i;
    STATUS                  status = SNMP_ERROR;


    /* Checking whether the version is supported or not */
    for(i=0; i < SNMP_SS_MODELS_NO; i++)
    {
        /* Matching the Security Model from Security Map with the index */
        if(snmp_sm == Snmp_Ss_Models[i].model)
        {
            /* Calling the callback function from Security Model */
            if(Snmp_Ss_Models[i].process_outgoing_cb != NU_NULL)
            {
                status = Snmp_Ss_Models[i].process_outgoing_cb(snmp_mp,
                                whole_message, msg_len, max_message_size,
                                snmp_sm, security_engine_id,
                                security_engine_id_len, security_name,
                                security_level, scoped_pdu,
                                security_state_ref);
            }

            return (status);
        }
    }

#if (INCLUDE_MIB_MPD == NU_TRUE)
    /* No security Model was found. */
    Snmp_Mpd_Mib.snmp_unknown_sm++;
#endif

    return (status);

} /* SNMP_Secure */

#endif

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Verify
*
*   DESCRIPTION
*
*       This function calls the security function for verification
*       according to the Security Model.
*
*   INPUTS
*
*       snmp_mp                 Message Processing Model
*       max_message_size        Maximum Message size for the Manager
*       *security_param         Security Parameters
*       snmp_sm                 Security Model
*       *security_level         Security Level Required
*       **whole_message         The complete message as received on the
*                               wire.
*       *msg_len                The message length
*       *security_engine_id     ID of the authoritative engine
*       *security_engine_id_len ID Length for the authoritative engine
*       *security_name          Security Name of the principal
*       *max_response_pdu       Maximum size of the response scoped PDU.
*       **security_state_ref    Security information for the session
*       *error_indication       Status information if any error was
*                               encountered.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Verify(UINT32 snmp_mp, UINT32 max_message_size,
                   UINT8 *security_param, UINT32 snmp_sm,
                   UINT8 *security_level, UINT8 **whole_message,
                   UINT32 *msg_len, UINT8 *security_engine_id,
                   UINT32 *security_engine_id_len, UINT8 *security_name,
                   UINT32 *max_response_pdu, VOID **security_state_ref,
                   SNMP_ERROR_STRUCT *error_indication)
{
    UINT8                   i;
    STATUS                  status = SNMP_ERROR;

    /* Check whether Message Processing Model is supported and whether
     * message processing supports the security model.
     */
    for (i = 0; i < SNMP_MP_MODELS_NO; i++)
    {
        /* If message processing model is supported. */
        if (snmp_mp == Snmp_Mp_Models[i].snmp_mp_model)
        {
            /* If security model is not supported by Message Processing
             * model.
             */
            if (snmp_sm != Snmp_Mp_Models[i].snmp_sm)
            {
                if ((!snmp_sm) || (snmp_sm > 0x7FFFFFFF))
                {
                    SnmpStat.InASNParseErrs++;
                }
                else
                {
#if (INCLUDE_MIB_MPD == NU_TRUE)
                    /* Increment the counter for Unknown security models. */
                    Snmp_Mpd_Mib.snmp_unknown_sm++;
#endif
                }

                /* Return error code. */
                return (ASN1_ERR_DEC_BADVALUE);
            }
            /* Set status to success code to represent that message
             * processing model and security model combination is
             * consistent.
             */
            status = NU_SUCCESS;

            /* Break through the loop. */
            break;
        }
    }

    /* If Message processing model and security model combination is not
     * consistent then return error code.
     */
    if (status != NU_SUCCESS)
    {
        /* Return error code. */
        return (status);
    }
    else
    {
        /* Set status to error code. */
        status = SNMP_ERROR;
    }

    /* Checking whether the version is supported or not */
    for(i=0; i < SNMP_SS_MODELS_NO; i++)
    {
        /* Matching the Security Model from Security Map with the index */
        if(snmp_sm == Snmp_Ss_Models[i].model)
        {
            /* Calling the callback function from Security Model */
            if(Snmp_Ss_Models[i].process_incoming_cb != NU_NULL)
            {
                status = Snmp_Ss_Models[i].process_incoming_cb(snmp_mp,
                            max_message_size, security_param, snmp_sm,
                            security_level, whole_message, msg_len,
                            security_engine_id, security_engine_id_len,
                            security_name, max_response_pdu,
                            security_state_ref, error_indication);
            }

            return (status);
        }
    }

#if (INCLUDE_MIB_MPD == NU_TRUE)
    /* No security Model was found. */
    Snmp_Mpd_Mib.snmp_unknown_sm++;
#endif

    return (status);

} /* SNMP_Verify */

