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
*       externs.h
*
*   DESCRIPTION
*
*       External definitions for functions in Nucleus NET.
*       This include file needs to go after other include files
*
*   DATA STRUCTURES
*
*
*   DEPENDENCIES
*
*       tcpdefs.h
*       target.h
*       mem_defs.h
*       bootp.h
*       dns.h
*       dev.h
*       ip.h
*       arp.h
*       rtab.h
*       mib2.h
*       netevent.h
*       net_bkcp.h
*
*************************************************************************/

#ifndef EXTERNS_H
#define EXTERNS_H

#include "networking/tcpdefs.h"
#include "networking/target.h"
#include "networking/mem_defs.h"
#include "networking/bootp.h"
#include "networking/dns.h"
#include "networking/dns_sd.h"
#include "networking/dev.h"
#include "networking/ip.h"
#include "networking/arp.h"
#include "networking/rtab.h"
#include "networking/netevent.h"
#include "networking/net_bkcp.h"
#include "networking/dll_i.h"
#include "networking/sll.h"
#include "networking/ip_tun.h"
#include "networking/netboot_query.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    #include "net/net_statcfg.h"         /* Include the configuration file
                                          *  for building static net */
#endif

/***** ETH.C *****/

VOID    statcheck (VOID);
STATUS  ETH_Add_Multi(DV_DEVICE_ENTRY *dev, const DV_REQ *d_req);
STATUS  ETH_Del_Multi(const DV_DEVICE_ENTRY *dev, const DV_REQ *d_req);
extern  NET_BUFFER_HEADER MEM_Buffer_List;
extern  NET_BUFFER_HEADER MEM_Buffer_Freelist;
extern  UINT8 IP_Time_To_Live;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
extern  NU_MEMORY_POOL    *MEM_Non_Cached;
#else
extern  CHAR              *MEM_Non_Cached;
#endif

/***/

#if (INCLUDE_IPV6 == NU_TRUE)
#define IP_IS_NULL_ADDR(addr)  \
    ((GET32(addr, 0)  == 0) && \
     (GET32(addr, 4)  == 0) && \
     (GET32(addr, 8)  == 0) && \
     (GET32(addr, 12) == 0) )
#else
#define IP_IS_NULL_ADDR(addr)  \
    (GET32(addr, 0)  == 0)
#endif

/***** BOOTP.C *****/

STATUS  NU_Bootp(const CHAR *dv_name, BOOTP_STRUCT *bp_ptr);
/***/

/***** DEV.C *****/

STATUS NU_Init_Devices(DEV_DEVICE *devices, INT dev_count);
STATUS NU_Device_Up(const CHAR *if_name);

/***/

/***** TLS.C *****/

UINT16  TLS_IP_Check (const UINT16 *s, UINT16 len);
UINT32  TLS_Longswap (UINT32);
UINT16  TLS_Intswap (UINT16);
INT     TLS_Comparen (const VOID *s1, const VOID *s2, unsigned int len);
UINT16  TLS_IP_Check_Buffer_Chain (NET_BUFFER *buf_ptr);
UINT16  TLS_TCP_Check (UINT16 *, NET_BUFFER *);
UINT32  TLS_Header_Memsum(VOID *s, UINT32 n);
VOID   *TLS_Normalize_Ptr(VOID *);
VOID    TLS_Put64(unsigned char *, unsigned int, long long);
VOID    TLS_Put32(unsigned char *, unsigned int, unsigned long);
VOID    TLS_Put16(unsigned char *, unsigned int, unsigned short);
VOID   *TLS_Put_String(unsigned char *dest, unsigned int offset,
                       const unsigned char *src, unsigned int size);
VOID   *TLS_Get_String(const unsigned char *src, unsigned int offset,
                       unsigned char *dest, unsigned int size);
int     TLS_Eq_String(const unsigned char *packet, unsigned int offset,
                      const unsigned char *local, unsigned int size);

unsigned long long TLS_Get64(unsigned char *, unsigned int);
unsigned long   TLS_Get32(unsigned char *, unsigned int);
unsigned short  TLS_Get16(unsigned char *, unsigned int);
unsigned long   TLS_IP_Address(const unsigned char *p);

/***/

/***** NBC.C *****/

VOID* NU_Block_Copy(VOID *d, const VOID *s, unsigned int n);

/***/

/***** ARP.C *****/

VOID    ARP_Init (VOID);
#define NU_Rarp     ARP_Rarp

/***/

/***** UTL.C *****/

VOID    UTL_Zero(VOID *ptr, UNSIGNED size);
UINT16  UTL_Checksum (NET_BUFFER *, UINT32, UINT32, UINT8);
UINT32  UTL_Rand(VOID);
VOID*   UTL_Sum_Memcpy(VOID *, const VOID *, UINT32, UINT32 *);

/***/

/***** DLL.C *****/

VOID    *DLL_Dequeue(VOID *h);
VOID    *DLL_Insert(VOID *h, VOID *i, VOID *l);
VOID    *DLL_Remove(VOID *h, VOID *i);
VOID    *DLL_Remove_Node(VOID *h, VOID *i);
VOID    *DLL_Enqueue(VOID *h, VOID *i);

