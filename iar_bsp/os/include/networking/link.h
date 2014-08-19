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
*       link.h                                                   
*
*   DESCRIPTION
*
*       This file contains the macros, data structures and function
*       declarations necessary to create and manage a link.
*
*   DATA STRUCTURES
*
*       link_s
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef LINK_H
#define LINK_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define LINK_FLAG_LINK    0x01
#define LINK_FLAG_BUFFER  0x02

typedef struct link_s   link_t;

struct link_s
{
    UINT8       *buffer;
    UINT16      offset;
    UINT16      length;
    link_t      *next;
    UINT16      size;
    UINT8       flags;
    UINT8       snmp_pad[1];
};

UINT8   *LinkPush(link_t **, UINT16);
UINT8   *LinkPop(link_t **, UINT16);
link_t  *LinkAlloc(link_t *, UINT8 *, UINT16, UINT16, UINT16, link_t *);
BOOLEAN LinkCopy(link_t *, UINT8 *, UINT16);
VOID    LinkFree(link_t *);
UINT16  LinkAvailSpace(const link_t *);
UINT16  LinkAvailData(const link_t *);
UINT16  LinkLength(link_t *);
UINT16  LinkSize(link_t *);
BOOLEAN LinkSplit(link_t *, link_t *, UINT16);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
