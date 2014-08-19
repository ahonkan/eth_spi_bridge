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
*       dbg_com_tcp.c
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Communication - Nucleus TCP
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
*       dbg_com_drv_nu_tcp_setup_server
*       dbg_com_drv_nu_tcp_check_client
*       dbg_com_drv_nu_tcp_driver_initialize
*       dbg_com_drv_nu_tcp_driver_terminate
*       dbg_com_drv_nu_tcp_port_open
*       dbg_com_drv_nu_tcp_port_close
*       dbg_com_drv_nu_tcp_port_write
*       dbg_com_drv_nu_tcp_port_read
*       dbg_com_drv_nu_tcp_port_info
*                     
*       DBG_COM_DRV_NU_TCP_Driver_Register
*                                                 
*   DEPENDENCIES
*
*       dbg.h
*       nu_net.h
*       runlevel.h
*                                                                      
*************************************************************************/

/***** include files */

#include "services/dbg.h"
#ifdef CFG_NU_OS_SVCS_DBG_ADV_ENABLE
#include "os/services/init/inc/runlevel.h"
#endif

#ifdef CFG_NU_OS_NET_STACK_ENABLE

/***** local functions */

/* Function prototypes */

static DBG_STATUS   dbg_com_drv_nu_tcp_setup_server(DBG_COM_DRV_NU_TCP_PORT *    p_tcp_port);

static DBG_STATUS   dbg_com_drv_nu_tcp_check_client(DBG_COM_DRV_NU_TCP_PORT *    p_tcp_port);

static DBG_STATUS   dbg_com_drv_nu_tcp_driver_initialize(DBG_COM_DRV *        p_driver);

static DBG_STATUS   dbg_com_drv_nu_tcp_driver_terminate(DBG_COM_DRV *        p_driver);

static DBG_STATUS   dbg_com_drv_nu_tcp_port_open(DBG_COM_DRV *       p_driver,
                                                 DBG_COM_PORT_DATA   port_data,
                                                 DBG_COM_PORT *      p_port);

static DBG_STATUS   dbg_com_drv_nu_tcp_port_close(DBG_COM_PORT *        p_port);

static DBG_STATUS   dbg_com_drv_nu_tcp_port_read(DBG_COM_PORT *         p_port,
                                                 VOID *                 p_data,
                                                 UINT                   data_size,
                                                 UINT *                 p_dataReadSize);

static DBG_STATUS   dbg_com_drv_nu_tcp_port_write(DBG_COM_PORT *        p_port,
                                                  VOID *                p_data,
                                                  UINT                  data_size);

static DBG_STATUS   dbg_com_drv_nu_tcp_port_info(DBG_COM_PORT *            p_port,
                                                 DBG_COM_PORT_INFO *       p_port_info);

#ifdef CFG_NU_OS_SVCS_DBG_ADV_ENABLE
extern CHAR    DBG_ADV_Regpath[];
#endif

