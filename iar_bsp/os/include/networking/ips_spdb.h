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
*       ips_spdb.h
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       Definitions for Security Policy database.
*
* DATA STRUCTURES
*
*       IPSEC_OUTBOUND_BUNDLE
*       IPSEC_BUNDLE_LIST
*       IPSEC_BUNDLE_SORT
*       IPSEC_POLICY
*       IPSEC_SPDB
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_SPDB_H
#define IPS_SPDB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

struct ipsec_policy_group;

/* Defining the structure IPSEC_OUTBOUND_BUNDLE. */
typedef struct ipsec_outbound_bundle
{
    /* Front link. */
    struct ipsec_outbound_bundle    *ipsec_flink;

    /* Unique identifier for this bundle. */
    IPSEC_SELECTOR      ipsec_selector;

    /* Array of out bundle SA indexes, which made up this bundle. */
    UINT32              ipsec_out_sa_indexes[IPSEC_MAX_SA_BUNDLE_SIZE];

    /* SA timeout variable used to ensure that no two IKE
       requests are made for a single SA establishment. */
    UINT32              ipsec_sa_req_timeout;
}IPSEC_OUTBOUND_BUNDLE;
/* End of IPSEC_OUTBOUND_BUNDLE structure definition. */

/* This structure is used to define the list of bundles. */
typedef struct ipsec_bundle_list
{
    /* Head of the list. */
    IPSEC_OUTBOUND_BUNDLE       *ipsec_head;

    /* Tail of the list. */
    IPSEC_OUTBOUND_BUNDLE       *ipsec_tail;

    /* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
    /* Lifetime associated with this bundle in seconds. */
    UINT32                      ipsec_lifetime;
#endif

}IPSEC_BUNDLE_LIST;

/* This structure is used to sort a bundle list before
* looking up SAs in a bundle.
*/
typedef struct ipsec_bundle_sort
{
    UINT32          *ipsec_sa_index;        /* Pointer to index of SA. */
    UINT8           ipsec_bundle_index;     /* Index of the bundle. */

    UINT8           ipsec_pad[3];
} IPSEC_BUNDLE_SORT;


/* Defining the structure IPSEC_POLICY. */
typedef struct ipsec_policy
{
    /* Front Link. */
    struct ipsec_policy     *ipsec_flink;

    /* An index that uniquely identifies a policy. */
    UINT32                  ipsec_index;

#if ( IPSEC_ENABLE_PRIORITY == NU_TRUE )
    /* Indicates the priority of a policy. Valid priorities range from 0 to
     * 4294967295. 0 is the highest priority and 4294967295 is the lowest
     * priority. Policies with the same priority are sorted according to the
     * order they were entered in to the policy database (those policies that
     * are entered first have a higher order).
     */
    UINT32                  ipsec_priority;
#endif

    /* The policy's selector. */
    IPSEC_SELECTOR          ipsec_select;

    /* Pointer to an array indicating how IPsec should be applied to
       packets matching this policy. */
    IPSEC_SECURITY_PROTOCOL *ipsec_security;

    /* List of outbound SA bundles. */
    IPSEC_BUNDLE_LIST       ipsec_bundles;

#if (INCLUDE_IKE == NU_TRUE)
    /* Maximum limit of lifetime for SAs negotiated under this
     * policy. This is only needed if IKE support is enabled.
     */
    IPSEC_SA_LIFETIME       ipsec_sa_max_lifetime;

    /* IKE group description. This is the IKE group to be used
     * for Perfect Forward Secrecy (PFS).
     */
    UINT16                  ipsec_pfs_group_desc;

    /* Defining this data member so that padding at the end of this
     * structure is not affected whether IKE is included or not.
     */
    UINT8                   ipsec_pad1[2];
#endif

    /* No. of elements in the array above.*/
    UINT8                   ipsec_security_size;

    /* Specifies action to be taken for packets matching this policy.*/
    UINT8                   ipsec_flags;

    /* Padding the structure. */
    UINT8                   ipsec_pad2[2];
}IPSEC_POLICY;
/* End of IPSEC_POLICY structure definition. */

/* Defining the structure IPSEC_SPDB. */
typedef struct ipsec_spdb
{
    IPSEC_POLICY            *ipsec_head;         /* Head. */
    IPSEC_POLICY            *ipsec_tail;         /* Tail. */

    /* Index of next policy.*/
    UINT32                  ipsec_next_policy_index;
}IPSEC_SPDB;
/* End of IPSEC_SPDB structure definition. */

