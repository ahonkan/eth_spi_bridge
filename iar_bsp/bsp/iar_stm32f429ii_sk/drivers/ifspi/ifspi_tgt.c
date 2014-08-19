/*
 * Avinash Honkan   Terabit Radios
 * 6 Aug 2014
 *
 * These device routines sets up spi and protocol to create a 
 * spi-based network interface.  This code interfaces to the ethernet
 * driver to connect to the net stack.  This code is based on the _Tgt 
 * functions found in the bsp/driver/ethernet directory.
 * There will be lots of references to ETHERNET_* as the stack assumes
 * only an ethernet device will be connected to it.  
 *
 * The IF SPI code will register itself as an ethernet device so 
 * that it can be registered with the stack with minimal code work.
 *
 */
/**************************************************************************
*
* DESCRIPTION
*
*     This file contains the source of the IF SPI driver for carrying
*     ethernet packets.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*
*     nu_bsp_drvr_ifspi_init
*     IF_SPI_Tgt_Read
*     IF_SPI_Tgt_Write
*     IF_SPI_Tgt_Target_Initialize
*     IF_SPI_Tgt_Controller_Init
*     IF_SPI_Tgt_Get_Address
*     IF_SPI_Tgt_Set_Address
*     IF_SPI_Tgt_Extended_Data
*     IF_SPI_Tgt_Receive_Packet
*     IF_SPI_Tgt_Configure
*     IF_SPI_Tgt_Enable
*     IF_SPI_Tgt_Disable
*
* DEPENDENCIES
*
*     nucleus.h
*     nu_kernel.h
*     nu_services.h
*     nu_networking.h
*     ethernet_tgt.h
*     if_spi_tgt.h
*
*************************************************************************/

/**********************************/
/* INCLUDE FILES                  */
/**********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "networking/nu_networking.h"

/* Neet ethernet headers since this is spoofing an ethernet device for 
 * the network stack
 */
#include "bsp/drivers/ethernet/stm32_emac/ethernet_tgt.h"
#include "bsp/drivers/ethernet/stm32_emac/phy.h"

#include "bsp/drivers/ifspi/ifspi_tgt_power.h"
#include "bsp/drivers/ifspi/ifspi_tgt.h"

#include "connectivity/lwspi.h"

/*********************************/
/* Defines                       */
/*********************************/

#define   IFSPI_FRAME_SOF         0xAAAA5555
#define   IFSPI_FRAME_EOF         0xAA55AA55
#define   IFSPI_FRAME_FS          0x8000         /* First Segment */
#define   IFSPI_DATA_LEN_MASK     0x0FFF
#define   IFSPI_FRAME_BLANK       0x0000         /* Blank Frame */


/*********************************/
/* IFSPI Monitor Thread          */
/*********************************/

/* Define the main task's stack size */
#define IFSPI_TASK_STACK_SIZE      (NU_MIN_STACK_SIZE)

/* Define the main task's priority */
#define IFSPI_TASK_PRIORITY   26

/* Define the main task's time slice */
#define IFSPI_TASK_TIMESLICE  20

/* Prototype for the main task's entry function */
static VOID IFSPI_Task_Entry(UNSIGNED argc, VOID *argv);


/*********************************/
/* GLOBAL VARIABLES              */
/*********************************/


