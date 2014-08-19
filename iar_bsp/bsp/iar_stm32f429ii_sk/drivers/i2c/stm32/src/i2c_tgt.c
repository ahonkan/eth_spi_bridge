/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       i2c_tgt.c
*
*   COMPONENT
*
*       STM32                               - STM32 controller driver
*
*   DESCRIPTION
*
*       This file contains the Serial Driver specific functions.
*
*   FUNCTIONS
*
*       nu_bsp_drvr_i2c_stm32_init
*       I2C_Tgt_Register
*       I2C_Tgt_Open
*       I2C_Tgt_Init
*       I2C_Tgt_Shutdown
*       I2C_Tgt_Unregister
*       I2C_Tgt_Close
*       I2C_Tgt_Read
*       I2C_Tgt_Write
*       I2C_Tgt_Ioctl
*       I2C_Tgt_Driver_ISR
*       I2C_Tgt_Driver_Set_Baudrate
*       I2C_Tgt_Enable_Interrupt
*       I2C_Tgt_Disable_Interrupt
*       I2C_Tgt_Enable
*       I2C_Tgt_Disable
*
*   DEPENDENCIES
*
*       string.h
*       reg_api.h
*       i2c_driver_extr.h
*       i2c_driver_defs.h
*       stm32.h
*       power_core.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"
#include "bsp/drivers/cpu/stm32f2x/cpu_tgt.h"
#include "bsp/drivers/i2c/stm32/i2c_tgt.h"

/*********************************/
/* Local function prototypes     */
/*********************************/

static STATUS _Get_Target_Info(const CHAR * key, I2C_INSTANCE_HANDLE *inst_info);
VOID   I2C_Tgt_Driver_ISR (INT vector);

/*********************************/
/* External function             */
/*********************************/

