/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*        mib2_if.c
*
* COMPONENT
*
*        MIB II - Interface Group.
*
* DESCRIPTION
*
*        This file contain the functions that are responsible for providing
*        statistics of interfaces.
*
* DATA STRUCTURES
*
*        MIB2_If_Tbl_Last_Change
*        NET_MIB2_Memory
*        NET_MIB2_Ext_Memory
*        NET_MIB2_HC_Memory
*
* FUNCTIONS
*
*        MIB2_Interface_Get
*        MIB2_Get_Next_Device_By_Index
*        MIB2_Interface_Init
*        MIB2_Interface_Count
*        MIB2_Init_Interface_Ext
*        MIB2_Get_IfOutQLen
*        MIB2_Add_IfOutQLen
*        MIB2_Get_IfOutErrors
*        MIB2_Add_IfOutErrors
*        MIB2_Get_IfOutDiscards
*        MIB2_Add_IfOutDiscards
*        MIB2_Get_IfOutNUcastPkts
*        MIB2_Add_IfOutNUcastPkts
*        MIB2_IfAdd_Counter64
*        MIB2_Get_IfOutUcastPkts
*        MIB2_Add_IfOutUcastPkts
*        MIB2_Get_IfOutOctets
*        MIB2_Add_IfOutOctets
*        MIB2_Get_IfInUcastPkts
*        MIB2_Add_IfInUcastPkts
*        MIB2_Get_IfInNUcastPkts
*        MIB2_Add_IfInNUcastPkts
*        MIB2_Get_IfInUnknownProtos
*        MIB2_Add_IfInUnknownProtos
*        MIB2_Get_IfInErrors
*        MIB2_Add_IfInErrors
*        MIB2_Get_IfInDiscards
*        MIB2_Add_IfInDiscards
*        MIB2_Get_IfInOctets
*        MIB2_Add_IfInOctets
*        MIB2_Get_IfDescr
*        MIB2_Set_IfDescr
*        MIB2_Get_IfType
*        MIB2_Set_IfType
*        MIB2_Get_IfIndex
*        MIB2_Get_Next_IfIndex_Ext
*        MIB2_Get_IfName
*        MIB2_Get_IfLinkTrapEnable
*        MIB2_Set_IfLinkTrapEnable
*        MIB2_Get_IfHighSpeed
*        MIB2_Get_IfConnectorPresent
*        MIB2_Get_IfAlias
*        MIB2_Set_IfAlias
*        MIB2_Get_IfCounterDiscontinTime
*        MIB2_Get_IfaceIndex
*        MIB2_Get_IfMtu
*        MIB2_Set_IfMtu
*        MIB2_Get_IfSpeed
*        MIB2_Set_IfSpeed
*        MIB2_Get_IfLastChange
*        MIB2_Set_IfLastChange
*        MIB2_Get_IfStatusAdmin
*        MIB2_Set_IfStatusAdmin
*        MIB2_Get_IfStatusOper
*        MIB2_Set_IfStatusOper
*        MIB2_Get_IfAddr
*        MIB2_Set_IfAddr
*        MIB2_Get_IfChannel
*        MIB2_Set_IfChannel
*        MIB2_Get_IfHostControl
*        MIB2_Set_IfHostControl
*        MIB2_Get_IfMatrixControl
*        MIB2_Set_IfMatrixControl
*        MIB2_Get_IfFrameId
*        MIB2_Inc_IfFrameId
*        MIB2_Get_ifSpecific
*        MIB2_Get_IfInBrdcastPkts
*        MIB2_Add_IfInBrdcastPkts
*        MIB2_Get_IfOutBrdcastPkts
*        MIB2_Add_IfOutBrdcastPkts
*        MIB2_Get_IfInMulticastPkts
*        MIB2_Add_IfInMulticastPkts
*        MIB2_Get_IfOutMulticastPkts
*        MIB2_Add_IfOutMulticastPkts
*        MIB2_Get_IfPromiscMode
*
* DEPENDENCIES
*
*        nu_net.h
*        sys.h
*        ip6_mib_no_s.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

#if (INCLUDE_SNMP == NU_TRUE)

#include "networking/sys.h"

#endif /* (INCLUDE_SNMP == NU_TRUE) */

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif /* (INCLUDE_SNMP == NU_TRUE) */

#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE) && \
     (INCLUDE_SNMP == NU_TRUE))
#include "networking/2465no.h"
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for MIB2_ETHERSTATS_STRUCT structures */
MIB2_ETHERSTATS_STRUCT      NET_MIB2_Memory[NET_MAX_MIB2_DEVICES];

/* Declare memory for  MIB2_INTERFACE_EXT_STRUCT structures */
MIB2_INTERFACE_EXT_STRUCT   NET_MIB2_Ext_Memory[NET_MAX_MIB2_DEVICES];

/* Declare memory for  MIB2_INTERFACE_HC_STRUCT structures */
MIB2_INTERFACE_HC_STRUCT    NET_MIB2_HC_Memory[NET_MAX_MIB2_DEVICES];

/* Memory for Devices */
extern DV_DEVICE_ENTRY      NET_Device_Memory[];

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

/*-----------------------------------------------------------------------
 * Global Definitions
 *----------------------------------------------------------------------*/


#if (INCLUDE_SNMP == NU_TRUE)
UINT32                      MIB2_If_Tbl_Last_Change;
#endif /* (INCLUDE_SNMP == NU_TRUE) */

/*-----------------------------------------------------------------------
 * SNMP Traps
 *----------------------------------------------------------------------*/

#if (INCLUDE_SNMP == NU_TRUE)

#define SNMP_sendTrap_linkUp(list,object_list_size) \
    SendENTTrap(SNMP_TRAP_LINKUP,0,list, object_list_size)

#define SNMP_sendTrap_linkDown(list,object_list_size) \
    SendENTTrap(SNMP_TRAP_LINKDOWN, 0, list, object_list_size)

#endif /* (INCLUDE_SNMP == NU_TRUE) */

/*-----------------------------------------------------------------------
 * Functions used internally
 *----------------------------------------------------------------------*/

#if (MIB2_IF_INCLUDE == NU_TRUE)

/* Interface Group. */
MIB2_INTERFACE_STRUCT           *MIB2_Interface_Get(UINT32);

#if (INCLUDE_IF_EXT == NU_TRUE)
STATIC INT                      MIB2_IfAdd_Counter64(UINT32 *lower32,
                                                     UINT32 *higher32,
                                                     UINT32 incr);
#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
#endif /* (MIB2_IF_INCLUDE == NU_TRUE) */

/*-----------------------------------------------------------------------
 * Interfaces Group
 *----------------------------------------------------------------------*/

