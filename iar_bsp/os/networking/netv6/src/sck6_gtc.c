/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       sck6_gtc.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Getsockopt_IPV6_TCLASS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IPV6_TCLASS
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Getsockopt_IPV6_TCLASS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the value of the traffic class Sticky 
*       Option that was previously set by the application.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *t_class                A pointer to the traffic class sticky 
*                               option previously set
*       *length                 The length of the pointer
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_PARM         Either t_class or length are invalid.
*                                                                         
*************************************************************************/
STATUS NU_Getsockopt_IPV6_TCLASS(INT socketd, INT *t_class, INT *length)
{
    STATUS  status;

    if ( (t_class == NU_NULL) || (*length < (INT)sizeof(INT)) )
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Get_IPV6_TCLASS(socketd, t_class, length);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Getsockopt_IPV6_TCLASS */
