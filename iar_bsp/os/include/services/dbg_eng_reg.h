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
*       dbg_eng_reg.h                                    
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Registers
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the declarations for the component.                        
*                                                                      
*   DATA STRUCTURES                                                      
*                             
*       DBG_ENG_REG_INIT_PARAM
*       DBG_ENG_REG_TERM_PARAM
*       DBG_ENG_REG_READ_PARAM
*       DBG_ENG_REG_WRITE_PARAM
*       DBG_ENG_REG_CB
*            
*   FUNCTIONS
*
*       DBG_ENG_REG_Initialize
*       DBG_ENG_REG_Terminate
*       DBG_ENG_REG_Register_Read
*       DBG_ENG_REG_Register_Write
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_ENG_REG_H
#define DBG_ENG_REG_H

/***** Global defines */

/* Register - Initialize parameters */

typedef struct _dbg_eng_reg_init_param_struct
{
    VOID *                      p_dbg_eng;      /* Pointer to debug engine */
    
} DBG_ENG_REG_INIT_PARAM;

/* Register - Terminate parameters */

typedef struct _dbg_eng_reg_term_param_struct
{
    VOID *                      reserved;       /* Reserved for future development */
    
} DBG_ENG_REG_TERM_PARAM;

/* Register - Read parameters */

typedef struct _dbg_eng_reg_read_param_struct
{
    NU_TASK *                   p_os_thread;            /* Thread to read values from */
    DBG_REGISTER_ID             reg_id;                 /* ID of registers to read */
    VOID *                      p_reg_data;             /* Location to read value into */
    UINT *                      p_actual_reg_data_size; /* Returned size of read values */
                                     
} DBG_ENG_REG_READ_PARAM;

/* Register - Write parameters */

typedef struct _dbg_eng_reg_write_param_struct
{
    NU_TASK *                   p_os_thread;    /* Thread to write values to */         
    DBG_REGISTER_ID             reg_id;         /* ID of registers to write */
    VOID *                      p_reg_data;     /* Location to write values from */
    
} DBG_ENG_REG_WRITE_PARAM;

/* control block. */

typedef struct _dbg_eng_reg_cb_struct
{
    VOID *                      p_dbg_eng;      /* Pointer to debug engine */
                                       
} DBG_ENG_REG_CB;

/***** Global functions */

DBG_STATUS  DBG_ENG_REG_Initialize(DBG_ENG_REG_CB *             p_dbg_eng_reg,
                                   DBG_ENG_REG_INIT_PARAM *     p_param);

DBG_STATUS  DBG_ENG_REG_Terminate(DBG_ENG_REG_CB *              p_dbg_eng_reg,
                                  DBG_ENG_REG_TERM_PARAM *      p_param);

DBG_STATUS  DBG_ENG_REG_Register_Read(DBG_ENG_REG_CB *          p_dbg_eng_reg,
                                      DBG_ENG_REG_READ_PARAM *  p_param);

DBG_STATUS  DBG_ENG_REG_Register_Write(DBG_ENG_REG_CB *             p_dbg_eng_reg,
                                       DBG_ENG_REG_WRITE_PARAM *    p_param);

#endif /* DBG_ENG_REG_H */
