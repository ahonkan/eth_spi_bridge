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
*       prot.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions specific to manipulating the
*       parameters of a prot_pkt_t packet.
*
*   DATA STRUCTURES
*
*       protsnmpString
*       ProtPtr
*
*   FUNCTIONS
*
*       ProtGetField
*       ProtFree
*       ProtExit
*       ProtsnmpPrint
*
*   DEPENDENCIES
*
*       externs.h
*       snmp.h
*       agent.h
*
************************************************************************/

#include "networking/externs.h"
#include "networking/snmp.h"
#include "networking/agent.h"

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

CHAR *protocolString = NU_NULL;

prot_ptr_t ProtPtr[] =
{
    {"UNKNOWN"},
    {"SNMP", NU_NULL, NU_NULL, ProtsnmpPrint, protsnmpString, 6}
};

CHAR *protsnmpString[] =
{
    "TYPE",
    "INTERFACE",
    "ID",
    "SIZE",
    "LEN",
    "TIME"
};

extern NU_MEMORY_POOL System_Memory;

#if (INCLUDE_MIB_RMON1==NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       ProtGetField
*
*   DESCRIPTION
*
*       This function extracts the object fields from a packet.
*
*   INPUTS
*
*       *Pkt                    The packet from which to extract the
*                               object fields.
*       *Obj                    The object into which to store the
*                               fields.
*
*   OUTPUTS
*
*       NU_TRUE                 The object fields were successfully
*                               extracted.
*       NU_FALSE                The object fields were not extracted.
*
*************************************************************************/
BOOL ProtGetField(prot_pkt_t *Pkt, prot_obj_t *Obj)
{
    BOOL    success = NU_TRUE;

    if ( (Obj->Level == 0) && (Obj->Id[1] == PROT_TYPE) )
    {
        UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
        Obj->Type = SNMP_GAUGE;
        Obj->Syntax.LngUns = (UINT32) Pkt->ChildProt;
    }

    else if (Obj->Id[0] == PROT_PKTSNMP)
    {
        switch (Obj->Id[1])
        {
            case 1:
                Obj->Type = SNMP_INTEGER;
                Obj->Syntax.LngInt = (INT32) Pkt->Frame->snmp.Type;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 2:
                Obj->Type = SNMP_INTEGER;
                Obj->Syntax.LngInt = (INT32) Pkt->Frame->snmp.IfIndex;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 3:
                Obj->Type = SNMP_INTEGER;
                Obj->Syntax.LngInt = (INT32)Pkt->Frame->snmp.ID;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 4:
                Obj->Type = SNMP_GAUGE;
                Obj->Syntax.LngUns = (UINT32) Pkt->Frame->snmp.Size;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 5:
                Obj->Type = SNMP_GAUGE;
                Obj->Syntax.LngUns = (UINT32) Pkt->Frame->snmp.Len;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 6:
                Obj->Type = SNMP_TIMETICKS;
                Obj->Syntax.LngUns = Pkt->Frame->snmp.Time;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 7:
                Obj->Type = SNMP_NULL;
                Obj->Syntax.Ptr = Pkt->Frame->snmp.Data;
                Obj->SyntaxLen = Pkt->Frame->snmp.Len;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            case 8:
                Obj->Type = SNMP_INTEGER;
                Obj->Syntax.LngInt = (INT32) Pkt->Frame->snmp.Status;
                UTL_Zero(&Obj->Id[2], SNMP_SIZE_OBJECTID - 4);
                break;
            default:
                success = NU_FALSE;
                break;
        }
    }

    else if (Obj->Level > 0 && Pkt->ChildProt != PROT_PKTUNKNOWN)
    {
        if (Pkt->Child == NU_NULL)
        {
            if (NU_Allocate_Memory(&System_Memory, (VOID**)&(Pkt->Child),
                                   sizeof(prot_pkt_t),
                                   NU_NO_SUSPEND) == NU_SUCCESS)
            {
                Pkt->Child = TLS_Normalize_Ptr(Pkt->Child);

                UTL_Zero(Pkt->Child, sizeof(prot_pkt_t));

                Pkt->Child->Ptr = Pkt->Frame->snmp.Data;
                Pkt->Child->Child = NU_NULL;
                Pkt->Child->DataLen = Pkt->Frame->snmp.Len;

                if ((ProtPtr[Pkt->ChildProt].Header(Pkt->Child)) ==
                                                                NU_FALSE)
                {
                    NU_Deallocate_Memory((VOID*)(Pkt->Child));
                    Pkt->Child = NU_NULL;
                    success = NU_FALSE;
                }
            }
            else
                success = NU_FALSE;
        }

        if (success == NU_TRUE)
        {
            Obj->Level--;
            success = ProtPtr[Pkt->ChildProt].Field (Pkt->Child, Obj);
        }
    }

    return (success);

} /* ProtGetField */

