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
*       1471lcp.c                                                
*
*   DESCRIPTION
*
*       This file contains MIB implementation of LCP as defined in
*       RFC1471.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       pppLinkSetNextIf
*       Get_pppLinkStatusEntry
*       pppLinkStatusEntry
*       Get_pppLinkConfigEntry
*       Set_pppLinkConfigEntry
*       pppLinkConfigEntry
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

#if (INCLUDE_LCP_MIB == NU_TRUE || INCLUDE_NCP_MIB == NU_TRUE)

DV_DEVICE_ENTRY* pppLinkSetNextIf(snmp_object_t *obj, UINT16 idlen);

/************************************************************************
*
*   FUNCTION
*
*       pppLinkSetNextIf
*
*   DESCRIPTION
*
*       Checks the device table to see if there is a next device,
*       then assigns its index to the object. If already at the last
*       device, the object will not be written. The next device's
*       address is returned so it can be used by the calling function.
*
*   INPUTS
*
*       *obj                    Pointer to the object identifier to be
*                               set.
*       idlen                   The index of the variable to set.
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        Next PPP interface exists.
*       NU_NULL                 Next PPP interface does not exist.
*
*************************************************************************/
DV_DEVICE_ENTRY* pppLinkSetNextIf(snmp_object_t *obj, UINT16 idlen)
{
    DV_DEVICE_ENTRY     *dev_ptr;

    /* Start with first device. */
    dev_ptr = DEV_Table.dv_head;

    /* Traverse the device table. */
    while (dev_ptr)
    {
        /* If we have reached at a PPP enabled next device. */
        if ((dev_ptr->dev_type == DVT_PPP) && (dev_ptr->dev_index >= obj->Id[idlen]))
        {
            /* Write the device index into the object. */
            obj->Id[idlen] = dev_ptr->dev_index + 1;
            obj->IdLen = idlen + 1;

            /* Break through the loop. */
            break;
        }

        /* Moving forward in the device table. */
        dev_ptr = dev_ptr->dev_next;
    }

    /* Return a pointer to the device, otherwise return NU_NULL. */
    return (dev_ptr);

} /* pppLinkSetNextIf */