/***/

/***** MEM.C *****/

STATUS MEM_Init(VOID);
#ifdef NU_DEBUG_NET_BUFFERS
NET_BUFFER *MEM_DB_Buffer_Dequeue(NET_BUFFER_HEADER *hdr, CHAR *, INT);
#else
NET_BUFFER *MEM_Buffer_Dequeue(NET_BUFFER_HEADER *hdr);
#endif
NET_BUFFER *MEM_Buffer_Enqueue (NET_BUFFER_HEADER *hdr, NET_BUFFER *item);
#ifdef NU_DEBUG_NET_BUFFERS
NET_BUFFER *MEM_DB_Buffer_Chain_Dequeue (NET_BUFFER_HEADER *header,
                                            INT32 nbytes, CHAR *, INT);
#else
NET_BUFFER *MEM_Buffer_Chain_Dequeue (NET_BUFFER_HEADER *header,
                                        INT32 nbytes);
#endif
NET_BUFFER *MEM_Update_Buffer_Lists (NET_BUFFER_HEADER *source,
                                     NET_BUFFER_HEADER *dest);
NET_BUFFER *MEM_Buffer_Insert(NET_BUFFER_HEADER *, NET_BUFFER *,
                              NET_BUFFER*, NET_BUFFER *);
VOID    MEM_Buffer_Chain_Free (NET_BUFFER_HEADER *source,
                             NET_BUFFER_HEADER *dest);
VOID    MEM_One_Buffer_Chain_Free (NET_BUFFER *source,
                                              NET_BUFFER_HEADER *dest);
VOID    MEM_Cat (NET_BUFFER *dest, NET_BUFFER *src);
VOID    MEM_Trim (NET_BUFFER *buf_ptr, INT32 length);
VOID    MEM_Buffer_Remove (NET_BUFFER_HEADER *hdr, NET_BUFFER *item);
VOID    MEM_Buffer_Cleanup (NET_BUFFER_HEADER *hdr);
VOID    MEM_Chain_Copy(NET_BUFFER *dest, NET_BUFFER *src, INT32 off,
                                INT32 len);
VOID    MEM_Buffer_Suspension_HISR (VOID);
INT32   MEM_Copy_Data(NET_BUFFER *buf_ptr, const CHAR HUGE *buffer,
                      INT32 numbytes, UINT16 flags);
INT32   MEM_Copy_Buffer(CHAR HUGE *buffer,
                       const struct sock_struct *sockptr, INT32 numbytes);
VOID    MEM_Multiple_Buffer_Chain_Free(NET_BUFFER *source);

/***/


/***** PROTINIT.C *****/

STATUS  PROT_Protocol_Init(VOID);


/***** NET.C *****/

STATUS NU_Init_Net(const NU_NET_INIT_DATA *init_struct);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
extern NU_MEMORY_POOL *MEM_Cached;
#endif


extern UNSIGNED  NET_Unused_Parameter;

/***/

/***** NLOG_CAE.c *****/
#define NU_Clear_Internal_Log   NLOG_Clear_All_Errors


/***** SCK_***.C *****/

INT     NU_Socket (INT16, INT16, INT16);
STATUS  NU_Bind (INT, struct addr_struct *, INT16);
STATUS  NU_Connect (INT, struct addr_struct *, INT16);
STATUS  NU_Listen (INT, UINT16);
STATUS  NU_Accept (INT, struct addr_struct *, INT16 *);
INT32   NU_Send (INT, CHAR *, UINT16, INT16);
INT32   NU_Send_To (INT, CHAR *, UINT16, INT16, const struct addr_struct *,
                    INT16);
INT32   NU_Send_To_Raw(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags,
                       const struct addr_struct *to, INT16 addrlen);
INT32   NU_Sendmsg(INT socketd, const msghdr *msg, INT16 flags);
INT32   NU_Recv (INT, CHAR *, UINT16, INT16);
INT32   NU_Recv_From (INT, CHAR *, UINT16, INT16, struct addr_struct *,
                      INT16 *);
INT32   NU_Recv_From_Raw (INT, CHAR *, UINT16, INT16,
                          struct addr_struct *, INT16 *);
INT32   NU_Recvmsg(INT, msghdr *, INT16);
STATUS  NU_Push (INT);
STATUS  NU_Is_Connected (INT);
INT16   UDP_Get_Pnum (const struct sock_struct *);
STATUS  NU_Fcntl (INT socketd, INT16 command, INT16 arguement);

STATUS  NU_Get_Host_By_Name(CHAR *name, NU_HOSTENT *host_entry);
NU_HOSTENT *NU_Get_IP_Node_By_Name(CHAR *name, INT16 family, INT16 flags,
                                   STATUS *error_num);
STATUS  NU_Get_Host_By_Addr(CHAR *, INT, INT, NU_HOSTENT *);
NU_HOSTENT *NU_Get_IP_Node_By_Addr(CHAR *addr, INT len, INT type,
                                   STATUS *error_num);
