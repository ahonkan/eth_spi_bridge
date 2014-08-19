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
*       dbg_com_serial.h                                
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Communication - Nucleus Serial                                 
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C source code declarations for the 
*       component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_COM_DRV_NU_SERIAL_PORT                                  
*       DBG_COM_DRV_NU_SERIAL_DRV
*                                                                      
*   FUNCTIONS                                                            
*       
*       DBG_COM_DRV_NU_SERIAL_Register
*                                                                      
*   DEPENDENCIES
*
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_COM_SERIAL_H
#define DBG_COM_SERIAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CFG_NU_OS_DRVR_SERIAL_ENABLE

/***** Global defines */

/* Register a driver if Nucleus Serial is enabled. */

#define DBG_COM_SERIAL_DRIVER_REGISTER                      DBG_COM_DRV_NU_SERIAL_Driver_Register

/* Serial Rx Time - The time duration (in system ticks) between polls 
   during Rx operations.  The default value is:
   (NU_PLUS_TICKS_PER_SEC / 100) */

#define DBG_COM_DRV_NU_SERIAL_RX_POLL_TIME                  (NU_PLUS_TICKS_PER_SEC / 100) 

/* RSP Packet Defines - The following defines are used to recognize and
   process RSP packets within a serial data stream. */

#define DBG_COM_DRV_NU_SERIAL_RSP_PKT_CHKSUM_SIZE           2

/* Serial Port */

typedef struct _dbg_com_drv_nu_serial_port_struct
{
    BOOLEAN             is_active;      /* Is Active? */    

} DBG_COM_DRV_NU_SERIAL_PORT;

/* Serial Driver */ 

typedef struct _dbg_com_drv_nu_serial_drv_struct
{
    BOOLEAN             is_active;      /* Is Active? */
    NU_SEMAPHORE        acc_sem;        /* Access Semaphore. */     
    
} DBG_COM_DRV_NU_SERIAL_DRV;

/***** Global functions */

DBG_STATUS   DBG_COM_DRV_NU_SERIAL_Driver_Register(DBG_COM_CB *         p_com,
                                                   DBG_COM_DRV *        p_driver);

#else

/* Do not register a driver if Nucleus Serial is not enabled. */

#define DBG_COM_SERIAL_DRIVER_REGISTER                      NU_NULL

#endif /* CFG_NU_OS_DRVR_SERIAL_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* DBG_COM_SERIAL_H */

