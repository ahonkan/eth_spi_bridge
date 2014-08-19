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
*       gre.h
*
*   DESCRIPTION
*
*       Contains the defines for the GRE protocol.
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

#ifndef GRE_H
#define GRE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* The following definitions make accessing the fields in the unions easier. */

#define GRE_VERSION1_PROTOCOL           0x880B
#define GRE_HEADER_LEN                  8
#define GRE_VERSION_OFFSET              0x07
#define GRE_VERSION_ONE                 1

#define GRE_VERSION_BYTE_OFFSET         1
#define GRE_PROTOCOL_OFFSET             2
#define GRE_CHECKSUM_OFFSET             4
#define GRE_RESERVED_OFFSET             6

#if (INCLUDE_GRE == NU_TRUE)
/* Function Prototypes. */
STATUS  GRE_Interpret(NET_BUFFER *buf_ptr, UINT32);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* GRE_H */
