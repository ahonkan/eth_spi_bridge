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
*  markers.h                                                    
*
* DESCRIPTION
*
*  This file defines the stroked markers font.
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
#ifndef _MARKERS_H_
#define _MARKERS_H_


fontRcd MarkerFont;

UINT16 locnTable[16];

UINT8 ofwdTable[32] = {
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0,
    16,0};

#define FontTable C_65[0]

UINT8 C_65[] = {
    5,8,
    4,7,
    5,6,
    6,7,
    5,8,
    255,255};   /* . */
UINT8 C_66[] = {
    5,14,
    5,0,
    255,254,
    0,7,
    10,7,
    255,255};   /* + */

UINT8 C_67[] = {
    5,14,
    5,0,
    255,254,
    0,7,
    10,7,
    255,254,
    1,12,
    9,2,
    255,254,
    1,2,
    9,12,
    255,255};   /* * */

UINT8 C_68[] = {
    0,14,
    0,0,
    10,0,
    10,14,
    0,14,
    255,255};   /* rectangle */

UINT8 C_69[] = {
    0,14,
    10,0,
    255,254,
    0,0,
    10,14,
    255,255};   /* x */

UINT8 C_70[] = {
    5,14,
    0,7,
    5,0,
    10,7,
    5,14,
    255,255};   /* diamond */

UINT8 C_71[] = {
    5,14,
    0,0,
    10,0,
    5,14,
    255,255};   /* triangle */

UINT8 C_72[] = {
    5,8,
    4,7,
    5,6,
    6,7,
    5,8,
    255,255};   /* . */

UINT8 C_73[] = {
    5,14,
    5,0,
    255,254,
    0,7,
    10,7,
    255,255};   /* + */

UINT8 C_74[] = {
    5,14,
    5,0,
    255,254,
    0,7,
    10,7,
    255,254,
    1,12,
    9,2,
    255,254,
    1,2,
    9,12,
    255,255};   /* * */

UINT8 C_75[] = {
    255,253,
    0,0,
    10,14,
    255,255};   /* Filled rectangle */

UINT8 C_76[] = {
    0,14,
    10,0,
    255,254,
    0,0,
    10,14,
    255,255};   /* x */

UINT8 C_77[] = {
    5,14,
    0,7,
    5,0,
    10,7,
    5,14,
    255,255};   /* diamond */

UINT8 C_78[] = {
    5,14,
    0,0,
    10,0,
    5,14,
    255,255};   /* triangle */

UINT8 C_79[] = {
    3,0,
    7,0,
    10,3,
    10,11,
    7,14,
    3,14,
    0,11,
    0,3,
    3,0,
    255,255};   /* octagon */

UINT8 C_80[] = {
    5,14,
    0,0,
    10,0,
    5,14,
    255,255};   /* triangle - bad character */

#define Font_End C_80 + 10


#endif /* _MARKERS_H_ */




