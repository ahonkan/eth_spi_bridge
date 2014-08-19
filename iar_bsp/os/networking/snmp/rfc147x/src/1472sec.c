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
*       1472sec.c                                                
*
*   DESCRIPTION
*
*       This file contains MIB implementation of SEC as defined in
*       RFC1472.
*
*   DATA STRUCTURES
*
*       PapObjectId
*       ChapObjectId
*
*   FUNCTIONS
*
*       NextCfgEntry
*       NextSecEntry
*       Get_pppSecurityConfigEntry
*       Set_pppSecurityConfigEntry
*       pppSecurityConfigEntry
*       Get_pppSecuritySecretsEntry
*       Set_pppSecuritySecretsEntry
*
*   DEPENDENCIES
*
*       snmp_g.h
*       snmp.h
*       snmp_api.h
*
*************************************************************************/
#include "networking/snmp_g.h"
#include "networking/snmp.h"
#include "networking/snmp_api.h"

#if (INCLUDE_SEC_MIB == NU_TRUE)

/* Object identifiers for the PAP and CHAP authentication protocols. */
INT32 PapObjectId[] = {OID_SEC_PAP};
INT32 ChapObjectId[] = {OID_SEC_CHAP};

/************************************************************************
*
*   FUNCTION
*
*       NextCfgEntry
*
*   DESCRIPTION
*
*       Sets the identifier of the next entry in the security configuration
*       table into the given SNMP object structure.
*
*   INPUTS
*
*       *obj                    Pointer to the object identifier to be
*                               set.
*       idlen                   The index of the variable to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful.
*       SNMP_ERROR              failure.
*
*************************************************************************/
STATUS NextCfgEntry(snmp_object_t *obj, UINT16 idlen)
{
    /* Status to return success or error code. */
    STATUS status;

    /* Get the next entry index based on link id and preference level. */
    status = PMSC_GetNextEntry((INT32*)&obj->Id[idlen],
                               (INT32*)&obj->Id[idlen+1]);

    if (status == NU_SUCCESS)
    {
        /* Adjust the length for the new indices. */
        obj->IdLen = (INT32)(idlen + 2);
    }

    /* PMSC_GetNextEntry() failed return error code. */
    else
    {
        /* Return error code. */
        status = SNMP_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* NextCfgEntry */

/************************************************************************
*
*   FUNCTION
*
*       NextSecEntry
*
*   DESCRIPTION
*
*       Sets the identifier of the next entry in the security secrets
*       table into the given SNMP object structure.
*
*   INPUTS
*
*       *obj                    Pointer to the object identifier to be
*                               set.
*       idlen                   The index of the variable to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful.
*       SNMP_ERROR              failure.
*
*************************************************************************/
STATUS NextSecEntry(snmp_object_t *obj, UINT16 idlen)
{
    STATUS status;

    /* Get the next entry index based on link id and user id. */
    status = PMSS_GetNextEntry((INT32*)&obj->Id[idlen],
                               (INT32*)&obj->Id[idlen+1]);

    if (status == NU_SUCCESS)
    {
        /* Write the new index into the object. */
        obj->IdLen = (INT32)(idlen + 2);
    }

    /* Call to PMSS_GetNextEntry failed, return error code. */
    else
    {
        /* Return error code. */
        status = SNMP_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* NextSecEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppSecurityConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppSecurityConfigTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
UINT16 Get_pppSecurityConfigEntry(snmp_object_t *obj, UINT8 getflag)
{
    INT32               link;
    INT32               pref;
    INT32               auth;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    if (!getflag)
    {
        /* Set the next table entry index and fall through to get it. */
        if (NextCfgEntry(obj, 12) != NU_SUCCESS)
            status = SNMP_NOSUCHINSTANCE;
    }

    if (status == NU_SUCCESS)
    {
        /* Get indices. */
        link = obj->Id[12];
        pref = obj->Id[13];

        /* If we do not have valid indices then return error code. */
        if ((link < 0) || (pref < 0))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If we have valid indices then proceed. */
        else
        {
            /* Switch to appropriate attribute. */
            switch (obj->Id[11])
            {
            case 1:                         /* pppSecurityConfigLink */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSC_GetSecurityConfigLink(link, pref,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 2:                         /* pppSecurityConfig
                                             * Preference
                                             */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSC_GetSecurityConfigPreference(link, pref,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 3:                         /* pppSecurityConfigProtocol */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSC_GetSecurityConfigProtocol(link, pref, &auth) ==
                                                               NU_SUCCESS)
                {
                    if ((auth == 1) || (auth == 2))
                    {
                        if (auth == 1)
                        {
                            NU_BLOCK_COPY(obj->Syntax.BufInt,
                                        ChapObjectId, PROTOCOL_OBJ_BYTES);
                        }
                        else /* if (auth == 2) */
                        {
                            NU_BLOCK_COPY(obj->Syntax.BufInt, PapObjectId,
                                                      PROTOCOL_OBJ_BYTES);
                        }

                        /* Setting Length. */
                        obj->SyntaxLen = PROTOCOL_OBJ_SIZE;
                    }

                    else
                    {
                        status = SNMP_GENERROR;
                    }
                }

                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 4:                         /* pppSecurityConfigStatus */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSC_GetSecurityConfigStatus(link, pref,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            default:

                /* We have reached at end of table. */
                status = SNMP_NOSUCHNAME;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_pppSecurityConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_pppSecurityConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       pppSecurityConfigTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*
*************************************************************************/
UINT16 Set_pppSecurityConfigEntry(snmp_object_t *obj)
{
    INT32               link;
    INT32               pref;
    INT32               auth = 0;
    UINT16              status = SNMP_NOERROR;

    /* Getting indices. */
    link = obj->Id[12];
    pref = obj->Id[13];

    /* If we have invalid indices then return error code. */
    if ((link < 0) || (pref < 0))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    else
    {
        /* Switch to appropriate attribute. */
        switch (obj->Id[11])
        {
        case 1:                         /* pppSecurityConfigLink */

            /* Call the PM function to set the value into the table. */
            if (obj->Syntax.LngInt >= 0)
            {
                if (PMSC_SetSecurityConfigLink(link, pref,
                                        obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        case 2:                         /* pppSecurityConfigPreference */

            /* Call the PM function to set the value into the table. */
            if (obj->Syntax.LngInt >= 0)
            {
                if (PMSC_SetSecurityConfigPreference(link, pref,
                                        obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        case 3:                         /* pppSecurityConfigProtocol */

            /* Call the PM function to set the value into the table. */
            if (obj->SyntaxLen == PROTOCOL_OBJ_SIZE)
            {
                if (memcmp(ChapObjectId, obj->Syntax.BufInt,
                                                 PROTOCOL_OBJ_BYTES) == 0)
                {
                    auth = 1;
                }

                else if (memcmp(PapObjectId, obj->Syntax.BufInt,
                                                 PROTOCOL_OBJ_BYTES) == 0)
                {
                    auth = 2;
                }

                else
                {
                    status = SNMP_WRONGVALUE;
                }

                if ((status == SNMP_NOERROR) &&
                    (PMSC_SetSecurityConfigProtocol(link, pref, auth) !=
                                                              NU_SUCCESS))
                {
                    status = SNMP_NOSUCHINSTANCE;
             
                }
            }

            else
            {
                status = SNMP_WRONGLENGTH;
            }

            break;

        case 4:                         /* pppSecurityConfigStatus */

            /* Call the PM function to set the value into the table. */
            if ((obj->Syntax.LngInt == 1) || (obj->Syntax.LngInt == 2))
            {
                if (PMSC_SetSecurityConfigStatus(link, pref,
                                        obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        default:

            /* We have reached at end of table. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_pppSecurityConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppSecurityConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       pppSecurityConfigTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
UINT16 pppSecurityConfigEntry(snmp_object_t *obj, UINT16 idlen, void *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Status for returning success or error code. */
    UINT16        status = SNMP_NOERROR;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    /* Identifying request type and taking appropriate actions. */
    switch(obj->Request)
    {
    case SNMP_PDU_GET:      /* Get request. */
        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:     /* Get next request. */

        /* Processing get operations. */
        status = Get_pppSecurityConfigEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppSecurityConfigEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        status = Set_pppSecurityConfigEntry(obj);
        break;

    case SNMP_PDU_COMMIT:   /* Commit request. */

        /* We always have successful set. */
        status = SNMP_NOERROR;
        break;

    default:            /* Invalid request. */

        /* Processing of set operations. */
        status = SNMP_GENERROR;
        break;
    }

    /* Return status. */
    return (status);
} /* pppSecurityConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppSecuritySecretsEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppSecuritySecretsTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
UINT16 Get_pppSecuritySecretsEntry(snmp_object_t *obj, UINT8 getflag)
{
    INT32               link;
    INT32               id;
    INT32               auth;
    UINT16              status = SNMP_NOERROR;

    /* If we are handling GETNEXT request. */
    if (!getflag)
    {
        /* Set the next table entry index and fall through to get it. */
        if (NextSecEntry(obj, 12) != NU_SUCCESS)
        {
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    if (status == SNMP_NOERROR)
    {
        /* Get indices. */
        link = obj->Id[12];
        id = obj->Id[13];

        /* If we have invalid indices then return error code. */
        if ((link < 0) || (id < 0))
        {
            status = SNMP_NOSUCHINSTANCE;
        }

        else
        {
            /* Switch to appropriate attribute. */
            switch (obj->Id[11])
            {
            case 1:                         /* pppSecuritySecretsLink */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsLink(link, id,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 2:                             /* pppSecuritySecretsIdIndex */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsIdIndex(link, id,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 3:                             /* pppSecuritySecretsDirection */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsDirection(link, id,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 4:                         /* pppSecuritySecretsProtocol */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsProtocol(link, id, &auth) ==
                                                               NU_SUCCESS)
                {
                    if ((auth == 1) || (auth == 2))
                    {
                        if (auth == 1)
                        {
                            NU_BLOCK_COPY(obj->Syntax.BufInt,
                                        ChapObjectId, PROTOCOL_OBJ_BYTES);
                        }

                        else /* if (auth == 2) */
                        {
                            NU_BLOCK_COPY(obj->Syntax.BufInt,
                                         PapObjectId, PROTOCOL_OBJ_BYTES);
                        }

                        obj->SyntaxLen = PROTOCOL_OBJ_SIZE;
                    }

                    else
                    {
                        status = SNMP_GENERROR;
                    }
                }

                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 5:                         /* pppSecuritySecretsIdentity */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsIdentity(link, id,
                                        obj->Syntax.BufChr) == NU_SUCCESS)
                {
                    obj->SyntaxLen = strlen((CHAR*)obj->Syntax.BufChr);
                }

                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 6:                         /* pppSecuritySecretsSecret */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsSecret(link, id,
                                        obj->Syntax.BufChr) == NU_SUCCESS)
                {
                    obj->SyntaxLen = strlen((CHAR*)obj->Syntax.BufChr);
                }

                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 7:                         /* pppSecuritySecretsStatus */

                /* Assign the variable based on information in the entry.
                 */
                if (PMSS_GetSecuritySecretsStatus(link, id,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            default:

                /* We have reached at end of the table. */
                status = SNMP_NOSUCHNAME;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_pppSecuritySecretsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_pppSecuritySecretsEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       pppSecuritySecretsTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*
*************************************************************************/
UINT16 Set_pppSecuritySecretsEntry(snmp_object_t *obj)
{
    INT32               link;
    INT32               id;
    INT32               auth = 0;
    UINT16              status = SNMP_NOERROR;

    /* Getting indices. */
    link = obj->Id[12];
    id = obj->Id[13];

    /* If we have invalid indices then return error code. */
    if ((link < 0) || (id < 0))
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    else
    {
        /* Switch to appropriate attribute. */
        switch (obj->Id[11])
        {
        case 1:                             /* pppSecuritySecretsLink */
        case 2:                             /* pppSecuritySecretsIdIndex */

            status = SNMP_READONLY;
            break;

        case 3:                             /* pppSecuritySecretsDirection */

            /* Call the PM function to set the value into the table. */
            if ((obj->Syntax.LngInt == 1) || (obj->Syntax.LngInt == 2))
            {
                if (PMSS_SetSecuritySecretsDirection(link, id,
                                        obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        case 4:                         /* pppSecuritySecretsProtocol */

            /* Call the PM function to set the value into the table. */
            if (obj->SyntaxLen == PROTOCOL_OBJ_SIZE)
            {
                if (memcmp(ChapObjectId, obj->Syntax.BufInt,
                                                 PROTOCOL_OBJ_BYTES) == 0)
                {
                    auth = 1;
                }
                else if (memcmp(PapObjectId, obj->Syntax.BufInt,
                                                 PROTOCOL_OBJ_BYTES) == 0)
                {
                    auth = 2;
                }

                else
                {
                    status = SNMP_WRONGVALUE;
                }

                if (status == SNMP_NOERROR)
                {
                    if (PMSS_SetSecuritySecretsProtocol(link, id, auth) !=
                                                               NU_SUCCESS)
                    {
                        status = SNMP_NOSUCHINSTANCE;
                    }
                }
            }

            else
            {
                status = SNMP_WRONGLENGTH;
            }

            break;

        case 5:                         /* pppSecuritySecretsIdentity */

            /* Call the PM function to set the value into the table. */
            if (obj->SyntaxLen <= 255)
            {
                if (PMSS_SetSecuritySecretsIdentity(link, id,
                                        obj->Syntax.BufChr) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGLENGTH;
            }

            break;

        case 6:                         /* pppSecuritySecretsSecret */

            /* Call the PM function to set the value into the table. */
            if (obj->SyntaxLen <= 255)
            {
                if (PMSS_SetSecuritySecretsSecret(link, id,
                                        obj->Syntax.BufChr) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGLENGTH;
            }

            break;

        case 7:                         /* pppSecuritySecretsStatus */

            /* Call the PM function to set the value into the device. */
            if ((obj->Syntax.LngInt == 1) || (obj->Syntax.LngInt == 2))
            {
                if (PMSS_SetSecuritySecretsStatus(link, id,
                                        obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }

            else
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        default:

            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_pppSecuritySecretsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppSecuritySecretsEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       pppSecuritySecretsTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
UINT16 pppSecuritySecretsEntry(snmp_object_t *obj, UINT16 idlen, void *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Status for returning success or error code. */
    UINT16        status = SNMP_NOERROR;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    /* Identifying request type and taking appropriate actions. */
    switch(obj->Request)
    {
    case SNMP_PDU_GET:      /* Get request. */
        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:     /* Get next request. */

        /* Processing get operations. */
        status = Get_pppSecuritySecretsEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppSecuritySecretsEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        status = Set_pppSecuritySecretsEntry(obj);
        break;

    case SNMP_PDU_COMMIT:   /* Commit request. */

        /* We always have successful set. */
        status = SNMP_NOERROR;
        break;

    default:            /* Invalid request. */

        /* Processing of set operations. */
        status = SNMP_GENERROR;
        break;
    }

    /* Return status. */
    return (status);
}

#endif /* INCLUDE_SEC_MIB */

