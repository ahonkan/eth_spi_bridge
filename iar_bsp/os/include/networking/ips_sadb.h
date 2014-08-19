/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_sadb.h
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       Definitions for Security Associations database.
*
* DATA STRUCTURES
*
*       IPSEC_SECURITY_PROTOCOL
*       IPSEC_OUTBOUND_INDEX
*       IPSEC_OUTBOUND_INDEX_REAL
*       IPSEC_SINGLE_IP_ADDR
*       IPSEC_IP_ADDR
*       IPSEC_SELECTOR
*       IPSEC_INBOUND_SA
*       IPSEC_INBOUND_SADB
*       IPSEC_OUTBOUND_SA
*       IPSEC_OUTBOUND_SADB
*       IPSEC_INBOUND_INDEX
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_SADB_H
#define IPS_SADB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

struct ipsec_policy_group;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
/* Define the SA request timeout equal to a Phase 2 timeout. */
#define IPSEC_SA_REQ_TIMEOUT            IKE_PHASE2_TIMEOUT
#endif

/* Defining the structure IPSEC_SECURITY_PROTOCOL. */
typedef struct ipsec_security_protocol
{
    /* When applying IPsec in tunnel mode these members indicate the
     * tunnel end-points. Note, these have been placed in the
     * beginning of the structure for matching purposes, so do not
     * modify them.
     */
    UINT8           ipsec_tunnel_destination[MAX_ADDRESS_SIZE];
    UINT8           ipsec_tunnel_source[MAX_ADDRESS_SIZE];

    /* Creation criteria for the Selector.*/
    UINT8           ipsec_sa_derivation;

    /* Padding the structure. */
    UINT8           ipsec_pad1[3];

    /* Mode in which the protocol is applied. */
    UINT16          ipsec_security_mode;

    /* Indicates the algorithm to be used by the authenticator. */
    UINT8           ipsec_auth_algo;

    /* Indicates the algorithm to be used by the Encryptor.*/
    UINT8           ipsec_encryption_algo;

    /* IPsec protocol that is applied. */
    UINT8           ipsec_protocol;

    /* IPsec flags. */
    UINT8           ipsec_flags;

    /* Defining the padding. */
    UINT8           ipsec_pad2[2];
}IPSEC_SECURITY_PROTOCOL;
/* End of IPSEC_SECURITY_PROTOCOL structure definition. */

/* Defining the structure IPSEC_OUTBOUND_INDEX. */
typedef struct ipsec_outbound_index
{
    /* Name of the group to which the SA is associated. */
    CHAR    *ipsec_group;

    /* Index used to uniquely identify the SA in this group. */
    UINT32  ipsec_index;
}IPSEC_OUTBOUND_INDEX;
/* End of IPSEC_OUTBOUND_INDEX structure definition. */

/* Defining the structure IPSEC_OUTBOUND_INDEX_REAL. */
typedef struct ipsec_outbound_index_real
{
    /* Name of the group to which the SA is associated. */
    CHAR            *ipsec_group;

    /* Security Parameter's Index. */
    UINT32          ipsec_spi;

    /* Destination address. */
    UINT8           *ipsec_dest;

    /* Type of the destination address. */
    UINT8           ipsec_dest_type;

    /* IPsec protocol that is applied. */
    UINT8           ipsec_protocol;

    /* Padding the structure. */
    UINT8           ipsec_pad[2];
}IPSEC_OUTBOUND_INDEX_REAL;
/* End of IPSEC_OUTBOUND_INDEX_REAL structure definition. */

/* Defining the structure IPSEC_SINGLE_IP_ADDR. */
typedef struct ipsec_single_ip_addr
{
    UINT8           *ipsec_addr;            /* Pointer to IP address. */
    UINT8           ipsec_type;             /* Family of IP address. */
    UINT8           ipsec_pad[3];           /* Pad the structure. */
}IPSEC_SINGLE_IP_ADDR;
/* End of IPSEC_SINGLE_IP_ADDR structure definition. */

/* Defining the structure IPSEC_IP_ADDR. */
typedef struct ipsec_ip_addr
{
    UINT8   ipsec_addr[MAX_ADDRESS_SIZE];   /* Address 1. */

    /* Union of Address 2 and prefix length. */
    union
    {
        /* Contains the end of an IP range or an IPv4 subnet mask. */
        UINT8       ipsec_addr2[MAX_ADDRESS_SIZE];

        /* Contains the prefix length, for an IPv6 subnet. */
        UINT8       ipsec_prefix_len;
    } ipsec_ext_addr;
}IPSEC_IP_ADDR;
/* End of IPSEC_IP_ADDR structure definition. */