STATUS  NU_Free_Host_Entry(NU_HOSTENT *host_entry);
STATUS  NU_Get_Peer_Name(INT, struct sockaddr_struct *, INT16 *);
STATUS  NU_Get_Sock_Name(INT, struct sockaddr_struct *, INT16 *);
STATUS  NU_Close_Socket (INT);
VOID    SCK_Suspend_Task(NU_TASK *);
INT     SCK_SearchTaskList(struct TASK_TABLE_STRUCT *, INT16, INT);
STATUS  SCK_TaskTable_Entry_Delete(INT);

STATUS  NU_Socket_Connected(INT);
STATUS  SCK_Socket_Connected(INT socketd);
STATUS  NU_Setsockopt(INT, INT, INT, VOID *, INT);
STATUS  NU_Getsockopt(INT, INT, INT, VOID *, INT *);
STATUS  NU_Abort(INT socketd);
INT     SCK_Create_Socket(INT protocol, INT16 family);
VOID    SCK_Cleanup_Socket(INT socketd);
STATUS  NU_Ioctl(INT optname, SCK_IOCTL_OPTION *option, INT optlen);
STATUS  NU_Add_Route(const UINT8 *dest, const UINT8* mask,
                     const UINT8 *gw_addr);
STATUS  NU_Delete_Route2(const UINT8 *ip_dest, const UINT8 *next_hop, INT16 family);
VOID    SCK_Kill_All_Open_Sockets(const DV_DEVICE_ENTRY *dev_ptr);
VOID    SCK_Resume_All (struct SCK_TASK_ENT *, INT);
INT     SCK_Get_Host_Name (CHAR *name, INT name_length);
INT     NU_Find_Socket (INT protocol, const struct addr_struct *local_addr,
                        const struct addr_struct *foreign_addr);
STATUS  NU_Detach_IP_From_Device(const CHAR *name);
STATUS  DEV_Detach_IP_From_Device(const CHAR *name);
STATUS  NU_Update_Route(const UINT8 *ip_address, const UINT8 *gateway,
                        UPDATED_ROUTE_NODE *new_route, INT16 family);
STATUS  NU_ARP_Update(ARP_ENTRY *arp_changes, const UINT8 *target_ip);
STATUS  NU_Ping2(UINT8 *ip_addr, UINT32 timeout, INT16 family);

STATUS  SCK_Recv_IF_Addr(INT socketd, UINT8 *if_addr);
INT     SCK_Get_Domain_Name (CHAR *name, INT name_length);
STATUS  SCK_Set_Socket_Error(struct sock_struct *sckptr, INT32 error);

INT     SCK_Set_Domain_Name (const CHAR *name, INT name_length);
INT     SCK_Set_Host_Name (const CHAR *name, INT name_length);

STATUS  SCK_Parse_Ancillary_Data(const msghdr *msg,
                                 tx_ancillary_data *anc_data_ptr);

VOID    SCK_Store_Ancillary_Data(struct sock_struct *, msghdr *);
VOID    SCK_Set_RX_Anc_BufPtr(struct sock_struct *, NET_BUFFER *);

STATUS  SCK_Set_TX_Sticky_Options(tx_ancillary_data **, const VOID *, INT, INT);

STATUS  SCK_Protect_Socket_Block(INT socketd);
VOID    SCK_Release_Socket(VOID);


/***/

/***** SCK_SEL.C *****/

STATUS  NU_Select(INT, FD_SET *, FD_SET *, FD_SET *, UNSIGNED);
INT     NU_FD_Check(INT, const FD_SET *);
VOID    NU_FD_Set(INT, FD_SET *);
VOID    NU_FD_Init(FD_SET *fd);
VOID    NU_FD_Reset(INT, FD_SET *);

/***/

/***** ICMP.C *****/

VOID    ICMP_Init(VOID);
STATUS  ICMP_Send_Echo_Request(const UINT8 *, UINT32);
#define NU_Ping ICMP_Send_Echo_Request

/***/

/***** IP.C *****/

extern  UINT8 IP_Forwarding;

STATUS  NU_Set_Default_TTL(UINT8 default_ttl);
STATUS  NU_Set_IP_Forwarding(UINT8 forwarding);

#define NU_Get_Default_TTL()     IP_Time_To_Live
#define NU_Get_IP_Forwarding()   IP_Forwarding

/***/

/***** IP_GTOS.C *****/

VOID IP_Getsockopt_IP_TOS(INT socketd, UINT8 *optval, INT *optlen);

/***** IP_STOS.C *****/

STATUS IP_Setsockopt_IP_TOS(INT socketd, UINT8 dscp);

/***** SOL_GRB.C *****/

STATUS SOL_Getsockopt_SO_RCVBUF(INT, INT32 *);

/***** SCK_GRB.C *****/

STATUS NU_Getsockopt_SO_RCVBUF(INT, INT32 *);

/***** SOL_SRB.C *****/

STATUS SOL_Setsockopt_SO_RCVBUF(INT, INT32);

/***** SCK_SRB.C *****/

STATUS NU_Setsockopt_SO_RCVBUF(INT, INT32);

/***** PRT.C *****/

