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
*       dbg_com_tcp.h
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Communication - Nucleus TCP
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C external interface for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                     
*       DBG_COM_DRV_NU_TCP_PORT                                  
*       DBG_COM_DRV_NU_TCP_DRV
*
*   FUNCTIONS
*
*       DBG_COM_DRV_NU_TCP_Register
*                                                                      
*   DEPENDENCIES
*                                                         
*       None                                    
*                                                                      
*************************************************************************/

#ifndef DBG_COM_TCP_H
#define DBG_COM_TCP_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CFG_NU_OS_NET_STACK_ENABLE

/***** Global defines */

/* Register a driver if Nucleus NET is enabled. */

#define DBG_COM_TCP_DRIVER_REGISTER                         DBG_COM_DRV_NU_TCP_Driver_Register

/* Server Backlog - Indicates the number of backlogged connections that
   may be queued for a the TCP server.  The default value is 2. */
   
#define DBG_COM_DRV_NU_TCP_SERVER_BACKLOG                   2

/* port control block. */

typedef struct _dbg_com_drv_nu_tcp_port_struct
{
    BOOLEAN                 is_valid;   /* Is Valid? */
    INT                     cli_sock;   /* Nucleus NET Client socket */
    struct addr_struct      cli_addr;   /* Nucleus NET Client address */
    INT                     serv_sock;  /* Nucleus NET Server socket */
    struct addr_struct      serv_addr;  /* Nucleus NET Server address */
    UINT                    port_num;   /* Port number */
    
} DBG_COM_DRV_NU_TCP_PORT;

/* control block */

typedef struct _dbg_com_drv_nu_tcp_drv_struct
{
    BOOLEAN                 is_active;  /* Is Active? */
    NU_SEMAPHORE            acc_sem;    /* Access Semaphore. */    
    
} DBG_COM_DRV_NU_TCP_DRV;

/***** Global functions */

DBG_STATUS   DBG_COM_DRV_NU_TCP_Driver_Register(DBG_COM_CB *        p_com,
                                                DBG_COM_DRV *       p_driver);

#else

/***** Global defines */

/* Do not register a driver if Nucleus NET is not enabled. */

#define DBG_COM_TCP_DRIVER_REGISTER                         NU_NULL

#endif /* CFG_NU_OS_NET_STACK_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* DBG_COM_TCP_H */
