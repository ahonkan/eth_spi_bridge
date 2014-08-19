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
*       trie_extr.h
*
* COMPONENT
*
*       TRIE
*
* DESCRIPTION
*
*       Contains the external interface for conversion 
*       functions that are needed for Unicode support.
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
#ifndef TRIE_EXTR_H
#define TRIE_EXTR_H

extern void * cp932_CP_idx[];
extern void * cp932_UNI_idx[];

extern void * cp936_CP_idx[];
extern void * cp936_UNI_idx[];

extern void * cp949_CP_idx[];
extern void * cp949_UNI_idx[];

extern void * cp950_CP_idx[];
extern void * cp950_UNI_idx[];




UINT8 Trie_UTF8_To_Unicode(UINT8 *unicode,UINT8 *utf8);
UINT8 Trie_Unicode_To_UTF8(UINT8 *utf8,UINT8 *unicode);

UINT8 Trie_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp, void *trie_idx);
UINT8 Trie_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8, void *trie_idx);


UINT8 Trie_CP932_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp);
UINT8 Trie_CP932_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8);

UINT8 Trie_CP936_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp);
UINT8 Trie_CP936_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8);

UINT8 Trie_CP949_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp);
UINT8 Trie_CP949_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8);

UINT8 Trie_CP950_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp);
UINT8 Trie_CP950_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8);

#endif /* TRIE_EXTR_H */
