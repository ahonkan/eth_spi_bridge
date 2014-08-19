/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       ripng.h                                                               
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       Holds the defines for the RIPng protocol.                         
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       RIPNG_PACKET
*       RIPNG_HEADER
*       RIPNG_ENTRY
*       RIPNG_STRUCT
*       RIPNG_LIST_NODE
*       RIPNG_LIST_STRUCT
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*      socketd.h
*      target.h
*                                                                       
*************************************************************************/

#ifndef _RIPNG_H_
#define _RIPNG_H_

#include "networking/socketd.h"
#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define RIPNG_VERSION1          1

#define RIPNG_PORT              521

#define RIPNG_REQUEST           1
#define RIPNG_RESPONSE          2

#define RIPNG_NEXTHOP_METRIC    0xFF

#define RIPNG_HEADER_LEN        4
#define RIPNG_RTE_LEN           20

struct ripng_header 
{
    UINT8   command;
    UINT8   version;
    INT16   unused;
    UINT8   ripng_prefix[IP6_ADDR_LEN];
    UINT16  ripng_routetag;
    UINT8   ripng_prefix_len;
    UINT8   ripng_metric;
};

typedef struct ripng_header RIPNG_HEADER;

struct ripng_entry 
{
    UINT8   ripng_prefix[IP6_ADDR_LEN];
    UINT16  ripng_routetag;
    UINT8   ripng_prefix_len;
    UINT8   ripng_metric;
};

typedef struct ripng_entry RIPNG_ENTRY;

/* This is the RIPng initialization structure. Applications will fill in one of
 * these for each device that RIPng should be used with. 
 */
typedef struct _RIPNG_STRUCT
{
    CHAR            *ripng_device_name;
    UINT8           ripng_metric;
    UINT8           ripng_padN[3];
    STATUS          ripng_status;
} RIPNG_STRUCT;

typedef struct _RIPNG_DEVICE
{
    UINT8   ripng_dev_metric;
    UINT8   ripng_dev_padN[3];
    UINT32  ripng_dev_index;
    UINT32  ripng_dev_mtu;
} RIPNG_DEVICE;

/* There will be a list of these that includes one for each device that RIPng 
 * is used with. 
 */
typedef struct _RIPNG_LIST_NODE
{
    struct _RIPNG_LIST_NODE     *ripng_next;
    struct _RIPNG_LIST_NODE     *ripng_prev;
    RIPNG_DEVICE                ripng_device;
} RIPNG_LIST_NODE;

typedef struct _RIPNG_LIST_STRUCT
{
    RIPNG_LIST_NODE      *ripng_head;
    RIPNG_LIST_NODE      *ripng_tail;
} RIPNG_LIST_STRUCT;


/* The function prototypes known to the outside world. */
STATUS NU_Ripng_Initialize(RIPNG_STRUCT *ripng, INT num);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* RIPNG_H */
