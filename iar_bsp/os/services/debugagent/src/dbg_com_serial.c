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
*       dbg_com_serial.c                                
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Communication - Nucleus Serial                                 
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C source code for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None
*                                                                      
*   FUNCTIONS                                                            
*       
*       dbg_com_drv_nu_serial_driver_initialize
*       dbg_com_drv_nu_serial_driver_terminate
*       dbg_com_drv_nu_serial_port_open
*       dbg_com_drv_nu_serial_port_close
*       dbg_com_drv_nu_serial_port_read
*       dbg_com_drv_nu_serial_port_write
*       dbg_com_drv_nu_serial_port_info
*
*       DBG_COM_DRV_NU_SERIAL_Driver_Register                 
*                                                    
*   DEPENDENCIES
*
*       dbg.h
*                                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"

#ifdef CFG_NU_OS_DRVR_SERIAL_ENABLE

/***** Local functions */

/* Local function prototypes */

static DBG_STATUS   dbg_com_drv_nu_serial_driver_initialize(DBG_COM_DRV *        p_driver);

static DBG_STATUS   dbg_com_drv_nu_serial_driver_terminate(DBG_COM_DRV *        p_driver);

static DBG_STATUS   dbg_com_drv_nu_serial_port_open(DBG_COM_DRV *       p_driver,
                                                    DBG_COM_PORT_DATA   port_data,
                                                    DBG_COM_PORT *      p_port);

static DBG_STATUS   dbg_com_drv_nu_serial_port_close(DBG_COM_PORT *         p_port);

static DBG_STATUS   dbg_com_drv_nu_serial_port_read(DBG_COM_PORT *          p_port,
                                                    VOID *                  p_data,
                                                    UINT                    data_size,
                                                    UINT *                  p_data_read_size);

static DBG_STATUS   dbg_com_drv_nu_serial_port_write(DBG_COM_PORT *         p_port,
                                                     VOID *                 p_data,
                                                     UINT                   data_size);

static DBG_STATUS   dbg_com_drv_nu_serial_port_info(DBG_COM_PORT *          p_port,
                                                    DBG_COM_PORT_INFO *     p_port_info);
                                                  
/* Local function definitions */

/* Global definitions */

static NU_SERIAL_PORT *serial_port;

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_driver_initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       Initializes the Agent communication driver.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_driver - Pointer to a driver control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory 
*                                           available to complete 
*                                           operation.
*
*       DBG_STATUS_FAILED - Indicates unable to create driver semaphore.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_driver_initialize(DBG_COM_DRV *        p_driver)
{
    DBG_STATUS                  dbg_status;
    STATUS                      nu_status;
    DBG_COM_DRV_NU_SERIAL_DRV * p_serial_drv;

    /* Get the standard IO serial port structure */
    serial_port = NU_SIO_Get_Port();
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_driver == NU_NULL)
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Allocate memory for a new serial driver. */
    
        p_serial_drv = DBG_System_Memory_Allocate(sizeof(DBG_COM_DRV_NU_SERIAL_DRV),
                                                  DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                  DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                  DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                  NU_NO_SUSPEND);    
        
        if (p_serial_drv == NU_NULL)
        {
            /* ERROR: insufficient memory. */
            
            dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
            
            /* ERROR RECOVERY: None */
            
        }      
    
    }      
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Create driver access semaphore. */
    
        nu_status = NU_Create_Semaphore(&p_serial_drv -> acc_sem,
                                        "DBG_DRV",
                                        1,
                                        NU_FIFO);
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to create driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
            /* ERROR RECOVERY: None */
            
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set initial driver state. */
        
        p_serial_drv -> is_active = NU_FALSE;
        
        /* Update Agent com driver with TCP driver data. */
        
        p_driver -> p_drv_data = p_serial_drv;

    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_driver_terminate
