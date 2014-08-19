/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       pap_defs.h
*
*   COMPONENT
*
*       PAP - Password Authentication Protocol
*
*   DESCRIPTION
*
*       Contains the constant and structure definitions for pap.c.
*
*   DATA STRUCTURES
*
*       AUTHENTICATION_LAYER
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_PAP_DEFS_H
#define PPP_INC_PAP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* Define that structure that will hold all information pertaining to
   authentication. */
typedef struct _authentication_layer
{
    CHAP_LAYER  chap;
    UINT32      um_id;
    UINT8       state;
    INT8        num_transmissions;
    UINT8       name_len;
    UINT8       pw_len;
    CHAR        login_name[PPP_MAX_ID_LENGTH];
    CHAR        login_pw[PPP_MAX_PW_LENGTH];
} AUTHENTICATION_LAYER;

/* We would use the identifier in the CHAP for both PAP and CHAP. */
#define auth_identifier             chap.challenge_identifier

#define PAP_AUTHENTICATE_REQUEST    1
#define PAP_AUTHENTICATE_ACK        2
#define PAP_AUTHENTICATE_NAK        3

#define PAP_ID_LENGTH_OFFSET        4
#define PAP_ID_OFFSET               5

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PAP_DEFS_H */
