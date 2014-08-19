/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*        mib2_ifs.c
*
* COMPONENT
*
*        MIB II - Interface Stack.
*
* DESCRIPTION
*
*        This files implementation of table that contains information on
*        the relationships between the multiple sub-layers of network
*        interfaces. In particular, it contains information on which
*        sub-layers run 'on top of' which other sub-layers, where each
*        sub-layer corresponds to a conceptual row in the ifTable. For
*        example, when the sub-layer with ifIndex value x runs over
*        the sub-layer with ifIndex value y, then this table
*        contains:
*
*           ifStackStatus.x.y=active
*
*        For each ifIndex value, 'I', which identifies an active
*        interface, there are always at least two instantiated rows
*        in this table associated with 'I'. For one of these rows, 'I'
*        is the value of ifStackHigherLayer; for the other, 'I' is the
*        value of ifStackLowerLayer. (If 'I' is not involved in
*        multiplexing, then these are the only two rows associated
*        with 'I'.)
*
*        For example, two rows exist even for an interface which has
*        no others stacked on top or below it:
*
*           ifStackStatus.0.x=active
*           ifStackStatus.x.0=active
*
* DATA STRUCTURES
*
*        MIB2_Interface_Stack_Root
*        MIB2_Temp_If_Stack
*        MIB2_If_Stack_Memory
*        MIB2_If_Stack_Used
*
* FUNCTIONS
*
*        MIB2_If_Stack_Get_Location
*        MIB2_If_Stack_Add_Entry
*        MIB2_If_Stack_Remove_Entry
*        MIB2_If_Stack_Remove_Dev
*        MIB2_IfStack_Get_Entry_Util
*        MIB2_If_Stack_Get_Entry
*        MIB2_If_Stack_Get_HI_Entry
*        MIB2_If_Stack_Get_LI_Entry
*        MIB2_If_Stack_Get_Next_Entry
*        MIB2_If_Stack_Get_Next_Entry2
*        MIB2_Get_IfStack_Util
*        MIB2_Get_IfStack
*        MIB2_Get_Next_IfStack
*        MIB2_Get_IfStack_Row_Status
*
* DEPENDENCIES
*
*        nu_net.h
*        sys.h
*        snmp.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_IF_STACK == NU_TRUE)

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/sys.h"
#include "networking/snmp.h"
#endif

/* If Stack Table */
MIB2_IF_STACK_ROOT          MIB2_Interface_Stack_Root;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for MIB2_IF_STACK_STRUCT structures */
MIB2_IF_STACK_STRUCT        MIB2_If_Stack_Memory[NET_MAX_IF_STACK_ENTRIES];

/* Array of flags when NU_TRUE represents the corresponding memory is not
   free. */
UINT8                       MIB2_If_Stack_Used[NET_MAX_IF_STACK_ENTRIES];

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

