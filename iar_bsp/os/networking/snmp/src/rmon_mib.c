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
*       rmon_mib.c                                               
*
*   DESCRIPTION
*
*       This file contains implementations of all the functions
*       required by the RMON.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RMON_MibInsert
*       RMON_MibRequest
*       RMON_Request
*       RMON_MibRmon
*       RMON_MibRemove
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       mib.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/mib.h"

extern mib_root_t   MibRoot;
extern snmp_stat_t  SnmpStat;
extern NU_MEMORY_POOL System_Memory;

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       RMON_MibInsert
*
*   DESCRIPTION
*
*       This function inserts a node into the specified list.
*
*   INPUTS
*
*       *Obj            A pointer to the object to insert.
*       **local         A pointer to a pointer to the list into which
*                       to insert the node.
*       IdLen           The length of the object ID.
*       IdSize          The length of the object sub ID.
*
*   OUTPUTS
*
*       NU_NULL         The MIB was not inserted.
*       *Junk           The MIB was successfully inserted.
*
*************************************************************************/
mib_local_t* RMON_MibInsert(snmp_object_t *Obj, mib_local_t **local,
                            UINT16 IdLen,UINT16 IdSize)
{
    mib_local_t *Junk = NU_NULL;
    mib_local_t *Local = *local;

    /* If the object ID length is valid */
    if ( (Obj->IdLen == (UINT32)(IdLen + IdSize)) &&
         (Obj->Id[IdLen] >= 1) )
    {
        /* Allocate memory for the new MIB */
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&Junk,
                               (UNSIGNED)(sizeof(mib_local_t)),
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            Junk = TLS_Normalize_Ptr(Junk);

            UTL_Zero(Junk, sizeof(mib_local_t));

            /* Set the index of the new MIB to the end of the object ID */
            Junk->Index = (INT32)Obj->Id[IdLen];

            /* If this is the first MIB or the new MIB is less than the
             * previous MIB, insert it before the previous MIB
             */
            if ( (Local == NU_NULL) || (Local->Index > Junk->Index) )
            {
                Junk->Next = Local;
                *local = Junk;
            }

            /* Otherwise, insert the MIB at the end of the list */
            else
            {
                while ( (Local->Next != NU_NULL) &&
                        (Local->Next->Index <= Junk->Index) )
                {
                    Local = Local->Next;
                }

                /* If the MIB already exists, deallocate the memory and
                 * exit.
                 */
                if (Local->Index == Junk->Index)
                {
                    NU_Deallocate_Memory((VOID*)Junk);
                    Junk = Local;
                }

                /* Otherwise, add the MIB to the end of the list */
                else
                {
                    Junk->Next = Local->Next;
                    Local->Next = Junk;
                }
            }
        }
    }

    /* Return a pointer to the MIB */
    return (Junk);

} /* RMON_MibInsert */

/************************************************************************
*
*   FUNCTION
*
*       RMON_MibRemove
*
*   DESCRIPTION
*
*       This function removes a MIB from a given list.
*
*   INPUTS
*
*       *Obj            A pointer to the object to remove.
*       **local         A pointer to a pointer to the list into which
*                       to remove the node.
*       IdLen           The length of the object ID.
*       IdSize          The length of the object sub ID.
*
*   OUTPUTS
*
*       NU_TRUE         The MIB was removed.
*       NU_FALSE        The MIB was not removed.
*
*************************************************************************/
BOOL RMON_MibRemove (snmp_object_t *Obj, mib_local_t **local,
                     UINT16 IdLen, UINT16 IdSize)
{
    BOOL        success = NU_TRUE;
    mib_local_t *Junk;
    mib_local_t *Local = *local;

    if ( (Obj != NU_NULL) && (Local != NU_NULL) )
    {
        /* If the object ID is invalid, return an error */
        if ( ((Obj->IdLen != (UINT32)(IdLen + IdSize)) ||
              (Obj->Id[IdLen] < 1)) && (Local == NU_NULL) )
             success = NU_FALSE;

        else if (Obj->IdLen != (UINT32)(IdLen + IdSize))
            success = NU_FALSE;

        /* Otherwise, the target MIB to remove is the first MIB, so
         * remove it.
         */
        else if ((UINT32)Local->Index == Obj->Id[IdLen])
        {
            Junk = Local->Next;
            NU_Deallocate_Memory((VOID*)Local);
            *local = Junk;
        }

        /* Otherwise, find the MIB in the list and remove it */
        else
        {
            /* While we have not found the target MIB and we have not
             * reached the end of the list, traverse the list.
             */
            while ( (Local->Next != NU_NULL) &&
                    ((UINT32)Local->Next->Index < Obj->Id[IdLen]) )
                Local = Local->Next;

            /* If we found the target, remove it */
            if ( (Local->Next != NU_NULL) &&
                 ((UINT32)Local->Next->Index == Obj->Id[IdLen]) )
            {
                Junk = Local->Next->Next;
                NU_Deallocate_Memory((VOID*)(Local->Next));
                Local->Next = Junk;
            }
        }
    }
    else
        success = NU_FALSE;

    return (success);

} /* RMON_MibRemove */

