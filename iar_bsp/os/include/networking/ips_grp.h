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
*       ips_grp.h
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       Definitions for Groups database.
*
* DATA STRUCTURES
*
*       IPSEC_POLICY_GROUP
*       IPSEC_GROUP_DB
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_GRP_H
#define IPS_GRP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

/* Defining the structure IPSEC_POLICY_GROUP.
 * The structure members should not be modified as this
 * structure is also used by IKE.
 */
typedef struct ipsec_policy_group
{
    /* Front Link. */
    struct ipsec_policy_group   *ipsec_flink;

    /* Name to identify the group.*/
    CHAR                *ipsec_group_name;

    /* List of IPsec Policies.*/
    IPSEC_SPDB          ipsec_policy_list;

    /* List of Inbound SAs. */
    IPSEC_INBOUND_SADB  ipsec_inbound_sa_list;

    /* List of Outbound SAs.*/
    IPSEC_OUTBOUND_SADB ipsec_outbound_sa_list;

    /* Flag for DF bit processing setting */
    UINT8               ipsec_df_processing;

    /* Add the required padding. */
    UINT8               ipsec_pad[3];
}IPSEC_POLICY_GROUP;
/* End of IPSEC_POLICY_GROUP structure definition. */

/* This structure is used to define the list of groups. */
typedef struct ipsec_group_db
{
    IPSEC_POLICY_GROUP      *head;          /* Head pointer. */
    IPSEC_POLICY_GROUP      *tail;          /* Tail pointer. */
}IPSEC_GROUP_DB;

/* Extern the IPsec groups DB. */
extern IPSEC_GROUP_DB       IPS_Group_DB;

/****** Function prototypes related to Groups. ******/
VOID   IPSEC_Group_Init(VOID);
STATUS IPSEC_Add_Group(CHAR *group_name);
STATUS IPSEC_Add_To_Group(CHAR *group_name, CHAR *interface_name);
STATUS IPSEC_Get_Group(CHAR *interface_name, CHAR *return_group,
                       UINT32 *total_len);
STATUS IPSEC_Get_Group_Entry_Real(IPSEC_GROUP_DB *group_db,
                                  CHAR *group_name,
                                  IPSEC_POLICY_GROUP **ret_group);
STATUS IPSEC_Remove_From_Group(CHAR *interface_name);
STATUS IPSEC_Get_Group_Opt(CHAR *group_name, INT optname,
                           VOID *optval, INT *optlen);
STATUS IPSEC_Set_Group_Opt(CHAR *group_name, INT optname,
                            VOID *optval, INT optlen);
STATUS IPSEC_Remove_Group(CHAR *group_name);
INT    IPSEC_Cmp_Groups(VOID *a, VOID *b);

#define IPSEC_Get_Group_Entry(group_name, ret_group) \
        IPSEC_Get_Group_Entry_Real(&IPS_Group_DB, group_name, \
                                   ret_group)

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_GRP_H */
