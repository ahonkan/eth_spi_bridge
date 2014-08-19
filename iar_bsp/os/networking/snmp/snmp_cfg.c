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
*       snmp_cfg.c                                               
*
*   DESCRIPTION
*
*       This file contains functions and variables specific to the
*       configuration of Nucleus SNMP.
*
*   DATA STRUCTURES
*
*       Snmp_Mp_Models
*       Snmp_Ss_Models
*       Usm_Auth_Prot_Table
*       Usm_Priv_Prot_Table
*       Usm_User_Table
*       CBSM_Community_Table
*       Snmp_Cfg_Tgr_Addr_Tbl
*       Snmp_Cfg_Tgr_Params_Tbl
*       Snmp_Cfg_Notify_Tbl
*       Snmp_Cfg_Fltr_Prof_Tbl
*       Snmp_Cfg_Fltr_Tbl
*       Snmp_Cfg_Context_Names
*       Snmp_Cfg_Sec2Groups
*       Snmp_Cfg_Access
*       Snmp_Cfg_Mib_View
*       cfig_snmpd
*       authentrap_onoff
*       coldtrap_onoff
*       cfig_portname
*       cfig_hostname
*       cfig_hostid
*       cfig_hosttype
*
*   FUNCTIONS
*
*       get_snmpport
*       get_authentrap
*       get_coldstarttrap
*       get_hostid
*       get_numports
*       nc_init
*
*   DEPENDENCIES
*
*       nu_net.h
*       xtypes.h
*       snmp.h
*       agent.h
*       snmp_dis.h
*       snmp_mp.h
*       snmp_ss.h
*       snmp_v1.h
*       snmp_v2.h
*       snmp_v3.h
*       cbsm_v1.h
*       cbsm_v2.h
*       usm.h
*       usm_md5.h
*       usm_sha.h
*       usm_des.h
*       snmp_no.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"

/* The Dispatcher. */
#include "networking/snmp_dis.h"

/* The Message Processing Subsystem. */
#include "networking/snmp_mp.h"

/* The Security Subsystem. */
#include "networking/snmp_ss.h"

/* The Header files for the Message Processing Models. */
#if(INCLUDE_SNMPv1 == NU_TRUE)
#include "networking/snmp_v1.h"
#endif
#if(INCLUDE_SNMPv2 == NU_TRUE)
#include "networking/snmp_v2.h"
#endif
#if(INCLUDE_SNMPv3 == NU_TRUE)
#include "networking/snmp_v3.h"
#endif

/* The Header file for the Security Models. */
#if(INCLUDE_SNMPv1 == NU_TRUE)
#include "networking/cbsm_v1.h"
#endif
#if(INCLUDE_SNMPv2 == NU_TRUE)
#include "networking/cbsm_v2.h"
#endif
#if(INCLUDE_SNMPv3 == NU_TRUE)
#include "networking/usm.h"

/* The Header file for Authentication and Privacy Protocols (used with
 * USM).
 */
#include "networking/usm_md5.h"
#include "networking/usm_sha.h"
#include "networking/usm_des.h"
#endif

/* Header file for Notification Originator */
#include "networking/snmp_no.h"

/* All the Message Processing models are added in to the following array.
 * Please make sure that the header file for the given model is also
 * included above. SNMP_MP_MODELS_NO will also need to be updated in
 * snmp_cfg.h.
 */
SNMP_MP_STRUCT Snmp_Mp_Models[SNMP_MP_MODELS_NO] =
{   /* MP Model, Security Model, Initialization Function, Config Function,
     * Encoding Function, Decoding Function, Notification Encoding
     * Function
     */
#if(INCLUDE_SNMPv1 == NU_TRUE)
    {SNMP_VERSION_V1, SNMP_CBSM_V1, SNMP_V1_Init, NU_NULL,
      SNMP_V1_Enc_Respond, SNMP_V1_Dec_Request, SNMP_V1_Notification_Enc},
#endif
#if(INCLUDE_SNMPv2 == NU_TRUE)
    {SNMP_VERSION_V2, SNMP_CBSM_V2, SNMP_V2_Init, NU_NULL,
      SNMP_V2_Enc_Respond, SNMP_V2_Dec_Request, SNMP_V2_Notification_Enc},
#endif
#if(INCLUDE_SNMPv3 == NU_TRUE)
    {SNMP_VERSION_V3, SNMP_USM, SNMP_V3_Init, NU_NULL,
      SNMP_V3_Enc_Respond, SNMP_V3_Dec_Request, SNMP_V3_Notification_Enc}
#endif
};


