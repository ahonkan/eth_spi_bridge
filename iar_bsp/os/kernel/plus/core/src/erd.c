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
*   FILE NAME
*
*       erd.c
*
*   COMPONENT
*
*       ER - Error Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       Error Management component.
*
*   DATA STRUCTURES
*
*       ERD_Error_Code                      Contains the system error
*                                           code
*       ERD_Assert_Count                    Contains the number of
*                                           failed assertions
*       ERD_Error_String                    Contains the ASCII system
*                                           error string
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                           System definitions
*       nu_kernel.h                         Kernel constants
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

/* ERD_Error_Code contains the system error code detected by the system.  */

INT             ERD_Error_Code;

#if (NU_ASSERT_ENABLE == NU_TRUE)

/* ERD_Assert_Count contains the number of detected failed assertions. */
UNSIGNED        ERD_Assert_Count;

#endif  /* NU_ASSERT_ENABLE == NU_TRUE */

#if (NU_ERROR_STRING == NU_TRUE)

/* ERD_Error_String is an ASCII string representation of the system error.  */
const CHAR     *ERD_Error_String;

#endif  /* NU_ERROR_STRING == NU_TRUE */