/* Function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_setup_server
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function sets up a TCP server on a socket and waits for a 
*       client connection.
*                                                                    
*       NOTE : this is potentially a BLOCKING function.
*                             
*   INPUTS                                                               
*                                                                      
*       p_tcp_port - Pointer to a TCP port structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_FAILED - Indicates operation failed (server not 
*                           created).
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_setup_server(DBG_COM_DRV_NU_TCP_PORT *    p_tcp_port)
{
    DBG_STATUS  dbg_status;
    INT         ret;
    STATUS      nu_status;

    /* Set initial state to success. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_tcp_port == NU_NULL)
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* The current server socket is invalid, open a new socket. */
        
        p_tcp_port -> serv_sock = NU_Socket(NU_FAMILY_IP, 
                                            NU_TYPE_STREAM, 
                                            NU_NONE);
                                         
        if (p_tcp_port -> serv_sock < 0)
        {
            /* ERROR: Unable to create new socket. */
        
            dbg_status = DBG_STATUS_FAILED;
        
        } 
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Disable the Naigle algorithm. */
            
            nu_status = NU_Push(p_tcp_port -> serv_sock);        
        
            if (nu_status != NU_SUCCESS)
            {
                /* ERROR: Nucleus NET Set Socket Option NODELAY failed. */
                
                dbg_status = DBG_STATUS_FAILED;
                
            } 
            
        } 
        
        if (dbg_status == DBG_STATUS_OK)
        {        
            /* Fill in a structure with the server address. */
            
            p_tcp_port -> serv_addr.family = NU_FAMILY_IP;
            p_tcp_port -> serv_addr.port = p_tcp_port -> port_num;
            *(UINT32 *)p_tcp_port -> serv_addr.id.is_ip_addrs = IP_ADDR_ANY;
            p_tcp_port -> serv_addr.name = "DBG_COM";

            /* Make a call to bind the server's address to this socket. */
               
            ret = NU_Bind(p_tcp_port -> serv_sock, 
                          &p_tcp_port -> serv_addr, 
                          0);
            
            if (ret < 0)
            {
                /* ERROR : Nucleus NET bind failed */
            
                dbg_status = DBG_STATUS_FAILED;
            
            } 
            
        }         
        
        if (dbg_status == DBG_STATUS_OK)
        {        
            /* Be ready to accept connection requests.  Note that we 
               allow a backlog of two only so that this thread will 
               suspend waiting for another connection if necessary.  
               We actually only allow 1 at a time, but don't want 
               this thread to poll. */
               
            nu_status = NU_Listen(p_tcp_port -> serv_sock, 
                                  DBG_COM_DRV_NU_TCP_SERVER_BACKLOG);

            if (nu_status != NU_SUCCESS)
            {
                /* ERROR : Nucleus NET listen failed. */
            
                dbg_status = DBG_STATUS_FAILED;
            
            } 
        } 
            
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_check_client
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function waits for a client connection if one is not present.
*       It is assumed that there is already a valid server socket created
*       and there is NOT a valid client socket yet.
*                                                                    
*       NOTE : this is potentially a BLOCKING function.
*                             
*   INPUTS                                                               
*                                                                      
*       p_tcp_port - Pointer to a TCP port structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_FAILED - Indicates operation failed (server not 
*                           created). 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_check_client(DBG_COM_DRV_NU_TCP_PORT *    p_tcp_port)
{
    DBG_STATUS  dbg_status;
    STATUS      nu_status;

    /* Set initial state to success. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_tcp_port == NU_NULL)
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Block in accept until a client attempts connection. */
        
        p_tcp_port -> cli_sock = NU_Accept(p_tcp_port -> serv_sock, 
                                          &p_tcp_port -> cli_addr,
                                          0);
        
        if (p_tcp_port -> cli_sock < 0)
        {
            /* ERROR: Accept returned an error. */
            
            dbg_status = DBG_STATUS_FAILED;
            
        }             
        
    }     

    if (dbg_status == DBG_STATUS_OK)
    {
#ifdef CFG_NU_OS_SVCS_DBG_ADV_ENABLE
        /* Shutdown the advertising service so we no longer send
           messages over the serial port */
        (VOID) NU_RunLevel_Component_Control(DBG_ADV_Regpath, 0);
#endif                    		
		
        /* Turn on blocking on reads for the new client 
           connection. */
        
        nu_status = NU_Fcntl(p_tcp_port -> cli_sock, 
                             NU_SETFLAG, 
                             NU_BLOCK);
        
        /* Update return status. */
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to turn on blocking reads for new client
               connection. */
               
            dbg_status = DBG_STATUS_FAILED;
            
        } 

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_driver_initialize
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
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory 
*                                           available to complete 
*                                           operation.
*
*       DBG_STATUS_FAILED - Indicates unable to create driver semaphore or
*                           unable to synchronize with Nucleus NET.
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_driver_initialize(DBG_COM_DRV *        p_driver)
{
    DBG_STATUS                  dbg_status;
    STATUS                      nu_status;
    DBG_COM_DRV_NU_TCP_DRV *    p_tcp_drv;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_driver == NU_NULL)
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }      
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Allocate memory for a new TCP com driver. */
        
        p_tcp_drv = DBG_System_Memory_Allocate(sizeof(DBG_COM_DRV_NU_TCP_DRV),
                                               DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                               DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                               DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                               NU_NO_SUSPEND);    
        
        if (p_tcp_drv == NU_NULL)
        {
            /* ERROR: insufficient memory. */
            
            dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
            
        }      
    
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Create driver access semaphore. */
    
        nu_status = NU_Create_Semaphore(&p_tcp_drv -> acc_sem,
                                        "DBG_DRV",
                                        1,
                                        NU_FIFO);    
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to create driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set initial driver state. */
        
        p_tcp_drv -> is_active = NU_FALSE;
        
        /* Update Agent com driver with TCP driver data. */
        
        p_driver -> p_drv_data = p_tcp_drv;

    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_driver_terminate
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
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_FAILED - Indicates unable to delete access semaphore or
*                           unable to free TCP driver memory.
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_driver_terminate(DBG_COM_DRV *        p_driver)
{
    DBG_STATUS                  dbg_status;
    STATUS                      nu_status;
    DBG_COM_DRV_NU_TCP_DRV *    p_tcp_drv;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_driver == NU_NULL)
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }      
    
    if(dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to TCP driver from Agent driver. */

        p_tcp_drv = (DBG_COM_DRV_NU_TCP_DRV *)p_driver -> p_drv_data;

        /* Delete driver access semaphore. */

        nu_status = NU_Delete_Semaphore(&p_tcp_drv -> acc_sem);

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete driver access semaphore. */

            dbg_status = DBG_STATUS_FAILED;
        }
    } 

    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Deallocate memory for the TCP driver. */
        
        dbg_status = DBG_System_Memory_Deallocate(p_tcp_drv);    
  
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
*       dbg_com_drv_nu_tcp_port_open
*                                                                      
*   DESCRIPTION   
*                                                       
*       Opens a new Agent communication port.
*
*       NOTE: This is a potentially blocking function.
*                                                                                                 
*   INPUTS                                                               
*                                     
*       p_driver - Pointer to the com driver control block.
*
*       port_data - Port type specific data used in opening a port.  For a
*                  TCP port this will be the port number.
*                                 
*       p_port - Pointer to a port control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_FAILED - Indicates unable to obtain or release the
*                           driver semaphore.  OR the Nucleus NET or 
*                           drivers are unavailable.
*       
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory to
*                                           complete the operation.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_port_open(DBG_COM_DRV *       p_driver,
                                                 DBG_COM_PORT_DATA   port_data,
                                                 DBG_COM_PORT *      p_port)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    UINT                            port_num;
    DBG_COM_DRV_NU_TCP_DRV *        p_tcp_drv;     
    DBG_COM_DRV_NU_TCP_PORT *       p_tcp_port;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Translate port data. */
    
    port_num = (UINT)port_data;
    
    if ((p_driver == NU_NULL) ||
        (p_port == NU_NULL) ||
        (port_num > 65535))
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }    
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get local pointer to TCP driver from Agent driver. */
    
        p_tcp_drv = (DBG_COM_DRV_NU_TCP_DRV *)p_driver -> p_drv_data;
        
        /* Synchronize with Nucleus NET only if the driver is in an 
           in-active state ('lazy behavior'). */
        
        if (p_tcp_drv -> is_active == NU_FALSE)
        {     
            
            /* Indicate driver is now synced to avoid sync again. */
                
            p_tcp_drv -> is_active = NU_TRUE;               
    
        }
        
        /* Obtain driver access semaphore, blocking if needed. */

        nu_status = NU_Obtain_Semaphore(&p_tcp_drv -> acc_sem,
                                        NU_SUSPEND);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to obtain driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }
    
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Allocate memory for a new TCP com port. */
        
        p_tcp_port = DBG_System_Memory_Allocate(sizeof(DBG_COM_DRV_NU_TCP_PORT),
                                                DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                NU_NO_SUSPEND);    
        
        if (p_tcp_port == NU_NULL)
        {
            /* ERROR: insufficient memory. */
            
            dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
            
        }
  
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup the TCP com port. */
           
        p_tcp_port -> port_num = port_num;
        
        /* Setup Agent com port. */ 
        
        p_port -> p_drv = p_driver;
        p_port -> p_port_data = p_tcp_port;

        /* Release driver access semaphore. */
        
        nu_status = NU_Release_Semaphore(&p_tcp_drv -> acc_sem);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to release driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup server. */
        
        dbg_status = dbg_com_drv_nu_tcp_setup_server(p_tcp_port);
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Ensure a client is connected. */
        
        dbg_status = dbg_com_drv_nu_tcp_check_client(p_tcp_port);
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Indicate port is valid. */
        
        p_tcp_port -> is_valid = NU_TRUE;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_port_close
