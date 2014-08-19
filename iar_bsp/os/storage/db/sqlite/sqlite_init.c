/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
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
*     sqlite_init.c
*
* COMPONENT
*
*     Nucleus SQLite
*
* DESCRIPTION
*
*     This file holds the nucleus runlevel initatilization logic for SQLite.
*
* DATA STRUCTURES
*
*     None
*
* FUNCTIONS
*
*     nu_os_stor_db_sqlite_init
*
* DEPENDENCIES
*
*     nucleus.h
*     nu_kernel.h
*     nu_services.h
*     sqliteInt.h
*
************************************************************************/
/* Includes */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "sqliteInt.h"

/******************************************************************************
*
* FUNCTION
*
*      nu_os_stor_db_sqlite_init
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
STATUS nu_os_stor_db_sqlite_init (const CHAR * key, INT startstop)
{
    STATUS status = NU_SUCCESS;

    if(startstop == RUNLEVEL_START)
    {
        /* Initialize Nucleus port of SQLite. */
        status = sqlite3_initialize();

    }
    else if(startstop == RUNLEVEL_STOP)
    {
        /* Shutdown SQLite. */
        status = sqlite3_shutdown();
    }

    return (status == SQLITE_OK ? NU_SUCCESS : -1);
}