INT32   PRT_Find_Matching_Port(INT16, INT, const UINT8 *, const UINT8 *,
                               UINT16, UINT16);
UINT16  PRT_Get_Unique_Port_Number(UINT16, INT16);
INT     PRT_Is_Unique_Port_Number(UINT16, UINT16, INT16, const UINT8 *, UINT8);

/***** PRT4.C *****/

INT32   PRT4_Find_Matching_Port(INT, UINT32, UINT32, UINT16, UINT16);

/***** TQ.C *****/
STATUS  TQ_Timerset(TQ_EVENT, UNSIGNED, UNSIGNED, UNSIGNED);
STATUS  TQ_Timerunset(TQ_EVENT, INT16, UNSIGNED, UNSIGNED);
UNSIGNED TQ_Check_Duetime(UNSIGNED);

/***** EQ.C *****/
STATUS  EQ_Put_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID    EQ_Init(VOID);

/***** SCK_SSB.C *****/
STATUS  NU_Setsockopt_SO_BROADCAST(INT, INT16);

/***** SCK_GSB.C *****/
STATUS  NU_Getsockopt_SO_BROADCAST(INT, INT16 *, INT *);

/***** SOL_SSB.C *****/
VOID    SOL_Setsockopt_SO_BROADCAST(INT, INT16);

/***** SOL_GSB.C *****/
VOID    SOL_Getsockopt_SO_BROADCAST(INT, INT16 *, INT *);

/*****  SCK_SKA.C ****/
STATUS NU_Setsockopt_SO_KEEPALIVE(INT socketd, UINT8 opt_val);

/***** SCK_GKA.C *****/
STATUS NU_Getsockopt_SO_KEEPALIVE(INT socketd, INT *optval, INT *optlen);

/***** SCK_SSL.C *****/
STATUS  NU_Setsockopt_SO_LINGER(INT, struct sck_linger_struct);

/***** SCK_GSL.C *****/
STATUS  NU_Getsockopt_SO_LINGER(INT, struct sck_linger_struct *, INT *);

/***** SOL_SSL.C *****/
VOID    SOL_Setsockopt_SO_LINGER(INT, struct sck_linger_struct);

/***** SOL_GSL.C *****/
VOID    SOL_Getsockopt_SO_LINGER(INT, struct sck_linger_struct *, INT *);

/***** SCK_SRI.C *****/
STATUS  NU_Setsockopt_IP_RECVIFADDR(INT, UINT16);

/***** SCK_GRI.C *****/
STATUS  NU_Getsockopt_IP_RECVIFADDR(INT, INT16 *, INT *);

/***** SCK_SMI.C *****/
STATUS  NU_Setsockopt_IP_MULTICAST_IF(INT, UINT8 *);

/***** SCK_GMI.C *****/
STATUS  NU_Getsockopt_IP_MULTICAST_IF(INT, UINT8 *, INT *);

/***** SCK_AMM.C *****/
STATUS  NU_Setsockopt_IP_ADD_MEMBERSHIP(INT, IP_MREQ *);

/***** SCK_DMM.C *****/
STATUS  NU_Setsockopt_IP_DROP_MEMBERSHIP(INT, IP_MREQ *);

/***** SCK_SMTTL.C *****/
STATUS  NU_Setsockopt_IP_MULTICAST_TTL(INT, UINT8);

/***** SCK_GMTTL.C *****/
STATUS  NU_Getsockopt_IP_MULTICAST_TTL(INT, UINT8 *, INT *);

/***** SCK_STTL.C *****/
STATUS  NU_Setsockopt_IP_TTL(INT, UINT16);

/***** SCK_GTTL.C *****/
STATUS  NU_Getsockopt_IP_TTL(INT, UINT16 *, INT *);

/***** SCK_GTOS.C ****/

STATUS NU_Getsockopt_IP_TOS(INT socketd, UINT8 *optval, INT *optlen);

/***** SCK_STOS.C ****/

STATUS NU_Setsockopt_IP_TOS(INT socketd, UINT8 sck_tos);

/***** SCK_SHI.C *****/
STATUS  NU_Setsockopt_IP_HDRINCL(INT, INT16);

/***** SCK_GHI.C *****/
STATUS  NU_Getsockopt_IP_HDRINCL(INT, INT16 *, INT *);

/***** SCK_SNC.C *****/
STATUS  NU_Setsockopt_UDP_NOCHECKSUM(INT, UINT8);

/***** SCK_GNC.C *****/
STATUS  NU_Getsockopt_UDP_NOCHECKSUM(INT, INT16 *, INT *);

/***** SCK_SND.C *****/
STATUS  NU_Setsockopt_TCP_NODELAY(INT, UINT8);

/***** SCK_GND.C *****/
STATUS  NU_Getsockopt_TCP_NODELAY(INT, INT *, INT *);

/***** SCK_SBI.C *****/
STATUS  NU_Setsockopt_IP_BROADCAST_IF(INT, UINT8 *);

/***** SCK_GBI.C *****/
STATUS  NU_Getsockopt_IP_BROADCAST_IF(INT, UINT8 *, INT *);

