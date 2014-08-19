/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       trap.h                                                   
*
*   DESCRIPTION
*
*       This file contains functions, macros and data structures
*       specific to traps.
*
*   DATA STRUCTURES
*
*       TrapSpecificCodes
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef TRAP_H
#define TRAP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

extern INT8 *TrapSpecific[];

enum TrapSpecificCodes
{
    TRAP_SPECIFIC_NONE = 0,
    TRAP_SPECIFIC_RISINGALARM,
    TRAP_SPECIFIC_FALLINGALARM,
    TRAP_SPECIFIC_PACKETMATCH,
    TRAP_SPECIFIC_REGISTER,
    TRAP_SPECIFIC_NEUROTHRESHOLD
};

#ifdef          __cplusplus
}
#endif /* __cplusplus */
#endif


