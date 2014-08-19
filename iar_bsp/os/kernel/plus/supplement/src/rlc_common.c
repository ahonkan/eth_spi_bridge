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
*       rlc_common.c
*
*   COMPONENT
*
*       RL - Release Information
*
*   DESCRIPTION
*
*       This file contains the common core routines for the Release
*       Information component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Get_Release_Version              Return release major and
*                                           minor version numbers
*       RLC_Initialize                      Release Information
*                                           initialization
*
*   DEPENDENCIES
*
*       nucleus.h                           System definitions
*
***********************************************************************/
#include        "nucleus.h"

/* Define external inner-component global data references.  */

extern const UINT   RLD_Major_Version;
extern const UINT   RLD_Minor_Version;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Get_Release_Version
*
*   DESCRIPTION
*
*       This function sets the release major and minor version numbers
*       The version numbers identify the current version of Nucleus PLUS.
*
*   CALLED BY
*
*       RLC_Initialize                      Release information
*                                           initialization
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       major                           Major version number
*       minor                           Minor version number
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Get_Release_Version(UINT* major, UINT* minor)
{
    /* Reference the release information global data to ensure that
       it is available to debuggers and kernel awareness */
    *major = RLD_Major_Version;
    *minor = RLD_Minor_Version;
}


/***********************************************************************
*
*   FUNCTION
*
*       RLC_Initialize
*
*   DESCRIPTION
*
*       This function initializes the global data structures that
*       contain the Nucleus PLUS release information.
*
*   CALLED BY
*
*       INC_Initialize                     The main initialization function
*                                          of the system.  All components are
*                                          initialized by this function.
*
*   CALLS
*
*       NU_Get_Release_Version              Get release version
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  RLC_Initialize(VOID)
{
    UINT major;
    UINT minor;

    /* Reference the release information global data to ensure that
       it is available to debuggers and kernel awareness */
    NU_Get_Release_Version(&major, &minor);
}
