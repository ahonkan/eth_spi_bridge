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
*       This file contains function prototypes of all functions
*       accessible to other components.
*
***********************************************************************/

/* Check to see if the file has been included already */
#ifndef     ERROR_MANAGEMENT_H
#define     ERROR_MANAGEMENT_H

#ifdef          __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* System error handling function definition.  */
VOID        ERC_System_Error(INT error_code);

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif  /* !ERROR_MANAGEMENT_H */