/***********************************/
/* LOCAL FUNCTION PROTOTYPES       */
/***********************************/
static STATUS   IF_SPI_Tgt_Receive_Packet (DV_DEVICE_ENTRY *device);
static STATUS   IF_SPI_Get_Target_Info(const CHAR * key, ETHERNET_INSTANCE_HANDLE *inst_handle);

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_ifspi_init (const CHAR * key, INT startstop)
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the component
*       and calls the component driver registration function.
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       IF_SPI_Tgt_Dv_Register
*       IF_SPI_Tgt_Dv_Unregister
*
*   INPUTS
*
*       CHAR     *key                       - Key
*       INT      startstop                  - Start or Stop flag
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_bsp_drvr_ifspi_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID            if_spi_dev_id = DV_INVALID_DEV;
    STATUS                      status = NU_SUCCESS;
    NU_MEMORY_POOL           *  sys_pool_ptr;
    ETHERNET_INSTANCE_HANDLE *  inst_handle;
    IF_SPI_TARGET_DATA       *  tgt_ptr;   

    if (startstop)
    {
        /********************************/
        /* GET A UNUSED INSTANCE HANDLE */
        /********************************/

        /* Allocate memory for the STM32_ETH_INSTANCE_HANDLE structure */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
        
        if (status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&inst_handle, sizeof (ETHERNET_INSTANCE_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (inst_handle, 0, sizeof (ETHERNET_INSTANCE_HANDLE));

            /* Save the config path in the instance handle */
            strncpy(inst_handle->config_path, key, sizeof(inst_handle->config_path));

            status = NU_Allocate_Memory (sys_pool_ptr, (VOID **)&tgt_ptr, 
                                         sizeof (IF_SPI_TARGET_DATA), NU_NO_SUSPEND);
        
        }

        /******************************/
        /* SAVE DEFAULT CFG/TGT INFO  */
        /******************************/

        if (status == NU_SUCCESS)
        {
            inst_handle->tgt_ptr = tgt_ptr;                

            /* Get target info */
            status = IF_SPI_Get_Target_Info(key, inst_handle);
        }

        if (status == NU_SUCCESS)
        {

            /******************************/
            /* CALL DEVICE SETUP FUNCTION */
            /******************************/

            /*****************************************************/
            /* Assign the target hardware manipulation functions */
            /*****************************************************/

            inst_handle->tgt_fn.Tgt_Read                 = &IF_SPI_Tgt_Read;
            inst_handle->tgt_fn.Tgt_Write                = &IF_SPI_Tgt_Write;
            inst_handle->tgt_fn.Tgt_Enable               = &IF_SPI_Tgt_Enable;
            inst_handle->tgt_fn.Tgt_Disable              = &IF_SPI_Tgt_Disable;
            inst_handle->tgt_fn.Tgt_Create_Extended_Data = &IF_SPI_Tgt_Create_Extended_Data;
            inst_handle->tgt_fn.Tgt_Target_Initialize    = &IF_SPI_Tgt_Target_Initialize;
            inst_handle->tgt_fn.Tgt_Controller_Init      = &IF_SPI_Tgt_Controller_Init;
            inst_handle->tgt_fn.Tgt_Get_ISR_Info         = &IF_SPI_Tgt_Get_ISR_Info;
            inst_handle->tgt_fn.Tgt_Set_Phy_Dev_ID       = &IF_SPI_Tgt_Set_Phy_Dev_ID;
            inst_handle->tgt_fn.Tgt_Get_Link_Status      = &IF_SPI_Tgt_Get_Link_Status;
            inst_handle->tgt_fn.Tgt_Update_Multicast     = &IF_SPI_Tgt_Update_Multicast;
            inst_handle->tgt_fn.Tgt_Phy_Initialize       = &IF_SPI_Tgt_Phy_Initialize;
            inst_handle->tgt_fn.Tgt_Notify_Status_Change = &IF_SPI_Tgt_Notify_Status_Change;

            inst_handle->tgt_fn.Tgt_Get_Address = &IF_SPI_Tgt_Get_Address;
            inst_handle->tgt_fn.Tgt_Set_Address = &IF_SPI_Tgt_Set_Address;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            inst_handle->tgt_fn.Tgt_Pwr_Default_State  = &IF_SPI_Tgt_Pwr_Default_State;
            inst_handle->tgt_fn.Tgt_Pwr_Set_State      = &IF_SPI_Tgt_Pwr_Set_State;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

            inst_handle->tgt_fn.Tgt_Pwr_Hibernate_Restore = &IF_SPI_Tgt_Pwr_Hibernate_Restore;

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif

            /* Call the STM32_EMAC component registration function */
            (VOID)Ethernet_Dv_Register(key, inst_handle);
        }
        else
        {
            /* Error:  Cannot initialize device.  Do some error reporting/handling here */
            while(1);                
        }
    }
    else
    {
        /* If we are stopping an already started device */
        if (if_spi_dev_id != DV_INVALID_DEV)
        {
            /* Call the component unregistration function */
            (VOID)Ethernet_Dv_Unregister (if_spi_dev_id);
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       CHAR      *key                      - Registry path
*       SERIAL_TGT *tgt_info                 - pointer to target info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - ETHERNET_REGISTRY_ERROR for error
*
*************************************************************************/
static STATUS IF_SPI_Get_Target_Info(const CHAR * key, ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    STATUS     reg_stat;
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[255];
    IF_SPI_TARGET_DATA       *tgt_ptr = (IF_SPI_TARGET_DATA*)inst_handle->tgt_ptr;   
    UINT32      temp;

    /* Get values from the registry */
    strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));

    reg_stat = REG_Get_String (strcat(reg_path, "/tgt_settings/dev_name"), inst_handle->name, sizeof(inst_handle->name));

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_String (strcat(reg_path, "/tgt_settings/spi_dev_name"), 
                                   tgt_ptr->spi_bus_name, NU_SPI_BUS_NAME_LEN);
    }

    if (reg_stat == NU_SUCCESS)
    {
        reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/spi_dev_ctrl", 
                                   &temp);

        if ((reg_stat == NU_SUCCESS) && (temp == 0))
        {
            tgt_ptr->spi_dev_ctrl = SPI_CFG_DEV_SLAVE;
        }
        else if ((reg_stat == NU_SUCCESS) && (temp == 1))
        {
            tgt_ptr->spi_dev_ctrl = SPI_CFG_DEV_MASTER;
        }
        else
            reg_stat = NU_INVALID_OPTIONS;
    }
        
    if (reg_stat == NU_SUCCESS)
    {
        reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/spi_baud_rate", 
                                   &(tgt_ptr->spi_baud_rate));

    }

    if (reg_stat != NU_SUCCESS)
    {
        status = reg_stat;
    }

    return(status);
}




