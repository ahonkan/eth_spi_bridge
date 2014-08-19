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
*       rld.c
*
*   COMPONENT
*
*       RL - Release Information
*
*   DESCRIPTION
*
*       This file contains information about this release of Nucleus
*       PLUS.
*
*   DATA STRUCTURES
*
*       RLD_Major_Version                   Release major version number
*       RLD_Minor_Version                   Release minor version number
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                           System definitions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

/* RLD Version numbers */
const UINT  RLD_Major_Version = NU_PLUS_RELEASE_MAJOR_VERSION;
const UINT  RLD_Minor_Version = NU_PLUS_RELEASE_MINOR_VERSION;
