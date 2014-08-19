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
*       snmp_mp.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used by the Message Processing Subsystems.
*
*   DATA STRUCTURES
*
*       SNMP_ENCODE_RESPOND_STRUCT
*       SNMP_DECODE_REQUEST_STRUCT
*       SNMP_ENCODE_NOTIFY_STRUCT
*       SNMP_BUFF_STRUCT
*       SNMP_FREEBUFF_STRUCT
*       SNMP_BUFFLIST_STRUCT
*       SNMP_FREELIST_STRUCT
*       SNMP_REQLIST_STRUCT
*       SNMP_CNV_STRUCT
*
*   DEPENDENCIES
*
*       snmp_dis.h
*
************************************************************************/

#ifndef SNMP_MP_H
#define SNMP_MP_H

#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef STATUS (*SNMP_ENCODE_RESPOND_STRUCT)(SNMP_MESSAGE_STRUCT *,
                                             SNMP_SESSION_STRUCT *);
typedef STATUS (*SNMP_DECODE_REQUEST_STRUCT)(SNMP_MESSAGE_STRUCT *,
                                             SNMP_SESSION_STRUCT *);
typedef STATUS (*SNMP_ENCODE_NOTIFY_STRUCT)(SNMP_MESSAGE_STRUCT *,
                                             SNMP_SESSION_STRUCT *);

/* Structure used for calling functions for a particular
 * Message Processing Model.
 */
typedef struct snmp_mp_struct
{
    /* Message Processing Model no. */
    UINT32                      snmp_mp_model;

    /* Security Model. */
    UINT32                      snmp_sm;

    /* Callback function used for initializing SNMP Model. */
    SNMP_INIT                   snmp_init_cb;

    /* Callback function used for (re)configuring SNMP Model. */
    SNMP_CONFIG                 snmp_config_cb;

    /* Call back function used for encoding responses */
    SNMP_ENCODE_RESPOND_STRUCT  snmp_enc_response_cb;

    /* Call back function used for decoding requests */
    SNMP_DECODE_REQUEST_STRUCT  snmp_dec_request_cb;

    /* Call back function used for encoding notifications. */
    SNMP_ENCODE_NOTIFY_STRUCT   snmp_enc_notify_callback;

} SNMP_MP_STRUCT;

/* Structures for storing the incoming messages. */
typedef struct snmp_buff_struct
{
    struct snmp_buff_struct         *snmp_next_link;
    struct snmp_buff_struct         *snmp_prev_link;
    SNMP_MESSAGE_STRUCT             *snmp_request;
    UINT32                          size;
} SNMP_BUFF_STRUCT;

typedef struct snmp_freebuff_struct
{
    struct snmp_freebuff_struct     *snmp_next_link;
    struct snmp_freebuff_struct     *snmp_prev_link;
    UINT32                          size;
} SNMP_FREEBUFF_STRUCT;

typedef struct snmp_bufflist_struct
{
    SNMP_BUFF_STRUCT                *buff_list_head;
    SNMP_BUFF_STRUCT                *buff_list_tail;
} SNMP_BUFFLIST_STRUCT;

typedef struct snmp_freelist_struct
{
    SNMP_FREEBUFF_STRUCT            *free_list_head;
    SNMP_FREEBUFF_STRUCT            *free_list_tail;
} SNMP_FREELIST_STRUCT;

typedef struct snmp_reqlist_struct
{
    SNMP_BUFFLIST_STRUCT            snmp_buff_list;
    SNMP_FREELIST_STRUCT            snmp_free_list;
} SNMP_REQLIST_STRUCT;


/* This structure maps the class and tag values to syntax. */
typedef struct snmp_cnv_struct
{
    UINT32  snmp_class;
    UINT32  snmp_tag;
    INT32   snmp_syntax;
} SNMP_CNV_STRUCT;


/* Functions related to decoding and encoding. */
STATUS SNMP_Decode(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_Notify(SNMP_MESSAGE_STRUCT *snmp_notification,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_Encode(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session);
STATUS SNMP_Mp_Init(VOID);
STATUS SNMP_Mp_Config(VOID);

/* Functions related to maintaining the message buffer. */
STATUS SNMP_Init_ReqList(SNMP_REQLIST_STRUCT *req_list, UINT32 list_size);
SNMP_MESSAGE_STRUCT *SNMP_Add_ReqList(SNMP_REQLIST_STRUCT *request_list,
                                 const SNMP_MESSAGE_STRUCT *snmp_request);
STATUS SNMP_Remove_ReqList(SNMP_REQLIST_STRUCT *request_list,
                           const SNMP_MESSAGE_STRUCT *snmp_request);

/* The following two functions are commonly used by the message processing
 * models, while decoding/encoding.
 */
BOOLEAN SNMP_Syn2TagCls (UINT32 *tag, UINT32 *cls, UINT32 syn);
BOOLEAN SNMP_TagCls2Syn (UINT32 tag, UINT32 cls, UINT32 *syn);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_MP_H */

