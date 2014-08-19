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
*       engine_mib.c                                             
*
*   DESCRIPTION
*
*       This file contains implementation of the SNMP Engine MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       snmpEngineID
*       snmpEngineBoots
*       snmpEngineTime
*       snmpEngineMaxMessageSize
*
*   DEPENDENCIES
*
*       agent.h
*       mib.h
*       snmp_cfg.h
*       engine_mib.h
*       snmp_g.h
*
*************************************************************************/

#include "networking/agent.h"
#include "networking/mib.h"
#include "networking/engine_mib.h"
#include "networking/snmp_g.h"

#if (INCLUDE_MIB_SNMP_ENGINE == NU_TRUE)

extern SNMP_ENGINE_STRUCT      Snmp_Engine;

/************************************************************************
*
*   FUNCTION
*
*       snmpEngineID
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpEngineID.
*
*   INPUTS
*
*       *obj                    A pointer to the object.
*       idlen                   The length of the object ID.
*       *param                  Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The request was processed successfully.
*
*************************************************************************/
UINT16 snmpEngineID(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we got SET request then return error code. */
    if (Obj->Request == SNMP_PDU_SET)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }
    else
    {
        /* If we have invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }
        else
        {
            /* Get the value of 'snmpEngineID'. */
            NU_BLOCK_COPY(Obj->Syntax.BufChr, Snmp_Engine.snmp_engine_id,
                          (unsigned int)Snmp_Engine.snmp_engine_id_len);

            /* Update the length of 'snmpEngineID'. */
            Obj->SyntaxLen = Snmp_Engine.snmp_engine_id_len;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpEngineID */

/************************************************************************
*
*   FUNCTION
*
*       snmpEngineBoots
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpEngineBoots.
*
*   INPUTS
*
*       *obj                    A pointer to the object.
*       idlen                   The length of the object ID.
*       *param                  Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The request was processed successfully.
*
*************************************************************************/
UINT16 snmpEngineBoots(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we got SET request then return error code. */
    if (Obj->Request == SNMP_PDU_SET)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    else
    {
        /* If we have invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }
        else
        {
            /* Get the value of 'snmpEngineBoots'. */
            Obj->Syntax.LngUns = Snmp_Engine.snmp_engine_boots;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpEngineBoots */

/************************************************************************
*
*   FUNCTION
*
*       snmpEngineTime
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpEngineTime.
*
*   INPUTS
*
*       *obj                    A pointer to the object.
*       idlen                   The length of the object ID.
*       *param                  Unused parameter.
*
*   OUTPUTS
*
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The request was processed successfully.
*
*************************************************************************/
UINT16 snmpEngineTime(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we got SET request return error code. */
    if (Obj->Request == SNMP_PDU_SET)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    else
    {
        /* If we got invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        else
        {
            /* Get the value of 'snmpEngineTime'. */
            if (SNMP_Engine_Time((UINT32*)&(Obj->Syntax.LngUns)) !=
                                                               NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to get engine time",
                                NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpEngineTime */

/************************************************************************
*
*   FUNCTION
*
*       snmpEngineMaxMessageSize
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       snmpEngineMaxMessageSize.
*
*   INPUTS
*
*       *obj                    A pointer to the object.
*       idlen                   The length of the object ID.
*       *param                  Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The request was processed successfully.
*
*************************************************************************/
UINT16 snmpEngineMaxMessageSize(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we got SET request then return error code. */
    if (Obj->Request == SNMP_PDU_SET)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    else
    {
        /* If we have invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        else
        {
            /* Get the value of 'snmpEngineMaxMessageSize'. */
            Obj->Syntax.LngUns = Snmp_Engine.snmp_max_message_size;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpEngineMaxMessageSize */

#endif