/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Read
*
*   DESCRIPTION
*
*       This function is to read data from the ethernet hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       UINT32       numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_read            - Number of bytes read
*
*   OUTPUTS
*
*       INT          status                 - NU_SUCCESS, or number of bytes
*                                             written if different than numbyte
*
*************************************************************************/
STATUS IF_SPI_Tgt_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read)
{
    *bytes_read = numbyte;

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Write
*
*   DESCRIPTION
*
*       This function is a stub routine for IF SPI interface as the 
*       IF SPI polls the device for data to be written.
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       UINT32       numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_written         - Number of bytes written
*
*   OUTPUTS
*
*       INT          status                 - NU_SUCCESS, or number of bytes
*                                             written if different than numbyte
*
*************************************************************************/
STATUS IF_SPI_Tgt_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte,
                              OFFSET_T byte_offset, UINT32 *bytes_written)
{
    *bytes_written = numbyte;

    return (NU_SUCCESS);
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Get_Address
*
*   DESCRIPTION

*       This function returns the MAC address of the spi device.
*
*		Since the SPI device is point to point, there is no need
*		for a unique MAC.
*
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle  - Pointer to instance of driver
*       UINT8           *ether_addr         - Pointer to a buffer where the MAC address
*                                             will be stored.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service
*                                             is successful. Otherwise an
*                                             error code is returned.
*
**************************************************************************/
STATUS  IF_SPI_Tgt_Get_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr)
{
    IF_SPI_TARGET_DATA       *tgt_ptr = (IF_SPI_TARGET_DATA*)inst_handle->tgt_ptr;

    /* Set ethernet MAC address. */
    ether_addr[0] = 0x00;
    ether_addr[1] = 0x22;

    /* Set last 4 bytes of MAC address to current hardware timer count */
    ether_addr[2] = 0x00;
    ether_addr[3] = 0x00;
    ether_addr[4] = 0x00;

    if (tgt_ptr->spi_dev_ctrl ==  SPI_CFG_DEV_MASTER)
    	ether_addr[5] = 1;
    else
    	ether_addr[5] = 0;

    return (NU_SUCCESS); /* This has to be replaced by an actual return. */
} /* IF_SPI_Tgt_Get_Address */

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Set_Address
*
*   DESCRIPTION
*
*       This function sets the MAC address of the specified ethernet
*       device.  Since this is a spi based interface, this function does
*       nothing.
*
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle - Handle to ethernet device
*       UINT8           *ether_addr           - Pointer to a buffer where the MAC address is
*                                             stored.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service
*                                             is successful. Otherwise an
*                                             error code is returned.
*
**************************************************************************/
STATUS  IF_SPI_Tgt_Set_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr)
{
    
    return (NU_SUCCESS); 
}   /* IF_SPI_Tgt_Set_Address */

