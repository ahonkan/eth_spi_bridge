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
*   FILENAME
*
*       test_extr.h
*
*   COMPONENT
*
*       Nucleus internal testing.
*
*   DESCRIPTION
*
*       This include file will handle functions related to Nucleus self-test
*       cases.  Note that these self-test cases are used for internal Mentor
*       Graphics testing only and cannot be enabled by the end-user.
*
*   DATA STRUCTURES
*
*       None.
*
*   DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef TEST_EXTR_H
#define TEST_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#if (CFG_NU_TF_NET_SHARED_SELF_TEST_INCLUDE_API_SELF_TEST == 1)
    #define FAIL_UNFINISHED_TESTS   NU_TRUE      /* Report failure for unfinished tests. */
    #define TEST_TCP_NEW_RENO       NU_TRUE      /* Enable test code for New Reno. */
    #define TEST_TCP_SACK           NU_TRUE      /* Enable test code for TCP SACK. */
    #define TEST_TCP_DSACK          NU_TRUE      /* Enable test code for TCP D-SACK. */
    #define TEST_TCP_WINDOWSCALE    NU_TRUE      /* Enable test code for Window Scale option. */
    #define TEST_TCP_TIMESTAMP      NU_TRUE      /* Enable test code for Timestamp option. */
    #define TEST_TCP_LMTD_TX        NU_TRUE      /* Enable test code for Limited Transmit. */
#else
    #define FAIL_UNFINISHED_TESTS   NU_FALSE     /* Report failure for unfinished tests. */
    #define TEST_TCP_NEW_RENO       NU_FALSE     /* Disable test code for New Reno. */
    #define TEST_TCP_SACK           NU_FALSE     /* Disable test code for TCP SACK. */
    #define TEST_TCP_DSACK          NU_FALSE     /* Disable test code for TCP D-SACK. */
    #define TEST_TCP_WINDOWSCALE    NU_FALSE     /* Disable test code for Window Scale option. */
    #define TEST_TCP_TIMESTAMP      NU_FALSE     /* Disable test code for Timestamp option. */
    #define TEST_TCP_LMTD_TX        NU_FALSE     /* Disable test code for Limited Transmit. */
#endif

#if (CFG_NU_TF_NET_SHARED_SELF_TEST_INCLUDE_DEFECTS_SELF_TEST == 1)
    #define TEST_TCP_DEFECTS        NU_TRUE      /* Enable test code for TCP defects. */
#else
    #define TEST_TCP_DEFECTS        NU_FALSE     /* Disable test code for TCP defects. */
#endif

#if (CFG_NU_TF_NET_SHARED_SELF_TEST_INCLUDE_UNIT_SELF_TEST == 1)
    #define TEST_IP_REASSEMBLY      NU_TRUE      /* Enable test code for IP reassembly. */
#else
    #define TEST_IP_REASSEMBLY      NU_FALSE     /* Disable test code for IP reassembly. */
#endif

#if ( (INCLUDE_NEWRENO == NU_FALSE) && (TEST_TCP_NEW_RENO == NU_TRUE) )
    #error INCLUDE_NEWRENO must be enabled to test the functionality
#endif

#if ( (NET_INCLUDE_SACK == NU_FALSE) && (TEST_TCP_SACK == NU_TRUE) )
    #error NET_INCLUDE_SACK must be enabled to test the functionality
#endif

#if ( (NET_INCLUDE_DSACK == NU_FALSE) && (TEST_TCP_DSACK == NU_TRUE) )
    #error NET_INCLUDE_DSACK must be enabled to test the functionality
#endif

#if ( (INCLUDE_TCP_KEEPALIVE == NU_FALSE) && (TEST_TCP_TIMESTAMP == NU_TRUE) )
    #error INCLUDE_TCP_KEEPALIVE must be enabled to test Timestamp functionality
#endif

#if ( (NET_INCLUDE_LMTD_TX == NU_FALSE) && (TEST_TCP_LMTD_TX == NU_TRUE) )
    #error NET_INCLUDE_LMTD_TX must be enabled to test Limited Transmit functionality
#endif

#if ( (INCLUDE_IP_REASSEMBLY == NU_FALSE) && (TEST_IP_REASSEMBLY == NU_TRUE) )
    #error INCLUDE_IP_REASSEMBLY must be enabled to test IP reassembly functionality
#endif

#if ( (TEST_TCP_NEW_RENO == NU_TRUE) || (TEST_TCP_SACK == NU_TRUE) || \
      (TEST_TCP_DSACK == NU_TRUE) || (TEST_TCP_TIMESTAMP == NU_TRUE) || \
      (TEST_TCP_LMTD_TX == NU_TRUE) || (TEST_TCP_DEFECTS == NU_TRUE) )

#define TEST_TCP_MODULE     NU_TRUE

extern STATUS TEST_TCP_Rx(TCPLAYER *p, UINT16 *tlen, UINT16 hlen, NET_BUFFER *buf_ptr,
                          TCP_PORT *prt);
#else

#define TEST_TCP_MODULE     NU_FALSE

