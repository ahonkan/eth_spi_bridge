/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILENAME                                               
*
*       mib.c                                                    
*
*   DESCRIPTION
*
*       This file contains functions specific to maintaining the global
*       list of MIBs in the system.
*
*   DATA STRUCTURES
*
*       MibRoot
*       MIB
*       Snmp_Object_List
*
*   FUNCTIONS
*
*       MIB_Search_Object_InMibTable
*       MibInit
*       MIB_Get_Instance
*       MIB_Get_Next_Instance
*       MIB_Set_Instance
*       MIB_Set_Request
*       MIB_Process_Bulk
*       MIB_V2_Request
*       MIB_Request_V2
*       MIBRequest
*       Request
*       MibSimple
*       MibCmpObjId
*       MibObjectFind
*       MibObjectInsert
*       SNMP_Mib_Register
*       SNMP_Find_Object
*       SNMP_Mib_Unregister
*       SNMP_Get_Bulk
*
*   DEPENDENCIES
*
*       snmp_cfg.h
*       snmp.h
*       vacm.h
*       agent.h
*       mib.h
*
*************************************************************************/

#include "networking/snmp_cfg.h"
#include "networking/snmp.h"
#include "networking/vacm.h"
#include "networking/agent.h"
#include "networking/mib.h"

extern snmp_stat_t      SnmpStat;
extern agent_stat_t     agentStat;
extern NU_MEMORY_POOL   System_Memory;

mib_root_t              MibRoot;
NU_SEMAPHORE            SNMP_Resource;
mib_rmon_t              Mib =  {NU_NULL, 0, NU_NULL, 0,};

/* The following is an object lists used to keep objects during processing
 * of requests.
 */
static snmp_object_t    Snmp_Object_List[AGENT_LIST_SIZE];

/************************************************************************
*
*   FUNCTION
*
*       MIB_Search_Object_InMibTable
*
*   DESCRIPTION
*
*       This function finds the index in the MibTable
*
*   INPUTS
*
*       *obj            A pointer to the object.
*       *lastindex      A pointer to the last accessed object in the MIB
*                       table.
*
*   OUTPUTS
*
*       0               We have found an MIB object whose ID is exactly
*                       the same ID as the passed SNMP object.
*       -1              We found an MIB object, which has the same type
*                       as the passed instance.
*       -2              Object 1 is less than object 2.
*       1,2             We have found an MIB object whose OID is greater
*                       than the OID for the passed object. This is the
*                       case for GET-NEXT.
*
*************************************************************************/
INT32 MIB_Search_Object_InMibTable(snmp_object_t *obj, UINT16 *lastindex)
{
    INT32   cmp;

    /* In case of only one entry in MibRoot, last index always increases
     * after user connects, This check ensures that lastindex is updated
     * to initial value when the user connects again.
     */
    if (*lastindex >= (UINT16)MibRoot.Count)
        *lastindex = 0;

    /* Compare the object ID in the packet with the object ID of the most
     * recently requested object.  MibRoot.Table is a table that holds
     * the definition for each MIB object.  The value of cmp will tell us
     * where in relation to the most recently requested object the newly
     * requested object is located.
     */
    cmp = MibCmpObjId(MibRoot.Table[*lastindex]->Id,
                      MibRoot.Table[*lastindex]->IdLen,
                      obj->Id, obj->IdLen);

    /* If the object ID of the most recently requested object is less than
     * the object ID in the packet and the user is requesting a GETNEXT
     * and there is a next object defined in MibRoot, compare the object
     * ID in the next packet with the object ID defined for that object.
     */
    if ((cmp == -2) && (obj->Request == SNMP_PDU_NEXT) &&
         (++(*lastindex) < (UINT16)MibRoot.Count))
    {
        cmp = MibCmpObjId(MibRoot.Table[*lastindex]->Id,
                          MibRoot.Table[*lastindex]->IdLen,
                          obj->Id, (UINT16)obj->IdLen);
    }

    /* Find the object */
    if ((cmp != -1) && (cmp != 0))
    {
        *lastindex = (UINT16)MibObjectFind(obj->Id, (INT32)(obj->IdLen),
                                          &cmp);
    }

    return (cmp);

} /* MIB_Search_Object_InMibTable */

/************************************************************************
*
*   FUNCTION
*
*       MibInit
*
*   DESCRIPTION
*
*       This function adds a MIB element to the global list of MIB
*       elements, MIB.
*
*   INPUTS
*
*       *mib                    A pointer to the MIB element to add.
*       mibsize                 The size of the MIB element to add.
*
*   OUTPUTS
*
*       NU_SUCCESS              MIB registeration successfull.
*       SNMP_NO_MEMORY          Memory allocation failed
*       SNMP_BAD_PARAMETER      Atleast one of the MIB objects is 
*                               already registered
*       NU_INVALID_SEMAPHORE    Invalid semaphore pointer
*       NU_INVALID_SUSPEND      Suspension requested from non-task
*
*************************************************************************/
STATUS MibInit(mib_element_t *mib, UINT16 mibsize)
{
    /* Success or failure code to return.*/
    STATUS    status = NU_SUCCESS;

    /* Set the Prf parameter of MIB to NULL */
    Mib.Prf = NU_NULL;

    /* Initialize count of element in MibRoot to 'zero(0)'. */
    MibRoot.Count = 0;

    /* Initialize size of MibRoot table to 'zero(0)'. */
    MibRoot.Size = 0;

    /* Initialize MibRoot Table pointer to 'NU_NULL'. */
    MibRoot.Table = NU_NULL;

    /* Create the SNMP Resource Semaphore. */
    status = NU_Create_Semaphore(&SNMP_Resource, "SNMP Resource", 1, NU_FIFO);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("SNMP: Failed to create semaphore", NERR_SEVERE,
                        __FILE__, __LINE__);

        /* Return error code if we failed to create SNMP Resource
         * semaphore.
         */
    }
    else
    {
        /* Populate the MIB table. */
       status = SNMP_Mib_Register(mib, mibsize);
  
    }

    /* Return success or failure code. */
    return (status);

} /* MibInit */