/* Defining the structure IPSEC_SELECTOR. */
typedef struct ipsec_selector
{
    union
    {
        UINT16      ipsec_src_port;          /* Source port. */
        UINT8       ipsec_icmp_id[2];        /* ICMP message type at index 0
                                              * and code at index 1.
                                              */
        UINT8       ipsec_mobility_hdr;      /* Mobile IP header. */

        /* Following is an internally used member of this structure.
         * This must not be used directly in user applications.
         */
        UINT16      ipsec_intern_src_port_end[2];
    }ipsec_src_tid;

    union
    {
        UINT16      ipsec_dst_port;          /* Destination port. */

        UINT8       ipsec_icmp_id[2];        /* ICMP message type at index 0
                                              * and code at index 1.
                                              */

        UINT8       ipsec_mobility_hdr;      /* Mobile IP header. */

        /* Following is an internally used member of this structure.
         * This must not be used directly in user applications.
         */
        UINT16      ipsec_intern_dst_port_end[2];
    }ipsec_dst_tid;

    IPSEC_IP_ADDR   ipsec_source_ip;        /* Source IP address. */
    IPSEC_IP_ADDR   ipsec_dest_ip;          /* Destination IP address. */

    UINT8           ipsec_transport_protocol;/* Upper layer protocol. */

    UINT8           ipsec_source_type;      /* Type for the source. */
    UINT8           ipsec_dest_type;        /* Type for the destination.*/
    UINT8           ipsec_pad[1];           /* Padding the structure. */
}IPSEC_SELECTOR;
/* End of IPSEC_SELECTOR structure definition. */

/* Defining the structure IPSEC_INBOUND_SA . */
typedef struct ipsec_inbound_sa
{
    /* Front link. */
    struct ipsec_inbound_sa *ipsec_flink;

    /* Protocol and the mode and encryptor algorithms that will be used.*/
    IPSEC_SECURITY_PROTOCOL ipsec_security;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
    /* Pointer to the soft lifetime for this SA. The instance of this
       lifetime will be present in the corresponding outgoing SA. */
    IPSEC_SA_LIFETIME       ipsec_soft_lifetime;

    /* Pointer to the hard lifetime for this SA. The instance of this
       lifetime will be present in the corresponding outgoing SA. */
    IPSEC_SA_LIFETIME       ipsec_hard_lifetime;
#endif

    /* Pointer to encryption request structure for this SA to decrypt
       any packet based on this SA. */
    IPSEC_ENCRYPT_REQ       *ipsec_encrypt_req;

    /* Protection against replay attacks. */
    IPSEC_ANTIREPLAY_WIN    ipsec_antireplay;

    /* SPI that is assigned to this SA. */
    UINT32                  ipsec_spi;

    /* The SA selector. */
    IPSEC_SELECTOR          ipsec_select;

    /* Authentication algorithm's key. */
    UINT8                   *ipsec_auth_key;

    /* Encryption algorithm's key. */
    UINT8                   *ipsec_encryption_key;
}IPSEC_INBOUND_SA;
/* End of IPSEC_INBOUND_SA  structure definition. */

/* Defining the structure IPSEC_INBOUND_SADB . */
typedef struct ipsec_inbound_sadb
{
    IPSEC_INBOUND_SA    *ipsec_head;        /* Front link. */
    IPSEC_INBOUND_SA    *ipsec_tail;        /* Back link. */

    /* Next value will be index for the ESP SA. */
    UINT32              ipsec_next_sa_index_esp;

    /* Next value will be index for the AH SA. */
    UINT32              ipsec_next_sa_index_ah;
}IPSEC_INBOUND_SADB;
/* End of IPSEC_INBOUND_SADB structure definition. */

