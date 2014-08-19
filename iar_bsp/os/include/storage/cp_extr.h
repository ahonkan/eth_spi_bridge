/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved                           */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       cp_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Code Page
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and extern to pointers character format 
*       conversions.
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

#include "storage/pcdisk.h"
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)

extern void * cp1250_CP[];
extern void * cp1250_UNI[];
extern void * cp1251_CP[];
extern void * cp1251_UNI[];
extern void * cp1252_CP[];
extern void * cp1252_UNI[];
extern void * cp1253_CP[];
extern void * cp1253_UNI[];
extern void * cp1254_CP[];
extern void * cp1254_UNI[];
extern void * cp1255_CP[];
extern void * cp1255_UNI[];
extern void * cp1256_CP[];
extern void * cp1256_UNI[];
extern void * cp1257_CP[];
extern void * cp1257_UNI[];
extern void * cp1258_CP[];
extern void * cp1258_UNI[];
extern void * cp874_CP[];
extern void * cp874_UNI[];
extern void * cp932_CP[];
extern void * cp932_UNI[];
extern void * cp936_CP[];
extern void * cp936_UNI[];
extern void * cp949_CP[];
extern void * cp949_UNI[];
extern void * cp950_CP[];
extern void * cp950_UNI[];
#define CP1250        0
#define CP1251        1
#define CP1252        2
#define CP1253        3
#define CP1254        4
#define CP1255        5
#define CP1256        6
#define CP1257        7
#define CP1258        8
#define CP874        9
#define CP932        10
#define CP936        11
#define CP949        12
#define CP950        13

#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 */
