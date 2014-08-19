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
*       dbg_com.c
*
*   COMPONENT
*
*       Debug Agent - Communications
*
*   DESCRIPTION
*
*       This file contains the communications component of the debug
*       agent.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       dbg_com_driver_register
*       dbg_com_driver_unregister
*       dbg_com_system_initialize
*       dbg_com_system_terminate
*       dbg_com_port_open
*       dbg_com_port_close
*       dbg_com_port_read
*       dbg_com_port_write
*       dbg_com_port_info
*       dbg_com_debug_event_callback
*       dbg_com_data_handler
*       dbg_com_disconnect_handler
*       dbg_com_error_handler
*       dbg_com_thread_entry
*
*       DBG_COM_Initialize
*       DBG_COM_Terminate
*       DBG_COM_Send
*
*   DEPENDENCIES
*
*       dbg.h
*
*************************************************************************/

/*Include files */

#include "services/dbg.h"

/* External variables */

extern  RSP_CB          rsp_pkt_mgmt;
extern DBG_CB *         DBG_p_cb;

/***** Local functions */

/* Local function prototypes */

static DBG_STATUS  dbg_com_driver_register(DBG_COM_CB *            p_dbg_com,
                                            DBG_COM_PORT_TYPE       drvPortType,
                                            DBG_COM_DRV_REG_FUNC    drvRegFunc);

static DBG_STATUS  dbg_com_driver_unregister(DBG_COM_CB *            p_dbg_com,
                                              DBG_COM_PORT_TYPE       drvPortType);

static DBG_STATUS  dbg_com_system_initialize(DBG_COM_CB *    p_dbg_com);

static DBG_STATUS  dbg_com_system_terminate(DBG_COM_CB *    p_dbg_com);

static DBG_STATUS  dbg_com_port_open(DBG_COM_CB *                  p_dbg_com,
                                      DBG_COM_PORT_TYPE             port_type,
                                      DBG_COM_PORT_DATA             port_data,
                                      DBG_COM_PORT *                p_port);

static DBG_STATUS  dbg_com_port_close(DBG_COM_PORT *           p_port);

static DBG_STATUS  dbg_com_port_read(DBG_COM_PORT *         p_port,
                                      VOID *                 p_data,
                                      UINT                   data_size,
                                      UINT *                 p_data_read_size);

static DBG_STATUS  dbg_com_port_write(DBG_COM_PORT *            p_port,
                                       VOID *                    p_data,
                                       UINT                      data_size);

static DBG_STATUS  dbg_com_port_info(DBG_COM_PORT *                p_port,
                                      DBG_COM_PORT_INFO *           p_portInfo);

static void dbg_com_debug_event_callback(VOID *                        p_context,
                                          DBG_EVENT_HANDLER_PARAM *     p_param);

static DBG_STATUS dbg_com_data_handler(DBG_COM_CB *         p_dbg_com,
                                            VOID *               p_data,
                                            UNSIGNED             data_size);

static DBG_STATUS dbg_com_disconnect_handler(DBG_COM_CB *  p_dbg_com);

static DBG_STATUS dbg_com_error_handler(DBG_COM_CB *       p_dbg_com);

