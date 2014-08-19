/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       vacm.h                                                   
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used by the View-Based Access Control Model.
*
*   DATA STRUCTURES
*
*       VACM_CONTEXT_STRUCT
*       VACM_CONTEXT_ROOT
*       VACM_SEC2GROUP_STRUCT
*       VACM_SEC2GROUP
*       VACM_SEC2GROUP_ROOT
*       VACM_ACCESS_STRUCT
*       VACM_ACCESS
*       VACM_ACCESS_ROOT
*       VACM_VIEWTREE_STRUCT
*       VACM_VIEWTREE
*       VACM_VIEWTREE_ROOT
*       VACM_MIB_STRUCT
*
*   DEPENDENCIES
*
*       snmp.h
*
************************************************************************/

#ifndef VACM_H
#define VACM_H

#include "networking/snmp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* VACM Context Match Values */
#define VACM_CTXTMATCH_EXACT                1
#define VACM_CTXTMATCH_PREFIX               2

/* Definitions for view family. */
#define VACM_SUBTREE_LEN                    32
#define VACM_MASK_SIZE                      (VACM_SUBTREE_LEN/2)

/* The maximum number of entries that will be selected at one time
 * to determine the correct access entry.
 */
#define VACM_SELECTENTRY_SIZE               32

/* VACM Family Type */
#define VACM_FAMILY_INCLUDED                1
#define VACM_FAMILY_EXCLUDED                2


/* VACM viewType */
#define VACM_READ_VIEW                      1
#define VACM_WRITE_VIEW                     2
#define VACM_NOTIFY_VIEW                    3

/* Wild card. */
#define VACM_ANY                            0

/* Error definitions specific to VACM. */
#define VACM_NOSUCHCONTEXT                  10
#define VACM_NOGROUPNAME                    11
#define VACM_NOACCESSENTRY                  12
#define VACM_NOSUCHVIEW                     13
#define VACM_NOTINVIEW                      14
#define VACM_ENDOFVIEW                      15

/* This structure provides information about the various contexts in which
 * data is available.
 */
typedef struct vacm_context_struct
{
    struct vacm_context_struct      *next;
    struct vacm_context_struct      *previous;

    /* A human readable name identifying a particular context at a
     * particular SNMP entity. The empty contextName (zero length)
     * represents the default context.
     */
    CHAR                            context_name[SNMP_SIZE_SMALLOBJECTID];

}VACM_CONTEXT_STRUCT;

typedef struct vacm_context_root
{
    struct vacm_context_struct      *next;
    struct vacm_context_struct      *previous;

}VACM_CONTEXT_ROOT;

/* This structure maps a combination of securityModel and securityName to
 * a groupName that is used to define an access control policy for a
 * group of principals. If this structure is modified make sure
 * that packed structure below is also updated.
 */
typedef struct vacm_sec2group_struct
{
    struct vacm_sec2group_struct          *next;
    struct vacm_sec2group_struct          *previous;

    /* The securityModel, by which the vacmSecurityName referenced by this
     * entry is provided. This object may not take the 'any' (0) value.
     */
    UINT32      vacm_security_model;

    /* The securityName for the principal, represented in a Security Model
     * independent format, which is mapped by this entry to a groupName.
     */
    UINT8       vacm_security_name[SNMP_SIZE_SMALLOBJECTID];

    /* The name of the group to which this entry (e.g., the combination of
     * securityModel and securityName) belongs. This groupName is used as
     * index into the vacmAccessTable to select an access control policy.
     */
    UINT8       vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

#if (INCLUDE_MIB_VACM == NU_TRUE)

    /* The storage type for this conceptual row. Conceptual rows having
     * the value 'permanent' need not allow write-access to any columnar
     * objects in the row.
     */
    UINT8       vacm_storage_type;

    /* The status of this conceptual row.*/
    UINT8       vacm_status;

    UINT8       vacm_row_flag;

    /* Make the structure word-aligned. */
    UINT8       vacm_pad[1];
#endif

}VACM_SEC2GROUP_STRUCT;

/* This structure is a packed version of the security to group structure
 * defined above. It is used for compile time configurations.
 */
