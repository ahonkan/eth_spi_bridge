/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/***************************************************************************
*                                                                          
* FILENAME                                                                 
*                                                                          
*       wpw_list.c                                              
*                                                                          
* COMPONENT
*
*       Nucleus WebServ
*
* DESCRIPTION                                                              
*                                                                          
*       This file contains the Nucleus WebServ user_id password            
*       initialization table for the server authentication schemes.                    
*                                                                          
* DATA STRUCTURES                                                          
*                                                                          
*       webpwTable              user_id password table
*                                                                          
* FUNCTIONS                                                                
*                                                                          
*       None
*                                                                          
* DEPENDENCIES                                                             
*           
*       None                            
*                                                                          
****************************************************************************/

#include "networking/nu_websr.h"    
#include "networking/wpw_auth.h"

/*  This table is in the format of {User_ID, Password} */
const struct WPW_AUTH_NODE WPW_Table[] =
{
    {"john",  "doe"},
    {"fred",  "fish"},
    {"willy", "wonka"},
    {"\0",    "\0"}
};