*                                                                      
*   DESCRIPTION   
*                                                       
*       Closes an existing Agent communication port.
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
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_FAILED - Indicates unable to obtain or release the
*                           driver semaphore or unable to free memory
*                           resources.  Or unable to abort or close socket
*                           resources.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port is invalid.
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_port_close(DBG_COM_PORT *        p_port)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    DBG_COM_DRV *                   p_drv;
    DBG_COM_DRV_NU_TCP_DRV *        p_tcp_drv;
    DBG_COM_DRV_NU_TCP_PORT *       p_tcp_port;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_port == NU_NULL)
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }      
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Get local pointer to driver control block. */
    
        p_drv = (DBG_COM_DRV *)p_port -> p_drv;
        p_tcp_drv = (DBG_COM_DRV_NU_TCP_DRV *)p_drv -> p_drv_data;
        
        /* Get pointer to TCP port. */
        
        p_tcp_port = (DBG_COM_DRV_NU_TCP_PORT *)p_port -> p_port_data;
        
        /* Obtain driver access semaphore, blocking if needed. */

        nu_status = NU_Obtain_Semaphore(&p_tcp_drv -> acc_sem,
                                        NU_SUSPEND);    
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to obtain driver access semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }      
   
    } 
   
    if (dbg_status == DBG_STATUS_OK)
    {        
        /* Mark TCP port as invalid so that any pending (blocked) 
           operations on the port will be aware it is inactive if they
           are freed by the following socket operations. */
           
        p_tcp_port -> is_valid = NU_FALSE;
        
        /* Abort any client connection taking into account that the client
           socket may already be disconnected due to an error. */
        
        nu_status = NU_Abort(p_tcp_port -> cli_sock);
    
        if ((nu_status != NU_SUCCESS) &&
            (nu_status != NU_NO_PORT_NUMBER))
        {
            /* ERROR: Unable to abort client connection. */
            
            dbg_status = DBG_STATUS_FAILED;
    
        } 
    
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Close the server socket. */
        
        nu_status = NU_Close_Socket(p_tcp_port -> serv_sock);
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to close server socket. */
            
            dbg_status = DBG_STATUS_FAILED;
    
        }     
    
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {     
        /* Deallocate memory for the TCP port. */
        
        dbg_status = DBG_System_Memory_Deallocate(p_port -> p_port_data);    
  
    } 
  
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update com port. */ 
        
        p_port -> p_port_data = NU_NULL;        
        
        /* Release driver access semaphore. */
        
        nu_status = NU_Release_Semaphore(&p_tcp_drv -> acc_sem);    
        
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
*       dbg_com_drv_nu_tcp_port_read
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
*       p_dataReceivedSize - Return parameter that will be updated to 
*                           contain the size (in bytes) of the data 
*                           received if the operation is successful.  If 
*                           the operation fails the value is undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port is invalid.
*
*       DBG_STATUS_FAILED - Indicates that the operation failed.
*
*       DBG_STATUS_INVALID_CONNECTION - Indicates the connection was lost
*                                       or the socket closed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_port_read(DBG_COM_PORT *         p_port,
                                                  VOID *                 p_data,
                                                  UINT                   data_size,
                                                  UINT *                 p_dataReadSize)
{
    DBG_STATUS                      dbg_status;
    DBG_COM_DRV_NU_TCP_PORT *       p_tcp_port;
    INT32                           bytes_read; 

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0) ||
        (p_dataReadSize == NU_NULL))
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }      
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Get pointer to TCP port. */
        
        p_tcp_port = (DBG_COM_DRV_NU_TCP_PORT *)p_port -> p_port_data;
                
        if (p_tcp_port -> is_valid == NU_FALSE)
        {
            /* ERROR: Port is not valid. */
            
            dbg_status = DBG_STATUS_INVALID_RESOURCE;
            
        } 

    }    
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Attempt to receive data. */
    
        bytes_read = NU_Recv(p_tcp_port -> cli_sock,
                             p_data, 
                             data_size,
                             0);
    
        if (bytes_read < 0)
        {
            /* Interpret the bytes read value as an error status. */
            
            switch (bytes_read)
            {
                case NU_NOT_CONNECTED :
                case NU_SOCKET_CLOSED :                
                {
                    /* ERROR: Client socket disconnected. */
                    
                    dbg_status = DBG_STATUS_INVALID_CONNECTION;
                    
                    break;
                    
                }
                
                default :
                {
                    /* ERROR: A socket error has been encountered. */
                    
                    dbg_status = DBG_STATUS_FAILED;
                    
                    break;
                    
                }
                
            }

            /* Mark the port as invalid. */
            
            p_tcp_port -> is_valid = NU_FALSE;

        }
        else
        {
            /* Update return parameters. */
            
            *p_dataReadSize = (UINT)bytes_read;
            
        } 
        
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_port_write
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
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port is invalid.
*
*       DBG_STATUS_FAILED - Indicates data not sent.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_port_write(DBG_COM_PORT *        p_port,
                                                   VOID *                p_data,
                                                   UINT                  data_size)
{
    DBG_STATUS                      dbg_status;
    INT32                           bytes_sent;    
    DBG_COM_DRV_NU_TCP_PORT *       p_tcp_port; 

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0))
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
        /* ERROR RECOVERY: None */
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Get pointer to TCP port. */
        
        p_tcp_port = (DBG_COM_DRV_NU_TCP_PORT *)p_port -> p_port_data;
                
        if (p_tcp_port -> is_valid == NU_FALSE)
        {
            /* ERROR: Port is not valid. */
            
            dbg_status = DBG_STATUS_INVALID_RESOURCE;
            
        } 

    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Attempt to send the data using the client socket. */
    
        bytes_sent = NU_Send(p_tcp_port -> cli_sock,
                             (CHAR *)p_data,
                             (UINT16)data_size,
                             0);

        if ((bytes_sent < 0) ||
            (bytes_sent != data_size))
        {
            /* ERROR: An error was encountered. */
            
            dbg_status = DBG_STATUS_FAILED;
            
            /* Mark the port as invalid. */
            
            p_tcp_port -> is_valid = NU_FALSE;
            
        } 

    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_drv_nu_tcp_port_info