#endif

/************************************************************************
*
*   FUNCTION
*
*       ProtFree
*
*   DESCRIPTION
*
*       This function frees up a packet from the list of packets.
*
*   INPUTS
*
*       *Pkt                    A pointer to the packet to delete.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ProtFree(prot_pkt_t *Pkt)
{
    if (Pkt->Child != NU_NULL)
    {
        ProtFree (Pkt->Child);
        NU_Deallocate_Memory((VOID*)(Pkt->Child));
        Pkt->Child = NU_NULL;
    }

    NU_Deallocate_Memory((VOID*)(Pkt->Frame));
    Pkt->Frame = NU_NULL;

} /* ProtFree */

/************************************************************************
*
*   FUNCTION
*
*       ProtExit
*
*   DESCRIPTION
*
*       This function deletes the global variable protocolString.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ProtExit(VOID)
{
    if (protocolString != NU_NULL)
        NU_Deallocate_Memory((VOID*)protocolString);

} /* ProtExit */

/************************************************************************
*
*   FUNCTION
*
*       ProtsnmpPrint
*
*   DESCRIPTION
*
*       This function places the parameters of the object into the global
*       variable protocolString.
*
*   INPUTS
*
*       *Obj                    A pointer to the object from which to
*                               extract the parameters.
*       **StrPtr                A pointer to a pointer to the new string.
*
*   OUTPUTS
*
*       NU_TRUE                 The parameters were extracted.
*       NU_FALSE                The parameters were not extracted.

*************************************************************************/
BOOL ProtsnmpPrint(prot_obj_t *Obj, UINT8 **StrPtr)
{
    BOOL    success = NU_TRUE;
    CHAR    temp[16];

    UTL_Zero(protocolString, SNMP_SIZE_BUFCHR);

    switch (Obj->Id[1])
    {
        case 1:                                 /* type */
        case 2:                                 /* IfIndex */
        case 4:                                 /* size */
        case 5:                                 /* len */

            strcat(protocolString,
                  (CHAR*)(NU_ULTOA(Obj->Syntax.LngUns, temp, 10)));
            *StrPtr = (UINT8 *)protocolString;
            break;

        case 3:                                 /* ID */

            strcat(protocolString,
                   (CHAR*)(NU_ITOA((INT)(Obj->Syntax.LngInt), temp, 10)));
            *StrPtr = (UINT8 *)protocolString;
            break;

        case 6:                                 /* time */

            strcat(protocolString, (CHAR*)(NU_ULTOA((Obj->Syntax.LngUns
                                        / 1000000L) % 1000, temp, 10)));
            strcat(protocolString, ":");
            strcat(protocolString, (CHAR*)(NU_ULTOA((Obj->Syntax.LngUns
                                        / 1000L) % 1000, temp, 10)));
            strcat(protocolString, ":");
            strcat(protocolString, (CHAR*)(NU_ULTOA((Obj->Syntax.LngUns
                                                 % 1000), temp, 10)));

            *StrPtr = (UINT8 *)protocolString;
            break;

        default:

            success = NU_FALSE;
    }

    return (success);

} /* ProtsnmpPrint */

#endif