/*************************************************************************
*
*   FUNCTION
*
*     IF_SPI_Tgt_Receive_Packet
*
*   DESCRIPTION
*
*       This function processes a new inbound frame in the RX DATA FIFO.
*
*   INPUTS
*
*      *device                    Device control block pointer.
*
*   OUTPUTS
*
*      None.
*
**************************************************************************/
STATUS IF_SPI_Tgt_Receive_Packet(DV_DEVICE_ENTRY *device)
{

    return (NU_FALSE);
}


/*************************************************************************
*
*   NOTE:
*   All functions listed below this point are used by the Generic IF_SPI
*   driver layer. The function names and parameters must remain the same,
*   however the function content will be specific to this device.
*
*************************************************************************/

/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Enable
*
*   DESCRIPTION
*
*       This function enables the LM3S_ETH hardware
*
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IF_SPI_Tgt_Enable (ETHERNET_INSTANCE_HANDLE *inst_handle)
{

    if (inst_handle != NU_NULL)
    {
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Disable
*
*   DESCRIPTION
*
*       This function disables the LM3S_ETH hardware
*
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IF_SPI_Tgt_Disable (ETHERNET_INSTANCE_HANDLE *inst_handle)
{

    if (inst_handle != NU_NULL)
    {
    }
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Create_Extended_Data
*
*   DESCRIPTION
*
*       This function creates the LM3S_ETH extended data structure then attaches
*       it to the device.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the structure of the
*                                             device.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS or error code
*
**************************************************************************/
STATUS IF_SPI_Tgt_Create_Extended_Data (DV_DEVICE_ENTRY *device)
{
    STATUS          status = NU_SUCCESS;
    VOID            *pointer;
    NU_MEMORY_POOL  *sys_pool_ptr;

    /* Allocate the memory required for the ethernet data. */
    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
    status = NU_Allocate_Memory (sys_pool_ptr, &pointer, sizeof(STM32_EMAC_XDATA), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Report error if memory allocation failed. */
        NLOG_Error_Log ("Failed to allocate memory for ethernet driver",
                        NERR_FATAL, __FILE__, __LINE__);
    }

    else
    {
        /* Clear the data. */
        UTL_Zero (pointer, sizeof (STM32_EMAC_XDATA));

        /* Use the user field in the device control block to store the
           extended device data. */
        device->user_defined_1 = (UINT32) pointer;

    }

    return (status);
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Get_ISR_Info
*
*   DESCRIPTION
*
*       This function does nothing as there is no isr functionality.
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the structure of the
*                                             device.
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS or error code
*
**************************************************************************/
VOID IF_SPI_Tgt_Get_ISR_Info (ETHERNET_SESSION_HANDLE *ses_handle, ETHERNET_ISR_INFO *isr_info)
{
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Target_Initialize
*
*   DESCRIPTION
*
*		There is no target initialization code for spi network interface
*
*   INPUTS
*		ETHERNET_INSTANCE_HANDLE *inst_handle
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID IF_SPI_Tgt_Target_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device)
{
}   /* IF_SPI_Tgt_Target_Initialize */

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Controller_Init
*
*   DESCRIPTION
*
*       This function is responsible for initializing and setting up the
*       LWSPI interface for the physical layer.
*
*   INPUTS
*
*       UINT8           *ether_addr         - Pointer to the ethernet address.
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS IF_SPI_Tgt_Controller_Init (ETHERNET_INSTANCE_HANDLE *inst_handle, 
                                   DV_DEVICE_ENTRY *device, UINT8 *ether_addr)
{
    STATUS                  status = NU_SUCCESS;
    IF_SPI_TARGET_DATA      *tgt_ptr = (IF_SPI_TARGET_DATA*)inst_handle->tgt_ptr;
    NU_MEMORY_POOL          *sys_pool_ptr;
    VOID                    *pointer;
    ETHERNET_SESSION_HANDLE *sess;


    status =  NU_SPI_Register(tgt_ptr->spi_bus_name, tgt_ptr->spi_baud_rate, 
                              SPI_CFG_16Bit, 
                              IF_SPI_CONFIG | tgt_ptr->spi_dev_ctrl,
                              &(tgt_ptr->spiHandle));

    if (status == NU_SUCCESS)
    {
      status = NU_SPI_DMA_Setup(tgt_ptr->spiHandle);
    }
    
    if (status == NU_SUCCESS)
    {
        /* Get the IF_SPI MAC address. */
        IF_SPI_Tgt_Get_Address (inst_handle, ether_addr);

        /* Set the IF_SPI MAC address to the controller registers. */
        IF_SPI_Tgt_Set_Address (inst_handle, ether_addr);
    }

    /* Create and start the send/receive thread for the ifspi interface */
    if (status == NU_SUCCESS)
    {
      status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
    }

    if (status == NU_SUCCESS)
    {
      status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&pointer, 
                                   IFSPI_TASK_STACK_SIZE, NU_NO_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&sess, 
                                   sizeof(ETHERNET_SESSION_HANDLE), NU_NO_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        sess->device = device;
        sess->inst_info = inst_handle;

        /* clear out tcb before creating task */
        memset(&(tgt_ptr->tcb), 0, sizeof(NU_TASK));

        status = NU_Create_Task(&(tgt_ptr->tcb), inst_handle->name, IFSPI_Task_Entry,
                                1, sess, pointer, IFSPI_TASK_STACK_SIZE,
                                IFSPI_TASK_PRIORITY, IFSPI_TASK_TIMESLICE,
                                NU_PREEMPT, NU_START);
    }


    return (status);
}   /* IF_SPI_Tgt_Controller_Init */

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Set_Phy_Dev_ID
*
*   DESCRIPTION
*
*       This function puts the IF_SPI device ID into the PHY structure
*   INPUTS
*
*       VOID            *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       NONE
*
**************************************************************************/
VOID IF_SPI_Tgt_Set_Phy_Dev_ID (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Get_Link_Status
*
*   DESCRIPTION
*
*       This function calls the PHY specific function to get link status
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*       INT             *link_status            - Pointer to where link status
*                                                 will be returned
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS IF_SPI_Tgt_Get_Link_Status (ETHERNET_INSTANCE_HANDLE *inst_handle, INT *link_status)
{
    STATUS      status = NU_SUCCESS;

    *link_status = PHY_LINK_UP;
    return(status);
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Update_Multicast
*
*   DESCRIPTION
*
*       No Multicast implementation for IF SPI
*
*   INPUTS
*
*       DV_DEVICE_ENTRY *device             - Pointer to the device
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID    IF_SPI_Tgt_Update_Multicast (DV_DEVICE_ENTRY *device)
{

}   /* EMAC_Update_Multicast */

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Phy_Initialize
*
*   DESCRIPTION
*
*       This function calls the PHY specific initialization function
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*       DV_DEVICE_ENTRY *device                 - Pointer to the device
*
*   OUTPUTS
*
*       STATUS          status              - Returns NU_SUCCESS if service is successful.
*                                             Otherwise an error code is returned.
*
**************************************************************************/
STATUS IF_SPI_Tgt_Phy_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device)
{
    return(NU_SUCCESS);
}

/**************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Notify_Status_Change
*
*   DESCRIPTION
*
*       This function calls the specific PHY link status change function
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*
*   OUTPUTS
*
*       NONE
*
**************************************************************************/
VOID IF_SPI_Tgt_Notify_Status_Change (ETHERNET_INSTANCE_HANDLE *inst_handle)
{
}



/**************************************************************************
*
*   FUNCTION
*
*       IFSPI_Task_Entry
*
*       ahonkan terabit radios
*      
*   DESCRIPTION
*
*       This thread 
*
*   INPUTS
*
*       ETHERNET_INSTANCE_HANDLE *inst_handle   - Device instance handle
*
*   OUTPUTS
*
*       NONE
*
**************************************************************************/
static VOID IFSPI_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS                    status;        
    ETHERNET_SESSION_HANDLE   *sess   = argv;
    DV_DEVICE_ENTRY           *device = sess->device; 
    ETHERNET_INSTANCE_HANDLE  *inst_handle =  sess->inst_info;
    IF_SPI_TARGET_DATA        *tgt_ptr = (IF_SPI_TARGET_DATA*)inst_handle->tgt_ptr;   
    NET_BUFFER                *buf_ptr = NU_NULL;
    UINT32                    pktSize;

    /* Tracking vars for receive path */
    NET_BUFFER*               headP = NULL;
    NET_BUFFER*               prevP = NULL;
    NET_BUFFER*               currP = NULL;
    

  tgt_ptr->tx_frame.sof = IFSPI_FRAME_SOF;
  tgt_ptr->tx_frame.len = IFSPI_FRAME_BLANK;
  memset(&(tgt_ptr->tx_frame.data), 0, IF_SPI_BUF_SIZE);
  tgt_ptr->tx_frame.eof = IFSPI_FRAME_EOF;

  
  /* Sync link if master.  The sync process will make sure framing lines up
   * on asynchronous master/slave interface power up.
   */
  if (tgt_ptr->spi_dev_ctrl == SPI_CFG_DEV_MASTER)
  {}


  /* Begin tx/rx loop */
  while(1)
  {
    /*--------------------- Prepare Data to TX ------------------------------*/

    tgt_ptr->tx_frame.sof = IFSPI_FRAME_SOF;
    tgt_ptr->tx_frame.eof = IFSPI_FRAME_EOF;
  
    /* No data available to send, assemble blank frame */
    if ((device->dev_transq.head == NULL) && 
         (buf_ptr == NU_NULL))
    {
      tgt_ptr->tx_frame.len = IFSPI_FRAME_BLANK;
      memset(&(tgt_ptr->tx_frame.data), 0, IF_SPI_BUF_SIZE);
    }
          
    /* Detected the transition where the local buffer ptr is null while
     * the ethernet driver has data to send.  
     */
    else if ((device->dev_transq.head != NULL) && 
         (buf_ptr == NU_NULL))
    {
      buf_ptr = device->dev_transq.head;

      /* Protect buffer boundary.  This condition
       *  should NEVER happen. 
       */
      if (buf_ptr->data_len > IF_SPI_BUF_SIZE)
        buf_ptr->data_len = IF_SPI_BUF_SIZE;

      tgt_ptr->tx_frame.len = IFSPI_FRAME_FS | buf_ptr->data_len;
      memcpy(&(tgt_ptr->tx_frame.data), buf_ptr->data_ptr, buf_ptr->data_len);

      buf_ptr = buf_ptr->next_buffer;

      if (buf_ptr == NULL)
          DEV_Recover_TX_Buffers (device);

    }

    /* Otherwise if the buf_ptr is not null, the system is in middle
     * of transmitting data
     */
    else if ((buf_ptr != NULL) && (buf_ptr->data_len != 0))
    {
      /* Protect buffer boundary.  This condition 
       * should NEVER happen. 
       */
      if (buf_ptr->data_len > IF_SPI_BUF_SIZE)
        buf_ptr->data_len = IF_SPI_BUF_SIZE;

      tgt_ptr->tx_frame.len = buf_ptr->data_len;
      memcpy(&(tgt_ptr->tx_frame.data), buf_ptr->data_ptr, buf_ptr->data_len);

      buf_ptr = buf_ptr->next_buffer;

      if (buf_ptr == NULL)
          DEV_Recover_TX_Buffers (device);

    }
    
    /* Catch-all condition- occurs if the data_len is 0 while the
     * buff_ptr is not null.  Something happened in the stack code that
     * caused this condition and needs to be investigated.
     */
    else
    {
      tgt_ptr->tx_frame.len = IFSPI_FRAME_BLANK;
      memset(&(tgt_ptr->tx_frame.data), 0, IF_SPI_BUF_SIZE);
    }

    /*--------------------- Initiate DMA ------------------------------*/

    /* Initiate DMA */
    status = NU_SPI_DMA_Transfer(tgt_ptr->spiHandle, 
                                 &(tgt_ptr->tx_frame),
                                 &(tgt_ptr->rx_frame),
                                 sizeof(IF_SPI_Frame)); 

    /*-------------------- Handle RX Data ------------------------------*/

    /* Check for the beginning of frame.  This occurs when 
     * the received frame has IFSPI_FRAME_FS
     * in the rx_frame.len element.
     */

    if (tgt_ptr->rx_frame.len & IFSPI_FRAME_FS)
    {
      /* If head is not null, it means a new frame was received and 
       * the previous one needs to be dispatched 
       */            
      if (headP != NULL)
      {
        headP->mem_total_data_len = pktSize;

        /* Put head of the chain onto NET stack. */
        MEM_Buffer_Enqueue(&MEM_Buffer_List, headP);

        /* Set NET notification event to show at least 1 frame was successfully received */
        NU_Set_Events (&Buffers_Available, (UNSIGNED)2, NU_OR);

        /* Clear out head pointer */
        headP = NULL;

      }

      /* Mask out the First Segment (FS) value and get data length */            
      tgt_ptr->rx_frame.len &= IFSPI_DATA_LEN_MASK;

      currP = (UINT32)MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

      if ((currP != NULL) && 
          (tgt_ptr->rx_frame.len <= IF_SPI_BUF_SIZE))
      {
        memcpy(currP, tgt_ptr->rx_frame.data, tgt_ptr->rx_frame.len);
        currP->mem_buf_device = device;
        currP->data_ptr = currP->mem_parent_packet;
        currP->data_len = tgt_ptr->rx_frame.len;
        pktSize = tgt_ptr->rx_frame.len;

        headP = currP;
        prevP = currP;
      }
    }

    /* This is a continuation of a larger frame, so if head is not null
     * continue linking buffers.
     */

    else if (headP != NULL) 
    {
      /* If the received frame length is 0 then the frame is terminated.
       * dispatch this one to the stack
       */

      if (tgt_ptr->rx_frame.len == 0)
      {
        headP->mem_total_data_len = pktSize;

        /* Put head of the chain onto NET stack. */
        MEM_Buffer_Enqueue(&MEM_Buffer_List, headP);

        /* Set NET notification event to show at least 1 frame was successfully received */
        NU_Set_Events (&Buffers_Available, (UNSIGNED)2, NU_OR);

        /* Clear out head pointer */
        headP = NULL;

      }

      /* Otherwise link buffers */
      else
      {
        currP = (UINT32)MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

        if ((currP != NULL) && 
            (tgt_ptr->rx_frame.len <= IF_SPI_BUF_SIZE))
        {
          memcpy(currP, tgt_ptr->rx_frame.data, tgt_ptr->rx_frame.len);
          currP->mem_buf_device = device;
          currP->data_ptr = currP->mem_parent_packet;
          currP->data_len = tgt_ptr->rx_frame.len;
          pktSize += tgt_ptr->rx_frame.len;

          prevP->next_buffer = currP;
          prevP = currP;
        }

      }

    }

    /* In master mode, set polling period to 1 tick */          
    if (tgt_ptr->spi_dev_ctrl == SPI_CFG_DEV_MASTER)
      NU_Sleep(1);

  }
}

