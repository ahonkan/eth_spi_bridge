/*************************************************************************
*                                                                      
*               Copyright 2005 Mentor Graphics Corporation                 
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
*       dbg_set.c                                
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Set
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C main functions for the component.  The
*       set functionality is a 'super linked list' implementation.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       NONE       
*                                                                      
*   FUNCTIONS                                                            
*
*       dbg_set_retrieve_default_add_location
*       dbg_set_retrieve_default_remove_location
*       dbg_set_node_find_adjacent
*       dbg_set_node_add_adjacent
*       dbg_set_node_list_find
*       dbg_set_node_list_add_to_end
*       dbg_set_node_list_remove
*
*       DBG_SET_Initialize
*       DBG_SET_Node_Find
*       DBG_SET_Node_Add
*       DBG_SET_Node_Remove
*       DBG_SET_Node_Count
*                                                                      
*   DEPENDENCIES
*
*       dbg.h
*                                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"

/***** Local functions */

/* Local function prototypes */

static DBG_SET_LOCATION dbg_set_retrieve_default_add_location(DBG_SET * p_set);

static DBG_SET_LOCATION dbg_set_retrieve_default_remove_location(DBG_SET * p_set);

static DBG_STATUS dbg_set_node_find_adjacent(DBG_SET_NODE *    p_node,
                                              BOOLEAN           get_next,
                                              DBG_SET_NODE **   p_p_node,
                                              UINT              iterations);

static DBG_STATUS dbg_set_node_add_adjacent(DBG_SET_NODE *     p_node,
                                             BOOLEAN            add_next,
                                             DBG_SET_NODE *     p_next_node);

static DBG_STATUS dbg_set_node_list_find(DBG_SET *         p_set,
                                          DBG_SET_NODE_ID   node_id,
                                          BOOLEAN           start_at_head,
                                          DBG_SET_NODE **   p_p_node);

static DBG_STATUS dbg_set_node_list_find_Index(DBG_SET *           p_set,
                                                UINT                index,
                                                DBG_SET_NODE **     p_p_node);

static DBG_STATUS dbg_set_node_list_add_to_end(DBG_SET *           p_set,
                                                DBG_SET_NODE *      p_node,
                                                BOOLEAN             add_to_head);