/***** SCK_FRBG.C *****/
ROUTE_ENTRY *NU_Find_Route_By_Gateway(const UINT8 *, const UINT8 *, INT16, INT32);

/***** SCK_GDR.C *****/
ROUTE_NODE *NU_Get_Default_Route(INT16);

/***** SCK_FNR.C *****/
ROUTE_ENTRY *NU_Find_Next_Route(const UINT8 *, INT16);

/***** SCK_FNRE.C *****/
ROUTE_ENTRY *NU_Find_Next_Route_Entry(ROUTE_ENTRY *, INT16);

/***** SCK_CPA.C *****/
VOID SCK_Copy_Addresses(struct sock_struct *, UINT8 HUGE *,
                        struct addr_struct *, UINT16);

/***** IP_SRMS.C *****/
STATUS NU_Set_Reasm_Max_Size(const CHAR *dev_name, UINT32 reasm_max_size);

/***** IP_GRMS.C *****/
STATUS NU_Get_Reasm_Max_Size(const CHAR *dev_name, UINT32 *reasm_max_size);

/***** SCK_IN.C *****/
STATUS NU_Inet_NTOP(INT, VOID *, CHAR *, INT);

/***** SCK_IP.C *****/
STATUS NU_Inet_PTON(INT, CHAR *, VOID *);

/***** SCK_AITD.C *****/
STATUS  NU_Attach_IP_To_Device(const CHAR *, UINT8 *, const UINT8 *);

/***** SCK_SD.C *****/
STATUS NU_Shutdown(INT, INT);

/***** SCK_IPML.C *****/
STATUS NU_IP_Multicast_Listen(INT, UINT8*, UINT8*, UINT16, const UINT8 *,
                              UINT16);

/***** VLAN.C *****/

STATUS VLAN_Initialize (DV_DEVICE_ENTRY *);

STATUS VLAN_Ether_Send(NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *, VOID *);

DV_DEVICE_ENTRY *VLAN_Search_VID(UINT16, const DV_DEVICE_ENTRY *);

/***** MULTI.C *****/
VOID Multi_Init(VOID);
STATUS Multi_Update_Device_State(const DV_DEVICE_ENTRY *, const UINT8 *,
                                 const MULTI_SCK_STATE *, UINT8, UINT8);

MULTI_SCK_STATE *Multi_Get_Sck_State_Struct(const UINT8 *, const MULTI_SCK_OPTIONS *,
                                            const DV_DEVICE_ENTRY *, INT, INT16);
/***** MULTI_GSSS.C *****/
INT Multi_Verify_Src_By_Filter(const UINT8 *, VOID *, UINT8, UINT8);

/***** Zero Copy Interface *****/
INT32   NU_ZC_Recv(INT, NET_BUFFER **, UINT16, INT16);
INT32   NU_ZC_Recv_From(INT, NET_BUFFER **, UINT16, INT16,
                        struct addr_struct *, INT16 *);
INT32   NU_ZC_Allocate_Buffer(NET_BUFFER **, UINT16, INT);
STATUS  NU_ZC_Deallocate_Buffer(NET_BUFFER *);
UINT32  NU_ZC_Bytes_Left(NET_BUFFER *, const NET_BUFFER *, INT);

#define NU_ZC_Send_To(a,b,c,d,e,f) NU_Send_To(a,(CHAR *)(b),c,d,e,f)
#define NU_ZC_Send(a,b,c,d) NU_Send(a,(CHAR *)(b),c,d)

/***** SOL_SRA.C *****/
VOID SOL_Setsockopt_SO_REUSEADDR(INT socketd, INT16 optval);

/***** SCK_SRA.C *****/
STATUS NU_Setsockopt_SO_REUSEADDR(INT socketd, INT16 optval);

/***** SCK_GRA.C *****/
STATUS NU_Getsockopt_SO_REUSEADDR(INT socketd, INT16 *optval, INT *optlen);

/***** SOL_GRA.C *****/
VOID SOL_Getsockopt_SO_REUSEADDR(INT socketd, INT16 *optval, INT *optlen);

/***** SCK_NTI.C *****/
INT32 NU_IF_NameToIndex(const CHAR *if_name);

/***** SCK_IF_NI.C *****/
struct if_nameindex *NU_IF_NameIndex(VOID);

/***** SCK_IF_ITN.C *****/
CHAR *NU_IF_IndexToName(INT32 if_index, CHAR *if_name);

/***** SCK_IF_FNI.C *****/
VOID NU_IF_FreeNameIndex(struct if_nameindex *ptr);

/******* SCK_GIF.C *****/
STATUS NU_Ioctl_SIOCGIFADDR(SCK_IOCTL_OPTION *option, INT optlen);

/******* IOCTL_GIF.C *****/
STATUS Ioctl_SIOCGIFADDR(SCK_IOCTL_OPTION *option);

/******* SCK_GID.C ********/
STATUS NU_Ioctl_SIOCGIFDSTADDR(SCK_IOCTL_OPTION *option, INT optlen);

/******* IOCTL_GID.C *******/
STATUS Ioctl_SIOCGIFDSTADDR(SCK_IOCTL_OPTION *option);

