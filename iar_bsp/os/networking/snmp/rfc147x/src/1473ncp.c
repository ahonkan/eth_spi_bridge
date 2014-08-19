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
*       This file contains MIB implementation of NCP as defined in
*       RFC1473.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Get_pppIpEntry
*       pppIpEntry
*       Get_pppIpConfigEntry
*       Set_pppIpConfigEntry
*       pppIpConfigEntry
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

#if (INCLUDE_NCP_MIB == NU_TRUE)
extern DV_DEVICE_ENTRY* pppLinkSetNextIf(snmp_object_t*, UINT16);

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppIpEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppIpTable.
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
UINT16 Get_pppIpEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we are handling GETNEXT request. */
        if (!getflag)
        {
            /* Set the next device index and fall through to get it. */
            dev_ptr = pppLinkSetNextIf(obj, 12);
        }

        /* If the oid parameter is 0, then it is invalid as we are
         * handling GET request, so don't get the device at this time.
         */
        else if ((obj->Id[12] > 0) && (obj->IdLen == 13))
        {
            /* Verify that the ifIndex is valid and get the device pointer. */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[12] - 1);
        }

        /* If we got the handle to the interface device then proceed. */
        if (dev_ptr != NU_NULL)
        {
            /* Switch to appropriate attribute. */
            switch (obj->Id[11])
            {
            case 1:                         /* pppIpOperStatus */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpOperStatus(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 2:                         /* pppIpLocalToRemoteCompression
                                             * Protocol
                                             */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpLocalToRemoteCompressionProtocol(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 3:                         /* pppIpRemoteToLocalCompressionProtocol */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpRemoteToLocalCompressionProtocol(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 4:                         /* pppIpRemoteMaxSlotId */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpRemoteMaxSlotId(dev_ptr, &obj->Syntax.LngInt)
                                                            != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 5:                         /* pppIpLocalMaxSlotId */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpLocalMaxSlotId(dev_ptr, &obj->Syntax.LngInt)
                                                            != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            default:

                /* We have reached at the end of table. */
                status = SNMP_NOSUCHNAME;
            }
        }

        /* If we did not get the handle to the interface device then return
         * error code.
         */
        else
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to obtain semaphore",
                        NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Get_pppIpEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppIpEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       pppIpTable.
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
UINT16 pppIpEntry(snmp_object_t *obj, UINT16 idlen, void *param)
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
        status = Get_pppIpEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppIpEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */
    case SNMP_PDU_COMMIT:   /* Commit request. */
    default:                /* Invalid request. */

        /* Processing of set operations. */
        status = SNMP_GENERROR;
        break;
    }

    /* Return status. */
    return (status);

} /* pppIpEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppIpConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppIpConfigTable.
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
UINT16 Get_pppIpConfigEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we are handling GETNEXT request. */
        if (!getflag)
        {
            /* Set the next device index and fall through to get it. */
            dev_ptr = pppLinkSetNextIf(obj, 12);
        }

        /* If the oid parameter is 0, then it is invalid as we are
         * handling GET request, so don't get the device at this time.
         */
        else if ((obj->Id[12] > 0) && (obj->IdLen == 13))
        {
            /* Verify that the ifIndex is valid and get the device pointer. */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[12] - 1);
        }

        /* If we got handle to the interface device. */
        if (dev_ptr != NU_NULL)
        {
            /* Switch to appropriate attribute. */
            switch (obj->Id[11])
            {
            case 1:                         /* pppIpConfigAdminStatus */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpConfigAdminStatus(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 2:                         /* pppIpConfigCompression */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PMN_GetIpConfigCompression(dev_ptr,
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

        /* If we did not get the handle to the interface device. */
        else
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore.*/
    else
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to grab the semaphore",
                        NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Get_pppIpConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_pppIpConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       pppIpConfigTable.
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
UINT16 Set_pppIpConfigEntry(snmp_object_t *obj)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {       
        /* If the oid parameter is 0, then it is invalid as we are
         * handling SET request, so don't get the device at this time.
         */
        if ((obj->Id[12] > 0) && (obj->IdLen == 13))
        {
            /* Verify that the ifIndex is valid and get the device pointer. */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[12] - 1);
        }

        if (dev_ptr != NU_NULL)
        {
            /* Switch to appropriate attribute. */
            switch (obj->Id[11])
            {
            case 1:                         /* pppIpConfigAdminStatus */

                /* Call the PM function to verify and set the value
                   into the device. */
                if ((obj->Syntax.LngInt == 1) ||
                    (obj->Syntax.LngInt == 2))
                {
                    if (PMN_SetIpConfigAdminStatus(dev_ptr,
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

            case 2:                         /* pppIpConfigCompression */

                /* Call the PM function to verify and set the value
                   into the device. */
                if ((obj->Syntax.LngInt == 1) ||
                    (obj->Syntax.LngInt == 2))
                {
                    if (PMN_SetIpConfigCompression(dev_ptr,
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

        /* If we did not get the handle to the interface device. */
        else
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to grab the semaphore",
                        NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Set_pppIpConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppIpConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       pppIpConfigTable.
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
UINT16 pppIpConfigEntry(snmp_object_t *obj, UINT16 idlen, void *param)
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
        status = Get_pppIpConfigEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppIpConfigEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        status = Set_pppIpConfigEntry(obj);
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

} /* pppIpConfigEntry */

#endif /* INCLUDE_NCP_MIB */

