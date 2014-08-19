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
*       uni_util_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       UNICODE
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains external interface for Unicode utility routines.
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
#include "storage/uni_defs.h"
#include "storage/trie_extr.h"
#include "storage/file_cfg.h"

#ifndef UNI_UTIL_EXTR_H
#define UNI_UTIL_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)


#define NUF_NEXT_CHAR(utf8_str) if(*utf8_str<=0x7f) utf8_str+=1; \
                                else if(*utf8_str>=0xc2 && *utf8_str<=0xdf) utf8_str+=2; \
                                else if(*utf8_str >=0xe0 && *utf8_str <=0xEF) utf8_str+=3;\
                                else utf8_str+=4
#define NUF_PREV_CHAR(utf8_str) (NUF_Uni_Prev_Char(utf8_str))
#define NUF_GET_CHAR(pos_char,char_str,char_num) (NUF_Uni_Get_Char(pos_char,char_str,char_num))
#define NUF_IS_EQUAL(s1,s2) (NUF_Uni_Is_Equal(s1,s2))
#define NUF_COPYBUFF(to,from,size) (NUF_Uni_Copybuff(to,from,size))
#define NUF_NCPBUFF(to,from,size) (NUF_Uni_Ncpbuf(to,from,size))
#define NUF_GET_CP_LEN(str)(get_char_length(str)< 2 ? 1 : 2)



STATUS  NUF_Uni_Get_Char(CHAR** pos_req,CHAR *s,INT char_req);
INT     NUF_Uni_Is_Equal(const CHAR *s1,const CHAR *s2);
STATUS  NUF_Valid_UTF8_Encoding(const CHAR *encode_seq,INT len);
VOID    NUF_Uni_Copybuff(VOID *vto,VOID *vfrom, INT n);
VOID    NUF_Uni_Ncpbuf(UINT8 *to,UINT8 *from,INT size);
VOID    NUF_Get_CP_Loaded(INT *cp_loaded);
VOID    NUF_Uni_Prev_Char(CHAR** str);
INT     NUF_Get_CP_Length(CHAR *short_filename);
STATUS  NUF_Codepage_To_UTF8(UINT8 *utf8_buff, UINT8 *cp,UINT8 (*cp_to_utf8_func)(UINT8*,UINT8*) );
STATUS  NUF_UTF8_To_Codepage(UINT8 *cp_buff, UINT8 *utf8,UINT8 (*utf8_to_cp_func)(UINT8*,UINT8*) );

#if (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
#define NUF_CP932_To_UTF8(utf8,cp) NUF_Codepage_To_UTF8(utf8,cp,&Trie_CP932_Codepage_To_UTF8)
#define NUF_UTF8_To_CP932(cp,utf8) NUF_UTF8_To_Codepage(cp,utf8,&Trie_CP932_UTF8_To_Codepage)
#endif

#if (ENABLE_W_SIMPLIFIED_CHINESE == NU_TRUE)
#define NUF_CP936_To_UTF8(utf8,cp) NUF_Codepage_To_UTF8(utf8,cp,&Trie_CP936_Codepage_To_UTF8)
#define NUF_UTF8_To_CP936(cp,utf8) NUF_UTF8_To_Codepage(cp,utf8,&Trie_CP936_UTF8_To_Codepage)
#endif

#if (ENABLE_W_KOREAN == NU_TRUE)
#define NUF_CP949_To_UTF8(utf8,cp) NUF_Codepage_To_UTF8(utf8,cp,&Trie_CP949_Codepage_To_UTF8)
#define NUF_UTF8_To_CP949(cp,utf8) NUF_UTF8_To_Codepage(cp,utf8,&Trie_CP949_UTF8_To_Codepage)
#endif

#if (ENABLE_W_TRADITIONAL_CHINESE == NU_TRUE)
#define NUF_CP950_To_UTF8(utf8,cp) NUF_Codepage_To_UTF8(utf8,cp,&Trie_CP950_Codepage_To_UTF8)
#define NUF_UTF8_To_CP950(cp,utf8) NUF_UTF8_To_Codepage(cp,utf8,&Trie_CP950_UTF8_To_Codepage)
#endif

INT     get_char_length(const CHAR *utf8_char);
UINT8   utf8_to_unicode(UINT8 *unicode,UINT8 *utf8);
UINT8   unicode_to_utf8(UINT8 *utf8,UINT8 *unicode);

#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 */ 

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* UNI_UTIL_EXTR_H */