/* Defining the structure IPSEC_OUTBOUND_SA. */
typedef struct ipsec_outbound_sa
{
    /* Forward link. */
    struct ipsec_outbound_sa  *ipsec_flink;

    /* The selector for this bundle. */
    IPSEC_SELECTOR          ipsec_select;

    /* How IPsec should be applied to packets being processed by
       this SA.*/
    IPSEC_SECURITY_PROTOCOL ipsec_security;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
    /* Pointer to the soft lifetime for this SA. The instance of this
       lifetime will be present in the corresponding incoming SA. */
    IPSEC_SA_LIFETIME       *ipsec_soft_lifetime;

    /* Pointer to the hard lifetime for this SA. The instance of this
       lifetime will be present in the corresponding incoming SA. */
    IPSEC_SA_LIFETIME       *ipsec_hard_lifetime;
#endif

    /* Pointer to encryption request structure for this SA. */
    IPSEC_ENCRYPT_REQ         *ipsec_encrypt_req;

    /* Unique local identifier for this SA. */
    UINT32                  ipsec_index;

    /* SPI that is assigned to this SA. */
    UINT32                  ipsec_remote_spi;

    /* The next sequence number. */
    IPSEC_SEQ_NUM           ipsec_seq_num;

    /* Authentication Algorithm's key. */
    UINT8                   *ipsec_auth_key;

    /* Encryption algorithm key.*/
    UINT8                   *ipsec_encryption_key;
}IPSEC_OUTBOUND_SA;
/* End of IPSEC_OUTBOUND_SA structure definition. */

/* Defining the structure IPSEC_OUTBOUND_SADB. */
typedef struct ipsec_outbound_sadb
{
    IPSEC_OUTBOUND_SA   *ipsec_head;        /* Head. */
    IPSEC_OUTBOUND_SA   *ipsec_tail;        /* Tail. */
    UINT32              ipsec_next_sa_index;/* Next value will be index
                                               for the SA. */
}IPSEC_OUTBOUND_SADB;
/* End of IPSEC_OUTBOUND_SADB structure definition. */


/* Defining the structure IPSEC_INBOUND_INDEX. */
typedef struct ipsec_inbound_index
{
    /* Security Parameter's Index. */
    UINT32              ipsec_spi;

    /* Originating destination address. */
    UINT8               ipsec_dest[MAX_ADDRESS_SIZE];

    /* Destination address type. */
    UINT8               ipsec_dest_type;

    /* IPsec protocol that is applied. */
    UINT8               ipsec_protocol;

    /* Padding the structure. */
    UINT8               ipsec_pad[2];
}IPSEC_INBOUND_INDEX;
/* End of IPSEC_INBOUND_INDEX structure definition. */

/* Defining the IPSEC_Match_Sec_Prot with memcmp function, matching the
 * whole function except first three members i.e. the tunnel end-points
 * and SA derivatives.
 */
#define IPSEC_MATCH_SEC_PROT(pkt_security, sa_security)                \
             ((memcmp((UINT8 *)pkt_security +                          \
                      ((MAX_ADDRESS_SIZE * 2) + (sizeof(UINT8) * 4)),  \
(UINT8 *)sa_security + ((MAX_ADDRESS_SIZE * 2) + (sizeof(UINT8) * 4)), \
                            (sizeof(IPSEC_SECURITY_PROTOCOL) -         \
                      ((MAX_ADDRESS_SIZE * 2) + (sizeof(UINT8) * 6)))) \
                                            == 0) ? NU_TRUE : NU_FALSE)

/**** Macros used for sorting. ****/
/* The address and security protocol index macros defined below are used
   for sorting the inbound SA list, in IPSEC_Cmp_Inbound_SAs(). */
#define IPSEC_ADDR_INDEX        {IPSEC_SINGLE_IP,\
                                 IPSEC_RANGE_IP,\
                                 IPSEC_SUBNET_IP,\
                                 IPSEC_WILDCARD}

#define IPSEC_SEC_PROT_INDEX    {IPSEC_AH,\
                                 IPSEC_ESP,\
                                 IPSEC_WILDCARD}

/* Selector macros for ports, ICMP message types and Mobility header. */
#define ipsec_source_port          ipsec_src_tid.ipsec_src_port
#define ipsec_destination_port     ipsec_dst_tid.ipsec_dst_port
#define ipsec_icmp_msg             ipsec_src_tid.ipsec_icmp_id[1]
#define ipsec_icmp_code            ipsec_src_tid.ipsec_icmp_id[0]
#define ipsec_icmp_msg_high        ipsec_dst_tid.ipsec_icmp_id[1]
#define ipsec_icmp_code_high       ipsec_dst_tid.ipsec_icmp_id[0]
#define ipsec_mobility_header      ipsec_src_tid.ipsec_mobility_hdr