/****** Prototypes related to Policies. ******/

UINT8   IPSEC_Match_Selectors(IPSEC_SELECTOR *policy_selector,
                              IPSEC_SELECTOR *pkt_selector,
                              UINT8 swap_flag);

STATUS IPSEC_Add_Policy(CHAR *group_name, IPSEC_POLICY *policy,
                        UINT32 *return_index);

VOID   IPSEC_Priority_Insert( IPSEC_SPDB *policy_list,
                              IPSEC_POLICY *new_policy );

STATUS IPSEC_Get_Policy_Index(CHAR *group_name, IPSEC_SELECTOR *selector,
                              UINT8 selector_type, UINT32 *return_index);

STATUS IPSEC_Get_Policy_Index_Narrow(CHAR *group_name,
                                     IPSEC_SELECTOR *selector,
                                     UINT8 selector_type,
                                     UINT32 *return_index,
                                     INT *return_narrow_proto);

STATUS IPSEC_Get_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                            VOID *optval, INT *optlen);

STATUS IPSEC_Set_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                            VOID *optval, INT optlen);

STATUS IPSEC_Remove_Policy(CHAR *group_name, UINT32 index);

STATUS IPSEC_Remove_Policy_Real(struct ipsec_policy_group *group_ptr,
                                IPSEC_POLICY *policy_ptr);

STATUS IPSEC_Get_Policy_Entry(struct ipsec_policy_group *group_ptr,
                              UINT32 policy_index,
                              IPSEC_POLICY **policy_ptr);

STATUS IPSEC_Get_Policy_By_Selector(struct ipsec_policy_group *group_ptr,
                                    IPSEC_POLICY *policy_ptr,
                                    IPSEC_SELECTOR *selector,
                                    UINT8 policy_type,
                                    IPSEC_POLICY **ret_policy_ptr);

STATUS IPSEC_Match_Policy_In(struct ipsec_policy_group *group,
                             IPSEC_SELECTOR *pkt_selector,
                             IPSEC_INBOUND_SA **in_bundle, UINT8 sa_count,
                             IPSEC_POLICY **policy_ret_ptr);

STATUS IPSEC_Match_Policy_Out(struct ipsec_policy_group *group,
                              IPSEC_SELECTOR *pkt_selector,
                              IPSEC_POLICY **ret_policy_ptr,
                              IPSEC_OUTBOUND_BUNDLE **out_bundle_ptr );

/***** Prototypes related to bundles. *****/
STATUS IPSEC_Add_Outbound_Bundle(IPSEC_POLICY *policy_ptr,
                                 IPSEC_SELECTOR *pkt_selector,
                                 IPSEC_OUTBOUND_BUNDLE **out_bundle);

STATUS IPSEC_Get_Bundle_By_Selector(IPSEC_OUTBOUND_BUNDLE *start_bundle,
                                    IPSEC_SELECTOR *selector,
                                    IPSEC_OUTBOUND_BUNDLE **ret_bundle);

STATUS IPSEC_Get_Bundle_SA_Entries(UINT32 dev_index,
                                   struct ipsec_policy_group *group_ptr,
                                   IPSEC_POLICY *policy_ptr,
                                   IPSEC_OUTBOUND_BUNDLE *out_bundle,
                                   IPSEC_SELECTOR *pkt_selector,
                                   IPSEC_OUTBOUND_SA **sa_ptr_bundle);

VOID IPSEC_Int_Ptr_Sort(IPSEC_BUNDLE_SORT *array, UINT8 array_size);

VOID IPSEC_IKE_SA_Request(UINT32 dev_index, IPSEC_POLICY *policy_ptr,
                          IPSEC_OUTBOUND_BUNDLE *out_bundle,
                          IPSEC_SELECTOR *pkt_selector);

STATUS IPSEC_Verify_Policy(IPSEC_POLICY *in_policy);

STATUS IPSEC_Normalize_Security(IPSEC_SECURITY_PROTOCOL *security);

STATUS IPSEC_Validate_Sec_Prot(IPSEC_SECURITY_PROTOCOL *sec_prot);

STATUS IPSEC_Validate_Selector(IPSEC_SELECTOR *selector);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_SPDB_H */
