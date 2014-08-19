/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       keypad_ge_extr.h
*
*   COMPONENT
*
*       Keypad driver
*
*   DESCRIPTION
*
*       This file contains the function prototypes for Nucleus Keypad
*       driver generic component.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       keypad_ge_defs.h
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     KEYPAD_GE_EXTR
#define     KEYPAD_GE_EXTR

#include    "drivers/keypad_ge_defs.h"

VOID        KP_Delay_usec(UINT32 no_of_usec);

#endif      /* KEYPAD_GE_EXTR */
