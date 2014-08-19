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
*        nu_usb_subsys_ext.c
*
* COMPONENT
*       Nucleus USB Software : subsystem component
*
* DESCRIPTION
*       This file contains the implementation of exported functions of
*       Nucleus USB software's subsystem component.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USB_SUBSYS_Add_Node          -adds a node to subsystem
*       NU_USB_SUBSYS_Allocate_Id       -allocate a unique id to subsystem
*                                       object.
*       NU_USB_SUBSYS_Delete            -deletes the instance of subsystem
*       NU_USB_SUBSYS_Established_Nodes -returns number of nodes
*       NU_USB_SUBSYS_Is_Valid_Object   -Validate an object in subsystem
*       NU_USB_SUBSYS_Lock              -Grabs the subsystem lock
*       NU_USB_SUBSYS_Node_Pointers     -returns list of nodes in subsystem
*       NU_USB_SUBSYS_Release_Id        -releases the id of an object.
*       NU_USB_SUBSYS_Remove_Node       -remove a node from subsystem.
*       NU_USB_SUBSYS_Set_Signature     -set a signature of subsystem type.
*       NU_USB_SUBSYS_Unlock            -releases the subsystem lock.
*       _NU_USB_SUBSYS_Create           -Creates an instance of subsystem.
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_SUBSYS_EXT_C
#define    USB_SUBSYS_EXT_C

/* ======================  Include Files  ============================= */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/*************************************************************************
* FUNCTION
*     _NU_USB_SUBSYS_Create
*
* DESCRIPTION
*     This function is an protected interface which initializes the
*     subsystem. This is meant to be used by subsystem extenders.
*
* INPUTS
*     cb          pointer to subsystem control block.
*     dispatch    dispatch table which initializes the virtual functions
*                 of subsystem.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS _NU_USB_SUBSYS_Create (NU_USB_SUBSYS * cb,
                              const VOID *dispatch)
{
    UINT8 i;
    NU_USB_SUBSYS *subsys;

    subsys = (NU_USB_SUBSYS *) cb;

    subsys->dispatch = (NU_USB_SUB_SYSTEM_DISPATCH *) dispatch;
    subsys->list_head = NU_NULL;
    subsys->signature = 0;
    subsys->last_allocated_id = 0;

    for (i = 0; i < (NU_USB_MAX_IDS / 8 + 1); i++)
        subsys->id_map[i] = 0;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Delete
*
* DESCRIPTION
*     This function deletes a specified subsystem.
*
* INPUTS
*     cb          pointer to subsystem control block.
*
* OUTPUTS
*     NU_SUCCESS  Subsystem Deleted successfully
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Delete (NU_USB_SUBSYS * cb)
{
    if ((cb) && (cb->dispatch) && (cb->dispatch->Delete))
        return (((NU_USB_SUBSYS *) cb)->dispatch->Delete (cb));
    else
        return (NU_USB_INVLD_ARG);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Add_Node
*
* DESCRIPTION
*     This function adds a node to a subsystem container. It places the
*     object on the linked list maintained by the container. This linked
*     list will be used for validating the objects by the container.
*
* INPUTS
*     cb          pointer to the Subsystem control block.
*     node        pointer to the node to be added
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Add_Node (NU_USB_SUBSYS * cb,
                               VOID *node)
{
    NU_Place_On_List ((CS_NODE **) & (((NU_USB_SUBSYS *) cb)->list_head),
                                       (CS_NODE *) node);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Remove_Node
*
* DESCRIPTION
*     This function deletes a node from a subsystem container. It
*     removes the object from the linked list maintained by the container.
*     Once deleted, the validate object always returns failure for this
*     object and for this container.
*
* INPUTS
*     cb          pointer to the Subsystem control block.
*     node        pointer to the node to be removed.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Remove_Node (NU_USB_SUBSYS * cb,
                                  VOID *node)
{
    NU_Remove_From_List ((CS_NODE **) & (((NU_USB_SUBSYS *) cb)->list_head),
                         (CS_NODE *) node);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Established_Nodes
*
* DESCRIPTION
*     This function returns the number of established nodes in the given
*     subsystem container.
*
* INPUTS
*     cb          pointer to subsystem control block.
*
* OUTPUTS
*     UNSIGNED    number of established nodes.
*
*************************************************************************/
UNSIGNED NU_USB_SUBSYS_Established_Nodes (NU_USB_SUBSYS * cb)
{
    CS_NODE *node, *head;
    UINT32 pointers;

    /* Initialize the number of pointers returned.  */
    pointers = 0;

    node = cb->list_head;
    head = cb->list_head;

    /* Loop until all node pointers are in the list or until the maximum
       list size is reached.  */
    while (node)
    {
        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node = node->cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node == head)
            /* The list search is complete.  */
            node = NU_NULL;
    }

    /* Return the number of pointers in the list.  */
    return (pointers);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Node_Pointers
