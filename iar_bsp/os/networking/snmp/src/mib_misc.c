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
*       mib_misc.c                                               
*
*   DESCRIPTION
*
*       This file contains functions for setting Status and Storage
*       type. These function are user by MIB's having Status and/or
*       Storage support implemented.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MIB_Set_Status
*       MIB_Set_Storage_Type
*
*   DEPENDENCIES
*
*       snmp.h
*       mib_misc.h
*
************************************************************************/

#include "networking/snmp.h"
#include "networking/mib_misc.h"

/************************************************************************
*
*   FUNCTION
*
*       MIB_Set_Status
*
*   DESCRIPTION
*
*       This function sets the row status for a row in the table.
*
*   INPUTS
*
*       *row_status      Pointer to the row status to be set.
*       new_status       New value to be set.
*       *row_flag        Row flag.
*       storage_type     Storage type.
*
*   OUTPUTS
*
*************************************************************************/
UINT16 MIB_Set_Status(UINT8 *row_status, UINT32 new_status,
                      UINT8 *row_flag, UINT8 storage_type)
{
    UINT16              status;

    /* If the row had been previously created, reset the flag. */
    if(*row_flag == SNMP_PDU_COMMIT)
        *row_flag = 0;

    if(storage_type == SNMP_STORAGE_READONLY &&
       *row_status != SNMP_ROW_CREATEANDWAIT &&
       *row_status != SNMP_ROW_CREATEANDGO)
    {
        status = SNMP_READONLY;
    }
    else if((new_status == SNMP_ROW_ACTIVE) ||
            ((storage_type != SNMP_STORAGE_PERMANENT) &&
             (new_status == SNMP_ROW_DESTROY)) ||
            (new_status == SNMP_ROW_NOTINSERVICE))
    {
        *row_status = (UINT8)new_status;
        status = NU_SUCCESS;
    }
    else if(((*row_status) == SNMP_ROW_CREATEANDWAIT) &&
            ((new_status == SNMP_ROW_CREATEANDWAIT) ||
             (new_status == SNMP_ROW_CREATEANDGO)))
    {
        *row_status = (UINT8)new_status;
        status = NU_SUCCESS;
    }
    else
    {
        status = SNMP_INCONSISTANTVALUE;
    }

    return (status);
}

/************************************************************************
*
*   FUNCTION
*
*       MIB_Set_StorageType
*
*   DESCRIPTION
*
*       This function sets the storage type for a row.
*
*   INPUTS
*
*       *storage_type    Pointer to storage type to be set.
*       new_storage      New value to be set.
*       *row_flag        Row flag.
*       row_status       Row Status.
*
*   OUTPUTS
*
*************************************************************************/
UINT16 MIB_Set_StorageType(UINT8 *storage_type, UINT32 new_storage_type,
                           UINT8 *row_flag, UINT8 row_status)
{
    UINT16              status = NU_SUCCESS;

    /* If the row had been previously created, reset the flag. */
    if(*row_flag == SNMP_PDU_COMMIT)
        *row_flag = 0;

    /*If the row is in creation process. */
    if(row_status == SNMP_ROW_CREATEANDWAIT ||
       row_status == SNMP_ROW_CREATEANDGO)
    {
        *storage_type = (UINT8)new_storage_type;
    }
    else
    {
        if((*storage_type != SNMP_STORAGE_PERMANENT) &&
           (*storage_type != SNMP_STORAGE_READONLY) &&
           (new_storage_type != SNMP_STORAGE_PERMANENT) &&
           (new_storage_type != SNMP_STORAGE_READONLY))
        {
            *storage_type = (UINT8)new_storage_type;
        }
        else
            status = SNMP_WRONGVALUE;
    }

    return (status);
}