#define TEST_TCP_Rx(p, tlen, hlen, buf_ptr, prt)    NU_SUCCESS

#endif

#if (TEST_TCP_NEW_RENO == NU_TRUE)

STATUS  TEST_TCP_NewReno(UINT32, UINT32, TCPLAYER *, UINT16);
VOID    TEST_TCP_Fast_Rx_Postconditions(TCP_BUFFER *tcp_buf, TCP_PORT *prt);
VOID    TEST_TCP_No_Fast_Rx(VOID);
VOID    TEST_TCP_DupACK_Postconditions(TCP_PORT *);
VOID    TEST_TCP_Exit_Fast_Rx_Postconditions(TCP_PORT *prt);
VOID    TEST_TCP_Partial_ACK_Postconditions(TCP_PORT *prt);
VOID    TEST_TCP_Restore_ACK_Postconditions(TCP_PORT *prt);
VOID    TEST_TCP_No_New_Data(VOID);
VOID    TEST_TCP_New_Data(UINT32 cwnd, TCP_PORT *prt);
VOID    TEST_TCP_Retrans_Data(UINT32 seq_num);
VOID    TEST_TCP_Full_ACK(TCP_PORT *prt);

#else

#define TEST_TCP_NewReno(seq_num, ack_num, bytes_acked, hdr)        NU_SUCCESS
#define TEST_TCP_Fast_Rx_Postconditions(tcp_buf, prt)
#define TEST_TCP_No_Fast_Rx()
#define TEST_TCP_DupACK_Postconditions(prt)
#define TEST_TCP_Exit_Fast_Rx_Postconditions(prt)
#define TEST_TCP_Partial_ACK_Postconditions(prt)
#define TEST_TCP_Restore_ACK_Postconditions(prt)
#define TEST_TCP_No_New_Data()
#define TEST_TCP_New_Data(cwnd, prt)
#define TEST_TCP_Retrans_Data(seq_num)
#define TEST_TCP_Full_ACK(prt)

#endif

#if (TEST_TCP_SACK == NU_TRUE)

VOID    TEST_TCP_SACK_Perm_Included(TCP_PORT *);
VOID    TEST_TCP_SACK_Included(UINT32, UINT32);
STATUS  TEST_TCP_SACK_Receive(TCP_PORT *, UINT16, UINT32, UINT32);
VOID    TEST_TCP_SACK_Retransmit(UINT32);

#else

#define TEST_TCP_SACK_Perm_Included(prt)
#define TEST_TCP_SACK_Included(left_edge, right_edge)
#define TEST_TCP_SACK_Receive(pport, dlen, seq_num, ack)    NU_SUCCESS
#define TEST_TCP_SACK_Retransmit(seq_num)

#endif

#if (TEST_TCP_DSACK == NU_TRUE)

VOID    TEST_TCP_DSACK_Included(UINT8 *, UINT16, UINT32);
STATUS  TEST_TCP_DSACK_Receive(TCP_PORT *, NET_BUFFER *, UINT16 *, UINT16, UINT32, UINT32);

#else

#define TEST_TCP_DSACK_Included(ptr, hlen, ack)
#define TEST_TCP_DSACK_Receive(pport, buffer, tlen, hlen, seq_num, ack) NU_SUCCESS

#endif

#if (TEST_TCP_TIMESTAMP == NU_TRUE)

STATUS  TEST_TCP_Timestamp_Send(UINT32 clock, TCP_PORT *);
STATUS  TEST_TCP_Timestamp_Receive(TCP_PORT *, NET_BUFFER *, UINT16, UINT16, UINT32);

#else

#define TEST_TCP_Timestamp_Send(clock, prt)
#define TEST_TCP_Timestamp_Receive(pport, buf_ptr, tlen, hlen, seq_num) NU_SUCCESS

#endif

#if (TEST_TCP_LMTD_TX == NU_TRUE)

STATUS  TEST_TCP_Lmtd_TX_Receive(UINT32, UINT32, TCPLAYER *, UINT16, UINT16);
VOID    TEST_TCP_LmtdTX_Postconditions(TCP_PORT *);

#else

#define TEST_TCP_Lmtd_TX_Receive(seq_num, ack_num, tcp_header, dlen, hlen)  NU_SUCCESS
#define TEST_TCP_LmtdTX_Postconditions(prt)

#endif

#if (TEST_TCP_DEFECTS == NU_TRUE)

STATUS TEST_TCP_Defect_Receive(UINT32, UINT32, TCPLAYER *, UINT16, UINT16);

#else

#define TEST_TCP_Defect_Receive(seq_num, ack_num, tcp_header, dlen, hlen)   NU_SUCCESS

#endif

#if (TEST_IP_REASSEMBLY == NU_TRUE)

STATUS TEST_IP_Reassembly_Input(IPLAYER **pkt, NET_BUFFER **buf_ptr, UINT16 hlen);

#else

#define TEST_IP_Reassembly_Input(pkt, buf_ptr, hlen)    NU_SUCCESS

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef TEST_EXTR_H */

