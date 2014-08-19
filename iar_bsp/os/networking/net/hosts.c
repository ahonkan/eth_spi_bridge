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

/**************************************************************************
*
*   FILENAME
*
*       hosts.c
*
*   DESCRIPTION
*
*       This file contains the hostTable structure.
*
*   DATA STRUCTURES
*
*       hostTable[]
*
*   FUNCTIONS
*
*       None.
*
*   DEPENDENCIES
*
*       externs.h
*
****************************************************************************/

#include "networking/externs.h"

/* The following array lists an example entry for the host's name to
 * IP mapping (used by DNS for an example).
 * The last entry should always be NULL.
 */

struct host hostTable[] =
{
    {"MODEL",      {198, 100, 100, 1}, NU_FAMILY_IP},
    {"\0",         {0, 0, 0, 0}, 0}
};
