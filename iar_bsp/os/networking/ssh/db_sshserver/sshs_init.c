/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*     sshs_init.c
*
* COMPONENT
*
*     Nucleus SSH Server
*
* DESCRIPTION
*
*     This file holds the initialization routine of SSH server.
*     Once the SSH server gets started it will strat listening on SSH port
*     for incoming requests.
*
* DATA STRUCTURES
*
*     None
*
* FUNCTIONS
*
*     nu_os_net_ssh_init
*
* DEPENDENCIES
*
*     nucleus.h
*     nucleus_gen_cfg.h
*     networking/nu_networking.h
*
************************************************************************/
/* Includes */
#include "nucleus.h"
#include "networking/nu_networking.h"
#include "nussh_includes.h"

extern STATUS NUSSH_Start(VOID);
/******************************************************************************
*
* FUNCTION
*
*      nu_os_net_ssh_db_sshserver_init
*
* DESCRIPTION
*
*      Initializes the tasks, queues and events used by the demonstration
*      application.
*
* INPUTS
*
*      key - registry key for component specific settings (Unused).
*
*      startstop - value to specify whether to start (NU_START)
*                  or stop (NU_STOP) a given component.
*
* OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*
******************************************************************************/
STATUS nu_os_net_ssh_db_sshserver_init (const CHAR * key, INT startstop)
{


    STATUS         status;

    /* Remove compiler warnings */
    UNUSED_PARAMETER(key);

    switch (startstop)
    {
        case RUNLEVEL_STOP :
        {
            status = NUSSH_Stop();
            break;
        }
        case RUNLEVEL_START :
        {
            status = NUSSH_Start();
            break;
        }
        case RUNLEVEL_HIBERNATE :
        case RUNLEVEL_RESUME :
        default :
        {
            status = -1;
            break;
        }
    }

    return status;

}
