/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_ext.c
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*       This file contains implementations of NU_USB services.
*
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*   NU_USB_Delete         - Deletes the control block.
*   NU_USB_Get_Name       - Returns the null terminated string of the
*                           control block.
*   NU_USB_Get_Object_Id  - Returns the object id of the control block.
*   _NU_USB_Create        - Creates a new NU_USB object, assigns a unique
*                           id to it and add the node to subsystem.
*   _NU_USB_Delete        - Deletes an object, removes the node and releases
*                           the unique id associated.
*   _NU_USB_Get_Name      - Returns name of the object.
*   _NU_USB_Get_Object_Id - Returns Id assigned to an object.
*
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_EXT_C
#define USB_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_Delete
*
* DESCRIPTION
*    This function calls the function pointer installed in the dispatch
* table. So any derivatives of NU_USB control block can be deleted
* using this function.
*
* INPUTS
*   cb   Pointer to the control block. can be any derivative of NU_USB.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that the control block to be deleted is
*                       invalid.
*
*************************************************************************/
STATUS NU_USB_Delete (VOID *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB *) cb)->usb_dispatch->Delete (cb);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_Get_Name
*
* DESCRIPTION
*    This function invokes the Get_Name function from the dispatch table.
*
* INPUTS
*   cb       Pointer to NU_USB or its derivatives control block.
*   name_out Pointer to array to hold the null terminated 7 CHAR string.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_Get_Name (NU_USB * cb,
                        CHAR * name_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name_out);
    NU_USB_PTRCHK_RETURN(cb->usb_dispatch);

    status = cb->usb_dispatch->Get_Name (cb, name_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_Get_Object_Id
*
* DESCRIPTION
*    This function invokes the Get_Object_Id function from the dispatch table.
*
* INPUTS
*   cb       Pointer to NU_USB or its derivatives control block.
*   name_out Pointer to variable to hold the object id.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_Get_Object_Id (NU_USB * cb,
                             UINT32 *id_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->usb_dispatch);
    NU_USB_PTRCHK_RETURN(id_out);

    status = cb->usb_dispatch->Get_Object_Id (cb, id_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_Create
*
* DESCRIPTION
*   This function creates NU_USB portion of the control block. The control
* block can be of type NU_USB or any derivative of it. It allocates an Id
* for each newly created control block.
*
* INPUTS
*   cb        - Pointer to NU_USB control block or any derivative of it.
*   subsys    - Pointer to the sub system control block, that it belongs
*               to.
*   name      - 7 character null terminated string naming the control
*               block.
*   dispatch  - Pointer to the dispatch table for the control block.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*    NU_USB_NOT_PRESENT Indicates that the maximum number of control blocks
*                       that could be created in the sub system has
*                       exceeded.
*
*************************************************************************/
STATUS _NU_USB_Create (NU_USB * cb,
                       NU_USB_SUBSYS * subsys,
                       CHAR * name,
                       const VOID *dispatch)
{
    STATUS status = NU_SUCCESS;
    INT32  i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(subsys);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_PTRCHK_RETURN(dispatch);

    /* Setup the data structure. */
    cb->usb_dispatch = dispatch;

    /* Fill in the name.
       Copy until the end of the string (NULL) or up to NU_MAX_NAME - 1.
       This ensures the last character will be NULL terminated. */
    for (i = 0; (i < (NU_MAX_NAME - 1)) && (name[i] != NU_NULL); i++)
    {
         cb->name[i] =  name[i];
    }

    /* Set last character in string to NULL */
    cb->name[i] = NU_NULL;
    cb->subsys = subsys;

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    /* Allocate an object Id */
    status = NU_USB_SUBSYS_Allocate_Id (subsys, &(cb->object_id));

    if (status == NU_SUCCESS)
        status = NU_USB_SUBSYS_Add_Node (subsys, cb);
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_Delete
*
* DESCRIPTION
*   This function releases the object Id thats assigned to the control
* block.
*
* INPUTS
*    cb   Pointer to NU_USB control block or its derivative.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS _NU_USB_Delete (VOID *cb)
{
    STATUS status = NU_SUCCESS;
#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    NU_USB *usb;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    usb = (NU_USB *) cb;

    /* Release the Id */
    status = NU_USB_SUBSYS_Release_Id (usb->subsys, usb->object_id);

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Remove_Node (usb->subsys, usb);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_Get_Name
*
* DESCRIPTION
*    Returns the 7 character null terminated string associated with the
* control block.
*
* INPUTS
*   cb       Pointer to NU_USB or its derivatives control block.
*   name_out Pointer to array to hold the null terminated 7 CHAR string.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS _NU_USB_Get_Name (NU_USB * cb,
                         CHAR * name_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name_out);

    strncpy (name_out, ((NU_USB *) cb)->name, NU_MAX_NAME);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_Get_Object_Id
*
* DESCRIPTION
*    Returns the object id of the control block.
*
* INPUTS
*   cb     Pointer to NU_USB control block or its derivative.
*   id_out Pointer to the variable to hold the value of the object id.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS _NU_USB_Get_Object_Id (NU_USB * cb,
                              UINT32 *id_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(id_out);

    *id_out = cb->object_id;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_EXT_C */
/*************************** end of file ********************************/