/************************************************************************
*
*   FUNCTION
*
*       RMON_MibRmon
*
*   DESCRIPTION
*
*       This function checks the syntax of an RMON MIB.
*
*   INPUTS
*
*       *Obj        A pointer to the object to verify.
*       *Local      A pointer to the list of objects.
*       IdLen       The length of the object ID.
*       IdSize      The length of sub ID.
*
*   OUTPUTS
*
*       NU_NULL     The list of objects is NU_NULL or the syntax is
*                   incorrect.
*       Local       The syntax is correct.
*
*************************************************************************/
mib_local_t *RMON_MibRmon (snmp_object_t *Obj, mib_local_t *Local,
                           UINT16 IdLen, UINT16 IdSize)
{
    if (Local != NU_NULL)
    {
        if ((Obj->Request != SNMP_PDU_NEXT) &&
            (Obj->Request != SNMP_PDU_BULK))
        {
            if (Obj->IdLen != (UINT32)(IdLen + IdSize))
            {
                return (NU_NULL);
            }

            while ((Local != NU_NULL) &&
                   ((UINT32)Local->Index < Obj->Id[IdLen]))
            {
                Local = Local->Next;
            }

            if ( (Local == NU_NULL) ||
                 ((UINT32)Local->Index != Obj->Id[IdLen]) )
            {
                return (NU_NULL);
            }
        }

        else
        {
            if (Obj->IdLen != IdLen)
            {
                while ( (Local != NU_NULL) &&
                        ((UINT32)Local->Index < Obj->Id[IdLen]) )
                {
                    Local = Local->Next;
                }

                if ( (Local == NU_NULL) ||
                     ((UINT32)Local->Index < Obj->Id[IdLen]) )
                {
                    return (NU_NULL);
                }
            }
        }
    }

    return (Local);

} /* RMON_MibRmon */