/* All the Security Models are added in to the following array. Please
 * make sure that the header file for the given model is also included
 * above. SNMP_SS_MODELS_NO will also need to be updated in snmp_cfg.h.
 */
SNMP_SS_STRUCT Snmp_Ss_Models[SNMP_SS_MODELS_NO] =
{
    /* Security Model, Initialization Function, Configuration Function,
     * Outgoing Processing, Incoming Processing.
     */
#if(INCLUDE_SNMPv1 == NU_TRUE)
    {SNMP_CBSM_V1, CBSM_Init, CBSM_Config,
        NU_NULL,    CBSM_V1_Verify},
#endif
#if(INCLUDE_SNMPv2 == NU_TRUE)
    {SNMP_CBSM_V2, CBSM_Init, CBSM_Config,
        NU_NULL,    CBSM_V2_Verify},
#endif
#if(INCLUDE_SNMPv3 == NU_TRUE)
    {SNMP_USM, USM_Init , USM_Config,
        USM_Secure, USM_Verify}
#endif
};


/* --------- Configurations for the User-based Security Model. -------- */

/* All the Authentication Protocols are added in to the following array.
 * Please make sure that the header file for the given protocol is also
 * included above. USM_MAX_AUTH_PROTOCOLS will also need to be updated
 * in snmp_cfg.h.
 *
 * NOTE: The first entry in the array is default. Do not modify this
 * entry.
 */
#if(INCLUDE_SNMPv3 == NU_TRUE)
USM_AUTH_PROT_STRUCT         Usm_Auth_Prot_Table[USM_MAX_AUTH_PROTOCOLS] =
{
    /* Protocol Index, Outgoing Processing, Incoming Processing,
     * Key Calculation, Key Change, Authentication Data Length, Key Length.
     */
    {USM_NOAUTH, NU_NULL, NU_NULL,
        NU_NULL, NU_NULL, 0, 0},
    {USM_MD5, USM_Md5_Secure, USM_Md5_Verify,
        USM_Md5_Key, USM_Md5_Key_Change, 12, 16},
    {USM_SHA, USM_Sha_Secure, USM_Sha_Verify,
        USM_Sha_Key, USM_Sha_Key_Change, 12, 20}
};


/* All the Privacy Protocols are added in to the following array. Please
 * make sure that the header file for the given protocol is also included
 * above. USM_MAX_PRIV_PROTOCOLS will also need to be updated in
 * snmp_cfg.h.

 * NOTE: The first entry in the array is default. Do not modify this
 * entry.
 */
USM_PRIV_PROT_STRUCT         Usm_Priv_Prot_Table[USM_MAX_PRIV_PROTOCOLS] =
{
    /* Protocol Index, Outgoing Processing, Incoming Processing,
     * Privacy Parameter's Length.
     */
    {USM_NOPRIV, NU_NULL, NU_NULL,
        0},
    {USM_DES, USM_Des_Encrypt, USM_Des_Decrypt,
        14}
};

/* The following users for the User-based Security model are added during
 * the initial configuration. USM is used in SNMPv3 messages.
 */
#if (USM_MAX_USER_USERS > 0)

SNMP_USM_USER_STRUCT            Usm_User_Table[USM_MAX_USER_USERS] =
{
    /* User name, Authentication protocol, Authentication password,
     * Privacy protocol, Privacy password.
     *
     * Note: The macros for the first entry are configured by the
     * configuration utility. The remaining two entries are templates
     * which can be used to clone new users.
     */
    {USM_USER_NAME_INITIAL, USM_MD5, USM_AUTH_PASSWORD_INITIAL,
        USM_DES, USM_PRIV_PASSWORD_INITIAL},
    {"templateMD5", USM_MD5, "template",
        USM_DES, "template"},
    {"templateSHA", USM_SHA, "template",
        USM_DES, "template"}
};

#endif
#endif

/* ----- End of Configurations for the User-based Security Model. ----- */

/* ------------ Configurations of for CBSMv1 and CBSMv2. -------------- */

#if((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))
#if (CBSM_MAX_COMMUNITIES > 0)

SNMP_CBSM_COMMUNITY_STRUCT  CBSM_Community_Table[CBSM_MAX_COMMUNITIES] =
{
    /* Community name, Security Name, Group Name, Context Name. */
    {"public", "public", "group1", ""}
};

#endif
#endif