/******* SCK_SPHY.C *******/
STATUS NU_Ioctl_SIOCSPHYSADDR(const SCK_IOCTL_OPTION *option)   ESAL_TS_RTE_DEPRECATED;

/****** IOCTL_SPHY.C ******/
STATUS Ioctl_SIOCSPHYSADDR(const SCK_IOCTL_OPTION *option);

/****** SCK_SHWA.C ******/
STATUS NU_Ioctl_SIOCSIFHWADDR(SCK_IOCTL_OPTION *option);

/****** IOCTL_SHWA.C ******/
STATUS Ioctl_SIOCSIFHWADDR(SCK_IOCTL_OPTION *option);

/****** SCK_GHWA.C ******/
STATUS NU_Ioctl_SIOCGIFHWADDR(SCK_IOCTL_OPTION *option);

/****** IOCTL_GHWA.C ******/
STATUS Ioctl_SIOCGIFHWADDR(SCK_IOCTL_OPTION *option);

/******* SCK_SVID.C *******/
STATUS NU_Ioctl_SIOCSETVLAN(SCK_IOCTL_OPTION *option);

/****** IOCTL_SVID.C ******/
STATUS Ioctl_SIOCSETVLAN(SCK_IOCTL_OPTION *option);

/******* SCK_GVID.C *******/
STATUS NU_Ioctl_SIOCGETVLAN(SCK_IOCTL_OPTION *option);

/****** IOCTL_GVID.C ******/
STATUS Ioctl_SIOCGETVLAN(SCK_IOCTL_OPTION *option);

/****** SCK_SVPRIO.C ********/
STATUS NU_Ioctl_SIOCSETVLANPRIO(SCK_IOCTL_OPTION *option);

/****** IOCTL_SVPRIO.C ********/
STATUS Ioctl_SIOCSETVLANPRIO(SCK_IOCTL_OPTION *option);

/****** SCK_GVPRIO.C ********/
STATUS NU_Ioctl_SIOCGETVLANPRIO(SCK_IOCTL_OPTION *option);

/****** IOCTL_GVPRIO.C ********/
STATUS Ioctl_SIOCGETVLANPRIO(SCK_IOCTL_OPTION *option);

/****** SCK_SIF.C ********/
STATUS NU_Ioctl_SIOCSIFADDR(const SCK_IOCTL_OPTION *option);

/****** IOCTL_SIF.C ******/
STATUS Ioctl_SIOCSIFADDR(const SCK_IOCTL_OPTION *option);

#if (INCLUDE_ARP == NU_TRUE)

/****** SCK_SARP.C *******/
STATUS NU_Ioctl_SIOCSARP(const SCK_IOCTL_OPTION *option);

/******* IOCTL_SARP.C *****/
STATUS Ioctl_SIOCSARP(const SCK_IOCTL_OPTION *option);

/******** SCK_GARP.C ******/
STATUS NU_Ioctl_SIOCGARP(SCK_IOCTL_OPTION *option);

/******** IOCTL_GARP.C *****/
STATUS Ioctl_SIOCGARP(SCK_IOCTL_OPTION *option);

/******** SCK_DARP.C *******/
STATUS NU_Ioctl_SIOCDARP(const SCK_IOCTL_OPTION *option);

/******** IOCTL_DARP.C ******/
STATUS Ioctl_SIOCDARP(const SCK_IOCTL_OPTION *option);

#endif

/******** SCK_REQ.C *******/
STATUS NU_Ioctl_SIOCIFREQ(SCK_IOCTL_OPTION *option);

/******* IOCTL_REQ.C *******/
STATUS Ioctl_SIOCIFREQ(SCK_IOCTL_OPTION *option);

/****** SCK_FREAD.C *****/
STATUS NU_Ioctl_FIONREAD(SCK_IOCTL_OPTION *option);

/***** IOCTL_FREAD.C ****/
STATUS Ioctl_FIONREAD(SCK_IOCTL_OPTION *option);

/***** SCK_GHWC.C ****/
STATUS NU_Ioctl_SIOCGHWCAP(SCK_IOCTL_OPTION *option);

/***** IOCTL_GHWC.C ****/
STATUS Ioctl_SIOCGHWCAP(SCK_IOCTL_OPTION *option);

/***** SCK_SHWC.C ****/
STATUS NU_Ioctl_SIOCSHWOPTS(const SCK_IOCTL_OPTION *option);

/***** IOCTL_SHWO.C ****/
STATUS Ioctl_SIOCSHWOPTS(const SCK_IOCTL_OPTION *option);

/***** DEV_RIFD.C ****/
STATUS NU_Remove_IP_From_Device(const CHAR *name, const UINT8 *ip_addr,
                                INT16 ip_family);

/***** SCK_GPMTU.C ****/
UINT32 NU_Get_PMTU(const UINT8 *src_addr, const UINT8 *dest_addr, INT16 family,
                   STATUS *status);

/***** DEV_RD.C ****/
STATUS NU_Remove_Device(const CHAR *name, UINT32 flags);