extern STATUS I2C_Get_Target_Info(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Set_Reg_Path(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Call_Setup_Func(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Pwr_Get_Clock_Rate(I2C_INSTANCE_HANDLE *i_handle, UINT32 *clock_rate);

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_i2c_stm32_init
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
*       I2C_Tgt_Register
*       I2C_Tgt_Unregister
*
*   INPUTS
*
*       CHAR     *key                       - Key
*       INT      starstop                   - Start or Stop flag
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_bsp_drvr_i2c_stm32_init (const CHAR * key, INT startstop)
{
    STATUS               status;
    NU_MEMORY_POOL      *sys_pool_ptr;
    I2C_INSTANCE_HANDLE *i_handle;
    VOID                *pointer;
    static DV_DEV_ID     dev_id;
    VOID                (*setup_fn)(VOID) = NU_NULL;
    VOID                (*cleanup_fn)(VOID) = NU_NULL;
    I2C_TGT             *tgt_ptr;
    STATUS              reg_status;

    if (startstop)
    {
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a new instance */
            status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                         sizeof(I2C_INSTANCE_HANDLE), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                (VOID)memset(pointer, 0, sizeof(I2C_INSTANCE_HANDLE));

                i_handle = (I2C_INSTANCE_HANDLE*)pointer;

                /* ah: terabit
                 * Substituting local get info function to set up
                 * periperal much easier
                 */
                status = _Get_Target_Info(key, i_handle);

                if (status == NU_SUCCESS)
                {
                    status = I2C_Set_Reg_Path(key, i_handle);
                }

                if (status == NU_SUCCESS)
                {
                    /* Allocate a new instance */
                    status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                                 sizeof(I2C_TGT), NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        memset (pointer, 0, sizeof(I2C_TGT));
                        tgt_ptr = (I2C_TGT *)pointer;

                        /* Get setup function */
                         reg_status = REG_Get_UINT32_Value(key, "/setup", (UINT32*)&setup_fn);
                         if (reg_status == NU_SUCCESS)
                         {
                             if (setup_fn != NU_NULL)
                             {
                                 tgt_ptr->setup_func = setup_fn;
                             }
                         }

                        /* Get cleanup function */
                        reg_status = REG_Get_UINT32_Value(key, "/cleanup", (UINT32*)&cleanup_fn);
                        if (reg_status == NU_SUCCESS)
                        {
                            if (cleanup_fn != NU_NULL)
                            {
                                tgt_ptr->cleanup_func = cleanup_fn;
                                cleanup_fn();
                            }
                        }

                        i_handle->i2c_reserved = tgt_ptr;
                    }
                }

                if (status == NU_SUCCESS)
                {
                    /* Call the STM32 component registration function */
                    status = I2C_Dv_Register (key, i_handle);
                }

                if (status != NU_SUCCESS)
                {
                    /* De-allocate memory. */
                    NU_Deallocate_Memory(pointer);
                    status = I2C_NO_INSTANCE_AVAILABLE;
                }

            }
        }
    }
    else
    {
        /* Unregister driver */
        I2C_Dv_Unregister (key, startstop, dev_id);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Init
*
*   DESCRIPTION
*
*       This function initializes the I2C module
*
*   INPUTS
*
*       VOID          *instance_handle      - Session handle of the driver
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS  I2C_Tgt_Init(VOID *session_handle)
{
    I2C_SESSION_HANDLE  *s_handle = (I2C_SESSION_HANDLE*)session_handle;
    VOID                (*old_lisr)(INT);
    UINT8               *i2c_base_address = I2C_GET_BASE_ADDRESS(s_handle);
    STATUS              status  = NU_SUCCESS;
    UINT                i2c_vector = I2C_GET_VECTOR(s_handle);
    I2C_TGT             *tgt_ptr = (I2C_TGT *)s_handle->instance_handle->i2c_reserved;

    /* Call the target specific setup function. */
    if(tgt_ptr->setup_func != NU_NULL)
    {
        tgt_ptr->setup_func();
    }

    /* Reset controller to ensure default working state. */
    I2C_TGT_OR_OUT(i2c_base_address + I2C_CR1, I2C_CR1_SWRST);

    while (!(ESAL_GE_MEM_READ16(i2c_base_address+I2C_CR1) & I2C_CR1_SWRST));

    I2C_TGT_AND_OUT(i2c_base_address + I2C_CR1, ~(I2C_CR1_SWRST));

    /* Enable the I2C module. */
    ESAL_GE_MEM_WRITE16(i2c_base_address + I2C_CR1,
                        (ESAL_GE_MEM_READ16(i2c_base_address + I2C_CR1)
                        | I2C_CR1_PE));

    /* Register the interrupt service routine for this device. */
    status = NU_Register_LISR(i2c_vector, I2C_Tgt_Driver_ISR, &old_lisr);

    /* Check if registration is successful. */
    if (status == NU_SUCCESS)
    {
        /* Register the I2C data structure with this vector ID. */
        ESAL_GE_ISR_VECTOR_DATA_SET(i2c_vector,
                                    (VOID *)session_handle);
    }

    /* Return the initialization status. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Shutdown
*
*   DESCRIPTION
*
*       This function closes the I2C controller.
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID I2C_Tgt_Shutdown(I2C_INSTANCE_HANDLE *i_handle)
{
    I2C_TGT *tgt_ptr = (I2C_TGT *)i_handle->i2c_reserved;

    /* Check for null and return the memory allocated to i2c_reserved. */
    if (i_handle->i2c_reserved != NU_NULL)
    {
        /* Call the target specific cleanup function. */
        if(tgt_ptr->cleanup_func != NU_NULL)
        {
            tgt_ptr->cleanup_func();
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Read
*
*   DESCRIPTION
*
*       This function reads a byte from the I2C bus
*
*   INPUTS
*
*       VOID*           session_handle      - Session handle
*       VOID*           buffer              - Buffer to copy data into
*       INT             numbyte             - Size of buffer
*
*   OUTPUTS
*
*       INT                                 - Size of data copied into buffer
*
*************************************************************************/
STATUS     I2C_Tgt_Read (VOID* session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read)
{
    I2C_SESSION_HANDLE  *s_handle = (I2C_SESSION_HANDLE*)session_handle;
    STATUS              status = NU_SUCCESS;
    UINT8               *i2c_base_address = I2C_GET_BASE_ADDRESS(s_handle);

    /* Check if driver is operating in polling mode. */
#if ( CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE )
    UINT32              timeout = 10000;

    /* Wait while data becomes available. */
    while ((!(ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1) & I2C_SR1_RXNE)) && timeout-- );

    if (timeout == 0)
    {
        status = I2C_DATA_RECEPTION_FAILED;
    }

#endif

    /* Get data from the register. */
    *((UINT8*)buffer) = ESAL_GE_MEM_READ16(i2c_base_address + I2C_DR);

    /* Read 1 byte */
    *bytes_read = 1;

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Write
*
*   DESCRIPTION
*
*       This function sends a byte to the I2C bus
*
*   INPUTS
*
*       VOID*           session_handle      - Session handle
*       VOID*           buffer              - Buffer to copy data from
*       INT             numbyte             - Size of data in buffer
*
*   OUTPUTS
*
*       INT                                 - Size of data written to device
*
*************************************************************************/
STATUS     I2C_Tgt_Write(VOID* session_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written)
{
    I2C_SESSION_HANDLE  *s_handle = (I2C_SESSION_HANDLE*)session_handle;
    STATUS              status = NU_SUCCESS;
    UINT8               data = *(UINT8*)buffer;
    UINT8               *i2c_base_address = I2C_GET_BASE_ADDRESS(s_handle);

#if ( CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE )
    UINT32              timeout = 10000;
#endif

    /* Write the data to data register for transmission. */
    ESAL_GE_MEM_WRITE16(i2c_base_address + I2C_DR, data);

    /* Check if driver is operating in polling mode. */
#if ( CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE )

    /* Wait while Tx operation is complete. */
    while ((!(ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1) & I2C_SR1_TXE)) && timeout--);

    if (timeout == 0)
    {
        status = I2C_DATA_TRANSMISSION_FAILED;
    }

#endif

    *bytes_written = 1;

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Local_Send_Start_Address
*
*   DESCRIPTION
*
*       This is a local function called by send start and restart
*       functions
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
static STATUS   I2C_Tgt_Local_Send_Start_Address(I2C_INSTANCE_HANDLE *i_handle, VOID *data)
{
    STATUS  status = NU_SUCCESS;
    UINT8  *i2c_base_address = (UINT8*)(i_handle->io_addr);
    I2C_CB *i2c_cb = i_handle->i2c_cb;


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE pmi_dev_entry = i_handle->pmi_dev;

    /* Check current state of the device. If the device is off, suspend on an
       event until the device power state changes to ON */
    if ((PMI_STATE_GET(pmi_dev_entry) == I2C_OFF)||(PMI_IS_PARKED(pmi_dev_entry) == NU_TRUE))
    {
        /* Wait until the device is available for a write operation */
        PMI_WAIT_CYCLE(pmi_dev_entry, status);
    }

#endif

#if ( CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE )

    /* Start the transfer */
    I2C_TGT_OR_OUT(i2c_base_address + I2C_CR1, I2C_CR1_START);

    /* Wait until SB is set to indicate that a start condition has
       been generated. */
    while ((ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1) & I2C_SR1_SB) != I2C_SR1_SB);

    /* Set the Address */
    if (i2c_cb->i2c_api_mode == I2C_AUTOMATIC_RX_API)
    {
        ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_DR),
                            ((( *((UINT16*)data) >> 1) << I2C_ADD_SHIFT) | 0x1));

        /* Set NACK if its a single byte transfer. Set auto ACK otherwise. */
        if (i2c_cb->i2c_io_buffer.i2cbm_bytes_to_receive == 1)
        {
            I2C_TGT_AND_OUT(i2c_base_address + I2C_CR1, ~(I2C_CR1_ACK_ENABLED));
        }
        else
        {
            I2C_TGT_OR_OUT(i2c_base_address + I2C_CR1, I2C_CR1_ACK_ENABLED);
        }
    }
    else
    {
        ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_DR),
                            (( *((UINT16*)data) >> 1) << I2C_ADD_SHIFT));
    }

    /* Wait until Address sent confirmation is set. */
    while ((ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1) & I2C_SR1_ADDR) != I2C_SR1_ADDR);

    /* Read of SR2 as mentioned in the hardware manual to clear the ADDR
       field in SR1. */
    (VOID)ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2);

    i2c_cb->i2c_reserved2 = I2C_DRIVER_MODE_MASTER;

