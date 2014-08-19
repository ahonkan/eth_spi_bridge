/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       pms_hibernate_exit_op.c
*
*   COMPONENT
*
*       Hibernate
*
*   DESCRIPTION
*
*       Contains all functionality for setting and getting the exit
*       hibernate operating point
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Set_Hibernate_Exit_OP
*       NU_PM_Get_Hibernate_Exit_OP
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       power_core.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern UINT8 PM_DVFS_OP_Count;

/* Hibernate Exit OP - The operating point that will be set when the
   system is resumed from a hibernate level. */
static UINT8 PM_Hibernate_Exit_OP;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Set_Hibernate_Exit_OP
*
*   DESCRIPTION
*
*       This function sets the operating point value that will be
*       transitioned to when the system exits a hibernate level.
*
*   INPUT
*
*       op_id - This is the desired new operating point id
*
*   OUTPUT
*
*       NU_SUCCESS - Indicates successful operation
*
*       PM_INVALID_OP_ID - Indicates an invalid operating point value was
*                          specified.
*
*************************************************************************/
STATUS NU_PM_Set_Hibernate_Exit_OP (UINT8 op_id)
{
    STATUS status = NU_SUCCESS;

    /* Ensure the OP value requested is valid */
    if (op_id >= PM_DVFS_OP_Count)
    {
        status = PM_INVALID_OP_ID;
    }
    else
    {
        /* Set the requested OP */
        PM_Hibernate_Exit_OP = op_id;
    }
    
    /* Trace log */
    T_HIB_EXIT_OP(op_id, status);
    
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_Hibernate_Exit_OP
*
*   DESCRIPTION
*
*       This function gets the operating point value that will be
*       transitioned to when the system exits a hibernate level.
*
*
*   INPUT
*
*       op_id_ptr - Return pointer that will be updated to contain the
*                   current operating point value that will be
*                   transitioned to when the system exits a hibernate
*                   level.
*
*   OUTPUT
*
*       NU_SUCCESS - Indicates successful operation.
*
*       PM_INVALID_POINTER - Indicates an invalid pointer has been passed
*                            as a parameter.
*
*************************************************************************/
STATUS NU_PM_Get_Hibernate_Exit_OP (UINT8 *op_id_ptr)
{
    STATUS status = NU_SUCCESS;

    /* Verify the pointer is valid */
    if (op_id_ptr == NU_NULL)
    {
        status = PM_INVALID_POINTER;
    }
    else
    {
        /* Set the pointer to the operating point set */
        *op_id_ptr = PM_Hibernate_Exit_OP;
    }

    return (status);
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */
