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
*       prot.h                                                   
*
*   DESCRIPTION
*
*       This file contains the macros, data structures and function
*       declarations used in the file PROT.C.
*
*   DATA STRUCTURES
*
*       prot_pkt_s
*       prot_obj_s
*       prot_ptr_s
*       prot_snmp_s
*       prot_ethernet_s
*       prot_ip_s
*       prot_arp_s
*       prot_udp_s
*       prot_tcp_s
*       prot_icmp_s
*       prot_dns_s
*       prot_frame_u
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef PROT_H
#define PROT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define PROT_TYPE           0
#define PROT_PKTUNKNOWN     0
#define PROT_PKTSNMP        1
#define PROT_PKTETHERNET    2

typedef union   prot_frame_u prot_frame_t;
typedef struct  prot_pkt_s  prot_pkt_t;

struct prot_pkt_s
{
    UINT8           *Ptr;
    prot_frame_t    *Frame;
    prot_pkt_t      *Child;
    UINT16          DataLen;
    UINT16          ChildProt;
};

typedef struct prot_obj_s
{
    UINT32          Level;
    INT32           Id[SNMP_SIZE_OBJECTID];
    UINT8           *ProtStr;
    UINT8           *FieldStr;
    snmp_syntax_t   Syntax;
    UINT16          Type;
    UINT16          SyntaxLen;

} prot_obj_t;

typedef struct prot_ptr_s
{
    CHAR            *Name;
    BOOLEAN         (*Header)(prot_pkt_t *Pkt);
    BOOLEAN         (*Field)(prot_pkt_t *Pkt, prot_obj_t *Obj);
    BOOLEAN         (*Print)(prot_obj_t *Obj, UINT8 **StrPtr);
    CHAR            **String;
    UINT16          StringLen;

    UINT8           snmp_pad[2];
} prot_ptr_t;

typedef struct prot_snmp_s
{
    UINT16          Type;             /*  1 : SNMP_GAUGE */
    UINT16          IfIndex;          /*  2 : SNMP_GAUGE */
    UINT32          ID;               /*  3 : ID */
    UINT16          Size;             /*  4 : SNMP_GAUGE */
    UINT16          Len;              /*  5 : SNMP_GAUGE */
    UINT32          Time;             /*  6 : SNMP_INTEGER */
    UINT8           *Data;            /*  7 : SNMP_NULL */
    UINT16          Status;           /*  8 : SNMP_INTEGER */

    UINT8           snmp_pad[2];
} prot_snmp_t;

typedef struct prot_ethernet_s
{
    UINT8           *DstAdr;          /*  1  : SNMP_OCTETSTR */
    UINT8           *SrcAdr;          /*  2  : SNMP_OCTETSTR */
    UINT8           *Type;            /*  3  : SNMP_GAUGE */
    UINT8           *Data;            /*  4  : SNMP_NULL */
} prot_ethernet_t;

typedef struct prot_ip_s
{
    UINT8           *Vers;            /*  1  : SNMP_GAUGE */
    UINT8           *HLen;            /*  2  : SNMP_GAUGE */
    UINT8           *Service;         /*  3  : SNMP_GAUGE */
    UINT8           *TLen;            /*  4  : SNMP_GAUGE */
    UINT8           *ID;              /*  5  : SNMP_GAUGE */
    UINT8           *Flags;           /*  6  : SNMP_GAUGE */
    UINT8           *Fragment;        /*  7  : SNMP_GAUGE */
    UINT8           *Time;            /*  8  : SNMP_TIMETICKS */
    UINT8           *Type;            /*  9  : SNMP_GAUGE */
    UINT8           *ChkSum;          /*  10 : SNMP_GAUGE */
    UINT8           *IPSrc;           /*  11 : SNMP_IPADDR */
    UINT8           *IPDst;           /*  12 : SNMP_IPADDR */
    UINT8           *IPOption;        /*  13 : SNMP_OCTETSTR */
    UINT8           *Data;            /*  14 : SNMP_NULL */
} prot_ip_t;

