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
*   FILE NAME
*
*       tftpc4.h
*
*   COMPONENT
*
*       TFTP -  Trivial File Transfer Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes of all TFTP functions.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef NU_TFTPC4_H
#define NU_TFTPC4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT32  TFTPC4_Get(UINT8 *, CHAR *, CHAR *, TFTP_OPTIONS *);
INT32  TFTPC4_Put(UINT8 *, CHAR *, CHAR *, TFTP_OPTIONS *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPC4_H */
