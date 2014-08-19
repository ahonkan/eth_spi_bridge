/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   DESCRIPTION
*
*       This file contains definitions used in the 
*       event notification service routines.
*
*************************************************************************/

#ifndef EVENT_NOTIFICATION_H
#define EVENT_NOTIFICATION_H
 
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Variable size message overhead - Variable-length message pipes
   require an additional 32-bit word of overhead for each message */
#define EN_VAR_PIPE_OVER (sizeof(UINT32))

/* Maximum pipe message size */
#define EN_MAX_PIPE_MSG_LEN (EN_VAR_PIPE_OVER + sizeof(EN_NOTIFY_MSG))

/* Maximum pipe size */
#define EN_MAX_PIPE_LEN (EN_MIN_MSG_CNT * EN_MAX_PIPE_MSG_LEN)

/* Notification message header */
typedef struct EN_NOTIFY_HDR_STRUCT
{
    DV_DEV_ID   en_dev_id;
    UINT32      en_type;
    UINT8       en_msg_len;

} EN_NOTIFY_HDR;

/* Notification message structure */
typedef struct EN_NOTIFY_MSG_STRUCT
{
    EN_NOTIFY_HDR  en_hdr;
    UINT8          en_msg[EN_MAX_MSG_LEN];

} EN_NOTIFY_MSG;


/* Notification Registry */
typedef struct EN_REGISTRY_STRUCT
{
    INT en_entry_active_flag;
    INT en_listen_cnt;
    struct _en_listen_struct  /* Listen */
    {
        VOID*   en_handle;
        UINT32  en_type;
        UINT32  en_type_mask;

    } en_listen[EN_MAX_LISTEN_CNT];

} EN_REGISTRY;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* EVENT_NOTIFICATION_H */
