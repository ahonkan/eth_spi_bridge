/*************************************************************************
*
*            Copyright Mentor Graphics Corporation 1993-2008
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        VERSION
*
*       chks.c                                        5.3
*
*   DESCRIPTION
*
*       This file contains an optimized version of the TCP checksum
*       function.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TLS_TCP_Check
*
*   DEPENDENCIES
*
*       net_cfg.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs.h"

#if (TCPCHECK_ASM)


#if (INCLUDE_IPSEC == NU_TRUE)
#define DATA_LEN_OFFSET (552+4)
#define DATA_PTR_OFFSET (548+4)
#define NEXT_BUF_OFFSET (544+4)
#else
#define DATA_LEN_OFFSET (552)
#define DATA_PTR_OFFSET (548)
#define NEXT_BUF_OFFSET (544)
#endif /* (INCLUDE_IPSEC == NU_FALSE) */


#ifdef __BIG_ENDIAN
#define LAST_BYTE_MASK  0xff00
#else
#define LAST_BYTE_MASK  0x00ff
#endif


/*************************************************************************
*
*   FUNCTION
*
*       TLS_TCP_Check
*
*   DESCRIPTION
*
*       This function checksums a TCP header.
*
*   INPUTS
*
*       *pseudoheader           Pointer to header to be checksummed
*       *buf_ptr                Pointer to the net buffer list
*
*   OUTPUTS
*
*       UINT16                  Checksum value of structure
*
*************************************************************************/

/* UINT16 TLS_TCP_Check(UINT16 *pseudoheader, NET_BUFFER *buf_ptr) */

__asm UINT16 TLS_TCP_Check(UINT16 *pseudoheader, NET_BUFFER *buf_ptr)
{
        MOV      a3,a1
        STMDB    sp!,{v1,lr}
        ADD      a3,a3,#4
        LDR      a4,[a3],#4
        LDR      a1,[a1,#0]
        LDR      a3,[a3,#0]
        ADDS     a1,a4,a1
        ADDCSS   a1,a1,#1
        ADDS     a1,a3,a1
        ADDCSS   a1,a1,#1
        MOV      a4,a2
        B        |L1.152|
|L1.40|
        LDR      a3,[a4,#DATA_PTR_OFFSET]
        LDR      a2,[a4,#DATA_LEN_OFFSET]
        TST      a3,#3
        LDRNEH   ip,[a3],#2
        SUBNE    a2,a2,#2
        MOV      lr,a2,LSR #2
        ADDNE    a1,ip,a1
        AND      ip,a2,#3
        MOV      a2,#0
        B        |L1.92|
|L1.80|
        LDR      v1,[a3,a2,LSL #2]
        ADD      a2,a2,#1
        ADDS     a1,v1,a1
        ADDCSS   a1,a1,#1
|L1.92|
        CMP      a2,lr
        BCC      |L1.80|
        CMP      ip,#0
        BEQ      |L1.148|
        ADD      a2,a3,a2,LSL #2
        TST      ip,#2
        BEQ      |L1.136|
        LDRH     a3,[a2],#2
        ADDS     a1,a3,a1
        ADDCSS   a1,a1,#1
        SUBS     a3,ip,#2
        BEQ      |L1.148|
|L1.136|
        LDRH     a2,[a2,#0]
        AND      a2,a2,#LAST_BYTE_MASK
        ADDS     a1,a2,a1
        ADDCSS   a1,a1,#1
|L1.148|
        LDR      a4,[a4,#NEXT_BUF_OFFSET]
|L1.152|
        CMP      a4,#0
        BNE      |L1.40|
        B        |L1.176|
|L1.164|
        MOV      a2,a1,LSL #16
        MOV      a2,a2,LSR #16
        ADD      a1,a2,a1,LSR #16
|L1.176|
        MOVS     a2,a1,ASR #16
        BNE      |L1.164|
        MVN      a1,a1
        MOV      a1,a1,LSL #16
        MOV      a1,a1,LSR #16
        LDMIA    sp!,{v1,lr}
        B        TLS_Intswap

        IMPORT TLS_Intswap
        IMPORT ||Lib$$Request$$armlib||, WEAK
}

#endif /* TCPCHECK_ASM */