typedef struct vacm_sec2group
{
    /* The securityModel, by which the vacmSecurityName referenced by this
     * entry is provided. This object may not take the 'any' (0) value.
     */
    UINT32      vacm_security_model;

    /* The securityName for the principal, represented in a Security Model
     * independent format, which is mapped by this entry to a groupName.
     */
    UINT8       vacm_security_name[SNMP_SIZE_SMALLOBJECTID];

    /* The name of the group to which this entry (e.g., the combination of
     * securityModel and securityName) belongs. This groupName is used as
     * index into the vacmAccessTable to select an access control policy.
     */
    UINT8       vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

}VACM_SEC2GROUP;

typedef struct vacm_sec2group_root
{
    struct vacm_sec2group_struct          *next;
    struct vacm_sec2group_struct          *previous;

}VACM_SEC2GROUP_ROOT;

/* This structure is for access rights of groups. Each entry is indexed by
 * a groupName, contextPrefix, securityModel and securityLevel. To
 * determine whether access is allowed, one entry from this table needs to
 * be selected and the proper viewName from that entry must be used for
 * access control checking. If this structure is modified make sure
 * that the packed structure below is also updated.
 */
typedef struct vacm_access_struct
{
    struct vacm_access_struct             *next;
    struct vacm_access_struct             *previous;

    /* In order to gain access rights allowed by this conceptual row,
     * this securityModel must be in use.
     */
    UINT32       vacm_security_model;

    /* Group name for this entry. */
    UINT8        vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* In order to gain the access rights allowed by this conceptual row,
     * a contextName must match exactly (if the value of
     * vacmAccessContextMatch is 'exact') or partially (if the value of
     * vacmAccessContextMatch is 'prefix') to the value of the instance
     * of this object
     */
    UINT8        vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* A securityLevel of noAuthNoPriv is less than authNoPriv which in
     * turn is less than authPriv. This member defines the minimum
     * security level required.
     */
    UINT8        vacm_security_level;

    /* Defines how the context name should match. Whether it is exact or
     * a prefix.
     */
    UINT8        vacm_context_match;

    /* Read MIB view. */
    UINT8        vacm_read_view[SNMP_SIZE_SMALLOBJECTID];

    /* Write MIB view. */
    UINT8        vacm_write_view[SNMP_SIZE_SMALLOBJECTID];

    /* Notify MIB view. */
    UINT8        vacm_notify_view[SNMP_SIZE_SMALLOBJECTID];

#if (INCLUDE_MIB_VACM == NU_TRUE)

    /* Storage type for this conceptual row. Conceptual rows having the
     * value 'permanent' need not allow write-access to any columnar
     * objects in the row.
     */
    UINT8        vacm_storage_type;

    /* The status of this conceptual row.*/
    UINT8        vacm_status;

    UINT8        vacm_row_flag;

    /* Make the structure word-aligned. */
    UINT8       vacm_pad[3];
#else

    /* Make the structure word-aligned. */
    UINT8       vacm_pad[2];
#endif

} VACM_ACCESS_STRUCT;

/* This structure is a packed version of the access structure
 * defined above. It is used for compile time configurations.
 */
typedef struct vacm_access
{
    /* In order to gain access rights allowed by this conceptual row,
     * this securityModel must be in use.
     */
    UINT32       vacm_security_model;

    /* Group name for this entry. */
    UINT8        vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* In order to gain the access rights allowed by this conceptual row,
     * a contextName must match exactly (if the value of
     * vacmAccessContextMatch is 'exact') or partially (if the value of
     * vacmAccessContextMatch is 'prefix') to the value of the instance
     * of this object
     */
    UINT8        vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Defines how the context name should match. Should it be exact or
     * a prefix.
     */
    UINT8        vacm_context_match;

    /* A securityLevel of noAuthNoPriv is less than authNoPriv which in
     * turn is less than authPriv. This member defines the minimum
     * security level required.
     */
    UINT8        vacm_security_level;

    /* Read MIB view. */
    UINT8        vacm_read_view[SNMP_SIZE_SMALLOBJECTID];

    /* Write MIB view. */
    UINT8        vacm_write_view[SNMP_SIZE_SMALLOBJECTID];

    /* Notify MIB view. */
    UINT8        vacm_notify_view[SNMP_SIZE_SMALLOBJECTID];

    UINT8       vacm_pad[2];
} VACM_ACCESS;

typedef struct vacm_access_root
{
    struct vacm_access_struct             *next;
    struct vacm_access_struct             *previous;

}VACM_ACCESS_ROOT;

/* Information about families of subtrees within MIB views. If
 * this structure is modified make sure that the corresponding
 * packed structure below is also updated.
 */
