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
*       ips_grp_rg.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Remove_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Remove_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Remove_Group
*
* DESCRIPTION
*
*       This function removes an IPsec group.
*
* INPUTS
*
*       *group_name             Name of the group to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         If required instance is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_Group(CHAR *group_name)
{
    STATUS              status;
    IPSEC_INBOUND_SA    *in_sa;
    IPSEC_INBOUND_SA    *in_next_sa;
    IPSEC_OUTBOUND_SA   *out_sa;
    IPSEC_OUTBOUND_SA   *out_next_sa;
    IPSEC_POLICY        *policy_ptr;
    IPSEC_POLICY        *nxt_policy_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;
    DV_DEVICE_ENTRY     *dev;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Also group name should be valid. */
    if(group_name == NU_NULL)
        status = IPSEC_INVALID_PARAMS;
    else
    {
        /* First grab the TCP semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status value. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain TCP semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* Grab the semaphore. */
            status =
                NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Get the pointer of the given group name. */
                status = IPSEC_Get_Group_Entry_Real(&IPS_Group_DB,
                                                group_name, &group_ptr);
                /* Check the status first. */
                if(status == NU_SUCCESS)
                {
                    /* First check if there is any device registered
                     * with this group. If so then we need to unregister
                     * it first.
                     */
                    for (dev = DEV_Table.dv_head;
                        (dev != NU_NULL); dev = dev->dev_next)
                    {
                        /* Match the two group pointers. */
                        if(dev->dev_ext.dev_phy->dev_phy_ips_group ==
                                                            group_ptr)
                        {
                            /* This device is registered with this group.
                               So unregister this device. */
                            dev->dev_ext.dev_phy->dev_phy_ips_group =
                                                                NU_NULL;
                        }
                    }

                    /* Policy database attached with this group need to be
                       flushed. */
                    for(policy_ptr = group_ptr->
                                        ipsec_policy_list.ipsec_head;
                      ((policy_ptr != NU_NULL) && (status == NU_SUCCESS));)
                    {
                        /* Back up the next policy address*/
                        nxt_policy_ptr = policy_ptr->ipsec_flink;

                        /* Remove this policy. */
                        status = IPSEC_Remove_Policy_Real(group_ptr,
                                                          policy_ptr);

                        /*Assign policy_ptr the address of next policy to be
                        deleted*/
                        policy_ptr = nxt_policy_ptr;
                    }

                    /* Check the status value. */
                    if(status == NU_SUCCESS)
                    {
                        /* Inbound SA database attached with this group
                         * need to be flushed.
                         */

                        /* Get the inbound SADB head. */
                        in_next_sa = group_ptr->ipsec_inbound_sa_list.
                                                            ipsec_head;
                        /* Traverse the whole database. */
                        for(in_sa = in_next_sa; (in_sa != NU_NULL);
                            in_sa = in_next_sa)
                        {
                            /* First store the next sa pointer. */
                            in_next_sa = in_next_sa->ipsec_flink;

#if (INCLUDE_IKE == NU_TRUE)
                            /* Make sure only those SAs can be removed
                             * which have been added by user and not by
                             * IKE.
                             */
                            if((in_sa->ipsec_soft_lifetime.ipsec_flags &
                                                    IPSEC_IKE_SA) != 0)
                            {
                                /* Remove timer events corresponding to
                                   this SA. */
                                TQ_Timerunset(IPSEC_Soft_Lifetime_Event,
                                            TQ_CLEAR_ALL_EXTRA,
                                            (UNSIGNED)&(in_sa->
                                            ipsec_soft_lifetime), 0);

                                TQ_Timerunset(IPSEC_Hard_Lifetime_Event,
                                            TQ_CLEAR_ALL_EXTRA,
                                            (UNSIGNED)&(in_sa->
                                            ipsec_hard_lifetime), 0);
                            }
#endif
                            /* Free the SA resources. */
                            IPSEC_Free_Inbound_SA(in_sa);
                        }

                        /* Outbound SA database attached with this group
                         * needs to be flushed.
                         */

                        /* Get the inbound SADB head. */
                        out_next_sa = group_ptr->ipsec_outbound_sa_list.
                                                            ipsec_head;
                        /* Traverse the whole database. */
                        for(out_sa = out_next_sa; (out_sa != NU_NULL);
                            out_sa = out_next_sa)
                        {
                            /* First store the next sa pointer. */
                            out_next_sa = out_next_sa->ipsec_flink;

                            /* Free the resources. */
                            IPSEC_Free_Outbound_SA(out_sa);
                        }

                        /* Its now safe to remove group from the database. */
                        SLL_Remove(&IPS_Group_DB, group_ptr);

                        /* Now deallocate the group memory too. */
                        if(NU_Deallocate_Memory(group_ptr) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to deallocate memory",
                                           NERR_SEVERE,__FILE__,__LINE__);
                        }
                    }
                }

                /* Now everything is done, release the semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release IPsec semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Failed to obtain IPsec semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Release the TCP semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the TCP semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_Group */