*                                                                      
*   DESCRIPTION   
*                                                       
*       Information about an Agent communication port.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_port - Pointer to a port control block.
*
*       p_portInfo - Pointer to a port information structure that will be
*                   updated with information upon successful operation.
*                   If the operation fails the value is undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port is invalid.
*
*       DBG_STATUS_FAILED - Indicates server address could not be 
*                           obtained.
*                                                                      
*************************************************************************/
static DBG_STATUS   dbg_com_drv_nu_tcp_port_info(DBG_COM_PORT *            p_port,
                                                 DBG_COM_PORT_INFO *       p_port_info)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;  
    DBG_COM_DRV_NU_TCP_PORT *       p_tcp_port;
    CHAR                            ip_addr_str[4];
    CHAR                            ip_port_str[6];
    UINT                            i;    
    struct sockaddr_struct          server_addr;
    INT16                           server_addr_size;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_port == NU_NULL) ||
        (p_port_info == NU_NULL))
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Get local pointer to TCP port control block. */
        
        p_tcp_port = (DBG_COM_DRV_NU_TCP_PORT *)p_port -> p_port_data;        
        
        if (p_tcp_port -> is_valid == NU_FALSE)
        {
            /* ERROR: Port is not valid. */
            
            dbg_status = DBG_STATUS_INVALID_RESOURCE;
            
        } 

    }    
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get server address (socket to the client). */
        
        server_addr_size = sizeof(server_addr);
        nu_status = NU_Get_Sock_Name(p_tcp_port -> cli_sock,
                                     &server_addr,
                                     &server_addr_size);
        
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to get server address. */
            
            dbg_status = DBG_STATUS_FAILED;
            
        } 
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Update port information with port type. */
        
        p_port_info -> type = DBG_COM_PORT_TYPE_NU_TCP_IP;
        
        /* Update port information with port description string. */
        
        p_port_info -> desc_str[0] = NU_NULL;
        
        (VOID)strcat(&p_port_info -> desc_str[0],
                     "TCP/IP Address: ");
        
        i = 0;
        while ((dbg_status == DBG_STATUS_OK) &&
               (i < 4))
        {
            dbg_status = DBG_STR_String_From_BYTE(&ip_addr_str[0],
                                                  (UINT)server_addr.ip_num.is_ip_addrs[i],
                                                  DBG_STR_RADIX_DECIMAL,
                                                  DBG_STR_INC_PREFIX_DISABLE,
                                                  DBG_STR_INC_LEADING_ZEROS_DISABLE,
                                                  DBG_STR_INC_NULL_TERM_ENABLE);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                
                (VOID)strcat(&p_port_info -> desc_str[0],
                             &ip_addr_str[0]);
                
                if (i < 3)
                {
                    (VOID)strcat(&p_port_info -> desc_str[0],
                                 ".");
                    
                }
                else
                {
                    (VOID)strcat(&p_port_info -> desc_str[0],
                                 ":");
                    
                } 
                    
                i++;
                
            } 
    
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get IP port string. */
        
        dbg_status = DBG_STR_String_From_UINT(&ip_port_str[0],
                                              server_addr.port_num,
                                              DBG_STR_RADIX_DECIMAL,
                                              DBG_STR_INC_PREFIX_DISABLE,
                                              DBG_STR_INC_LEADING_ZEROS_DISABLE);        
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Combine strings together. */
        
        (VOID)strcat(&p_port_info -> desc_str[0],
                     &ip_port_str[0]);

    } 

    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_COM_DRV_NU_TCP_Driver_Register
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
*       DBG_STATUS_INVALID_PARAMETERS - Invalid parameter values detected.
*                                                                      
*************************************************************************/
DBG_STATUS   DBG_COM_DRV_NU_TCP_Driver_Register(DBG_COM_CB *        p_com,
                                                DBG_COM_DRV *       p_driver)
{
    DBG_STATUS      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if ((p_com == NU_NULL) ||
        (p_driver == NU_NULL))
    {
        /* ERROR: Invalid parameter. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup driver control block. */
        
        p_driver -> p_dbg_com = (VOID *)p_com;
        p_driver -> p_drv_data = (VOID *)NU_NULL;
        p_driver -> drv_init_func = (VOID *)dbg_com_drv_nu_tcp_driver_initialize;
        p_driver -> drv_term_func = (VOID *)dbg_com_drv_nu_tcp_driver_terminate;
        p_driver -> port_open_func = (VOID *)dbg_com_drv_nu_tcp_port_open;
        p_driver -> port_close_func = (VOID *)dbg_com_drv_nu_tcp_port_close;
        p_driver -> port_read_func = (VOID *)dbg_com_drv_nu_tcp_port_read;        
        p_driver -> port_write_func = (VOID *)dbg_com_drv_nu_tcp_port_write;
        p_driver -> port_info_func = (VOID *)dbg_com_drv_nu_tcp_port_info;        

    } 

    return (dbg_status);
}

#endif /* CFG_NU_OS_NET_STACK_ENABLE */