static VOID dbg_com_thread_entry(UNSIGNED     argc,
                                      VOID *       argv);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_driver_register
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function registers a driver with the communications 
*       component.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_com - pointer to control block.
*
*       drv_port_type - The type of ports the driver will create.
*
*       drv_reg_func - The driver registration function. 
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory to
*                                           create drivers.
*
*       DBG_STATUS_INVALID_DRIVER - Indicates registered driver is 
*                                   invalid.
*
*       DBG_STATUS_FAILED - Indicates unable to de-allocate memory 
*                           resources after operation failure.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_driver_register(DBG_COM_CB *            p_dbg_com,
                                            DBG_COM_PORT_TYPE       drv_port_type,
                                            DBG_COM_DRV_REG_FUNC    drv_reg_func)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Allocate memory for a new driver. */
    
    p_drv = DBG_System_Memory_Allocate(sizeof(DBG_COM_DRV),
                                       DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                       DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                       DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                       NU_NO_SUSPEND);    
 
    if (p_drv == NU_NULL)
    {
        /* ERROR: Insufficient memory resources. */
        
        dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
        
    }             
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Call driver registration function for port type. */
        
        dbg_status = drv_reg_func(p_dbg_com,
                                  p_drv);

    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Check validity of registered driver. */
        
        if ((p_drv -> p_dbg_com == NU_NULL) ||
            (p_drv -> drv_init_func == NU_NULL) ||
            (p_drv -> drv_term_func == NU_NULL) ||
            (p_drv -> port_open_func == NU_NULL) ||
            (p_drv -> port_close_func == NU_NULL) ||
            (p_drv -> port_read_func == NU_NULL) ||   
            (p_drv -> port_write_func == NU_NULL) ||
            (p_drv -> port_info_func == NU_NULL))
        {
            /* ERROR: Invalid driver */
            
            dbg_status = DBG_STATUS_INVALID_DRIVER;
            
        } 
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update driver list with new driver control block. */
        
        p_dbg_com -> drv_list[drv_port_type] = p_drv;    
    
    } 
    
    if (dbg_status != DBG_STATUS_OK)
    {
        /* Clean up any locally allocated resources. */
        
        if (p_drv != NU_NULL)
        {
            dbg_status = DBG_System_Memory_Deallocate(p_drv);
    
        } 
    
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_driver_unregister
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function un-registers a driver with the communications 
*       component for the specified port type, whether a driver currently
*       exists or not.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_com - pointer to control block.
*
*       drv_port_type - The type of ports the driver will create.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates unable to de-allocate memory 
*                           resources for driver.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_driver_unregister(DBG_COM_CB *            p_dbg_com,
                                              DBG_COM_PORT_TYPE       drv_port_type)
{
    DBG_STATUS              dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_dbg_com -> drv_list[drv_port_type] != NU_NULL)
    {
        /* Deallocate memory resources for driver. */
        
        dbg_status = DBG_System_Memory_Deallocate(p_dbg_com -> drv_list[drv_port_type]);
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update driver list to indicate no driver present. */
        
        p_dbg_com -> drv_list[drv_port_type] = NU_NULL;
        
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_system_initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function initializes the communications system.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_com - pointer to control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_DRIVER - Indicates a driver without an
*                                   initialization function was
*                                   encountered during operation.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_system_initialize(DBG_COM_CB *    p_dbg_com)
{
    DBG_STATUS              dbg_status;
    DBG_COM_PORT_TYPE       cur_port_type;
    DBG_COM_DRV_INIT_FUNC   drv_init_func;    
    
    DBG_COM_DRV_REG_FUNC    drv_reg_array[] = {DBG_COM_SERIAL_DRIVER_REGISTER,
                                               DBG_COM_TCP_DRIVER_REGISTER};
    
    VOID *                  unused_func = dbg_com_port_info;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Remove warnings for unused function. */
    
    if (unused_func)
    {
        unused_func = NU_NULL;

    }    

    /* Register communications drivers. */

    cur_port_type = DBG_COM_PORT_TYPE_FIRST;
    while ((dbg_status == DBG_STATUS_OK) &&
           (cur_port_type < DBG_COM_PORT_TYPE_LIST_SIZE))
    {
        if (drv_reg_array[cur_port_type] == NU_NULL)
        {
            /* Un-register the driver for current port type. */
            
            dbg_status = dbg_com_driver_unregister(p_dbg_com,
                                                    cur_port_type);
            
        }
        else
        {
            /* Register the driver for current port type. */
            
            dbg_status = dbg_com_driver_register(p_dbg_com,
                                                 cur_port_type,
                                                 drv_reg_array[cur_port_type]);

        }
         
        cur_port_type++;
        
    }

    /* Initialize all registered communications drivers. */

    cur_port_type = DBG_COM_PORT_TYPE_FIRST;
    while ((dbg_status == DBG_STATUS_OK) &&
           (cur_port_type < DBG_COM_PORT_TYPE_LIST_SIZE))
    {
        if (p_dbg_com -> drv_list[cur_port_type] != NU_NULL)
        {
            /* Initialize the driver for current port type. */
            
            drv_init_func = (DBG_COM_DRV_INIT_FUNC)p_dbg_com -> drv_list[cur_port_type] -> drv_init_func;
        
            dbg_status = drv_init_func(p_dbg_com -> drv_list[cur_port_type]);
 
        }
 
        cur_port_type++;
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update state to allow operations. */
        
        p_dbg_com -> is_active = NU_TRUE;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_system_terminate
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function terminates the communications component.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_com_dbg - pointer to control block.
*
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_DRIVER - Indicates a driver without a
*                                   termination function was
*                                   encountered during operation.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_system_terminate(DBG_COM_CB *    p_dbg_com)
{
    DBG_STATUS              dbg_status;
    DBG_COM_PORT_TYPE       cur_port_type;
    DBG_COM_DRV_TERM_FUNC   drv_term_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Terminate all registered communications drivers. */

    cur_port_type = DBG_COM_PORT_TYPE_FIRST;
    while ((dbg_status == DBG_STATUS_OK) &&
           (cur_port_type < DBG_COM_PORT_TYPE_LIST_SIZE))
    {
        if (p_dbg_com -> drv_list[cur_port_type] != NU_NULL)
        {        
            /* Terminate the driver for current port type. */            
            
            drv_term_func = (DBG_COM_DRV_TERM_FUNC)p_dbg_com -> drv_list[cur_port_type] -> drv_term_func;
    
            dbg_status = drv_term_func(p_dbg_com -> drv_list[cur_port_type]);
        
        } 
            
        cur_port_type++;
        
    } /* while */

    /* Unregister communications drivers. */

    cur_port_type = DBG_COM_PORT_TYPE_FIRST;
    while ((dbg_status == DBG_STATUS_OK) &&
           (cur_port_type < DBG_COM_PORT_TYPE_LIST_SIZE))
    {
        /* Un-register the driver for current port type. */
        
        dbg_status = dbg_com_driver_unregister(p_dbg_com,
                                              cur_port_type);
            
        cur_port_type++;
        
    } /* while */

    /* Update state to prevent any further operations. */
    
    p_dbg_com -> is_active = NU_FALSE;

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_port_open
*                                                                      
*   DESCRIPTION   
*                                                       
*       API function opens a new communications port.
*
*       NOTE: This function MUST be called from a thread context.
*                                                                                                 
*   INPUTS                                                               
*                
*       p_dbg_com - Pointer to communications control block.
*
*       port_type - The type of port to be opened.
*
*       port_data - Port type specific data to be used to open the port.
*                  The usage of the data is dependent on the type of port
*                  being opened.
*                                                      
*       p_port - Pointer to a com port control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_TYPE - Indicates an invalid or unavailable port
*                                 type was requested.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port or com control
*                                     block resources is invalid.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_port_open(DBG_COM_CB *                  p_dbg_com,
                                      DBG_COM_PORT_TYPE             port_type,
                                      DBG_COM_PORT_DATA             port_data,
                                      DBG_COM_PORT *                p_port)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;
    DBG_COM_PORT_OPEN_FUNC  port_open_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if ((p_dbg_com == NU_NULL) ||
        (p_port == NU_NULL))
    {
        /* ERROR: Invalid resource. */
        
        dbg_status = DBG_STATUS_INVALID_RESOURCE;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine how to proceed based on the requested port type. */
        
        switch (port_type)
        {
            case DBG_COM_PORT_TYPE_NU_SERIAL :
            case DBG_COM_PORT_TYPE_NU_TCP_IP :
            {
                /* Get pointer to driver. */
                
                p_drv = p_dbg_com -> drv_list[port_type];            
                
                break;
                
            } 
    
            default :
            {
                /* ERROR: Invalid port type. */
                
                dbg_status = DBG_STATUS_INVALID_TYPE;
                
                break;
                
            } 
    
        } 

    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Ensure that the requested port type is available. */
        
        if (p_drv == NU_NULL)
        {
            /* ERROR: Unavailable port type. */
            
            dbg_status = DBG_STATUS_INVALID_TYPE;
            
        } 
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get driver port open function from driver. */
        
        port_open_func = (DBG_COM_PORT_OPEN_FUNC)p_drv -> port_open_func;
        
        /* Invoke the driver port open function. */
        
        dbg_status = port_open_func(p_drv,
                                    port_data,
                                    p_port);
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_port_close
*                                                                      
*   DESCRIPTION   
*                                                       
*       API function closes an existing communications port.
*
*       NOTE: This function MUST be called from a thread context.
*                                                                                                 
*   INPUTS                                                               
*                
*       p_port - Pointer to a com port control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port resources is 
*                                     invalid.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_port_close(DBG_COM_PORT *           p_port)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;
    DBG_COM_PORT_CLS_FUNC   port_close_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if (p_port == NU_NULL)
    {
        /* ERROR: Invalid resource. */
        
        dbg_status = DBG_STATUS_INVALID_RESOURCE;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to driver from port. */
        
        p_drv = (DBG_COM_DRV *)p_port -> p_drv;        
        
        /* Get driver port close function from driver. */
        
        port_close_func = (DBG_COM_PORT_CLS_FUNC)p_drv -> port_close_func;
        
        /* Invoke the driver port close function. */
        
        dbg_status = port_close_func(p_port);
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_port_read
*                                                                      
*   DESCRIPTION   
*                                                       
*       API function reads data from a communications port.
*
*       NOTE: This function MUST be called from a thread context.
*                                                                                                 
*   INPUTS                                                               
*                
*       p_port - Pointer to a com port control block.
*
*       p_data - Pointer to data buffer that will contain received data.
*
*       data_size - The maximum size (in bytes) of data that may be 
*                  received into the data buffer.
* 
*       p_data_read_size - Return parameter that will be updated to contain 
*                       the size (in bytes) of the data received if the 
*                       operation is successful.  If the operation fails 
*                       the value is undefined.
*                                                                     
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port, data buffer
*                                     pointer or data buffer size max 
*                                     resource is invalid.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_port_read(DBG_COM_PORT *         p_port,
                                      VOID *                 p_data,
                                      UINT                   data_size,
                                      UINT *                 p_data_read_size)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;     
    DBG_COM_PORT_READ_FUNC  port_read_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0))
    {
        /* ERROR: Invalid resource. */
        
        dbg_status = DBG_STATUS_INVALID_RESOURCE;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to driver from port. */
        
        p_drv = (DBG_COM_DRV *)p_port -> p_drv;        
        
        /* Get driver port read function from driver. */
        
        port_read_func = (DBG_COM_PORT_READ_FUNC)p_drv -> port_read_func;
        
        /* Invoke the driver port read function. */
        
        dbg_status = port_read_func(p_port,
                                    p_data,
                                    data_size,
                                    p_data_read_size);
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_port_write
*                                                                      
*   DESCRIPTION   
*                                                       
*       API function writes data to a communications port.
*
*       NOTE: This function MUST be called from a thread context.
*                                                                                                 
*   INPUTS                                                               
*                
*       p_port - Pointer to a com port control block.
*
*       p_data - Pointer to data to be sent.
*
*       data_size - The size (in bytes) of the data.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port, data buffer
*                                     pointer or data buffer size 
*                                     resource is invalid.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_port_write(DBG_COM_PORT *            p_port,
                                       VOID *                    p_data,
                                       UINT                      data_size)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;    
    DBG_COM_PORT_WRT_FUNC   port_write_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if ((p_port == NU_NULL) ||
        (p_data == NU_NULL) ||
        (data_size == 0))
    {
        /* ERROR: Invalid resource. */
        
        dbg_status = DBG_STATUS_INVALID_RESOURCE;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to driver from port. */
        
        p_drv = (DBG_COM_DRV *)p_port -> p_drv;        
        
        /* Get driver port write function from driver. */
        
        port_write_func = (DBG_COM_PORT_WRT_FUNC)p_drv -> port_write_func;
        
        /* Invoke the driver port write function. */
        
        dbg_status = port_write_func(p_port,
                                  p_data,
                                  data_size);
        
        /* Save this buffer in case retransmission is requested. */
        rsp_pkt_mgmt.last_pkt_buff = p_data;
        rsp_pkt_mgmt.last_pkt_size = data_size;
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_com_port_info
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function retrieves information about a communications port.
*
*       NOTE: This function MUST be called from a thread context.
*                                                                                                 
*   INPUTS                                                               
*                
*       p_port - Pointer to a com port control block.
*
*       p_portInfo - Pointer to a com port information structure that will
*                   be filled out upon the successful completion of this
*                   function.  If the function fails the value is 
*                   undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_RESOURCE - Indicates the port resources is 
*                                     invalid.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_com_port_info(DBG_COM_PORT *                p_port,
                                     DBG_COM_PORT_INFO *           p_portInfo)
{
    DBG_STATUS              dbg_status;
    DBG_COM_DRV *           p_drv;      
    DBG_COM_PORT_INFO_FUNC  port_info_func;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    if (p_port == NU_NULL)
    {
        /* ERROR: Invalid resource. */
        
        dbg_status = DBG_STATUS_INVALID_RESOURCE;
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to driver from port. */
        
        p_drv = (DBG_COM_DRV *)p_port -> p_drv;        
        
        /* Get driver port information function from driver. */
        
        port_info_func = (DBG_COM_PORT_INFO_FUNC)p_drv -> port_info_func;
        
        /* Invoke the driver port information function. */
        
        dbg_status = port_info_func(p_port,
                                    p_portInfo);
        
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_com_debug_event_callback
*
*   DESCRIPTION
*
*       Event callback function for debug engine events.  This function
*       conforms to the debug service event handler function prototype.
*
*   INPUTS
*
*       p_context - Pointer to the handler context.
*
*       p_param - Pointer to the handler parameters.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID dbg_com_debug_event_callback(VOID *                        p_context,
                                          DBG_EVENT_HANDLER_PARAM *     p_param)
{
    RSP_STATUS          rsp_status;
    DBG_COM_CB *        p_dbg_com;
    unsigned int        size;
    DBG_THREAD_ID       thread_id;

    /* Setup pointer to control block */

    p_dbg_com = (DBG_COM_CB *)p_context;

    /* Determine how to proceed based on the debug event that has
       occurred. */

    switch (p_param -> id)
    {
        case DBG_EVENT_ID_BKPT_HIT:
        {
            /* Get the thread ID. */

            thread_id = p_param -> id_param.bkpt_hit.exec_ctxt_id;

            /* Process specific breakpoint hits only. */

            if ((thread_id != DBG_THREAD_ID_ALL) &&
                (thread_id != DBG_THREAD_ID_ANY))
            {
                /* Handle breakpoint hit event. */
    
                rsp_status = RSP_Send_Notification(RSP_BREAKPOINT_NOTIFICATION,
                                                   0,
                                                   rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                                   &size,
                                                   thread_id);
    
                if (rsp_status == RSP_STATUS_OK)
                {
                    /* Transmit breakpoint event */
    
                    DBG_COM_Send(p_dbg_com,
                                 (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                 size);
                }

            }

            break;

        }

        case DBG_EVENT_ID_STEP_CPLT :
        {
            /* Handle step complete event. */

            /* Get the thread ID for the step completion. */

            thread_id = p_param -> id_param.step_cplt.exec_ctxt_id;

            rsp_status = RSP_Send_Notification(RSP_SINGLE_STEP_COMPLETE_NOTIFICATION, 0, rsp_pkt_mgmt.p_out_rsp_pkt_buff,&size, thread_id);

            if (rsp_status == RSP_STATUS_OK)
            {
                /* Transmit breakpoint event */

                DBG_COM_Send(p_dbg_com,
                             (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                             size);
            }

            break;

        }

        case DBG_EVENT_ID_THD_STOP:
        {
            /* Add stopped thread to thread queue. */

            rsp_status = RSP_Thread_Queue_Send(&rsp_pkt_mgmt.thread_queue,
                                               p_param -> id_param.thd_stop.exec_ctxt_id);

            break;

        }

        case DBG_EVENT_ID_NONE:
        default:
        {
            /* Unhandled debug event ID.  Do nothing. */

            break;

        }

    }

    return;
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_com_data_handler
*
*   DESCRIPTION
*
*       This function will be called to handle any data received by the
*       debug service.
*
*   INPUTS
*
*       p_dbg_com - Pointer to debug com control block.
*
*       p_data - pointer to the data to be processed.
*
*       data_size - size (in bytes) of the data to be processed.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
static DBG_STATUS dbg_com_data_handler(DBG_COM_CB *         p_dbg_com,
                                            VOID *               p_data,
                                            UNSIGNED             data_size)
{
    DBG_STATUS          dbg_status;

    unsigned int        rsp_pkt_addr;
    unsigned int        rsp_pkt_size;
    unsigned int        action;
    RSP_STATUS          rsp_status;
    CHAR *              p_pkt_data;
    unsigned int        data_processed;
    unsigned int        pkt_data_processed;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    rsp_status = RSP_STATUS_OK;

    /* RSP data processing */

    data_processed = 0;
    p_pkt_data = (CHAR *)p_data;

    while (data_processed < data_size)
    {
        action = RSP_Check_Packet((VOID *)&p_pkt_data[data_processed],
                                  (data_size - data_processed),
                                  (VOID*)&rsp_pkt_addr,
                                  &rsp_pkt_size,
                                  &pkt_data_processed);

        /* Update total data processed. */

        data_processed += pkt_data_processed;

        if (action == RSP_ACK_AND_PROCESS_PACKET)
        {
            /* Get RSP ACK packet */

            rsp_status = RSP_Send_Notification(RSP_ACK_NOTIFICATION,
                                               0,
                                               rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                               &rsp_pkt_mgmt.pkt_size,0);

            /* Transmit ACK packet */

            DBG_COM_Send(p_dbg_com,
                         (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                         rsp_pkt_mgmt.pkt_size);

            /* Process received RSP packet */

            rsp_status = RSP_Process_Packet((char*)rsp_pkt_addr,
                                            rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                            &rsp_pkt_mgmt.pkt_size);

            if ((rsp_status == RSP_STATUS_OK) | (rsp_status == RSP_STATUS_ERROR_RESP))
            {
                /* Transmit RSP response */

                DBG_COM_Send(p_dbg_com,
                             (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                             rsp_pkt_mgmt.pkt_size);

            }

        }

        if (action == RSP_PROCESS_PACKET)
        {
            /* Process received RSP packet */

            rsp_status = RSP_Process_Packet((char*)rsp_pkt_addr,
                                            rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                            &rsp_pkt_mgmt.pkt_size);

            if ((rsp_status == RSP_STATUS_OK) ||
                (rsp_status == RSP_STATUS_ERROR_RESP))
            {
                /* Transmit response */

                DBG_COM_Send(p_dbg_com,
                             (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                             rsp_pkt_mgmt.pkt_size);

            }

        }

        if (action == RSP_REQ_CLIENT_RETRANSMISSION)
        {
            /* Get RSP NO ACK packet */

            rsp_status = RSP_Send_Notification(RSP_NO_ACK_NOTIFICATION,
                                               0,
                                               rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                                               &rsp_pkt_mgmt.pkt_size,0);

            /* Transmit NO ACK packet */

            DBG_COM_Send(p_dbg_com,
                         (VOID *)rsp_pkt_mgmt.p_out_rsp_pkt_buff,
                         rsp_pkt_mgmt.pkt_size);

        }

        if (action == RSP_REQ_SERVER_RETRANSMISSION)
        {
            /* Retransmit response */

            DBG_COM_Send(p_dbg_com,
                         (VOID *)rsp_pkt_mgmt.last_pkt_buff,
                         rsp_pkt_mgmt.last_pkt_size);

        }

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_com_disconnect_handler
*
*   DESCRIPTION
*
*       This function will be called to handle a communications disconnect
*       event.
*
*   INPUTS
*
*       p_dbg_com - Pointer to debug com control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates that the communications error was
*                           not handled.
*
*************************************************************************/
static DBG_STATUS dbg_com_disconnect_handler(DBG_COM_CB *  p_dbg_com)
{
    DBG_STATUS          dbg_status;
    RSP_STATUS          rsp_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    rsp_status = RSP_STATUS_OK;

    /* Notify the RSP component that a disconnect has occurred. */

    rsp_status = RSP_Com_Disconnect_Notify();

    if (rsp_status != RSP_STATUS_OK)
    {
        /* ERROR: Notification of RSP component failed. */

        dbg_status = DBG_STATUS_FAILED;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_com_error_handler
*
*   DESCRIPTION
*
*       This function will be called to handle any errors that occur
*       during communications.
*
*   INPUTS
*
*       p_dbg_com - Pointer to debug com control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates that the communications error was
*                           not handled.
*
*************************************************************************/
static DBG_STATUS dbg_com_error_handler(DBG_COM_CB *       p_dbg_com)
{
    DBG_STATUS          dbg_status;
    RSP_STATUS          rsp_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    rsp_status = RSP_STATUS_OK;

    /* Notify the RSP component that an error has occurred. */

    rsp_status = RSP_Com_Error_Notify();

    if (rsp_status != RSP_STATUS_OK)
    {
        /* ERROR: Notification of RSP component failed. */

        dbg_status = DBG_STATUS_FAILED;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_com_thread_entry
*
*   DESCRIPTION
*
*       This is the debug service communications thread.
*
*   INPUTS
*
*       p_dbg_com - Pointer to control block.
*
*       p_dbg - Pointer to the RMD control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
static VOID dbg_com_thread_entry(UNSIGNED     argc,
                                 VOID *       argv)
{
    DBG_STATUS          dbg_status;
    STATUS              nu_status;
    BOOLEAN             is_data_received;
    DBG_COM_CB *        p_dbg_com;
    DBG_CMD             dbg_cmd;
    UINT8               rx_data[DBG_CFG_COM_RX_BUFFER_SIZE];
    UINT                rx_data_size;

    /* Get pointer to control block. */

    p_dbg_com = (DBG_COM_CB *)argv;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Wait for appropriate run-level before continuing. */

    nu_status = NU_RunLevel_Complete_Wait(CFG_NU_OS_SVCS_DBG_RUNLEVEL+1,
                                          NU_SUSPEND);

    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Nucleus system unavailable. */

        dbg_status = DBG_STATUS_FAILED;

    }

    /* Thread main loop: wait for and process data. */

    while (dbg_status == DBG_STATUS_OK)
    {
        /* Open a debug session to start the debug engine. */

        dbg_cmd.op = DBG_CMD_OP_SESSION_OPEN;
        dbg_cmd.op_param.ses_opn.version_id = DBG_VERSION_ID_1_0;
        dbg_cmd.op_param.ses_opn.p_session_id = (INT*)&rsp_pkt_mgmt.dbg_session_id;

        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                     &dbg_cmd);
    
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Conditionally set startup breakpoint.  The startup 
               breakpoint will stop all application threads and be removed
               automatically after hitting one time. */
            
            dbg_cmd.op_param.bkpt_set.p_address = DBG_OS_Startup_Breakpoint_Address();
            
            if (dbg_cmd.op_param.bkpt_set.p_address != NU_NULL)
            {
                dbg_cmd.op = DBG_CMD_OP_BREAKPOINT_SET;
                dbg_cmd.op_param.bkpt_set.pass_count = 0;
                dbg_cmd.op_param.bkpt_set.hit_count = 1;
                dbg_cmd.op_param.bkpt_set.hit_thread_id = DBG_THREAD_ID_ALL;
                dbg_cmd.op_param.bkpt_set.stop_thread_id = DBG_THREAD_ID_ALL;
        
                dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, 
                                             &dbg_cmd);        
        
            }
        
        }        

        /* Communications session loop. */

        while (dbg_status == DBG_STATUS_OK)
        {
            /* Open communications port. */
    
            dbg_status = dbg_com_port_open(p_dbg_com,
                                           DBG_CFG_COM_PORT_TYPE,
                                           DBG_CFG_COM_PORT_NUMBER,
                                           &p_dbg_com -> com_port);

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Reset data received state for new communications
                   session. */

                is_data_received = NU_FALSE;

                /* Register an event handler for debug engine events. */

                dbg_cmd.op = DBG_CMD_OP_EVENT_HANDLER_REGISTER;
                dbg_cmd.op_param.evt_hdlr_reg.handler_func = dbg_com_debug_event_callback;
                dbg_cmd.op_param.evt_hdlr_reg.handler_context = (VOID *)p_dbg_com;

                dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                             &dbg_cmd);
            }

            /* Communications data loop. */

            while (dbg_status == DBG_STATUS_OK)
            {
                /* Attempt to receive data. */

                dbg_status = dbg_com_port_read(&p_dbg_com -> com_port,
                                               &rx_data[0],
                                               DBG_CFG_COM_RX_BUFFER_SIZE,
                                               &rx_data_size);

                /* Determine how to proceed based on the resulting status. */

                switch(dbg_status)
                {
                    case DBG_STATUS_OK :
                    {
                        /* Update data received state. */

                        is_data_received = NU_TRUE;

                        /* Handle received data. */

                        dbg_status = dbg_com_data_handler(p_dbg_com,
                                                          &rx_data[0],
                                                          rx_data_size);

                        break;

                    }

                    case DBG_STATUS_INVALID_CONNECTION :
                    {
                        /* Handle disconnect only if data was received. */

                        if (is_data_received == NU_TRUE)
                        {
                            (VOID)dbg_com_disconnect_handler(p_dbg_com);
                        }

                        break;

                    }

                    default :
                    {
                        /* Handle communications error. */

                        (VOID)dbg_com_error_handler(p_dbg_com);

                        break;

                    }

                }

            }

            /* Unregister the event handler for debug engine events. */

            dbg_cmd.op = DBG_CMD_OP_EVENT_HANDLER_UNREGISTER;

            (VOID)DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                  &dbg_cmd);

            /* Close communications port. */

            (VOID)dbg_com_port_close(&p_dbg_com -> com_port);

            /* Determine if communications loop should continue based on
               whether any data was received during previous loop. */

            if (is_data_received == NU_FALSE)
            {
                dbg_status = DBG_STATUS_OK;
            }

        }

        /* Close the debug session to terminate debug engine. */

        dbg_cmd.op = DBG_CMD_OP_SESSION_CLOSE;

        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                     &dbg_cmd);

    }

    return;
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_COM_Initialize
*
*   DESCRIPTION
*
*       This function initializes the Debug Agent communications system.
*
*   INPUTS
*
*       p_dbg_com - Pointer to control block.
*
*       p_dbg - Pointer to the RMD control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
DBG_STATUS DBG_COM_Initialize(DBG_COM_CB *  p_dbg_com,
                              VOID *        p_dbg)
{
    DBG_STATUS          dbg_status;
    RSP_STATUS          rsp_status;
    STATUS              nu_status;
    VOID *              p_memory;

    /* Setup pointer to Debug service control block. */

    p_dbg_com -> p_dbg = p_dbg;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Initialize communications system. */
    
    dbg_status = dbg_com_system_initialize(p_dbg_com);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get current run-level and use it to set the run-level that the
           com thread should wait for (next after current). */
    
        nu_status = NU_RunLevel_Current(&p_dbg_com -> thd_run_level);
    
        if (nu_status == NU_SUCCESS)
        {
            p_dbg_com -> thd_run_level++;
    
        }
        else
        {
            /* ERROR: Unable to determine current run-level. */
    
            dbg_status = DBG_STATUS_FAILED;
    
        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Create the Com Thread. */

        nu_status = NU_Create_Task(&p_dbg_com -> thd_cb,
                                   "DBG_COM",
                                   dbg_com_thread_entry,
                                   1,
                                   (VOID *)p_dbg_com,
                                   &p_dbg_com -> thd_stack[0],
                                   DBG_CFG_COM_TASK_STACK_SIZE,
                                   DBG_CFG_COM_TASK_PRIORITY,
                                   DBG_CFG_COM_TASK_TIME_SLICING,
                                   NU_PREEMPT,
                                   NU_NO_START);

        if (nu_status == NU_SUCCESS)
        {
            /* Bind task to kernel module. */

            nu_status = NU_BIND_TASK_TO_KERNEL(&p_dbg_com -> thd_cb);
        }

        if (nu_status == NU_SUCCESS)
        {
            /* Start task. */

            /* NOTE: Once the thread is started, if the OS/scheduler is
               running, execution will transfer to the thread. */

            nu_status = NU_Resume_Task(&p_dbg_com -> thd_cb);
        }

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to create debug communications thread. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Allocate memory for RSP Support Component */

        p_memory = DBG_System_Memory_Allocate(RSP_COMMS_BUFFER_SIZE,
                                              DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                              DBG_SYSTEM_MEMORY_ALLOC_UNCACHED,
                                              DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                              NU_NO_SUSPEND);



        /* Initialize RSP Support Component */

        rsp_status = RSP_Initialize(p_memory);

        /* Set debug status values */

        if (rsp_status != RSP_STATUS_OK)
        {
            dbg_status = DBG_STATUS_FAILED;
            
        }

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_COM_Terminate
*
*   DESCRIPTION
*
*       This function terminates the Debug Agent communications system.
*
*   INPUTS
*
*       p_dbg_com - Pointer to control block.
*
*       p_dbg - Pointer to the RMD control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
DBG_STATUS DBG_COM_Terminate(DBG_COM_CB *  p_dbg_com)
{
    DBG_STATUS          dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Initialize communications system. */
    
    dbg_status = dbg_com_system_terminate(p_dbg_com);

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_COM_Send
*
*   DESCRIPTION
*
*       Sends data.
*
*   INPUTS
*
*       p_dbg_com - Pointer to the control block.
*
*       p_data - Pointer to the data to be sent.
*
*       data_size - Size (in bytes) of data to be sent.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates data could not be sent.
*
*************************************************************************/
DBG_STATUS DBG_COM_Send(DBG_COM_CB *    p_dbg_com,
                        VOID *          p_data,
                        UINT            data_size)
{
    DBG_STATUS          dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Send data. */

    dbg_status = dbg_com_port_write(&p_dbg_com -> com_port,
                                    p_data,
                                    data_size);

    if (dbg_status != DBG_STATUS_OK)
    {
        /* ERROR: Unable to send data. */

        dbg_status = DBG_STATUS_FAILED;

        /* ERROR RECOVERY: None */

    }

    return(dbg_status);
}
