/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*  	FILE NAME
*
*       cxx_rte.c
*
*   COMPONENT
*
*		C++ Service - Run-Time Environment (RTE)
*
*   DESCRIPTION
*
*		Run-Time Environment (RTE) base functionality.
*
*  	DATA STRUCTURES
*
*      	None
*
*   FUNCTIONS
*
*       CXX_RTE_Initialize_Module_Objects
*       CXX_RTE_Initialize
*       nu_os_svcs_cxx_init
*
*   DEPENDENCIES
*
*       nuclues.h
*       nu_kernel.h
*       cxx_rte.h
*       esal.h
*       proc_extern.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/cxx_rte.h"
#include "os/kernel/plus/core/inc/esal.h"

#if (CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS == NU_TRUE)

#include "kernel/proc_extern.h"

#endif /* CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS */

/*************************************************************************
*
* 	FUNCTION
*
*      	CXX_RTE_Initialize
*
*  	DESCRIPTION
*
*      	Allocates memory.
*
*  	INPUTS
*
*		None
*
*  	OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
STATUS CXX_RTE_Initialize()
{
#if (CFG_NU_OS_SVCS_CXX_INIT_STATIC_OBJECTS == NU_TRUE)

    /* Call tool-set specific function to initialize C++ static
       objects for system. */
    ESAL_GE_RTE_Cxx_System_Objects_Initialize();

#endif /* (CFG_NU_OS_SVCS_CXX_INIT_STATIC_OBJECTS == NU_TRUE) */

#if (CFG_NU_OS_SVCS_CXX_INIT_EXCEPTION_SUPPORT == NU_TRUE)

    /* Call tool-set specific function to initialize C++ exception
       handling. */
    ESAL_GE_RTE_Cxx_Exceptions_Initialize();

#endif /* (CFG_NU_OS_SVCS_CXX_INIT_EXCEPTIONS == NU_TRUE) */

    return (NU_SUCCESS);
}

/*************************************************************************
*
* 	FUNCTION
*
*   	nu_os_svcs_cxx_init
*
*   DESCRIPTION
*
*       Entry point from the kernel to initialize C++ services.
*
*   INPUTS
*
*       key - String indicating the registry key value assigned to
*             service.
*
*       startstop - Control value indicating whether service should start
*                   or stop execution.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
STATUS nu_os_svcs_cxx_init(const CHAR *key, INT startstop)
{
	STATUS	status = NU_SUCCESS;

#if (CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.svcs.cxx */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_SVCS_CXX);

#endif /* CFG_NU_OS_SVCS_CXX_EXPORT_SYMBOLS */

    /* Determine how to proceed based on the control command. */
    switch (startstop)
    {
        case 0 :
        {
            /* ERROR: Service does not support shutdown. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }

        case 1 :
        {
            /* Initialize the C++ service. */
        	CXX_RTE_Initialize();

            break;
        }

        default :
        {
            /* ERROR: Unknown control command value. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }

    }

    return (status);
}
