/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       ip6_sck_smi.c
*
*   DESCRIPTION
*
*       This file contains the routine for setting the multicast
*       interface.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Set_IPV6_MULTICAST_IF
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Set_IPV6_MULTICAST_IF
*
*   DESCRIPTION
*
*       This function sets the multicast interface for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       if_index                The interface index of the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                if_index is invalid.
*
*************************************************************************/
STATUS IP6_Set_IPV6_MULTICAST_IF(INT socketd, INT32 if_index)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS   *moptions;
    STATUS              status = NU_SUCCESS;

    /* Is there a multicast option buffer attached to the socket. */
    if (sck_ptr->s_moptions_v6 == NU_NULL)
    {
        /* Allocate a multicast option buffer. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&sck_ptr->s_moptions_v6,
                               sizeof(MULTI_SCK_OPTIONS),
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
            return (NU_MEM_ALLOC);

        /* Initialize the option buffer to the default values. */
        moptions = sck_ptr->s_moptions_v6;
        UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
        moptions->multio_device = NU_NULL;
        moptions->multio_hop_lmt = IP6_DEFAULT_MULTICAST_TTL;
        moptions->multio_loop = IP6_DEFAULT_MULTICAST_LOOP;
        moptions->multio_num_mem_v6 = 0;
    }
    else
        moptions = sck_ptr->s_moptions_v6;

    /* If the index is -1 then just remove the interface */
    if (if_index == -1)
        moptions->multio_device = NU_NULL;
    else
    {
        moptions->multio_device = DEV_Get_Dev_By_Index((UINT32)if_index);

        /* The interface does not exist on the node or the interface is not
         * enabled for IPv6.
         */
        if ( (!moptions->multio_device) ||
             (!(moptions->multio_device->dev_flags & DV6_IPV6)) )
        {
            status = NU_INVAL;
        }
    }

    /* If all the options have default values, then there is no need
     * to keep the structure.
     */
    if ( (moptions->multio_device == NU_NULL) &&
         (moptions->multio_hop_lmt == IP6_DEFAULT_MULTICAST_TTL) &&
         (moptions->multio_loop == IP6_DEFAULT_MULTICAST_LOOP) &&
         (moptions->multio_num_mem_v6 == 0) )
    {
        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for multicast options",
                           NERR_SEVERE, __FILE__, __LINE__);

        sck_ptr->s_moptions_v6 = NU_NULL;
    }

    return (status);

} /* IP6_Set_IPV6_MULTICAST_IF */
