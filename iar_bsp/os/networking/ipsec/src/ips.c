/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips.c
*
* COMPONENT
*
*       IPSEC - Initialization
*
* DESCRIPTION
*
*       This file contains all the variables which are global to
*       all the IPsec implementation along with the IPsec initialization
*       routines.
*
* DATA STRUCTURES
*
*       *IPSEC_Memory_Pool
*       IPSEC_Resource
*       IPSEC_State
*       IPSEC_Registry_Path
*
* FUNCTIONS
*
*       nu_os_net_ipsec_init
*       IPSEC_Initialize
*       IPSEC_Apply_To_Interface
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#if INCLUDE_IKE == NU_TRUE
#include "networking/ike_api.h"
#endif
#include        "os/kernel/plus/supplement/inc/error_management.h"

/* Defining the global memory pool pointer which is used by the Nucleus
   IPsec. */
NU_MEMORY_POOL      *IPSEC_Memory_Pool;

/* Declaring the IPsec semaphore for protection. */
NU_SEMAPHORE        IPSEC_Resource;

/* Flag to check for multiple initialization of IPsec. */
UINT8               IPSEC_State = IPSEC_STOPPED;

/* String to store registry path for ipsec */
CHAR                IPSEC_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

/************************************************************************
* FUNCTION
*
*       nu_os_net_ipsec_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus IPsec and IKE
*       modules. It is the entry point of the product initialization
*       sequence.
*
* INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       NU_NO_MEMORY            Memory not available.
*       IPSEC_ALREADY_RUNNING   IPsec already initialized.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS nu_os_net_ipsec_init(CHAR *path, INT startstop)
{
    STATUS            status = -1;

    if(path != NU_NULL)
    {
        /* Save a copy locally. */
        strcpy(IPSEC_Registry_Path, path);
    }

    if(startstop)
    {
        /* Initialize the Nucleus IPsec component. */
        status = IPSEC_Initialize(&System_Memory);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error at call to IPSEC_Initialize().\n",
                        NERR_FATAL, __FILE__, __LINE__);
            /* Call error handling function */
            ERC_System_Error(status);
        }

#ifdef CFG_NU_OS_NET_IKE_ENABLE
        else
        {
            /* Initialize the Nucleus IKE component. */
            status = IKE_Initialize(&System_Memory);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Error at call to IKE_Initialize().\n",
                        NERR_FATAL, __FILE__, __LINE__);
                /* Call error handling function */
                ERC_System_Error(status);
            }
        }
#endif /* CFG_NU_OS_NET_IKE_ENABLE */
    }
    else
    {
        /* Stop requested, for now nothing to do */
        status = NU_SUCCESS;
    }

    return (status);

} /* nu_os_net_ipsec_init */

/************************************************************************
* FUNCTION
*
*        IPSEC_Initialize
*
* DESCRIPTION
*
*        This function initializes the Nucleus IPsec module.
*
* INPUTS
*
*       *memory_pool            Pointer to memory pool which will be
*                               used throughout the IPsec component.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       NU_NO_MEMORY            Memory not available.
*       Error Status            Error status returned by
*                               IPSEC_Lifetimes_Init() call.
*       IPSEC_ALREADY_RUNNING   IPsec already initialized.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Initialize(NU_MEMORY_POOL *memory_pool)
{
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if(IPSEC_State != IPSEC_STOPPED)
    {
        status = IPSEC_ALREADY_RUNNING;
    }

    else
    {
        /* Make sure that the passed pool pointer is not null. */
        if(memory_pool == NU_NULL)
        {
            /* Invalid memory pool pointer. */
            status = NU_NO_MEMORY;
        }
        else
        {
            /* Assigning the given memory pool to the IPsec memory pool. */
            IPSEC_Memory_Pool = memory_pool;

            /* Creating the IPsec semaphore. */
            status = NU_Create_Semaphore(&IPSEC_Resource, "IPsec",
                                         (UNSIGNED)1, NU_FIFO);
            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Initializing the policy group component. */
                IPSEC_Group_Init();

                /* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)

                /* Initialize the lifetime init. */
                status = IPSEC_Lifetimes_Init();

                if(status == NU_SUCCESS)
                {
#endif
                    IPSEC_State = IPSEC_INITIALIZED;
#if (INCLUDE_IKE == NU_TRUE)
                }
#endif
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Now return the status value. */
    return (status);

} /* IPSEC_Initialize. */

/************************************************************************
* FUNCTION
*
*        IPSEC_Apply_To_Interface
*
* DESCRIPTION
*
*        This function enables or disables IPsec on a network device.
*
* INPUTS
*
*       *if_name                Pointer to interface name.
*       state_flag              Interface state to switch i.e.IPSEC_ENABLE
*                               or IPSEC_DISABLE.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       IPSEC_NOT_FOUND         Interface is not present.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Apply_To_Interface(CHAR *if_name, UINT8 state_flag)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *device;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(if_name == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Obtain TCP Semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

    if (status == NU_SUCCESS)
    {
        /* Get the Device Entry */
        device = DEV_Get_Dev_By_Name(if_name);

        /* Make sure we got the device. */
        if(device != NU_NULL)
        {
            /* Make sure device is registered with some group, otherwise
             * there is no point of enabling the IPsec
             * processing on the device.
             */
            if(device->dev_ext.dev_phy->dev_phy_ips_group != NU_NULL)
            {
                if (state_flag == IPSEC_ENABLE)
                {
                    /* Enable IPsec. */
                    device->dev_flags2 |= DV_IPSEC_ENABLE;
                }

                else if (state_flag == IPSEC_DISABLE)
                {
                    /* Disable IPsec.*/
                    device->dev_flags2 &= ~DV_IPSEC_ENABLE;
                }

                else
                {
                    /* Invalid status flag is passed. */
                    status = IPSEC_INVALID_PARAMS;
                }
            }
            else
            {
                /* Device is not registered with an IPsec group. */
                status = IPSEC_NOT_FOUND;
            }
        }
        else
        {
            /* Device is not present. */
            status = IPSEC_NOT_FOUND;
        }

        /* Now everything is done, release the semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE,__FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Unable to obtain IPsec semaphore.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Apply_To_Interface. */
