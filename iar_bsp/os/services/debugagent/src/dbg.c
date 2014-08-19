/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
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
*       dbg.c                                         
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C main functions source code for the 
*       component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_cb       
*                                                                      
*   FUNCTIONS                                                            
*                                                                      
*       dbg_initialize
*                                        
*   DEPENDENCIES
*
*       dbg.h
*                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"

/***** Global variables. */

/* Debug service control block. */

DBG_CB          DBG_cb;

/* Debug service control block pointer. */

DBG_CB *        DBG_p_cb = NU_NULL;

/***** Local functions */

/* Function declarations */

static DBG_STATUS   dbg_initialize(VOID);

/* Function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       Initialize the Debug service.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       None
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicate the service is initialized.
*
*       DBG_STATUS_FAILED - Indicates that the service initialization
*                           failed.
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_initialize(VOID)
{
    DBG_STATUS          dbg_status;
    DBG_CB *            p_dbg;
    DBG_OS_INIT_PARAM   os_init_param; 

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    
    /* Get local pointer to the service control block. */
    
    p_dbg = &DBG_cb;

    /* Setup control block. */

    /* Initialize the OS component. */
    
    dbg_status = DBG_OS_Initialize(&os_init_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {      
        /* Initialize communication component. */
        
        dbg_status = DBG_COM_Initialize(&p_dbg -> com,
                                        (VOID *)p_dbg);    
            
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Indicate debug service is active by setting control block
           pointer. */
           
        DBG_p_cb = p_dbg;

    }

    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       nu_os_svcs_dbg_init
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function is called by the Nucleus OS run-level system to 
*       initialize or terminate the Debug Agent service.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       path - Path of the Nucleus OS registry entry for the Nucleus 
*              Agent.
*
*       startstop - Value that indicates if Nucleus Agent system should be
*                   started (1) or stopped (0).
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       NU_SUCCESS - Indicates successful operation.
*
*       NU_INVALID_OPERATION - Indicates operation failed.
*
*       <other> - Indicates (other) internal error occurred.
*                                                                      
*************************************************************************/
STATUS nu_os_svcs_dbg_init(CHAR *path, INT startstop)
{
    STATUS      nu_status;
    DBG_STATUS  dbg_status;

    /* Set initial function status. */
    
    nu_status = NU_SUCCESS;

    /* Determine how to proceed based on the control command. */

    switch (startstop)
    {
        case 0 :
        {
            /* ERROR: Debug Agent does not support shutdown. */
            
            /* ERROR RECOVERY: Report success and do nothing. */
            
            break;
        }
        
        case 1 :
        {
            /* Initialize the Debug Agent system. */
            
            dbg_status = dbg_initialize();
            
            if (dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to initialize the Debug Agent */
                
                nu_status = NU_INVALID_OPERATION;
                
            }
            
            break;
        }
        
        default :
        {
            /* ERROR: Unknown control command value. */
            
            /* ERROR RECOVERY: Report success and do nothing. */
            
            break;
        }

    }

    return (nu_status);

}