*                                                                      
*   DESCRIPTION   
*                                                       
*       Terminates the Agent communication driver.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_driver - Pointer to a driver control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_FAILED - Indicates unable to delete access semaphore or
*                           unable to free serial driver memory.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_driver_terminate(DBG_COM_DRV *        p_driver)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    DBG_COM_DRV_NU_SERIAL_DRV *     p_serial_drv;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_driver == NU_NULL)
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to TCP driver from Agent driver. */
    
        p_serial_drv = (DBG_COM_DRV_NU_SERIAL_DRV *)p_driver -> p_drv_data;
    
        /* Delete driver access semaphore. */
    
        nu_status = NU_Delete_Semaphore(&p_serial_drv -> acc_sem);
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        } 

    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Deallocate memory for the Serial driver. */
        
        dbg_status = DBG_System_Memory_Deallocate(p_serial_drv);    
  
    } 

    if (dbg_status == DBG_STATUS_OK)
    { 
        /* Update Agent com driver. */
        
        p_driver -> p_drv_data = NU_NULL;
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_port_open
*                                                                      
*   DESCRIPTION   
*                                                       
*       Creates a new Agent communication port and opens it.
*
*   INPUTS                                                               
*                                     
*       p_driver - Pointer to the com driver control block.
*                                 
*       port_data - Port type specific data used in opening a port.  For a
*                  Serial port this data is NOT USED.
*
*       p_port - Pointer to a port control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_FAILED - Indicates unable to obtain or release the
*                           driver semaphor.  OR Nucleus serial drivers
*                           are unavailable.
*       
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory to
*                                           complete the operation. OR a
*                                           com port has already been
*                                           created (only one allowed).
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_port_open(DBG_COM_DRV *       p_driver,
                                                    DBG_COM_PORT_DATA   port_data,
                                                    DBG_COM_PORT *      p_port)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;    
    DBG_COM_DRV_NU_SERIAL_DRV *     p_serial_drv;     
    DBG_COM_DRV_NU_SERIAL_PORT *    p_serial_port;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_driver == NU_NULL) ||
        (p_port == NU_NULL))
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to serial driver from Agent driver. */
    
        p_serial_drv = (DBG_COM_DRV_NU_SERIAL_DRV *)p_driver -> p_drv_data; 

        /* Obtain driver access semaphore, blocking if needed. */
        
        nu_status = NU_Obtain_Semaphore(&p_serial_drv -> acc_sem,
                                       NU_SUSPEND);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to obtain driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }    
    
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Allocate memory for a new serial com port. */
        
        p_serial_port = DBG_System_Memory_Allocate(sizeof(DBG_COM_DRV_NU_SERIAL_PORT),
                                                   DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                   DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                   DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                   NU_NO_SUSPEND);    
        
        if (p_serial_port == NU_NULL)
        {
            /* ERROR: insufficient memory. */
            
            dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
            
        }
  
    }
  
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set serial port state. */
        
        p_serial_port -> is_active = NU_TRUE;
        
        /* Setup Agent com port. */ 
        
        p_port -> p_drv = p_driver;
        p_port -> p_port_data = p_serial_port;
        
        /* Release driver access semaphore. */
        
        nu_status = NU_Release_Semaphore(&p_serial_drv -> acc_sem);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to release driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        } 

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_port_close
*                                                                      
*   DESCRIPTION   
*                                                       
*       Closes and deletes an existing Agent communication port.
*
*       NOTE: This is a potentially blocking function.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_port - Pointer to a port control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_FAILED - Indicates unable to obtain or release the
*                           driver semaphore or unable to free memory
*                           resources.  OR unable to abort or close socket
*                           resources.  OR no serial port currently
*                           active to delete.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the port already closed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_port_close(DBG_COM_PORT *        p_port)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    DBG_COM_DRV_NU_SERIAL_DRV *     p_serial_drv;
    DBG_COM_DRV_NU_SERIAL_PORT *    p_serial_port;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_port == NU_NULL)
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to driver control block. */
    
        p_serial_drv = (DBG_COM_DRV_NU_SERIAL_DRV *)p_port -> p_drv;
        
        /* Get pointer to serial port. */
        
        p_serial_port = (DBG_COM_DRV_NU_SERIAL_PORT *)p_port -> p_port_data;
        
        if (p_serial_port -> is_active == NU_FALSE)
        {
            /* ERROR: Operation is invalid since port is not active. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
            
        } 
        
    }         
        
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Obtain driver access semaphore, blocking if needed. */
    
        nu_status = NU_Obtain_Semaphore(&p_serial_drv -> acc_sem,
                                       NU_SUSPEND);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to obtain driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }     
   
    } 
   
    if (dbg_status == DBG_STATUS_OK)
    {        
        /* Mark port as inactive so that any pending (blocked) 
           operations on the port will be aware it is inactive if they
           are freed by the following socket operations. */
           
        p_serial_port -> is_active = NU_TRUE;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Deallocate memory for the serial port. */
        
        dbg_status = DBG_System_Memory_Deallocate(p_serial_port);    
  
    } 
  
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update com port. */ 
        
        p_port -> p_port_data = NU_NULL;
        
        /* Release driver access semaphore. */
        
        nu_status = NU_Release_Semaphore(&p_serial_drv -> acc_sem);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to release driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }   

    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_port_read