*
* DESCRIPTION
*     This function retrieves and fills 'pointer_list' with 'max_nodes'
*     number of nodes from the subsystem container. If the value
*     'max_nodes' is greater than the number of nodes contained,
*     then only available nodes are copied on to the 'pointer_list'.
*     The number of nodes copied are returned as the function return value.
*
* INPUTS
*     cb              pointer to subsystem control block
*     pointer_list    pointer to the list area.
*     max_nodes       maximum number of nodes required.
*
* OUTPUTS
*     UNSIGNED        number of nodes returned.
*
*************************************************************************/
UNSIGNED NU_USB_SUBSYS_Node_Pointers (NU_USB_SUBSYS * cb,
                                      VOID **pointer_list,
                                      UNSIGNED max_nodes)
{
    CS_NODE *node, *head;
    UINT32 pointers;

    /* Initialize the number of pointers returned.  */
    pointers = 0;

    node = cb->list_head;
    head = cb->list_head;

    /* Loop until all node pointers are in the list or until the maximum
       list size is reached.  */
    while ((node) && (pointers < max_nodes))
    {
        /* Place the node into the destination list.  */
        *pointer_list++ = (VOID *)node;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node = node->cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node == head)
            /* The list search is complete.  */
            node = NU_NULL;
    }

    /* Return the number of pointers in the list.  */
    return (pointers);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Allocate_Id
