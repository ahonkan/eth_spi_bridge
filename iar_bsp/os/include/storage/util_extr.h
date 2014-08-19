/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       util_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Util
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains external interface for utility routines
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/util_defs.h"
#include "storage/file_cfg.h"

#ifndef UTIL_EXTR_H
#define UTIL_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
#define NUF_NEXT_CHAR(utf8_str) (utf8_str++)
#define NUF_PREV_CHAR(utf8_str) (--(*utf8_str))
    /* Always returns NU_SUCCESS after calculating the character position requested. */
#define NUF_GET_CHAR(pos_req,s,char_req) (((*(*pos_req = (s+char_req))) != 0) ? NU_SUCCESS : NU_SUCCESS)
#define NUF_IS_EQUAL(s1,s2) (*s1==*s2 ? 1 : 0)
#define NUF_COPYBUFF(to,from,size) (NUF_Copybuff(to,from,size))
#define NUF_NCPBUFF(to,from,size) (NUF_Ncpbuf(to,from,size))
#define NUF_GET_CP_LEN(str) (1)
#endif

VOID    NUF_Copybuff(VOID *vto, VOID *vfrom, INT size);
VOID    NUF_Memfill(VOID *vto, INT size, UINT8 c);
INT     NUF_Strncmp(const CHAR *s1, const CHAR *s2, INT n);
VOID    NUF_Ncpbuf(UINT8 *to, UINT8 *from, INT size); 
INT     NUF_Is_Equal(const CHAR *s1,const CHAR *s2);
INT     NUF_Atoi(const char *nptr);
UINT32  NUF_Get_Str_Len(const CHAR *str);

/* Endianness conversion routines */
VOID NUF_Fswap16(UINT16 *to, UINT16 *from);     /* Preserve little endian */
VOID NUF_Fswap32(UINT32 *to, UINT32 *from);     /* Preserve little endian */
VOID NUF_Swap16(UINT16 *to, UINT16 *from);
VOID NUF_Swap32(UINT32 *to, UINT32 *from);
VOID NUF_Through16(UINT16 *to, UINT16 *from);
VOID NUF_Through32(UINT32 *to, UINT32 *from);

/* Memory allocation */
VOID *NUF_Alloc(INT nbytes);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* UTIL_EXTR_H */
