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
/*
UINT16 TLS_TCP_Check(UINT16 *pseudoheader, NET_BUFFER *buf_ptr)
{
*/

/*
   On entry,
   pseudoheader  = r3
   buf_ptr       = r4
   */

#pragma asm
    .extern     TLS_Intswap

    .sect   .text
    .lflags r
    .align      2

    .globl      TLS_TCP_Check
TLS_TCP_Check:
    stwu    r1,-8(r1)
    mfspr   r0,lr
    addi    r5,r3,0         # Move the address in r3 into r5
    lwz     r3,0(r3)        # Load the first word of the pseudoheader into r3
    lwz     r11,4(r5)       # Load the 2nd word of pseudoheader into r11
    cmpi    0,0,r4,0
    lwz     r10,8(r5)       # Load the 3rd word of pseudoheader into r10
    addc    r3,r3,r11       # Add the 1st and 2nd words of pseudoheader
    addze   r3,r3           # Add the carry
    stw     r0,12(r1)
    addc    r3,r3,r10       # Add the 3rd word of pseudoheader to the checksum
    addze   r3,r3           # Add the carry
    addi    r5,r4,0
    bc      12,2,.L11       # eq
.L3:
    lwz     r6,DATA_LEN_OFFSET(r5)
    lwz     r7,DATA_PTR_OFFSET(r5)
    rlwinm  r4,r6,0,30,31
    rlwinm. r6,r6,30,2,31
    addi    r9,r0,0         #
    bc      12,2,.L4        # eq
    rlwinm. r11,r6,0,31,31
    addi    r8,r7,-4
    addi    r9,r6,0
    bc      12,2,.L13       # eq
    lwzu    r12,4(r8)
    addic.  r9,r6,-1
    addc    r3,r3,r12
    addze   r3,r3
    bc      12,2,.L14       # eq
.L13:
    rlwinm  r9,r9,31,1,31
.L6:
    lwzu    r12,4(r8)
    addic.  r9,r9,-1
    lwzu    r11,4(r8)
    addc    r3,r3,r12
    addze   r3,r3
    addc    r3,r3,r11
    addze   r3,r3
    bc      4,2,.L6         # ne
.L14:
    subf    r9,r9,r6
.L4:
    cmpi    0,0,r4,0
    bc      12,2,.L9        # eq
    rlwinm. r11,r4,0,30,30
    rlwinm  r12,r9,2,0,29
    add     r7,r7,r12
    bc      12,2,.L7        # eq
    addi    r4,r4,-2
    lhz     r12,0(r7)
    addi    r7,r7,2
    addc    r3,r3,r12
    addze   r3,r3
.L7:
    cmpi    0,0,r4,0
    bc      12,2,.L9        # eq
    lhz     r12,0(r7)
    rlwinm  r12,r12,0,16,23
    rlwinm  r12,r12,0,16,31
    addc    r3,r3,r12
    addze   r3,r3
.L9:
    lwz     r5,NEXT_BUF_OFFSET(r5)
    cmpi    0,0,r5,0
    bc      4,2,.L3         # ne
.L11:
    rlwinm. r12,r3,16,16,31
    bc      12,2,.L10       # eq
.L12:
    rlwinm  r12,r3,16,16,31
    rlwinm  r11,r3,0,16,31
    add     r3,r12,r11
    rlwinm. r10,r3,16,16,31
    bc      4,2,.L12        # ne
.L10:
    nor     r12,r3,r3
    rlwinm  r3,r12,0,16,31
    bl      TLS_Intswap
    lwz     r0,12(r1)
    mtspr   lr,r0
    addi    r1,r1,8
    blr


    .lflags nr

#pragma endasm

#endif /* TCPCHECK_ASM */
