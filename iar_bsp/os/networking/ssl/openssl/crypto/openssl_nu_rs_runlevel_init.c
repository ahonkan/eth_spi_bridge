/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2011
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
*      openssl_nu_rs_runlevel_init.c
*
* COMPONENT
*
*      Nucleus SSL
*
* DESCRIPTION
*
*      This file contains Nucleus RS specific run-level initialization
*      functions for OpenSSL.
*
* DATA STRUCTURES
*                                                                      
*      None.
*                                                                      
* FUNCTIONS                                                            
*                                                                      
*      nu_os_net_ssl_openssl_crypto_init    Run-level initialization routine.
*      nu_os_net_ssl_openssl_ssl_init       Run-level initialization routine.
*
* DEPENDENCIES
*
*      o_nucleus.h
*                                                                      
************************************************************************/

/************************************************************************
* FUNCTION
*
*       nu_os_net_ssl_openssl_crypto_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus port of OpenSLL.
*       It is the entry point of the product initialization
*       sequence.
*
* INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       NU_NO_MEMORY            Memory not available.
*       <other> -               Indicates (other) internal error occured.
*
************************************************************************/

#include "o_nucleus.h"
#include "services/runlevel_init.h"

STATUS nu_os_net_ssl_openssl_crypto_init (const CHAR* path, INT startstop)
{
    STATUS status = NU_SUCCESS;

    if(startstop == RUNLEVEL_START)
    {
        /* Initialize Nucleus port of OpenSSL. */
        status = cos_program_init();

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error at call to cos_program_init().\n",
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else if(startstop == RUNLEVEL_STOP)
    {
        /* Shutdown OpenSSL. */
        cos_program_deinit();

        /* Return success */
        status = NU_SUCCESS;
    }

    return (status);
} /* nu_os_net_ssl_openssl_crypto_init */

/************************************************************************
* FUNCTION
*
*       nu_os_net_ssl_openssl_ssl_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus port of OpenSLL.
*       It is the entry point of the product initialization
*       sequence.
*
* INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
* OUTPUTS
*
*       NU_SUCCESS              Always, being a stub function.
*
************************************************************************/
STATUS nu_os_net_ssl_openssl_ssl_init (const CHAR* path, INT startstop)
{
	/* Just a stub function. */

	return (NU_SUCCESS);
} /* nu_os_net_ssl_openssl_ssl_init */
