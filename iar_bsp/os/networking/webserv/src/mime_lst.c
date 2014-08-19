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

/************************************************************************
*                                                                       
* FILE NAME                                                             
*                                                                       
*       mime_lst.c 
*                                                                       
* COMPONENT                                                             
*      
*       Nucleus WebServ                                                                 
*                                                                       
* DESCRIPTION                                                           
*                 
*       This file holds the table of mime extensions and their
*       associated MIME header descriptions.  These are placed into
*       the outgoing HTTP header for the browser to translate.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       HTTP_Mime_Table         Holds the mime extensions.       
*                                                                
* FUNCTIONS                                                      
*                                                                
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h                     
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"


/* The mime table maps file types to mime extensions.
 * if the file type does not require a header sent with
 * the reply, NU_NULL is associated with it.
 */

const WS_MIME_TABLE MIME_Mime_Table[]=
{
    {"txt",  "text/plain"},
    {"text", "text/plain"},
    {"html", "text/html"},
    {"htm",  "text/html"},
    {"gif",  "image/gif"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png",  "image/png"},
    {"css",  "text/css"},
    {"jar",  "application/octet-stream"},
    {NU_NULL, NU_NULL}
};