/************************************************************************
*
*   FUNCTION
*
*       MIB_Get_Instance
*
*   DESCRIPTION
*
*       This function processes a GET request for a single instance.
*
*   INPUTS
*
*       *obj            Pointer to an SNMP object whose value is desired.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       security_model  Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*       view_type       Specifies the view type (Notify or Read). Note
*                       that we use this function to get instances to be
*                       sent in notifications. At such times the view is
*                       Notify.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       SNMP_NOSUCHOBJECT       Unknown object.
*       SNMP_NOACCESS           Object type exists, but we do not have
*                               read access on this object.
*       SNMP_NOSUCHINSTANCE     Required instance of the object was not
*                               found.
*       SNMP_AUTHORIZATIONERROR User is not allowed access to the required
*                               instance.
*
*************************************************************************/
STATUS MIB_Get_Instance(snmp_object_t *obj, UINT16 *lastindex,
                        UINT32 security_model, UINT8 security_level,
                        UINT8 *security_name, UINT8 *context_name,
                        UINT8 view_type, UINT32 notify_access)
{
    /* Status of the request. */
    STATUS              status;

    /* Variable used to do comparisons of the OIDs. */
    INT32               cmp;

    /* First make sure that we have access to the instance we are trying
     * to retrieve.
     */
    status = VACM_CheckInstance_Access(security_model, security_level,
                                       security_name, context_name,
                                       view_type, obj->Id, obj->IdLen);

    /* If we have been allowed access. */
    if(status == NU_SUCCESS)
    {
        /* Search for the MIB object which is the base type for this
         * instance.
         */
        cmp = MIB_Search_Object_InMibTable(obj, lastindex);

        /* Make sure that the object was found in MIB table. */
        if(((cmp != -1) && (cmp != 0)) ||
           (MibRoot.Table[*lastindex]->Rqs == NU_NULL))
        {
            /* The requested object does not exist. */
            status = SNMP_NOSUCHOBJECT;
        }

        /* Also make sure that the object has read access or it has 
         accessible-for-notify access and the call is for notification
         */
        else if (((MibRoot.Table[*lastindex]->Support & 
                           MIB_ACCESSIBLE_FOR_NOTIFY) &&
                        (!notify_access)) ||
                    (!((MibRoot.Table[*lastindex]->Support & MIB_READ) ||
                        (MibRoot.Table[*lastindex]->Support & 
                           MIB_ACCESSIBLE_FOR_NOTIFY))
                ))
        {
            /* We do not have read access to this object. */
            status = SNMP_NOSUCHOBJECT;
        }

        /* Otherwise, we found the matching object. */
        else
        {
            /* Make a call to get the specified instance of this object.
             */
            status = MibRoot.Table[*lastindex]->Rqs(
                        obj,
                        (UINT16)(MibRoot.Table[*lastindex]->IdLen),
                        NU_NULL);

            /* If the instance was successfully retrieved. */
            if(status == NU_SUCCESS)
            {
                /* Get the basic type of this object. */
                obj->Type = MibRoot.Table[*lastindex]->Type;

                if (view_type == VACM_READ_VIEW)
                {
                    ((AgentStatistics())->InTotalReqVars)++;
                }
            }

            /* Otherwise, if this is a GET operation, than we did not
             * find the instance.
             */
            else if(obj->Request == SNMP_PDU_GET)
            {
                /* The instance that we are looking for was not found. */
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    else if ((status == VACM_NOSUCHCONTEXT) ||
             (status == VACM_NOACCESSENTRY))
    {
        /* Authorization failed. */
        status = SNMP_AUTHORIZATIONERROR;
    }

    else if (status == VACM_NOTINVIEW)
    {
        /* Not accessible. */
        status = SNMP_NOACCESS;
    }

    else
    {
        /* Authorization failed. */
        status = SNMP_NOSUCHOBJECT;
    }

    return (status);

} /* MIB_Get_Instance */

/************************************************************************
*
*   FUNCTION
*
*       MIB_Get_Next_Instance
*
*   DESCRIPTION
*
*       This function processes a GET-NEXT request for a single instance.
*
*   INPUTS
*
*       *obj            Pointer to an SNMP object whose next instance is
*                       desired.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       security_model  Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       SNMP_ENDOFMIBVIEW       There are no more instances greater than
*                               what was passed.
*       SNMP_NOSUCHOBJECT       Unknown object.
*       SNMP_NOACCESS           Object type exists, but we do not have
*                               read access on this object.
*       SNMP_NOSUCHINSTANCE     Required instance of the object was not
*                               found.
*       SNMP_AUTHORIZATIONERROR User is not allowed access to the required
*                               instance.
*
*************************************************************************/
STATUS MIB_Get_Next_Instance(snmp_object_t *obj, UINT16 *lastindex,
                             UINT32 security_model, UINT8 security_level,
                             UINT8 *security_name, UINT8 *context_name, 
                             UINT32 notify_access)
{
    /* Status of the request. */
    STATUS              status = NU_SUCCESS;

    /* Variable used to do comparisons of the OIDs. */
    INT32               cmp;

    UINT32          max_repetitions;


    /* Get the no of repetitions for this particular object. */
    max_repetitions = obj[0].Syntax.LngUns;

    /* Search for the MIB object which is the base type for this
     * instance. If such an object is not found, then the following
     * call will return index for the next object. This is the lowest
     * OID found that is also greater than the passed object ID.
     */
    cmp = MIB_Search_Object_InMibTable(obj, lastindex);

    /* We could not find an object that had a greater OID. This means
     * that we have reached the end of our MIB view.
     */
    if (cmp == -2)
    {
        /* We have reached end of MIB view. */
        status = SNMP_ENDOFMIBVIEW;
    }

    /* We have found an object which has an OID greater than what was
     * passed but this is the lowest such OID.
     */
    else if((cmp == 1) || (cmp == 2))
    {
        /* Update the object ID to point to this new OID. We first clear
         * the ID just to make sure that none of the identifiers from
         * previous ID remain.
         */
        UTL_Zero(obj->Id, SNMP_SIZE_OBJECTID * sizeof(UINT32));
        NU_BLOCK_COPY(obj->Id, MibRoot.Table[*lastindex]->Id,
               MibRoot.Table[*lastindex]->IdLen * sizeof(UINT32));
        obj->IdLen = MibRoot.Table[*lastindex]->IdLen;
    }

    /* We will keep in this loop till we reach the end of MIB or we find
     * an appropriate instance.
     */
    while(status != SNMP_ENDOFMIBVIEW)
    {
        /* If in the last iteration, we did not find an instance for the
         * selected object. Move to the next object. Make sure that we
         * did not encounter an authorization error. If we did, this means
         * that an instance was returned in our last call but it failed
         * access checks. It is possible that instances for the same
         * object exist that will pass the access checks. In this case
         * we will not advance in the MIB table.
         */
        if((status != NU_SUCCESS) && (status != SNMP_AUTHORIZATIONERROR))
        {
            /* Since the MIB table is ordered in ascending order of OID's
             * going one step up takes us to the next eligible candidate.
             */
            (*lastindex)++;

            /* Make sure that we do not go outside the MIB view, if we do
             * return end of MIB view.
             */
            if((*lastindex) >= (UINT16)MibRoot.Count)
            {
                status = SNMP_ENDOFMIBVIEW;

                /* We are done with the processing. Move out of the loop.
                 */
                break;

            }

            /* Also make sure that this object is readable. */
            else if(!((MibRoot.Table[*lastindex]->Support & MIB_READ) &&
                      (MibRoot.Table[*lastindex]->Rqs != NU_NULL)))
            {
                /* If read is not supported then go to the next index. */
                continue;
            }
            else
            {
                /* Update the object ID to point to this new OID. We first
                 * clear the ID just to make sure that none of the
                 * identifiers from previous ID remain.
                 */
                UTL_Zero(obj->Id, SNMP_SIZE_OBJECTID * sizeof(UINT32));
                NU_BLOCK_COPY(obj->Id, MibRoot.Table[*lastindex]->Id,
                       MibRoot.Table[*lastindex]->IdLen * sizeof(UINT32));
                obj->IdLen = MibRoot.Table[*lastindex]->IdLen;
            }
        }

        if (obj->Request == SNMP_PDU_BULK)
            obj[0].Syntax.LngUns = max_repetitions;

        /* Also make sure that the object has read access or it has 
        accessible-for-notify access and the call is for notification
        */
        if (((MibRoot.Table[*lastindex]->Support & 
                MIB_ACCESSIBLE_FOR_NOTIFY) &&
            (!notify_access)) ||
            (!((MibRoot.Table[*lastindex]->Support & MIB_READ) ||
            (MibRoot.Table[*lastindex]->Support & 
                MIB_ACCESSIBLE_FOR_NOTIFY))
            ))
        {
            status = SNMP_NOSUCHOBJECT;
        }
        else
        {
        /* Make the Get-Next Request. */
            status = MibRoot.Table[*lastindex]->Rqs(obj,
                            (UINT16)(MibRoot.Table[*lastindex]->IdLen),
                            NU_NULL);
        }

        /* If the instance was successfully retrieved, then check whether
         * specified user has access to this instance. We do this check
         * only for the GET-NEXT case, for the GET-BULK case there may
         * be more than one instances returned, we check each instance
         * in the GET-BULK function which called this.
         */
        if((status == NU_SUCCESS) &&
           (obj->Request == SNMP_PDU_NEXT))
        {
            status = VACM_CheckInstance_Access(
                        security_model, security_level, security_name,
                        context_name, VACM_READ_VIEW,
                        obj->Id, obj->IdLen);

            /* If the user is allowed access to the found instance.
             * Get the type of the instance, and break out of the loop.
             * We have found the entry we were looking for.
             */
            if(status == NU_SUCCESS)
            {
                obj->Type = MibRoot.Table[*lastindex]->Type;
                break;

            }
            else
            {
                /* Authorization failed. */
                status = SNMP_AUTHORIZATIONERROR;
            }
        }

        /* If for the GET-BULK case we encounter success, then just
         * copy type of object and return.
         */
        else if((status == NU_SUCCESS) &&
                (obj->Request == SNMP_PDU_BULK))
        {
            /* Save the type of object. Note that all instances returned
             * will be of the same type. We save the type in the first
             * instance. We will copy this type to all instances later.
             */
            obj->Type = MibRoot.Table[*lastindex]->Type;
            break;
        }
    }

    return (status);

} /* MIB_Get_Next_Instance */

/************************************************************************
*
*   FUNCTION
*
*       MIB_Set_Instance
*
*   DESCRIPTION
*
*       This function processes a SET request for a single instance.
*
*   INPUTS
*
*       *obj            Pointer to an SNMP object whose value is to be
*                       set.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       security_model  Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       SNMP_NOTWRITABLE        Object is not-writable.
*       SNMP_WRONGTYPE          The value being set is not of the required
*                               type.
*       SNMP_WRONGLENGTH        Length of the value being set in not in
*                               range.
*       SNMP_AUTHORIZATIONERROR User is not allowed access to the required
*                               instance.
*
*************************************************************************/
STATUS MIB_Set_Instance(snmp_object_t *obj, UINT16 *lastindex,
                        UINT32 security_model, UINT8 security_level,
                        UINT8 *security_name, UINT8 *context_name)
{
    /* Status of the request. */
    STATUS              status;

    /* Make sure that the user has access to this instance. */
    status = VACM_CheckInstance_Access(
                security_model, security_level, security_name,
                context_name, VACM_WRITE_VIEW,
                obj->Id, obj->IdLen);

    /* If the user has access to set this view. */
    if(status == NU_SUCCESS)
    {
        /* Find the object corresponding to this entry. Note that we will
         * always find the object. Reason for this is that we do a GET
         * before every set request. If we found the object at that
         * point we will also find it now. If we did not find it
         * we would not get to this point.
         */
        MIB_Search_Object_InMibTable(obj, lastindex);

        /* Verify that the object is writable. */
        if (!(MibRoot.Table[*lastindex]->Support & MIB_WRITE) &&
            !(MibRoot.Table[*lastindex]->Support & MIB_CREATE))
        {
            status = SNMP_NOTWRITABLE;
        }

        /* Verify that we are trying to write a value with the correct
         * type.
         */
        else if(obj->Type != MibRoot.Table[*lastindex]->Type)
        {
            status = SNMP_WRONGTYPE;
        }

        /* Verify that the lengths have not exceed the maximum limit. */
        else if (((obj->Type == SNMP_OBJECTID) &&
                  (obj->SyntaxLen > SNMP_SIZE_BUFINT)) ||
                 (((obj->Type == SNMP_OCTETSTR) ||
                   (obj->Type == SNMP_DISPLAYSTR) ||
                   (obj->Type == SNMP_OPAQUE)) &&
                  (obj->SyntaxLen >= SNMP_SIZE_BUFCHR)))
        {
            status = SNMP_WRONGLENGTH;
        }
        else
        {
            /* Otherwise, everything is in order. Now make the set call.
             */
            status = MibRoot.Table[*lastindex]->Rqs(
                    obj,
                    (UINT16)(MibRoot.Table[*lastindex]->IdLen),
                    NU_NULL);
        }
    }
    else if (status == VACM_NOTINVIEW)
    {
        /* Not accessible. */
        status = SNMP_NOACCESS;
    }

    else
    {
        /* Authorization failed. */
        status = SNMP_NOSUCHOBJECT;
    }

    /* Return the request. */
    return (status);

} /* MIB_Set_Instance */

/************************************************************************
*
*   FUNCTION
*
*       MIB_Set_Request
*
*   DESCRIPTION
*
*       This function processes a complete SET request PDU.
*
*   INPUTS
*
*       *obj_list       Pointer to an SNMP object list with values that
*                       need to be set.
*       list_length     Length of list.
*       *index          If an error occurred, this variable indicates the
*                       index of the object which caused the error. This
*                       index is relative to the original packet.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       security_model  Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       SNMP_NOTWRITABLE        Object is not-writable.
*       SNMP_WRONGTYPE          The value being set is not of the required
*                               type.
*       SNMP_WRONGLENGTH        Length of the value being set in not in
*                               range.
*
*************************************************************************/
STATUS MIB_Set_Request(snmp_object_t *obj_list, UINT32 list_length,
                       UINT16 *index, UINT16 *lastindex,
                       UINT32 security_model, UINT8 security_level,
                       UINT8 *security_name, UINT8 *context_name)
{
    /* Status of the request. */
    STATUS                  status = NU_SUCCESS;

    /* Saves an error which may have been encountered during a set
     * operations.
     */
    STATUS                  set_error = NU_SUCCESS;

    /* An array which specifies the four steps used in processing
     * of a SET Request.
     */
    UINT32                  set_steps[] = {SNMP_PDU_GET_SET, SNMP_PDU_SET,
                                          SNMP_PDU_COMMIT, SNMP_PDU_UNDO};

    /* Temporary variables used in loops. */
    UINT8                   i;

    /* This pointer points to a backup list which is used to store values
     * before the set operation. If we need to undo this operation, we
     * will use these values.
     */
    snmp_object_t           *backup_list = Snmp_Object_List;

    /* This pointer specifies the list which we are working on and on
     * which the requested operations will be executed.
     */
    snmp_object_t           *current_list;

    /* Make a copy of the passed list. */
    NU_BLOCK_COPY(backup_list, obj_list,
                  list_length * sizeof(snmp_object_t));

    /* The set request is executed in a maximum of four steps:
     * Get, Set, Commit and Undo. This loop is responsible for
     * the correct execution of these steps.
     *
     * Phase 1: In the first phase we get values for each of the
     * instances that we want to set. These values are saved,
     * should we require to undo the values that we set.
     *
     * Phase 2: In the second phase we set the instances with the
     * proposed values. If all the instances are successfully set
     * we move on to the commit phase. Otherwise, we perform Undo.
     *
     * Phase 3: In the third and possibly final phase we commit
     * the values that were set. If any of the values were not
     * successfully committed we perform Undo operation.
     *
     * Phase 4: In this phase we clean up from a failed set
     * or commit operation. We effectively move to the state
     * before the set operation was executed.
     */
    for(i = 0; (i < 4) && (status == NU_SUCCESS); i++)
    {
        /* If this is the GET or undo phase, we will use the backup
         * list.
         */
        if((set_steps[i] == SNMP_PDU_GET_SET) ||
           (set_steps[i] == SNMP_PDU_UNDO))
        {
            current_list = backup_list;
        }

        /* Otherwise, we will use the passed list. */
        else
        {
            current_list = obj_list;
        }

        /* Perform the current phase on all the objects in list, or
         * till we encounter an error.
         */
        for((*index) = 0; ((*index) < list_length) &&
                          (status == NU_SUCCESS); (*index)++)
        {
            /* Specify the action that needs to be taken. */
            current_list[(*index)].Request = set_steps[i];

            /* Now execute the request. */
            status = Request(&current_list[(*index)], lastindex,
                             security_model, security_level,
                             security_name, context_name);

            /* If the request type is GET and the status returned is
             * noSuchName or noSuchInstance, then we will ignore this
             * error. The reason is that this could be a create request
             * and therefore the instance does not exist.
             */
            if((set_steps[i] == SNMP_PDU_GET_SET) &&
               ((status == SNMP_NOSUCHNAME) ||
                (status == SNMP_NOSUCHINSTANCE)))
            {
                /* Clear the backup object. It may be required during the
                 * undo operation.
                 */
                UTL_Zero(&(current_list[(*index)].Syntax),
                           sizeof(snmp_syntax_t));

                /* Reset the status to success. */
                status = NU_SUCCESS;
            }
            /* If the previous get request was referring to a non-existent
             * object in view, then change the status to NO ACCESS.*/
            else if (status == SNMP_NOSUCHOBJECT) 
            {
                status = SNMP_NOACCESS;
            }
        }

        /* If the request that we were processing was either SET or Commit
         * and we encountered an error, then we will go to the undo stage.
         */
        if((status != NU_SUCCESS) &&
           ((set_steps[i] == SNMP_PDU_SET) ||
            (set_steps[i] == SNMP_PDU_COMMIT)))
        {
            /* Save the error that was encountered, we would want to pass
             * this error back.
             */
            set_error = status;

            /* Reset the status to success. */
            status = NU_SUCCESS;

            /* If undo phase occurs before the set phase concluded, then
             * just undo those elements that were set. We do this by
             * modifying the list length. So that our undo loop ends
             * there.
             */
            if(set_steps[i] == SNMP_PDU_SET)
                list_length = (*index);

            /* Set i to 2, since we are at the end of loop, this will be
             * incremented to 3 and therefore, we will execute undo.
             */
            i = 2;
        }

        /* Otherwise, if this was the commit phase, and we successfully
         * committed the values, than break out of the loop.
         */
        else if((status == NU_SUCCESS) &&
           (set_steps[i] == SNMP_PDU_COMMIT))
        {
            break;
        }
    }

    /* If we had encountered an error during the set or commit phase,
     * and we successfully undid that error, we want to still pass
     * the error that was encountered.
     */
    if((status == NU_SUCCESS) &&
       (set_error != NU_SUCCESS))
    {
        status = set_error;
    }

    if (status == NU_SUCCESS)
    {
        ((AgentStatistics())->InTotalSetVars)+= list_length;
    }

    return (status);

} /* MIB_Set_Request */

#if ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE))
/************************************************************************
*
*   FUNCTION
*
*       MIB_Process_Bulk
*
*   DESCRIPTION
*
*       This function processes a GET-BULK request.
*
*   INPUTS
*
*       *list_len       Length of the list of requested objects. This
*                       may change due to the GET-BULK processing.
*                       Therefore, a pointer is passed.
*       *list           Array of requested objects. On successful
*                       completion the list contains retrieved objects.
*       *index          If an error occurred, this variable indicates the
*                       index of the object which caused the error. This
*                       index is relative to the original packet.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       *snmp_pdu       Pointer to SNMP v2 PDU structure.
*       snmp_sm         Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS      Successful operation.
*
*************************************************************************/
STATUS MIB_Process_Bulk(UINT32 *list_len, snmp_object_t *list,
                        UINT16 *index, UINT16 *lastindex,
                        snmp_pdu_t *snmp_pdu, UINT32 snmp_sm,
                        UINT8 security_level, UINT8 *security_name,
                        UINT8 *context_name)
{
    /* Status of the request. */
    STATUS                  status = NU_SUCCESS;

    /* The number of columnar objects in this request. */
    UINT32                  columnar_objects_no = 0;

    /* Pointer to a list of columnar objects. Whenever, we retrieve a list
     * of columnar objects, we place it in this list.
     */
    snmp_object_t           *columnar_objects = Snmp_Object_List;

    /* Number of columnar objects that have been retrieved. */
    UINT32                  retrieved_instances;
    UINT32                  verified_instances;

    /* Type of retrieved instances. */
    UINT32                  instance_type;

    /* Variables used in temporary list. */
    UINT32                  i;
	
	
    /* Variable  for showing status VACM_ACCESS. */
    BOOLEAN                 VACM_Access_Status = NU_TRUE; 


    /* If value in non-repeaters field is less than zero then make it
     * zero (RFC 1905).
     */
    if(snmp_pdu->Bulk.snmp_non_repeaters < 0)
    {
        snmp_pdu->Bulk.snmp_non_repeaters = 0;
    }

    /* If value in max-repetitions field is less than zero make it zero
     * (RFC 1905).
     */
    if(snmp_pdu->Bulk.snmp_max_repetitions < 0)
    {
        snmp_pdu->Bulk.snmp_max_repetitions = 0;
    }

    /* If the number of non-repeaters is greater than the list length then
     * set the value of non repeaters to list length.
     */
    if((UINT32)snmp_pdu->Bulk.snmp_non_repeaters > (*list_len))
    {
        snmp_pdu->Bulk.snmp_non_repeaters = (INT32)(*list_len);
    }

    /* Otherwise, if list length is less than the number of non-repeaters
     * then calculate number of columnar objects as list length minus
     * the non repeaters.
     */
    else if((UINT32)snmp_pdu->Bulk.snmp_non_repeaters < (*list_len))
    {
        columnar_objects_no = (*list_len) -
                                (UINT32)snmp_pdu->Bulk.snmp_non_repeaters;

        /* Now make sure that the maximum number of repetitions do not
         * cause us to go beyond the maximum number of objects that
         * we can send in a PDU.
         */
        if((INT32)(columnar_objects_no *
                   snmp_pdu->Bulk.snmp_max_repetitions) >
           (INT32)(AGENT_LIST_SIZE -
                   snmp_pdu->Bulk.snmp_non_repeaters))
        {
            /* If we are exceeding our limit, then just update max
             * repetitions.
             */
            snmp_pdu->Bulk.snmp_max_repetitions =
                (AGENT_LIST_SIZE - snmp_pdu->Bulk.snmp_non_repeaters)/
                (columnar_objects_no);
        }
    }

    /* Loop through all the non-repeaters in the list and get values for
     * each, retrieving one at a time.
     */
    for((*index) = 0;
        ((*index) < ((UINT16)snmp_pdu->Bulk.snmp_non_repeaters)) &&
        (status == NU_SUCCESS);
        (*index)++)
    {
        /* This is the non-repeaters so this is actually a GET-NEXT
         * request.
         */
        list[(*index)].Request = SNMP_PDU_NEXT;

        /* Make the GET-Bulk request. */
        status = Request(&list[(*index)], lastindex, snmp_sm,
                        security_level, security_name,
                        context_name);


        /* Now convert the request back to a GET-BULK request. */
        list[(*index)].Request = SNMP_PDU_BULK;

        /* If we encounter end of MIB view, than we will continue to
         * retrieve the remaining instances in list.
         */
        if(status == SNMP_ENDOFMIBVIEW)
        {
            /* For the instance, for which this error occurred, set the
             * object type to reflect the error.
             */
            list[(*index)].Type = (UINT32)status;

            /* Reset the status to success. */
            status = NU_SUCCESS;
        }
    }

    /* If we successfully got all non-repeaters, than get the columnar
     * objects.
     */
    if(status == NU_SUCCESS)
    {
        /* Clear the columnar objects list. */
        UTL_Zero(columnar_objects,
                 sizeof(snmp_object_t) * AGENT_LIST_SIZE);

        /* Now loop through all the columnar objects in the list and get
         * the next instance for each object, retrieving instances one
         * column object at a time.
         */
        for(i = 0; (i  < columnar_objects_no) && (status == NU_SUCCESS);
            i++)
        {
            /* Copy the columnar objects to the columnar objects list,
             * keep a distance of max repetitions between each object,
             * the repeaters for the object will be placed in this gap.
             */
            NU_BLOCK_COPY(&columnar_objects[i *
                                    snmp_pdu->Bulk.snmp_max_repetitions],
                          &list[(i + snmp_pdu->Bulk.snmp_non_repeaters)],
                          sizeof(snmp_object_t));

            /* Reset the number of instances that have already been
             * retrieved and verified.
             */
            verified_instances = 0;

            /* Keep on retrieving objects, till we have gotten the number
             * of objects that we required or we encounter end of MIB
             * view.
             */
            do
            {
                /* Calculate the number of instances that still remain to
                 * be retrieved.
                 */
                columnar_objects[(i * snmp_pdu->Bulk.snmp_max_repetitions)
                                  + verified_instances].Syntax.LngUns
                    = (snmp_pdu->Bulk.snmp_max_repetitions -
                                                    verified_instances);

                /* Make the GET-Bulk request. */
                status = Request(&columnar_objects[
                                 (i * snmp_pdu->Bulk.snmp_max_repetitions)
                                                +   verified_instances],
                                lastindex, snmp_sm,
                                security_level, security_name,
                                context_name);

                /* Determine the instances that have already been
                 * retrieved. If success is returned than at least one
                 * instance has been found.
                 */
                if(status == NU_SUCCESS)
                {
                    retrieved_instances =
                        (i * snmp_pdu->Bulk.snmp_max_repetitions) +
                                                    verified_instances;

                    instance_type =
                               columnar_objects[retrieved_instances].Type;

                    /* While we still need more instances. */
                    for(;
                        retrieved_instances < ((i + 1) *
                                    snmp_pdu->Bulk.snmp_max_repetitions);
                        retrieved_instances++)
                    {
                        /* If the OID length of this object is greater
                         * than zero, then a valid instance is living
                         * here.
                         */
                        if(columnar_objects[retrieved_instances].IdLen > 0)
                        {
                            /* Make sure the user has access to this
                             * instance.
                             */
                            status = VACM_CheckInstance_Access(
                             snmp_sm, security_level,
                             security_name, context_name,
                             VACM_READ_VIEW,
                             columnar_objects[retrieved_instances].Id,
                             columnar_objects[retrieved_instances].IdLen);

                            if(status == NU_SUCCESS)
                            {
                                VACM_Access_Status = NU_TRUE; 

								
                                /* Update the object type. */
                                columnar_objects[retrieved_instances].Type
                                                          = instance_type;

                                /* Copy object to the original list. */
                                NU_BLOCK_COPY(
                                 &list[snmp_pdu->Bulk.snmp_non_repeaters
                                       + i + (verified_instances *
                                                    columnar_objects_no)],
                                 &columnar_objects[retrieved_instances],
                                 sizeof(snmp_object_t));

                                /* We have found another instance, which
                                 * has also passed security checks.
                                 * Increment counter.
                                 */
                                verified_instances++;
                            }
														        
                            else
                            {	
                                VACM_Access_Status = NU_FALSE; 
								
                                /*If status is not SUCCESS, break the loop. */
                                break;
                            }

                        }

                        /* Otherwise, we break out of the loop. We still
                         * need more instances.
                         */
                        else
                        {
                            /* Before we break out of the loop. Update the
                             * OID in the columnar object which we will
                             * use to get the remaining instances.
                             */
                            columnar_objects[(i * snmp_pdu->Bulk.snmp_max_repetitions)
                                                + verified_instances].IdLen =
                                columnar_objects[retrieved_instances - 1].IdLen;

                            NU_BLOCK_COPY(columnar_objects[(i * snmp_pdu->Bulk.snmp_max_repetitions)
                                                + verified_instances].Id,
                                          columnar_objects[retrieved_instances - 1].Id,
                                          sizeof(UINT32) * SNMP_SIZE_OBJECTID);

                            /* Set the request to GET-BULK. */
                            columnar_objects[(i * snmp_pdu->Bulk.snmp_max_repetitions)
                                                + verified_instances].Request = SNMP_PDU_BULK;

                            break;
                        }
                    }
                }

                /* If we encounter end of MIB view, than we will continue
                 * to retrieve the remaining instances in list.
                 */
                else if(status == SNMP_ENDOFMIBVIEW)
                {
                    /* For the instance, for which this error occurred, set
                     * the object type for the remaining repeaters to
                     * reflect the error.
                     */
                    for(; verified_instances <
                          (UINT32)snmp_pdu->Bulk.snmp_max_repetitions;
                          verified_instances++)
                    {
                        /* Set the endOfMibView error. */
                        list[snmp_pdu->Bulk.snmp_non_repeaters
                             + i + (verified_instances * columnar_objects_no)].Type =
                            (UINT32)status;

                        /* Update the OID for the objects to reflect the
                         * OID from columnar object.
                         */
                        list[snmp_pdu->Bulk.snmp_non_repeaters
                             + i + (verified_instances * columnar_objects_no)].IdLen =
                            list[(i + snmp_pdu->Bulk.snmp_non_repeaters)].IdLen;

                        NU_BLOCK_COPY(
                            &(list[snmp_pdu->Bulk.snmp_non_repeaters
                             + i + (verified_instances * columnar_objects_no)].Id),
                            &(list[(i + snmp_pdu->Bulk.snmp_non_repeaters)].Id[0]),
                            sizeof(UINT32) * SNMP_SIZE_OBJECTID);

                    }

                    /* Reset the status to success. */
                    status = NU_SUCCESS;
                }

                /* Otherwise, we encountered some other error, than get
                 * out of this loop.
                 */
                else
                {
                    break;
                }
				
                if ( VACM_Access_Status == NU_FALSE  )
                { 
                    break;
                } 
				
            }while(verified_instances !=
                   (UINT32)snmp_pdu->Bulk.snmp_max_repetitions);
				   
            if ( VACM_Access_Status == NU_FALSE  )
            { 
                break;
            } 				   
        }

        /* If we broke out of the loop because of an error, then determine the
         * index where the error was caused.
         */
        if(status != NU_SUCCESS)
        {
            (*index) = (UINT16)(i + snmp_pdu->Bulk.snmp_non_repeaters);
        }
        else
        {
            /* Update the list length. */
            *list_len = (snmp_pdu->Bulk.snmp_max_repetitions * columnar_objects_no) +
                        snmp_pdu->Bulk.snmp_non_repeaters;
        }

    }

    return (status);

} /* MIB_Process_Bulk */

/************************************************************************
*
*   FUNCTION
*
*       MIB_V2_Request
*
*   DESCRIPTION
*
*       This function processes an incoming request for v2 and v3.
*
*   INPUTS
*
*       *list_len               Length of the list of objects. This value
*                               may change for the case of GET-BULK.
*       *list                   List of objects to be processed.
*       *index                  If an error occurred, this variable
*                               indicates the index of the object which
*                               caused the error. This index is relative
*                               to the original packet.
*       *snmp_pdu               Version 2 Request PDU.
*       snmp_sm                 Security model.
*       security_level          Security level.
*       *security_name          Security name.
*       *context_name           Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       Error indication
*
*************************************************************************/
STATUS MIB_V2_Request(UINT32 *list_len, snmp_object_t *list,
                      UINT16 *index, snmp_pdu_t *snmp_pdu,
                      UINT32 snmp_sm, UINT8 security_level,
                      UINT8 *security_name, UINT8 *context_name)
{
    /* Status of the request. */
    STATUS                  status = NU_SUCCESS;

    /* This variable is used to remember the last object that was
     * retrieved. This helps in SNMP walks which will retrieve the
     * next instance. Therefore, we will not need to do a search
     * of the MIB table again.
     */
    static UINT16           last_index = 0;

    /* If this is a set request, then call the function which is
     * responsible for processing that request.
     */
    if(list[0].Request == SNMP_PDU_SET)
    {
        /* Execute the set request. All objects in the list
         * are set through this function call.
         */
        status = MIB_Set_Request(list, *list_len, index,
                                 &last_index, snmp_sm,
                                 security_level, security_name,
                                 context_name);
    }

    /* If this is a GET-BULK request, then call the function that
     * is responsible for processing that request.
     */
    else if(list[0].Request == SNMP_PDU_BULK)
    {
        /* Execute the GET-BULK request. All objects in the
         * list are processed through this function call.
         */
        status = MIB_Process_Bulk(list_len, list,
                                  index, &last_index,
                                  snmp_pdu, snmp_sm,
                                  security_level, security_name,
                                  context_name);
    }

    /* Otherwise this is one of the get requests (either a GET request
     * or a GET-NEXT request.
     */
    else
    {
        /* Loop through all the instance in the list and get values for
         * each, retrieving one at a time.
         */
        for((*index) = 0;
            ((*index) < (*list_len)) && (status == NU_SUCCESS);
            (*index)++)
        {
            /* Make the get request. */
            status = Request(&list[(*index)], &last_index, snmp_sm,
                            security_level, security_name,
                            context_name);

            /* If we encounter noSuchObject or noSuchName errors, than
             * we will continue to retrieve the remaining instances
             * in list.
             */
            if((status == SNMP_NOSUCHOBJECT) ||
               (status == SNMP_NOSUCHINSTANCE) ||
               (status == SNMP_ENDOFMIBVIEW))
            {
                /* For the instance, for which this error occurred, set the
                 * object type to reflect the error.
                 */
                list[(*index)].Type = (UINT32)status;

                /* Reset the status to success. */
                status = NU_SUCCESS;
            }
        }
    }

    /* If the processing was successful, then the error index will be
     * zero.
     */
    if(status == NU_SUCCESS)
        (*index) = 0;

    return (status);

} /* MIB_V2_Request */

/************************************************************************
*
*   FUNCTION
*
*       MIB_Request_V2
*
*   DESCRIPTION
*
*       This function has been left for backward compatibility. It
*       calls a function which is now used to do actual processing
*       of the object.
*
*   INPUTS
*
*       *Obj            A pointer to the object.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       securityModel   Security model.
*       securityLevel   Security level.
*       securityName    Security name.
*       contextName     Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS      Successful operation.
*
*************************************************************************/
STATUS MIB_Request_V2(snmp_object_t *Obj, UINT16 *lastindex,
                      UINT32 securityModel, UINT8 securityLevel,
                      UINT8 *securityName, UINT8 *contextName)
{
    return (Request(Obj, lastindex, securityModel,
                    securityLevel, securityName, contextName));

} /* MIB_Request_V2 */

#endif /* ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE)) */

#if (INCLUDE_SNMPv1 == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       MibRequest
*
*   DESCRIPTION
*
*       This function processes an incoming request for v1.
*
*   INPUTS
*
*       list_len                Length of the list of objects.
*       *list                   List of objects to be processed.
*       *index                  If an error occurred, this variable
*                               indicates the index of the object which
*                               caused the error. This index is relative
*                               to the original packet.
*       snmp_sm                 Security model.
*       security_level          Security level.
*       *security_name          Security name.
*       *context_name           Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       SNMP_NOSUCHNAME         No such instance.
*       SNMP_BADVALUE           The value being trying to set is not
*                               valid.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
STATUS MibRequest(UINT32 list_len, snmp_object_t *list, UINT16 *index,
                  UINT32 snmp_sm, UINT8 security_level,
                  UINT8 *security_name, UINT8 *context_name)
{
    /* Status of the request. */
    STATUS                  status = NU_SUCCESS;

    /* This variable is used to remember the last object that was
     * retrieved. This helps in SNMP walks which will retrieve the
     * next instance. Therefore, we will not need to do a search
     * of the MIB table again.
     */
    static UINT16           last_index = 0;

    /* If this is a set request, then call the function which is
     * responsible for processing that request.
     */
    if(list[0].Request == SNMP_PDU_SET)
    {
        /* Execute the set request. All objects in the list
         * are set through this function call.
         */
        status = MIB_Set_Request(list, list_len, index,
                                 &last_index, snmp_sm,
                                 security_level, security_name,
                                 context_name);
    }

    /* Otherwise this is one of the get requests (either a GET request
     * or a GET-NEXT request.
     */
    else
    {
        /* Loop through all the instance in the list and get values for
         * each, retrieving one at a time.
         */
        for((*index) = 0; ((*index) < list_len) && (status == NU_SUCCESS);
            (*index)++)
        {
            /* SNMPv1 does not support 64-bit counters. This loop is for
             * the case where we are doing a GET-NEXT and we encounter
             * a 64-bit counter. We will ignore this instance and find
             * the next instance which is not 64-bit.
             */
            do
            {
                /* Make the get request. */
                status = Request(&list[(*index)], &last_index, snmp_sm,
                                security_level, security_name,
                                context_name);

            }while((list[(*index)].Request == SNMP_PDU_NEXT) &&
                   (status == NU_SUCCESS) &&
                   (list[(*index)].Type == SNMP_COUNTER64));

            /* Make sure that we do not have an instance which has a
             * 64-bit data type. If we do, then we simply return no
             * such name.
             */
            if((status == NU_SUCCESS) &&
               (list[(*index)].Type == SNMP_COUNTER64))
            {
                status = SNMP_NOSUCHNAME;
            }
        }
    }

    /* If an error was encountered, then convert this error to SNMPv1
     * error.
     */
    if(status != NU_SUCCESS)
    {
        switch (status)
        {

        case SNMP_AUTHORIZATIONERROR:

            /* RFC 3584 section 4.4 states:  Whenever the SNMPv2
             * error-status value of authorizationError is translated to
             * an SNMPv1 error-status value of noSuchName, the value of
             * snmpInBadCommunityUses MUST be incremented.
             */
            (AgentStatistics())->InBadCommunityNames++;

        case SNMP_NOSUCHNAME:
        case SNMP_NOACCESS:
        case SNMP_NOTWRITABLE:
        case SNMP_NOCREATION:
        case SNMP_INCONSISTANTNAME:
        case SNMP_NOSUCHOBJECT:
        case SNMP_NOSUCHINSTANCE:
        case SNMP_ENDOFMIBVIEW:

            /* Update the error code and increment the appropriate
             * counter.
             */
            status = SNMP_NOSUCHNAME;
            SnmpStat.OutNoSuchNames++;
            break;

        case SNMP_BADVALUE:
        case SNMP_WRONGVALUE:
        case SNMP_WRONGENCODING:
        case SNMP_WRONGTYPE:
        case SNMP_WRONGLENGTH:
        case SNMP_INCONSISTANTVALUE:

            /* Update the error code and increment the appropriate
             * counter.
             */
            status = SNMP_BADVALUE;
            SnmpStat.OutBadValues++;
            break;

        case SNMP_GENERROR:
        case SNMP_RESOURCEUNAVAILABLE:
        case SNMP_COMMITFAILED:
        case SNMP_UNDOFAILED:
        default:

            /* Update the error code and increment the appropriate
             * counter.
             */
            status = SNMP_GENERROR;
            SnmpStat.OutGenErrs++;
            break;
         }
    }

    /* If the processing was successful, then the error index will be
     * zero.
     */
    if(status == NU_SUCCESS)
        (*index) = 0;

    return (status);

} /* MibRequest */

#endif /* (INCLUDE_SNMPv1 == NU_TRUE) */

/************************************************************************
*
*   FUNCTION
*
*       Request
*
*   DESCRIPTION
*
*       This function calls the proper function according to the object
*       passed.
*
*   INPUTS
*
*       *obj            Pointer to the object.
*       *lastindex      This variable contains index in the MIB table of
*                       the last retrieved object. This helps us quickly
*                       determine the index if we are doing sequential
*                       access. For example, when we are doing a walk.
*       security_model  Security model.
*       security_level  Security level.
*       *security_name  Security name.
*       *context_name   Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS      Operation successful.
*       SNMP_GENERROR   General error.
*
*************************************************************************/
STATUS Request(snmp_object_t *obj, UINT16 *lastindex,
               UINT32 security_model, UINT8 security_level,
               UINT8 *security_name, UINT8 *context_name)
{
    STATUS          status;
    UINT8           view_type = VACM_READ_VIEW;
    UINT32           notify_read = 0;

    /* If there are no objects that have been registered, then we cannot
     * possibly serve any request.
     */
    status = NU_Obtain_Semaphore(&SNMP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        if (MibRoot.Table != NU_NULL)
        {
            /* Check the type of request and make appropriate call */
            switch (obj->Request)
            {

            case SNMP_PDU_TRAP_V2:
            case SNMP_PDU_TRAP_V1:

                /* These are get request for instances that will be part of
                 * a notification that is being generated.
                 */

                /* We will use the notification MIB view for these requests.
                 */
                view_type = VACM_NOTIFY_VIEW;

                /* Accessible-for-notify flag set. This request is for trap */
                notify_read = 1;

                /* Change the request to a GET-Request, this will enable us to
                 * retrieve the value of the instance.
                 */
                obj->Request = SNMP_PDU_GET;

                break;

            case SNMP_PDU_GET_SET:

                /* This is a GET-Request in the context of a SET request, we
                 * will use the write view. From here-on the request will
                 * also be know as a GET-Request.
                 */
                obj->Request = SNMP_PDU_GET;

            case SNMP_PDU_COMMIT:

                /* For the commit case as well, we use the write view. */
                view_type = VACM_WRITE_VIEW;
                break;
            }

            switch (obj->Request)
            {

            case SNMP_PDU_COMMIT:
            case SNMP_PDU_GET:

                /* Commit/Get an instance according to the parameters
                 * specified.
                 */
                status = MIB_Get_Instance(obj, lastindex, security_model,
                                          security_level, security_name,
                                          context_name, view_type, 
                                          notify_read);
                break;

            case SNMP_PDU_UNDO:
            case SNMP_PDU_SET:

                /* Sets a value as specified in the object. This case is
                 * triggered when we get a set request as well as when
                 * we are undoing a set request. Note that in the case
                 * of undo we merely set the last committed value.
                 */
                status = MIB_Set_Instance(obj, lastindex,
                            security_model, security_level,
                            security_name, context_name);

                break;

            case SNMP_PDU_BULK:
            case SNMP_PDU_NEXT:

                /* Get the next greater instance from MIB bank. */
                status =  MIB_Get_Next_Instance(obj, lastindex,
                            security_model, security_level,
                            security_name, context_name, notify_read);

                break;

            default:

                /* We do not know of this request return General Error. */
                status = SNMP_GENERROR;
                break;

            }
        }

        /* We did not find any object in the MIB table. One can argue that
         * we should therefore return noSuchObject here. However, we should
         * have had a few objects in the MIB table. For example sysUpTime is
         * always to be present. If it is not, this means that SNMP was
         * not initialized properly. The error therefore, is much greater
         * than not simply finding an object.
         */
        else
        {
            status = SNMP_GENERROR;
        }

        if (NU_Release_Semaphore(&SNMP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to release the semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("SNMP: Failed to grab the semaphore", NERR_SEVERE,
                        __FILE__, __LINE__);

        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Request */

/************************************************************************
*
*   FUNCTION
*
*       MibSimple
*
*   DESCRIPTION
*
*       This function determines if the object is valid.
*
*   INPUTS
*
*       *Obj                    A pointer to the object.
*       IdLen                   The length of the object ID.
*
*   OUTPUTS
*
*       NU_TRUE                 The object is valid.
*       NU_FALSE                The object is not valid.
*
*************************************************************************/
BOOLEAN MibSimple(snmp_object_t *Obj, UINT16 IdLen)
{
    BOOLEAN    success = NU_TRUE;

    /* The request is a GET */
    if (Obj->Request != SNMP_PDU_NEXT && Obj->Request != SNMP_PDU_BULK)
    {
        /* Check that the length of the OID is valid */
        if ( ((Obj->IdLen == (UINT32)(IdLen + 1)) && (Obj->Id[IdLen] == 0))
            || (Obj->IdLen == ((UINT32)IdLen)))
            success = NU_TRUE;
        else
            success = NU_FALSE;
    }

    /* Otherwise, if the length of the OID equals the length passed in,
     * set the next parameter in the OID to 0 and increment the length.
     */
    else if (Obj->IdLen == IdLen)
    {
        Obj->Id[IdLen] = 0;
        Obj->IdLen++;
    }

    else
        success = NU_FALSE;

    return (success);

} /* MibSimple */

/************************************************************************
*
*   FUNCTION
*
*       MibCmpObjId
*
*   DESCRIPTION
*
*       This function determines whether two objects are the same.
*
*   INPUTS
*
*       *ObjId1                 A pointer to the first object.
*       ObjIdLen1               The length of the first object.
*       *ObjId2                 A pointer to the second object.
*       ObjLen2                 The length of the second object.
*
*   OUTPUTS
*
*       0                       Passed objects are equal.
*       -1                      Object 1 has a lesser length than object
*                               2. All sub-identifiers compared were found
*                               to be equal.
*       1                       Object 2 has a lesser length than object
*                               1. All sub-identifiers compared were found
*                               to be equal.
*       -2                      Object 1 is less than object 2.
*       2                       Object 1 is greater than object 2.
*
*************************************************************************/
INT32 MibCmpObjId(const HUGE UINT32 *ObjId1, UINT32 ObjIdLen1,
                  const HUGE UINT32 *ObjId2, UINT32 ObjIdLen2)
{
    INT32   result;

    while ( (ObjIdLen1 > 0) && (ObjIdLen2 > 0) && (*ObjId1 == *ObjId2) )
    {
        ObjIdLen1--;
        ObjIdLen2--;
        ObjId1++;
        ObjId2++;
    }

    if (!ObjIdLen1 && !ObjIdLen2)
        result = 0;

    else if (!ObjIdLen1)
        result = -1;

    else if (!ObjIdLen2)
        result = 1;

    else if (*ObjId1 < *ObjId2)
        result = -2;

    else
        result = 2;

    return (result);

} /* MibCmpObjId */

/************************************************************************
*
*   FUNCTION
*
*       MibObjectFind
*
*   DESCRIPTION
*
*       This function finds an object in the MIB list based on the
*       object ID.
*
*   INPUTS
*
*       *Id                     A pointer to the ID of the object.
*       IdLen                   The length of the ID of the object.
*       *cmp                    The relation of this object to the
*                               previous object.
*
*   OUTPUTS
*
*       0                       The object is not in the list.
*       mindex                  The index of the object in the list.
*
*************************************************************************/
INT32 MibObjectFind(const UINT32 *Id, INT32 IdLen, INT32 *cmp)
{
    INT32   first = 0;
    INT32   last = MibRoot.Count - 1;
    INT32   mindex = -1;

    *cmp = 2;

    if (last >= 0)
    {
        while ( (first < last) && (*cmp != 0) && (*cmp != -1))
        {
            /* Start in the middle of the table */
            mindex = (first + last)/2;

            /* Determine whether the target is in the upper or lower half
             * of the list.
             */
            *cmp = MibCmpObjId(MibRoot.Table[mindex]->Id,
                               MibRoot.Table[mindex]->IdLen, Id,
                               (UINT16)IdLen);

            switch (*cmp)
            {
            /* The target is in the upper half of the list */
            case -2:
                first = mindex + 1;
                break;

            /* We found the target */
            case -1:
            case 0:
                break;

            /* The target is in the lower half of the list. */
            case 1:
            case 2:
                last = mindex;
                break;
            }
        }

        /* If we found the object */
        if ( (first != -1) && (*cmp != 0) && (*cmp != -1))
        {
            /* Get the index of the first object */
            mindex = first;

            /* Return the location of the object */
            *cmp = MibCmpObjId(MibRoot.Table[mindex]->Id,
                               MibRoot.Table[mindex]->IdLen,
                               Id, (UINT16)IdLen);
        }
    }
    else
        mindex = 0;

    return (mindex);

} /* MibObjectFind */

/************************************************************************
*
*   FUNCTION
*
*       MibObjectInsert
*
*   DESCRIPTION
*
*       This function inserts an object into the MIB table.
*
*   INPUTS
*
*       *Object                 A pointer to the object to insert.
*
*   OUTPUTS
*
*       NU_TRUE                 The object was successfully inserted.
*       NU_FALSE                The object was not inserted.
*
*************************************************************************/
BOOLEAN MibObjectInsert(mib_object_t *Object)
{
    BOOLEAN            success = NU_TRUE;
    INT32           mindex, cmp, i;

    MibRoot.Table[MibRoot.Count] = Object;
    MibRoot.Count++;

    mindex = MibObjectFind(Object->Id, (INT32)Object->IdLen, &cmp);

    if ( (cmp == 0) && (mindex != MibRoot.Count-1) )
    {
        MibRoot.Count--;
        success = NU_FALSE;
    }

    else
    {
        for (i = MibRoot.Count-1; i > mindex; i--)
            MibRoot.Table[i] = MibRoot.Table[i-1];

        MibRoot.Table[mindex] = Object;
    }

    return (success);

} /* MibObjectInsert */

/*************************************************************************
*
* FUNCTION
*
*       SNMP_Mib_Register
*
* DESCRIPTION
*
*       This function is used to register MIBs with Nucleus SNMP.
*
* INPUTS
*
*       *mib                    Pointer to the array of MIB Elements.
*       mibsize                 Size of array pointed by 'mib'.
*
* OUTPUTS
*
*       NU_SUCCESS              MIB registeration successfull.
*       SNMP_NO_MEMORY          Memory allocation failed
*       SNMP_BAD_PARAMETER      Atleast one of the MIB objects is 
*                               already registered
*       NU_INVALID_SEMAPHORE    Invalid semaphore pointer
*       NU_INVALID_SUSPEND      Suspension requested from non-task
*
*************************************************************************/
STATUS SNMP_Mib_Register(mib_element_t *mib, UINT16 mibsize)
{
    mib_object_t**          table;
    mib_object_t**          temp_table;
    STATUS                  status;
    UINT16                  i;

    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    status = NU_Obtain_Semaphore(&SNMP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Allocate the memory. */
        status = NU_Allocate_Memory(&System_Memory, (VOID **)(&table),
                                    (UNSIGNED)((MibRoot.Size + mibsize) *
                                    sizeof(mib_object_t *)),
                                    NU_NO_SUSPEND);

        /* If memory allocated successfully. */
        if (status == NU_SUCCESS)
        {
            table = TLS_Normalize_Ptr(table);

            /* Clear out the memory allocated. */
            UTL_Zero(table, (UINT32)((MibRoot.Size + mibsize) *
                                        sizeof(mib_object_t *)));

            /* Copy the previous MIB tree.  */
            NU_BLOCK_COPY(table, MibRoot.Table, (UNSIGNED)((MibRoot.Size *
                                                sizeof(mib_object_t *))));

            /* Update the size of MIb tree. */
            MibRoot.Size = MibRoot.Size + mibsize;

            /* Keep the old handle to the tree, we will use to deallocate
             * memory if successful and to restore old value if failure.
             */
            temp_table = MibRoot.Table;

            /* Update the MIB table. */
            MibRoot.Table = table;

            /* Loop to insert the MIB objects. */
            for( i = 0; i < mibsize; i++)
            {
                /* Insert the object. */
                if (!MibObjectInsert(&(mib[i])))
                {
                    /* If we failed to insert the MIB object. */

                    /* Deallocate the new MIB table. */
                    if (NU_Deallocate_Memory(MibRoot.Table) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Restore the MIB table size. */
                    MibRoot.Size -= mibsize;

                    /* Get the old handle to the MIB tree. */
                    MibRoot.Table = temp_table;

                    /* Return error code. */
                    status = SNMP_BAD_PARAMETER;

                    /* Break through the loop. */
                    break;
                }
            }
             
            /* If the procedure was successful and we had old MIB tree as
             * NON-NULL the deallocate the memory.
             */
            if ((status == NU_SUCCESS) && (temp_table))
            {
                /* Deallocate the memory. */
                if (NU_Deallocate_Memory(temp_table) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
        else
        {
            status = SNMP_NO_MEMORY;
        }
        /* Release the semaphore. */
        if (NU_Release_Semaphore(&SNMP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to release the semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    NU_USER_MODE();

    /* Return success or error code. */
    return (status);

} /* SNMP_Mib_Register */


/*************************************************************************
*
* FUNCTION
*
*       SNMP_Find_Object
*
* DESCRIPTION
*
*       This function is used to find the passed Id in the passed 
*       MIB element.
*
* INPUTS
*
*       *mib                    Pointer to the array of MIB Elements.
*       mibsize                 Size of array pointed by 'mib'.
*       *Id                     Id to be searched.
*       Idlen                   Length of the passed Id.
*
* OUTPUTS
*
*       NU_SUCCESS              if Id found.
*       SNMP_INVAL              otherwise.
*
*************************************************************************/
STATUS SNMP_Find_Object(mib_element_t *mib, UINT32 *Id, UINT16 mibsize, 
                        UINT32 Idlen)
{
    UINT16                  i;

    for (i = 0; i < mibsize; i++)
    {
        if (MibCmpObjId(mib[i].Id, mib[i].IdLen, Id, Idlen) == 0)
            return (NU_SUCCESS);
    }

    return (NU_INVAL);

} /* SNMP_Find_Object */

/*************************************************************************
*
* FUNCTION
*
*       SNMP_Mib_Unregister
*
* DESCRIPTION
*
*       This function is used to unregister MIBs with Nucleus SNMP.
*
* INPUTS
*
*       *mib                    Pointer to the array of MIB Elements.
*       mibsize                 Size of array pointed by 'mib'.
*
* OUTPUTS
*
*       NU_SUCCESS              if processing succeeded.
*       SNMP_NO_MEMORY          Memory allocation failed.
*       SNMP_ERROR              Failed to reinsert old mib objects. 
*                               does not exist. 
*       NU_INVALID_SEMAPHORE    Invalid semaphore pointer
*       NU_INVALID_SUSPEND      Suspension requested from non-task
*
*************************************************************************/
STATUS SNMP_Mib_Unregister(mib_element_t *mib, UINT16 mibsize)
{
    mib_object_t**          table;
    mib_object_t**          temp_table;
    STATUS                  status;
    UINT16                  i;
    UINT32                  original_size = MibRoot.Size;
    UINT32                  new_size = MibRoot.Size - mibsize;
    UINT32                  original_count = MibRoot.Count;

    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    status = NU_Obtain_Semaphore(&SNMP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Allocate the memory. */
        status = NU_Allocate_Memory(&System_Memory, (VOID **)(&table),
                                    (UNSIGNED)((original_size) *
                                    sizeof(mib_object_t *)),
                                    NU_NO_SUSPEND);

        /* If memory allocated successfully. */
        if (status == NU_SUCCESS)
        {
            table = TLS_Normalize_Ptr(table);

            /* Clear out the memory allocated. */
            UTL_Zero(table, (UINT32)((original_size) *
                                        sizeof(mib_object_t *)));

            /* Copy the previous MIB tree.  */
            NU_BLOCK_COPY(table, MibRoot.Table, (UNSIGNED)((MibRoot.Size *
                                                sizeof(mib_object_t *))));

            /* Keep the old handle to the tree, we will use to deallocate
             * memory if successful and to restore old value if failure.
             */
            temp_table = table;

            /* Deallocate the original table */
            status = NU_Deallocate_Memory(MibRoot.Table);

            if (status == NU_SUCCESS)
            {
                /* Allocate the table with new size */
                status = NU_Allocate_Memory(&System_Memory, 
                                    (VOID **)(&MibRoot.Table),
                                    (UNSIGNED)((new_size) *
                                    sizeof(mib_object_t *)),
                                    NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Clear out the newly allocated table */
                    UTL_Zero(MibRoot.Table, (UINT32)((new_size) *
                                        sizeof(mib_object_t *)));

                    /* Update the MIB table. */
                    MibRoot.Size = 0;
                    MibRoot.Count = 0;

                    /* Loop to insert the MIB objects. */
                    for( i = 0; i < original_size; i++)
                    {
                        /* Insert the object. */
                        if (SNMP_Find_Object(mib, table[i]->Id, mibsize,
                                        table[i]->IdLen) == NU_INVAL)
                        {
                            if (!MibObjectInsert((table[i])))
                            {
                                /* If we failed to insert the MIB object. */

                                /* Deallocate the new MIB table. */
                                if (NU_Deallocate_Memory(MibRoot.Table)
                                            != NU_SUCCESS)
                                {
                                    NLOG_Error_Log(
                                        "SNMP: Failed to deallocate memory",
                                         NERR_SEVERE, __FILE__, __LINE__);
                                }

                                /* Restore the MIB table size. */
                                MibRoot.Size = original_size;

                                /* Restore the original count value */
                                MibRoot.Count = original_count;

                                /* Get the old handle to the MIB tree. */
                                MibRoot.Table = temp_table;

                                /* Return error code. */
                                status = SNMP_ERROR;

                                /* Break through the loop. */
                                break;
                            }
                        }
                    }
                }
                else
                {
                    NLOG_Error_Log("SNMP: Failed to allocate memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                    status = SNMP_NO_MEMORY;
                }
            }
            else
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                status = SNMP_NO_MEMORY;
            }

            /* If the procedure was successful and we had old MIB tree as
             * NON-NULL then deallocate the memory.
             */
            if ((status == NU_SUCCESS) && (table))
            {
                /* Set the new table size */
                MibRoot.Size = new_size;

                /* Deallocate the memory. */
                if (NU_Deallocate_Memory(table) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&SNMP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to release the semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    NU_USER_MODE();

    /* Return success or error code. */
    return (status);

} /* SNMP_Mib_Unregister */

/*************************************************************************
*
* FUNCTION
*
*       SNMP_Get_Bulk
*
* DESCRIPTION
*
*       This function is used to get bulk responses against a single
*       object.
*
* INPUTS
*
*       *obj                    SNMP Object*
*
*       snmp_get_function       Pointer to getter function.
*
* OUTPUTS
*
*       NU_SUCCESS if processing succeeded.
*       SNMP_NOSUCHNAME otherwise.
*
*************************************************************************/
UINT16 SNMP_Get_Bulk(snmp_object_t *obj,
                     SNMP_GET_FUNCTION snmp_get_function)
{
    UINT16          status = NU_SUCCESS;
    snmp_object_t   dummy_object;
    UINT32          max_repetitions;
    UINT32          index;

    /* Get the no of repetitions for this particular object. */
    max_repetitions = obj[0].Syntax.LngUns;

    /* Get the object. */
    dummy_object= obj[0];

    /* Loop for max number of repetitions. */
    for(index = 0;
                  (index < max_repetitions) && (status == SNMP_NOERROR);
                  index++)
    {
        /* Clear the temporary object where, the instance will be
           retrieved. */
        UTL_Zero(dummy_object.Syntax.BufChr, SNMP_SIZE_BUFCHR);

        /* Get the next object. */
        if(snmp_get_function((&dummy_object), 0) != NU_SUCCESS)
        {
            /* If no next object was available and this is the first loop
               (no object is found) return no such  name. This will tell
               the MIB engine not to look at the SNMP object list. If this
               is not the first loop (at least one instance has been
               retrieved), return success.The MIB engine will retrieve
               the values from the SNMP object list. */
            if(index == 0)
            {
                status = SNMP_NOSUCHNAME;
            }

            break;
        }

        /* The temporary object should indicate that this is a GET-BULK
           request. */
        dummy_object.Request = SNMP_PDU_BULK;

        /* Put the retrieved instance in to the SNMP object list. */
        obj[index] = dummy_object;
    }

    /* Return status. */
    return (status);

} /* SNMP_Get_Bulk */