typedef struct prot_arp_s
{
    UINT8           *Hardw;           /*  1  : SNMP_GAUGE */
    UINT8           *Type;            /*  2  : SNMP_GAUGE */
    UINT8           *HLen;            /*  3  : SNMP_GAUGE */
    UINT8           *PLen;            /*  4  : SNMP_GAUGE */
    UINT8           *Operation;       /*  5  : SNMP_GAUGE */
    UINT8           *HwSender;        /*  6  : SNMP_OCTETSTR */
    UINT8           *ProtSender;      /*  7  : SNMP_OCTETSTR */
    UINT8           *HwTarget;        /*  8  : SNMP_OCTETSTR */
    UINT8           *ProtTarget;      /*  9  : SNMP_OCTETSTR */
    UINT8           *Data;            /*  10 : SNMP_NULL */
} prot_arp_t;

typedef struct prot_udp_s
{
    UINT8           *SrcPort;         /*  1  : SNMP_GAUGE */
    UINT8           *DstPort;         /*  2  : SNMP_GAUGE */
    UINT8           *MLen;            /*  3  : SNMP_GAUGE */
    UINT8           *ChkSum;          /*  4  : SNMP_GAUGE */
    UINT8           *Data;            /*  5  : SNMP_NULL */
} prot_udp_t;

typedef struct prot_tcp_s
{
    UINT8           *SrcPort;         /*  1  : SNMP_GAUGE */
    UINT8           *DstPort;         /*  2  : SNMP_GAUGE */
    UINT8           *SeqNr;           /*  3  : SNMP_GAUGE */
    UINT8           *AckNr;           /*  4  : SNMP_GAUGE */
    UINT8           *HLen;            /*  5  : SNMP_GAUGE */
    UINT8           *Reserved;        /*  6  : SNMP_GAUGE */
    UINT8           *Code;            /*  7  : SNMP_GAUGE */
    UINT8           *Window;          /*  8  : SNMP_GAUGE */
    UINT8           *ChkSum;          /*  9  : SNMP_GAUGE */
    UINT8           *UrgePtr;         /*  10 : SNMP_GAUGE */
    UINT8           *option;          /*  11 : SNMP_OCTETSTR */
    UINT8           *Data;            /*  12 : SNMP_NULL */
} prot_tcp_t;

typedef struct prot_icmp_s
{
    UINT8           *Type;            /*  1  : SNMP_GAUGE */
    UINT8           *Code;            /*  2  : SNMP_GAUGE */
    UINT8           *ChkSum;          /*  3  : SNMP_GAUGE */
    UINT8           *Data;            /*  4  : SNMP_NULL */
} prot_icmp_t;

typedef struct prot_dns_s
{
    UINT8           *ID;              /*  1  : SNMP_GAUGE */
    UINT8           *Parameter;       /*  2  : SNMP_GAUGE */
    UINT8           *NroQuest;        /*  3  : SNMP_GAUGE */
    UINT8           *NroAnsw;         /*  4  : SNMP_GAUGE */
    UINT8           *NroAuth;         /*  5  : SNMP_GAUGE */
    UINT8           *NroAdd;          /*  6  : SNMP_GAUGE */
    UINT8           *Entry;           /*  7,8,9,10  : variable */
    UINT8           *Data;            /* 11  : SNMP_NULL */
} prot_dns_t;

union prot_frame_u
{
    prot_snmp_t     snmp;
    prot_ethernet_t Ethernet;
    prot_ip_t       Ip;
    prot_tcp_t      Tcp;
    prot_udp_t      Udp;
    prot_icmp_t     Icmp;
    prot_arp_t      Arp;
    prot_dns_t      Dns;
};

VOID    ProtFree(prot_pkt_t *);
VOID    ProtExit(VOID);
BOOLEAN ProtGetField(prot_pkt_t *, prot_obj_t *);
BOOLEAN ProtsnmpPrint (prot_obj_t *, UINT8 **);

extern  CHAR *protocolString;
extern  CHAR *protsnmpString[];

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