/************************************************************************
*
*   FUNCTION
*
*       RMON_Request
*
*   DESCRIPTION
*
*       This function checks the syntax of Obj, and if syntactically
*       correct, makes the proper PDU call.
*
*   INPUTS
*
*       *Obj            A pointer to the object.
*       *lastindex      A pointer to the last object in the MIB table
*                       accessed.
*
*   OUTPUTS
*
*       SNMP_NOERROR        The call was successfully completed.
*       SNMP_GENERROR       There are no requests to process.
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_BADVALUE       There object is not valid.
*
*************************************************************************/
STATUS RMON_Request (snmp_object_t *Obj, INT32 *lastindex)
{
    INT32   mindex, cmp;
    STATUS  status;

    /* If there are no requests to process, set error status and return */
    if (MibRoot.Table != NU_NULL)
    {
        status = SNMP_NOSUCHNAME;

        /* Compare the object ID in the packet with the object ID of the
         * most recently requested object.  MibRoot.Table is a table that
         * holds the definition for each MIB II object.  The value of cmp
         * will tell us where in relation to the most recently requested
         * object the newly requested object is located.
         */
        cmp = MibCmpObjId(MibRoot.Table[*lastindex]->Id,
                          MibRoot.Table[*lastindex]->IdLen,
                          Obj->Id, (UINT16)Obj->IdLen);

        /* If the object ID of the most recently requested object is less
         * than the object ID in the packet and the user is requesting a
         * GETNEXT and there is a next object defined in MibRoot, compare
         * the object ID in the next packet with the object ID defined for
         * that object.
         */
        if ( (cmp == -2) && (Obj->Request == SNMP_PDU_NEXT) &&
             (++(*lastindex) < MibRoot.Count) )
        {
            cmp = MibCmpObjId(MibRoot.Table[*lastindex]->Id,
                              MibRoot.Table[*lastindex]->IdLen,
                              Obj->Id, (UINT16)Obj->IdLen);
        }

        /* If we found the object in the MibRoot.Table, assign the mindex
         * to that index value.
         */
        if ( (cmp == -1) || (cmp == 0) )
        {
            mindex = *lastindex;
        }

        /* Otherwise, find the object */
        else
        {
            mindex = (*lastindex) =
                        MibObjectFind(Obj->Id, (INT32)(Obj->IdLen), &cmp);
        }

        /* Check the syntax for the request and make the call */
        switch (Obj->Request)
        {
        case SNMP_PDU_GET:

            /* If we did not find the object in the MibRoot.Table, break
             * and return an error.
             */
            if ( ((cmp != -1) && (cmp != 0)) ||
                 (MibRoot.Table[mindex]->Rqs == NU_NULL) )
                break;

            /* If the object is not read-accessible, return an error */
            if (!(MibRoot.Table[mindex]->Support & MIB_READ))
                break;

            /* Make the get call */
            if ((status = MibRoot.Table[mindex]->Rqs(Obj,
                                  (UINT16)(MibRoot.Table[mindex]->IdLen),
                                    NU_NULL)) == SNMP_NOERROR)
                Obj->Type = MibRoot.Table[mindex]->Type;

            break;

        case SNMP_PDU_SET:

            if ( ((cmp != -1) && (cmp != 0)) ||
                 (MibRoot.Table[mindex]->Rqs == NU_NULL) )
                break;

            /* If the object is not write-accessible, return an error */
            if (!(MibRoot.Table[mindex]->Support & MIB_WRITE))
            {
                status = SNMP_NOSUCHNAME;
                break;
            }

            if ( (Obj->Type == SNMP_OBJECTID) &&
                 (Obj->SyntaxLen > SNMP_SIZE_BUFINT) )
            {
                status = SNMP_BADVALUE;
                break;
            }

            /* If the new data is larger than the defined size for that
             * data type, return an error.
             */
            if ( ((Obj->Type == SNMP_OCTETSTR) ||
                  (Obj->Type == SNMP_DISPLAYSTR) ||
                  (Obj->Type == SNMP_OPAQUE)) &&
                 (Obj->SyntaxLen >= SNMP_SIZE_BUFCHR) )
            {
                status = SNMP_BADVALUE;
                break;
            }

            /* If the type of the new data is not the same as the type
             * defined for that object's data type, return an error.
             */
            if (Obj->Type != MibRoot.Table[mindex]->Type)
            {
                status = SNMP_BADVALUE;
                break;
            }

            /* Make the set call */
            status = MibRoot.Table[mindex]->Rqs(Obj,
                        (UINT16)(MibRoot.Table[mindex]->IdLen), NU_NULL);

            break;

        case SNMP_PDU_NEXT:

            if (cmp == -2)
                break;

            if ( (cmp == -1) || (cmp == 0) )
            {
                /* If the request field is not NU_NULL and the object is
                 * read-accessible
                 */
                if ((MibRoot.Table[mindex]->Rqs != NU_NULL) &&
                    (MibRoot.Table[mindex]->Support & MIB_READ))
                {
                    /* Make the get next call */
                    if ((status = MibRoot.Table[mindex]->Rqs(Obj,
                                  (UINT16)(MibRoot.Table[mindex]->IdLen),
                                  NU_NULL)) == SNMP_NOERROR)
                        Obj->Type = MibRoot.Table[mindex]->Type;
                    else
                        mindex++;
                }
                else
                    mindex++;
            }

            /* If the status was SNMP_NOSUCHNAME and there are more
             * requests to process, process the remaining requests.
             */
            while ( (status == SNMP_NOSUCHNAME) &&
                    (mindex < MibRoot.Count) )
            {
                NU_BLOCK_COPY(Obj->Id, MibRoot.Table[mindex]->Id,
                            (unsigned int)(MibRoot.Table[mindex]->IdLen *
                                                          sizeof(INT32)));

                Obj->IdLen = MibRoot.Table[mindex]->IdLen;

                if (MibRoot.Table[mindex]->Support & MIB_READ &&
                    MibRoot.Table[mindex]->Rqs != NU_NULL)
                {
                    if ((status = MibRoot.Table[mindex]->Rqs(Obj,
                                  (UINT16)(MibRoot.Table[mindex]->IdLen),
                                    NU_NULL)) == SNMP_NOERROR)
                    {
                        Obj->Type = MibRoot.Table[mindex]->Type;
                        break;
                    }
                }
                mindex++;
            }
            break;

        case SNMP_PDU_COMMIT:
        case SNMP_PDU_UNDO:

            /* Undo the previous command */
            if( ((cmp != -1) && (cmp != 0)) ||
                 MibRoot.Table[mindex]->Rqs == NU_NULL)
                break;

            status = MibRoot.Table[mindex]->Rqs(Obj,
                        (UINT16)(MibRoot.Table[mindex]->IdLen), NU_NULL);

            break;
        }
    }

    else
        status = SNMP_GENERROR;

    return (status);

} /* RMON_Request */