/***** IP_PML.C ****/
STATUS IP_Process_Multicast_Listen(INT, UINT8 *, UINT8 *, UINT16,
                                   const UINT8 *, UINT16);

/***** IP_MC_DM.C *****/
VOID IGMP_Leave(IP_MULTI *);

/***** IGMP_S.C *****/
STATUS  IGMP_Send(IP_MULTI *, UINT32, UINT8, UINT8);

/***** DHCP_GIAID.C *****/
STATUS NU_Get_DHCP_IAID(const UINT32, UINT32 *);

/***** DHCP_SIAID.C *****/
STATUS NU_Set_DHCP_IAID(const UINT32, UINT32);

/***** IOCTL_GSM.C *****/
STATUS Ioctl_SIOCGIFNETMASK(SCK_IOCTL_OPTION *);

/***** SCK_GSM.C *****/
STATUS NU_Ioctl_SIOCGIFNETMASK(SCK_IOCTL_OPTION *, INT );

/***** DNS_RES.C *****/
DNS_HOST  *DNS_Add_Host(const CHAR *, const CHAR *, UNSIGNED, INT16, INT16, INT16,
                        UINT8, VOID *);

/***** DNS_GNBN.C *****/
DNS_HOST  *DNS_Find_Matching_Host_By_Name(const CHAR *, CHAR *, INT16);

/***** SCK_DHE.C *****/
STATUS NU_Delete_Host_Entry(NU_HOSTENT *);

/***** DNS_MX.C *****/
STATUS NU_Get_Host_MX(CHAR *, DNS_MX_RR *, UINT16 *);

/***** SCK_SFPT.C *****/
STATUS NU_Setsockopt_TCP_FIRST_PROBE_TIMEOUT(INT, INT32);

/***** SCK_GFPT.C *****/
STATUS NU_Getsockopt_TCP_FIRST_PROBE_TIMEOUT(INT, INT32 *);

/***** SCK_SIL.C ****/
STATUS NU_Ioctl_SIOCICMPLIMIT(CHAR *, UINT8, UINT32);

/***** IOCTL_SIL.C ****/
STATUS Ioctl_SIOCICMPLIMIT(CHAR *, UINT8, UINT32);

/***** SCK_SPT.C *****/
STATUS NU_Setsockopt_TCP_PROBE_TIMEOUT(INT socketd, INT32 timeout);

/***** SCK_GPT.C *****/
STATUS NU_Getsockopt_TCP_PROBE_TIMEOUT(INT socketd, INT32 *timeout);

/***** SCK_SMP.C *****/
STATUS NU_Setsockopt_TCP_MAX_PROBES(INT socketd, UINT8 max_probes);

/***** SCK_GMP.C *****/
STATUS NU_Getsockopt_TCP_MAX_PROBES(INT socketd, UINT8 *max_probes);

/***** SCK_SMSL.C *****/
STATUS NU_Setsockopt_TCP_MSL(INT socketd, UINT32 msl);

/***** SCK_GMSL.C *****/
STATUS NU_Getsockopt_TCP_MSL(INT socketd, UINT32 *msl);

/***** SCK_SFRTO.C *****/
STATUS NU_Setsockopt_TCP_FIRST_RTO(INT socketd, INT32 timeout);

/***** SCK_GFRTO.C *****/
STATUS NU_Getsockopt_TCP_FIRST_RTO(INT socketd, INT32 *timeout);

/***** SCK_SMRTO.C *****/
STATUS NU_Setsockopt_TCP_MAX_RTO(INT socketd, UINT32 timeout);

/***** SCK_GMRTO.C *****/
STATUS NU_Getsockopt_TCP_MAX_RTO(INT socketd, UINT32 *timeout);

/***** SCK_SMR2.C *****/
STATUS NU_Setsockopt_TCP_MAX_R2(INT socketd, UINT8 max_retrans);

/***** SCK_GMR2.C *****/
STATUS NU_Getsockopt_TCP_MAX_R2(INT socketd, UINT8 *max_retrans);

/***** SCK_SDA.C *****/
STATUS NU_Setsockopt_TCP_DELAY_ACK(INT socketd, UINT32 delay);

/***** SCK_GDA.C *****/
STATUS NU_Getsockopt_TCP_DELAY_ACK(INT socketd, UINT32 *delay);

/***** SCK_SKAW.C *****/
STATUS NU_Setsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 delay);

/***** SCK_GKAW.C *****/
STATUS NU_Getsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 *delay);

/***** SCK_SKAR2.C *****/
STATUS NU_Setsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 max_retrans);

/***** SCK_GKAR2.C *****/
STATUS NU_Getsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 *max_retrans);

/***** SCK_SMSR2.C *****/
STATUS NU_Setsockopt_TCP_MAX_SYN_R2(INT socketd, UINT8 max_retrans);

/***** SCK_GMSR2.C *****/
STATUS NU_Getsockopt_TCP_MAX_SYN_R2(INT socketd, UINT8 *max_retrans);

/***** SCK_GCSACK.C *****/
STATUS NU_Getsockopt_TCP_CFG_SACK(INT socketd, UINT8 *optval);

