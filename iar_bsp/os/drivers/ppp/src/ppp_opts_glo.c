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
* FILE NAME
*
*       ppp_opts_glo.c
*
* COMPONENT
*
*       PPP - API
*
* DESCRIPTION
*
*       This file contains the API function get PPP link options.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PPP_Get_Link_Option
*
* DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/

#include "drivers/nu_ppp.h"

/*************************************************************************
*
*   FUNCTION
*
*       PPP_Get_Link_Option
*
*   DESCRIPTION
*
*       This function gets PPP Link options.
*
*   INPUTS
*
*       *link_name            Name of the tunnel
*       optname                 The option name itself
*       *optval                 Pointer to the option value
*       *optlen                 Pointer to the size of the area pointed
*                               to by optval
*
*   OUTPUTS
*
*       NU_SUCCESS              Successfully retrieved the value.
*       NU_PPP_ENTRY_NOT_FOUND  A valid entry was not found.
*       NU_PPP_INVALID_OPTION   The option specified is invalid.
*       NU_PPP_INVALID_PARAMS   Either optval or optlen is invalid.
*
*************************************************************************/
STATUS PPP_Get_Link_Option(CHAR *link_name, INT optname, VOID *optval,
                           INT *optlen)
{
    LCP_LAYER       *lcp;

#if (INCLUDE_IPV4 == NU_TRUE)
    NCP_LAYER       *ncp;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
    CCP_LAYER       *ccp;
#endif

    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;

    /* Status to be returned. */
    STATUS          status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();                   /* switch to supervisor mode */

    if ((link_name == NU_NULL) ||(optval == NU_NULL) ||
        (optlen == NU_NULL))
    {
        status = NU_PPP_INVALID_PARAMS;
    }

    if (status == NU_SUCCESS)
    {
        /* Grab the semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    }

    if(status == NU_SUCCESS)
    {
        /* Find the device for this link name. */
        dev_ptr = DEV_Get_Dev_By_Name(link_name);

        /* Make sure the device was found and that it is a PPP device. */
        if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
        {
            link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

            /* Get pointers to the LCP, CCP and NCP structures. */
            lcp = &link_layer->lcp;

#if (PPP_ENABLE_MPPE == NU_TRUE)
            ccp = &link_layer->ccp;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            ncp = &link_layer->ncp;
#endif
        }
        else
        {
            status = NU_PPP_ENTRY_NOT_FOUND;
        }

        /* If the required PPP device was successfully retrieved */
        if (status == NU_SUCCESS)
        {
            /* Get the value of option specified by the user. */
            switch(optname)
            {
            case PPP_DEFAULT_ACCM:

                if (*optlen >= (INT)sizeof(UINT32))
                {
                    *(UINT32*)optval = lcp->options.local.default_accm;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT32);

                break;

            case PPP_DEFAULT_AUTH_PROTOCOL:

                if (*optlen >= (INT)sizeof(UINT16))
                {
                    /* authentication protocol, MSCHAPv2 */
                    if(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS2)
                        *(UINT16*) optval = PPP_CHAP_MS2_PROTOCOL;

                    /* authentication protocol, MSCHAPv1 */
                    else if(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS1)
                        *(UINT16*) optval = PPP_CHAP_MS1_PROTOCOL;

                    /* authentication protocol, CHAP */
                    else if(lcp->options.local.default_flags & PPP_FLAG_CHAP)
                        *(UINT16*) optval = PPP_CHAP_PROTOCOL;

                    /* authentication protocol, PAP */
                    else if(lcp->options.local.default_flags & PPP_FLAG_PAP)
                        *(UINT16*) optval = PPP_PAP_PROTOCOL;

                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT16);

                break;

            case PPP_DEFAULT_FCS:

                if (*optlen >= (INT)sizeof(UINT32))
                {
                    *(UINT32*) optval = lcp->options.local.default_fcs_size;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT32);

                break;

            case PPP_DEFAULT_MRU:

                if (*optlen >= (INT)sizeof(UINT16))
                {
                    *(UINT16*) optval = lcp->options.local.default_mru;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT16);

                break;


            case PPP_NUM_OF_DNS_SERVERS:

                if (*optlen >= (INT)sizeof(UINT32))
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    /* Request DNS server addresses. */
                    if(ncp->options.ipcp.default_flags & (PPP_FLAG_DNS1 | PPP_FLAG_DNS2))
                    {
                        *(UINT32*) optval = 2;
                    }

                    else if(ncp->options.ipcp.default_flags & PPP_FLAG_DNS1)
                    {
                        *(UINT32*) optval = 1;
                    }
#else
                    *(UINT32*) optval = 0;
#endif
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT32);

                break;

#if(INCLUDE_PPP_MP == NU_TRUE)
            case PPP_DEFAULT_MP_MRRU:

                if (*optlen >= (INT)sizeof(UINT16))
                {
                    *(UINT16*) optval = lcp->options.local.mp_mrru;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                *optlen = sizeof(UINT16);

                break;

            case PPP_ENABLE_MP_MRRU:

                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (lcp->options.local.default_flags & PPP_FLAG_MRRU)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;
#endif
#if (PPP_ENABLE_MPPE == NU_TRUE)

            case PPP_ENABLE_128BIT_ENCRYPTION:
                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (ccp->options.local.mppe.mppe_default_flags & 
                        CCP_FLAG_S_BIT)
                    {
                        *(UINT16*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT16*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_56BIT_ENCRYPTION:
                if (*optlen >= (INT)sizeof(UINT8))
                {

                    if (ccp->options.local.mppe.mppe_default_flags &
                        CCP_FLAG_M_BIT)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }

                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_40BIT_ENCRYPTION:
                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (ccp->options.local.mppe.mppe_default_flags &
                        CCP_FLAG_L_BIT)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_ENCRYPTION_REQUIRED:
                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (ccp->options.local.mppe.mppe_require_encryption 
                        == 1)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

#endif

            case PPP_ENABLE_ACC:
                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (lcp->options.local.default_flags & PPP_FLAG_ACC)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_PFC:
                /* Protocol field compression. */
                if (*optlen >= (INT)sizeof(UINT8))
                {
                    if (lcp->options.local.default_flags & PPP_FLAG_PFC)
                    {
                        *(UINT8*) optval = NU_TRUE;
                    }
                    else
                    {
                        *(UINT8*) optval = NU_FALSE;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            default:
                status = NU_PPP_INVALID_OPTION;
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
            __FILE__, __LINE__);
    }

    NU_USER_MODE();                         /* return to user mode */

    /* return status */
    return (status);

} /* PPP_Get_Link_Option */

