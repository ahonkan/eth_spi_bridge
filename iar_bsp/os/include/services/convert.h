/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*		convert.h
*
* COMPONENT
*
*		RTL - RunTime Library.
*
* DESCRIPTION
*
*		This file contains the ASCII to XXX conversion routines.
*
* DATA STRUCTURES
*
* DEPENDENCIES
*
*		"limits.h"							Contains implementation
*											defined constants.
*
*************************************************************************/
#ifndef __CONVERT_H_
#define __CONVERT_H_

#include "services/limits.h"

/***********************************************************************
*   Macros to negate an unsigned long, and convert to a signed long
*   and to negate an unsigned int, and convert to a signed int.
*   It is needed to prevent possible overflows for large negatives.
*   These macros should work on any form of integer representation.
************************************************************************/

#define SNEGATE(uvalue)    ((uvalue <= LONG_MAX)        \
                ?  (-(long) uvalue )                    \
                :  (-(long)(uvalue-LONG_MAX) - LONG_MAX))

#define SINEGATE(uvalue)   ((uvalue <= INT_MAX)         \
                ?  (-(int) uvalue)                      \
                :  (-(int)(uvalue-INT_MAX) - INT_MAX))


#endif /* __CONVERT_H_ */