#if (MIB2_IF_INCLUDE == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Interface_Get
*
* DESCRIPTION
*
*       This function returns a pointer to the interface associated with
*       the interface index passed in to the function.
*
* INPUTS
*
*       index                   The interface index of the device.
*
* OUTPUTS
*
*       MIB2_INTERFACE_STRUCT * If an interface is found with the
*                               specified index, this function
*                               returns a pointer to the interface.
*       NU_NULL                 When interface is not found with specified
*                               index.
*
*************************************************************************/
MIB2_INTERFACE_STRUCT *MIB2_Interface_Get(UINT32 index)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Handle to the interface for returning purpose. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Find the target device. */
    dev = DEV_Get_Dev_By_Index(index);

    /* If dev is not NU_NULL then get the pointer to the interface.
     * Otherwise return NU_NULL.
     */
    if (dev != NU_NULL)
        iface = &(dev->dev_mibInterface);
    else
        iface = NU_NULL;

    /* Return a pointer to the interface */
    return (iface);

} /* MIB2_Interface_Get */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_Next_Device_By_Index
*
* DESCRIPTION
*
*       This function set the index passed in to the ifIndex of the device
*       having least greater/equal index with respect to the index passed
*       in.
*
* INPUTS
*
*       *index                  The interface index of the device.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_Next_Device_By_Index(UINT32 *index)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }

    else
    {
        /* If index passed in is 0 then we are required to set the index
           passed in to ifIndex of first device entry. */
        if ((*index) == 0)
        {
            /* If there exists at least one device then set the index
               passed in to ifIndex of first device entry, otherwise
               return error code. */
            if (DEV_Table.dv_head != NU_NULL)
                (*index) = DEV_Table.dv_head->dev_index;
            else
                status = MIB2_UNSUCCESSFUL;
        }
        else
        {
            /* Search the handle to the device that has ifIndex greater/
               equal ifIndex. */
            for (dev = DEV_Table.dv_head;
                 ( (dev != NU_NULL) && (dev->dev_index < (*index)) );
                 dev = dev->dev_next)
                ;

            /* If we did not find the device with greater/equal ifIndex
               then set the index passed in to ifIndex of first device
               entry, otherwise return error code. */
            if (dev == NU_NULL)
            {
                if (DEV_Table.dv_head != NU_NULL)
                    (*index) = DEV_Table.dv_head->dev_index;
                else
                    status = MIB2_UNSUCCESSFUL;
            }
            else
            {
                (*index) = dev->dev_index;
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status. */
    return (status);

}/* MIB2_Get_Next_Device_By_Index */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Interface_Init
*
* DESCRIPTION
*
*       This function initializes the Interface part of the
*       DV_DEVICE_ENTRY
*
* INPUTS
*
*       *dev_ptr                The pointer to the device entry.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       OS_INVALID_POOL         Indicates the dynamic memory pool is
*                               invalid.
*       NU_INVALID_POINTER      Indicates the return pointer is NULL.
*       OS_INVALID_SIZE         Indicates an invalid size request.
*       OS_INVALID_SUSPEND      Indicates that suspend attempted from a
*                               non-task thread.
*       NU_NO_MEMORY            Indicates the memory request could not be
*                               immediately satisfied.
*       NU_TIMEOUT              Indicates the requested memory is still
*                               unavailable even after suspending for the
*                               specified timeout value.
*       OS_POOL_DELETED         Dynamic memory pool was deleted while the
*                               task was suspended.
*
*************************************************************************/
STATUS MIB2_Interface_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS          status;

    /*  Clear out the structure.  */
    UTL_Zero(&(dev_ptr->dev_mibInterface),
             sizeof(MIB2_INTERFACE_STRUCT));

    /* Set last change to zero, as the system is being initialized. */
    dev_ptr->dev_mibInterface.lastChange = 0;

    /* Set status admin to down(2), NU_FALSE. */
    dev_ptr->dev_mibInterface.statusAdmin = NU_FALSE;

    /* Set status oper to down(2), MIB2_IF_OPER_STAT_DOWN. */
    dev_ptr->dev_mibInterface.statusOper = MIB2_IF_OPER_STAT_DOWN;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate memory to MIB2_ETHERSTATS_STRUCT structure. */
    status = NU_Allocate_Memory(MEM_Cached,
                                (VOID **)(&(dev_ptr->dev_mibInterface.eth)),
                                (UNSIGNED)(sizeof(MIB2_ETHERSTATS_STRUCT)),
                                (UNSIGNED)NU_NO_SUSPEND);

    /* If memory allocation succeeded. */
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for ether statistics",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    else

#else /* (INCLUDE_STATIC_BUILD == NU_FALSE) */
        /* Assigning memory to MIB2_ETHERSTATS_STRUCT structure. */
        dev_ptr->dev_mibInterface.eth =
            &NET_MIB2_Memory[dev_ptr - NET_Device_Memory];

#endif /* (INCLUDE_STATIC_BUILD == NU_FALSE) */
    {
        /*  Clear out the structure. */
        UTL_Zero(dev_ptr->dev_mibInterface.eth,
                 sizeof(MIB2_ETHERSTATS_STRUCT));

#if (INCLUDE_IF_EXT == NU_TRUE)

        status = MIB2_Init_Interface_Ext(dev_ptr);

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
    }

#if (INCLUDE_SNMP == NU_TRUE)

    /* Update the value of interface table last change. */
    if (status == NU_SUCCESS)
        MIB2_If_Tbl_Last_Change = SysTime();

#endif /* (INCLUDE_SNMP == NU_TRUE) */

    /* Done with the initialization. */
    return (status);

} /* MIB2_Interface_Init */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Interface_Count
*
* DESCRIPTION
*
*       This function returns the number of interfaces for the system.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       UINT16                  Number of interfaces for the system.
*
*************************************************************************/
UINT16 MIB2_Interface_Count(VOID)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Interface count. */
    UINT16                  count = 0;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    else
    {
        /* Point to the head of the device table.*/
        dev = DEV_Table.dv_head;
        /* Traverse through the whole table. */
        while (dev)
        {
            /* Incrementing interface count. */
            count++;

            /* Updating handle to point at next interface handle. */
            dev = dev->dev_next;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return the number of interfaces. */
    return (count);

} /* MIB2_Interface_Count */

#if (INCLUDE_IF_EXT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Init_Interface_Ext
*
* DESCRIPTION
*
*       This function initializes the MIB-II extensions structure and
*       higher counters depending upon the flags value.
*
* INPUTS
*
*       *dev_ptr                Interface for which the extensions
*                               are required.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request is successful
*       OS_INVALID_POOL         Indicates the dynamic memory pool is
*                               invalid.
*       NU_INVALID_POINTER      Indicates the return pointer is NULL.
*       OS_INVALID_SIZE         Indicates an invalid size request.
*       OS_INVALID_SUSPEND      Indicates that suspend attempted from a
*                               non-task thread.
*       NU_NO_MEMORY            Indicates the memory request could not be
*                               immediately satisfied.
*       NU_TIMEOUT              Indicates the requested memory is still
*                               unavailable even after suspending for the
*                               specified timeout value.
*       OS_POOL_DELETED         Dynamic memory pool was deleted while the
*                               task was suspended.
*
*************************************************************************/
STATUS MIB2_Init_Interface_Ext(DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS                      status = NU_SUCCESS;
    MIB2_INTERFACE_STRUCT       *iface;

    /* If interface interface extension is required. */
    if ((dev_ptr->dev_flags & DV_INT_EXT) != 0)
    {
        /* Getting handle to interface. */
        iface = &(dev_ptr->dev_mibInterface);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        iface->mib2_ext_tbl =
            &(NET_MIB2_Ext_Memory[dev_ptr - NET_Device_Memory]);

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        /* Allocating memory for MIB2 Extension Table. */
        status = NU_Allocate_Memory(MEM_Cached,
                                    (VOID **)&(iface->mib2_ext_tbl),
                                    sizeof(MIB2_INTERFACE_EXT_STRUCT),
                                    NU_NO_SUSPEND);

        /* If memory allocated successfully. */
        if (status == NU_SUCCESS)
        {
#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

            /* Initializing MIB2 Extension table. */
            UTL_Zero(iface->mib2_ext_tbl,
                     sizeof(MIB2_INTERFACE_EXT_STRUCT));

            /* If higher counters are also required then allocate
               memory for them. */
            if ((dev_ptr->dev_flags & DV_INT_HC) != 0)
            {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

                iface->mib2_ext_tbl->mib2_hc =
                     &NET_MIB2_HC_Memory[dev_ptr - NET_Device_Memory];

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                /* Allocating memory for higher counters. */
                status = NU_Allocate_Memory(MEM_Cached,
                                            (VOID **)&(iface->mib2_ext_tbl->mib2_hc),
                                            sizeof(MIB2_INTERFACE_HC_STRUCT),
                                            NU_NO_SUSPEND);

                /* If memory allocated successfully. */
                if (status == NU_SUCCESS)
                {
#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                    /* Initializing all the counters with zero
                       value. */
                    UTL_Zero(iface->mib2_ext_tbl->mib2_hc,
                             sizeof(MIB2_INTERFACE_HC_STRUCT));

                    /* The default value of link up down trap enable
                       is defined to be 'true'. */
                    iface->mib2_ext_tbl->mib2_linkupdown_trap_enable = NU_TRUE;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                }

                else
                {
                    NLOG_Error_Log("Memory Allocation for higher counter failed.",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

#endif /* (INCLUDE_STATIC_BUILD == NU_FALSE) */
            }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        }

        else
        {
            NLOG_Error_Log("Memory Allocation for interface extension table failed.",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

#endif /* (INCLUDE_STATIC_BUILD == NU_FALSE) */
    }

    /* Return status. */
    return (status);

} /* MIB2_Init_Interface_Ext */

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Get_IfOutQLen
*
*   DESCRIPTION
*
*       This function returns the length of the output packet queue.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       length                  Pointer to the location where the length
*                               is to be placed.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Get_IfOutQLen(UINT32 index, UINT32 *length)
{
    DV_DEVICE_ENTRY     *dev;
    INT16               status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        dev = DEV_Get_Dev_By_Index(index);

        /* If device is found, get the length and return success code. */
        if ( (dev != NU_NULL) && (!(dev->dev_flags & DV_VIRTUAL_DEV)) )
        {
            /* Getting value of length. */
            (*length) = dev->dev_transq_length;

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return the status */
    return (status);

} /* MIB2_Get_IfOutQLen */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Add_IfOutQLen
*
*   DESCRIPTION
*
*       This function adds/sets the length of the output packet queue.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       length                  The length to be stored.
*       reset                   If this value is NU_TRUE, the value is set.
*                               If this value is NU_FALSE, the value is
*                               added.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Add_IfOutQLen(UINT32 index, INT32 length, INT8 reset)
{
    DV_DEVICE_ENTRY     *dev;
    INT16               status;

    /* Find the target device. */
    dev = DEV_Get_Dev_By_Index(index);

    /* If device is found, set the length. */
    if ( (dev != NU_NULL) && (!(dev->dev_flags & DV_VIRTUAL_DEV)) )
    {
        if (reset == NU_TRUE)
            dev->dev_transq_length = (UINT32)length;
        else
            dev->dev_transq_length += (UINT32)length;

        /* Trace log */
        T_DEV_TRANSQ_LEN(dev->dev_net_if_name, dev->dev_transq_length);
        
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Return the status */
    return (status);

} /* MIB2_Add_IfOutQLen */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutErrors
*
* DESCRIPTION
*
*       This function returns the number of outbound packets that
*       could not be transmitted because of errors.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *errors                 Pointer to the location where the error
*                               count is to be placed
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutErrors(UINT32 index, UINT32 *errors)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device was found, get the number of errors. */
        if (iface != NU_NULL)
        {
            (*errors) = iface->eth->outErrors;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutErrors
*
* DESCRIPTION
*
*       This function sets/adds the number of outbound packets that
*       could not be transmitted because of errors.
*
* INPUTS
*
*       index                   The interface index of the device.
*       errors                  The number of errors to be added.
*       reset                   If this value is NU_TRUE, the value
*                               is set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutErrors(UINT32 index, UINT32 errors, INT8 reset)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of errors. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->outErrors = errors;
        else
        {
            iface->eth->outErrors += errors;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE))

            /* If counter discontinuity and if extension is enabled. */
            if ( (iface->eth->outErrors < errors) &&
                 (iface->mib2_ext_tbl != NU_NULL) )
            {
                /* Update the counter discontinuity time */
                iface->mib2_ext_tbl->mib2_counter_discontinuity = SysTime();
            }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE)) */
        }

        /* Return success code. */
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Return status. */
    return (status);

} /* MIB2_Add_IfOutErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutDiscards
*
* DESCRIPTION
*
*       This function returns the number of outbound packets which
*       were chosen to be discarded even though no errors had been
*       detected to prevent them being transmitted.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *discards               The pointer to the location where the
*                               discards are to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutDiscards(UINT32 index, UINT32 *discards)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of discards. */
        if (iface != NU_NULL)
        {
            *discards = iface->eth->outDiscards;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutDiscards */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutDiscards
*
* DESCRIPTION
*
*       This function sets/adds the number of outbound packets which
*       were chosen to be discarded even though no errors had been
*       detected to prevent them being transmitted.
*
* INPUTS
*
*       index                   The interface index of the device.
*       discards                The value to be set/added.
*       reset                   If this value is NU_TRUE, the value
*                               is set. If this value is NU_FALSE,
*                               the value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutDiscards(UINT32 index, UINT32 discards, INT8 reset)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of discards. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->outDiscards = discards;
        else
        {
            iface->eth->outDiscards += discards;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE))

            /* If counter discontinuity. */
            if ( (iface->eth->outDiscards < discards) &&
                 (iface->mib2_ext_tbl != NU_NULL) )
            {
                /* Update the counter discontinuity time. */
                iface->mib2_ext_tbl->mib2_counter_discontinuity = SysTime();
            }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE)) */

        }
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_IfOutDiscards */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Get_IfOutNUcastPkts
*
*   DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested be transmitted to a non-unicast.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the number
*                               of packets is to be stored.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Get_IfOutNUcastPkts(UINT32 index, UINT32 *packets)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

        /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets. */
        if (iface != NU_NULL)
        {
            (*packets) = iface->eth->outNUcastPkts;

#if (INCLUDE_IF_EXT == NU_TRUE)

            if (iface->mib2_ext_tbl != NU_NULL)
            {
                (*packets) +=
                         ((iface->mib2_ext_tbl->mib2_out_broadcast_pkts) +
                          (iface->mib2_ext_tbl->mib2_out_multicast_pkts));
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return the status */
    return (status);

} /* MIB2_Get_IfOutNUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Add_IfOutNUcastPkts
*
*   DESCRIPTION
*
*       This function adds/sets the total number of packets that
*       higher-level protocols requested be transmitted to a non-unicast.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Add_IfOutNUcastPkts(UINT32 index, UINT32 packets, INT8 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of packets. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->outNUcastPkts = packets;
        else
            iface->eth->outNUcastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Return the status */
    return (status);

} /* MIB2_Add_IfOutNUcastPkts */

#if (INCLUDE_IF_EXT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_IfAdd_Counter64
*
* DESCRIPTION
*
*       This function is used to increment counter by specified value.
*
* INPUTS
*
*       *lower32                The pointer to memory location where lower
*                               32 bits of the counter are stored.
*       *higher32               The pointer to memory location where
*                               higher 32 bits of the counter are stored.
*                               NU_NULL value of the is counter represent
*                               that counter is of 32 bits.
*       incr                    The value that is to be incremented in the
*                               counter.
*
* OUTPUTS
*
*       NU_TRUE                 If counter wrap up.
*       NU_FALSE                If counter did not wrap up.
*
*************************************************************************/
STATIC INT MIB2_IfAdd_Counter64(UINT32 *lower32, UINT32 *higher32,
                                UINT32 incr)
{
    INT counter_wrap_up = NU_FALSE;

    /* If we have valid parameters. */
    if (lower32 != NU_NULL)
    {
        (*lower32) += incr;

        /* If lower 32 bits of counter wrap up. */
        if ((*lower32) < incr)
        {
            /* If we have higher 32 bit of the counter then add carry to
               it. */
            if (higher32 != NU_NULL)
            {
                /* If carry occurs, it will always be one under all
                   cases, So, incrementing Higher Counter by one. */
               (*higher32)++;

                /* If higher 32 bits also wrapped up then then return
                   NU_TRUE. */
                if ((*higher32) == 0)
                {
                    counter_wrap_up = NU_TRUE;
                }
            }

            /* We don't have higher 32 bits and lower 32 bits have wrapped
               up so return NU_TRUE. */
            else
            {
                counter_wrap_up = NU_TRUE;
            }
        }
    }

    /* Returning NU_TRUE if counter wrap up, otherwise returning
       NU_FALSE. */
    return (counter_wrap_up);

} /* MIB2_IfAdd_Counter64 */

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutUcastPkts
*
* DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested to be transmitted to a
*       subnetwork-unicast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutUcastPkts(UINT32 index, UINT32 *packets,
                              UINT8 counter_type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets. */
        if (iface != NU_NULL)
        {
            packets[0] = iface->eth->outUcastPkts;

            /* If 64 bit counter is requested. */
            if (counter_type == MIB2_COUNTER_64)
            {
#if (INCLUDE_IF_EXT == NU_TRUE)

                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if ( (iface->mib2_ext_tbl != NU_NULL) &&
                     (iface->mib2_ext_tbl->mib2_hc != NU_NULL) )
                {
                    /* Getting higher 32 bits. */
                    packets[1] =
                         iface->mib2_ext_tbl->mib2_hc->mib2_hc_out_ucast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

                /* Returning error code. */
                status = MIB2_UNSUCCESSFUL;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

            }
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutUcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutUcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the total number of packets that
*       higher-level protocols requested to be transmitted to a
*       subnetwork-unicast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutUcastPkts(UINT32 index, UINT32 packets, INT8 reset)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *device;

    /* Status for returning success or error code. */
    INT16                   status;

#if (INCLUDE_IF_EXT == NU_TRUE)

    /* Pointer lower 32 bits of counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)
    /* Variable when true represent that counter wrap-up during
       increment. */
    INT                     wrap_up;
#endif

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

    /* Getting handle to the device. */
    device = DEV_Get_Dev_By_Index(index);

    /* If device is found, update the number of packets. */
    if (device != NU_NULL)
    {
        if (reset == NU_TRUE)
        {
            device->dev_mibInterface.eth->outUcastPkts = packets;

#if (INCLUDE_IF_EXT == NU_TRUE)

            /* If 64 bits counters are also implemented. */
            if ( (device->dev_flags) & (DV_INT_EXT | DV_INT_HC) )
            {
                /* Set higher 32 bits to zero. */
                device->dev_mibInterface.mib2_ext_tbl->
                            mib2_hc->mib2_hc_out_ucast_pkts = 0;
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

        }
        else
        {

#if (INCLUDE_IF_EXT == NU_TRUE)

            if ((device->dev_flags) & DV_INT_EXT)
            {
                /* If counter is of 64 bits. */
                if ((device->dev_flags) & DV_INT_HC)
                {
                    /* Getting pointer to lower 32 bits. */
                    lower32 = &(device->dev_mibInterface.eth->outUcastPkts);

                    /* Getting pointer to the higher 32 bits. */
                    higher32 = &(device->dev_mibInterface.mib2_ext_tbl->
                                 mib2_hc->mib2_hc_out_ucast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                    /* Incrementing the counter. */
                    wrap_up =
#endif
                    MIB2_IfAdd_Counter64(lower32, higher32, packets);
                }

                /* If 32 bit counter is implemented. */
                else
                {
                    /* Incrementing the counter. */
                    device->dev_mibInterface.eth->outUcastPkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                    /* Determining if the increment cause counter to wrap-up  */
                    if (device->dev_mibInterface.eth->outUcastPkts >= packets)
                        wrap_up = NU_FALSE;
                    else
                        wrap_up = NU_TRUE;

#endif /* (INCLUDE_SNMP == NU_TRUE) */

                }

#if (INCLUDE_SNMP == NU_TRUE)

                /* If counter wrapped up then up-date the counter
                   discontinuity time stamp. */
                if (wrap_up == NU_TRUE)
                {
                    device->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

            }
            else
            {
                device->dev_mibInterface.eth->outUcastPkts += packets;
            }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

            device->dev_mibInterface.eth->outUcastPkts += packets;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

        }

        status = NU_SUCCESS;
    }
    /* If we did not get the handle to the device then return error code. */
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status. */
    return (status);

} /* MIB2_Add_IfOutUcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutOctets
*
* DESCRIPTION
*
*       This function returns the total number of octets transmitted
*       out of the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *octets                 Pointer to the location where the
*                               number of octets are to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutOctets(UINT32 index, UINT32 *octets,
                           UINT8 counter_type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of octets. */
        if (iface != NU_NULL)
        {
            *octets = iface->eth->outOctets;

            if (counter_type == MIB2_COUNTER_64)
            {
#if (INCLUDE_IF_EXT == NU_TRUE)

                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return error
                   code. */
                if ( (iface->mib2_ext_tbl != NU_NULL) &&
                     (iface->mib2_ext_tbl->mib2_hc != NU_NULL) )
                {
                    /* Getting higher 32 bits. */
                    octets[1] = iface->mib2_ext_tbl->mib2_hc->mib2_hc_out_octets;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

                /* Returning error code. */
                status = MIB2_UNSUCCESSFUL;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

            }
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutOctets
*
* DESCRIPTION
*
*       This function sets/adds to the total number of octets
*       transmitted out of the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       octets                  The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set.  If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutOctets(UINT32 index, UINT32 octets, INT8 reset)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *device;

    /* Status for returning success or error code. */
    INT16                   status;

#if (INCLUDE_IF_EXT == NU_TRUE)

    /* Pointer lower 32 bits of counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)
    /* Variable when true represent that counter wrap-up during
       increment. */
    INT                     wrap_up;
#endif

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

    /* Getting handle to the device. */
    device = DEV_Get_Dev_By_Index(index);

    /* If device is found, update the number of packets. */
    if (device != NU_NULL)
    {
        if (reset == NU_TRUE)
        {
            device->dev_mibInterface.eth->outOctets = octets;

#if (INCLUDE_IF_EXT == NU_TRUE)

            /* If 64 bits counters are also implemented. */
            if ( (device->dev_flags) & (DV_INT_EXT | DV_INT_HC) )
            {
                /* Set higher 32 bits to zero. */
                device->dev_mibInterface.mib2_ext_tbl->
                            mib2_hc->mib2_hc_out_octets = 0;
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

        }
        else
        {
#if (INCLUDE_IF_EXT == NU_TRUE)

            if ((device->dev_flags) & DV_INT_EXT)
            {
                /* If counter is of 64 bits. */
                if ((device->dev_flags) & DV_INT_HC)
                {
                    /* Getting pointer to lower 32 bits. */
                    lower32 = &(device->dev_mibInterface.eth->outOctets);

                    /* Getting pointer to the higher 32 bits. */
                    higher32 = &(device->dev_mibInterface.mib2_ext_tbl->
                                 mib2_hc->mib2_hc_out_octets);

#if (INCLUDE_SNMP == NU_TRUE)
                    /* Incrementing the counter. */
                    wrap_up =
#endif
                    MIB2_IfAdd_Counter64(lower32, higher32, octets);
                }

                /* If 32 bit counter is implemented. */
                else
                {
                    /* Incrementing the counter. */
                    device->dev_mibInterface.eth->outOctets += octets;

#if (INCLUDE_SNMP == NU_TRUE)

                    /* Determining if the increment cause counter to wrap-up  */
                    if (device->dev_mibInterface.eth->outOctets >= octets)
                        wrap_up = NU_FALSE;
                    else
                        wrap_up = NU_TRUE;

#endif /* (INCLUDE_SNMP == NU_TRUE) */

                }

#if (INCLUDE_SNMP == NU_TRUE)

                /* If counter wrapped up then up-date the counter
                   discontinuity time stamp. */
                if (wrap_up == NU_TRUE)
                {
                    device->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

            }
            else
            {
                device->dev_mibInterface.eth->outOctets += octets;
            }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

            device->dev_mibInterface.eth->outOctets += octets;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
        }

        status = NU_SUCCESS;
    }

    /* If we did not get the handle to the device then return error code. */
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status. */
    return (status);

} /* MIB2_Add_IfOutOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInUcastPkts
*
* DESCRIPTION
*
*       This function returns The number of subnetwork-unicast packets
*       delivered to a higher-layer protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInUcastPkts(UINT32 index, UINT32 *packets,
                             UINT8 counter_type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get number of packets. */
        if (iface != NU_NULL)
        {
            *packets = iface->eth->inUcastPkts;

            if (counter_type == MIB2_COUNTER_64)
            {
#if (INCLUDE_IF_EXT == NU_TRUE)

                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return the
                   error code. */
                if ( (iface->mib2_ext_tbl != NU_NULL) &&
                     (iface->mib2_ext_tbl->mib2_hc != NU_NULL) )
                {
                    /* Getting higher 32 bits. */
                    packets[1] =
                          iface->mib2_ext_tbl->mib2_hc->mib2_hc_in_ucast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

                /* Returning error code. */
                status = MIB2_UNSUCCESSFUL;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
            }
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfInUcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInUcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the number of subnetwork-unicast packets
*       delivered to a higher-layer protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInUcastPkts(UINT32 index, UINT32 packets, INT8 reset)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *device;

    /* Status for returning success or error code. */
    INT16                   status;

#if (INCLUDE_IF_EXT == NU_TRUE)

    /* Pointer lower 32 bits of counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)

    /* Variable when true represent that counter wrap-up during
       increment. */
    INT                     wrap_up;
#endif

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

    /* Getting handle to the device. */
    device = DEV_Get_Dev_By_Index(index);

    /* If device is found, update the number of packets. */
    if (device != NU_NULL)
    {
        if (reset == NU_TRUE)
        {
            device->dev_mibInterface.eth->inUcastPkts = packets;

#if (INCLUDE_IF_EXT == NU_TRUE)

            /* If 64 bits counters are also implemented. */
            if ( (device->dev_flags) & (DV_INT_EXT | DV_INT_HC) )
            {
                /* Set higher 32 bits to zero. */
                device->dev_mibInterface.mib2_ext_tbl->
                            mib2_hc->mib2_hc_in_ucast_pkts = 0;
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

        }
        else
        {
#if (INCLUDE_IF_EXT == NU_TRUE)

            if ((device->dev_flags) & DV_INT_EXT)
            {
                /* If counter is of 64 bits. */
                if ((device->dev_flags) & DV_INT_HC)
                {
                    /* Getting pointer to lower 32 bits. */
                    lower32 = &(device->dev_mibInterface.eth->inUcastPkts);

                    /* Getting pointer to the higher 32 bits. */
                    higher32 = &(device->dev_mibInterface.mib2_ext_tbl->
                                 mib2_hc->mib2_hc_in_ucast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                    /* Incrementing the counter. */
                    wrap_up =
#endif
                    MIB2_IfAdd_Counter64(lower32, higher32, packets);
                }

                /* If 32 bit counter is implemented. */
                else
                {
                    /* Incrementing the counter. */
                    device->dev_mibInterface.eth->inUcastPkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                    /* Determining if the increment cause counter to wrap-up  */
                    if (device->dev_mibInterface.eth->inUcastPkts >= packets)
                        wrap_up = NU_FALSE;
                    else
                        wrap_up = NU_TRUE;

#endif /* (INCLUDE_SNMP == NU_TRUE) */
                }

#if (INCLUDE_SNMP == NU_TRUE)

                /* If counter wrapped up then up-date the counter
                   discontinuity time stamp. */
                if (wrap_up == NU_TRUE)
                {
                    device->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
            }

            else
            {
                device->dev_mibInterface.eth->inUcastPkts += packets;
            }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

            device->dev_mibInterface.eth->inUcastPkts += packets;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
        }

        status = NU_SUCCESS;
    }
    /* If we did not get the handle to the device then return error code. */
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status. */
    return (status);

} /* MIB2_Add_IfInUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Get_IfInNUcastPkts
*
*   DESCRIPTION
*
*       This function returns the number of non-unicast packets
*       delivered to a higher-layer protocol.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       packets                 Pointer to the location where the number
*                               of packets is to be stored.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Get_IfInNUcastPkts(UINT32 index, UINT32 *packets)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets. */
        if (iface != NU_NULL)
        {
            (*packets) = iface->eth->inNUcastPkts;

#if (INCLUDE_IF_EXT == NU_TRUE)

            if (iface->mib2_ext_tbl != NU_NULL)
            {
                (*packets) +=
                          ((iface->mib2_ext_tbl->mib2_in_broadcast_pkts) +
                           (iface->mib2_ext_tbl->mib2_in_multicast_pkts));
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

            /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Return the status */
    return (status);

} /* MIB2_Get_IfInNUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       MIB2_Add_IfInNUcastPkts
*
*   DESCRIPTION
*
*       This function adds/sets the number of non-unicast packets
*       delivered to a higher-layer protocol.
*
*   INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
*   OUTPUTS
*
*       INT16                   If a device is found with the specified
*                               index, this function returns NU_SUCCESS.
*                               Otherwise, it returns MIB2_UNSUCCESSFUL.
*
*************************************************************************/
INT16 MIB2_Add_IfInNUcastPkts(UINT32 index, UINT32 packets, INT8 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of packets. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->inNUcastPkts = packets;
        else
            iface->eth->inNUcastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Return the status */
    return (status);

} /* MIB2_Add_IfInNUcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInUnknownProtos
*
* DESCRIPTION
*
*       This function returns the number of packets received via the
*       interface which were discarded because of an unknown or
*       unsupported protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the number
*                               of packets is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInUnknownProtos(UINT32 index, UINT32 *packets)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets. */
        if (iface != NU_NULL)
        {
            (*packets) = iface->eth->inUnknownProtos;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfInUnknownProtos */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInUnknownProtos
*
* DESCRIPTION
*
*       This function adds/sets the number of packets received via the
*       interface which were discarded because of an unknown or
*       unsupported protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInUnknownProtos(UINT32 index, UINT32 packets, INT8 reset)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of packets. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->inUnknownProtos = packets;
        else
        {
            iface->eth->inUnknownProtos += packets;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE))

            /* If counter discontinuity. */
            if ( (iface->eth->inUnknownProtos < packets) &&
                 (iface->mib2_ext_tbl != NU_NULL) )
            {
                /* Update the counter discontinuity time. */
              iface->mib2_ext_tbl->mib2_counter_discontinuity = SysTime();
            }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE)) */
        }

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_IfInUnknownProtos */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInErrors
*
* DESCRIPTION
*
*       This function returns the number of inbound packets that
*       contained errors preventing them from being delivered to a
*       higher-layer protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *errors                 Pointer to the location where the error
*                               count is to be placed
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInErrors(UINT32 index, UINT32 *errors)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of errors. */
        if (iface != NU_NULL)
        {
            (*errors) = iface->eth->inErrors;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Returning status */
    return (status);

} /* MIB2_Get_IfInErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInErrors
*
* DESCRIPTION
*
*       This function adds/sets the number of inbound packets that
*       contained errors preventing them from being delivered to a
*       higher-layer protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInErrors(UINT32 index, UINT32 packets, INT8 reset)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of errors. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->inErrors = packets;
        else
        {
            iface->eth->inErrors += packets;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE))

            /* If counter discontinuity. */
            if ( (iface->eth->inErrors < packets) &&
                 (iface->mib2_ext_tbl != NU_NULL) )
            {
                /* Update counter discontinuity. */
              iface->mib2_ext_tbl->mib2_counter_discontinuity = SysTime();
            }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE) */
        }

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_IfInErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInDiscards
*
* DESCRIPTION
*
*       This function returns the number of inbound packets which were
*       chosen to be discarded even though no errors had been detected
*       to prevent them being deliverable to a higher-layer protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *discards               The pointer to the location where number
*                               of discards is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInDiscards(UINT32 index, UINT32 *discards)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of discards. */
        if (iface != NU_NULL)
        {
            (*discards) = iface->eth->inDiscards;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Returning status */
    return (status);

} /* MIB2_Get_IfInDiscards */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInDiscards
*
* DESCRIPTION
*
*       This function adds/sets the number of inbound packets which
*       were chosen to be discarded even though no errors had been
*       detected to prevent them being deliverable to a higher-layer
*       protocol.
*
* INPUTS
*
*       index                   The interface index of the device.
*       packets                 The value to be added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInDiscards(UINT32 index, UINT32 packets, INT8 reset)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, update the number of discards. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->inDiscards = packets;
        else
        {
            iface->eth->inDiscards += packets;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE))

            if ( (iface->eth->inDiscards < packets) &&
                 (iface->mib2_ext_tbl != NU_NULL) )
            {
              iface->mib2_ext_tbl->mib2_counter_discontinuity = SysTime();
            }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IF_EXT == NU_TRUE)) */
        }

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_IfInDiscards */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInOctets
*
* DESCRIPTION
*
*       This function returns the total number of octets received on the
*       interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *octets                 Pointer to the location where the number
*                               of octets are to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInOctets(UINT32 index, UINT32 *octets,
                          UINT8 counter_type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of octets. */
        if (iface != NU_NULL)
        {
            (*octets) = iface->eth->inOctets;

            if (counter_type == MIB2_COUNTER_64)
            {
#if (INCLUDE_IF_EXT == NU_TRUE)

                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if ( (iface->mib2_ext_tbl != NU_NULL) &&
                     (iface->mib2_ext_tbl->mib2_hc != NU_NULL) )
                {
                    /* Getting higher 32 bits. */
                    octets[1] =
                              iface->mib2_ext_tbl->mib2_hc->mib2_hc_in_octets;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

            /* Returning error code. */
            status = MIB2_UNSUCCESSFUL;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
            }
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfInOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInOctets
*
* DESCRIPTION
*
*       This function adds/sets to the total number of octets received
*       on the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       octets                  The value to add/set.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInOctets(UINT32 index, UINT32 octets, INT8 reset)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *device;

    /* Status for returning success or error code. */
    INT16                   status;

#if (INCLUDE_IF_EXT == NU_TRUE)
    /* Pointer lower 32 bits of counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)
    /* Variable when true represent that counter wrap-up during
       increment. */
    INT                     wrap_up;
#endif
#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

    /* Getting handle to the device. */
    device = DEV_Get_Dev_By_Index(index);

    /* If device is found, update the number of packets. */
    if (device != NU_NULL)
    {
        if (reset == NU_TRUE)
        {
            device->dev_mibInterface.eth->inOctets = octets;

#if (INCLUDE_IF_EXT == NU_TRUE)

            /* If 64 bits counters are also implemented. */
            if ( (device->dev_flags) & (DV_INT_EXT | DV_INT_HC) )
            {
                /* Set higher 32 bits to zero. */
                device->dev_mibInterface.mib2_ext_tbl->
                            mib2_hc->mib2_hc_in_octets = 0;
            }

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

        }
        else
        {
#if (INCLUDE_IF_EXT == NU_TRUE)

            if ((device->dev_flags) & DV_INT_EXT)
            {
                /* If counter is of 64 bits. */
                if ((device->dev_flags) & DV_INT_HC)
                {
                    /* Getting pointer to lower 32 bits. */
                    lower32 = &(device->dev_mibInterface.eth->inOctets);

                    /* Getting pointer to the higher 32 bits. */
                    higher32 = &(device->dev_mibInterface.mib2_ext_tbl->
                                 mib2_hc->mib2_hc_in_octets);

#if (INCLUDE_SNMP == NU_TRUE)
                    /* Incrementing the counter. */
                    wrap_up =
#endif
                    MIB2_IfAdd_Counter64(lower32, higher32, octets);
                }

                /* If 32 bit counter is implemented. */
                else
                {
                    /* Incrementing the counter. */
                    device->dev_mibInterface.eth->inOctets += octets;

#if (INCLUDE_SNMP == NU_TRUE)

                    /* Determining if the increment cause counter to wrap-up  */
                    if (device->dev_mibInterface.eth->inOctets >= octets)
                        wrap_up = NU_FALSE;
                    else
                        wrap_up = NU_TRUE;

#endif /* (INCLUDE_SNMP == NU_TRUE) */
                }

#if (INCLUDE_SNMP == NU_TRUE)

                /* If counter wrapped up then up-date the counter
                   discontinuity time stamp. */
                if (wrap_up == NU_TRUE)
                {
                    device->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
            }

            else
            {
                device->dev_mibInterface.eth->inOctets += octets;
            }

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

            device->dev_mibInterface.eth->inOctets += octets;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
        }

        status = NU_SUCCESS;
    }
    /* If we did not get the handle to the device then return error code. */
    else
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* Returning status. */
    return (status);

} /* MIB2_Add_IfInOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfDescr
*
* DESCRIPTION
*
*       This function returns a textual string containing information
*       about the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *string                 Textual information about the interface.
*       *length                 Length of the string.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfDescr(UINT32 index, UINT8 *string, UINT32 *length)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the string and its length. */
        if (iface != NU_NULL)
        {
            (*length) = strlen((CHAR *)iface->descr);

            NU_BLOCK_COPY(string, iface->descr, (UINT16)(*length));

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfDescr */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfDescr
*
* DESCRIPTION
*
*       This function sets the textual string containing information
*       about the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       string                  Textual information about the interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfDescr(UINT32 index, UINT8 *string)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* String length. */
    UINT16                 length;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, set the string and its length. */
    if (iface != NU_NULL)
    {
        /* Determine the length.  The length is the minimum
        of the string length and MIB2_INT_DESC_LENGTH - 1 */
        length = (UINT16)(strlen((CHAR *)string));
        if ( length > (MIB2_INT_DESC_LENGTH - 1))
        {
            length = (MIB2_INT_DESC_LENGTH - 1);
        }

        /* Copy length chars from the string. */
        NU_BLOCK_COPY(iface->descr, string, length);

        /* Terminate the possibly partial string */
        iface->descr[length] = NU_NULL;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfDescr */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfType
*
* DESCRIPTION
*
*       This function returns the type of interface, distinguished
*       according to the physical/link protocol(s) immediately `below'
*       the network layer in the protocol stack.
*
* INPUTS
*
*       index                   The interface index of the device.
*       type                    The pointer to the location where the
*                               type is to be placed.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfType(UINT32 index, INT32 *type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the type. */
        if (iface != NU_NULL)
        {
            (*type) = iface->type;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfType */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfType
*
* DESCRIPTION
*
*       This function sets the type of interface, distinguished according
*       to the physical/link protocol(s) immediately `below' the network
*       layer in the protocol stack.
*
* INPUTS
*
*       index                   The interface index of the device.
*       type                    The type of the interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfType(UINT32 index, INT32 type)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, set the type. */
    if (iface != NU_NULL)
    {
        iface->type = (UINT16)type;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfType */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfIndex
*
* DESCRIPTION
*
*       This function checks whether an interface with the given index
*       exists.
*
* INPUTS
*
*       index                   The interface index of the device.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfIndex(UINT32 index)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status for returning success or error code. */
    INT16               status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }

    else
    {
        /* Find the target device. */
        dev = DEV_Get_Dev_By_Index(index);

        /* If device is found, set the status. */
        if (dev != NU_NULL)
            status = NU_SUCCESS;

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfIndex */

#if (INCLUDE_IF_EXT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_Next_IfIndex_Ext
*
* DESCRIPTION
*
*       This update the value pointed by the argument to the next the
*       index of next device that has interface extension enabled.
*
* INPUTS
*
*       *index                  The interface index of the device.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_Next_IfIndex_Ext(UINT32 *index)
{
    DV_DEVICE_ENTRY         *dev;
    INT16                   status = MIB2_UNSUCCESSFUL;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
    else
    {
        /* If index value is zero then search for the first device with
           mib interface extension enabled. */
        if((*index) == 0)
        {
            /* Loop through the device table to search the first device
               with mib interface extension enabled. */
            for (dev = DEV_Table.dv_head; dev != NU_NULL;
                 dev = dev->dev_next)
            {
                /* If we have reached at a device with mib interface
                   extension enabled then update the index passed in,
                   return success code and break through the loop. */
                if (dev->dev_mibInterface.mib2_ext_tbl != NU_NULL)
                {
                    /* Returning success code. */
                    status = NU_SUCCESS;

                    /* Updating index value passed in. */
                    (*index) = dev->dev_index;

                    /* Breaking through the loop. */
                    break;
                }
            }
        }

        /* If index value is non-zero then search for the device with
           least minimum greater or equal interface index as passed in and
           whose mib interface extension is enabled. */
        else
        {
            /* Loop through the device table to search the next device */
            for (dev = DEV_Table.dv_head; dev != NU_NULL;
                 dev = dev->dev_next)
            {
                /* If we have reached at the device with least minimum
                   greater or equal interface index as passed in and whose
                    mib interface extension is enabled, then update the
                    index value passed, return success code and break
                    through the loop. */
                if ((dev->dev_index >= (*index)) &&
                    (dev->dev_mibInterface.mib2_ext_tbl != NU_NULL))
                {
                    /* Returning success code. */
                    status = NU_SUCCESS;

                    /* Updating index passed in. */
                    (*index) = dev->dev_index;

                    /* Breaking through the loop. */
                    break;
                }
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_Next_IfIndex_Ext */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfName
*
* DESCRIPTION
*
*       This function gets ifName for the interface specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_name                The pointer to the memory location where
*                               ifName value is to be copied.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfName(UINT32 index, CHAR *if_name)
{
    /* Handle to the device name. */
    CHAR                    *if_name_ptr;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Calling the routine that will map interface index to interface
       name. */
    if_name_ptr  = NU_IF_IndexToName((INT32)index, if_name);

    /* If return value from NU_IF_IndexToName is NU_NULL then the routine
       failed to map interface index to interface name. So, return error
       code in that case. */
    if(if_name_ptr == NU_NULL)
    {
        status = MIB2_UNSUCCESSFUL;
    }

    /* If NU_IF_IndexToName succeeded to map interface index to interface
       name, then return success code. */
    else
    {
        status = NU_SUCCESS;
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfName */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfLinkTrapEnable
*
* DESCRIPTION
*
*       This function gets IfLinkUpDownTrapEnable for the interface
*       specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_link_trap_enable    The pointer to the memory location where
*                               link trap enable/disable value is to be
*                               written.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfLinkTrapEnable(UINT32 index, UINT32 *if_link_trap_enable)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_link_trap_enable pointer is valid,
         * get the link_trap enable/disable value and return success code.
         */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting if_link_trap_enable value. In SNMP 1 represent
             * boolean value 'true' and 2 represent boolean value 'false'.
             */
            (*if_link_trap_enable) =
                ((iface->mib2_ext_tbl->mib2_linkupdown_trap_enable == NU_TRUE)
                                                                 ? 1 : 2);

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfLinkTrapEnable */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfLinkTrapEnable
*
* DESCRIPTION
*
*       This function sets IfLinkUpDownTrapEnable for the interface
*       specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       if_link_trap_enable     The value IfLinkUpDownTrapEnable that is
*                               required to set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfLinkTrapEnable(UINT32 index, UINT32 if_link_trap_enable)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_link_trap_enable pointer is valid, get
           the link_trap enable/disable value and return success code. */
        if ( (iface != NU_NULL) && (iface->mib2_ext_tbl != NU_NULL) )
        {
            /* Setting if_link_trap_enable value. In SNMP 1 represent
             * boolean value 'true' and 2 represent boolean value 'false'.
             */
            iface->mib2_ext_tbl->mib2_linkupdown_trap_enable =
                                       (UINT8)((if_link_trap_enable & 1));

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Set_IfLinkTrapEnable */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfHighSpeed
*
* DESCRIPTION
*
*       This function gets ifHighSpeed for the interface specified by
*       index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_high_speed          The pointer to the memory location where
*                               ifHighSpeed value is to be written.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfHighSpeed(UINT32 index, UINT32 *if_high_speed)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_high_speed pointer is valid, get
           the ifHighSpeed value and return success code. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting if_link_trap_enable value. */
            (*if_high_speed) = iface->mib2_ext_tbl->mib2_high_speed;

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfHighSpeed */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfConnectorPresent
*
* DESCRIPTION
*
*       This function gets ifConnectorPresent for the interface specified
*       by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_connector_present   The pointer to the memory location where
*                               ifConnectorPresent value is to be written.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfConnectorPresent(UINT32 index, UINT32 *if_connector_present)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_connector_present pointer is valid,
           get the ifConnectorPresent value and return success code. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting if_link_trap_enable value. In SNMP 1 represent
               boolean value 'true' and 2 represent boolean value 'false'. */
            (*if_connector_present) =
               ((iface->mib2_ext_tbl->mib2_connector_present == NU_TRUE) ?
                                                                   1 : 2);

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfConnectorPresent */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfAlias
*
* DESCRIPTION
*
*       This function gets ifName for the interface specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_alias               The pointer to the memory location where
*                               ifAlias value is to be copied.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfAlias(UINT32 index, CHAR *if_alias)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_alias pointer is valid,
           get the ifAlias value and return success code. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting ifName value. */
            strcpy(if_alias, (CHAR *)iface->mib2_ext_tbl->mib2_alias);

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfAlias */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfAlias
*
* DESCRIPTION
*
*       This function gets ifName for the interface specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_alias               The pointer to the memory location where
*                               new value ifAlias is present.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfAlias(UINT32 index, const CHAR *if_alias)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_alias pointer is valid,
           get the ifAlias value and return success code. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting ifName value. */
            strcpy((CHAR *)iface->mib2_ext_tbl->mib2_alias, if_alias);

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Set_IfAlias */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfCounterDiscontinTime
*
* DESCRIPTION
*
*       This function gets ifCounterDiscontinuityTime for the interface
*       specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *if_discontinuity_time  The pointer to the memory location where
*                               ifCounterDiscontinuityTime value is to be
*                               written.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfCounterDiscontinTime(UINT32 index,
                                      UINT32 *if_discontinuity_time)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If interface is found and if_connector_present pointer is valid,
           get the ifCounterDiscontinuityTime value and return success code. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            /* Getting ifCounterDiscontinuityTime value. */
            (*if_discontinuity_time) =
                          iface->mib2_ext_tbl->mib2_counter_discontinuity;

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfCounterDiscontinTime */

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfaceIndex
*
* DESCRIPTION
*
*       This function returns the interface index of a device based on
*       the name of the device.
*
* INPUTS
*
*       name                    The device name.
*
* OUTPUTS
*
*       INT16                   Device index having name as that name
*                               passed in.
*       -1                      If device is not found.
*
*************************************************************************/
INT16 MIB2_Get_IfaceIndex(const CHAR *name)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY     *dev;

    /* Index for returning purpose. */
    INT16               index;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        index = -1;
    }
    else
    {
        /* Find the target device. */
        dev = DEV_Get_Dev_By_Name(name);

        /* If the device is found, set the index. */
        if (dev != NU_NULL)
            index = (INT16)(dev->dev_index);
        else
            index = -1;
    }
    /* Return the index. */
    return (index);

} /* MIB2_Get_IfaceIndex */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfMtu
*
* DESCRIPTION
*
*       This function returns the size of the largest datagram which can
*       be sent/received on the interface, specified in octets.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *octets                 Pointer to the location where the
*                               number of octets are to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfMtu(UINT32 index, UINT32 *octets)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        dev = DEV_Get_Dev_By_Index(index);

        /* If the device is found, get the MTU. */
        if (dev != NU_NULL)
        {
            *octets = dev->dev_mtu;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfMtu */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfMtu
*
* DESCRIPTION
*
*       This function sets the size of the largest datagram which can be
*       sent/received on the interface, specified in octets.
*
* INPUTS
*
*       index                   The interface index of the device.
*       octets                  The value to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfMtu(UINT32 index, UINT32 octets)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target device. */
    dev = DEV_Get_Dev_By_Index(index);

    /* If device is found, set the MTU. */
    if (dev != NU_NULL)
    {
        dev->dev_mtu = octets;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfMtu */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfSpeed
*
* DESCRIPTION
*
*       This function returns an estimate of the interface's current
*       bandwidth in bits per second
*
* INPUTS
*
*       index                   The interface index of the device.
*       *speed                  The pointer to the location where the
*                               current bandwidth is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfSpeed(UINT32 index, UINT32 *speed)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the speed. */
        if (iface != NU_NULL)
        {
            *speed = iface->speed;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfSpeed */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfSpeed
*
* DESCRIPTION
*
*       This function sets the interface's current bandwidth
*       in bits per second
*
* INPUTS
*
*       index                   The interface index of the device.
*       speed                   The value to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfSpeed(UINT32 index, UINT32 speed)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, set the speed. */
    if (iface != NU_NULL)
    {
        iface->speed = speed;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfSpeed */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfLastChange
*
* DESCRIPTION
*
*       This function returns the value of sysUpTime at the time the
*       interface entered its current operational state.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *time                   The pointer to where the time is to be
*                               placed.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfLastChange(UINT32 index, UINT32 *time)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the sysUpTime. */
        if (iface != NU_NULL)
        {
            *time = iface->lastChange;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfLastChange */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfLastChange
*
* DESCRIPTION
*
*       This function sets the value of sysUpTime at the time the
*       interface entered its current operational state.
*
* INPUTS
*
*       index                   The interface index of the device.
*       time                    The value to be set.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfLastChange(UINT32 index, UINT32 time)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If device is found, set the sysUpTime. */
    if (iface != NU_NULL)
    {
        iface->lastChange = time;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfLastChange */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfStatusAdmin
*
* DESCRIPTION
*
*       This function returns the desired state of the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       state                   The location where state is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfStatusAdmin(UINT32 index, INT32 *state)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the desired state. */
        if (iface != NU_NULL)
        {
            if (iface->statusAdmin == NU_TRUE)
                *state = 1;
            else
                *state = 2;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfStatusAdmin */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfStatusAdmin
*
* DESCRIPTION
*
*       This function sets the the desired state of the interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       value                   The value for the new desired state of
*                               the interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfStatusAdmin(UINT32 index, INT32 value)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device.  */
        dev = DEV_Get_Dev_By_Index(index);

        /* If device is found, set the desired state. */
        if (dev != NU_NULL)
        {
            if (value == 1)
            {
                dev->dev_mibInterface.statusAdmin = NU_TRUE;

                /* Set ifOperStatus to MIB2_IF_OPER_STAT_UP. */
                MIB2_Set_IfStatusOper(dev, MIB2_IF_OPER_STAT_UP);

                /* Set the device as UP */
                dev->dev_flags |= DV_UP;

                status = NU_SUCCESS;
            }

            else if (value == 2)
            {
                dev->dev_mibInterface.statusAdmin = NU_FALSE;

                /* Set ifOperStatus to MIB2_IF_OPER_STAT_DOWN. */
                MIB2_Set_IfStatusOper(dev, MIB2_IF_OPER_STAT_DOWN);

                /* Set the device as not UP */
                dev->dev_flags &= (~DV_UP);

                status = NU_SUCCESS;
            }

            /* Otherwise, return an error */
            else
                status = MIB2_UNSUCCESSFUL;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return the status. */
    return ((INT16)status);

} /* MIB2_Set_IfStatusAdmin */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfStatusOper
*
* DESCRIPTION
*
*       This function gets the current operational state of the
*       interface.
*
* INPUTS
*
*       index                   The interface index of the device.
*       state                   The pointer to the location where the
*                               state should be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfStatusOper(UINT32 index, INT32 *state)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the current state. */
        if (iface != NU_NULL)
        {
            (*state) = iface->statusOper;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfStatusOper */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfStatusOper
*
* DESCRIPTION
*
*       This function sets the operational state of the interface.
*
* INPUTS
*
*       iface                   A pointer to the interface structure.
*       value                   The value for the new operation state
*                               of the interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfStatusOper(DV_DEVICE_ENTRY *dev_ptr, INT32 value)
{
    MIB2_INTERFACE_STRUCT   *iface;

#if (INCLUDE_SNMP == NU_TRUE)
    static snmp_object_t    object_list[3];
    UINT32                  if_oid[] = {1,3,6,1,2,1,2,2,1};
    UINT32                  if_oid_len = 11;
    UINT16                  object_list_length = 3;
    UINT32                  prev_down;
    UINT32                  new_down;

#endif

    /* Getting handle to device interface. */
    iface = &(dev_ptr->dev_mibInterface);

#if (INCLUDE_SNMP == NU_TRUE)
    /* Updating boolean variable to represent the current down state of
       device. */
    prev_down = (((iface->statusOper == MIB2_IF_OPER_STAT_DOWN) ||
                  (iface->statusOper == MIB2_IF_OPER_STAT_LOWER_DOWN))
                 ? (NU_TRUE) : (NU_FALSE) );


    /* Updating boolean variable to represent the down state of device
       after setting the ifOperStatus. */
    new_down = (((value == MIB2_IF_OPER_STAT_DOWN) ||
                 (value == MIB2_IF_OPER_STAT_LOWER_DOWN))
                ? (NU_TRUE) : (NU_FALSE) );

#endif

    /* Setting value to ifOperStatus. */
    iface->statusOper = (INT16)value;

#if (INCLUDE_SNMP == NU_TRUE)
    /* If SNMP Traps are to be sent. */
    if (prev_down != new_down)
    {
        /* Update the value of interface last change. */
        iface->lastChange = SysTime();

        /* Adding the ifIndex in the object List to be sent in
           linkup or link down notification */
        NU_BLOCK_COPY(object_list[0].Id, if_oid,
                      (UINT16)((if_oid_len - 2)* sizeof(UINT32)));

        object_list[0].Id[(UINT16)(if_oid_len - 2)] = 1;

        object_list[0].Id[(UINT16)(if_oid_len -1)] = (dev_ptr->dev_index + 1);

        object_list[0].IdLen = if_oid_len;

        object_list[0].Type = SNMP_INTEGER;

        object_list[0].Request = SNMP_PDU_GET;

        /* Assigning the ifIndex value. Add one because interface index start
           from '0' in Nucleus Net and it start from '1' in SNMP. */
        object_list[0].Syntax.LngUns = (dev_ptr->dev_index + 1);

        /* Adding ifAdminStatus in the object list to be sent in link up/down
           notification.*/
        NU_BLOCK_COPY(object_list[1].Id, if_oid,
                      (UINT16)((if_oid_len - 2) * sizeof(UINT32)));

        object_list[1].Id[(UINT16)(if_oid_len - 2)] = 7;

        object_list[1].Id[(UINT16)(if_oid_len - 1)] = (dev_ptr->dev_index + 1);

        object_list[1].IdLen = if_oid_len;

        object_list[1].Type = SNMP_INTEGER;

        object_list[1].Request = SNMP_PDU_GET;

        /* Assigning the ifAdminStatus value. */
        object_list[1].Syntax.LngUns = ((dev_ptr->dev_mibInterface.statusAdmin
                                            == NU_TRUE) ? 1 : 2);

        /* Adding ifOperStatus in the object list to be sent in link up/down
           notification. */
        NU_BLOCK_COPY(object_list[2].Id, if_oid,
                      (UINT16)((if_oid_len - 2) * sizeof(UINT32)));

        object_list[2].Id[(UINT16)(if_oid_len - 2)] = 8;

        object_list[2].Id[(UINT16)(if_oid_len - 1)] = (dev_ptr->dev_index + 1);

        object_list[2].IdLen = if_oid_len;

        object_list[2].Type = SNMP_INTEGER;

        object_list[2].Request = SNMP_PDU_GET;

        /* Assigning the new value of ifOperStatus. */
        object_list[2].Syntax.LngInt = (value);

        if(new_down == NU_TRUE)
        {
            SNMP_sendTrap_linkDown(object_list, object_list_length);
        }

        else
        {
            SNMP_sendTrap_linkUp(object_list, object_list_length);
        }

#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE))
        if (dev_ptr->dev_flags & DV6_IPV6)
            IP6_Send_Notification(dev_ptr);
#endif

    }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

    /* Return the status. */
    return (NU_SUCCESS);

} /* MIB2_Set_IfStatusOper */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfAddr
*
* DESCRIPTION
*
*       This function returns the interface's address at the protocol
*       layer immediately `below' the network layer in the protocol
*       stack.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *address                The pointer to where the address is to
*                               be stored.
*       *length                 The pointer to where the address length
*                               is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfAddr(UINT32 index, UINT8 *address, UINT32 *length)
{
    DV_DEVICE_ENTRY     *dev;
    INT16               status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target device. */
        dev = DEV_Get_Dev_By_Index(index);

        /* If device is found, get the physical address. */
        if (dev)
        {
            NU_BLOCK_COPY(address, dev->dev_mac_addr, DADDLEN);

            (*length) = DADDLEN;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfAddr */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfAddr
*
* DESCRIPTION
*
*       This function sets the interface's address at the protocol layer
*       immediately `below' the network layer in the protocol stack.
*
* INPUTS
*
*       index                   The interface index of the device.
*       address                 The address to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfAddr(UINT32 index, UINT8 *address)
{
    DV_DEVICE_ENTRY     *dev;
    INT16               status;

    /* Find the target device. */
    dev = DEV_Get_Dev_By_Index(index);

    /* If device is found, set the physical address. */
    if (dev)
    {
        /* Set the physical address. */
        if (DEV_Set_Phys_Address(dev, address) == NU_SUCCESS)
            status = NU_SUCCESS;

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Set_IfAddr */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfChannel
*
* DESCRIPTION
*
*       This function gets the interface's channel
*
* INPUTS
*
*       index                   The interface index of the device.
*       channel                 The channel.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfChannel(UINT32 index, VOID **channel)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the channel. */
        if (iface != NU_NULL)
        {
            *channel = iface->channel;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfChannel */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfChannel
*
* DESCRIPTION
*
*       This function sets the interface's channel
*
* INPUTS
*
*       index                   The interface index of the device.
*       channel                 The channel to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfChannel(UINT32 index, VOID *channel)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, set the channel. */
        if (iface != NU_NULL)
        {
            iface->channel = channel;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Set_IfChannel */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfHostControl
*
* DESCRIPTION
*
*       This function gets the interface's host control
*
* INPUTS
*
*       index                   The interface index of the device.
*       hostcontrol             The hostcontrol.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfHostControl(UINT32 index, VOID **hostcontrol)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the hostcontrol. */
        if (iface != NU_NULL)
        {
            *hostcontrol = iface->hostcontrol;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfHostControl */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfHostControl
*
* DESCRIPTION
*
*       This function sets the interface's host control
*
* INPUTS
*
*       index                   The interface index of the device.
*       hostcontrol             The hostcontrol to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfHostControl(UINT32 index, VOID *hostcontrol)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, set the hostcontrol. */
        if (iface != NU_NULL)
        {
            iface->hostcontrol = hostcontrol;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Set_IfHostControl */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfMatrixControl
*
* DESCRIPTION
*
*       This function gets the interface's matrix control
*
* INPUTS
*
*      index                    The interface index of the device.
*      matrixcontrol            The matrix control.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfMatrixControl(UINT32 index, VOID **matrixcontrol)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the matrixcontrol. */
        if (iface != NU_NULL)
        {
            *matrixcontrol = iface->matrixcontrol;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Returning status */
    return (status);

} /* MIB2_Get_IfMatrixControl */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_IfMatrixControl
*
* DESCRIPTION
*
*       This function sets the interface's matrix control
*
* INPUTS
*
*       index                   The interface index of the device.
*       matrixcontrol           The matrixcontrol to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Set_IfMatrixControl(UINT32 index, VOID *matrixcontrol)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, set the matrixcontrol. */
        if (iface != NU_NULL)
        {
            iface->matrixcontrol = matrixcontrol;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Set_IfMatrixControl */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfFrameId
*
* DESCRIPTION
*
*       This function gets the interface's frame id.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *frameid                The frameid.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfFrameId(UINT32 index, UINT32 *frameid)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the frame id. */
        if (iface != NU_NULL)
        {
            *frameid = iface->frameId;
            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfFrameId */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Inc_IfFrameId
*
* DESCRIPTION
*
*       This function increments the interface's frame id.
*
* INPUTS
*
*       index                   The interface index of the device.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Inc_IfFrameId(UINT32 index)
{
    /* Handle to the interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, increment the frame id. */
        if (iface != NU_NULL)
        {
            iface->frameId++;

            if (iface->frameId > 0x7FFFFFFFL)
                iface->frameId = 0;

            status = NU_SUCCESS;
        }

        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Inc_IfFrameId */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_ifSpecific
*
* DESCRIPTION
*
*       IfSpecific has depreciated, our object in adding this function we
*       attain backward compatibility with SNMP 2.2.
*
* INPUTS
*
*       index                   The interface index of the device.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_ifSpecific(VOID)
{
    return (MIB2_UNSUCCESSFUL);
}

#if (INCLUDE_IF_EXT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInBrdcastPkts
*
* DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested to be received by a
*       subnetwork-multicast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInBrdcastPkts(UINT32 index, UINT32 *packets,
                                 UINT8 counter_type)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets broadcasted. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            packets[0] = iface->mib2_ext_tbl->mib2_in_broadcast_pkts;

            /* If 64 bit counter is requested. */
            if (counter_type == MIB2_COUNTER_64)
            {
                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if (iface->mib2_ext_tbl->mib2_hc != NU_NULL)
                {
                    /* Getting higher 32 bits. */
                    packets[1] =
                      iface->mib2_ext_tbl->mib2_hc->mib2_hc_in_broadcast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }
            }
        }
        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfInBrdcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInBrdcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the total number of packets broadcasted
*       that higher-level protocols requested to be received to a
*       multicast address.
*
* INPUTS
*
*       *dev                    The handle to the interface device.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInBrdcastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets,
                               INT8 reset)
{
    /* Pointer to lower 32 bits of the counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits of the counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)

    /* Flag to represent the counter wrap up. */
    INT                     wrap_up;
#endif

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* If interface extension is implemented then update the counter. */
    if ( (dev->dev_flags) & DV_INT_EXT)
    {
        if (reset == NU_TRUE)
        {
            dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_broadcast_pkts = packets;

            if ((dev->dev_flags) & DV_INT_HC)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_in_broadcast_pkts = 0;
            }
        }
        else
        {
            /* If counter is of 64 bits. */
            if ((dev->dev_flags) & DV_INT_HC)
            {
                lower32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_broadcast_pkts);

                higher32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_in_broadcast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                wrap_up =
#endif
                MIB2_IfAdd_Counter64(lower32, higher32, packets);
            }
            else
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_broadcast_pkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                /* Determining if the increment cause counter to wrap-up  */
                if (dev->dev_mibInterface.
                    mib2_ext_tbl->mib2_in_broadcast_pkts >= packets)
                {
                    wrap_up = NU_FALSE;
                }
                else
                {
                    wrap_up = NU_TRUE;
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
            }
#if (INCLUDE_SNMP == NU_TRUE)

            /* If counter wrapped up then up-date the counter
               discontinuity time stamp. */
            if (wrap_up == NU_TRUE)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
            }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
        }
    }

    /* If counter is not implemented. */
    else
    {
        status = MIB2_Add_IfInNUcastPkts(dev->dev_index, packets, reset);
    }

    /* Return status. */
    return (status);

} /* MIB2_Add_IfInBrdcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutBrdcastPkts
*
* DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested to be transmitted to a
*       subnetwork-broadcast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutBrdcastPkts(UINT32 index, UINT32 *packets,
                                UINT8 counter_type)
{
    /* Handle to interface extension. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets broadcasted. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            packets[0] = iface->mib2_ext_tbl->mib2_out_broadcast_pkts;

            /* If 64 bit counter is requested. */
            if (counter_type == MIB2_COUNTER_64)
            {
                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if (iface->mib2_ext_tbl->mib2_hc)
                {
                    /* Getting higher 32 bits. */
                    packets[1] = iface->mib2_ext_tbl->mib2_hc->
                                  mib2_hc_out_broadcast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }
            }
        }
        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutBrdcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutBrdcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the total number of packets broadcasted
*       that higher-level protocols requested to be transmitted to a
*       broadcast address.
*
* INPUTS
*
*       *dev                    The handle to the device interface.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutBrdcastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets,
                                INT8 reset)
{
    /* Pointer to lower 32 bits of the counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits of the counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)
    /* Flag to represent the counter wrap up. */
    INT                     wrap_up;
#endif

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* If interface extension is implemented then update the counter. */
    if ((dev->dev_flags) & DV_INT_EXT)
    {
        if (reset == NU_TRUE)
        {
            dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_broadcast_pkts = packets;

            if ((dev->dev_flags) & DV_INT_HC)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_out_broadcast_pkts = 0;
            }
        }
        else
        {
            /* If counter is of 64 bits. */
            if ((dev->dev_flags) & DV_INT_HC)
            {
                lower32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_broadcast_pkts);

                higher32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_out_broadcast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                wrap_up =
#endif
                MIB2_IfAdd_Counter64(lower32, higher32, packets);
            }
            else
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_broadcast_pkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                /* Determining if the increment cause counter to wrap-up  */
                if (dev->dev_mibInterface.
                    mib2_ext_tbl->mib2_out_broadcast_pkts >= packets)
                {
                    wrap_up = NU_FALSE;
                }
                else
                {
                    wrap_up = NU_TRUE;
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
            }

#if (INCLUDE_SNMP == NU_TRUE)

            /* If counter wrapped up then up-date the counter
               discontinuity time stamp. */
            if (wrap_up == NU_TRUE)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
            }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
        }
    }

    /* If counter is not implemented. */
    else
    {
        status = MIB2_Add_IfOutNUcastPkts(dev->dev_index, packets, reset);
    }

    /* Return status. */
    return (status);

} /* MIB2_Add_IfOutBrdcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfInMulticastPkts
*
* DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested to be transmitted to a
*       subnetwork-multicast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfInMulticastPkts(UINT32 index, UINT32 *packets,
                                 UINT8 counter_type)
{
    /* Handle to interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of packets multicasted. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            packets[0] = iface->mib2_ext_tbl->mib2_in_multicast_pkts;

            /* If 64 bit counter is requested. */
            if (counter_type == MIB2_COUNTER_64)
            {
                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if (iface->mib2_ext_tbl->mib2_hc)
                {
                    /* Getting higher 32 bits. */
                    packets[1] = iface->mib2_ext_tbl->mib2_hc->
                                 mib2_hc_in_multicast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }
            }
        }
        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
    /* Returning status */
    return (status);

} /* MIB2_Get_IfInMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfInMulticastPkts
*
* DESCRIPTION
*
*       This function adds/sets the total number of packets broadcasted
*       that higher-level protocols requested to be transmitted to a
*       broadcast address.
*
* INPUTS
*
*       *dev                    The handle to the device interface.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfInMulticastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets,
                                 INT8 reset)
{
    /* Pointer to lower 32 bits of the counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits of the counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)
    /* Flag to represent the counter wrap up. */
    INT                     wrap_up;
#endif

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* If interface extension is implemented then update the counter. */
    if ((dev->dev_flags) & DV_INT_EXT)
    {
        if (reset == NU_TRUE)
        {
            dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_multicast_pkts = packets;

            if ((dev->dev_flags) & DV_INT_HC)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_in_multicast_pkts = 0;
            }
        }
        else
        {
            /* If counter is of 64 bits. */
            if ((dev->dev_flags) & DV_INT_HC)
            {
                lower32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_multicast_pkts);

                higher32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_in_multicast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                wrap_up =
#endif
                MIB2_IfAdd_Counter64(lower32, higher32, packets);
            }
            else
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_in_multicast_pkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                /* Determining if the increment cause counter to wrap-up  */
                if (dev->dev_mibInterface.
                    mib2_ext_tbl->mib2_in_multicast_pkts >= packets)
                {
                    wrap_up = NU_FALSE;
                }
                else
                {
                    wrap_up = NU_TRUE;
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

            }
#if (INCLUDE_SNMP == NU_TRUE)

            /* If counter wrapped up then up-date the counter
               discontinuity time stamp. */
            if (wrap_up == NU_TRUE)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
            }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
        }
    }

    /* If counter is not implemented. */
    else
    {
        status = MIB2_Add_IfInNUcastPkts(dev->dev_index, packets, reset);
    }

    /* Returning status */
    return (status);

} /* MIB2_Add_IfInMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfOutMulticastPkts
*
* DESCRIPTION
*
*       This function returns the total number of packets that
*       higher-level protocols requested to be transmitted to a
*       subnetwork-multicast address.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *packets                Pointer to the location where the
*                               number of packets is to be stored.
*       counter_type            If this value is COUNTER32 then counter is
*                               treated as 32 bit counter. If this value
*                               is COUNTER64 then counter will be treated
*                               as 64 bit counter.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfOutMulticastPkts(UINT32 index, UINT32 *packets,
                                 UINT8 counter_type)
{
    /* Handle to interface. */
    MIB2_INTERFACE_STRUCT   *iface;

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If device is found, get the number of multicast packets sent. */
        if ( (iface) && (iface->mib2_ext_tbl) )
        {
            packets[0] = iface->mib2_ext_tbl->mib2_out_multicast_pkts;

            /* If 64 bit counter is requested. */
            if (counter_type == MIB2_COUNTER_64)
            {
                /* If 64 bit extension implemented for the device then set
                   higher counter with respective value, otherwise return
                   error code. */
                if (iface->mib2_ext_tbl->mib2_hc)
                {
                    /* Getting higher 32 bits. */
                    packets[1] = iface->mib2_ext_tbl->mib2_hc->
                                 mib2_hc_out_multicast_pkts;
                }
                else
                {
                    /* Returning error code. */
                    status = MIB2_UNSUCCESSFUL;
                }
            }
        }
        /* Otherwise, return an error */
        else
            status = MIB2_UNSUCCESSFUL;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (status);

} /* MIB2_Get_IfOutMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_IfOutMulticastPkts
*
* DESCRIPTION
*
*       This function adds/sets the total number of packets multicasted
*       that higher-level protocols requested to be transmitted to a
*       multicast address.
*
* INPUTS
*
*       *dev                    The handle to the interface device.
*       packets                 The value to be set/added.
*       reset                   If this value is NU_TRUE, the value is
*                               set. If this value is NU_FALSE, the
*                               value is added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_IfOutMulticastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets,
                                  INT8 reset)
{
    /* Pointer to lower 32 bits of the counter. */
    UINT32                  *lower32;

    /* Pointer to higher 32 bits of the counter. */
    UINT32                  *higher32;

#if (INCLUDE_SNMP == NU_TRUE)

    /* Flag to represent the counter wrap up. */
    INT                     wrap_up;
#endif

    /* Status for returning success or error code. */
    INT16                   status = NU_SUCCESS;

    /* If we got the handle to the device and interface extension is
       implemented then update the counter. */
    if ((dev->dev_flags) & DV_INT_EXT)
    {
        if (reset == NU_TRUE)
        {
            dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_multicast_pkts = packets;

            if ((dev->dev_flags) & DV_INT_HC)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_out_multicast_pkts = 0;
            }
        }
        else
        {
            /* If counter is of 64 bits. */
            if ((dev->dev_flags) & DV_INT_HC)
            {
                lower32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_multicast_pkts);

                higher32 = &(dev->dev_mibInterface.mib2_ext_tbl->
                                mib2_hc->mib2_hc_out_multicast_pkts);

#if (INCLUDE_SNMP == NU_TRUE)
                wrap_up =
#endif
                MIB2_IfAdd_Counter64(lower32, higher32, packets);
            }
            else
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                             mib2_out_multicast_pkts += packets;

#if (INCLUDE_SNMP == NU_TRUE)

                /* Determining if the increment cause counter to wrap-up  */
                if (dev->dev_mibInterface.
                    mib2_ext_tbl->mib2_out_multicast_pkts >= packets)
                {
                    wrap_up = NU_FALSE;
                }
                else
                {
                    wrap_up = NU_TRUE;
                }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

            }
#if (INCLUDE_SNMP == NU_TRUE)

            /* If counter wrapped up then up-date the counter
               discontinuity time stamp. */
            if (wrap_up == NU_TRUE)
            {
                dev->dev_mibInterface.mib2_ext_tbl->
                                   mib2_counter_discontinuity = SysTime();
            }

#endif /* (INCLUDE_SNMP == NU_TRUE) */
        }
    }

    /* If counter is not implemented. */
    else
    {
        status = MIB2_Add_IfOutNUcastPkts(dev->dev_index, packets, reset);
    }

    /* Returning status */
    return (status);

} /* MIB2_Add_IfOutMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfPromiscMode
*
* DESCRIPTION
*
*       This function is used to get a boolean value that represent
*       current enable/disable state of promiscuous mode of the device
*       specified by index.
*
* INPUTS
*
*       index                   The interface index of the device.
*       *mode                   Pointer to the location where the
*                               promiscuous mode is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfPromiscMode(UINT32 index, UINT32 *mode)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Getting device handle. */
        dev = DEV_Get_Dev_By_Index(index);

        /* If we have got the handle to the device then process the request
           and return success code, otherwise return error code. */
        if (dev != NU_NULL)
        {
            /* If promiscuous bit is set then promiscuous mode is On. */
            if ((dev->dev_flags & DV_PROMISC) != 0)
            {
                /* In SNMP 1 represent TRUE. */
                (*mode) = 1;
            }

            /* If promiscuous bit is clear then promiscuous mode is Off. */
            else
            {
                /* In SNMP 2 represent FALSE. */
                (*mode) = 2;
            }

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* If we did not got the handle to the device then return error
           code. */
        else
        {
            /* Returning error code. */
            status = MIB2_UNSUCCESSFUL;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfPromiscMode */

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
#endif /* (MIB2_IF_INCLUDE == NU_TRUE) */
#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */


