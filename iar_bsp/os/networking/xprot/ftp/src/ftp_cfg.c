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
*       ftp_cfg.c                                      
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP client/server
*       configuration.
*
*   DESCRIPTION
*
*       This file contains configuration data for the FTP client and
*       FTP server. No functions are contained in this file. Only data
*       structures to be customized by the application developer are
*       included. See FTP_CFG.H for related definitions.
*
*   DATA STRUCTURES
*
*       FTP_Password_List[]    A sample list of passwords for simple user
*                              authentication.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       externs.h
*       ftps_def.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/ftps_def.h"

/* A sample list of passwords for simple user authentication. Edit this
   list to add users to this server. Note: No dynamic user administration
   functions are available, but can be added by the application developer. */
FTPSACCT FTP_Password_List[] =
{
    {"jon",  "doe"},
    {"fred", "fish"},
    {"joe",  "blow"},
    {"\0",   "\0"}
};



