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
*       rtab4.h
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       Holds the defines for IPv4 routing.
*
*   DATA STRUCTURES
*
*       rtab4_route_entry
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef _RTAB4_H
#define _RTAB4_H

#if (INCLUDE_IPV4 == NU_TRUE)

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

struct rtab4_route_entry
{
    struct rtab4_route_entry    *rt_entry_next;
    struct rtab4_route_entry    *rt_entry_prev;
    struct route_entry_parms    rt_entry_parms;
    struct route_node           *rt_route_node; /* ROUTE_NODE to which the entry
                                                 * belongs. */
    SCK_SOCKADDR_IP             rt_gateway_v4;  /* gateway for route, if any */
};

RTAB4_ROUTE_ENTRY   *RTAB4_Find_Route(const SCK_SOCKADDR_IP *, INT32);
STATUS              RTAB4_Add_Route(DV_DEVICE_ENTRY *, UINT32, UINT32,
                                    UINT32, UINT32);
STATUS              RTAB4_Set_Default_Route(DV_DEVICE_ENTRY *, UINT32,
                                            UINT32);
VOID                RTAB4_Init(VOID);
ROUTE_NODE          *RTAB4_Get_Root_Node(VOID);
UINT8               RTAB4_Find_Prefix_Length(const UINT8 *);
ROUTE_NODE          *RTAB4_Setup_New_Node(const ROUTE_NODE *);
INT                 RTAB4_Insert_Route_Entry(ROUTE_NODE *,
                                             const ROUTE_NODE *);
ROUTE_NODE          *RTAB4_Get_Default_Route(VOID);
VOID                RTAB4_Redirect(UINT32, UINT32, INT32, UINT32);
STATUS              RTAB4_Delete_Route(const UINT8 *, const UINT8 *);
STATUS              RTAB4_Delete_Routes_For_Device(const CHAR *);
RTAB4_ROUTE_ENTRY   *RTAB4_Find_Next_Route(const UINT8 *);
ROUTE_ENTRY         *RTAB4_Find_Next_Route_Entry(const ROUTE_ENTRY *current_route);
STATUS              RTAB4_Update_Route(const UINT8 *, const UINT8 *,
                                       const UPDATED_ROUTE_NODE *);
ROUTE_NODE          *RTAB4_Find_Route_For_Device(DV_DEVICE_ENTRY *);
STATUS              RTAB4_Delete_Route_Entry(ROUTE_NODE *,
                                             RTAB4_ROUTE_ENTRY *);
STATUS              RTAB4_Delete_Node(ROUTE_NODE *);
UINT8               RTAB4_Determine_Matching_Prefix(const UINT8 *, const UINT8 *,
                                                    const UINT8 *, UINT8);
ROUTE_ENTRY         *RTAB4_Find_Route_By_Gateway(const UINT8 *, const UINT8 *, INT32);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
VOID                RTAB4_Memory_Init(VOID);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef _RTAB4_H */
#endif