*
* DESCRIPTION
*     This function prepares an object for being a node in a given
*     subsystem container. It
*     1. assigns an id for the object .
*     2. Sets the object signature corresponding to the subsystem
*     3. Adds to the container the new object.
*
* INPUTS
*     cb              pointer to subsystem control block
*     new_object_id   pointer to a variable to hold returned new id.
*
* OUTPUTS
*     NU_SUCCESS      Indicates successful completion.
*     NU_NOT_PRESENT  Indicates no more Id's can be allocated by the subsystem.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Allocate_Id (NU_USB_SUBSYS * cb,
                                  UINT32 *new_object_id)
{
    UINT8  id, last_id;
    UINT32 object_id;
    STATUS status;

    status = NU_USB_SUBSYS_Lock (cb);

    if (status == NU_SUCCESS)
    {
        /* search from the beginning if reached to the max */
        if(cb->last_allocated_id == NU_USB_MAX_IDS)
            last_id = 0;
        else
            last_id = cb->last_allocated_id;

        /* Allocate an Id from the bit map */
        id = usb_get_id (((NU_USB_SUBSYS *) cb)->id_map,
                         (NU_USB_MAX_IDS / 8 + 1), last_id);

        /* If all Ids are allocated, return failure */
        /* Otherwise allocate the id */
        if (id != USB_NO_ID)
        {
            /* Add sub system's signature to the assigned id to make it
             * distinguishable from objects of other sub system's.
             */
            object_id = 0;
            object_id |= id;
            object_id |= (((NU_USB_SUBSYS *) cb)->signature) << 16;
            *new_object_id = object_id;

            /* Save the new id thats allocated */
            ((NU_USB_SUBSYS *) cb)->last_allocated_id = id;
        }
        else
        {
            status = NU_NOT_PRESENT;
        }

        status |= NU_USB_SUBSYS_Unlock (cb);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Release_Id
*
* DESCRIPTION
*     This function prepares an object for removing from a container. It
*     claims back the id that was assigned to the object previously.
*
* INPUTS
*     cb          pointer to subsystem control block.
*     object_id   id to be released.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Release_Id (NU_USB_SUBSYS * cb,
                                 UINT32 object_id)
{
    UINT8  id;
    STATUS status;

    status = NU_USB_SUBSYS_Lock (cb);

    if (status == NU_SUCCESS)
    {
        /* Deduct the sub system signature, to get the assigned id */
        id = object_id & 0xff;

        /* Release the id */
        usb_release_id (((NU_USB_SUBSYS *) cb)->id_map,
                        (NU_USB_MAX_IDS / 8 + 1), id);

        status |= NU_USB_SUBSYS_Unlock (cb);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Is_Valid_Object
*
* DESCRIPTION
*     This function checks if the supplied  object is valid and
*     known to the Stack software. If present, NU_SUCCESS is
*     returned by this function. Otherwise, NU_USB_INVLD_ARG is returned.
*
* INPUTS
*     cb          pointer to subsystem control block.
*     object      pointer to USB object control block to be verified.
*     reply       pointer to a variable to hold returned validity of object.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Is_Valid_Object (NU_USB_SUBSYS * cb,
                                      NU_USB * object,
                                      DATA_ELEMENT * reply)
{
    UINT32 object_id;
    UINT8  id;
    STATUS status;

    status = NU_USB_SUBSYS_Lock (cb);

    if (status == NU_SUCCESS)
    {
        /* Get object id */
        status = NU_USB_Get_Object_Id (object, &object_id);

        /* Deduct the sub system signature, to get the assigned id */
        id = object_id & 0xff;

        /* Check for the id */
        *reply = usb_is_id_set (((NU_USB_SUBSYS *) cb)->id_map,
                                (NU_USB_MAX_IDS / 8 + 1), id);
        status |= NU_USB_SUBSYS_Unlock (cb);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Set_Signature
*
* DESCRIPTION
*     This function inserts a cross check signature in the subsystem
*     object. This signature varies depending on the type of the
*     subsystem and is used for validating various objects.
*
* INPUTS
*     cb          pointer to subsystem control block
*     signature   signature to be inserted to the object subsystem.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Set_Signature (NU_USB_SUBSYS * cb,
                                    UINT16 signature)
{
    ((NU_USB_SUBSYS *) cb)->signature = signature;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Lock
*
* DESCRIPTION
*     This function is used to provide multi-threaded safety to critical
*     data structure. This function grabs the lock.
*
* INPUTS
*     cb          pointer to subsystem control block.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Lock (NU_USB_SUBSYS * cb)
{
    if ((cb) && (cb->dispatch) && (cb->dispatch->Lock))
        return (((NU_USB_SUBSYS *) cb)->dispatch->Lock (cb));
    else
        return (NU_USB_INVLD_ARG);
}

/*************************************************************************
* FUNCTION
*     NU_USB_SUBSYS_Unlock
*
* DESCRIPTION
*     This function is used to provide multi-threaded safety to critical
*     data structure. This function releases the lock.
*
* INPUTS
*     cb          pointer to subsystem control block.
*
* OUTPUTS
*     NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_SUBSYS_Unlock (NU_USB_SUBSYS * cb)
{
    if ((cb) && (cb->dispatch) && (cb->dispatch->Unlock))
        return (((NU_USB_SUBSYS *) cb)->dispatch->Unlock (cb));
    else
        return (NU_USB_INVLD_ARG);
}

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/*************************************************************************/

#endif /* USB_SUBSYS_EXT_C */
/*************************** end of file ********************************/