#elif ( !CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE )

    if (i2c_cb->i2c_api_mode == I2C_AUTOMATIC_RX_API)
    {
        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count =0;
    }

    if (i_handle->i2c_reserved != NU_NULL)
    {
        I2C_TGT *tgt_ptr = (I2C_TGT *)i_handle->i2c_reserved;

        tgt_ptr->slave_addr = *((UINT16*)data);

        /* Start the transfer */
        I2C_TGT_OR_OUT(i2c_base_address + I2C_CR1, I2C_CR1_START);
    }
    else
    {
        status = ~(NU_SUCCESS);
    }

#endif

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Start_Address
*
*   DESCRIPTION
*
*       This function sends start signal with address
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS   I2C_Tgt_Send_Start_Address(I2C_INSTANCE_HANDLE *i_handle, VOID *data)
{
    STATUS  status;
    UINT8  *i2c_base_address = (UINT8*)(i_handle->io_addr);

    /* Check if bus is free or if it is send restart command. */
    if(I2C_DRIVER_CHECK_BUS_FREE(i2c_base_address) == 1)
    {
        status = I2C_Tgt_Local_Send_Start_Address(i_handle, data);
    }
    else
    {
      /* Set status to indicate that I2C bus is occupied
         already probably by some other master. */
        status = I2C_BUS_BUSY;
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Restart_Address
*
*   DESCRIPTION
*
*       This function sends restart signal with address
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS   I2C_Tgt_Send_Restart_Address(I2C_INSTANCE_HANDLE *i_handle, VOID *data)
{
    STATUS status;
    status = I2C_Tgt_Send_Start_Address(i_handle, data);
    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Address2
*
*   DESCRIPTION
*
*       This function sends second byte of 10-bit address
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS   I2C_Tgt_Send_Address2(I2C_INSTANCE_HANDLE *i_handle, VOID *data)
{
    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Stop
*
*   DESCRIPTION
*
*       This function sends stop
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Send_Stop(I2C_INSTANCE_HANDLE *i_handle)
{
    UINT8               *i2c_base_address = (UINT8*)(i_handle->io_addr);
    UINT32              timeout = 10000;

    /* Send STOP signal. */
    I2C_DRIVER_SEND_STOP_SIGNAL(i2c_base_address);

    /* Wait till bus is busy. */
    while((ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2) & I2C_SR2_MSL) && timeout--);

    if (timeout == 0)
    {
        return I2C_BUS_BUSY;
    }

    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Ack
*
*   DESCRIPTION
*
*       This function sends ACK
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Send_Ack(I2C_INSTANCE_HANDLE *i_handle)
{
    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Send_Nack
*
*   DESCRIPTION
*
*       This function sends NACK
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Send_Nack(I2C_INSTANCE_HANDLE *i_handle)
{
    UINT8           *i2c_base_address = (UINT8*)(i_handle->io_addr);
    STATUS          status = NU_SUCCESS;

    I2C_TGT_AND_OUT (i2c_base_address + I2C_CR1, ~(I2C_CR1_ACK_ENABLED));

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Check_Data_Ack
*
*   DESCRIPTION
*
*       This function checks if data ACK is received
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Check_Data_Ack(I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS  status;
    UINT8  *i2c_base_address = (UINT8*)(i_handle->io_addr);

    if(I2C_DRIVER_ACK_DETECTED(i2c_base_address))
    {
        status = NU_SUCCESS;
    }
    else
    {
        status = I2C_SLAVE_NOT_ACKED;
    }
    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Check_Bus_Free
*
*   DESCRIPTION
*
*       This function checks if bus is free
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Check_Bus_Free(I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS  status;
    UINT8  *i2c_base_address = (UINT8*)(i_handle->io_addr);

    if(I2C_DRIVER_CHECK_BUS_FREE(i2c_base_address))
    {
        status = NU_SUCCESS;
    }
    else
    {
         /* Set status to indicate that I2C bus is occupied. */
        status = I2C_BUS_BUSY;
    }
    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Check_Address_Ack
*
*   DESCRIPTION
*
*       This function checks if address ACK has been received
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Check_Address_Ack(I2C_INSTANCE_HANDLE *i_handle)
{
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Set_Node_Mode_RX
*
*   DESCRIPTION
*
*       This function sets node mode to RX
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Set_Node_Mode_RX(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Not used for this driver. */

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Set_Node_Mode_TX
*
*   DESCRIPTION
*
*       This function sets the node mode to TX
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Set_Node_Mode_TX(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Not used for this driver. */

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Free_Bus
*
*   DESCRIPTION
*
*       This function frees the bus
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Free_Bus(I2C_INSTANCE_HANDLE *i_handle)
{
    UINT8           *i2c_base_address = (UINT8*)(i_handle->io_addr);
    UINT32          timeout = 10000;

    /* Check if the bus is occupied. */
    if(I2C_DRIVER_CHECK_BUS_FREE(i2c_base_address) == 0)
    {
        /* Generate stop signal. */
        I2C_DRIVER_SEND_STOP_SIGNAL(i2c_base_address);

        /* Wait till bus is busy. */
        while((I2C_DRIVER_CHECK_BUS_FREE(i2c_base_address) == 0) && timeout--);

        if(timeout == 0)
            return (I2C_TRANSFER_STOP_FAILED);

    }

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Set_Slave_Address
*
*   DESCRIPTION
*
*       This function sets slave address
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Set_Slave_Address(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Slave mode not supported. */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Set_Master_Mode
*
*   DESCRIPTION
*
*       This function sets master mode
*
*   INPUTS
*
*       I2C_INSTANCE_HANDLE    *i_handle    - Device instance handle
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS I2C_Tgt_Set_Master_Mode(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Master mode is enabled by default and cannot be changed. */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       I2C_Tgt_Driver_ISR
*
* DESCRIPTION
*
*       This is the entry function for the ISR (Interrupt Service Routine)
*       that services the I2C module.
*
* INPUTS
*
*       vector                 Interrupt vector.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    I2C_Tgt_Driver_ISR(INT vector)
{
    I2C_CB             *i2c_cb;
    UINT8              *i2c_base_address;
    STATUS              status = NU_SUCCESS;
    INT                 dev_id = 0;
    UINT8               data_byte;
    I2C_HANDLE          i2c_handle;
    I2C_SESSION_HANDLE  *s_handle;
    UINT32              sr1_value, cr2_value;
    BOOLEAN             complete_read = NU_FALSE;

    /* Declare variables required by critical section handling. */
    INT     old_level;

    /* Start critical section. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    s_handle = (I2C_SESSION_HANDLE  *)ESAL_GE_ISR_VECTOR_DATA_GET(vector);

    /* Get a pointer to the I2C structure for this vector. */
    i2c_cb = I2C_GET_CB_FROM_SESSION_HANDLE(s_handle);


     /* Get base address of the I2C module. */
    i2c_base_address = I2C_GET_BASE_ADDRESS(s_handle);

    I2CS_Get_Handle(i2c_cb, &i2c_handle);

    cr2_value = ESAL_GE_MEM_READ16(i2c_base_address + I2C_CR2);

    /* See if it is an event interrupt. */
    if (cr2_value & I2C_CR2_ITEVFEN)
    {
        sr1_value = ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1);

        /* Start byte sent interrupt. */
        if (sr1_value & I2C_SR1_SB)
        {
            if (s_handle->instance_handle->i2c_reserved != NU_NULL)
            {
                I2C_TGT *tgt_ptr = (I2C_TGT *)s_handle->instance_handle->i2c_reserved;

                /* Set the Address */
                if (i2c_cb->i2c_api_mode == I2C_AUTOMATIC_RX_API)
                {
                    ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_DR),
                                        (((tgt_ptr->slave_addr >> 1) << I2C_ADD_SHIFT) | 0x1));
                }
                else
                {
                    ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_DR),
                                        ((tgt_ptr->slave_addr >> 1) << I2C_ADD_SHIFT));
                }
            }
            else
            {
                status = ~(NU_SUCCESS);
            }
        }
        /* Address ack interrupt. */
        else if (sr1_value & I2C_SR1_ADDR)
        {
            /* Set the Address */
            if (i2c_cb->i2c_api_mode == I2C_AUTOMATIC_RX_API)
            {
                if (i2c_cb->i2c_io_buffer.i2cbm_bytes_to_receive == 1)
                {
                    I2C_TGT_AND_OUT(i2c_base_address + I2C_CR1, ~(I2C_CR1_ACK_ENABLED));

                    /* Read of SR2 as mentioned in the hardware manual to clear the ADDR
                       field in SR1. */
                    (VOID)ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2);
                }
                else
                {
                    /* Read of SR2 as mentioned in the hardware manual to clear the ADDR
                       field in SR1. */
                    (VOID)ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2);

                    I2C_TGT_OR_OUT(i2c_base_address + I2C_CR1, I2C_CR1_ACK_ENABLED);
                }
            }
            else
            {
                /* Read of SR2 as mentioned in the hardware manual to clear the ADDR
                   field in SR1. */
                (VOID)ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2);
            }
        }
        else if (sr1_value & I2C_SR1_TXE)
        {
            /* Check if automatic transmission API is in operation. */
            if (i2c_cb->i2c_api_mode == I2C_AUTOMATIC_TX_API)
            {
                /* Get the data byte from the buffer. */
                status = I2CBM_Get_Output_Buffer(i2c_cb, &data_byte);

                /* Check if data retrieved successfully from
                         the buffer. */
                if (status == NU_SUCCESS)
                {
                    /* Write the data to data register for transmission. */
                    I2C_DRIVER_SEND_DATA(i2c_base_address, data_byte);
                }
                /* The last data byte has been transmitted. */
                else
                {
                    /* Send STOP pulse on the line with last data. */
                    status = I2C_Tgt_Send_Stop(s_handle->instance_handle);

                    /* Set the handler that address
                       acknowledgment from slave has been
                       received. */
                    i2c_cb->i2c_handler_type = I2C_MASTER_DATA_TX_COMPLETE;

                    /* Call the handler to pass it to
                       application through a HISR. */
                    I2C_Handler(i2c_cb, NU_TRUE);

                    /* Check if it is multi transfer and
                       another slave needs to be processed. */
                    if (I2C_MORE_TRANSFER_LEFT(i2c_cb))
                    {
                        status = I2CMC_Process_Next_Slave(i2c_handle);
                    }

                    /* No more slave need to be processed. */
                    else
                    {
                        /* Set the node state to idle. */
                        i2c_cb->i2c_node_state = I2C_NODE_IDLE;
                    }
                }
            }
        }
        else if (sr1_value & I2C_SR1_RXNE)
        {
            /* Get the data from the data register. */
            data_byte = I2C_DRIVER_GET_DATA(i2c_base_address);

            complete_read = NU_TRUE;
        }

        if (complete_read == NU_TRUE)
        {
            /* Put the data in the input buffer. */
            status = I2CBM_Put_Input_Buffer(i2c_cb, data_byte);

            /* Transfer finished? */
            if (i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count ==
                i2c_cb->i2c_io_buffer.i2cbm_bytes_to_receive)
            {
                /* Send STOP signal. */
                status = I2C_Tgt_Send_Stop(s_handle->instance_handle);

                /* Set the handler that data has been received. */
                i2c_cb->i2c_handler_type = I2C_MASTER_DATA_RECEIVED;

                /* Call the handler to pass it to
                   application through a HISR. */
                I2C_Handler(i2c_cb, NU_TRUE);

                /* Set the node state to idle. */
                i2c_cb->i2c_node_state = I2C_NODE_IDLE;
            }
            /* Last byte ? */
            else if(i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count ==
                i2c_cb->i2c_io_buffer.i2cbm_bytes_to_receive - 1)
            {
                (VOID)I2C_Tgt_Send_Nack(s_handle->instance_handle);
            }
        }
    }

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(dev_id);
    NU_UNUSED_PARAM(i2c_handle);

    /* End critical section. */
    NU_Local_Control_Interrupts(old_level);
}

/*************************************************************************
* FUNCTION
*
*       I2C_Tgt_Driver_Set_Baudrate
*
* DESCRIPTION
*
*       This function sets the baud rate for Nucleus I2C network
*       transfer.
*
* INPUTS
*
*      i_handle                             Instance  handle
*
*
*       baudrate                            Baud rate to set for the
*                                           network transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_INVALID_BAUDRATE                Baud rate is invalid for
*                                           I2C network.
*
*************************************************************************/
STATUS  I2C_Tgt_Driver_Set_Baudrate (I2C_INSTANCE_HANDLE *i_handle, UINT16 baud_rate)
{
    STATUS      status = NU_SUCCESS;
    UINT32      ccr_bits = 0;
    UINT8*      i2c_base_address = ((UINT8*)(i_handle->io_addr));
    UINT32      clock_rate = ESAL_PR_TMR_OS_CLOCK_RATE;

    /* Check if a valid baud rate value was requested. */
    if (status == NU_SUCCESS)
    {

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        status = I2C_Tgt_Pwr_Get_Clock_Rate(i_handle, &clock_rate);

#else

        clock_rate = ESAL_PR_TMR_OS_CLOCK_RATE;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        clock_rate /= 1000000;

        if (baud_rate <= 100)
        {
            /* Set the peripheral clock speed in CR2. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_CR2, clock_rate);

            /* Disable peripheral for updating baudrate. */
            I2C_TGT_AND_OUT(i2c_base_address + I2C_CR1, (~(I2C_CR1_PE)));

            ccr_bits = ((clock_rate * 1000000) / ((baud_rate*1000) << 1));

            /* Make sure that the CCR bits value is within range. */
            if (ccr_bits < 4)
            {
                ccr_bits = 4;
            }

            /* Set the value for TRISE register. According to the manual, it should
               be clock rate + 1. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_TRISE, (clock_rate + 1));

            /* Program the CCR bits. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_CCR, ccr_bits);

            /* Enable the module again. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_CR1, I2C_CR1_PE);
        }
        /* i2c peripheral clock frequency must be a multiple of 10 for high speed. */
        else if ((baud_rate <= 400) && (clock_rate % 10 == 0))
        {
            /* Disable peripheral for updating baudrate. */
            I2C_TGT_AND_OUT(i2c_base_address + I2C_CR1, (~(I2C_CR1_PE)));

            ccr_bits = ((clock_rate * 1000000) / ((baud_rate*1000) * 25));

            /* Make sure that the CCR bits value is within range. */
            if (ccr_bits < 1)
            {
                ccr_bits = 1;
            }

            /* Set the value for TRISE register. According to the manual, it should
               be clock rate + 1. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_TRISE, (((clock_rate * 300) / 1000) + 1));

            /* Program the CCR bits. Also set the Fast Mode and Duty flags. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_CCR, (ccr_bits | I2C_CCR_FM | I2C_CCR_DUTY));

            /* Enable the module again. */
            I2C_TGT_OR_OUT (i2c_base_address + I2C_CR1, I2C_CR1_PE);
        }
        else
        {
            status = I2C_INVALID_BAUDRATE;
        }
    }

    /* Return the completion status of the service. */
    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Enable_Interrupt
*
*   DESCRIPTION
*
*       This function enables the I2C interrupt
*
*   INPUTS
*
*       VOID           *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID     I2C_Tgt_Enable_Interrupt(I2C_INSTANCE_HANDLE *i_handle)
{
#if (!CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE)

    UINT8  *i2c_base_address = (UINT8*)i_handle->io_addr;
    UINT32 i2c_vector = i_handle->irq;
    UINT32 priority = i_handle->irq_priority;
    INT     old_level;

    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Enable the I2C interrupt from the interrupt controller */
    (VOID)ESAL_GE_INT_Enable(i2c_vector,
                    ESAL_TRIG_NOT_SUPPORTED, priority);

    /* Disable all interrupts */
    ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_CR2),
                                    I2C_CR2_ITBUFEN | I2C_CR2_ITEVFEN);

    NU_Local_Control_Interrupts(old_level);
#endif
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Disable_Interrupt
*
*   DESCRIPTION
*
*       This function disables the I2C interrupt
*
*   INPUTS
*
*       VOID           *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID     I2C_Tgt_Disable_Interrupt(I2C_INSTANCE_HANDLE *i_handle)
{
#if (!CFG_NU_OS_CONN_I2C_POLLING_MODE_ENABLE)

    UINT8  *i2c_base_address = (UINT8*)i_handle->io_addr;
    UINT32 i2c_vector = i_handle->irq;
    INT     old_level;

    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Enable no I2C interrupts from the I2C controller*/
    ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_CR2), 0);

    /* Disable the I2C interrupt from the master interrupt controller */
    (VOID)ESAL_GE_INT_Disable(i2c_vector);

    NU_Local_Control_Interrupts(old_level);

#endif
}
/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Enable
*
*   DESCRIPTION
*
*       This function enables the I2C
*
*   INPUTS
*
*       VOID           *inst_handle         - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID     I2C_Tgt_Enable_Device (I2C_INSTANCE_HANDLE *i_handle)
{

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

    /* Update MPL for DVFS */
    (VOID)PMI_DVFS_Update_MPL_Value(i_handle->pmi_dev, PM_NOTIFY_ON);

    /* Set baud rate for the network transfer. */
    I2C_Tgt_Driver_Set_Baudrate (i_handle, i_handle->i2c_cb->i2c_baudrate);

#endif /* defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Disable
*
*   DESCRIPTION
*
*       This function places disables I2C
*
*   INPUTS
*
*       VOID            *i_handle        - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID I2C_Tgt_Disable_Device (I2C_INSTANCE_HANDLE *i_handle)
{

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

    /* Update MPL for DVFS */
    (VOID)PMI_DVFS_Update_MPL_Value(i_handle->pmi_dev, PM_NOTIFY_OFF);

#endif /* defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

}




/*************************************************************************
*
* FUNCTION
*
*       _Get_Target_Info
*
* DESCRIPTION
*
*       This local function retrieves I2C port target parameters and 
*       sets them to the Nucleus Serial parameters.  The STM hardware
*       parameters can be derived from a more basic set and it makes
*       the configuration simpler.
*
* INPUTS
*
*
* OUTPUTS
*
*       None
*
*************************************************************************/
static STATUS _Get_Target_Info(const CHAR * key, I2C_INSTANCE_HANDLE *inst_info)
{
    STATUS     status;
    UINT32     temp32;


    /* Process Serial Device and set the io address, vector and peripheral clock source 
     * i2c_dev:        [1,2,3] 
     */
    status = REG_Get_UINT32_Value(key, "/tgt_settings/i2c_dev", (UINT32*)&temp32);
    
    if (status != NU_SUCCESS)
      return  SERIAL_REGISTRY_ERROR;

    inst_info->number = temp32;
    inst_info->irq_type = ESAL_TRIG_LEVEL_LOW;
    strncpy(inst_info->ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);

    switch(temp32)
    {
      case 1: 
        inst_info->io_addr = I2C1_BASE;
        inst_info->irq = ESAL_PR_I2C1_EV_INT_VECTOR_ID;
        break;

      case 2: 
        inst_info->io_addr = I2C2_BASE;
        inst_info->irq = ESAL_PR_I2C2_EV_INT_VECTOR_ID;
        break;

      case 3: 
        inst_info->io_addr = I2C3_BASE;
        inst_info->irq = ESAL_PR_I2C3_EV_INT_VECTOR_ID;
        break;
      default:  return  NU_INVALID_OPTIONS;
        
    }    

    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/priority", &temp32);
        if (status == NU_SUCCESS)
        {
            inst_info->irq_priority = temp32;
        }
    }

    return status;

}


/*------------------------------------ EOF ----------------------------------*/
