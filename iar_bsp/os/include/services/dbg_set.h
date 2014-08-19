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
*       dbg_set.h                                              
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Set
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C definitions for the Set component.
*
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_SET_NODE_DBG_STRUCT
*       DBG_SET_NODE_STRUCT
*       DBG_SET_STRUCT
*              
*   FUNCTIONS
*
*       DBG_SET_Initialize
*       DBG_SET_Node_Find
*       DBG_SET_Node_Add
*       DBG_SET_Node_Remove
*       DBG_SET_Node_Count
*
*   DEPENDENCIES
*                                                         
*       NONE                             
*                                                                      
*************************************************************************/

#ifndef DBG_SET_H
#define DBG_SET_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* set node ID - is an identifier for that may be associated with the
   node.  The underlying data type MUST be the largest possible
   since anything smaller could (potentially) limit the ID values that
   are assigned.  For this reason the UINT (variable) abstract data
   type is used.  
   
   NOTE: the Set component does NOT generate set node ID values.  It
   only accepts whatever value it is given and assigns it to the node
   after conversion of the ID value into the UINT type. */
   
typedef UINT                            DBG_SET_NODE_ID;

/* set node ID predefine value : any - indicates ANY ID value.  This
   value is used in conjunction with the API calls. */
   
#define DBG_SET_NODE_ID_ANY             ((UINT)-1)

/* set node ID predefined value : min - the minimum value that a set node
   ID may be.  This value is used for programming purposes. */
   
#define DBG_SET_NODE_ID_MIN             0

/* set node ID predefine value : max - the maximum value that a set node
   ID may be.  This value is used for programming purposes.
   
   NOTE: there is a relationship between the set location value and the
   set node ID value range since a set node ID may be passed as a set
   location value. */
   
#define DBG_SET_NODE_ID_MAX             ((UINT)-2)

/* set behavior - indicates the (default) behavior of the set. */

typedef enum _dbg_set_behavior_enum
{
    DBG_SET_BEHAVIOR_FIFO,      /* First-In-First-Out (queue) behavior. */
    DBG_SET_BEHAVIOR_LIFO,      /* Last-In-First-Out (stack) behavior. */
    DBG_SET_BEHAVIOR_ORDERED,   /* Ordered based on set node ID values. */
    DBG_SET_BEHAVIOR_INVALID    /* Indicates an invalid value. */

} DBG_SET_BEHAVIOR;

/* set location - indicates the location within a set that an operation
   will be performed on. */

typedef enum _dbg_set_location_eunm
{
    DBG_SET_LOCATION_HEAD,          /* First element in the current sets ordering scheme */
    DBG_SET_LOCATION_TAIL,          /* Last element in the current sets ordering scheme. */
    DBG_SET_LOCATION_NEXT,          /* Next node in a set in a circular fashion. */
    DBG_SET_LOCATION_PREV,          /* Previous node in a set in a circular fashion. */
    DBG_SET_LOCATION_NEXT_LINEAR,   /* Next node in a set in a linear fashion. */
    DBG_SET_LOCATION_PREV_LINEAR,   /* Previous node in a set in a linear fashion. */
    DBG_SET_LOCATION_DEFAULT,       /* Next node as determined by default set bahavior. */
    DBG_SET_LOCATION_INDEX,         /* NOT IMPLEMENTED */ 
    DBG_SET_LOCATION_INVALID        /* Indicates invalid set location. */

} DBG_SET_LOCATION;

/* location value - the location value is used in conjunction with the
   location values to specify a location within a set that an operation
   is to be performed on.  For example if the location is INDEX then 
   the location value would indicate the actual index value to be used. */
   
typedef UINT                            DBG_SET_LOCATION_VALUE;

/* set node - structure which allows any other structure to be added to
   a set when it is placed as the first member of the other structure. */

typedef struct _dbg_set_node_struct
{
    struct _dbg_set_node_struct *   p_next;     /* Pointer to next node in the list. */
    struct _dbg_set_node_struct *   p_prev;     /* Pointer to previous node in the list. */
    DBG_SET_NODE_ID                 node_id;    /* Identifier value for the node. */
    
} DBG_SET_NODE;

/* set - a single set. */

typedef struct _dbg_set_struct
{
    DBG_SET_NODE *          p_node_list_head;   /* Pointer to node list head. */
    DBG_SET_NODE *          p_node_list_tail;   /* Pointer to node list tail. */
    UINT                    node_count;         /* The current node count. */
    DBG_SET_BEHAVIOR        default_behavior;   /* The default behavior of the set. */

} DBG_SET;

/***** Global functions */

DBG_STATUS DBG_SET_Initialize(DBG_SET *            p_set,
                              DBG_SET_BEHAVIOR     default_behavior);

DBG_STATUS DBG_SET_Node_Find(DBG_SET *                  p_set,
                             DBG_SET_LOCATION           location,
                             DBG_SET_LOCATION_VALUE     location_value,
                             DBG_SET_NODE **            p_p_node);

DBG_STATUS DBG_SET_Node_Add(DBG_SET *                   p_set,
                            DBG_SET_LOCATION            location,
                            DBG_SET_LOCATION_VALUE      location_value,
                            DBG_SET_NODE_ID             node_id,
                            DBG_SET_NODE *              p_node);
                        
DBG_STATUS DBG_SET_Node_Remove(DBG_SET *                p_set,
                               DBG_SET_LOCATION         location,
                               DBG_SET_LOCATION_VALUE   location_value,
                               DBG_SET_NODE **          p_p_node);
                           
UINT DBG_SET_Node_Count(DBG_SET *    p_set);

#ifdef __cplusplus
}
#endif

#endif /* DBG_SET_H */
