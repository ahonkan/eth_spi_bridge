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
*       dbg_eng.h                                    
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C external interface for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_ENG_CB
*            
*   FUNCTIONS
*
*       None
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_ENG_H
#define DBG_ENG_H

/***** Global defines */

/* Debug Engine Illegal Instruction Stops Thread - This define determines
   if illegal instructions stop the thread that the hit occured on or the
   entire application.  A value of NU_TRUE indicates that the thread will
   be stopped when an illegal instruction is encountered.  A value of 
   NU_FALSE will stop the application.  The default value is NU_FALSE.
      
   NOTE: This functionality supports the Debug Agent Assert system.  */
   
#define DBG_ENG_ILLEGAL_INST_STOPS_THREAD           NU_FALSE

/* Debug Engine Illegal Instruction Replace - This define determines
   if an illegal instruction is replaced with a benign instruction (e.g.
   a NOP).  A value of NU_TRUE indicates that the illegal instruction is 
   replaced.  A value of NU_FALSE will leave the illegal instruction in
   place.  The default value is NU_TRUE.
      
   NOTE: This functionality supports the Debug Agent Assert system.  */
   
#define DBG_ENG_ILLEGAL_INST_REPLACE                NU_TRUE

/* Debug Engine Illegal Instruction Debug Event - This define determines
   if illegal instructions cause a debug event or not.  A value of
   NU_TRUE causes a debug event to occur and a value of NU_FALSE does
   not.  The default value is NU_TRUE. */
   
#define DBG_ENG_ILLEGAL_INST_DEBUG_EVENT            NU_TRUE

/* Debug Engine Checking Enabled - This define determines if the debug
   engine performs checking of all parameters passed into it or not.  A
   value of NU_TRUE enables checking and a value of NU_FALSE disables
   checking.  The default value is NU_TRUE. */
   
#define DBG_ENG_CHECKING_ENABLED                    NU_TRUE

/* Debug Engine control block. */

typedef struct _dbg_eng_cb_struct
{
    VOID *                          p_dbg;                  /* Service control block */
    DBG_ENG_BKPT_CB                 bkpt;                   /* Breakpoint control block. */
    DBG_ENG_REG_CB                  reg;                    /* Register control block. */
    DBG_ENG_MEM_CB                  mem;                    /* Memory control block. */
    DBG_ENG_EXEC_CB                 exec;                   /* Execution control block. */    
    DBG_ENG_EVT_CB                  evt;                    /* Event control block. */
    DBG_VERSION_ID                  version_id;             /* Service API version. */
    VOID *                          p_cmd_func_array;       /* Command function array. */
    UINT                            cmd_func_array_size;    /* Command function array size. */
    BOOLEAN                         checking_enabled;       /* Internal checking enabled. */   
    DBG_CMD_OP                      cur_op;                 /* Current executing operation. */
    NU_SEMAPHORE                    api_sem;                /* API semaphore. */
    NU_TASK **                      p_thd_list;             /* Pointer to thread list. */
    UINT                            thd_list_size;          /* Thread list size. */
    UINT                            thd_list_idx;           /* Thread list index. */
    
} DBG_ENG_CB;

/***** Global functions */

/* None */

#endif /* DBG_ENG_H */
