/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME                                              
*
*       ftp_zc_extr.h                                  
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package
*
*   DESCRIPTION
*
*       This file contains the function prototypes for the routines to
*       read data from and write data to Zero Copy buffers.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#ifndef FTP_ZC_EXTR
#define FTP_ZC_EXTR

#include "networking/nu_net.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT     FTP_ZC_Write_Data(NET_BUFFER *, INT);
INT     FTP_ZC_Read_Data(NET_BUFFER **, UINT32, INT, INT);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* FTP_ZC_EXTR */