/************************************************************************
*
*   FUNCTION
*
*       RMON_MibRequest
*
*   DESCRIPTION
*
*       This function processes an incoming request.
*
*   INPUTS
*
*       listLen         The length of the list of requests.
*       *list           A pointer to the list of requests.
*       *errindex       The error encountered.
*
*   OUTPUTS
*
*       errstatus       The error encountered.
*
*************************************************************************/
UINT16 RMON_MibRequest (UINT32 listLen, snmp_object_t *list,
                        UINT16 *errindex)
{
    UINT16                  status, mindex, errstatus;
    static  snmp_object_t   MibRequestbackup[AGENT_LIST_SIZE];
    static  INT32           MibRequestlastindex[AGENT_LIST_SIZE];

    /* Process each request in the list */
    for (mindex = 0, status = SNMP_NOERROR;
         ((mindex < listLen) && (status == SNMP_NOERROR));
         mindex++)
    {
        if (list[mindex].Request != SNMP_PDU_SET)
            MibRequestbackup[mindex] = list[mindex];
        else
            MibRequestbackup[mindex].Request = SNMP_PDU_SET;

        /* object request type in list is set correctly */
        status = (UINT16)(RMON_Request(list + mindex,
                                       MibRequestlastindex + mindex));
    }

    errstatus = status;
    *errindex = mindex;

    /* If it's a set, then go through the list a second time and actually
     * do the set, if there were no errors the first time.
     */
    if (errstatus == SNMP_NOERROR)
    {
        for (mindex = 0, status = SNMP_NOERROR;
             ((mindex < listLen) && (status == SNMP_NOERROR));
             mindex++)
        {
            if (list[mindex].Request == SNMP_PDU_SET)
            {
                list[mindex].Request = SNMP_PDU_COMMIT;
                status = (UINT16)(RMON_Request(list + mindex,
                                           MibRequestlastindex + mindex));
            }
        }

        errstatus = status;
        *errindex = mindex;
    }

    if (errstatus != SNMP_NOERROR)
    {
        /* Count the errors by types for reporting snmp group errors */
        switch (errstatus)
        {
            case SNMP_NOSUCHNAME:
                SnmpStat.OutNoSuchNames++;
                break;
            case SNMP_BADVALUE:
                SnmpStat.OutBadValues++;
                break;
            case SNMP_READONLY:
                break;
            case SNMP_GENERROR:
                SnmpStat.OutGenErrs++;
                break;
            default:
                break;
        }

        for (mindex = 0, status = SNMP_NOERROR;
             ((mindex < *errindex) && (status == SNMP_NOERROR));
             mindex++)
        {
            /* If a set was attempted, this can undo those sets when a set
             * fails.
             */
            if ( (list[mindex].Request == SNMP_PDU_SET) ||
                 (list[mindex].Request == SNMP_PDU_COMMIT) )
            {
                list[mindex].Request = SNMP_PDU_UNDO;
                status = (UINT16)(RMON_Request(list + mindex,
                                           MibRequestlastindex + mindex));
            }
            else
                list[mindex] = MibRequestbackup[mindex];
        }
    }

    return (errstatus);

} /* RMON_MibRequest */

#endif


