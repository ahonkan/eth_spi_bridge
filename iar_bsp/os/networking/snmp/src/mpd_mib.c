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
*       mpd_mib.c                                                
*
*   DESCRIPTION
*
*       This file contains implementations of the SNMP-MPD-MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       snmpUnknownSecurityModels
*       snmpInvalidMsgs
*       snmpUnknownPDUHandlers
*
*   DEPENDENCIES
*
*       snmp.h
*       mpd_mib.h
*       snmp_mp.h
*       mib.h
*       snmp_g.h
*
************************************************************************/

#include "networking/snmp.h"
#include "networking/mpd_mib.h"
#include "networking/snmp_mp.h"
#include "networking/mib.h"
#include "networking/snmp_g.h"

#if (INCLUDE_MIB_MPD == NU_TRUE)

extern SNMP_MPD_MIB_STRUCT             Snmp_Mpd_Mib;

/************************************************************************
*
*   FUNCTION
*
*       snmpUnknownSecurityModels
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       snmpUnknownSecurityModels.
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
UINT16 snmpUnknownSecurityModels(snmp_object_t *Obj, UINT16 IdLen,
                                 VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoid compilation warning. */
    UNUSED_PARAMETER(param);

    /* If we got SET request then return error. */
    if (Obj->Request == SNMP_PDU_SET)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    else
    {
        /* If we got an invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        else
        {
            /* Get the value of 'snmpUnknownSecurityModels'. */
            Obj->Syntax.LngUns = Snmp_Mpd_Mib.snmp_unknown_sm;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpUnknownSecurityModels */

/************************************************************************
*
*   FUNCTION
*
*       snmpInvalidMsgs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInvalidMsgs.
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
UINT16 snmpInvalidMsgs(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
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
        /* If we got an invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        else
        {
            /* Get the value of 'snmpInvalidMsgs'. */
            Obj->Syntax.LngUns = Snmp_Mpd_Mib.snmp_invalid_msg;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* snmpInvalidMsgs */

/************************************************************************
*
*   FUNCTION
*
*       snmpUnknownPDUHandlers
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       snmpUnknownPDUHandlers.
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
UINT16 snmpUnknownPDUHandlers(snmp_object_t *Obj, UINT16 IdLen,
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
        /* If we got an invalid request then return error code. */
        if (MibSimple(Obj, IdLen) == NU_FALSE)
        {
            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        else
        {
            /* Get the value of 'snmpUnknownPDUHandlers'. */
            Obj->Syntax.LngUns = Snmp_Mpd_Mib.snmp_unknown_pduhandlers;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
    }

    return (status);

} /* snmpUnknownPDUHandlers */

#endif


