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
*
*   FILE NAME
*
*       ip_smttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the multicast Time To Live
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_MULTICAST_TTL
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
extern MULTI_SCK_OPTIONS NET_Amm_Memory[];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       IP_Setsockopt_IP_MULTICAST_TTL
*
*   DESCRIPTION
*
*       This function sets the multicast TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       sck_ttl                 The new Time To Live.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*
*************************************************************************/
STATUS IP_Setsockopt_IP_MULTICAST_TTL(INT socketd, UINT8 sck_ttl)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS   *moptions;

    /* Is there a multicast option buffer attached to the socket. */
    if (sck_ptr->s_moptions_v4 == NU_NULL)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Allocate a multicast option buffer. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&sck_ptr->s_moptions_v4,
                               sizeof(*sck_ptr->s_moptions_v4),
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
            return (NU_MEM_ALLOC);
#else
        /* Assign memory to the multicast options*/
        sck_ptr->s_moptions_v4 = &NET_Amm_Memory[socketd];
#endif

        /* Initialize the option buffer to the default values. */
        moptions = sck_ptr->s_moptions_v4;

        UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
        moptions->multio_device = NU_NULL;
        moptions->multio_ttl = IP_DEFAULT_MULTICAST_TTL;
        moptions->multio_loop = IP_DEFAULT_MULTICAST_LOOP;
        moptions->multio_num_mem_v4 = 0;
    }
    else
        moptions = sck_ptr->s_moptions_v4;

    /* Set the new TTL. */
    moptions->multio_ttl = sck_ttl;

    /* If all the options have default values, then there is no need to
     * keep the structure.
     */
    if ( (moptions->multio_device == NU_NULL) &&
         (moptions->multio_ttl == IP_DEFAULT_MULTICAST_TTL) &&
         (moptions->multio_loop == IP_DEFAULT_MULTICAST_LOOP) &&
         (moptions->multio_num_mem_v4 == 0) )
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for multicast options",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif
        sck_ptr->s_moptions_v4 = NU_NULL;
    }

    return (NU_SUCCESS);

} /* IP_Setsockopt_IP_MULTICAST_TTL */

#endif
