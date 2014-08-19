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
*       um_defs.h
*
*   COMPONENT
*
*       User Management Interface
*
*   DESCRIPTION
*
*       Defines the User Management interfaces.
*
*   DATA STRUCTURES
*
*       UM_USER_ELEMENT
*       UM_USER
*       UM_USER_LIST
*
*   DEPENDENCIES
*
*       os.h
*
*************************************************************************/


#ifndef UM_DEFS_H
#define UM_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Customer Defined permission bit maps */

#define UM_FTP      0x00000001      /* give user access to FTP services */
#define UM_PPP      0x00000002      /* give user access to PPP services */
#define UM_WEB      0x00000004      /* give user access to WEBSERV services */
#define UM_SSH      0x00000008      /* give user access to SSHSERV services */
#define UM_ALL      0xFFFFFFFFul    /* give user access to all NET products */

/* flags */
#define UM_UPDATE_MODE      0x01    /* Update user info. */
#define UM_ADD_MODE         0x02    /* Add new User */

/* User Management Error Codes (-3751 to -3800) */
#define UM_USER_UNKNOWN       -3751    /* user name does not exists within UM database */
#define UM_USER_LISR_EMPTY    -3752    /* no users defined in the UM database */
#define UM_USER_EXISTS        -3753    /* user already exists error when adding */
#define UM_INVALID_NAME       -3754    /* user name is not valid */
#define UM_INVALID_PASSWORD   -3755    /* password is not valid */
#define UM_INVALID_PERMISSION -3756    /* permission is not valid */
#define UM_PW_MISMATCH        -3757    /* password mismatch */
#define UM_PV_MISMATCH        -3758    /* permissions mismatch */
#define UM_LIST_FULL          -3759    /* unable to assign a unique user id. */

/* User information entry */

struct um_user_element
{
    struct um_user_element *flink;         /* pointer to next UM entry (DLL.C I/F) */
    struct um_user_element *blink;         /* pointer to prev UM entry (DLL.C I/F) */
    CHAR um_name[UM_MAX_NAME_SIZE + 1];    /* ASCII name field */
    CHAR um_pw[UM_MAX_PW_SIZE + 1];        /* ASCII password field */
    UINT32 um_pv;                          /* NET services permission field */
    UINT32 um_id;                          /* Unique identifier for this entry. */
};

typedef struct um_user_element UM_USER_ELEMENT;

/* application level user structure. */
struct um_app_user_info
{
    CHAR um_name[UM_MAX_NAME_SIZE + 1];    /* ASCII name field */
    CHAR um_pw[UM_MAX_PW_SIZE + 1];        /* ASCII password field */
    UINT32 um_pv;                          /* NET services permission field */
    UINT32 um_id;                          /* Unique identifier for this entry. */
};

typedef struct um_app_user_info UM_USER;

struct um_user_list
{
    struct um_user_element *flink;            /* pointer to next UM entry (DLL.C I/F) */
    struct um_user_element *blink;            /* pointer to prev UM entry (DLL.C I/F) */
};

typedef struct um_user_list UM_USER_LIST;


/* User Management Prototypes */

VOID UM_Initialize(VOID);
STATUS UM_Check_User_Name (const CHAR *name);
STATUS UM_Check_Pass_Word (const CHAR *pw);
STATUS UM_Add_User (const CHAR *name, const CHAR *pw, UINT32 pv, UINT16 mode);
STATUS UM_Delete_User_Permissions(const CHAR *name, UINT32 pv);
STATUS UM_Find_User (const CHAR *name, UM_USER *dp);
STATUS UM_Find_User_Next (const CHAR *name, UM_USER *dp, UINT32 pv);
STATUS UM_Find_User_First (UM_USER *dp, UINT32 pv);
STATUS UM_Validate_User (const CHAR *name, const CHAR *pw, UINT32 pv);
UINT16 UM_Get_User_Count (UINT32 pv);
VOID   UM_Initialize(VOID);
STATUS UM_FindUserByID(UINT32 id, UM_USER *user);
STATUS UM_Find_User_By_ID(UINT32 id, UM_USER *user);

#define UM_Del_User(user)   UM_Delete_User_Permissions(user, 0xFFFFFFFFUL)


UM_USER_ELEMENT *UM_Scan_User (const CHAR *name);

/* External variables and functions */
extern UM_USER_LIST UM_List;                    /* User Management anchor node */
extern NU_PROTECT   UM_Protect;                 /* protection for UM global variables */
extern UINT32       NET_Initialized_Modules;    /* module init bit map */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
