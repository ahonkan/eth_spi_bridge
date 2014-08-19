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
*       mib_api.c                                                
*
*   DESCRIPTION
*
*       This file contains API functions specific to
*       maintenance of MIB list in the system.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MibInsert
*       MibRemove
*       MibDeRegister
*       MibObjectDelete
*
*   DEPENDENCIES
*
*       snmp.h
*       mib.h
*
*************************************************************************/

#include "networking/snmp.h"
#include "networking/mib.h"

extern mib_root_t       MibRoot;
extern NU_MEMORY_POOL   System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       MibInsert
*
*   DESCRIPTION
*
*       This function inserts a node into the specified list.
*
*   INPUTS
*
*       *Obj                    A pointer to the object to insert.
*       **local                 A pointer to a pointer to the list into
*                               which to insert the node.
*       IdLen                   The length of the object ID.
*       IdSize                  The length of the object sub ID.
*
*   OUTPUTS
*
*       NU_NULL                 The MIB was not inserted.
*       mib_local_t *           The MIB was successfully inserted.
*
*************************************************************************/
mib_local_t *MibInsert(snmp_object_t *Obj, mib_local_t **local,
                       UINT16 IdLen, UINT16 IdSize)
{
    mib_local_t *Junk = NU_NULL;
    mib_local_t *Local = *local;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */


    /* If the object ID length is valid */
    if ( (Obj->IdLen == (UINT32)(IdLen + IdSize)) &&
         (Obj->Id[IdLen] >= 1) )
    {
        /* Allocate memory for the new MIB */
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&Junk,
                               (UNSIGNED)(sizeof(mib_local_t)),
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
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
                    Local = Local->Next;

                /* If the MIB already exists, deallocate the memory and
                 * exit.
                 */
                if (Local->Index == Junk->Index)
                {
                    if (NU_Deallocate_Memory((VOID*)Junk) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }
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

    NU_USER_MODE();    /* return to user mode */

    /* Return a pointer to the MIB */
    return (Junk);

} /* MibInsert */

/************************************************************************
*
*   FUNCTION
*
*       MibRemove
*
*   DESCRIPTION
*
*       This function removes a MIB from a given list.
*
*   INPUTS
*
*       *Obj                    A pointer to the object to remove.
*       **local                 A pointer to a pointer to the list
*                               into which to remove the node.
*       IdLen                   The length of the object ID.
*       IdSize                  The length of the object sub ID.
*
*   OUTPUTS
*
*       NU_TRUE                 The MIB was removed.
*       NU_FALSE                The MIB was not removed.
*
*************************************************************************/
BOOLEAN MibRemove(snmp_object_t *Obj, mib_local_t **local, UINT16 IdLen,
               UINT16 IdSize)
{
    BOOLEAN        success = NU_TRUE;
    mib_local_t *Junk;
    mib_local_t *Local = *local;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */


    if ( (Obj != NU_NULL) && (Local != NU_NULL) )
    {
        /* If the object ID is invalid, return an error */
        if ( ((Obj->IdLen != (UINT32)(IdLen + IdSize)) ||
              (Obj->Id[IdLen] < 1) ) &&
             (Local == NU_NULL) )
        {
             success = NU_FALSE;
        }

        else if (Obj->IdLen != (UINT32)(IdLen + IdSize))
            success = NU_FALSE;

        /* Otherwise, the target MIB to remove is the first MIB, so remove
         * it.
         */
        else if ((UINT32)Local->Index == Obj->Id[IdLen])
        {
            Junk = Local->Next;
            if (NU_Deallocate_Memory((VOID*)Local) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            
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
                if (NU_Deallocate_Memory(Local->Next) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }

                Local->Next = Junk;
            }
        }
    }
    else
        success = NU_FALSE;

    NU_USER_MODE();    /* return to user mode */

    return (success);

} /* MibRemove */

/************************************************************************
*
*   FUNCTION
*
*       MibDeRegister
*
*   DESCRIPTION
*
*       This function removes an object from the global MIB list, MIB.
*
*   INPUTS
*
*       *object     A pointer to the object to remove.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID MibDeRegister(mib_object_t *object)
{
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */


    MibObjectDelete(object);
    NU_Deallocate_Memory((VOID*)(object->Id));
    NU_Deallocate_Memory((VOID*)object);

    NU_USER_MODE();    /* return to user mode */

} /* MibDeRegister */

/************************************************************************
*
*   FUNCTION
*
*       MibObjectDelete
*
*   DESCRIPTION
*
*       This function deletes a specified object from the MIB list.
*
*   INPUTS
*
*       *Object                 A pointer to the object to delete.
*
*   OUTPUTS
*
*       NU_TRUE                 The object was successfully deleted.
*       NU_FALSE                The object was not deleted.
*
*************************************************************************/
BOOLEAN MibObjectDelete(mib_object_t *Object)
{
    BOOLEAN    success = NU_TRUE;
    INT32   mindex, cmp, i;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */


    /* Find the object to delete */
    mindex = MibObjectFind(Object->Id, (INT32)Object->IdLen, &cmp);

    if (cmp != 0)
    {
        MibRoot.Count--;

        /* Remove the object */
        for (i = mindex; i < MibRoot.Count; i++)
            MibRoot.Table[i] = MibRoot.Table[i + 1];
    }
    else
        success = NU_FALSE;

    /* return to user mode */
    NU_USER_MODE();
    return (success);

} /* MibObjectDelete */


