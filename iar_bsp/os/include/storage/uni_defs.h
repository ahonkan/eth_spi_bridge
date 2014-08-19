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
*       uni_defs.h
*
* COMPONENT
*
*       UNICODE
*
* DESCRIPTION
*
*       Contains defines for the unicode services.
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
#ifndef UNI_DEFS_H
#define UNI_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* 
    Default Character Set. 
*/
#define ENABLE_ASCII                       NU_TRUE
/*
    Single Byte Character Set Codepages
    CP1250	Central Europe (Windows)
*/
#define ENABLE_W_CENTRAL_EUROPE            NU_FALSE 
/*
    CP1251  Cyrillic (Windows)
*/
#define ENABLE_W_CYRILLIC                  NU_FALSE
/*
    CP1252  Latin I (Windows)
*/    
#define ENABLE_W_LATIN_I                   NU_FALSE
/*
    CP1253  Greek (Windows)
*/
#define ENABLE_W_GREEK                     NU_FALSE
/*
    CP1254  Turkish (Windows)
*/    
#define ENABLE_W_TURKISH                   NU_FALSE
/*
    CP1255  Hebrew (Windows)
*/
#define ENABLE_W_HEBREW                    NU_FALSE
/*
    CP1256  Arabic (Windows)
*/
#define ENABLE_W_ARABIC                    NU_FALSE
/*
    CP1257  Baltic (Windows)
*/
#define ENABLE_W_BALTIC                    NU_FALSE
/*
    CP1258  Vietnam (Windows)
*/
#define ENABLE_W_VIETNAM                   NU_FALSE
/*
    CP874   Thai (Windows)
*/
#define ENABLE_W_THAI                      NU_FALSE
/*
    Double Byte Character Set Codepages
    CP932   Japanese Shift-JIS (Windows)
*/
#define ENABLE_W_JAPANESE_SHIFT_JIS        NU_TRUE
/*
    CP936   Simplified Chinese GBK (Windows)
*/
#define ENABLE_W_SIMPLIFIED_CHINESE        NU_TRUE
/*
    CP949   Korean (Windows)
*/
#define ENABLE_W_KOREAN                    NU_TRUE
/*
    CP950   Traditional Chinese Big5 (Windows)
*/
#define ENABLE_W_TRADITIONAL_CHINESE       NU_TRUE

/****************DEFINE CODEPAGE UNIQUE ID***************/
#define CP_CENTRAL_EUROPE            1250
#define CP_CYRILLIC                  1251
#define CP_LATIN_I                   1252
#define CP_GREEK                     1253
#define CP_TURKISH                   1254
#define CP_HEBREW                    1255
#define CP_ARABIC                    1256
#define CP_BALTIC                    1257
#define CP_VIETNAM                   1258
#define CP_THAI                      874
#define CP_JAPANESE_SHIFT_JIS        932
#define CP_SIMPLIFIED_CHINESE        936
#define CP_KOREAN                    949
#define CP_TRADITIONAL_CHINESE       950
#define CP_ASCII                     1

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* UNI_UTIL_EXTR_H */


