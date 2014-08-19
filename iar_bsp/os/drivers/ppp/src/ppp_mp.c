/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_mp.c
*
*   COMPONENT
*
*       MP - PPP Multilink Protocol
*
*   DESCRIPTION
*
*       This file contains the handling of PPP Multilink protocol. It
*       contains user APIs and other MP routines.
*
*   DATA STRUCTURES
*
*       MP_Bundle_List
*       MP_Semaphore
*
*   FUNCTIONS
*
*       PPP_MP_Initialize
*       PPP_MP_Init
*       PPP_MP_Attach_Link
*       PPP_MP_Add_Link
*       PPP_MP_Remove_Link
*       PPP_MP_Create_Bundle
*       PPP_MP_Get_Virt_If_By_Device
*       PPP_MP_Get_Virt_If_By_User
*       PPP_MP_Get_Opt
*       PPP_MP_Find_Bundle_By_Device
*       PPP_MP_Get_Bundle
*       PPP_MP_Terminate_Links
*       PPP_MP_Output
*       PPP_MP_Input
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       ppp_mpfrag.h
*       snmp_api.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

/* If PPP Multilink Protocol is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)

#include "drivers/ppp_mpfrag.h"

/* List of bundles for the MP. */
PPP_MP_BUNDLE_LIST  MP_Bundle_List;

/* Semaphore for protection in MP module. */
NU_SEMAPHORE        MP_Semaphore;

