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
*       ips_esn.h
*
* COMPONENT
*
*       Extended Sequence Number (ESN)
*
* DESCRIPTION
*
*       This modules specifies an Extended Sequence Number (ESN)
*       implementation which allows 64-bit sequence numbers in IPsec. This
*       structure is also used for 32-bit sequence numbers.
*
* DATA STRUCTURES
*
*       IPSEC_SEQ_NUM
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_ESN_H
#define IPS_ESN_H

#ifdef          __cplusplus
extern  "C" {                             /* C declarations in C++ */
#endif /* _cplusplus */

/* Initializes the passed sequence number structure. */
#define IPSEC_SEQ_INIT(seq, esn)       \
    seq.ipsec_is_esn = esn;            \
    seq.ipsec_low_order = 0;           \
    seq.ipsec_high_order = 0

/* Increments the passed sequence number. If the lower bytes overflow
 * (becomes zero), the high order is incremented if this is an ESN
 * number.
 */
#define IPSEC_SEQ_INC(seq)             \
    seq.ipsec_low_order++;             \
    seq.ipsec_high_order +=            \
    ( ( seq.ipsec_is_esn ) && ( !seq.ipsec_low_order ) ) ? 1 : 0

/* Finds the difference between two sequence numbers. It is given that
 * seq1 is always greater than seq2. Their difference is never more than
 * 0xFFFFFFFF and so can be depicted in an unsigned 32-bit integer.
 * There are two cases:
 *
 * CASE 1: seq1.ipsec_high_order > seq2.ipsec_high_order
 *
 * This means that seq1.ipsec_low_order has cycled and therefore the difference
 * between the two sequence numbers is the sum of the numbers it will take
 * seq2 to cycle (0xFFFFFFFF - seq2.ipsec_low_order) and seq1.ipsec_low_order
 * (i.e. how far seq1 has gone after cycling). Please note that the difference
 * between the high orders will never be more than one, otherwise the difference
 * between the two sequence numbers will become more than 0xFFFFFFFF.
 *
 * CASE 2: seq1.ipsec_high_order == seq2.ipsec_high_order
 *
 * seq1 can never be less than seq2, so the high order has to be equal if
 * CASE 1 is false. This case now becomes very simple as we just have to
 * find the difference between seq1.ipsec_low_order and seq2.ipsec_low_order.
 *
 */
#define IPSEC_SEQ_DIFF(seq1, seq2)                                     \
    ( seq1.ipsec_high_order > seq2.ipsec_high_order ) ?                 \
    ( seq1.ipsec_low_order + (0xFFFFFFFFUL - seq2.ipsec_low_order) ) : \
    ( seq1.ipsec_low_order - seq2.ipsec_low_order)

/* Subtracts a constant from the passed sequence number. The constant will
 * never be such that the sequence number becomes negative. If the lower
 * order is greater than or equal to the passed constant, then we do not
 * have to worry about the higher order 32 bits. However, if the lower order
 * is less than the constant, we then 'borrow' from higher order and
 * determine the lower order.
 *
 * Note that the else statement should only be executed for the 64-bit
 * sequence numbers. Since we are assured that the sequence number will not
 * become negative, we do not check for this condition.
 */
#define IPSEC_SEQ_SUB_CONST(seq, num)                                    \
    if ( seq.ipsec_low_order >= num )                                    \
        seq.ipsec_low_order -= num;                                      \
    else                                                                 \
    {                                                                    \
        seq.ipsec_high_order--;                                          \
        seq.ipsec_low_order = 0xFFFFFFFFUL - (num - seq.ipsec_low_order);\
    }

/* Determines if the sequence number has cycled. Note that we just check for
 * 0's in both the fields (which is the condition after initialization as
 * well. This check will only be accurate once the sequence number has been
 * incremented after initialization.
 */
#define IPSEC_SEQ_HAS_CYCLED(seq)      \
    ( ( !seq.ipsec_low_order ) && ( !seq.ipsec_high_order ))

/* Determines if the two sequence numbers are equal. */
#define IPSEC_SEQ_IS_EQUAL(seq1, seq2)                               \
    ( ( seq1.ipsec_high_order == seq2.ipsec_high_order ) &&          \
      ( seq1.ipsec_low_order == seq2.ipsec_low_order ) )

/* The following structure is used to specify AH and ESP parameters during
 * IPsec processing for IPv4 or IPv6 packets.
 */
typedef struct ipsec_seq_num
{
    UINT32              ipsec_low_order;    /* Low order 32-bits of ESN. */
    UINT32              ipsec_high_order;   /* High order 32-bits of ESN. */
    UINT8               ipsec_is_esn ;      /* Flag indicating whether this
                                               number is ESN. */
    UINT8               ipsec_pad[3];       /* Pad to align structure on word
                                             * boundary. Variable not used.
                                             */
}IPSEC_SEQ_NUM;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IPS_ESN_H */