typedef struct vacm_viewtree_struct
{
    struct vacm_viewtree_struct           *next;
    struct vacm_viewtree_struct           *previous;

    /* Subtree. */
    UINT32       vacm_subtree[VACM_SUBTREE_LEN];

    /* Length of subtree. */
    UINT32       vacm_subtree_len;

    /* Length of the mask in bytes. */
    UINT32       vacm_mask_length;

    /* Human readable name for a family of view subtrees. */
    UINT8        vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Mask used to determine the determining identifiers of the
     * sub-tree.
     */
    UINT8        vacm_family_mask[VACM_MASK_SIZE];

    /* Family type: included(1) or excluded(2). */
    UINT8        vacm_family_type;

#if (INCLUDE_MIB_VACM == NU_TRUE)

    /* Storage type. */
    UINT8        vacm_storage_type;

    /* Row status. */
    UINT8        vacm_status;

    UINT8        vacm_row_flag;

#else

    /* Make the structure word-aligned. */
    UINT8       vacm_pad[3];
#endif

}VACM_VIEWTREE_STRUCT;

typedef struct vacm_viewtree
{
    /* Human readable name for a family of view subtrees. */
    UINT8        vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree. */
    UINT32       vacm_subtree[VACM_SUBTREE_LEN];

    /* Length of subtree. */
    UINT32       vacm_subtree_len;

    /* Mask used to determine the relevant identifiers of the
     * sub-tree.
     */
    UINT8        vacm_family_mask[VACM_MASK_SIZE];

    /* Length of the mask in bytes. */
    UINT32       vacm_mask_length;

    /* Family type: included(1) or excluded(2). */
    UINT8        vacm_family_type;

    UINT8       vacm_pad[3];
}VACM_VIEWTREE;

typedef struct vacm_viewtree_root
{
    struct vacm_viewtree_struct           *next;
    struct vacm_viewtree_struct           *previous;

}VACM_VIEWTREE_ROOT;

/* General structure which holds all the tables that are used in VACM
 * decision making.
 */
typedef struct vacm_mib_struct
{
    VACM_CONTEXT_ROOT                      vacm_context_table;
    VACM_SEC2GROUP_ROOT                    vacm_security_to_group_tab;
    VACM_ACCESS_ROOT                       vacm_access_tab;
#if (INCLUDE_MIB_VACM == NU_TRUE)
    UINT32                                 vacm_view_spin_lock;
#endif
    VACM_VIEWTREE_ROOT                     vacm_view_tree_family_tab;

} VACM_MIB_STRUCT;

/*all those functions which are implemented in vacm.c*/
STATUS VACM_Init(VOID);
STATUS VACM_Config(VOID);

STATUS VACM_Search_Context(const UINT8 *context_name,
                           VACM_CONTEXT_STRUCT **loc_ptr);
STATUS VACM_Search_Group(UINT32 snmp_sm, const UINT8 *security_name,
                         VACM_SEC2GROUP_STRUCT **location_ptr);
STATUS VACM_Search_AccessEntry(const UINT8 *group_name,
                               const UINT8 *context_prefix,
                               UINT32 snmp_sm, UINT8 security_level,
                               VACM_ACCESS_STRUCT **location_ptr);
STATUS VACM_Search_MibView(const UINT8 *view_name, const UINT32 *subtree,
                           UINT32 subtree_len,
                           VACM_VIEWTREE_STRUCT **location_ptr);

VOID VACM_Filling_FamilyMask(UINT8 *vacm_family_mask, UINT32 mask_len);

STATUS VACM_CheckInstance_Access (UINT32 snmp_sm, UINT8 security_level,
                                  UINT8 *security_name,
                                  UINT8 *context_name, UINT8 view_type,
                                  UINT32 *oid, UINT32 length);

VACM_CONTEXT_STRUCT *VACM_Get_Context_Name_Util(const UINT8 *context_name,
                                                UINT32 context_name_len,
                                                UINT8 getflag);

VACM_SEC2GROUP_STRUCT *VACM_Get_Sec2Group_Entry_Util(
                        UINT32 vacm_sec_model, UINT32 vacm_sec_name_len,
            const UINT8 *vacm_sec_name, const VACM_SEC2GROUP_ROOT * root);

VACM_SEC2GROUP_STRUCT *VACM_Get_Sec2Group_Entry(UINT32 vacm_sec_model,
                                                UINT32 vacm_sec_name_len,
                                                UINT8 *vacm_sec_name,
                                                UINT8 getflag);

