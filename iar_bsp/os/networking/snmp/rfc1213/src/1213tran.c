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
*       1213tran.c                                               
*
* DESCRIPTION
*
*        This file contains those functions specific to processing PDU
*        requests on parameters in the Transmission Group.
*
* DATA STRUCTURES
*
*        SNMP_Commit_Left
*
* FUNCTIONS
*
*        transmission
*
* DEPENDENCIES
*
*        nu_net.h
*        ip_tun.h
*        snmp_api.h
*        mib.h
*        1213tran.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ip_tun.h"
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/1213tran.h"

/* Variable to hold number of commit left operation left. */
UINT32          SNMP_Commit_Left;

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

extern  rfc1213_vars_t          rfc1213_vars;


/************************************************************************
*
* FUNCTION
*
*        transmission
*
* DESCRIPTION
*
*        This function processes the PDU action on the transmission
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 transmission(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_TRUE)
    {
        NU_BLOCK_COPY(Obj->Syntax.BufInt,
                     rfc1213_vars.rfc1213_trans.transNumber,
                     rfc1213_vars.rfc1213_trans.transLen * sizeof(INT32));

        Obj->SyntaxLen = rfc1213_vars.rfc1213_trans.transLen;
    }
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* transmission */


#endif /* INCLUDE_SNMPv1_FULL_MIBII */

