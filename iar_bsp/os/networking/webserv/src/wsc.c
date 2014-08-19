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
*       wsc.c                                                           
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Nucleus WebServ                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file contains functions for run-time configuration.        
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       WSC_Set_Use_Hostname    Set configuration variable to determine 
*                               whether to use use hostname addressing  
*                               instead of use IP literal addressing in 
*                               building a URL.                         
*                                                                       
*       WSC_Get_Use_Hostname    Get the value of the configuration      
*                               variable that determines whether to use 
*                               hostname addressing instead of using IP 
*                               literal addressing in building a URL.   
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h                                                     
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"


/* Runtime configuration variables */
extern UINT32     WSC_Use_Hostname;
extern NU_PROTECT WS_Protect;


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WSC_Set_Use_Hostname                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Set configuration variable to determine whether to use use      
*       hostname addressing instead of use IP literal addressing in     
*       building a URL.                                                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       setting                 value to set. NU_TRUE or NU_FALSE.      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS                                                      
*                                                                       
*************************************************************************/

STATUS WSC_Set_Use_Hostname(UINT32 setting)
{
    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();


    /* Make sure we have either NU_TRUE or NU_FALSE. */
    if (setting != NU_TRUE)
    {
        setting = NU_FALSE;
    }

    /* Arbitrate access to this global configuration variable */
    NU_Protect(&WS_Protect);

    /* Set this configuration variable to NU_TRUE or NU_FALSE. */
    WSC_Use_Hostname = setting;

    /* release the protection */
    NU_Unprotect();


    NU_USER_MODE();

    return (NU_SUCCESS);
}


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WSC_Get_Use_Hostname                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Get the vallue of the configuration variable which is used to   
*       determine whether to use use  hostname addressing or IP literal 
*       addressing in building a URL.                                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       setting_p               Pointer to the variable that will be set
*                               to the value of the global configura-   
*                               tion variable. The variable pointed to  
*                               will be set to NU_TRUE or NU_FALSE.     
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful completion of the service.   
*       NU_INVALID_PARM         The input parameter is invalid.         
*                                                                       
*************************************************************************/

STATUS WSC_Get_Use_Hostname(UINT32 *setting_p)
{
    STATUS status;

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES


    /* Make sure we have either NU_TRUE or NU_FALSE. */
    if (setting_p != NU_NULL)
    {
        /* Switch to supervisor mode. */
        NU_SUPERVISOR_MODE();

        /* Arbitrate access to this global configuration variable */
        NU_Protect(&WS_Protect);
    
        /* Get the setting of this configuration variable. */
        *setting_p = WSC_Use_Hostname;
    
        /* release the protection */
        NU_Unprotect();
        
        status = NU_SUCCESS;

        NU_USER_MODE();

    }
    else
        status = NU_INVALID_PARM;
 
    return (status);
}
