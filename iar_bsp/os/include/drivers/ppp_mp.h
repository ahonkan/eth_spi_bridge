/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_mp.h
*
*   COMPONENT
*
*       MP - PPP Multilink Protocol
*
*   DESCRIPTION
*
*       This include file defines constants and data structures
*       relating to the PPP Multilink Protocol (MP).
*
*   DATA STRUCTURES
*
*       PPP_MP_BUNDLE
*       PPP_MP_BUNDLE_LIST
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_MP_H
#define PPP_MP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* This is a PPP Multilink Protocol's bundle structure. Each bundle is
   defined by a collection of links between two peers. Each bundle has a
   MP virtual interface associated with it. */
typedef struct ppp_mp_bundle
{
    /* The following two pointers are used to maintain a list of this
    structure. */
    struct ppp_mp_bundle    *mp_flink;
    struct ppp_mp_bundle    *mp_blink;

    /* List of the fragments received, which are not yet re-assembled. */
    NET_BUFFER              *mp_fragment_list;

    /* Interface index of the virtual MP device associated with the bundle
   . */
    UINT32                  mp_device_ifindex;

    /* Sequence number of the last fragment sent. */
    UINT32                  mp_sequence_no;

    /* Current minimum of the most recently received sequence number over all
    the links. */
    UINT32                  mp_latest_min_seq_num;

    /* Total baud rate of all the links in the bundle. */
    UINT32                  mp_total_baud_rate;

    /* Number of E bit fragments in the fragment list. E bit is used to
    indicate the last fragment of a packet. */
    UINT8                   mp_e_bit;

    /* Number of links in the bundle. */
    UINT8                   mp_num_links;

    /* Correct alignment (padding) for 32-bit CPUs. */
    UINT8                   NU_PAD[2];

}PPP_MP_BUNDLE;


/* The following structure is used to maintain a list of all bundles. */
typedef struct ppp_mp_bundle_list
{
    struct ppp_mp_bundle    *mp_flink;
    struct ppp_mp_bundle    *mp_blink;

}PPP_MP_BUNDLE_LIST;


/* PPP MP Header related defines. */
#define PPP_MP_PID_SIZE                 2
#define PPP_MP_PID                      0x003d
#define PPP_MP_COM_PID                  0x3d
#define PPP_MP_HEADER_B_BIT             0x80
#define PPP_MP_HEADER_E_BIT             0x40
#define PPP_MP_LONG_SEQ_NUM_OFFSET      6
#define PPP_MP_LONG_FRAG_DATA_OFFSET    4
#define PPP_MP_SHORT_FRAG_DATA_OFFSET   2
#define PPP_MP_LONG_FRAG_HEAD_SIZE      4
#define PPP_MP_SHORT_FRAG_HEAD_SIZE     2

#define PPP_MP_USER_NAME                1
#define PPP_MP_END_DISCRIMINATOR        2
#define PPP_MP_END_DISCRIMINATOR_CLASS  3
#define PPP_MP_MRRU                     4
#define PPP_MP_GET_ALL_LINKS            5
#define PPP_MP_IS_INTERFACE             6
#define PPP_MP_NEXT_INTERFACE           7

/* PPP MP Application Services. */
#define NU_MP_Dial_Link                     PPP_MP_Dial_Link
#define NU_MP_Terminate_Links               PPP_MP_Terminate_Links
#define NU_MP_Get_Virt_If_By_Device         PPP_MP_Get_Virt_If_By_Device
#define NU_MP_Get_Virt_If_By_User           PPP_MP_Get_Virt_If_By_User
#define NU_MP_Get_Opt                       PPP_MP_Get_Opt


/* NULL CLASS. */
#define MP_CLASS_NULL                       LCP_ENDPOINT_DISC_NULL
/* Internet Protocol Address. */
#define MP_CLASS_IP                         LCP_ENDPOINT_DISC_IP
/* IEEE 802.1 Globally Assigned MAC Address. */
#define MP_CLASS_MAC                        LCP_ENDPOINT_DISC_MAC
/* Public Switched Network Directory Number. */
#define MP_CLASS_PSND                       LCP_ENDPOINT_DISC_DIRECTORY

/* Function prototypes. */
STATUS PPP_MP_Initialize(VOID);
STATUS PPP_MP_Init(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_MP_Attach_Link(DV_DEVICE_ENTRY *mp_dev,
                          DV_DEVICE_ENTRY *ppp_dev);
STATUS PPP_MP_Add_Link(DV_DEVICE_ENTRY *ppp_dev, DV_DEVICE_ENTRY *mp_dev);
STATUS PPP_MP_Find_Bundle_By_Device (PPP_MP_BUNDLE **bundle,
                                     DV_DEVICE_ENTRY *ppp_dev_ptr);
STATUS PPP_MP_Create_Bundle(UINT32 mp_dev_index);
STATUS PPP_MP_Find_Bundle_By_Device (PPP_MP_BUNDLE **bundle,
                                     DV_DEVICE_ENTRY *ppp_dev_ptr);
STATUS PPP_MP_Delete_Bundle(PPP_MP_BUNDLE *bundle);
STATUS PPP_MP_Terminate_Links(UINT32 mp_ifindex);
STATUS PPP_MP_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *dev_ptr,
                     VOID *, VOID *);
STATUS PPP_MP_Input(VOID);
STATUS PPP_MP_Find_Bundle_By_User(PPP_MP_BUNDLE *bundle, CHAR *user);
STATUS PPP_MP_Get_Virt_If_By_Device (UINT32 *mp_ifindex, CHAR *link_name);
STATUS PPP_MP_Get_Virt_If_By_User (UINT32 *mp_ifindex, CHAR *user_name);
STATUS PPP_MP_Get_Opt(UINT32 mp_ifindex, INT optname, VOID *optval,
                      INT *optlen);
VOID PPP_MP_Remove_Link(UINT32 ppp_ifindex);
STATUS PPP_MP_Get_Bundle(PPP_MP_BUNDLE **bundle, DV_DEVICE_ENTRY *dev_ptr,
                         DV_DEVICE_ENTRY *mp_ptr);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_MP_H */
