/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  str_utils.h                                                  
*
* DESCRIPTION
*
*  This file contains prototypes and externs for str_utils.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _STR_UTILS_H_
#define _STR_UTILS_H_


#ifdef __cplusplus      /* If C++, disable "mangling" */
extern "C" {
#endif

/* externed function prototypes */
extern VOID* MEM_calloc(UINT32 cnt, UINT32 size);    


/* function prototypes */
UNICHAR *STR_all_trim(UNICHAR *str);
INT32    STR_between(INT32 a, INT32 b, INT32 c);
INT32    STR_empty(UNICHAR *str);
INT32    STR_in_between(INT32 a, INT32 b, INT32 c);
VOID    *STR_mem_chr (const VOID *s, INT32 c, INT32 n);
INT32    STR_mem_cmp (const VOID *s1, const VOID *s2, INT32 n);
VOID     STR_mem_cpy (VOID *d, VOID *s, INT32 n);
VOID    *STR_mem_move (VOID *s1, const VOID *s2, INT32 n);
VOID     STR_mem_set( VOID *s, INT32 c, INT32 cnt);
UNICHAR *STR_str_cat( UNICHAR *str_destination, const UNICHAR *str_source );
UNICHAR *STR_str_chr( const UNICHAR *string, INT32 c );
INT32    STR_str_cmp( const UNICHAR *string1, const UNICHAR *string2 );
VOID     STR_str_cpy(UNICHAR *d, const UNICHAR *s);
UNICHAR *STR_str_dup(const UNICHAR *s);
UINT32   STR_str_len( const UNICHAR *string );
UNICHAR *STR_strr_chr(const UNICHAR *s, INT32 c);
UNICHAR *STR_str_rev(UNICHAR *s);
UINT32   STR_str_spn( const UNICHAR *s1, const UNICHAR *s2 );
UNICHAR *STR_str_str(const UNICHAR *s1, const UNICHAR *s2 ) ;
UNICHAR *STR_str_tok(UNICHAR *s1, const UNICHAR *s2 );
INT32    STR_stri_cmp( const UNICHAR *string1, const UNICHAR *string2 );
INT32    STR_strn_cmp(const UNICHAR *s1, const UNICHAR *s2, UINT32 n);
VOID     STR_strn_cpy(UNICHAR *d, const UNICHAR *s, INT32 n);
UNICHAR *STR_sub_str(UNICHAR *str, INT32 b, INT32 len);


#ifdef __cplusplus
}
#endif

#endif /* _STR_UTILS_H_ */
