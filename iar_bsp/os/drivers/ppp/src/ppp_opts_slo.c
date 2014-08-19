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
*       ppp_opts_slo.c
*
* COMPONENT
*
*       PPP - API
*
* DESCRIPTION
*
*       This file contains the API function to set PPP link options.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PPP_Set_Link_Option
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
*       PPP_Set_Link_Option
*
*   DESCRIPTION
*
*       This function sets PPP Link options.
*
*   INPUTS
*
*       *link_name                      Name of the device
*       optname                         The option name itself
*       *optval                         Pointer to the option value
*       optlen                          Size of the area pointed to 
*                                       by optval
*
*   OUTPUTS
*
*       NU_SUCCESS                      Value was successfully set.
*       NU_PPP_ENTRY_NOT_FOUND          A valid entry was not found.
*       NU_PPP_INVALID_OPTION           The option specified is invalid.
*       NU_PPP_INVALID_PARAMS           optval or optlen is invalid.
*
*************************************************************************/
STATUS PPP_Set_Link_Option(CHAR *link_name, INT optname, VOID *optval,
                           INT optlen)
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
    STATUS          status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();                  /* switch to supervisor mode */

    if ((link_name == NU_NULL) ||(optval == NU_NULL))
    {
        status = NU_PPP_INVALID_PARAMS;
    }
    else
    {
        /* Grab the semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
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

                if (optlen == sizeof(UINT32))
                {
                    lcp->options.local.default_accm = *(UINT32*)optval;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_DEFAULT_FCS:

                if (optlen == (INT)sizeof(UINT32))
                {
                    lcp->options.local.default_fcs_size = 
                        *(UINT32*) optval;
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_DEFAULT_MRU:

                if (optlen == (INT)sizeof(UINT16))
                {
                    if ((*(UINT16*) optval > 0) && 
                        (*(UINT16*) optval <= PPP_MRU))
                    {
                        lcp->options.local.default_mru = 
                            *(UINT16*) optval;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_NUM_OF_DNS_SERVERS:

                if (optlen == (INT)sizeof(UINT32))
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    if(*(UINT32*) optval == 1)
                    {
                        ncp->options.ipcp.default_flags |= PPP_FLAG_DNS1;
                    }

                    else if(*(UINT32*) optval == 2)
                    {
                        ncp->options.ipcp.default_flags |= 
                            (PPP_FLAG_DNS1 | PPP_FLAG_DNS2);
                    }
                    else if(*(UINT32*) optval == 0)
                    {
                        ncp->options.ipcp.default_flags &= 
                            ~(PPP_FLAG_DNS1 | PPP_FLAG_DNS2);
                    }
                    else
                    {
                        status = (NU_PPP_INVALID_PARAMS);
                    }
#endif
                }
                else
                {
                    status = (NU_PPP_INVALID_PARAMS);
                }

                break;

#if(INCLUDE_PPP_MP == NU_TRUE)
            case PPP_DEFAULT_MP_MRRU:

                if (optlen == (INT)sizeof(UINT16))
                {
                    lcp->options.local.mp_mrru = *(UINT16*) optval;
                }
                else
                {
                    status = (NU_PPP_INVALID_PARAMS);
                }

                break;
#endif
            case PPP_ENABLE_PFC:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_PFC;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= ~PPP_FLAG_PFC;
                    }

                    else
                    {
                        status = (NU_PPP_INVALID_PARAMS);
                    }
                }
                else
                {
                    status = (NU_PPP_INVALID_PARAMS);
                }

                break;

            case PPP_ENABLE_ACC:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_ACC;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= ~PPP_FLAG_ACC;
                    }

                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_MAGIC_NUMBER:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= 
                            PPP_FLAG_MAGIC;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= 
                            ~PPP_FLAG_MAGIC;
                    }

                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

            case PPP_ENABLE_ACCM:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_ACCM;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= 
                            ~PPP_FLAG_ACCM;
                    }

                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_MRU:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_MRU;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= ~PPP_FLAG_MRU;
                    }

                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_DEFAULT_AUTH_PROTOCOL:

                if (optlen == (INT)sizeof(UINT16))
                {
                    /* authentication protocol, MSCHAPv2 */
                    if ((*(UINT16*) optval == NU_PPP_CHAP_MS2_PROTOCOL))
                    {
                        lcp->options.local.default_flags |= 
                            PPP_FLAG_CHAP_MS2;
                    }
                    /* authentication protocol, MSCHAPv1 */
                    else if ((*(UINT16*) optval == 
                        NU_PPP_CHAP_MS1_PROTOCOL))
                    {
                        lcp->options.local.default_flags |= 
                            PPP_FLAG_CHAP_MS1;
                    }

                    /* authentication protocol, CHAP */
                    else if ((*(UINT16*) optval == NU_PPP_CHAP_PROTOCOL))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_CHAP;
                    }

                    /* authentication protocol, PAP */
                    else if ((*(UINT16*) optval == NU_PPP_PAP_PROTOCOL))
                    {
                        lcp->options.local.default_flags |= PPP_FLAG_PAP;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;


#if(INCLUDE_PPP_MP == NU_TRUE)
            case PPP_ENABLE_MP_MRRU:

                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval == NU_TRUE))
                    {
                        lcp->options.local.default_flags |= 
                            PPP_FLAG_MRRU;
                    }

                    else if ((*(UINT8*) optval == NU_FALSE))
                    {
                        lcp->options.local.default_flags &= 
                            ~PPP_FLAG_MRRU;
                    }

                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
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
                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval) == NU_TRUE)
                    {
                        ccp->options.local.mppe.mppe_default_flags |= 
                            CCP_FLAG_S_BIT;
                    }
                    else if ((*(UINT8*) optval) == NU_FALSE)
                    {
                        ccp->options.local.mppe.mppe_default_flags &= 
                            ~CCP_FLAG_S_BIT;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;
            case PPP_ENABLE_56BIT_ENCRYPTION:
                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval) == NU_TRUE)
                    {
                        ccp->options.local.mppe.mppe_default_flags |= 
                            CCP_FLAG_M_BIT;
                    }
                    else if ((*(UINT8*) optval) == NU_FALSE)
                    {
                        ccp->options.local.mppe.mppe_default_flags &= 
                            ~CCP_FLAG_M_BIT;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_40BIT_ENCRYPTION:
                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval) == NU_TRUE)
                    {
                        ccp->options.local.mppe.mppe_default_flags |= 
                            CCP_FLAG_L_BIT;
                    }
                    else if ((*(UINT8*) optval) == NU_FALSE)
                    {
                        ccp->options.local.mppe.mppe_default_flags &= 
                            ~CCP_FLAG_L_BIT;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;

            case PPP_ENABLE_ENCRYPTION_REQUIRED:
                if (optlen == (INT)sizeof(UINT8))
                {
                    if ((*(UINT8*) optval) == NU_TRUE)
                    {
                        ccp->options.local.mppe.mppe_require_encryption
                            = 1;
                    }
                    else if ((*(UINT8*) optval) == NU_FALSE)
                    {
                        ccp->options.local.mppe.mppe_require_encryption
                            = 0;
                    }
                    else
                    {
                        status = NU_PPP_INVALID_PARAMS;
                    }
                }
                else
                {
                    status = NU_PPP_INVALID_PARAMS;
                }

                break;
#endif

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

    NU_USER_MODE();                         /* return to user mode */

    /* Return status */
    return (status);

} /* PPTP_Set_Link_Option */
