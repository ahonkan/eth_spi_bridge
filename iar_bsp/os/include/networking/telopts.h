/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/* Portions of this program were written by: */
/*************************************************************************
*
*      part of:
*      TCP/IP kernel for NCSA Telnet
*      by Quincey Koziol
*
*      National Center for Supercomputing Applications
*      152 Computing Applications Building
*      605 E. Springfield Ave.
*      Champaign, IL  61820
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME                                              
*
*       telopts.h                                      
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       The main MACROs for telnet codes and common telnet definitions
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef TELOPTS_H
#define TELOPTS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* commonly defined macros */
#define NK_TELNET   23

#define TN_CLIENT   NU_FALSE

#define TN_1_1      1
#define TN_1_2      2

#define TN_VERSION_COMP     TN_1_2

/* Macros in NEGOTIAT.c */
#define NEG_SUBNEG_CODE     1
#define NEG_IAC_CODE        2
#define NEG_ASCII_CODE      4
#define NEG_ESC_CODE        8

/* so far, the longest telnet command can be received when the client
   is stating the name of his current (or only) terminal
   type.  In this case, the following subnegotiation command is sent
   to the server:
                  IAC SB TERMINAL-TYPE IS term_type_name IAC SE
   where term_type_name can be up to 40 chars long.
   See http://www.faqs.org/rfcs/rfc1091.html */
#define MAX_TN_COMMAND      47  /* 40 chars for name, 6 for escape chars, 1 for NULL */

#define NEG_TIMEOUT         5   /* amount of requests during negotiation before moving on */

#define NUMLMODEOPTIONS     30

/* Definitions for telnet protocol */

#define STNORM      0

/* Definition of the lowest telnet byte following an IAC byte */
#define LOW_TEL_OPT 236

#define TO_EOF         236
#define TO_SUSP        237
#define TO_ABORT       238

#define TO_SE          240
#define TO_NOP         241
#define TO_DM          242
#define TO_BREAK       243
#define TO_IP          244
#define TO_AO          245
#define TO_AYT         246
#define TO_EC          247
#define TO_EL          248
#define TO_GOAHEAD     249
#define TO_SB          250
#define TO_WILLTEL     251
#define TO_WONTTEL     252
#define TO_DOTEL       253
#define TO_DONTTEL     254
#define TO_IAC         255

/* Assigned Telnet Options */
#define TO_BINARY              0
#define TO_ECHO                1
#define TO_RECONNECT           2
#define TO_SGA                 3
#define TO_AMSN                4
#define TO_STATUS              5
#define TO_TIMING              6
#define TO_RCTAN               7
#define TO_OLW                 8
#define TO_OPS                 9
#define TO_OCRD                10
#define TO_OHTS                11
#define TO_OHTD                12
#define TO_OFFD                13
#define TO_OVTS                14
#define TO_OVTD                15
#define TO_OLFD                16
#define TO_XASCII              17
#define TO_LOGOUT              18
#define TO_BYTEM               19
#define TO_DET                 20
#define TO_SUPDUP              21
#define TO_SUPDUPOUT           22
#define TO_SENDLOC             23
#define TO_TERMTYPE            24
#define TO_EOR                 25
#define TO_TACACSUID           26
#define TO_OUTPUTMARK          27
#define TO_TERMLOCNUM          28
#define TO_REGIME3270          29
#define TO_X3PAD               30
#define TO_NAWS                31
#define TO_TERMSPEED           32
#define TO_TFLOWCNTRL          33
#define TO_LINEMODE            34
#define TO_MODE                1
#define TO_MODE_EDIT           1
#define TO_MODE_TRAPSIG        2
#define TO_MODE_ACK            4
#define TO_MODE_SOFT_TAB       8
#define TO_MODE_LIT_ECHO       16

/* Telnet Terminal-Type Subnegotiation Parameters, see rfc 1091 */
#define TO_IS               0  /* client tells server client's term.type      */
#define TO_SEND             1  /* server asks client about client's term.type */

#define FORWARDMASK         2

#define SLC                 3
#define SLC_DEFAULT         3
#define SLC_VALUE           2
#define SLC_CANTCHANGE      1
#define SLC_NOSUPPORT       0
#define SLC_LEVELBITS       3

#define SLC_ACK             128
#define SLC_FLUSHIN         64
#define SLC_FLUSHOUT        32

#define SLC_SYNCH           1
#define SLC_BRK             2
#define SLC_IP              3
#define SLC_AO              4
#define SLC_AYT             5
#define SLC_EOR             6
#define SLC_ABORT           7
#define SLC_EOF             8
#define SLC_SUSP            9
#define SLC_EC              10
#define SLC_EL              11
#define SLC_EW              12
#define SLC_RP              13
#define SLC_LNEXT           14
#define SLC_XON             15
#define SLC_XOFF            16
#define SLC_FORW1           17
#define SLC_FORW2           18
#define SLC_MCL             19
#define SLC_MCR             20
#define SLC_MCWL            21
#define SLC_MCWR            22
#define SLC_MCBOL           23
#define SLC_MCEOL           24
#define SLC_INSRT           25
#define SLC_OVER            26
#define SLC_ECR             27
#define SLC_EWR             28
#define SLC_EBOL            29
#define SLC_EEOL            30

#define XDISPLOC            35
#define ENVIRONMENT         36
#define AUTHENTICATION      37
#define DATA_ENCRYPTION     38
#define XOPTIONS            255

#define LINEMODE_MODES_SUPPORTED    0x1B
/* set this flag for linemode special functions which are supported by Telnet,
 even though they are not currently active. This is to allow the other side
 to negotiate to a "No Support" state for an option and then change later to
 supporting it, so we know it's ok to change our "No Support"
 state to something else ("Can't Change", "Value", whatever) */
#define SLC_SUPPORTED       0x10

#define ESCFOUND 5
#define IACFOUND 6
#define NEGOTIATE 1

/* the following MACROs are used to record the results of negotiations */
#define NEGO_I_DO               0x001 /* bit 1 */
#define NEGO_I_DONT             0x002 /* bit 2 */
#define NEGO_I_WILL             0x004 /* bit 3 */
#define NEGO_I_WONT             0x008 /* bit 4 */
#define NEGO_HE_DO              0x010 /* bit 5 */
#define NEGO_HE_DONT            0x020 /* bit 6 */
#define NEGO_HE_WILL            0x040 /* bit 7 */
#define NEGO_HE_WONT            0x080 /* bit 8 */

#define NEGO_I_REQUIRED_SUBNEG  0x100 /* bit  9 -- I requested subnegotiation info. */
#define NEGO_HE_SENT_SUBNEG     0x200 /* bit 10 -- He sent subnegotiation info.     */
#define NEGO_HE_REQUIRED_SUBNEG 0x400 /* bit 11 -- He requested subnegotiation info.*/
#define NEGO_I_SENT_SUBNEG      0x800 /* bit 12 -- I sent subnegotiation info.      */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* telopts.h */