static DBG_STATUS dbg_set_node_list_remove(DBG_SET *           p_set,
                                            DBG_SET_NODE *      p_node);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_set_retrieve_default_add_location
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function returns the default add location for a specified
*       set based on the default behavior of the set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_SET_LOCATION - default start location for set based on sets
*       default behavior.
*                                                                      
*************************************************************************/
static DBG_SET_LOCATION dbg_set_retrieve_default_add_location(DBG_SET * p_set)
{
    DBG_SET_LOCATION    default_location;
    
    /* determine default location based on the sets default behavior. */
    
    switch (p_set -> default_behavior)
    {
        case DBG_SET_BEHAVIOR_FIFO:
        case DBG_SET_BEHAVIOR_LIFO:
        {
            /* NOTE: FIFO and LIFO behaviors both add to the tail of a
               list.  The actual FIFO/LIFO behavior is exhibited during
               the removal process... */
        
            default_location = DBG_SET_LOCATION_TAIL;
        
            break;
        
        } 
        
        default:
        {
            /* ERROR: invalid default set behavior. */
            
            /* ERROR RECOVERY: Assume default add location is tail. */
            
            default_location = DBG_SET_LOCATION_TAIL;
        
            break;
        
        } 
    
    } 
    
    return (default_location);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_set_retrieve_default_remove_location
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function returns the default removal start location for a 
*       specified set based on the default behavior of the set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_SET_LOCATION - default start location for set based on sets
*       default behavior.
*                                                                      
*************************************************************************/
static DBG_SET_LOCATION dbg_set_retrieve_default_remove_location(DBG_SET * p_set)
{
    DBG_SET_LOCATION    default_location;
    
    /* Determine default location based on the sets default behavior. */
    
    switch (p_set -> default_behavior)
    {
        case DBG_SET_BEHAVIOR_FIFO:
        {
            default_location = DBG_SET_LOCATION_HEAD;
        
            break;
        
        } 
                
        case DBG_SET_BEHAVIOR_LIFO:
        {
            default_location = DBG_SET_LOCATION_TAIL;
        
            break;
        
        } 
        
        case DBG_SET_BEHAVIOR_ORDERED:
        default:
        {
            /* ERROR: Invalid default set behavior. */

            /* ERROR RECOVERY: Assume default behavior is FIFO. */
            
            default_location = DBG_SET_LOCATION_HEAD;
        
            break;
        
        } 
    
    } 
    
    return (default_location);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_find_adjacent
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to find the node adjacent to the specified 
*       node.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_node - pointer to the starting node.
*
*       get_next - boolean value that indicates if the target node is the
*                  next node or the previous node.  A value of NU_TRUE 
*                  indicates the next node and a value of NU_FALSE 
*                  indicates the previous node.
*
*       p_p_node - return parameter pointer to pointer to specified set 
*                  node if operation is successful.  Otherwise it will be
*                  set to NU_NULL.
*
*       iterations - the number of times the next or previous operation
*                    should be performed.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that the target node
*                                         could not be found.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_set_node_find_adjacent(DBG_SET_NODE *    p_node,
                                              BOOLEAN           get_next,
                                              DBG_SET_NODE **   p_p_node,
                                              UINT              iterations)
{
    DBG_STATUS      dbg_status;
    DBG_SET_NODE *  p_trg_node; 
    UINT            i;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Default to the starting node parameter. */
    
    p_trg_node = p_node;
    
    /* Determine if there are any nodes in the list. */
    
    if (p_trg_node != NU_NULL)
    {    
        /* Determine if the target node is next or previous to the specified
           node. */
           
        if (get_next == NU_TRUE)
        {
            /* Target node is next node. */
        
            /* Perform the operation the specified number of times.  NOTE:
               it is possible to loop if the number of iterations is larger
               than the number of nodes in the list. */
    
            for (i = iterations; i != 0; i--)
            {
                p_trg_node = p_trg_node -> p_next;
    
            } 
            
        }
        else
        {
            /* Target node is previous node. */
    
            /* Perform the operation the specified number of times.  NOTE:
               it is possible to loop if the number of iterations is larger
               than the number of nodes in the list. */
    
            for (i = iterations; i != 0; i--)
            {
                p_trg_node = p_trg_node -> p_prev;
    
            } 
        
        } 

        /* Update return parameter. */

        *p_p_node = p_trg_node;
        
    }
    else
    { 
        /* Update return parameter. */
    
        *p_p_node = NU_NULL;
    
        /* Operation failed. */
    
        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
    
    } 
    
    return (dbg_status);
}                                        

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_add_adjacent
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to add a node adjacent to the specified
*       node.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_node - pointer to the specified node.
*
*       add_next - boolean value that indicates if the target node is the
*                  next node or the previous node.  A value of NU_TRUE 
*                  indicates the next node and a value of NU_FALSE 
*                  indicates the previous node.
*
*       p_next_node - pointer to the new node. 
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_set_node_add_adjacent(DBG_SET_NODE *     p_node,
                                             BOOLEAN            add_next,
                                             DBG_SET_NODE *     p_next_node)
{
    /* determine if the new node is to be added next or previous to the
       specified node. */
       
    if (add_next == NU_TRUE)
    {
        /* add the new node after the specified node. */
        
        /* update new nodes previous. */
        
        p_next_node -> p_prev = p_node;
        
        /* update new nodes next. */
        
        p_next_node -> p_next = p_node -> p_next;        

        /* update the specified nodes next nodes previous. */
        
        p_node -> p_next -> p_prev = p_next_node;
        
        /* update the specified nodes next. */
        
        p_node -> p_next = p_next_node;
    }
    else
    {
        /* add the new node before the specified node. */
    
        /* update new nodes previous. */
        
        p_next_node -> p_prev = p_node -> p_prev;
        
        /* update new nodes next. */
        
        p_next_node -> p_next = p_node;        

        /* update the specified nodes previous nodes next. */
        
        p_node -> p_prev -> p_next = p_next_node;
        
        /* update the specified nodes previous. */
        
        p_node -> p_prev = p_next_node;    
    
    } 

    return(DBG_STATUS_OK);
}                                           

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_list_find
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to find the element containing the
*       specified data ID value (and the previous element) in the set 
*       list.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       node_id - ID of the data to search for.
*
*       start_at_head - boolean value that indicates if the operation 
*                       should start at the head of the list.  If NU_TRUE
*                       then operation starts at the head otherwise 
*                       operation starts at the tail.
*
*       p_p_node - return parameter pointer to pointer to set element
*                  of specified data ID if operation is successful.  
*                  Otherwise it will be set to NU_NULL.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that the target node
*                                         could not be found.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_set_node_list_find(DBG_SET *         p_set,
                                          DBG_SET_NODE_ID   node_id,
                                          BOOLEAN           start_at_head,
                                          DBG_SET_NODE **   p_p_node)
{
    DBG_STATUS      dbg_status;
    BOOLEAN         found;
    DBG_SET_NODE *  p_node;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Indicate not found. */
    
    found = NU_FALSE;
    
    /* determine if there are any nodes in the list. */
    
    if (p_set -> p_node_list_head != NU_NULL)
    { 
        /* perform search based on starting location. */
        
        if (start_at_head == NU_TRUE)
        {
            /* set start node to head. */
        
            p_node = p_set -> p_node_list_head;
        
            /* determine if the target set node ID is specific or ANY ID. */
            
            if (node_id == DBG_SET_NODE_ID_ANY)        
            {
                /* search for ANY ID. */ 
                
                /* determine if ANY set node ID was found. */
                
                if (p_node != NU_NULL)
                {
                    /* A set node was found. */
                    
                    found = NU_TRUE;
                
                }             
            }
            else
            {
                /* search for a specific set node ID. */
            
                /* search through all entries in the data ID list looking for the 
                   specified node_id. */
                 
                while ((p_node -> node_id != node_id) &&
                       (p_node -> p_next != p_set -> p_node_list_head))
                {
                    /* update search variables. */
            
                    p_node = p_node -> p_next;
                
                } 
                
                /* determine if the set node ID was found. */
                
                if (p_node -> node_id == node_id)
                {
                    /* set node ID was found. */
                    
                    found = NU_TRUE;
                
                }             
                
            } 
    
        }
        else
        {
            /* set start node to tail. */
            
            p_node = p_set -> p_node_list_tail;
        
            /* determine if the target set node ID is specific or ANY ID. */
            
            if (node_id == DBG_SET_NODE_ID_ANY)        
            {
                /* search for ANY ID. */ 
                
                /* determine if ANY set node ID was found. */
                
                if (p_node != NU_NULL)
                {
                    /* A set node was found. */
                    
                    found = NU_TRUE;
                
                }             
            }
            else
            {    
                /* search for a specific set node ID. */    
        
                /* search through all entries in the data ID list looking for the 
                   specified node_id. */
                 
                while ((p_node -> node_id != node_id) &&
                       (p_node -> p_prev != p_set -> p_node_list_tail))
                {
                    /* update search variables. */
            
                    p_node = p_node -> p_prev;
                
                }         
    
                /* determine if the set node ID was found. */
                
                if (p_node -> node_id == node_id)
                {
                    /* set node ID was found. */
                    
                    found = NU_TRUE;
                
                } 
                
            } 
    
        } 

    } 

    /* update return parameter based on success of operation. */
    
    if (found == NU_TRUE)
    {
        *p_p_node = p_node;
        
    }
    else
    {
        /* ERROR: Unable to find target node. */
        
        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
        *p_p_node = NU_NULL;
        
    } 

    return(dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_list_find_Index
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to find the element containing the
*       specified data ID value (and the previous element) in the set 
*       list.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       index - the index of the node to return.
*
*       p_p_node - return parameter pointer to pointer to set element of 
*                  specified data ID if operation is successful.  
*                  Otherwise it will be set to NU_NULL.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that the target node
*                                         could not be found.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_set_node_list_find_Index(DBG_SET *           p_set,
                                                UINT                index,
                                                DBG_SET_NODE **     p_p_node)
{
    DBG_STATUS      dbg_status;
    
    /* determine if there are enough set nodes in the set to perform
       the requested operation. */

    if (index < p_set -> node_count)
    {
        /* the operation may be performed. */

        dbg_status = dbg_set_node_find_adjacent(p_set -> p_node_list_head,
                                                 NU_TRUE,
                                                 p_p_node,
                                                 index);
        
    }
    else
    {
        /* the operation can not be performed as there are not enough set
           nodes in the set. */

        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;

        *p_p_node = NU_NULL;

    } 

    return(dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_list_add_to_end
*                                                                      
*   DESCRIPTION   
*                                                       
*       Adds a node to the end of a node list.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to set.
*
*       p_node - pointer to the node.
*
*       add_to_head - boolean value that indicates that the new node 
*                     should be added to the head or tail.  A value of 
*                     NU_TRUE indicates that the node should be added to 
*                     the head otherwise the node will be added to the 
*                     tail.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_set_node_list_add_to_end(DBG_SET *           p_set,
                                                DBG_SET_NODE *      p_node,
                                                BOOLEAN             add_to_head)
{
    /* determine if node list is currently empty (special case) or if
       there are already nodes in it (normal case). */
       
    if (p_set -> p_node_list_head == NU_NULL)
    {
        /* node list is empty so add new node as only node in list. */
     
        /* update new node. */
        
        p_node -> p_next = p_node;
        p_node -> p_prev = p_node;
        
        /* update node list head. */
        
        p_set -> p_node_list_head = p_node;
         
        /* update node list tail. */
        
        p_set -> p_node_list_tail = p_node;
        
    }
    else
    {
        /* node list contains nodes so add new node to start of list. */
        
        /* update new node */
        
        p_node -> p_next = p_set -> p_node_list_head;
        p_node -> p_prev = p_set -> p_node_list_tail;
        
        /* determine if adding to the head or tail. */
        
        if (add_to_head == NU_TRUE)
        {
            /* update node list head node. */
            
            p_set -> p_node_list_head -> p_prev = p_node;        
            
            /* update node list head. */
            
            p_set -> p_node_list_head = p_node;
            
            /* update node list tail node. */
            
            p_set -> p_node_list_tail -> p_next = p_node;

        }
        else
        {
            /* update node list tail node. */
            
            p_set -> p_node_list_tail -> p_next = p_node;        
            
            /* update node list tail. */
            
            p_set -> p_node_list_tail = p_node;
            
            /* update node list head node. */
            
            p_set -> p_node_list_head -> p_prev = p_node;        
        
        } 
        
    } 
    
    /* update statistics. */
    
    p_set->node_count++;
    
    return(DBG_STATUS_OK);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_set_node_list_remove
*                                                                      
*   DESCRIPTION   
*                                                       
*       Removes a node from the set list.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to set.
*
*       p_node - pointer to the node to be removed.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*                                                                      
************************************************************************/
static DBG_STATUS dbg_set_node_list_remove(DBG_SET *           p_set,
                                            DBG_SET_NODE *      p_node)
{
    /* determine if node is head of list. */
       
    if (p_node == p_set -> p_node_list_head)
    {
        /* node is head of list. */
        
        /* determine if node is only node in list. */
        
        if (p_node == p_set -> p_node_list_tail)
        {
            /* node is only node in list. */
            
            /* update node list head. */
            
            p_set -> p_node_list_head = NU_NULL;
            
            /* update node list tail. */
            
            p_set -> p_node_list_tail = NU_NULL;
            
        }
        else
        {
            /* node is NOT only node in list. */
            
            /* update node list head. */
            
            p_set -> p_node_list_head = p_node -> p_next;
           
            /* update node list head node. */
            
            p_set -> p_node_list_head -> p_prev = p_set -> p_node_list_tail;             
            
            /* update node list tail node. */
            
            p_set -> p_node_list_tail -> p_next = p_set -> p_node_list_head;
            
        } 
        
    }
    else
    {
        /* node is NOT head of list. */ 
        
        /* determine if node is tail of list. */
        
        if (p_node == p_set -> p_node_list_tail)
        {
            /* node is tail of list. */
            
            /* update node list tail. */
            
            p_set -> p_node_list_tail = p_node -> p_prev;
        
            /* update node list tail node. */
            
            p_set -> p_node_list_tail -> p_next = p_set -> p_node_list_head;
            
            /* update node list head node. */
            
            p_set -> p_node_list_head -> p_prev = p_set -> p_node_list_tail;
        
        }
        else
        {
            /* node is in middle of list. */
            
            /* update next node. */
            
            p_node -> p_next -> p_prev = p_node -> p_prev;
            
            /* update previous node. */
            
            p_node -> p_prev -> p_next = p_node -> p_next;
        
        }         
        
    } 
    
    /* update node. */
    
    p_node -> p_next = NU_NULL;
    p_node -> p_prev = NU_NULL;
    p_node -> node_id = DBG_SET_NODE_ID_ANY;
    
    /* update statistics. */
    
    p_set -> node_count--;
    
    return(DBG_STATUS_OK);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_SET_Initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function initializes a set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       default_behavior - default behavior for the set.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_SET_Initialize(DBG_SET *            p_set,
                              DBG_SET_BEHAVIOR     default_behavior)
{
    /* setup element list. */
    
    p_set -> p_node_list_head = NU_NULL;
    p_set -> p_node_list_tail = NU_NULL;
    
    /* setup statistics. */
    
    p_set -> node_count = 0;
    
    /* setup behavior. */
    
    p_set -> default_behavior = default_behavior;
    
    return(DBG_STATUS_OK);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_SET_Node_Find
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function finds existing node in a set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       location - location within set that operation is performed.
*
*       location_value - additional location data.
*
*       p_p_node - return parameter that is pointer to pointer to node IF 
*                  operation is successful otherwise NU_NULL returned.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that the target node
*                                         could not be found.
*
*       DBG_STATUS_FAILED - Indicates an invalid location.
*                                                                      
*************************************************************************/                           
DBG_STATUS DBG_SET_Node_Find(DBG_SET *                  p_set,
                             DBG_SET_LOCATION           location,
                             DBG_SET_LOCATION_VALUE     location_value,
                             DBG_SET_NODE **            p_p_node)
{   
    DBG_STATUS          dbg_status;
    DBG_SET_LOCATION    find_location;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Set start location of operation. */
    
    if (location == DBG_SET_LOCATION_DEFAULT)
    {
        /* NOTE: The find operation uses the default behavior locations 
           of the remove operation. */
    
        find_location = dbg_set_retrieve_default_remove_location(p_set);
    }
    else
    {
        /* Specified location is NOT default so its value should be used
           as the starting location. */
           
        find_location = location;
    
    } 
    
    /* Determine how to proceed based on start location of operation. */
    
    switch (find_location)
    {
        case DBG_SET_LOCATION_HEAD :
        {
            dbg_status = dbg_set_node_list_find(p_set,
                                                 (DBG_SET_NODE_ID)location_value,
                                                 NU_TRUE,
                                                 p_p_node);
        
            break;
        
        } 
        
        case DBG_SET_LOCATION_TAIL :
        {
            dbg_status = dbg_set_node_list_find(p_set,
                                                 (DBG_SET_NODE_ID)location_value,
                                                 NU_FALSE,
                                                 p_p_node);        
        
            break;
        
        } 
        
        case DBG_SET_LOCATION_NEXT :
        {
            dbg_status = dbg_set_node_find_adjacent((DBG_SET_NODE *)location_value,
                                                     NU_TRUE,
                                                     p_p_node,
                                                     1);        
        
            break;
        
        }         
        
        case DBG_SET_LOCATION_PREV :
        {
            dbg_status = dbg_set_node_find_adjacent((DBG_SET_NODE *)location_value,
                                                     NU_FALSE,
                                                     p_p_node,
                                                     1);        
        
            break;
        
        }          

        case DBG_SET_LOCATION_NEXT_LINEAR :
        {
            /* Ensure that requested operation does not cross the tail
               boundary. */

            if ((DBG_SET_NODE *)location_value != p_set -> p_node_list_tail)
            {    
                dbg_status = dbg_set_node_find_adjacent((DBG_SET_NODE *)location_value,
                                                         NU_TRUE,
                                                         p_p_node,
                                                         1);        

            }
            else
            {
                dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
            
            } 
                
            break;
        
        }         
        
        case DBG_SET_LOCATION_PREV_LINEAR :
        {
            /* Ensure that requested operation does not cross the head
               boundary. */

            if ((DBG_SET_NODE *)location_value != p_set -> p_node_list_head)
            {
                dbg_status = dbg_set_node_find_adjacent((DBG_SET_NODE *)location_value,
                                                        NU_FALSE,
                                                        p_p_node,
                                                        1);        

            }
            else
            {
                dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
            
            } 
                
            break;
        
        }  
        
        case DBG_SET_LOCATION_INDEX :
        {
            dbg_status = dbg_set_node_list_find_Index(p_set,
                                                      (UINT)location_value,
                                                      p_p_node);
                                                     
            break;
            
        } 
        
        default :
        {
            /* ERROR: Invalid start location for set find operation. */
            
            dbg_status = DBG_STATUS_FAILED;
        
            break;
        
        } 
    
    } 

    return(dbg_status);
}  

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_SET_Node_Add
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function adds new node to a set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       location - location within set that operation is performed.
*
*       location_value - additional location data.
*
*       node_id - the node ID value.
*
*       p_node - pointer to node.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates the operation failed.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_SET_Node_Add(DBG_SET *                   p_set,
                            DBG_SET_LOCATION            location,
                            DBG_SET_LOCATION_VALUE      location_value,
                            DBG_SET_NODE_ID             node_id,
                            DBG_SET_NODE *              p_node)
{
    DBG_STATUS          dbg_status;
    DBG_SET_LOCATION    add_location;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* set start location of operation. */
    
    if (location == DBG_SET_LOCATION_DEFAULT)
    {
        add_location = dbg_set_retrieve_default_add_location(p_set);
    }
    else
    {
        /* specified location is NOT default so its value should be used
           as the starting location. */
           
        add_location = location;
    
    } 

    /* initialize the new node. */
    
    p_node -> node_id = node_id;

    /* determine how to proceed based on location of operation. */
    
    switch (add_location)
    {
        case DBG_SET_LOCATION_HEAD :
        {
            /* NOTE: the term END could be either the tail OR the 
               head.  It is referring to one END of the list. */
        
            dbg_status = dbg_set_node_list_add_to_end(p_set,
                                                       p_node,
                                                       NU_TRUE);
        
            break;
        
        } 
        
        case DBG_SET_LOCATION_TAIL :
        {
            /* NOTE: the term END could be either the tail OR the 
               head.  It is referring to one END of the list. */        
        
            dbg_status = dbg_set_node_list_add_to_end(p_set,
                                                      p_node,
                                                      NU_FALSE);
        
            break;
        
        } 

        case DBG_SET_LOCATION_NEXT_LINEAR :
        case DBG_SET_LOCATION_NEXT :
        {
            dbg_status = dbg_set_node_add_adjacent((DBG_SET_NODE *)location_value,
                                                    NU_TRUE,
                                                    p_node);
            break;
        
        }         

        case DBG_SET_LOCATION_PREV_LINEAR :
        case DBG_SET_LOCATION_PREV :
        {
            dbg_status = dbg_set_node_add_adjacent((DBG_SET_NODE *)location_value,
                                                   NU_FALSE,
                                                   p_node);        
        
            break;
        
        }          
                
        case DBG_SET_LOCATION_INDEX :   /* NOT IMPLEMENTED */
        default :
        {
            /* ERROR: invalid location for set add operation. */
            
            dbg_status = DBG_STATUS_FAILED;
        
            break;
        
        } 
    
    } 
    
    return(dbg_status);
}                             
       
/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_SET_Node_Remove
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function removes existing node from a set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*       location - location within set that operation is performed.
*
*       location_value - additional location data.
*
*       p_p_node - return parameter that is pointer to pointer to node IF
*                  operation is successful otherwise is DBG_NULL.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates no nodes to remove.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/                        
DBG_STATUS DBG_SET_Node_Remove(DBG_SET *                p_set,
                               DBG_SET_LOCATION         location,
                               DBG_SET_LOCATION_VALUE   location_value,
                               DBG_SET_NODE **          p_p_node)
{
    DBG_STATUS      dbg_status;

    /* Attempt to find node in the set. */
    
    dbg_status = DBG_SET_Node_Find(p_set,
                                   location,
                                   location_value,
                                   p_p_node);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Node ID is in the set so remove it. */
        
        dbg_status = dbg_set_node_list_remove(p_set,
                                              *p_p_node);
        
    } 

    return(dbg_status);
}                                

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_SET_Node_Count
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function returns number of nodes in a set.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_set - pointer to the set.
*
*   OUTPUTS                                                              
*                                                                      
*       Returns current number of nodes in set.
*                                                                      
*************************************************************************/
UINT DBG_SET_Node_Count(DBG_SET *    p_set)
{
    return(p_set -> node_count);
}
