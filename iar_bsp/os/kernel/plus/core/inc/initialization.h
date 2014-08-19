/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains data structure definitions and constants for
*       the Initialization component.
*
***********************************************************************/

/* Check to see if the file has been included already.  */

#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#ifdef          __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* Constants used to indicate start and end of initialization for Nucleus PLUS. */
#define INC_START_INITIALIZE                0
#define INC_END_INITIALIZE                  2

/* Define function prototypes */
VOID            INC_Initialize(VOID *first_available_memory);
VOID            INCT_Sys_Mem_Pools_Initialize(VOID  *avail_mem_ptr);

/* External variable declarations */
extern INT      INC_Initialize_State;

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif /* INITIALIZATION_H */
