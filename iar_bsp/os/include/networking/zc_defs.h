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
*       zc_defs.h
*
*   COMPONENT
*
*       ZeroCopy Interface
*
*   DESCRIPTION
*
*       Defines the ZEROCOPY definitions.
*
*   DATA STRUCTURES
*
*
*   DEPENDENCIES
*
*       os.h
*       mem_defs.h
*       socketd.h
*
*
*************************************************************************/
#include "networking/mem_defs.h"
#include "networking/socketd.h"

#ifndef ZC_DEFS_H
#define ZC_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Interface macros to allow application to use ZC approach with chained
 * NET Buffers
 */

#define NU_ZC_BUF_LEN(a) ((a) ? (a)->mem_total_data_len : 0)

#define NU_ZC_SEGMENT_BYTES_AVAIL(a) ((a) ? (a)->data_len : 0)

#define NU_ZC_SEGMENT_NEXT(a) ((a) ? (a)->next_buffer : 0)

#define NU_ZC_SEGMENT_DATA(a) ((a) ? (CHAR*)((a)->data_ptr) : 0)

#define NU_ZC_SEGMENT_BYTES_LEFT(a, b, c) NU_ZC_Bytes_Left(a, b, c)

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