/***** Prototypes related to Security association. *****/
INT    IPSEC_Cmp_Index(UINT8 a_type, UINT8 b_type, UINT8 *index,
                       UINT8 index_length);
INT    IPSEC_Cmp_Inbound_SAs(VOID *a, VOID *b);
STATUS IPSEC_Add_Outbound_SA(CHAR *group_name, IPSEC_OUTBOUND_SA *sa_entry,
                             UINT32 *return_index);
STATUS IPSEC_Add_Outbound_ESN_SA(CHAR *group_name, IPSEC_OUTBOUND_SA *sa_entry,
                             UINT32 *return_index);
STATUS IPSEC_Add_Outbound_SA_Real(CHAR *group_name,
                                  IPSEC_OUTBOUND_SA *sa_entry,
                                  IPSEC_OUTBOUND_SA **return_sa_ptr);
STATUS IPSEC_Get_Outbound_SA(struct ipsec_policy_group *group_ptr,
                            IPSEC_SELECTOR *selector,
                            IPSEC_SECURITY_PROTOCOL *security,
                            IPSEC_OUTBOUND_SA **sa_ptr);
STATUS IPSEC_Get_Outbound_SA_By_Index(UINT32 sa_index,
                                      IPSEC_OUTBOUND_SA *sa_start,
                                      IPSEC_OUTBOUND_SA **sa_ptr);
STATUS IPSEC_Get_Outbound_SA_Opt(IPSEC_OUTBOUND_INDEX *index, INT optname,
                                 VOID *optval, INT *optlen);
STATUS IPSEC_Free_Outbound_SA(IPSEC_OUTBOUND_SA *sa_ptr);
STATUS IPSEC_Remove_Outbound_SA(IPSEC_OUTBOUND_INDEX *index);
STATUS IPSEC_Add_Inbound_SA(CHAR *group_name, IPSEC_INBOUND_SA *sa_entry,
                            UINT32 *return_spi);
STATUS IPSEC_Add_Inbound_ESN_SA(CHAR *group_name, IPSEC_INBOUND_SA *sa_entry,
                            UINT32 *return_spi);
STATUS IPSEC_Add_Inbound_SA_Real(CHAR *group_name,
                                 IPSEC_INBOUND_SA *sa_entry,
                                 IPSEC_INBOUND_SA **return_sa_ptr);
STATUS IPSEC_Get_Inbound_SA(struct ipsec_policy_group *group_ptr,
                            IPSEC_INBOUND_INDEX *index,
                            IPSEC_INBOUND_SA **sa_ptr);
STATUS IPSEC_Get_Inbound_SA_Entry(struct ipsec_policy_group *group_ptr,
                                  IPSEC_INBOUND_SA **sa_ptr, UINT32 index,
                                  UINT8 option);
STATUS IPSEC_Get_Inbound_SA_Opt(CHAR *group_name,
                                IPSEC_INBOUND_INDEX *index, INT optname,
                                VOID *optval, INT *optlen);
STATUS IPSEC_Free_Inbound_SA(IPSEC_INBOUND_SA *sa_ptr);
STATUS IPSEC_Remove_Inbound_SA(CHAR *group_name,
                               IPSEC_INBOUND_INDEX *index);
STATUS IPSEC_Remove_In_SA_By_SPI(CHAR *group_name, UINT32 sa_spi);
STATUS IPSEC_Remove_In_SA_By_Selector(CHAR *group_name,
                                      IPSEC_SELECTOR *selector);
STATUS IPSEC_Add_SA_Pair(UINT32 if_index, IPSEC_OUTBOUND_SA *out_sa,
                         IPSEC_INBOUND_SA *in_sa);
STATUS IPSEC_Remove_SA_Pair(IPSEC_OUTBOUND_INDEX_REAL *index);
STATUS IPSEC_Remove_SAs_By_Addr(CHAR *group_name,
                                IPSEC_SINGLE_IP_ADDR *local_addr,
                                IPSEC_SINGLE_IP_ADDR *remote_addr);
STATUS IPSEC_Check_Initial_Contact(CHAR *group_name,
                                   IPSEC_SINGLE_IP_ADDR *remote_addr);
STATUS IPSEC_Rehash_Outbound_SAs(CHAR *group_name,
                                 IPSEC_SELECTOR *selector,
                                 IPSEC_SECURITY_PROTOCOL *security);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_SADB_H */