/* --------- End of Configurations for CBSMv1 and CBSMv2. ------------- */
/* ----------------- Configurations of target Hosts. ------------------ */

/* Nucleus SNMP is initialized with the following targets. These targets
 * are used to send traps/notifications. These targets are also used
 * to specify SNMP managers that belong to CBSMv1 and CBSMv2 communities.
 * The following target addresses need to be modified for your particular
 * network.
 */
#if (TGR_ADDR_TBL_SIZE > 0)

SNMP_TARGET_ADDR_TABLE Snmp_Cfg_Tgr_Addr_Tbl[TGR_ADDR_TBL_SIZE] = {
    {"Host1", SNMP_UDP, SNMP_HOST1_ADDRESS, NU_FAMILY_IP, 0, "group1", 6, "NoAuthNoPriv-v1", 15},
    {"Host2", SNMP_UDP, SNMP_HOST2_ADDRESS, NU_FAMILY_IP, 0, "group1", 6, "NoAuthNoPriv-v2", 15},
    {"Host3", SNMP_UDP, SNMP_HOST3_ADDRESS, NU_FAMILY_IP, 0, "group1", 6, "NoAuthNoPriv-v3", 15},
    {"Host4", SNMP_UDP, SNMP_HOST4_ADDRESS, NU_FAMILY_IP, 0, "group1", 6, "AuthNoPriv-v3", 13},
    {"Host5", SNMP_UDP, SNMP_HOST5_ADDRESS, NU_FAMILY_IP, 0, "group1", 6, "AuthPriv-v3", 11}
};

#endif

/* The following entries specify the parameters to use when sending
 * notifications to the targets defined above.
 */
#if (TGR_PARAMS_TBL_SIZE > 0)

SNMP_TARGET_PARAMS_TABLE_CONFIG Snmp_Cfg_Tgr_Params_Tbl[TGR_PARAMS_TBL_SIZE] =
{
    /* Entry name, MP Model, Security Model, Security Name (or Community
     * Name for SNMPv1 and SNMPv2), Security Level.
     */
    {"NoAuthNoPriv-v1", SNMP_VERSION_V1, SNMP_CBSM_V1, "public",
        SNMP_SECURITY_NOAUTHNOPRIV},
    {"NoAuthNoPriv-v2", SNMP_VERSION_V2, SNMP_CBSM_V2, "public",
        SNMP_SECURITY_NOAUTHNOPRIV},
    {"NoAuthNoPriv-v3", SNMP_VERSION_V3, SNMP_USM,     "initial",
        SNMP_SECURITY_NOAUTHNOPRIV},
    {"AuthNoPriv-v3"  , SNMP_VERSION_V3, SNMP_USM,     "initial",
        SNMP_SECURITY_AUTHNOPRIV},
    {"AuthPriv-v3"    , SNMP_VERSION_V3, SNMP_USM,     "initial",
        SNMP_SECURITY_AUTHPRIV}
};

#endif
/* ------------- End of Configurations for target Hosts. -------------- */
/* ------Configurations of Notification Originator Application. ------ */

/* The Notify table is initialized with the following entries. For each
 * entry in the notify table, a notification is sent to all targets
 * that have matching tag in their tag list.
 */

#if (NOTIFY_TBL_SIZE > 0)

SNMP_NOTIFY_TABLE_CONFIG Snmp_Cfg_Notify_Tbl[NOTIFY_TBL_SIZE] =
{
    /* Entry name, Target Tag, Tag Length. */
    {"group1", "group1", 6}
};

#endif

/* Filter profile table is initialized with the following entries.
 * Each target is associated with a parameters entry through the
 * params name. This table associates each parameters entry with
 * a filter.
 */

#if (FLTR_PROF_TBL_SIZE > 0)

SNMP_NOTIFY_FILTER_PROFILE_TABLE_CONFIG Snmp_Cfg_Fltr_Prof_Tbl[FLTR_PROF_TBL_SIZE] =
{
    /* Params Name, Filter Name */
    {"NoAuthNoPriv-v1", "filter-1"},
    {"NoAuthNoPriv-v2", "filter-1"},
    {"NoAuthNoPriv-v3", "filter-1"},
    {"AuthNoPriv-v3",   "filter-1"},
    {"AuthPriv-v3",     "filter-1"}
};

#endif

/* Notify filter profile table is initialized with the following entries.
 * Each notification passes a filter if the filter does not exclude
 * notification OID or the objects in the notification. The filter also
 * needs to specifically include the notification OID.
 */
