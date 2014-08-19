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
*       ips_tim.h
*
* COMPONENT
*
*       LIFETIMES
*
* DESCRIPTION
*
*       Definitions required for implementation of SA and bundle
*       lifetimes.
*
* DATA STRUCTURES
*
*       IPSEC_SA_LIFETIME
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_TIM_H
#define IPS_TIM_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

#if (INCLUDE_IKE == NU_TRUE)

/* SA expiry action defines. */
#define IPSEC_LOG_WARNING               0x01
#define IPSEC_REFRESH_SA                0x02

struct ipsec_outbound_sa;
struct ipsec_inbound_sa;

/* Defining the structure IPSEC_SA_LIFETIME. */
typedef struct ipsec_sa_lifetime
{
    /* Pointer to outbound SA using this lifetime. */
    struct ipsec_outbound_sa    *ipsec_out_sa;

    /* Pointer to inbound SA using this lifetime. */
    struct ipsec_inbound_sa     *ipsec_in_sa;

    /* The number of seconds for which this SA is valid. */
    UINT32                      ipsec_no_of_secs;

    /* Action to be taken when this timer is triggered (expires). */
    UINT8                       ipsec_expiry_action;

    /* Flags showing whether the attached outbound or inbound SA has been
       used or not. */
    UINT8                       ipsec_flags;

    /* Padding the structure. */
    UINT8                       ipsec_pad[2];
}IPSEC_SA_LIFETIME;
/* End of IPSEC_SA_LIFETIME structure definition. */

/*** Function Prototypes. ***/
STATUS IPSEC_Lifetimes_Init(VOID);
VOID IPSEC_Soft_Lifetime_Expired(TQ_EVENT event, UNSIGNED lifetime,
                                 UNSIGNED dev_id);
VOID IPSEC_Hard_Lifetime_Expired(TQ_EVENT event, UNSIGNED lifetime,
                                 UNSIGNED dev_id);
VOID IPSEC_Bundle_Lifetime_Expired(TQ_EVENT event, UNSIGNED bundle,
                                       UNSIGNED policy);
#endif

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* #ifndef IPS_TIM_H */