VACM_ACCESS_STRUCT *VACM_MIB_Get_AccessEntry_Util(
               UINT32 vacm_group_name_len, const UINT8 *vacm_group_name,
               UINT32 vacm_context_prefix_len,
               const UINT8 *vacm_context_prefix,
               UINT32 vacm_sec_model, UINT32 vacm_sec_level,
               const VACM_ACCESS_ROOT *root);

VACM_VIEWTREE_STRUCT *VACM_GetViewTree_Entry_Util(
                UINT32 vacm_view_name_len, const UINT8 *vacm_view_name,
                UINT32 vacm_subtree_len, const UINT32 *vacm_subtree,
                const VACM_VIEWTREE_ROOT *root);

VACM_VIEWTREE_STRUCT *VACM_GetViewTreeFamily_Entry(
                        UINT32 vacm_view_name_len, UINT8 *vacm_view_name,
                        UINT32 vacm_subtree_len, UINT32 *vacm_subtree,
                        UINT8 getflag);

VACM_ACCESS_STRUCT *VACM_MIB_Get_AccessEntry(UINT32 vacm_group_name_len,
                   UINT8 *vacm_group_name, UINT32 vacm_context_prefix_len,
                   UINT8 *vacm_context_prefix, UINT32 vacm_sec_model,
                   UINT32 vacm_sec_level, UINT8 getflag);

STATUS VACM_Find_AccessEntry(const CHAR *group_name,
                             const CHAR *context_name,
                             UINT32 snmp_sm, UINT8 security_level,
                             VACM_ACCESS_STRUCT **location_ptr);
STATUS VACM_FindIn_MibView(const CHAR *view_name, UINT32 *in_oid,
                           UINT32 in_oid_len);

STATUS VACM_InsertContext(VACM_CONTEXT_STRUCT *node);
STATUS VACM_Add_Context(const UINT8 *context_name);
STATUS VACM_InsertGroup_Util(VACM_SEC2GROUP_STRUCT *node,
                             VACM_SEC2GROUP_ROOT *root);
STATUS VACM_InsertGroup(VACM_SEC2GROUP_STRUCT *node);
STATUS VACM_InsertAccessEntry_Util(VACM_ACCESS_STRUCT *node,
                                   VACM_ACCESS_ROOT *root);
STATUS VACM_InsertAccessEntry(VACM_ACCESS_STRUCT *node);
STATUS VACM_InsertMibView_Util(VACM_VIEWTREE_STRUCT *node,
                               VACM_VIEWTREE_ROOT *root);
STATUS VACM_InsertMibView(VACM_VIEWTREE_STRUCT *node);

STATUS VACM_Add_Group (UINT32 snmp_sm, UINT8 *security_name,
                       UINT8 *group_name, UINT8 storage_type,
                       UINT8 row_status);
STATUS VACM_Add_AccessEntry(UINT8 *group_name, UINT8 *context_prefix,
                            UINT32 snmp_sm, UINT8 security_level,
                            UINT8 contextMatch, UINT8 *read_view,
                            UINT8 *write_view, UINT8 *notify_view,
                            UINT8 storage_type, UINT8 row_status);
STATUS VACM_Add_MibView (UINT8 *view_name, UINT32 *subtree,
                         UINT32 subtree_len, UINT8 familyType,
                         UINT8 *family_mask, UINT32 mask_len,
                         UINT8 storage_type, UINT8 row_status);

STATUS VACM_Remove_Context(UINT8 *context_name);
STATUS VACM_Remove_Group (VACM_SEC2GROUP_STRUCT *location_ptr);
STATUS VACM_Remove_AccessEntry (VACM_ACCESS_STRUCT *location_ptr);
STATUS VACM_Remove_MibView  (VACM_VIEWTREE_STRUCT *location_ptr);

INT32  VACM_Compare_Group(VOID *left_side, VOID *right_side);
STATUS VACM_Save_Group(VACM_SEC2GROUP_STRUCT *vacm_security_to_group);
INT32  VACM_Compare_Access(VOID *left_side, VOID *right_side);
STATUS VACM_Save_Access(VACM_ACCESS_STRUCT *vacm_access);
INT32  VACM_Compare_View(VOID *left_side, VOID *right_side);
STATUS VACM_Save_View(VACM_VIEWTREE_STRUCT *vacm_view_tree_family);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* VACM_H */

