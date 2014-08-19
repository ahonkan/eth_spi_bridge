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
*       ips_ar.h
*
* COMPONENT
*
*       ANTI-REPLAY
*
* DESCRIPTION
*
*       Definitions required to support Anti-Replay services.
*
* DATA STRUCTURES
*
*       IPSEC_ANTIREPLAY_WIN
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_AR_H
#define IPS_AR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

/* Determines if a window is set. If it is set, this macro will be 1,
 * otherwise, it will be 0. ipsec_window is an array of 32 bit integers,
 * each bit depicting a sequence number. bit_num = 0 means least significant
 * bit of ipsec_window[0] and bit_num = 1 means least significant bit of
 * ipsec_window[1].
 *
 * We get the index of the 32-bit number in which our bit lies using bit_num/32,
 * The position of the actual bit comes from the first 0x1F bits, so we shift
 * by that much and check the least significant bit.
 */
#define IPSEC_ARWIN_ISSET(ipsec_window, bit_num)                \
    ( ( ipsec_window[(bit_num / 32)] >> (bit_num & 0x1F) ) & 0x01 )

/* Sets a bit in the window. ipsec_window is an array of 32 bit integers,
 * each bit depicting a sequence number. bit_num = 0 means least significant
 * bit of ipsec_window[0] and bit_num = 1 means least significant bit of
 * ipsec_window[1].
 *
 * We get the index of the 32-bit number in which our bit lies using bit_num/32,
 * The position of the actual bit comes from the first 0x1F bits, so we shift
 * by that much and set the required bit.
 */
#define IPSEC_ARWIN_SET_BIT(ipsec_window, bit_num)                \
    ipsec_window[(bit_num / 32)] |= 0x00000001UL << (bit_num & 0x1F)

/* Clears the window. */
#define IPSEC_ARWIN_CLEAR(ipsec_window)                           \
    UTL_Zero( ipsec_window,                                       \
              sizeof(UINT32) * IPSEC_SLIDING_WINDOW_SIZE )

/* Anti replay sliding window range. */
#define IPSEC_SLIDING_WINDOW_RANGE (IPSEC_SLIDING_WINDOW_SIZE * \
                                   (sizeof(UINT32) << 3))

/* Defining the structure IPSEC_ANTIREPLAY_WIN. */
typedef struct ipsec_antireplay_win
{
    /* The greatest sequence number that has been received by this
     * window.
     */
    IPSEC_SEQ_NUM    ipsec_last_seq;

    /* The window available. This window indicates the packets that
       have already been received. */
    UINT32           ipsec_window[IPSEC_SLIDING_WINDOW_SIZE];

}IPSEC_ANTIREPLAY_WIN;

/*** Function Prototypes. ***/
STATUS IPSEC_Replay_Check(IPSEC_ANTIREPLAY_WIN *ar_win, IPSEC_SEQ_NUM *seq_no);
STATUS IPSEC_Replay_Update(IPSEC_ANTIREPLAY_WIN *ar_win, IPSEC_SEQ_NUM  *seq_no);
VOID   IPSEC_Arwin_Shift_Left(UINT32 *ipsec_window, UINT32 num);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_AR_H */