*                                                                      
*   DESCRIPTION   
*                                                       
*       Reads data from an Agent communication port.
*
*       NOTE: This is a potentially blocking function.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_port - Pointer to a port control block.
*
*       p_data - Pointer to data buffer for received data.
*
*       data_size - The maximum size (in bytes) of data that may be 
*                  received into the data buffer.
* 
*       p_data_read_size - Return parameter that will be updated to 
*                           contain the size (in bytes) of the data 
*                           received if the operation is successful.  If 
*                           the operation fails the value is undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the port is invalid.  OR
*                                      Port has been deleted.
*
*       DBG_STATUS_FAILED - Indicates that the operation failed.  OR
*                           unable to obtain serial Rx semaphore.  OR 
*                           unable to disable Rx process timer.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_port_read(DBG_COM_PORT *         p_port,
                                                     VOID *                 p_data,
                                                     UINT                   data_size,
                                                     UINT *                 p_data_read_size)
{
    DBG_STATUS                      dbg_status;
    DBG_COM_DRV_NU_SERIAL_PORT *    p_serial_port;
    UINT8 *                         pReadData;
    INT                             readDataSize;
    INT                             byte;
    INT                             start_flag = NU_FALSE;
    INT                             checksum_count = 0;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0) ||
        (p_data_read_size == NU_NULL))
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to Serial port. */
        
        p_serial_port = (DBG_COM_DRV_NU_SERIAL_PORT *)p_port -> p_port_data;
        
        if (p_serial_port -> is_active == NU_FALSE)
        {
            /* ERROR: Port is not active. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
            
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Process Rx data. */
              
        pReadData = (UINT8 *)p_data;
        readDataSize = 0;

        do
        {
            /* Stay here until we receive a character */
            while ((byte = NU_Serial_Getchar(serial_port)) == NU_SERIAL_NO_CHAR_AVAIL)
            {
                NU_Sleep(DBG_COM_DRV_NU_SERIAL_RX_POLL_TIME);
            }

            /* Is this an ACK character that is not part of a packet? */
            if ((start_flag == NU_FALSE) && (byte == RSP_ACK_CHAR))
            {
                /* Yes, create a one byte packet */
                readDataSize = 0;

                /* Put the ACK character into the buffer */
                pReadData[readDataSize] = (UINT8)byte;
                readDataSize++;

                /* Reset the packet processing flags */
                start_flag = NU_FALSE;
                checksum_count = 0;

                /* Exit the loop and process the packet */
                break;
            }

            /* Is this a NACK character that is not part of a packet? */
            if ((start_flag == NU_FALSE) && (byte == RSP_NO_ACK_CHAR))
            {
                /* Yes, create a one byte packet */
                readDataSize = 0;

                /* Put the NACK character into the buffer */
                pReadData[readDataSize] = (UINT8)byte;
                readDataSize++;

                /* Reset the packet processing flags */
                start_flag = NU_FALSE;
                checksum_count = 0;

                /* Exit the loop and process the packet */
                break;
            }

            /* Is this a Halt character that is not part of a packet? */
            if ((start_flag == NU_FALSE) && (byte == RSP_HALT_PKT_CHAR))
            {
                /* Yes, create a one byte packet */
                readDataSize = 0;

                /* Put the HALT character into the buffer */
                pReadData[readDataSize] = (UINT8)byte;
                readDataSize++;

                /* Reset the packet processing flags */
                start_flag = NU_FALSE;
                checksum_count = 0;

                /* Exit the loop and process the packet */
                break;
            }

            /* Is this a start packet command? */
            else if (byte == RSP_PKT_START_CHAR)
            {
                /* Yes, reset the packet buffer index */
                readDataSize = 0;

                /* Put the START character into the buffer */
                pReadData[readDataSize] = (UINT8)byte;
                readDataSize++;

                /* Show that we are building a new packet */
                start_flag = NU_TRUE;
                checksum_count = 0;
            }

            /* Is this a legal end packet command? */
            else if ((start_flag == NU_TRUE) && (byte == RSP_PKT_END_CHAR))
            {
                /* Put the END character into the buffer */
                pReadData[readDataSize] = (UINT8)byte;
                readDataSize++;

                /* Show that we need to read in the 2 byte checksum */
                checksum_count = DBG_COM_DRV_NU_SERIAL_RSP_PKT_CHKSUM_SIZE;
            }

            /* Build the packet */
            else
            {
                /* If we found a START character, build the rest of the packet */
                if (start_flag == NU_TRUE)
                {
                    /* Put the received character into the buffer */
                    pReadData[readDataSize] = (UINT8)byte;
                    readDataSize++;

                    /* If we found the END character, this must be one of the checksum bytes */
                    if (checksum_count != 0)
                    {
                        /* Did we receive 2 checksum bytes? */
                        if (--checksum_count == 0)
                        {
                            /* Yes. Reset the packet processing flag */
                            start_flag = NU_FALSE;

                            /* Exit the loop and process the packet */
                            break;
                        }
                    }
                }
            }

        /* If we receive more data than the buffer can hold, exit the loop */
        } while (readDataSize < data_size);
    }

    if (dbg_status == DBG_STATUS_OK)
    {        
        /* Update return parameters. */

        *p_data_read_size = readDataSize;
        
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_port_write
*                                                                      
*   DESCRIPTION   
*                                                       
*       Writes data to an Agent communication port.
*
*       NOTE: This is a potentially blocking function.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_port - Pointer to a port control block.
*
*       p_data - Pointer to data to be sent.
*
*       data_size - Size of the data to be sent.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the port is invalid.
*
*       DBG_STATUS_FAILED - Indicates data could not be sent.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_port_write(DBG_COM_PORT *        p_port,
                                                      VOID *                p_data,
                                                      UINT                  data_size)
{
    DBG_STATUS                      dbg_status;    
    DBG_COM_DRV_NU_SERIAL_PORT *    p_serial_port;
    UINT8 *                         pWriteData;
    UINT                            i;
    INT                             byte; 

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0))
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Get pointer to serial port. */
        
        p_serial_port = (DBG_COM_DRV_NU_SERIAL_PORT *)p_port -> p_port_data;
            
        if (p_serial_port -> is_active == NU_FALSE)
        {
            /* ERROR: Port is not active. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
            
        } 
   
    } 
   
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Loop writing all data to the Nucleus PLUS serial driver. */
        
        pWriteData = (UINT8 *)p_data;
        i = 0;
        while ((dbg_status == DBG_STATUS_OK) &&
               (i < data_size))
        {
            byte = NU_SIO_Putchar((INT)pWriteData[i]);
                             
            if (byte == NU_EOF)
            {
                /* ERROR: Character transmit failed. */
                
                dbg_status = DBG_STATUS_OK;
                
            }
            else
            {
                i++;
                
            } 
              
        } 

    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_serial_port_info
*                                                                      
*   DESCRIPTION   
*                                                       
*       Information about an Agent communication port.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_port - Pointer to a port control block.
*
*       p_port_info - Pointer to a port information structure that will be
*                   updated with information upon successful operation.
*                   If the operation fails the value is undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port is invalid.
*
*       DBG_STATUS_FAILED - Indicates information could not be obtained.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_serial_port_info(DBG_COM_PORT *            p_port,
                                                     DBG_COM_PORT_INFO *       p_port_info)
{
    DBG_STATUS                      dbg_status;
    DBG_COM_DRV_NU_SERIAL_PORT *    p_serial_port;
    UINT                            str_copy_len;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_port_info == NU_NULL))
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to serial port control block. */
        
        p_serial_port = (DBG_COM_DRV_NU_SERIAL_PORT *)p_port -> p_port_data;
    
        if (p_serial_port -> is_active == NU_FALSE)
        {
            /* ERROR: Port is not active. */
            
            dbg_status = DBG_STATUS_INVALID_RESOURCE;
            
            /* ERROR RECOVERY: None */
            
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Indicate a serial port until more detail information is 
           available through the Nucleus serial API... */

        p_port_info -> type = DBG_COM_PORT_TYPE_NU_SERIAL;
        strncpy(&p_port_info -> desc_str[0],
                "Nucleus Serial",
                DBG_COM_PORT_DESC_STR_SIZE);
        str_copy_len = strlen(&p_port_info -> desc_str[0]);
        
        if (str_copy_len == 0)
        {
            /* ERROR: Unable to copy information string. */
            
            dbg_status = DBG_STATUS_FAILED;
            
        } 
        
    } 

    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_COM_DRV_NU_SERIAL_Driver_Register