STATIC MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Location(UINT32 h_if_index,
                                                        UINT32 l_if_index);

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_Location
*
* DESCRIPTION
*
*       This function return the first stack entry that have either equal
*       or greater interface indexes passed in.
*
* INPUTS
*
*       h_if_index              Higher interface index.
*       l_if_index              Lower interface index.
*
* OUTPUTS
*
*       Pointer to first interface stack entry with equal or greater
*       interface indexes passed if one was found otherwise NU_NULL.
*
*************************************************************************/
STATIC MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Location(UINT32 h_if_index,
                                                        UINT32 l_if_index)
{
    /* Handle to interface stack. */
    MIB2_IF_STACK_STRUCT    *if_stack_entry;

    /* Handle to higher interface device. */
    DV_DEVICE_ENTRY         *h_dev;

    /* Boolean variable to represent whether we have found the required
       entry. */
    UINT32                  l_found;

    /* Getting handle to first interface stack entry in the list. */
    if_stack_entry = MIB2_Interface_Stack_Root.mib2_flink;

    /* If higher interface index passed in is not zero then skip all
       the entries having lesser higher interface index. */
    if (h_if_index != 0)
    {
        /* Skipping all the stack entries that have lesser higher
           interface index. */
        while (if_stack_entry)
        {
            /* If we have skipped all the stack entry with lesser
               higher interface index as passed in then break through
               the loop. */
            if ( (if_stack_entry->mib2_higher_layer) &&
                 (if_stack_entry->mib2_higher_layer->dev_index >=
                    (h_if_index - 1) ) )
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            if_stack_entry = if_stack_entry->mib2_flink;
        }
    }

    /* If lower interface index is not zero then skip all the
       entries with equal higher interface index and lesser lower
       interface index. */
    if(l_if_index != 0)
    {
        /* If current stack entry is not NU_NULL. */
        if (if_stack_entry != NU_NULL)
        {
            /* Update found flag to NU_TRUE if we have found the stack
               entry to be returned. */
            l_found = ( (h_if_index == 0) &&
                        (if_stack_entry->mib2_higher_layer) );

            /* Update found flag to NU_TRUE if we have found the stack
               entry to be returned. */
            l_found = ( l_found ||
                        ( (h_if_index != 0) &&
                          (if_stack_entry->mib2_higher_layer->dev_index >
                                (h_if_index - 1)) ) );

            /* If we have not found the stack entry to be returned then
               search the appropriate stack entry to return. */
            if ( !(l_found) )
            {
                /* Getting higher layer of current stack entry. */
                h_dev = if_stack_entry->mib2_higher_layer;

                /* Skipping all the entries with equal higher interface
                   index and lesser lower interface index. */
                while (if_stack_entry)
                {
                    /* If we have reached at a stack entry with
                       greater higher interface index then break
                       through the loop. */
                    if (if_stack_entry->mib2_higher_layer != h_dev)
                    {
                        /* Breaking through the loop. */
                        break;
                    }

                    /* If we have skipped all  stack entry with lesser
                       lower interface index as passed in. */
                    if ( (if_stack_entry->mib2_lower_layer) &&
                         (if_stack_entry->mib2_lower_layer->dev_index >=
                            (l_if_index - 1) ) )
                    {
                        /* Breaking through the loop. */
                        break;
                    }

                    /* Moving forward in the list. */
                    if_stack_entry = if_stack_entry->mib2_flink;
                }
            }
        }
    }

    /* Returning handle to the interface stack entry with equal or greater
       indexes as passed if found otherwise NU_NULL. */
    return (if_stack_entry);
}

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Add_Entry
*
* DESCRIPTION
*
*       This function adds a stack entry to the stack list. The entries
*       are sorted in ascending order of (Higher Interface, Lower
*       Interface).
*
* INPUTS
*
*       *node                   The new node to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request is successful.
*       NU_INVALID_PARM         If parameter was invalid.
*       NU_NO_MEMORY            Memory allocation fails.
*
*************************************************************************/
STATUS MIB2_If_Stack_Add_Entry(const MIB2_IF_STACK_STRUCT *node)
{
    /* Dummy pointer for parsing the stack list for proper place. */
    MIB2_IF_STACK_STRUCT    *if_stack_ptr;

    /* Higher interface index. */
    UINT32                  h_if_index;

    /* Lower interface index. */
    UINT32                  l_if_index;

    /* Node to be added in the list. */
    MIB2_IF_STACK_STRUCT    *new_node = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Variable for used in finding memory location. */
    UINT16                  loop;

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    /* Status for returning success or error code. */
    STATUS                  status;

    /* Getting higher interface index. */
    h_if_index = ( (node->mib2_higher_layer) ?
                   (node->mib2_higher_layer->dev_index + 1) : 0);

    /* Getting lower interface index. */
    l_if_index = ( (node->mib2_lower_layer) ?
                   (node->mib2_lower_layer->dev_index + 1) : 0);

    /* If current stack entry is not default stack entry then remove
       corresponding default stack entry. */
    if (h_if_index != 0)
    {
        /* Getting handle to the default entry. */
        if_stack_ptr = MIB2_If_Stack_Get_HI_Entry(h_if_index, NU_FALSE);

        /* If default entry exist then remove the default interface stack
           entry. */
        if ( (if_stack_ptr) &&
             (if_stack_ptr->mib2_lower_layer == NU_NULL) )
        {
            /* Removing default stack entry. */
            MIB2_If_Stack_Remove_Entry(if_stack_ptr);
        }
    }

    /* If current stack entry is not default stack entry then remove
       corresponding default stack entry. */
    if (l_if_index != 0)
    {
        /* Getting handle to second default entry. */
        if_stack_ptr = MIB2_If_Stack_Get_LI_Entry(l_if_index, NU_FALSE);

        /* If we have got the handle to second default stack entry then
           remove default entry. */
        if ( (if_stack_ptr) &&
             (if_stack_ptr->mib2_higher_layer == NU_NULL) )
        {
            MIB2_If_Stack_Remove_Entry(if_stack_ptr);
        }
    }
    /* Getting handle to proper location where new interface stack entry
       is to be inserted. */
    if_stack_ptr = MIB2_If_Stack_Get_Location(h_if_index, l_if_index);

    /* If similar stack is not already present the add the stack entry,
       otherwise return error code. */
    if ( (if_stack_ptr == NU_NULL) ||
         (if_stack_ptr->mib2_higher_layer != node->mib2_higher_layer) ||
         (if_stack_ptr->mib2_lower_layer != node->mib2_lower_layer) )
    {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        status = NU_SUCCESS;

        /* Loop to find the unused memory location for the new entry. */
        for (loop = 0; loop < NET_MAX_IF_STACK_ENTRIES; loop++)
        {
            /* If we have reached at unused memory location then break
               through the loop. */
            if (MIB2_If_Stack_Used[loop] == NU_FALSE)
            {
                /* Getting the handle to memory location. */
                new_node = &(MIB2_If_Stack_Memory[loop]);

                /* Mark the memory as used. */
                MIB2_If_Stack_Used[loop] = NU_TRUE;

                /* Breaking through the loop. */
                break;
            }
        }

        /* If we did not find any free memory location then return
           error code. */
        if (loop >= NET_MAX_IF_STACK_ENTRIES)
        {
            status = NU_NO_MEMORY;
        }

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&new_node,
                             sizeof(MIB2_IF_STACK_STRUCT), NU_NO_SUSPEND);

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        if (status == NU_SUCCESS)
        {
            /* Copying data from node passed in. */
            NU_BLOCK_COPY(new_node, node,
                          sizeof(MIB2_IF_STACK_STRUCT));

            /* If node is to be added at end of list. */
            if (if_stack_ptr == NU_NULL)
            {
                /* Adding node at end of list. */
                DLL_Enqueue(&MIB2_Interface_Stack_Root, new_node);
            }
            else
            {
                /* Adding node at proper place. */
                DLL_Insert(&MIB2_Interface_Stack_Root, new_node,
                           if_stack_ptr);
            }

#if (INCLUDE_SNMP == NU_TRUE)

            /* Update Time Stamp value that represent the time of
               interface stack last change. */
            MIB2_Interface_Stack_Root.mib2_stack_last_change = SysTime();

#endif
        }

        /* If memory allocation failed then log the error. */
        else
        {
            NLOG_Error_Log("Failed to allocate memory for ifStack entry",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* If similar stack entry already exist in stack entry list then
       return error code. */
    else
    {
        status = NU_INVALID_PARM;
    }

    /* Returning success or error code.. */
    return (status);

} /* MIB2_If_Stack_Add_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Remove_Entry
*
* DESCRIPTION
*
*       This function removes an entry from the stack list.
*
* INPUTS
*
*       *node                   The node to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successful.
*
*************************************************************************/
STATUS MIB2_If_Stack_Remove_Entry(MIB2_IF_STACK_STRUCT *node)
{
    /* Interface stack entry for adding default stack entry. */
    MIB2_IF_STACK_STRUCT        if_stack_node;

    /* Temporary handle to Interface stack entry. */
    MIB2_IF_STACK_STRUCT        *temp_stack_entry;

    /* Removing node from the list. */
    DLL_Remove(&MIB2_Interface_Stack_Root, node);

    /* If we have removed entry like stack entry (a,b) where both 'a' and
     * 'b' are not NU_NULL then add default entries (a,0) if no stack
     * entry like (a,x) is present and (0,b) if no stack entry like (x,b)
     * is present.
     */
    if ( (node->mib2_higher_layer) && (node->mib2_lower_layer) )
    {
        /* Getting stack entry of the form (a,x) if present. */
        temp_stack_entry =  MIB2_If_Stack_Get_HI_Entry(
                      (node->mib2_higher_layer->dev_index + 1), NU_FALSE);

        /* If there does not exist stack entry like (a,x) then add default
           stack entry i.e. (a,0). */
        if (temp_stack_entry == NU_NULL)
        {
            /* Clearing node to be added. */
            UTL_Zero(&if_stack_node, sizeof(MIB2_IF_STACK_STRUCT));

            /* Setting status of stack entry to 'active'. */
            MIB2_IF_STACK_ACTIVATE(&if_stack_node);

            /* Setting higher layer as 'a' to form the stack entry (a,0)
             * that is to be added.
             */
            if_stack_node.mib2_higher_layer = node->mib2_higher_layer;

            /* Adding default stack entry of the form (a,0). */
            MIB2_If_Stack_Add_Entry((&if_stack_node));
        }

        /* Getting stack entry of the form (x,b) if present. */
        temp_stack_entry = MIB2_If_Stack_Get_LI_Entry(
                       (node->mib2_lower_layer->dev_index + 1), NU_FALSE);

        /* If there does not exist the stack entry like (x,b) then add the
         * default stack entry i.e. (0,b).
         */
        if (temp_stack_entry == NU_NULL)
        {
            /* Clearing stack entry that is to be added. */
            UTL_Zero(&if_stack_node, sizeof(MIB2_IF_STACK_STRUCT));

            /* Activating stack entry that is to be added. */
            MIB2_IF_STACK_ACTIVATE(&if_stack_node);

            /* Setting lower layer as 'b' to form the stack entry (0,b)
             * that is to be added.
             */
            if_stack_node.mib2_lower_layer = node->mib2_lower_layer;

            /* Adding the default stack entry of the form (0,b). */
            MIB2_If_Stack_Add_Entry(&if_stack_node);
        }
    }


#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Mark the memory as unused. */
    MIB2_If_Stack_Used[(UINT8)(node - MIB2_If_Stack_Memory)] = NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    /* Deallocating memory attained by the removed node. */
    if (NU_Deallocate_Memory(node) != NU_SUCCESS)
    {
        NLOG_Error_Log("Memory Deallocation Failed",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

#if (INCLUDE_SNMP == NU_TRUE)

    /* Update Time Stamp value that represents the time of
       interface stack last change. */
    MIB2_Interface_Stack_Root.mib2_stack_last_change = SysTime();

#endif

    /* Returning status. */
    return (NU_SUCCESS);

} /* MIB2_If_Stack_Remove_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Remove_Dev
*
* DESCRIPTION
*
*       This function removes all the stack entries from the stack list
*       that have reference to the device entry whose pointer passed in.
*
* INPUTS
*
*       *dev                    The pointer to the device entry.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID MIB2_If_Stack_Remove_Dev(const DV_DEVICE_ENTRY *dev)
{
    /* Dummy handle for parsing the Interface Stack table. */
    MIB2_IF_STACK_STRUCT         *if_stack_ptr;

    /* Dummy handle for removing the stack entry. */
    MIB2_IF_STACK_STRUCT        *curr_stack_entry;

    /* Temporary variable to hold the stack entry. */
    MIB2_IF_STACK_STRUCT        temp_stack_node;

    /* Start parsing. */
    if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;

    /* Clearing temporary node. */
    UTL_Zero(&temp_stack_node, sizeof(MIB2_IF_STACK_STRUCT));

    /* Activating temporary stack entry node. */
    MIB2_IF_STACK_ACTIVATE(&temp_stack_node);

    /* Loop through all the stack entry. */
    while (if_stack_ptr)
    {
        /* Copying the current value. */
        curr_stack_entry = if_stack_ptr;

        /* Moving forward in the list. */
        if_stack_ptr = if_stack_ptr->mib2_flink;

        /* If we found a stack entry with of the form (a,x) where a is the
           device that whose stack entry are to be removed. */
        if (curr_stack_entry->mib2_higher_layer == dev)
        {
            /* Setting higher layer to NU_NULL to map the stack entry
             * (a,x) to (0,x). This is done so that remove function would
             * not create default entry of 'a' i.e. (a,0).
             */
            curr_stack_entry->mib2_higher_layer = NU_NULL;

            /* Holding the lower device pointer in the temporary stack
             * entry because after removing 'curr_stack_entry' we can't
             * access it via stack entry handle 'curr_stack_entry'.
             */
            temp_stack_node.mib2_lower_layer =
                    curr_stack_entry->mib2_lower_layer;

            /* Removing current stack entry. */
            MIB2_If_Stack_Remove_Entry(curr_stack_entry);

            /* If lower layer 'x' is not NU_NULL then check if stack entry
             * like (b,x) is present or not. If not present then add
             * default stack entry of the form (0,x).
             */
            if (temp_stack_node.mib2_lower_layer)
            {
                /* Getting handle to stack entry of the form (b,x) if
                 * present.
                 */
                curr_stack_entry = MIB2_If_Stack_Get_LI_Entry(
                        (temp_stack_node.mib2_lower_layer->dev_index + 1),
                        NU_FALSE);

                /* If stack entry of the form (b,x) is not present then
                   add the stack entry (0,x). */
                if (curr_stack_entry == NU_NULL)
                {
                    /* Forming stack entry (0,x) in 'temp_stack_node'. */
                    temp_stack_node.mib2_higher_layer = NU_NULL;

                    /* Adding stack entry (0,x). */
                    MIB2_If_Stack_Add_Entry(&temp_stack_node);
                }
            }
        }

        /* If we found a stack entry of the form (x,a) where a is the
           device whose stack entries are to be removed. */
        else if (curr_stack_entry->mib2_lower_layer == dev)
        {
            /* Setting lower layer to NU_NULL to map (x,a) to (x,0), */
            curr_stack_entry->mib2_lower_layer = NU_NULL;

            /* Holding the higher device pointer in the temporary stack
             * entry because after removing 'curr_stack_entry' we can't
             * access it via stack entry handle 'curr_stack_entry'.
             */
            temp_stack_node.mib2_higher_layer =
                curr_stack_entry->mib2_higher_layer;

            /* Removing current stack entry. */
            MIB2_If_Stack_Remove_Entry(curr_stack_entry);

            /* If higher layer 'x' is not NU_NULL then check if stack
             * entry like (x,b) is present or not. If not present then add
             * default stack entry of the form (x,0).
             */
            if (temp_stack_node.mib2_higher_layer)
            {
                /* Getting handle stack entry like (x,b) if present. */
                curr_stack_entry = MIB2_If_Stack_Get_HI_Entry(
                       (temp_stack_node.mib2_higher_layer->dev_index + 1),
                       NU_FALSE);

                /* If stack entry of the form (x,b) is not present then
                   add the default stack entry (x,0). */
                if (curr_stack_entry == NU_NULL)
                {
                    /* Setting lower to NU_NULL to form default stack
                     * entry i.e. (x,0).
                     */
                    temp_stack_node.mib2_lower_layer = NU_NULL;

                    /* Adding default stack entry i.e. (x,0). */
                    MIB2_If_Stack_Add_Entry(&temp_stack_node);
                }
            }
        }
    }

} /* MIB2_If_Stack_Remove_Dev */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_Entry
*
* DESCRIPTION
*
*       This function finds a stack entry corresponding to the pair of
*       (Higher Interface, Lower Interface).
*
* INPUTS
*
*       higher_interface        Higher interface for the stack entry.
*       lower_interface         Lower interface for this stack entry.
*
* OUTPUTS
*
*       Pointer to the instance if found, NU_NULL if no instance was
*       found.
*
*************************************************************************/
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Entry(UINT32 higher_interface,
                                              UINT32 lower_interface)
{
    /* Handle for interface stack entry for searching the node to be
       returned. */
    MIB2_IF_STACK_STRUCT            *if_stack_ptr;

    /* Local variable for higher and lower ifIndex that will be used for
       comparison and validation purpose only.  */
    UINT32                          h_if_index;
    UINT32                          l_if_index;

    /* IfIndex value 0 is representing that interface is NU_NULL. For an
       ifStack entry at most one of higher interface and lower interface
       can be NU_NULL. */
    if ( (higher_interface != 0) || (lower_interface != 0) )
    {
        /* Getting handle to the stack entry with equal/greater interface
           indexes as passed in. */
        if_stack_ptr = MIB2_If_Stack_Get_Location(higher_interface,
                                                  lower_interface);

        /* If have the handle to interface stack entry then validate the
           interface stack entry. */
        if (if_stack_ptr)
        {
            /* Getting higher interface index of found stack entry. */
            h_if_index = ( (if_stack_ptr->mib2_higher_layer) ?
                    (if_stack_ptr->mib2_higher_layer->dev_index + 1) : 0);

            /* Getting lower interface index of found stack entry. */
            l_if_index = ( (if_stack_ptr->mib2_lower_layer) ?
                     (if_stack_ptr->mib2_lower_layer->dev_index + 1) : 0);

            /* If interface indexes of the found entry mis-match with
               interface indexes passed in the return NU_NULL, otherwise
               return handle to the found entry. */
            if ( (higher_interface != h_if_index) ||
                 (lower_interface != l_if_index) )
            {
                /* Returning NU_NULL. */
                if_stack_ptr = NU_NULL;
            }
        }
    }

    /* If interface indexes passed in are invalid then return NU_NULL. */
    else
    {
        /* Returning NU_NULL. */
        if_stack_ptr =  NU_NULL;
    }

    /* Returning handle stack entry if present, otherwise NU_NULL. */
    return (if_stack_ptr);

} /* MIB2_If_Stack_Get_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_HI_Entry
*
* DESCRIPTION
*
*       This function finds the first  stack entry which has a higher
*       interface index value corresponding to the value passed.
*
* INPUTS
*
*       higher_interface        Higher interface for the stack entry.
*       active                  When NU_TRUE return 'active' stack entry.
*
* OUTPUTS
*
*       Pointer to the instance if found, NU_NULL if no instance was found.
*
*************************************************************************/
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_HI_Entry(UINT32 higher_interface,
                                                 UINT8 active)
{
    /* Handle for interface stack entry for searching the node to be
       returned. */
    MIB2_IF_STACK_STRUCT    *if_stack_ptr;

    /* Temporary higher interface index. */
    UINT32                  h_index;

    /* Initializing dummy handle to find the entry. */
    if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;

    /* Searching the node with greater/equal higher interface device
       index. */
    while (if_stack_ptr != NU_NULL)
    {
        /* Getting higher interface index. */
        h_index = ( (if_stack_ptr->mib2_higher_layer) ?
                    (if_stack_ptr->mib2_higher_layer->dev_index + 1) : 0);

        /* If we have reached at stack entry with desire higher interface. */
        if (higher_interface == h_index)
        {
            /* If we are not interested in status of stack entry
             * then break through the loop and return current
             * stack entry.
             */
            if (active == NU_FALSE)
            {
                break;
            }

            /* If current stack entry is 'active' then break through the
               loop. */
            else IF_MIB2_IF_STACK_IS_ACTIVE(if_stack_ptr)
            {
                /* Breaking through the loop. */
                break;
            }
        }

        /* If we have passed all the stack entries with lesser/equal
         * higher interface index then break through the loop and return
         * NU_NULL.
         */
        else if (h_index > higher_interface)
        {
            if_stack_ptr = NU_NULL;

            break;
        }

        /* Moving forward in the list. */
        if_stack_ptr = if_stack_ptr->mib2_flink;
    }

    /* Returning node found if present, otherwise NU_NULL. */
    return (if_stack_ptr);

} /* MIB2_If_Stack_Get_HI_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_LI_Entry
*
* DESCRIPTION
*
*       This function finds the first stack entry which has a lower
*       interface index value corresponding to the value passed.
*
* INPUTS
*
*       lower_interface         Lower interface for the stack entry.
*       active                  When NU_TRUE return 'active' stack entry.
*
* OUTPUTS
*
*       Pointer to the instance if found, NU_NULL if no instance was
*       found.
*
*************************************************************************/
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_LI_Entry(UINT32 lower_interface,
                                                 UINT8 active)
{
    /* Handle for interface stack entry for searching the node to be
       returned. */
    MIB2_IF_STACK_STRUCT    *if_stack_ptr;

    /* Handle to the lower interface device. */
    DV_DEVICE_ENTRY         *l_dev;

    /* If lower interface index passed in is non-zero then get the handle
       to corresponding interface device. */
    if (lower_interface != 0)
    {
        /* Getting handle to lower interface device. */
        l_dev = DEV_Get_Dev_By_Index(lower_interface - 1);
    }

    /* If lower interface index passed in is zero then set lower interface
       handle to NU_NULL. */
    else
    {
        l_dev = NU_NULL;
    }

    /* If we have valid parameters. */
    if ( (lower_interface == 0) || (l_dev) )
    {
        /* Initializing dummy handle for parsing. */
        if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;

        /* Loop through the interface stack entry list to find the entry
           with lower interface index as passed in. */
        while(if_stack_ptr)
        {
            /* If we have reached at a node with equal lower ifIndex as
             * passed in then break through the loop if it is required
             * stack entry.
             */
            if (if_stack_ptr->mib2_lower_layer == l_dev)
            {
                /* If we are not interested in status of stack entry then
                 * break through the loop and return current stack entry.
                 */
                if (active == NU_FALSE)
                    break;

                /* If current stack entry is 'active' then break through
                   the loop and return current stack entry handle. */
                else IF_MIB2_IF_STACK_IS_ACTIVE(if_stack_ptr)
                {
                    /* Break through the loop. */
                    break;
                }
            }

            /* Moving forward in the list. */
            if_stack_ptr = if_stack_ptr->mib2_flink;
        }
    }
    else
    {
        if_stack_ptr = NU_NULL;
    }

    /* Returning node found if present, otherwise NU_NULL. */
    return (if_stack_ptr);

} /* MIB2_If_Stack_Get_LI_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_Next_Entry
*
* DESCRIPTION
*
*       This function finds the next greater stack entry to the value
*       of (Higher Interface, Lower Interface). If there is no such entry
*       then the first element is returned.
*
* INPUTS
*
*       higher_interface        Higher interface for the stack entry.
*       lower_interface         Lower interface for this stack entry.
*
* OUTPUTS
*
*       Pointer to the instance. NU_NULL if the list is empty.
*
*************************************************************************/
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Next_Entry(UINT32 higher_interface,
                                                   UINT32 lower_interface)
{
    /* Handle for interface stack entry for searching the node to be
       returned. */
    MIB2_IF_STACK_STRUCT    *if_stack_ptr;

    if ( (higher_interface != 0) || (lower_interface != 0) )
    {
        /* Increment lower interface index to get next stack entry. */
        ++lower_interface;

        /* Getting handle to next stack entry. */
        if_stack_ptr = MIB2_If_Stack_Get_Location(higher_interface,
                                                  lower_interface);

        /* If we did not get the handle to next interface stack entry then
           return handle to the first stack entry in the list. */
        if (if_stack_ptr == NU_NULL)
        {
            /* Return handle to first stack entry in the list. */
            if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;
        }
    }

    /* If higher and lower interface indexes passed are zero then return
       handle to the first stack entry. */
    else
    {
        if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;
    }

    /* Returning node found if present, otherwise NU_NULL. */
    return (if_stack_ptr);

} /* MIB2_If_Stack_Get_Next_Entry */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_If_Stack_Get_Next_Entry2
*
* DESCRIPTION
*
*       This function finds the next greater stack entry to the value
*       of stack entry (Higher Interface, Lower Interface). If there is
*       no such entry then the first element is returned.
*
* INPUTS
*
*       *stack_entry            pointer to the stack entry.
*       active                  When NU_TRUE return 'active' stack entry.
*
* OUTPUTS
*
*       Pointer to the instance. NU_NULL if the list is empty.
*
*************************************************************************/
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Next_Entry2(const MIB2_IF_STACK_STRUCT *stack_entry,
                                                    UINT8 active)
{
    /* Handle for parsing the stack entry list. */
    MIB2_IF_STACK_STRUCT    *if_stack_ptr;

#if (INCLUDE_SNMP == NU_TRUE)

    /* If stack entry passed in not NU_NULL then start searching from next
       stack entry. */
    if (stack_entry)
    {
        /* Initializing stack entry handle to stack searching. */
        if_stack_ptr = stack_entry->mib2_flink;
    }

    /* If stack entry passed in not NU_NULL then set start searching from
       start of the stack entry list. */
    else
    {
        /* Setting stack entry handle to NU_NULL to get started from start
           of stack entry list. */
        if_stack_ptr = NU_NULL;
    }

    /* Loop through the stack entry list starting from the stack entry
       passed in. */
    while (if_stack_ptr)
    {
        /* If we are not interested in status of stack entry then
         * break through the loop and return current stack entry.
         */
        if (active == NU_FALSE)
        {
            break;
        }

        /* If current stack entry is 'active' then break through the loop
           and return current stack entry. */
        else IF_MIB2_IF_STACK_IS_ACTIVE(if_stack_ptr)
        {
            /* Breaking through the loop. */
            break;
        }

        /* Moving forward in the list. */
        if_stack_ptr = if_stack_ptr->mib2_flink;
    }

    /* If we did not get the handle to stack entry then start parsing from
       the start of the list. */
    if (if_stack_ptr == NU_NULL)
    {
        /* Starting search of the first 'active' stack entry. */
        if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;

        /* Searching for the first 'active' stack entry. */
        while (if_stack_ptr)
        {
            /* If we have reached back to to the node that is passed in
               then break through the loop. */
            if (if_stack_ptr == stack_entry)
            {
                /* If current stack entry is not 'active' then return
                   NU_NULL. */
                if (if_stack_ptr->mib2_row_status != SNMP_ROW_ACTIVE)
                    if_stack_ptr = NU_NULL;

                /* Breaking through the loop. */
                break;
            }

            /* If we are not interested in status of stack entry then
             * break through the loop and return current stack entry.
             */
            if (active == NU_FALSE)
            {
                break;
            }

            /* If have reached at active stack entry then break through
               the loop and return current stack entry. */
            else IF_MIB2_IF_STACK_IS_ACTIVE(if_stack_ptr)
            {
                /* Breaking through the loop and returning current stack
                   entry. */
                break;
            }

            /* Moving forward in the list. */
            if_stack_ptr = if_stack_ptr->mib2_flink;
        }

    }

#else /* (INCLUDE_SNMP == NU_TRUE) */

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(active);

    if (stack_entry)
    {
        if_stack_ptr = stack_entry->mib2_flink;

        /* If we have not found the stack entry then return first stack
           entry. */
        if (if_stack_ptr == NU_NULL)
        {
            /* Returning first stack entry. */
            if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;
        }
    }

    else
    {
        /* Returning the first stack entry. */
        if_stack_ptr = MIB2_Interface_Stack_Root.mib2_flink;
    }

#endif /* (INCLUDE_SNMP == NU_TRUE) */

    /* Returning handle to next stack entry. */
    return (if_stack_ptr);

} /* MIB2_If_Stack_Get_Next_Entry2 */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE))

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfStack
*
* DESCRIPTION
*
*       This function checks whether an interface stack entry with the
*       given indexes exists.
*
* INPUTS
*
*       higher_if_index         The interface index of the higher interface.
*       lower_if_index          The interface index of the lower interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfStack(UINT32 higher_if_index, UINT32 lower_if_index)
{
    /* Dummy handle to interface stack entry. */
    MIB2_IF_STACK_STRUCT        *stack_entry;

    /* Status for returning success or error code. */
    INT16                       status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Getting handle to the interface stack entry. */
        stack_entry = MIB2_If_Stack_Get_Entry(higher_if_index, lower_if_index);

        /* If we have found the stack entry then return success code,
           otherwise return error code. */
        if (stack_entry != NU_NULL)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = MIB2_UNSUCCESSFUL;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfStack */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_Next_IfStack
*
* DESCRIPTION
*
*       This function updates the higher and lower interface indexes
*       passed in to the higher and lower interfaces of next stack entry.
*
* INPUTS
*
*       *higher_if_index        The interface index of the higher interface.
*       *lower_if_index         The interface index of the lower interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_Next_IfStack(UINT32 *higher_if_index, UINT32 *lower_if_index)
{
    /* Dummy handle to interface stack entry. */
    MIB2_IF_STACK_STRUCT        *stack_entry;

    /* Status for returning success or error code. */
    INT16                       status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }

    /* If we have successfully obtained the semaphore the proceed. */
    else
    {
        /* Getting handle to the next stack entry. */
        stack_entry = MIB2_If_Stack_Get_Next_Entry((*higher_if_index),
                                                   (*lower_if_index));

        /* If we have found the the handle to the next stack entry
         * then update the indexes passed in and return success code.
         */
        if (stack_entry != NU_NULL)
        {
            /* Updating higher interface index passed in. */
            (*higher_if_index) =
                    ((stack_entry->mib2_higher_layer != NU_NULL) ?
                     (stack_entry->mib2_higher_layer->dev_index + 1) : 0);

            /* Updating the lower interface index passed in. */
            (*lower_if_index) =
                    ((stack_entry->mib2_lower_layer != NU_NULL) ?
                     (stack_entry->mib2_lower_layer->dev_index + 1) : 0);

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* If we did not get the handle to the interface stack entry
           then return error code. */
        else
        {
            /* Returning error code. */
            status = MIB2_UNSUCCESSFUL;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_Next_IfStack */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_IfStack_Row_Status
*
* DESCRIPTION
*
*       This function updates the row status of the interface stack entry
*       specified by higher and lower interface indexes passed in.
*
* INPUTS
*
*       higher_if_index        The interface index of the higher interface.
*       lower_if_index         The interface index of the lower interface.
*       *row_status            The memory location where row status is to
*                              be copied.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_IfStack_Row_Status(UINT32 higher_if_index,
                                  UINT32 lower_if_index, UINT8 *row_status)
{
    /* Dummy handle to interface stack entry. */
    MIB2_IF_STACK_STRUCT    *stack_entry;

    /* Status for returning success or error code. */
    INT16                   status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Getting handle to the stack entry. */
        stack_entry = MIB2_If_Stack_Get_Entry(higher_if_index, lower_if_index);

        /* If stack entry found then update the value row status passed in
           and return success code, otherwise return error code. */
        if (stack_entry != NU_NULL)
        {
            /* Updating row status. */
            (*row_status) = stack_entry->mib2_row_status;

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* If we did not get the handle to the interface stack entry then
           return error code. */
        else
        {
            /* Returning error code. */
            status = MIB2_UNSUCCESSFUL;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_IfStack_Row_Status */

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE)) */
#endif /* (INCLUDE_IF_STACK == NU_TRUE) */