#if (FLTR_TBL_SIZE > 0)

SNMP_NOTIFY_FILTER_TABLE_CONFIG Snmp_Cfg_Fltr_Tbl[FLTR_TBL_SIZE] =
{
    /* Filter Name, Subtree, Subtree len, Filter Mask, Mask Length
     * (in octets), Filter Type.
     */
    {"filter-1", {1,3,6,1}, 4, {0xF0}, 1,
        1}
};

#endif

/* ---------------- End of Notification Configurations. --------------- */

/* ----- Configurations for the View-based Access Control Model. ------ */

#if (VACM_CONTEXT_TBL_SIZE > 0)

/* Context names that will be initialized. "" is the default context. */
UINT8 Snmp_Cfg_Context_Names[VACM_CONTEXT_TBL_SIZE][SNMP_SIZE_SMALLOBJECTID] = {
    ""
};

#endif

#if (VACM_SEC2GRP_TBL_SIZE > 0)

/* The access group associated with the combination of security model and
 * security name.
 */
VACM_SEC2GROUP Snmp_Cfg_Sec2Groups [VACM_SEC2GRP_TBL_SIZE] = {

    /* Security model, Security name, Group name. */
    {SNMP_CBSM_V1, "public", "group1"},
    {SNMP_CBSM_V2, "public", "group1"},
    {SNMP_USM, USM_USER_NAME_INITIAL, "group1"},
    {SNMP_USM, "templateMD5", "group1"},
    {SNMP_USM, "templateSHA", "group1"},
};

#endif

#if (VACM_ACCESS_TBL_SIZE > 0)

/* The access view which defines the read, write and notify access based
 * on security model, group name, context prefix and security level.
 */
VACM_ACCESS Snmp_Cfg_Access [VACM_ACCESS_TBL_SIZE] = {

    /* Security model, Group name, Context prefix, Security level,
       Context match, Read view, Write view, Notify view. */
    {SNMP_ANY, "group1", "", VACM_CTXTMATCH_EXACT,
        SNMP_SECURITY_NOAUTHNOPRIV, "restricted", "restricted",
        "restricted"},
    {SNMP_USM, "group1", "", VACM_CTXTMATCH_EXACT,
        SNMP_SECURITY_AUTHNOPRIV, "internet", "internet",
        "internet"}
};

#endif

#if (VACM_MIB_VIEW_TBL_SIZE > 0)

VACM_VIEWTREE Snmp_Cfg_Mib_View [VACM_MIB_VIEW_TBL_SIZE] = {
    /* View name, Subtree, Subtree len, Family mask, Mask length,
     * Family type.
     */
    {"restricted", {1, 3, 6, 1}, 4, {0xF0}, 1,
        VACM_FAMILY_INCLUDED},
    {"internet",   {1, 3, 6, 1}, 4, {0xF0}, 1,
        VACM_FAMILY_INCLUDED}
};

#endif

/* -- End of Configuration for the View-based Access Control Model. --- */

UINT32      cfig_snmpd                                  = SNMP_PORT;
UINT32      authentrap_onoff                            = ON;
UINT32      coldtrap_onoff                              = ON;
UINT8       cfig_portname[MAX_PORTS][SNMP_HOSTNAME_LEN] = {"Port1"};
UINT8       cfig_hostname[SNMP_HOSTNAME_LEN+1]          = "SNMPv3_Host";
UINT8       cfig_hostid[SNMP_MAX_IP_ADDRS];
UINT8       cfig_hosttype = 1;

/* ------------------- End of Configurable section  ------------------- */
/* ----------------- Utility routines for general configuration ------- */

UINT32  get_snmpport(VOID)              { return (cfig_snmpd); }
UINT32  get_authentrap(VOID)            { return (authentrap_onoff); }
UINT32  get_coldstarttrap(VOID)         { return (coldtrap_onoff); }
UINT32  get_numports(VOID)              { return (MAX_PORTS); }

/************************************************************************
*
*   FUNCTION
*
*       nc_init
*
*   DESCRIPTION
*
*       This function initializes the state of authentraps and
*       coldstarttraps.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID nc_init (VOID)
{
    if (authentrap_onoff == NU_TRUE)
        AgentSetAuthenTraps(NU_TRUE);
    else
        AgentSetAuthenTraps(NU_FALSE);

    if (coldtrap_onoff == NU_TRUE)
        AgentSetColdTraps(NU_TRUE);
    else
        AgentSetColdTraps(NU_FALSE);
}