*                                                                      
*   DESCRIPTION   
*                                                       
*       Registers driver with Agent communications system.
*                                                                                                 
*   INPUTS                                                               
*                        
*       p_com - Pointer to the communications control block.
*                                              
*       p_driver - Pointer to a driver control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameters detected.
*                                                                      
*************************************************************************/
DBG_STATUS   DBG_COM_DRV_NU_SERIAL_Driver_Register(DBG_COM_CB *         p_com,
                                                   DBG_COM_DRV *        p_driver)
{
    DBG_STATUS      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if ((p_com == NU_NULL) ||
        (p_driver == NU_NULL))
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup driver control block. */
        
        p_driver -> p_dbg_com = (VOID *)p_com;
        p_driver -> p_drv_data = (VOID *)NU_NULL;
        p_driver -> drv_init_func = (VOID *)dbg_com_drv_nu_serial_driver_initialize;
        p_driver -> drv_term_func = (VOID *)dbg_com_drv_nu_serial_driver_terminate;
        p_driver -> port_open_func = (VOID *)dbg_com_drv_nu_serial_port_open;
        p_driver -> port_close_func = (VOID *)dbg_com_drv_nu_serial_port_close;
        p_driver -> port_write_func = (VOID *)dbg_com_drv_nu_serial_port_write;        
        p_driver -> port_read_func = (VOID *)dbg_com_drv_nu_serial_port_read;        
        p_driver -> port_info_func = (VOID *)dbg_com_drv_nu_serial_port_info;        

    } 

    return (dbg_status);
}

#endif /* CFG_NU_OS_DRVR_SERIAL_ENABLE */
