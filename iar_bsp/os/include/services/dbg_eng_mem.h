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
*       dbg_eng_mem.h                                    
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Memory
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the declarations for the component.                        
*                                                                      
*   DATA STRUCTURES                                                      
*
*       DBG_ENG_MEM_COPY_PARAM
*       DBG_ENG_MEM_OP_CNCL_PARAM
*       DBG_ENG_MEM_INIT_PARAM
*       DBG_ENG_MEM_TERM_PARAM
*       DBG_ENG_MEM_CB
*            
*   FUNCTIONS
*
*       DBG_ENG_MEM_Initialize
*       DBG_ENG_MEM_Terminate
*       DBG_ENG_MEM_Memory_Copy
*       DBG_ENG_MEM_Operation_Cancel
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_ENG_MEM_H
#define DBG_ENG_MEM_H

/***** Global defines */

/* Memory Copy parameters */

typedef struct _dbg_eng_mem_copy_param_struct
{
    VOID *                  p_src;                  /* Source */
    VOID *                  p_dst;                  /* Destination */
    UINT                    copy_size;              /* Size */
    DBG_MEM_ACCESS_MODE     access_mode;            /* Access Mode */      
    UINT *                  p_actual_copy_size;     /* Actual Size */

} DBG_ENG_MEM_COPY_PARAM;

/* Operation Cancel parameters */

typedef struct _dbg_eng_mem_op_cncl_param_struct
{
    VOID *                  reserved;   /* Reserved for future development. */

} DBG_ENG_MEM_OP_CNCL_PARAM;

/* Initialize parameters */

typedef struct _dbg_eng_mem_init_param_struct
{
    VOID *                  p_dbg_eng;  /* Pointer to debug engine */

} DBG_ENG_MEM_INIT_PARAM;

/* Terminate parameters */

typedef struct _dbg_eng_mem_term_param_struct
{
    VOID *                  reserved;   /* Reserved for future development. */

} DBG_ENG_MEM_TERM_PARAM;

/* control block. */

typedef struct _dbg_eng_mem_cb_struct
{
    VOID *                  p_dbg_eng;  /* Debug Engine control block. */
    BOOLEAN                 op_ok;      /* Indicates if a current operation should proceed */
    
} DBG_ENG_MEM_CB;

/***** Global functions */

DBG_STATUS  DBG_ENG_MEM_Initialize(DBG_ENG_MEM_CB *              p_dbg_eng_mem,
                                   DBG_ENG_MEM_INIT_PARAM *      p_param);

DBG_STATUS  DBG_ENG_MEM_Terminate(DBG_ENG_MEM_CB *              p_dbg_eng_mem,
                                  DBG_ENG_MEM_TERM_PARAM *      p_param);

DBG_STATUS DBG_ENG_MEM_Memory_Copy(DBG_ENG_MEM_CB *           p_dbg_eng_mem,
                                   DBG_ENG_MEM_COPY_PARAM *   p_param);

DBG_STATUS DBG_ENG_MEM_Operation_Cancel(DBG_ENG_MEM_CB *                p_dbg_eng_mem,
                                        DBG_ENG_MEM_OP_CNCL_PARAM *     p_param);

#endif /* DBG_ENG_MEM_H */