#if(INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if MP_DEBUG_PRINT_OK
#define PrintInfo(s)        PPP_Printf(s)
#else
#define PrintInfo(s)
#endif

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Initialize
*
* DESCRIPTION
*
*       Initialization function for the MP Component.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Initialization was successful.
*       NU_NO_MEMORY            Memory not available.
*
*************************************************************************/
STATUS PPP_MP_Initialize(VOID)
{
    /* Declare variable. */
    STATUS       status = NU_SUCCESS;
    static UINT8 semaphore_created = NU_FALSE;

    /* If the semaphore for protection in MP module is not already
    created, create one */
    if (semaphore_created == NU_FALSE)
    {
        /* Create the semaphore. */
        status = NU_Create_Semaphore(&MP_Semaphore, "PPP_MP",
                                     (UNSIGNED)1, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create MP semaphore.",
                           NERR_FATAL, __FILE__, __LINE__);
        }
        else
        {
            /* The semaphore has been created */
            semaphore_created = NU_TRUE;
        }
    }

    return (status);

} /* PPP_MP_Initialize */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Init
*
* DESCRIPTION
*
*       Initialization function for an MP device. It does the required
*       initializations and creates a new bundle and associates the MP
*       device with that bundle.
*
* INPUTS
*
*       *mp_dev                 MP virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS              Initialization was successful.
*       NU_PPP_INIT_FAILURE     Error in initialization of PPP module.
*       NU_NO_MEMORY            Memory not available.
*
*************************************************************************/
STATUS PPP_MP_Init(DV_DEVICE_ENTRY *mp_dev)
{
    /* Declare variable. */
    STATUS       status;

    PrintInfo("Initializing MP virtual interface.\n");

    /* Set to the device input and output functions. */
    mp_dev->dev_output = PPP_MP_Output;
    mp_dev->dev_input  = PPP_MP_Input;

    /* Allocate memory for the PPP layer structure. This structure will be
    pointed to by the device structure for this device.. */
    status = NU_Allocate_Memory(MEM_Cached,(VOID **)&mp_dev->dev_link_layer,
                                sizeof(LINK_LAYER), (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for PPP data structure.",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Set the status. */
        status = NU_NO_MEMORY;
    }

    else
    {
        /* Zero out the PPP layer information. */
        UTL_Zero(mp_dev->dev_link_layer, sizeof (LINK_LAYER));

        /* Initialize PPP layer for this MP device. */
        status = PPP_Initialize(mp_dev);

        /* Check if the PPP initialization was successful. */
        if (status == NU_SUCCESS)
        {
            /* Set the type of this MP device as virtual. */
            ((LINK_LAYER *)mp_dev->dev_link_layer)->hwi.itype |=
                PPP_ITYPE_VIRTUAL;

            /* Create a new bundle for the MP device. */
            status = PPP_MP_Create_Bundle(mp_dev->dev_index);
        }
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Init */

/*************************************************************************
* FUNCTION
*
*     PPP_MP_Attach_Link
*
* DESCRIPTION
*
*      This function associates a device (link) with MP virtual interface.
*
* INPUTS
*
*      *mp_dev                  MP virtual interface device to associate
*                               the PPP link to.
*      *ppp_dev                 PPP device to be added to the bundle and
*                               on which a call will to placed.
*
*
* OUTPUTS
*
*      NU_SUCCESS               PPP device has been associated with
*                               the MP device.
*      NU_INVALID_LINK          The link supplied is not a valid PPP
*                               device or MP device.
*
*************************************************************************/
STATUS PPP_MP_Attach_Link(DV_DEVICE_ENTRY *mp_dev,
                          DV_DEVICE_ENTRY *ppp_dev)
{
    /* Declare Variables. */
    MIB2_IF_STACK_STRUCT  stack_entry;
    LCP_LAYER             *lcp;
    STATUS                status = NU_INVALID_LINK;
    UINT8                 mac_address[] = PPP_MP_END_MAC;

    /* If we have got the PPP device. */
    if ( (ppp_dev) && (mp_dev) )
    {
        /* Get a pointer to the LCP layer of the PPP device. */
        lcp = &(((LINK_LAYER *)ppp_dev->dev_link_layer)->lcp);

        /* Set link options to negotiate MP protocol on this link. */
        lcp->options.local.default_flags |= (PPP_FLAG_MRRU |
                                             PPP_FLAG_ENDPOINT_DISC);

        lcp->options.remote.default_flags |= PPP_FLAG_MRRU;

#if (PPP_MP_DEFAULT_USE_SHORT_SEQ_NUM == NU_TRUE)
        /* Set the use of the short sequence number header format. */
        lcp->options.local.default_flags |= PPP_FLAG_SHORT_SEQ_NUM;
#endif

        /* Set the MRRU of this link. */
        lcp->options.local.mp_mrru = PPP_MP_DEFAULT_MRRU_SIZE;

        /* Set the endpoint discriminator, MAC address, for this link. */
        NU_BLOCK_COPY(lcp->options.local.mp_endpoint_disc, mac_address,
                      LCP_ENDPOINT_DISC_MAC_SIZE);

        /* Set the length of the endpoint discriminator. */
        lcp->options.local.mp_endpoint_disc_len =
            LCP_ENDPOINT_DISC_MAC_SIZE;

        /* Set the endpoint discriminator class. */
        lcp->options.local.mp_endpoint_class = LCP_ENDPOINT_DISC_MAC;

        /* Set the login and password on the PPP link. This will be same
        as that of the MP virtual device. */
        PPP_Set_Login(((LINK_LAYER *)mp_dev->dev_link_layer)->authentication.login_name,
                      ((LINK_LAYER *)mp_dev->dev_link_layer)->authentication.login_pw,
                      ppp_dev);

        /* Set the interface stack entry; higher layer will be MP device. */
        stack_entry.mib2_higher_layer = mp_dev;

        /* Lower layer is the PPP device. */
        stack_entry.mib2_lower_layer = ppp_dev;

        /* Put the stack entry in the NET interface stack. */
        status = MIB2_If_Stack_Add_Entry(&stack_entry);
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Attach_Link */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Add_Link
*
* DESCRIPTION
*
*       This function adds the new link negotiated to the existing bundle.
*
* INPUTS
*
*       *ppp_dev                PPP device (link) to add to bundle.
*       *mp_dev                 MP device to associate this
*                               link to. If not specified (i.e.
*                               NU_NULL) then a new MP device is
*                               created
* OUTPUTS
*
*       NU_SUCCESS              If a new PPP link has been added to a
*                               bundle.
*       NU_INVALID_LINK         The PPP/MP link specified does not
*                               exist.
*
*
*************************************************************************/
STATUS PPP_MP_Add_Link(DV_DEVICE_ENTRY *ppp_dev, DV_DEVICE_ENTRY *mp_dev)
{
    /* Declaring Variables. */
    NU_DEVICE                   mp_new_device;
    MIB2_IF_STACK_STRUCT        stack_entry;
    PPP_MP_BUNDLE               *bundle = NU_NULL;
    STATUS                      status;

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }

    /* Get the bundle associated with the device. */
    status = PPP_MP_Get_Bundle(&bundle, ppp_dev, mp_dev);

    /* If a match is not found then create a new bundle in the bundle list
     */
    if ( (bundle == NU_NULL) && (status == NU_NOT_PRESENT) )
    {
        /* Release the semaphore acquired by the calling function. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* create a new device. */
        mp_new_device.dv_name = "__MP__";
        mp_new_device.dv_init = PPP_MP_Init;
        mp_new_device.dv_flags = DV_VIRTUAL_DEV | DV_POINTTOPOINT;

        mp_new_device.dv_flags |= ppp_dev->dev_flags;

        /* Register the device. */
        status = DEV_Init_Devices(&mp_new_device, 1);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to initialize MP virtual device ",
                           NERR_FATAL, __FILE__, __LINE__);
        }

        /* Obtain the semaphore again. */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* Get the MP interface created . */
        mp_dev = DEV_Get_Dev_By_Name(mp_new_device.dv_name);

        /* Check if the MP device has been created */
        if (mp_dev)
        {
            /* Append the device index to the name for uniqueness. */
            strcpy(mp_dev->dev_net_if_name, "__MP__");
            NU_ITOA(mp_dev->dev_index, &mp_dev->dev_net_if_name[6], 10);

            /* Get the bundle associated with the virtual interface . */
            bundle = (PPP_MP_BUNDLE *)mp_dev->dev_extension;

            /* Set the interface stack entry; higher layer will be MP device
             */
            stack_entry.mib2_higher_layer = mp_dev;

            /* Lower layer is the PPP device. */
            stack_entry.mib2_lower_layer = ppp_dev;

            /* Put the stack entry in the NET interface stack. */
            status = MIB2_If_Stack_Add_Entry(&stack_entry);
        }
    }

    /* If a bundle is found and the MP device is passed NULL as a parameter. */
    else if ( (mp_dev == NU_NULL) && (status == NU_SUCCESS) )
    {
        /* Set the interface stack entry; higher layer will be MP device */
        stack_entry.mib2_higher_layer = DEV_Get_Dev_By_Index(bundle->mp_device_ifindex);

        /* Lower layer is the PPP device. */
        stack_entry.mib2_lower_layer = ppp_dev;

        /* Put the stack entry in the NET interface stack. */
        status = MIB2_If_Stack_Add_Entry(&stack_entry);
    }

    /* If there are no links already associated with the bundle. */
    if ( (status == NU_SUCCESS) && (bundle != NU_NULL) &&
         (bundle->mp_num_links == 0) )
    {
        /* Copy the entire link layer structure to the MP device. */
        NU_BLOCK_COPY(mp_dev->dev_link_layer,ppp_dev->dev_link_layer,
                      sizeof(LINK_LAYER));

        /* Check if the MRRU is greater than the minimum reassembly size. */
        if (((LINK_LAYER *)mp_dev->dev_link_layer)->lcp.options.local.mp_mrru
            > MIN_REASM_MAX_SIZE)
        {
           /* Set the max reassembly size on our side. */
           mp_dev->dev_reasm_max_size =
               ((LINK_LAYER *)mp_dev->dev_link_layer)->lcp.options.local.mp_mrru;
        }

        /* Set the type of the MP device as virtual. */
        ((LINK_LAYER *)mp_dev->dev_link_layer)->hwi.itype =
            PPP_ITYPE_VIRTUAL;
    }

    if (status == NU_SUCCESS)
    {
        if (mp_dev == NU_NULL)
        {
            /* Get a pointer to the MP virtual interface. */
            mp_dev = DEV_Get_Dev_By_Index(bundle->mp_device_ifindex);
        }

        /* Indicate that the PPP device is up. */
        ppp_dev->dev_flags |= DV_UP;

        /* If the stack entry is successfully placed in the interface
        stack and there are already some links present in the bundle. */
        if (bundle->mp_num_links)
        {
            /* Set the data size which the peer can reassemble at its end.
             */
            mp_dev->dev_mtu =
                ((LINK_LAYER *)mp_dev->dev_link_layer)->lcp.options.remote.mp_mrru;

            /* Copy the LCP local options in case they are changed. */
            NU_BLOCK_COPY(&((LINK_LAYER *)mp_dev->dev_link_layer)->
                          lcp.options.local,
                          &((LINK_LAYER *)ppp_dev->dev_link_layer)->
                          lcp.options.local, sizeof(LCP_OPTIONS));

            /* Copy the LCP remote options in case they are changed. */
            NU_BLOCK_COPY(&((LINK_LAYER *)mp_dev->dev_link_layer)->
                          lcp.options.remote,
                          &((LINK_LAYER *)ppp_dev->dev_link_layer)->
                          lcp.options.remote, sizeof(LCP_OPTIONS));
        }

        /* Update the header len. This is only done when there are more
         * than one links associated with a bundle. If there is only one
         * link then MP header is not added to the outgoing packet.
         */
        if (mp_dev->dev_hdrlen < ppp_dev->dev_hdrlen)
        {
            mp_dev->dev_hdrlen = ppp_dev->dev_hdrlen;
        }

        /* Increment the number of links associated with the bundle. */
        bundle->mp_num_links++;

        /* Update the baud rate of the MP device. */
        bundle->mp_total_baud_rate += ppp_dev->dev_baud_rate;

        /* Activate the stack entry. */
        MIB2_IF_STACK_ACTIVATE(MIB2_If_Stack_Get_Entry(mp_dev->dev_index + 1,
                                                       ppp_dev->dev_index + 1));

        PrintInfo("   The new PPP link has been added to a bundle.\n");
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Add_Link */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Remove_Link
*
* DESCRIPTION
*
*       This function removes a link from the bundle, and if all the
*       links are removed then the bundle is removed.
*
* INPUTS
*
*       ppp_ifindex             Interface index of the PPP device to be
*                               removed from a link.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PPP_MP_Remove_Link(UINT32 ppp_ifindex)
{
    /* Declaring variables. */
    DV_DEVICE_ENTRY             *device, *mp_dev;
    MIB2_IF_STACK_STRUCT        *stack_entry;
    PPP_MP_BUNDLE               *bundle;

    /* Get the device given its device index. */
    device = DEV_Get_Dev_By_Index(ppp_ifindex);

    /* Get the MP device associated with the link. */
    stack_entry = MIB2_If_Stack_Get_LI_Entry(ppp_ifindex + 1, NU_TRUE);

    /* If we have got the stack entry. */
    if ( (stack_entry) && (stack_entry->mib2_higher_layer) )
    {
        /* Get the pointer to the MP interface. */
        mp_dev = stack_entry->mib2_higher_layer;

        /* Obtain the MP semaphore. */
        if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
        }

        /* Get the bundle associated with the MP interface */
        bundle = (PPP_MP_BUNDLE *)mp_dev->dev_extension;

        /* Check if the bundle is found. */
        if (bundle)
        {
            /* Remove the link from the interface stack which is under the
             * MP device and check if the link is successfully removed from
             * the stack.
             */
            if (MIB2_If_Stack_Remove_Entry(stack_entry) == NU_SUCCESS)
            {
                /* Update the number of links associated with the bundle. */
                bundle->mp_num_links--;

                /* Update the baud rate of the MP device. */
                bundle->mp_total_baud_rate -= device->dev_baud_rate;

                PrintInfo("Removed PPP link from the bundle.\n");

                /* Check if there are no links associated with the bundle. */
                if (bundle->mp_num_links == 0)
                {
                    /* Detach the IP address and all routes associated
                    with the virtual device. */
                    NCP_Clean_Up_Link(mp_dev);

                    /* Set the state of the MP device connection. */
                    ((LINK_LAYER *)mp_dev->dev_link_layer)->hwi.state = INITIAL;

                    /* If that was the only link in the bundle and the
                     * bundle was created in response to the remote client
                     * connection, remove the bundle.
                     */
                    if (memcmp(mp_dev->dev_net_if_name, "__MP__", 6) == 0)
                    {
                        /* Remove the bundle reference in the device. */
                        mp_dev->dev_extension = NU_NULL;

                        /* Remove it from the bundle list. */
                        DLL_Remove(&MP_Bundle_List, bundle);

                        /* Deallocate the memory for the bundle. */
                        if (NU_Deallocate_Memory((VOID *)bundle) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to deallocate memory for the bundle",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        /* Remove the MP virtual device. */
                        DEV_Remove_Device(mp_dev, mp_dev->dev_flags);
                    }
                }
            }
        }

        /* Release the semaphore obtained above. */
        if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

} /* PPP_MP_Remove_Link. */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Create_Bundle
*
* DESCRIPTION
*
*       This function creates a new bundle and adds that to the bundle
*       list for the MP protocol.
*
* INPUTS
*
*       mp_if_index             MP device index.
*
* OUTPUTS
*
*       NU_SUCCESS              Bundle was successfully created.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS PPP_MP_Create_Bundle(UINT32 mp_if_index)
{
    /* Declare Variables. */
    PPP_MP_BUNDLE               *bundle;
    DV_DEVICE_ENTRY             *mp_device;
    STATUS                      status;

    /* Allocate memory for a new bundle. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&bundle,
                                sizeof(PPP_MP_BUNDLE),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to allocate memory for a new bundle ",
                       NERR_FATAL, __FILE__, __LINE__);
    }

    else
    {
        PrintInfo("A new bundle created for MP interface.\n");

        /* Clear (zero out) the bundle structure. */
        UTL_Zero(bundle, sizeof(PPP_MP_BUNDLE));

        /* Store the interface index of the MP interface in the bundle. */
        bundle->mp_device_ifindex = mp_if_index;

        /* Put this newly created bundle into the bundle list. */
        DLL_Enqueue(&MP_Bundle_List, bundle);

        /* Get the MP device. */
        mp_device = DEV_Get_Dev_By_Index(mp_if_index);

        /* Store the bundle pointer in the MP device */
        mp_device->dev_extension = bundle;
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Create_Bundle */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Get_Virt_If_By_Device
*
* DESCRIPTION
*
*       This function returns the MP virtual interface index associated
*       with the PPP device.
*
* INPUTS
*
*       *mp_ifindex             MP virtual interface index found.
*       *link_name              PPP link name.
*
*
* OUTPUTS
*
*       NU_SUCCESS              MP virtual interface is found.
*       NU_NOT_PRESENT          MP virtual interface could not be found.
*
*************************************************************************/
STATUS PPP_MP_Get_Virt_If_By_Device (UINT32 *mp_ifindex, CHAR *link_name)
{
    /* Declaring Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry;
    DV_DEVICE_ENTRY             *device;
    STATUS                      status = NU_NOT_PRESENT;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the pointer to the PPP device. */
    device = DEV_Get_Dev_By_Name(link_name);

    if (device)
    {
        /* Get the stack entry for this device. The higher interface will
        be the MP interface. */
        stack_entry =
            MIB2_If_Stack_Get_LI_Entry(device->dev_index + 1, NU_TRUE);

        /* If a stack entry exists. */
        if ( (stack_entry) && (stack_entry->mib2_higher_layer) )
        {
            /* Copy the index to the passed pointer. */
            *mp_ifindex = stack_entry->mib2_higher_layer->dev_index;

            /* Set the status. */
            status = NU_SUCCESS;
        }
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* PPP_MP_Get_Virt_If_By_Device */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Get_Virt_If_By_User
*
* DESCRIPTION
*
*       This function returns the MP virtual interface index associated
*       with the PPP User.
*
* INPUTS
*
*       *mp_ifindex             MP virtual interface index found.
*       *user_name              MP user name.
*
*
* OUTPUTS
*
*       NU_SUCCESS              MP virtual interface found.
*       NU_NOT_PRESENT          MP virtual interface is not found.
*
*************************************************************************/
STATUS PPP_MP_Get_Virt_If_By_User (UINT32 *mp_ifindex, CHAR *user_name)
{
    /* Declaring Variables. */
    PPP_MP_BUNDLE       *bundle;
    DV_DEVICE_ENTRY     *mp_device;
    STATUS              status = NU_NOT_PRESENT;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }

    /* Search the bundle list and return the MP virtual device interface
    index which matches the user name passed in. */
    for (bundle = MP_Bundle_List.mp_flink;
         bundle;
         bundle = bundle->mp_flink)
    {
        /* Get the pointer to the MP device. */
        mp_device = DEV_Get_Dev_By_Index (bundle->mp_device_ifindex);

        /* Match the user names. */
        if (NU_STRICMP(((LINK_LAYER *)mp_device->dev_link_layer)
                             ->authentication.login_name, user_name) == 0)
        {
            /* Copy the interface index. */
            *mp_ifindex = bundle->mp_device_ifindex;

            /* Match is found. */
            status = NU_SUCCESS;

            /* Come out of the loop. */
            break;
        }
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* PPP_MP_Get_Virt_If_By_User */

/*************************************************************************
*
* FUNCTION
*
*       PPP_MP_Get_Opt
*
* DESCRIPTION
*
*       This function gets MP bundle options.
*
* INPUTS
*
*       mp_ifindex              MP virtual interface index.
*       optname                 Integer specifying the option name.
*       *optval                 Pointer to the option value.
*       *optlen                 Specifies the size in bytes of the
*                               location pointed to by optval.
*
* OUTPUTS
*
*       NU_SUCCESS              Options successfully returned.
*       NU_INVALID_PARM         Invalid parameters.
*       NU_INVALID_OPTION       The option does not exist.
*       NU_NOT_PRESENT          MP interface passed could not be found.
*
*************************************************************************/
STATUS PPP_MP_Get_Opt(UINT32 mp_ifindex, INT optname, VOID *optval,
                      INT *optlen)
{
    /* Declaring Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry = NU_NULL;
    PPP_MP_BUNDLE               *bundle;
    DV_DEVICE_ENTRY             *mp_device;
    INT                         len;
    STATUS                      status = NU_SUCCESS;
    UINT8                       i;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check parameters passed in. */
    if ( (optval == NU_NULL) || (optlen == NU_NULL) )
    {
        /* Invalid parameters. */
        status = NU_INVALID_PARM;
    }

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the pointer to the MP device. */
    mp_device = DEV_Get_Dev_By_Index(mp_ifindex);

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
       NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                      __FILE__, __LINE__);
    }

    if (status == NU_SUCCESS)
    {
        /* Get the bundle associated with the MP device. */
        bundle = (PPP_MP_BUNDLE *)mp_device->dev_extension;
    }

    /* If a bundle associated with the MP interface is found. */
    if ( (bundle) && (status == NU_SUCCESS) )
    {
        switch(optname)
        {
            /* User name associated with the bundle. */
            case PPP_MP_USER_NAME:

                /* Check if the space to copy the value is enough. */
                if (strlen(((LINK_LAYER *)mp_device->dev_link_layer)
                    ->authentication.login_name) <= (UINT32)(*optlen))
                {
                    /* Copy the user name. */
                    strcpy((CHAR *)optval, ((LINK_LAYER *)mp_device->
                           dev_link_layer)->authentication.login_name);

                    /* Length of the user name copied. */
                    *optlen = (INT)(strlen(((LINK_LAYER *)mp_device->
                        dev_link_layer)->authentication.login_name));
                }
                else
                    status = NU_INVALID_PARM;

                break;

            case PPP_MP_END_DISCRIMINATOR:

                if (((LINK_LAYER*)mp_device->dev_link_layer)-> lcp.options.
                    remote.mp_endpoint_disc_len <= (UINT8)(*optlen))
                {
                    /* Put the length of the endpoint discriminator. */
                    *optlen = ((LINK_LAYER *)mp_device->dev_link_layer)->
                        lcp.options.remote.mp_endpoint_disc_len;

                    /* Copy the peer endpoint discriminator. */
                    NU_BLOCK_COPY(optval, ((LINK_LAYER *)mp_device->
                                  dev_link_layer)->lcp.options.remote.
                                  mp_endpoint_disc, (UINT32)(*optlen));
                }
                else
                    status = NU_INVALID_PARM;

                break;

            case PPP_MP_END_DISCRIMINATOR_CLASS:

                /* Put the value. */
                *(UINT8 *)optval = ((LINK_LAYER *)mp_device->
                    dev_link_layer)->lcp.options.remote.mp_endpoint_class;

                /* Put the length. */
                *optlen = 1;
                break;

            case PPP_MP_MRRU:

                if (*optlen >= 2)
                {
                    /* Put the value. */
                    *(UINT16 *)optval = ((LINK_LAYER*)mp_device->
                        dev_link_layer)->lcp.options.remote.mp_mrru;

                    *optlen = 2;
                }
                else
                    status = NU_INVALID_PARM;

                break;

            /* All links associated with the bundle separated by comma. */
            case PPP_MP_GET_ALL_LINKS:

                /* Clear the location. */
                strcpy(optval, "");

                /* Get the size of the space allocated. */
                len = *optlen;

                for (i = 0; i < bundle->mp_num_links; i++)
                {
                    /* Get the next stack entry. */
                    stack_entry =
                        MIB2_If_Stack_Get_Next_Entry2(stack_entry, NU_TRUE);

                    if (stack_entry == NU_NULL)
                    {
                        /* break out of the loop. */
                        break;
                    }

                    /* Check if we have got a stack entry with higher layer
                    same as MP virtual interface. */
                    if (stack_entry->mib2_higher_layer != mp_device)
                    {
                       /* Get the first link in the NET interface stack entry. */
                       stack_entry =
                           MIB2_If_Stack_Get_HI_Entry(mp_device->dev_index + 1,
                                                      NU_TRUE);
                    }

                    if ( (stack_entry) && (stack_entry->mib2_lower_layer) )
                    {
                        /* If the length provided is enough, then copy the
                        device name. */
                        if ((UINT32)len >= strlen(stack_entry->
                            mib2_lower_layer->dev_net_if_name))
                        {
                            strcat(optval, stack_entry->mib2_lower_layer->
                                   dev_net_if_name);

                            len -= (INT)(strlen(stack_entry->
                                mib2_lower_layer->dev_net_if_name));
                        }

                        /* Put the comma after the name. */
                        strcat(optval, ",");
                    }
                    else
                    {
                        break;
                    }
                }

                /* Update the bytes copied. */
                *optlen -= len;
                break;

            /* Interface index of the next MP interface. */
            case PPP_MP_NEXT_INTERFACE:

                /* Point to the next bundle in the bundle list. */
                bundle = bundle->mp_flink;

                /* If the bundle pointer is not null. */
                if (bundle)
                {
                    *(UINT32 *)optval = bundle->mp_device_ifindex;
                }

                /* Get the first node in the bundle list. */
                else
                {
                    *(UINT32 *)optval =
                        MP_Bundle_List.mp_flink->mp_device_ifindex;
                }

                /* Return the length. */
                *optlen = sizeof(UINT32);
                break;

            /* This case is handled at the end. */
            case PPP_MP_IS_INTERFACE:
                break;

            default:
                status = NU_INVALID_OPTION;
                break;
        }
    }

    /* If the option is to get whether the index passed in correspond to a
     * MP interface.
     */
    if ( (optname == PPP_MP_IS_INTERFACE) && (status == NU_SUCCESS) )
    {
        /* If a bundle was found for the MP device. */
        if(bundle)
        {
           *(UINT8 *)optval = NU_TRUE;
        }

        /* If a bundle was not found for the MP device. */
        else
        {
           *(UINT8 *)optval = NU_FALSE;
        }

        *optlen = 1;
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* PPP_MP_Get_Opt */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Find_Bundle_By_Device
*
* DESCRIPTION
*
*       This function finds the bundle associated with a PPP device.
*
* INPUTS
*
*       **bundle                Bundle returned which contains the device.
*       *dev_ptr                PPP device.
*
*
* OUTPUTS
*
*       NU_SUCCESS              Bundle is found.
*       NU_NOT_PRESENT          Bundle is not found.
*
*************************************************************************/
STATUS PPP_MP_Find_Bundle_By_Device(PPP_MP_BUNDLE **bundle,
                                    DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry;
    STATUS                      status = NU_NOT_PRESENT;

    /* Get the stack entry for this PPP device; the higher interface will
    be the MP interface. */
    stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1,
                                             NU_TRUE);

    if ( (stack_entry) && (stack_entry->mib2_higher_layer) )
    {
        /* Point to the MP device. */
        *bundle =
            (PPP_MP_BUNDLE *)(stack_entry->mib2_higher_layer->dev_extension);

        /* Set the status. */
        status = NU_SUCCESS;
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Find_Bundle_By_Device */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Get_Bundle
*
* DESCRIPTION
*
*       This function returns the bundle which is/can be associated
*       with the PPP/MP device.
*
* INPUTS
*
*       **bundle            Pointer to the bundle if found,
*                           otherwise NULL.
*       *dev_ptr            PPP device.
*       *mp_ptr             MP device.
*
* OUTPUTS
*
*       NU_SUCCESS          If bundle is found.
*       NU_NOT_PRESENT      If bundle is not found.
*       NU_INVALID_LINK     If the PPP device passed in cannot be
*                           associated to the MP device passed in.
*
*************************************************************************/
STATUS PPP_MP_Get_Bundle(PPP_MP_BUNDLE **bundle, DV_DEVICE_ENTRY *dev_ptr,
                         DV_DEVICE_ENTRY *mp_ptr)
{
    /* Declaring Variables. */
    DV_DEVICE_ENTRY     *mp_dev;
    LCP_LAYER           *ppp_lcp, *mp_lcp;
    STATUS              status = NU_NOT_PRESENT;

    /* Check if MP interface is passed in. */
    if (mp_ptr != NU_NULL)
    {
        /* Get the bundle associated with the MP device. */
        *bundle = (PPP_MP_BUNDLE *) mp_ptr->dev_extension;

        /* If there are already links present in the bundle then we need
        to match the user name and endpoint discriminators. This is done
        later. */
        if ((*bundle)->mp_num_links != 0)
        {
            *bundle = NU_NULL;
        }

        else
        {
            /* If this would be the only link in the bundle. */
            status = NU_SUCCESS;
        }
    }

    /* Check if we have got a bundle. */
    if (status != NU_SUCCESS)
    {
        /* Find the bundle associated with the MP device. */
        for (*bundle = MP_Bundle_List.mp_flink;
             *bundle;
             *bundle = (*bundle)->mp_flink)
        {
           /* Get the MP device. */
           mp_dev = DEV_Get_Dev_By_Index((*bundle)->mp_device_ifindex);

           /* Match the user names of the PPP device and the MP virtual
           device. */
           if (NU_STRICMP (((LINK_LAYER *)dev_ptr->dev_link_layer)->
               authentication.login_name, ((LINK_LAYER *)mp_dev->dev_link_layer)
               ->authentication.login_name) == 0)
           {
               /* Get the LCP layer for the MP device. */
               mp_lcp  = &((LINK_LAYER *)mp_dev->dev_link_layer)->lcp;

               /* Get the LCP layer for the PPP device. */
               ppp_lcp = &((LINK_LAYER *)dev_ptr->dev_link_layer)->lcp;

               /* Compare the endpoint discriminator and its class. */
               if ( (memcmp(mp_lcp->options.remote.mp_endpoint_disc,
                            ppp_lcp->options.remote.mp_endpoint_disc,
                            mp_lcp->options.remote.mp_endpoint_disc_len) == 0) &&
                    (mp_lcp->options.remote.mp_endpoint_class ==
                     ppp_lcp->options.remote.mp_endpoint_class) )
               {
                   /* Bundle found. */
                   status = NU_SUCCESS;
                   break;
               }
           }
        }
    }

    /* If an MP device was passed in then we need to check if the MP device
     * we have found matches with it.
     */
    if (mp_ptr)
    {
       /* If the MP device passed in and that of the bundle is same. */
       if ( (*bundle) &&
            ((*bundle)->mp_device_ifindex == mp_ptr->dev_index) )
       {
           /* Bundle found. */
           status = NU_SUCCESS;
       }
       else
       {
           /* Invalid MP device (mp_ptr) passed in. */
           status = NU_INVALID_LINK;
       }
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Get_Bundle */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Terminate_Links
*
* DESCRIPTION
*
*       This function terminates all the links in the bundle and then
*       deletes the bundle from the bundle list.
*
* INPUTS
*
*       mp_ifindex              Index of the MP interface with.
*
*
* OUTPUTS
*
*       NU_SUCCESS              Links were terminated successfully.
*       NU_NOT_PRESENT          MP interface not found or PPP link could
*                               not be removed.
*
*************************************************************************/
STATUS PPP_MP_Terminate_Links(UINT32 mp_ifindex)
{
    /* Declaring Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry;
    PPP_MP_BUNDLE               *bundle = NU_NULL;
    DV_DEVICE_ENTRY             *mp_dev;
    STATUS                      status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the pointer to the MP device. */
    mp_dev = DEV_Get_Dev_By_Index(mp_ifindex);

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
       NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                      __FILE__, __LINE__);
    }

    if (mp_dev)
    {
        /* Get the bundle associated with the MP device. */
        bundle = (PPP_MP_BUNDLE *)(mp_dev->dev_extension);
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    if (bundle)
    {
        /* One by one, hangup all the links associated with the bundle. */
        for (;;)
        {
            /* Get the stack entry for the MP interface associated with
            this bundle. */
            stack_entry = MIB2_If_Stack_Get_HI_Entry(mp_ifindex + 1,
                                                     NU_TRUE);

            /* If an entry is not found get out of the loop. */
            if ( (stack_entry) && (stack_entry->mib2_lower_layer == NU_NULL) )
            {
                break;
            }

            /* Hangup this link if it is up. */
            PPP_Hangup(NU_FORCE, stack_entry->mib2_lower_layer->dev_net_if_name);
        }
    }

    else
    {
        /* Set the status if the bundle is not present. */
        status = NU_NOT_PRESENT;
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* PPP_MP_Terminate_Links */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Output
*
* DESCRIPTION
*
*       This function adds the PPP header to the IP packet and then calls
*       fragment send function which sends the data on the links
*       associated with the MP virtual interface.
*
* INPUTS
*
*       *buf_ptr                Packet to be transmitted.
*       *dev_ptr                Virtual MP Interface on which to transmit
*                               the packet.
*       *unused_1               Unused Variable.
*       *unused_2               Unused Variable.
*
* OUTPUTS
*
*       NU_SUCCESS              Packet transmitted successfully.
*       NU_HOST_UNREACHABLE     The host is unreachable.
*
*************************************************************************/
STATUS PPP_MP_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *dev_ptr,
                     VOID *unused_1, VOID *unused_2)
{
    /* Declare Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry;
    PPP_MP_BUNDLE               *bundle;
    LINK_LAYER                  *link_layer;
    UINT16                      pkt_type;
    STATUS                      status = NU_SUCCESS;

    /* Remove warnings. */
    UNUSED_PARAMETER(unused_1);
    UNUSED_PARAMETER(unused_2);

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
       NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                      __FILE__, __LINE__);
    }

    /* Get the bundle associated with this virtual interface . */
    bundle = (PPP_MP_BUNDLE *)dev_ptr->dev_extension;

    if (bundle)
    {
        /* If there is only one PPP link associated with the bundle then
        there is no need to fragment the packet. */
        if (bundle->mp_num_links == 1)
        {
            /* Point to the PPP link in the NET interface stack, lower of
            the MP Device. */
            stack_entry =
                MIB2_If_Stack_Get_HI_Entry(bundle->mp_device_ifindex + 1,
                                           NU_TRUE);

            if ( (stack_entry) && (stack_entry->mib2_lower_layer) )
            {
                /* Send the packet over the PPP link. */
                status = (*(stack_entry->mib2_lower_layer->dev_output))
                  (buf_ptr, stack_entry->mib2_lower_layer, NU_NULL, NU_NULL);
            }
       }

       else
       {
            /* Point to the link layer of the MP interface. */
            link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;

            if (buf_ptr->mem_flags & (NET_IP | NET_IP6))
            {
                /* Verify that the device is up and running. */
                if ( ((dev_ptr->dev_flags & (DV_UP | DV_RUNNING)) ==
                     (DV_UP | DV_RUNNING)) && (link_layer->lcp.state == OPENED) )
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    if ( (buf_ptr->mem_flags & NET_IP) &&
                         (link_layer->ncp.state == OPENED) )
                        pkt_type = PPP_IP_PROTOCOL;
                    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                    if ( (buf_ptr->mem_flags & NET_IP6) &&
                         (link_layer->ncp6.state == OPENED) )
                        pkt_type = PPP_IPV6_PROTOCOL;
                    else
#endif
                        status = NU_HOST_UNREACHABLE;

                    if (status == NU_SUCCESS)
                    {
                        /* See if we need to compress the protocol header. */
                        if (link_layer->lcp.options.remote.flags & PPP_FLAG_PFC)
                        {
                            buf_ptr->data_ptr           -= PPP_PROTOCOL_HEADER_1BYTE;
                            buf_ptr->data_len           += PPP_PROTOCOL_HEADER_1BYTE;
                            buf_ptr->mem_total_data_len += PPP_PROTOCOL_HEADER_1BYTE;

                            /* Add the protocol header. */
                            buf_ptr->data_ptr[0] = (UINT8)pkt_type;
                        }
                        else
                            /* Add PPP protocol header. */
                            PPP_Add_Protocol(buf_ptr, pkt_type);

                        /* If it is, update the last_activity_time variable
                        which maintains the time of the last IP activity
                        through the PPP link. */
                        link_layer->last_activity_time = NU_Retrieve_Clock();
                    }
                }
                else
                    status = NU_HOST_UNREACHABLE;
            }

            if (status == NU_SUCCESS)
            {
                /* Fragment the packet and send it over the links
                associated with the MP virtual interface. */
                status = PPP_MP_Fragment_Send(buf_ptr, dev_ptr);
            }
       }
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Output */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Input
*
* DESCRIPTION
*
*       This function calls the MP de-fragmentation function which returns
*       a pointer to a complete packet if the packet is complete and
*       NU_NULL otherwise. If the packet is complete then the PPP_Input
*       function is called which calls the correct protocol routine to
*       handle the PPP encapsulated packet.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS PPP_MP_Input(VOID)
{
    /* Declaring Variables. */
    NET_BUFFER           *packet;
    INT                  old_level;
    STATUS               status = NU_SUCCESS;

    /* Add the fragment into the fragment list associated with a bundle */
    packet = PPP_MP_New_Fragment();

    /* If a packet has been completed. */
    if (packet)
    {
        /* Before passing the reassembled packet to the IP interpreter,
         * we must put this packet onto the head of the received buffer
         * list. This is done because all the upper layers assume that
         * the packet they are processing is the first one on the list.
         * Turn off interrupts while we are modifying the buffer list.
         */

        /* Temporarily lockout interrupts to protect the global buffer
        variables. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Point this one to the head of the list. */
        packet->next = MEM_Buffer_List.head;

        /* Point the head to this one. */
        MEM_Buffer_List.head = packet;

        /* If there is nothing on the list, then point the tail to this
        one. Otherwise, the tail does not need to be touched. */
        if (MEM_Buffer_List.tail == NU_NULL)
        {
            MEM_Buffer_List.tail = packet;
        }

        /* We are done. Turn interrupts back on. */
        NU_Local_Control_Interrupts(old_level);

        /* Set the packet's buffers free list. */
        packet->mem_dlist = &MEM_Buffer_Freelist;

        /* Call the PPP layer to handle this packet. */
        status = PPP_Input();
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Input */

#endif