/***** SCK_SCSACK.C *****/
STATUS NU_Setsockopt_TCP_CFG_SACK(INT socketd, UINT8 opt_val);

/***** SCK_GCC.C *****/
STATUS NU_Getsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 *optval);

/***** SCK_SCC.C *****/
STATUS NU_Setsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 opt_val);

/***** SCK_SCDSACK.C *****/
STATUS NU_Setsockopt_TCP_CFG_DSACK(INT socketd, UINT8 opt_val);

/***** SCK_GCDSACK.C *****/
STATUS NU_Getsockopt_TCP_CFG_DSACK(INT socketd, UINT8 *optval);

/***** SCK_SWS.C *****/
STATUS NU_Setsockopt_TCP_WINDOWSCALE(INT socketd, INT opt_val);

/***** SCK_GWS.C *****/
STATUS NU_Getsockopt_TCP_WINDOWSCALE(INT socketd, INT *optval);

/***** SCK_SSWS.C *****/
STATUS NU_Setsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 opt_val);

/***** SCK_GSWS.C *****/
STATUS NU_Getsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 *optval);

/***** SCK_GRWS.C *****/
STATUS NU_Getsockopt_TCP_SND_WINDOWSIZE(INT socketd, UINT32 *optval);

/***** SCK_STS.C *****/
STATUS NU_Setsockopt_TCP_TIMESTAMP(INT socketd, UINT8 opt_val);

/***** SCK_GTS.C *****/
STATUS NU_Getsockopt_TCP_TIMESTAMP(INT socketd, UINT8 *optval);

/***** SCK_IPML.C ****/
STATUS NU_IP_Multicast_Listen(INT socketd, UINT8 *interface_addr,
                              UINT8 *multi_addr, UINT16 filter_mode,
                              const UINT8 *source_list, UINT16 num_source_addr);

/***** netboot_initialize.c *****/
STATUS NU_Ethernet_Link_Up(CHAR *dev_name);
STATUS NU_Ethernet_Link_Down(CHAR *dev_name);
STATUS NU_Test_Link_Up(CHAR *dev_name, UINT32 *state);
STATUS NETBOOT_Wait_For_Network_Up(UNSIGNED suspend);

typedef struct dhcp_duid_struct
{
    UINT8   duid_id[DHCP_DUID_MAX_ID_NO_LEN];   /* DUID ID when using EN type DUID. */
    UINT8   duid_ll_addr[DADDLEN];              /* DUID link-layer address when using LL type DUID. */
    UINT16  duid_id_no_len;                     /* ID number length when using EN type DUID. */
    UINT16  duid_type;                          /* Type of DUID being used. */
    UINT16  duid_hw_type;                       /* DUID hardware type when using LL type DUID. */
    UINT32  duid_ent_no;                        /* DUID enterprise number using EN type DUID. */
} DHCP_DUID_STRUCT;

extern  DHCP_DUID_STRUCT    DHCP_Duid;

/***** DHCP_GDUID.C *****/
STATUS NU_Get_DHCP_DUID(DHCP_DUID_STRUCT *duid_ptr);

/***** DHCP_SDUID.C *****/
STATUS NU_Set_DHCP_DUID(const DHCP_DUID_STRUCT *duid_ptr);

/***** SCK.C *****/
VOID SCK_Clear_Socket_Error(INT socketd);

/***** SCK_SHN.C *****/
STATUS NU_Set_Device_Hostname(CHAR *name, UINT32 dev_index);

/***** SCK_UDR.C *****/
STATUS NU_Update_DNS_Record(UNSIGNED *handle, NU_DNS_HOST *dns_record, UINT8 action);

/***** SCK_SPI.C *****/
STATUS NU_Setsockopt_IP_PKTINFO(INT socketd, INT optval);

/***** SCK_GPI.C *****/
STATUS NU_Getsockopt_IP_PKTINFO(INT socketd, INT *optval, INT *optlen);

/***** SCK_DNSSD_B.C *****/
STATUS NU_DNS_SD_Browse(INT action, CHAR *type, CHAR *domain);

/***** SCK_DNSSD_R.C *****/
STATUS NU_DNS_SD_Register_Service(INT action, CHAR *name, CHAR *type,
                                  CHAR *domain, UINT16 port, CHAR *keys,
                                  INT32 if_index);

/***** DNS_BNP_KEY.C *****/
INT32 NU_DNS_Build_Key(CHAR *buffer, const char *kv_pair, INT32 length);

INT32 NU_DNS_Parse_Key(CHAR *buffer, INT32 offset, CHAR **out_key,
                       CHAR **out_value);

/***** SCK_DNSSD_RFRSH.C *****/
NU_DNS_SD_INSTANCE *NU_DNS_SD_Refresh(CHAR *type, CHAR *domain, STATUS *status);

/***** SCK_DNSSD_L.C *****/
NU_DNS_SD_SERVICE *NU_DNS_SD_Look_Up(CHAR *name, CHAR *type, CHAR *domain,
                                     STATUS *status);

/* DUID types. */
#define DHCP_DUID_LLT       1
#define DHCP_DUID_EN        2
#define DHCP_DUID_LL        3

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* EXTERNS_H */
