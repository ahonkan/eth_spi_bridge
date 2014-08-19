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

/*************************************************************************
* FILE NAME
*
*        mib2_icmp.c
*
* COMPONENT
*
*        MIB II - ICMP Group.
*
* DESCRIPTION
*
*        This file contain the function(s) and data structures that
*        provides the ICMP statistics incorporation of macros defined in
*        mib2.h
*
* DATA STRUCTURES
*
*        Mib2_Icmp_Data
*
* FUNCTIONS
*
*        None
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

/*-----------------------------------------------------------------------
 * Global Definitions
 *----------------------------------------------------------------------*/

#if (MIB2_ICMP_INCLUDE == NU_TRUE)

MIB2_ICMP_STRUCT             Mib2_Icmp_Data;

#endif /* (MIB2_ICMP_INCLUDE == NU_TRUE) */


#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */
