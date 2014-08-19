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
*       ips_ar.c
*
* COMPONENT
*
*       ANTI-REPLAY
*
* DESCRIPTION
*
*       This file contains the implementation of Anti-Reply services.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Replay_Check
*       IPSEC_Replay_Update
*       IPSEC_Arwin_Shift_Left
*
* DEPENDENCIES
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"

#if (IPSEC_ANTI_REPLAY == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Replay_Check
*
* DESCRIPTION
*
*       This function takes as input an Anti-Replay window structure
*       and a sequence number of a packet just received and checks
*       whether the packet is not a replay.
*
* INPUTS
*
*       *ar_win                 Pointer to the AntiReplay window.
*       seq_no                  Sequence no. which has been retrieved
*                               from the received packet.
*
* OUTPUTS
*
*       NU_SUCCESS              If the packet is not a replay
*       IPSEC_PKT_DISCARD       Not allowed or packet is a replay
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Replay_Check(IPSEC_ANTIREPLAY_WIN *ar_win, IPSEC_SEQ_NUM *seq_no)
{
    IPSEC_SEQ_NUM       win_lower_bound;
    STATUS              status = IPSEC_PKT_DISCARD;
    INT                 diff;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameter. */
    if(ar_win == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Compute the lower bound of the window. */
    win_lower_bound.ipsec_low_order =
        ar_win->ipsec_last_seq.ipsec_low_order;

    win_lower_bound.ipsec_high_order =
        ar_win->ipsec_last_seq.ipsec_high_order;

    win_lower_bound.ipsec_is_esn =
        ar_win->ipsec_last_seq.ipsec_is_esn;

    IPSEC_SEQ_SUB_CONST(win_lower_bound, (IPSEC_SLIDING_WINDOW_RANGE - 1) );

    /* Make sure sequence window does not span two sequence number spaces.
     * That is the sequence number window starts with one value for the
     * higher order bits but this rolls over to the next value. This case
     * will always be executed for non ESN sequence numbers.
     */
    if( ar_win->ipsec_last_seq.ipsec_low_order >=
        IPSEC_SLIDING_WINDOW_RANGE - 1 )
    {
        /* If passed sequence no. is greater then current sequence no.
         * of AR window.
         */
        if( seq_no->ipsec_low_order > ar_win->ipsec_last_seq.ipsec_low_order )
        {
            /* Set the status value to success. Larger sequence no. is
             * fine.
             */
            status = NU_SUCCESS;

            /* Set the higher order bits in the packet sequence number.
             * This is used for digest calculation.
             */
            seq_no->ipsec_high_order = ar_win->ipsec_last_seq.ipsec_high_order;
        }

        /* If the received sequence number is between the lower bound of
         * the anti-replay window and the highest received sequence number
         * then check the corresponding bit in the window to see if the
         * packet's sequence number has already been seen.
         */
        else if ( ( seq_no->ipsec_low_order >=
                    win_lower_bound.ipsec_low_order ) &&
                  ( seq_no->ipsec_low_order <=
                    ar_win->ipsec_last_seq.ipsec_low_order ) )
        {

            /* Get the difference between the two. */
            diff = ar_win->ipsec_last_seq.ipsec_low_order -
                   seq_no->ipsec_low_order;

            /* Check if some packet with particular sequence no. has
             * been already received. If note, then accept this
             * packet.
             */
            if(!( IPSEC_ARWIN_ISSET( ar_win->ipsec_window, diff) ))
            {
                /* The packet is within acceptable range. */
                status = NU_SUCCESS;

                /* Set the higher order bits in the packet sequence number.
                 * This is used for digest calculation.
                 */
                seq_no->ipsec_high_order =
                             ar_win->ipsec_last_seq.ipsec_high_order;
            }
        }

        /* If ESN is enabled, and the lower order bits of the sequence
         * number in the packet is less than the anti-replay low bound,
         * then that means that the higher order bits may have incremented
         * by one.
         */
        else if ( ( ar_win->ipsec_last_seq.ipsec_is_esn ) &&
                  ( seq_no->ipsec_low_order < win_lower_bound.ipsec_low_order ) )
        {
            status = NU_SUCCESS;

            seq_no->ipsec_high_order =
               ar_win->ipsec_last_seq.ipsec_high_order + 1;
        }

    }

    /* This case is for ESN only. It happens when the anti-replay window falls
     * on two sequence subspaces (that is the higher order bits change value).
     * Since the window is pegged on the highest packet received, we need to
     * check whether the received packet lies in that part of the window where
     * its higher order bits are one less than the highest packet received.
     */
    else if ( ar_win->ipsec_last_seq.ipsec_is_esn )
    {
        /* If the received packet has lower bits greater than the
         * lower bound in the window, this means that the received
         * packet lies in the lower sequence subspace.
         */
        if ( seq_no->ipsec_low_order >= win_lower_bound.ipsec_low_order )
        {
            /* Determine the bit position for this sequence number. All our
             * positions are with respect to the highest sequence number
             * received. Therefore, the difference here is just the sum of
             * the lower bits received and the lower bits of the highest
             * sequence number.
             */
            diff = (0xFFFFFFFF - seq_no->ipsec_low_order) +
                    ar_win->ipsec_last_seq.ipsec_low_order;

            /* Check if some packet with particular sequence no. has
             * been already received. If note, then accept this
             * packet.
             */
            if(!( IPSEC_ARWIN_ISSET( ar_win->ipsec_window, diff) ))
            {
                status = NU_SUCCESS;

                seq_no->ipsec_high_order =
                    ar_win->ipsec_last_seq.ipsec_high_order - 1;
            }
        }

        /* If the received packet has lower bits less than the
         * lower bits of the highest sequence number of the window,
         * this means that the received packet lies in the
         * higher sequence subspace.
         */
        else if ( seq_no->ipsec_low_order <=
                  ar_win->ipsec_last_seq.ipsec_low_order )
        {
            /* Determine the bit position for this sequence number. */
            diff = ar_win->ipsec_last_seq.ipsec_low_order -
                       seq_no->ipsec_low_order;

            if(!( IPSEC_ARWIN_ISSET( ar_win->ipsec_window, diff) ))
            {
                status = NU_SUCCESS;

                seq_no->ipsec_high_order =
                       ar_win->ipsec_last_seq.ipsec_high_order;
            }
        }

        /* We will come here if the received packet has a sequence number
         * greater than the current largest sequence number of the window.
         */
        else
        {
            status = NU_SUCCESS;

            seq_no->ipsec_high_order =
               ar_win->ipsec_last_seq.ipsec_high_order;
        }
    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Replay_Check. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Replay_Update
*
* DESCRIPTION
*
*       This function takes as input an Anti-Replay window structure
*       and a sequence number of a packet just received and validated.
*       The window is updated here.
*
* INPUTS
*
*       *ar_win                 Pointer to the AntiReplay window.
*       seq_no                  Sequence no. which has been retrieved
*                               from the received packet.
*
* OUTPUTS
*
*       NU_SUCCESS              Anti-replay window updated.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Replay_Update(IPSEC_ANTIREPLAY_WIN *ar_win, IPSEC_SEQ_NUM *seq_no)
{
    STATUS              status = NU_SUCCESS;
    UINT32              diff;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameter. */
    if(ar_win == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* If passed sequence no. is greater than the current greatest
     * sequence no. of AR window.
     */
    if( ( seq_no->ipsec_high_order >
          ar_win->ipsec_last_seq.ipsec_high_order ) ||
        ( ( seq_no->ipsec_high_order ==
            ar_win->ipsec_last_seq.ipsec_high_order ) &&
          ( seq_no->ipsec_low_order >
            ar_win->ipsec_last_seq.ipsec_low_order ) ) )
    {
        /* Get the difference between the two. */
        diff = IPSEC_SEQ_DIFF((*seq_no), ar_win->ipsec_last_seq);

        /* Shift the window by this much difference. */
        IPSEC_Arwin_Shift_Left(ar_win->ipsec_window, diff );

        /* Set the first bit to indicate that the highest packet has been
         * received.
         */
        IPSEC_ARWIN_SET_BIT( ar_win->ipsec_window, 0 );

        /* Update the window to indicate the largest received sequence. */
        ar_win->ipsec_last_seq.ipsec_low_order = seq_no->ipsec_low_order;
        ar_win->ipsec_last_seq.ipsec_high_order = seq_no->ipsec_high_order;

    }

    /* The packet sequence number is less than the sequence number
     * received.
     */
    else
    {
        /* Get the difference between the two. */
        diff = IPSEC_SEQ_DIFF(ar_win->ipsec_last_seq, (*seq_no));

        /* Update the window. */
        IPSEC_ARWIN_SET_BIT( ar_win->ipsec_window, diff );

    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Replay_Update. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Arwin_Shift_Left
*
* DESCRIPTION
*
*       All bits in the passed bit map are shifted to the left by 'num'
*       positions. num number of bits from the end of the bitmap will overflow.
*       'num' number of unset bits will be added to the beginning of the
*       bitmap.
*
* INPUTS
*
*       *ipsec_window           Array of 32-bit integers for the window.
*       num                     Number of bits to shift left.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_Arwin_Shift_Left(UINT32 *ipsec_window, UINT32 num)
{
    INT            i;
    UINT32         flags = 0xFFFFFFFF;
    UINT32         index_shift;

    /* Sanity check. If we are shifting more than IPSEC_SLIDING_WINDOW_SIZE
     * then just clear the window and return.
     */
    if ( num >= IPSEC_SLIDING_WINDOW_RANGE )
    {
        IPSEC_ARWIN_CLEAR(ipsec_window);
    }

    else
    {
        /* If we need to shift more than 32 bits, then we first shift in
         * multiples of 32 bits and then do the remainder later. Shifting
         * multiple of 32-bits is easy, we just copy the whole 32-bits over.
         */
        if ( num >= 32 )
        {
            index_shift = num / 32;
            num = num - ( index_shift * 32 );

            for ( i = IPSEC_SLIDING_WINDOW_SIZE - ( index_shift + 1);
                  i >= 0; i-- )
            {
                ipsec_window[i + index_shift] = ipsec_window[i];
            }

            /* Clear those integers that would have beome zero because of
             * overflowing.
             */
            for ( i = 0 ; i < index_shift ; i++ )
            {
                ipsec_window[i] = 0;
            }
        }

        /* Make sure we still have some more bits to shift. */
        if ( num > 0 )
        {
            /* Take care of the most significant 32 bits first. This shifts the
             * left most 32 bits. 'num' bits over flow.
             */
            ipsec_window[IPSEC_SLIDING_WINDOW_SIZE - 1] <<= num;

            /* For each 32 bits, we will shift left. This will cause some bits
             * to overflow. We need to capture the over flowing and put them
             * at the beginning of the next 32-bits. Here we create a flag
             * which we will use to capture the bits before we shift left.
             */
            flags <<= 32 - num;

            /* Now we shift the bits in the array. We go from the most
             * significant to the least. In each loop we first capture the
             * overflowing bits, shift them in to the more significant 32
             * bits and then do the shift.
             */
            for ( i = IPSEC_SLIDING_WINDOW_SIZE - 2; i >= 0; i-- )
            {
                ipsec_window[i + 1] |=
                    ( ( ipsec_window[i] & flags ) >> (32 - num) );

                ipsec_window[i] <<= num;
            }
        }
    }
}


#endif /* #if (IPSEC_ANTI_REPLAY == NU_TRUE) */