#if (INCLUDE_LCP_MIB == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppLinkStatusEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppLinkStatusTable.
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
UINT16 Get_pppLinkStatusEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        /* Log the error if we failed to grab the semaphore. */
        NLOG_Error_Log("Failed to grab semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }
    
    /* If we successfully grabbed the semaphore. */
    else
    {
        /* If the OID parameter is 0 and we are handling GET request,
         * then the agent is looking for the first device, so don't
         * get the device at this time.
         */
        if ((getflag) && (obj->Id[13] > 0) && (obj->IdLen == 14))
        {
            /* Verify that the ifIndex is valid and get the device pointer. */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[13] - 1);
        }

        /* If we are handling GETNEXT request. */
        else if (!getflag)
        {
            /* Set the next device index and fall through to get it. */
            dev_ptr = pppLinkSetNextIf(obj, 13);
        }

        /* If we got the handle to the interface device then proceed,
         * otherwise return error code.
         */
        if (dev_ptr != NU_NULL)
        {
            /* Switch for handling appropriate attribute. */
            switch (obj->Id[12])
            {
            case 1:             /* pppLinkStatusPhysicalIndex */

                /* Assign the variable based on information in this
                 * device.
                 */
                obj->Syntax.LngInt = PML_GetPhysicalIndex(dev_ptr);
                if (obj->Syntax.LngInt  < 0)
                    status = SNMP_NOSUCHINSTANCE;

                break;

            case 2:             /* pppLinkStatusBadAddresses */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetBadAddresses(dev_ptr, &obj->Syntax.LngUns) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 3:             /* pppLinkStatusBadControls */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetBadControls(dev_ptr, &obj->Syntax.LngUns) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 4:             /* pppLinkStatusPacketTooLongs */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetPacketTooLongs(dev_ptr, &obj->Syntax.LngUns) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 5:             /* pppLinkStatusBadFCSs */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetBadFCSs(dev_ptr, &obj->Syntax.LngUns) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 6:             /* pppLinkStatusLocalMRU */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetLocalMRU(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 7:             /* pppLinkStatusRemoteMRU */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetRemoteMRU(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 8:             /* pppLinkStatusLocalToPeerACCMap */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetLocalToPeerACCMap(dev_ptr, obj->Syntax.BufChr)
                                                            == NU_SUCCESS)
                {
                    obj->SyntaxLen = 4;
                }
                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 9:             /* pppLinkStatusPeerToLocalACCMap */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetPeerToLocalACCMap(dev_ptr, obj->Syntax.BufChr)
                                                            == NU_SUCCESS)
                {
                    obj->SyntaxLen = 4;
                }
                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 10:            /* pppLinkStatusLocalToRemoteProtocol
                                 * Compression
                                 */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetLocalToRemoteProtocolCompression(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 11:            /* pppLinkStatusRemoteToLocalProtocol
                                 * Compression
                                 */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetRemoteToLocalProtocolCompression(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 12:            /* pppLinkStatusLocalToRemote
                                 * ACCompression
                                 */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetLocalToRemoteACCompression(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 13:            /* pppLinkStatusRemoteToLocal
                                 * ACCompression
                                 */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetRemoteToLocalACCompression(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 14:            /* pppLinkStatusTransmitFcsSize */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetTransmitFcsSize(dev_ptr, &obj->Syntax.LngInt)
                                                            != NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 15:            /* pppLinkStatusReceiveFcsSize */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetReceiveFcsSize(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                      status = SNMP_NOSUCHINSTANCE;
                }

                break;

            default:
                /* We have reached at end of the table. */
                status = SNMP_NOSUCHNAME;
            }
        }

        /* If we failed to get handle to the interface device return error code.
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

    /* Return success or error code. */
    return (status);

} /* Get_pppLinkStatusEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppLinkStatusEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       pppLinkStatusEntry.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       idlen               Not used.
*       *param              Not used.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*       SNMP_GENERROR       Invalid request.
*
*************************************************************************/
UINT16 pppLinkStatusEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_pppLinkStatusEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppLinkStatusEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */
    case SNMP_PDU_COMMIT:   /* Commit request. */
    default:            /* Invalid request. */

        /* Processing of set operations. */
        status = SNMP_GENERROR;

        break;
    }

    /* Return status. */
    return (status);

} /* pppLinkStatusEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_pppLinkConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       pppLinkConfigTable.
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
UINT16 Get_pppLinkConfigEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If the oid parameter is 0 and we are handling GET request,
         * then the agent is looking for the first device, so don't
         * get the device at this time.
         */
        if ((getflag) && (obj->Id[13] > 0) && (obj->IdLen == 14))
        {
            /* Verify that the ifIndex is valid and get the device pointer. */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[13] - 1);
        }

        /* If we are handling GETNEXT request. */
        else if (!getflag)
        {
            /* Set the next device index and fall through to get it. */
            dev_ptr = pppLinkSetNextIf(obj, 13);
        }

        /* If we got the handle to the interface device then proceed,
         * otherwise return error code.
         */
        if (dev_ptr != NU_NULL)
        {
            /* Switch to appropriate attribute. */
            switch(obj->Id[12])
            {
            case 1:             /* pppLinkConfigInitialMRU */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetInitialMRU(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 2:             /* pppLinkConfigReceiveACCMap */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetInitialReceiveACCMap(dev_ptr,
                                        obj->Syntax.BufChr) == NU_SUCCESS)
                {
                    obj->SyntaxLen = 4;
                }
                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 3:             /* pppLinkConfigTransmitACCMap */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetInitialTransmitACCMap(dev_ptr,
                                        obj->Syntax.BufChr) == NU_SUCCESS)
                {
                    obj->SyntaxLen = 4;
                }
                else
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 4:             /* pppLinkConfigMagicNumber */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetInitialMagicNumber(dev_ptr,
                                       &obj->Syntax.LngInt) != NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            case 5:             /* pppLinkConfigFcsSize */

                /* Assign the variable based on information in this
                 * device.
                 */
                if (PML_GetInitialFcsSize(dev_ptr, &obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                {
                    status = SNMP_NOSUCHINSTANCE;
                }

                break;

            default:
                /* We have reached at end of the table. */
                status = SNMP_NOSUCHNAME;
            }
        }

        /* We failed get the handle to the interface device. */
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
        NLOG_Error_Log("Failed to grab semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Get_pppLinkConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_pppLinkConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       pppLinkConfigTable.
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
UINT16 Set_pppLinkConfigEntry(snmp_object_t *obj)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If the oid parameter is 0 and we are handling GET request,
         * then the agent is looking for the first device, so don't
         * get the device at this time.
         */
        if ((obj->Id[13] > 0) && (obj->IdLen == 14))
        {
            /* Verify that the ifIndex is valid and get the device
             * pointer.
             */
            dev_ptr = DEV_Get_Dev_By_Index(obj->Id[13] - 1);
        }

        /* If we got the handle to the interface device then proceed,
         * otherwise return error code.
         */
        if (dev_ptr != NU_NULL)
        {
            /* Switch to appropriate attribute. */
            switch(obj->Id[12])
            {
            case 1:             /* pppLinkConfigInitialMRU */

                /* Call the PM function to verify and set the value
                 * into the device.
                 */
                if (obj->Syntax.LngInt >= 0)
                {
                    if (PML_SetInitialMRU(dev_ptr, obj->Syntax.LngInt) !=
                                                               NU_SUCCESS)
                    {
                        status = SNMP_NOSUCHINSTANCE;
                    }
                }
                else
                {
                    status = SNMP_WRONGVALUE;
                }

                break;

            case 2:             /* pppLinkConfigReceiveACCMap */

                /* Call the PM function to verify and set the value
                 * into the device.
                 */
                if (obj->SyntaxLen == 4)
                {
                    if (PML_SetInitialReceiveACCMap(dev_ptr,
                                        obj->Syntax.BufChr) != NU_SUCCESS)
                    {
                        status = SNMP_NOSUCHINSTANCE;
                    }
                }
                else
                {
                    status = SNMP_WRONGVALUE;
                }

                break;

            case 3:             /* pppLinkConfigTransmitACCMap */

                /* Call the PM function to verify and set the value
                 * into the device.
                 */
                if (obj->SyntaxLen == 4)
                {
                    if (PML_SetInitialTransmitACCMap(dev_ptr,
                                        obj->Syntax.BufChr) != NU_SUCCESS)
                    {
                        status = SNMP_NOSUCHINSTANCE;
                    }
                }
                else
                {
                    status = SNMP_WRONGVALUE;
                }

                break;

            case 4:             /* pppLinkConfigMagicNumber */

                /* Call the PM function to verify and set the value
                 * into the device.
                 */
                if ((obj->Syntax.LngInt == 1) ||
                    (obj->Syntax.LngInt == 2))
                {
                    if (PML_SetInitialMagicNumber(dev_ptr,
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

            case 5:             /* pppLinkConfigFcsSize */

                /* Call the PM function to verify and set the value
                 * into the device.
                 */
                if ( (obj->Syntax.LngInt >= 0) &&
                     (obj->Syntax.LngInt <= 128) )
                {
                    if (PML_SetInitialFcsSize(dev_ptr, obj->Syntax.LngInt)
                                                            != NU_SUCCESS)
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

        /* We failed to get handle to the interface device. */
        else
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* Release semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
    }

    /* We failed to grab the semaphore. */
    else
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to grab semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Set_pppLinkConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       pppLinkConfigEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       pppLinkConfigTable.
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
UINT16 pppLinkConfigEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_pppLinkConfigEntry(obj, getflag);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_pppLinkConfigEntry);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        status = Set_pppLinkConfigEntry(obj);
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

#endif /* INCLUDE_LCP_MIB */
#endif

