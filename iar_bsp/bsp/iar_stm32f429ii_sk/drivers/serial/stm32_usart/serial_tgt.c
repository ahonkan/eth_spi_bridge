/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       serial_tgt.c
*
*   COMPONENT
*
*       STM32_USART                           - STM32_USART controller driver
*
*   DESCRIPTION
*
*       This file contains the Serial Driver specific functions.
*
*   FUNCTIONS
*
*       nu_bsp_drvr_serial_stm32_usart_init
*       Serial_Tgt_Rx_Err_Get
*       Serial_Tgt_Ints_Pending
*       Serial_Tgt_Tx_Int_Enable
*       Serial_Tgt_Tx_Int_Clear
*       Serial_Tgt_Rx_Int_Clear
*       Serial_Tgt_Tx_Int_Done
*       Serial_Tgt_Setup
*       Serial_Tgt_Enable
*       Serial_Tgt_Rx_Int_Enable
*       Serial_Tgt_PR_Int_Enable
*       Serial_Tgt_Disable
*       Serial_Tgt_PR_Int_Disable
*       Serial_Tgt_Baud_Rate_Set
*       Serial_Tgt_Tx_Busy
*       Serial_Tgt_Read
*       Serial_Tgt_Write
*       Serial_Tgt_LISR
*
*   DEPENDENCIES
*
*       string.h
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
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
#include "bsp/drivers/cpu/stm32f2x/cpu_tgt.h"
#include "bsp/drivers/serial/stm32_usart/serial_tgt.h"


/* STM Specific target info parser */
static STATUS _Get_Target_Info(const CHAR * key, SERIAL_INSTANCE_HANDLE *inst_info);


/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_serial_stm32_usart_init
*
*   DESCRIPTION
*
*       Serial_TGT entry function
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       Serial_Tgt_Register
*       Serial_Tgt_Unregister
*
*   INPUTS
*
*       CHAR    *key                        - Path to registry
*       INT     startstop                   - Option to Register or Unregister
*
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_bsp_drvr_serial_stm32_usart_init(const CHAR * key, INT startstop)
{
    static DV_DEV_ID            dev_id;
    VOID                        (*setup_fn)(VOID) = NU_NULL;
    VOID                        (*cleanup_fn)(VOID) = NU_NULL;
    SERIAL_TGT_HANDLE           *tgt_handle;
    STATUS                      reg_status;
    STATUS                      status;
    SERIAL_INSTANCE_HANDLE      *inst_handle;
    VOID                        *pointer;
    NU_MEMORY_POOL              *sys_pool_ptr;
    CHAR                        reg_path[REG_MAX_KEY_LENGTH];


    if (key != NU_NULL)
    {
        if (startstop)
        {
            /* Get system memory pool */
            status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

            if (status == NU_SUCCESS)
            {
                /* Allocate a new instance */
                status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                             sizeof(SERIAL_INSTANCE_HANDLE), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Clear memory block */
                    (VOID)memset(pointer, 0, sizeof(SERIAL_INSTANCE_HANDLE));
                    inst_handle = (SERIAL_INSTANCE_HANDLE*)pointer;

                    /* Allocate a new target structure */
                    status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                                 sizeof(SERIAL_TGT_HANDLE), NU_NO_SUSPEND);

                }
                    
                if (status == NU_SUCCESS)
                {
                    /* Clear memory block */
                    (VOID)memset(pointer, 0, sizeof(SERIAL_TGT_HANDLE));
                    tgt_handle = (SERIAL_TGT_HANDLE*)pointer;

                    /* Get target info */
                    status = _Get_Target_Info(key, inst_handle);

                    if(status == NU_SUCCESS)
                    {
                        /********************************/
                        /* COPY REG PATH */
                        /********************************/
                        strncpy(inst_handle->reg_path,key,sizeof(inst_handle->reg_path));

                        /********************************/
                        /* Get Default Config */
                        /********************************/

                        /* Get default config */
                        Serial_Get_Default_Cfg(key, &(inst_handle->attrs));

                        /******************************/
                        /* CALL BOARD SETUP FUNCTION  */
                        /******************************/
                        if (status == NU_SUCCESS)
                        {
                             /* Get setup function */
                            /* If there is a setup function, save it */
                            strncpy(reg_path, key, sizeof(reg_path));
                            strcat(reg_path, "/setup");

                            if (REG_Has_Key(reg_path))
                            {
                                reg_status = REG_Get_UINT32 (reg_path, (UINT32*)&setup_fn);

                                if (reg_status == NU_SUCCESS && setup_fn != NU_NULL)
                                {
                                    tgt_handle->setup_func = setup_fn;
                                }
                            }

                            /* Get cleanup function */
                            /* If there is a cleanup function, save it */
                            strncpy(reg_path, key, sizeof(reg_path));
                            strcat(reg_path, "/cleanup");

                            if (REG_Has_Key(reg_path))
                            {
                                reg_status = REG_Get_UINT32 (reg_path, (UINT32*)&cleanup_fn);

                                if (reg_status == NU_SUCCESS && cleanup_fn != NU_NULL)
                                {
                                    tgt_handle->cleanup_func = cleanup_fn;

                                    /* Call the cleanup function */
                                    cleanup_fn();
                                }
                            }

                            if (status == NU_SUCCESS)
                            {
                                /* Save target specific structure. */
                                inst_handle->serial_reserved = tgt_handle;
                            
                                /* Call the register function */
                                (VOID)Serial_Dv_Register (key, inst_handle);
                            }
                            else
                            {
                                status = SERIAL_NO_INSTANCE_AVAILABLE;
                            }
                        }
                    }
                }
            }

            if (status != NU_SUCCESS)
            {
                (VOID)NU_Deallocate_Memory(pointer);
            }
        }
        else
        {
            Serial_Dv_Unregister (key, startstop, dev_id);
        }
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Rx_Err_Get
*
*   DESCRIPTION
*
*       This function returns the error status of the current
*       received byte
*
*   INPUTS
*
*       base_addr                           - Serial base address
*
*   OUTPUTS
*
*       INT             rx_status           - SD_RX_NO_ERROR
*                                           - SD_RX_OVERRUN_ERROR
*                                           - SD_RX_PARITY_ERROR
*                                           - SD_RX_FRAME_ERROR
*
*************************************************************************/
INT Serial_Tgt_Rx_Err_Get (UINT32 base_addr)
{
    INT         rx_status = SD_RX_NO_ERROR;
    UINT32      temp32;


    /* Get the current value of Receive Status/Error register. */
    temp32 = ESAL_GE_MEM_READ32 ((base_addr + STM32_USART_SR));

    /* Check for RX overrun error */
    if ((temp32 & STM32_USART_SR_ORE) != 0UL)
    {
        rx_status |= SD_RX_OVERRUN_ERROR;
    }

    /* Check for RX parity error */
    if ((temp32 & STM32_USART_SR_PE) != 0UL)
    {
        rx_status |= SD_RX_PARITY_ERROR;
    }

    /* Check for RX frame error */
    if ((temp32 & STM32_USART_SR_FE) != 0UL)
    {
        rx_status |= SD_RX_FRAME_ERROR;
    }

    /* Return the rx status */
    return (rx_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Ints_Pending
*
*   DESCRIPTION
*
*       This function returns what interrupts are pending
*
*   INPUTS
*
*        SERIAL_INSTANCE_HANDLE *inst_handle - Device instance handle
*
*   OUTPUTS
*
*       INT             int_status          - SD_NO_INTERRUPT
*                                           - SD_TX_INTERRUPT
*                                           - SD_RX_INTERRUPT
*
*************************************************************************/
INT Serial_Tgt_Ints_Pending (UINT32 base_addr)
{
    INT                   int_status = SD_NO_INTERRUPT;
    UINT32                uart_status;


    /* Get the interrupt status for unmasked interrupts only */
    uart_status = ESAL_GE_MEM_READ32 ((base_addr + STM32_USART_SR));

    /* Check if Read Data register is not empty */
    if (((uart_status & STM32_USART_SR_RXNE) != 0UL))
    {
        /* Set bit for RX interrupt pending */
        int_status |= SD_RX_INTERRUPT;
    }

    /* Check if TX interrupt */
    if ((uart_status & STM32_USART_SR_TXE))
    {
        /* Set bit for TX interrupt pending */
        int_status |= SD_TX_INTERRUPT;
    }

    /* Return the interrupt status */
    return (int_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Tx_Int_Enable
*
*   DESCRIPTION
*
*       This function enables the UART TX interrupt
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *inst_handle  - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Tx_Int_Enable (SERIAL_INSTANCE_HANDLE *inst_handle)
{
    UINT32  base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_handle);
    UINT32  reg_val;
    INT     int_level;


    /* Disable interrupts before setting shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get the current value of the STM32_USART Control register 1 */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);

    /* Set bit to enable the TX interrupt */
    reg_val |= STM32_USART_CR1_TXEIE;

    /* Write new value to the STM32_USART Control register 1 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

    /* Set flag to show TX interrupt enabled */
    inst_handle->tx_intr_en_shadow = NU_TRUE;

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Tx_Int_Clear
*
*   DESCRIPTION
*
*       This function clears the UART TX interrupt.
*
*   INPUTS
*
*       base_addr                       - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Tx_Int_Clear (UINT32 base_addr)
{
    UINT32  reg_val;

    /* Get the current value of the STM32_USART Control register 1 */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);

    /* Clear bit to disable the TX interrupt */
    reg_val &= ~STM32_USART_CR1_TXEIE;

    /* Write new value to the STM32_USART Control register 1 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Rx_Int_Clear
*
*   DESCRIPTION
*
*       This function clears the UART RX interrupt.
*
*   INPUTS
*
*       base_addr                       - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Rx_Int_Clear (UINT32 base_addr)
{
    UINT32  reg_val;

    /* Get the current value of the STM32_USART Control register 1 */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);

    /* Clear bit to disable the RX interrupt */
    reg_val &= ~STM32_USART_CR1_RXNEIE;

    /* Write new value to the STM32_USART Control register 1 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Tx_Int_Done
*
*   DESCRIPTION
*
*       This function performs necessary actions when TX interrupt is
*       complete
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *instance_handle  - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Tx_Int_Done (SERIAL_INSTANCE_HANDLE *instance_handle)
{
    INT       int_level;
    UINT32    reg_val;
    UINT32    base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_handle);

    /* Disable interrupts before setting shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Clear flag showing TX interrupt is disabled */
    instance_handle->tx_intr_en_shadow = NU_FALSE;

    /* Disable the transmitter */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
    reg_val &=  ~STM32_USART_CR1_TE;
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

    /* Write the Data register to suppress TXE bit in Status register */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_DR), 0);

    /* Disable transmitter */
    instance_handle->tx_en_shadow = NU_FALSE;

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Setup
*
*   DESCRIPTION
*
*       This function sets up the UART hardware
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE   *inst_handle    - Device instance handle
*       SERIAL_ATTR              *uart_ptr       - Pointer to UART config structure
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Setup (SERIAL_INSTANCE_HANDLE *inst_handle, SERIAL_ATTR *uart_ptr)
{
    UINT32                 cr1 = 0;
    UINT32                 cr2 = 0;
    UINT32                 base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_handle);
    SERIAL_TGT_HANDLE      *tgt_handle = (SERIAL_TGT_HANDLE *) inst_handle->serial_reserved;
    VOID                   (*setup_fn)(VOID) = NU_NULL;

    /* Call setup function if available. */
    setup_fn = tgt_handle->setup_func;
    if(setup_fn != NU_NULL)
    {
        /* Call the setup function */
        setup_fn();
    }

    /* Read the USART Control register 1 and 2 */
    cr1 = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
    cr2 = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR2);

    /* Disable USART so we can change the registers. */
    cr1 &= ~STM32_USART_CR1_UE;
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), cr1);

    /* Set Data length */
    if (uart_ptr->data_bits == DATA_BITS_8)
    {
        /* Set bits for 8 data bits */
        cr1 = STM32_USART_CR1_M_DATA_8;
    }
    else
    {
        /* Set bits for 9 data bits */
        cr1 = STM32_USART_CR1_M_DATA_9;
    }

    /* Set parity value */
    if (uart_ptr->parity == PARITY_ODD)
    {
        /* Set bits for odd parity */
        cr1 |= (STM32_USART_CR1_PCE | STM32_USART_CR1_PS_ODD);
    }
    else if (uart_ptr->parity == PARITY_EVEN)
    {
        /* Set bits for even parity */
        cr1 |= (STM32_USART_CR1_PCE | STM32_USART_CR1_PS_EVEN);
    }
    else
    {
        /* Set bits for no parity */
        cr1 &= ~STM32_USART_CR1_PCE;
    }

    /* Set stop bit */
    if (uart_ptr->stop_bits == STOP_BITS_2)
    {
        /* Set bits for 2 stop bits */
        cr2 |= STM32_USART_CR2_STOP_2;
    }
    else
    {
        /* Set bits for 1 stop bits */
        cr2 |= STM32_USART_CR2_STOP_1;
    }

    /* Write Control register 1 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), cr1);

    /* Write Control register 2 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR2), cr2);

    /* Write the Data register to suppress TXE bit in Status register */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_DR), 0);

    /* Set baud rate */
    Serial_Tgt_Baud_Rate_Set (inst_handle, uart_ptr->baud_rate);

    /*  Wait until previous transmission is completed. */
    /*  After writing the last data into the USART_DR register, wait until TC = 1. 
        This indicates that the transmission of the last frame is complete. 
        This is required for instance when the USART is disabled or enters the Halt mode 
        to avoid corrupting the last transmission. */
    while (!(ESAL_GE_MEM_READ32 (base_addr + STM32_USART_SR) & STM32_USART_SR_TC));
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Enable
*
*   DESCRIPTION
*
*       This function enables the UART hardware
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *inst_handle  - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Enable (SERIAL_INSTANCE_HANDLE *instance_handle)
{
    INT         int_level;
    UINT32      reg_val;
    UINT32      base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_handle);


    /* Disable interrupts before setting shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Enable the usart, receiver and transmitter */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
    reg_val |= (STM32_USART_CR1_UE | STM32_USART_CR1_TE | STM32_USART_CR1_RE);
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

    /* Enable transmitter */
    instance_handle->tx_en_shadow = NU_TRUE;

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Rx_Int_Enable
*
*   DESCRIPTION
*
*       This function enables the UART RX interrupt
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *inst_handle  - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Rx_Int_Enable (SERIAL_INSTANCE_HANDLE *inst_handle)
{
    UINT32  reg_val;
    UINT32  base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_handle);

    /* Get the current value of the STM32_USART Control register 1 */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);

    /* Set bit to enable the RX interrupt */
    reg_val |= STM32_USART_CR1_RXNEIE;

    /* Write new value to the STM32_USART Control register 1 */
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);
}

/*************************************************************************
*
* FUNCTION
*
*       Serial_Tgt_PR_Int_Enable
*
* DESCRIPTION
*
*       This function initializes processor-level TX interrupt
*
* INPUTS
*
*       VOID           *sess_handle         - Device session handle
*
* OUTPUTS
*
*       STATUS         status               - Status resulting from
*                                             NU_Register_LISR calls.
*
*************************************************************************/
STATUS Serial_Tgt_PR_Int_Enable(SERIAL_SESSION_HANDLE *session_handle)
{
    SERIAL_INSTANCE_HANDLE *instance_handle = (session_handle->instance_ptr);
    STATUS                 status = NU_SUCCESS;
    VOID                   (*old_lisr)(INT);

    /* Check if TX interrupt needs to be registered and enabled */
    if ((instance_handle->attrs.tx_mode) == USE_IRQ)
    {
        /* Register UART handler for the TX interrupt */
        status = NU_Register_LISR (instance_handle->serial_tx_vector,
                                   &Serial_Tgt_LISR, &old_lisr);

        /* Check if registration successful */
        if (status == NU_SUCCESS)
        {
            /* Enable the UART TX interrupt */
            (VOID) ESAL_GE_INT_Enable (instance_handle->serial_tx_vector,
                                       instance_handle->serial_tx_irq_type,
                                       instance_handle->serial_tx_irq_priority);

            /* Register the UART data structure with this vector id */
            ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_tx_vector, session_handle);
        }

    }

    /* Check if RX interrupt needs to be registered and enabled */
    if ((instance_handle->attrs.rx_mode) == USE_IRQ)
    {
        /* Register RX UART vector */
        status = NU_Register_LISR(instance_handle->serial_rx_vector,
                                  &Serial_Tgt_LISR, &old_lisr);

        /* Check if registration successful */
        if (status == NU_SUCCESS)
        {

            /* Enable the UART RX interrupt */
            (VOID) ESAL_GE_INT_Enable (instance_handle->serial_rx_vector,
                                       instance_handle->serial_rx_irq_type,
                                       instance_handle->serial_rx_irq_priority);

            /* Register the UART data structure with this vector id */
            ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_rx_vector, session_handle);
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Disable
*
*   DESCRIPTION
*
*       This function places disables Serial_TGT
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *inst_handle  - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Disable (SERIAL_INSTANCE_HANDLE *instance_ptr)
{
    INT                int_level;
    UINT32             temp32;
    UINT32             base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_ptr);
    SERIAL_TGT_HANDLE  *tgt_handle = (SERIAL_TGT_HANDLE *) instance_ptr->serial_reserved;
    VOID               (*cleanup_fn)(VOID) = NU_NULL;

    /* Disable interrupts before setting shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Disable the receiver and transmitter */
    temp32 = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
    temp32 &= ~(STM32_USART_CR1_UE | STM32_USART_CR1_TE | STM32_USART_CR1_RE);
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), temp32);

    /* Disable transmitter */
    instance_ptr->tx_en_shadow = NU_FALSE;

    /* Call cleanup function if available. */
    cleanup_fn = tgt_handle->cleanup_func;
    if(cleanup_fn != NU_NULL)
    {
        /* Call the cleanup function */
        cleanup_fn();
    }

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);
}

/*************************************************************************
*
* FUNCTION
*
*       Serial_Tgt_PR_Int_Disable
*
* DESCRIPTION
*
*       This function initializes processor-level TX interrupt
*
* INPUTS
*
*       VOID           *sess_handle         - Device session handle
*
* OUTPUTS
*
*       STATUS         status               - Status resulting from
*                                             NU_Register_LISR calls.
*
*************************************************************************/
STATUS Serial_Tgt_PR_Int_Disable(SERIAL_INSTANCE_HANDLE *stm32_usart_inst_ptr)
{
    STATUS                status = NU_SUCCESS;
    VOID                  (*old_lisr)(INT);

    /* Disable TX interrupts */
    if (stm32_usart_inst_ptr->serial_tx_vector != ESAL_DP_INT_VECTOR_ID_DELIMITER)
    {
        /* Disable the UART TX interrupt */
        (VOID) ESAL_GE_INT_Disable (stm32_usart_inst_ptr->serial_tx_vector);

        /* Register the UART data structure with this vector id */
        ESAL_GE_ISR_VECTOR_DATA_SET (stm32_usart_inst_ptr->serial_tx_vector, NU_NULL);

        /* Unregister TX UART LISR */
        status = NU_Register_LISR (stm32_usart_inst_ptr->serial_tx_vector, NU_NULL, &old_lisr);
    }

    /* Disable RX interrupts */
    if (stm32_usart_inst_ptr->serial_rx_vector != ESAL_DP_INT_VECTOR_ID_DELIMITER)
    {
        /* Disable the UART TX interrupt */
        (VOID) ESAL_GE_INT_Disable (stm32_usart_inst_ptr->serial_rx_vector);

        /* Register the UART data structure with this vector id */
        ESAL_GE_ISR_VECTOR_DATA_SET (stm32_usart_inst_ptr->serial_rx_vector, NU_NULL);

        /* Unregister RX UART LISR */
        status = NU_Register_LISR (stm32_usart_inst_ptr->serial_rx_vector, NU_NULL, &old_lisr);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Baud_Rate_Set
*
*   DESCRIPTION
*
*       This function sets the UART baud rate.
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE  *inst_handle - Device instance handle
*       UINT32                 baud_rate    - The new baud rate.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATUS Serial_Tgt_Baud_Rate_Set (SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate)
{
    UINT8            op_id;
    UINT32           usartdiv_man;
    UINT32           usartdiv_fra;
    UINT32           usartdiv;
    UINT32           temp1;
    UINT32           temp2;
    STATUS           status = NU_SUCCESS;
    UINT32           base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_handle);
    UINT32           stm32_usart_clock;

    /* Try to get the current OP ID. This will return success only
       if the DVFS driver is present and initialized */
    status = CPU_Get_Device_Frequency(&op_id, &(inst_handle->serial_ref_clock[0]), &stm32_usart_clock);

    /* Calculate baud_div. */
    /* Calculate Integer part. */
    temp1 = ((25 * stm32_usart_clock) / (4 * baud_rate));
    usartdiv_man = ((temp1 / 100) << 4);

    /* Calculate Fractional part. */
    temp2 = temp1 - ((usartdiv_man >> 4) * 100);
    usartdiv_fra = ((((temp2 * 16) + 50) / 100) & 0xF);

    /* Calculate baudrate divisor. */
    usartdiv = (usartdiv_man | usartdiv_fra);

    if(status == NU_SUCCESS)
    {
        /* Set usartdiv in BRR. */
        ESAL_GE_MEM_WRITE32 (base_addr + STM32_USART_BRR, usartdiv);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Tx_Busy
*
*   DESCRIPTION
*
*       This function returns NU_TRUE if busy
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE  *inst_handle - Device instance handle
*
*   OUTPUTS
*
*       INT                     busy         - NU_TRUE if busy
*
*************************************************************************/
INT Serial_Tgt_Tx_Busy (SERIAL_INSTANCE_HANDLE *instance_ptr)
{
    INT         busy;
    UINT32      base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_ptr);

    /* Read the Status register */
    busy = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_SR) & STM32_USART_SR_TXE;

    /* Return status of TX */
    return (!busy);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Read
*
*   DESCRIPTION
*
*       This function reads a byte from the UART
*
*   INPUTS
*
*       VOID     *session_handle            - Session handle
*       VOID     *buffer                    - Buffer to copy data into
*       UINT32   numbyte                    - Size of buffer
*       OFFSET_T byte_offset                - offset from buffer start
*       UINT32   *bytes_read                - Return number of bytes read
*
*   OUTPUTS
*
*       INT      status                     - Size of data copied into buffer
*
*************************************************************************/
STATUS Serial_Tgt_Read(VOID* session_handle, VOID *buffer, UINT32 numbyte,
                      OFFSET_T byte_offset, UINT32 *bytes_read)
{
    SERIAL_SESSION_HANDLE   *session_ptr = ((SERIAL_SESSION_HANDLE*)session_handle);
    SERIAL_INSTANCE_HANDLE  *inst_ptr = session_ptr->instance_ptr;
    STATUS                  status = NU_SUCCESS;
    UINT32                  base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_ptr);
    UINT32                  reg_val;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE pmi_dev = (inst_ptr->pmi_dev);
    PMI_WAIT_CYCLE(pmi_dev, status);
#endif

    /* Device state is now ON, so we can read */
    if (status == NU_SUCCESS)
    {
        /* Read the usart status register */
        reg_val = ESAL_GE_MEM_READ32 ((base_addr + STM32_USART_SR));

        /* If a character has been received, read it */
        if ((reg_val & STM32_USART_SR_RXNE) == STM32_USART_SR_RXNE)
        {
            /* Read a character */
            *(UINT8*)buffer = STM32_USART_READ(base_addr);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
            (VOID)PMI_Reset_Watchdog(pmi_dev);
#endif
            /* Read 1 byte */
            *bytes_read = 1;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Write
*
*   DESCRIPTION
*
*       This function writes a byte to the UART
*
*   INPUTS
*
*       VOID     *session_handle            - Session handle
*       VOID     *buffer                    - Buffer to copy data into
*       UINT32   numbyte                    - Size of buffer
*       OFFSET_T byte_offset                - offset from buffer start
*       UINT32   *bytes_written             - Return number of bytes written
*
*   OUTPUTS
*
*       INT      status                     - Size of data written to device
*
*************************************************************************/
STATUS Serial_Tgt_Write(VOID* session_handle, const VOID *buffer, UINT32 numbyte,
                       OFFSET_T byte_offset, UINT32 *bytes_written)
{
    SERIAL_SESSION_HANDLE   *session_ptr = ((SERIAL_SESSION_HANDLE*)session_handle);
    SERIAL_INSTANCE_HANDLE  *inst_ptr = session_ptr->instance_ptr;
    STATUS                  status = NU_SUCCESS;
    UINT32                  base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_ptr);
    UINT32                  reg_val;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE  pmi_dev = inst_ptr->pmi_dev;
    PMI_WAIT_CYCLE(pmi_dev, status);
#endif

    /* Device state is now ON, so we can write */
    if (status == NU_SUCCESS)
    {
        /* Enable the transmitter */
        reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
        reg_val |= (STM32_USART_CR1_TE);
        ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

        /* Transmit the character */
        STM32_USART_WRITE(base_addr, *(UINT8*)buffer);

        *bytes_written = 1;

        /* Check if working in interrupt mode. */
        if((inst_ptr->attrs.tx_mode) == USE_IRQ)
        {
            /* Enable TX Interrupts */
            Serial_Tgt_Tx_Int_Enable (inst_ptr);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       Serial_Tgt_LISR
*
* DESCRIPTION
*
*       This is the entry function for the ISR that services the UART.
*
* INPUTS
*
*       INT      vector                     - Interrupt vector.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_LISR (INT vector)
{
    SERIAL_SESSION       *port;
    INT                  ints_pending;
    INT                  rx_byte_status;
    UINT8                ch;
    SERIAL_SESSION_HANDLE *stm32_usart_ses_ptr;
    SERIAL_INSTANCE_HANDLE *instance_ptr;
    UINT32               base_addr;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

    BOOLEAN activate_hisr = NU_FALSE;

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

    /* Get a pointer to the UART structure for this vector */
    stm32_usart_ses_ptr = (SERIAL_SESSION_HANDLE*) ESAL_GE_ISR_VECTOR_DATA_GET (vector);
    instance_ptr = stm32_usart_ses_ptr->instance_ptr;
    base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_ptr);
    port = stm32_usart_ses_ptr->ser_mw_ptr;

    /* Get interrupt status */
    ints_pending = Serial_Tgt_Ints_Pending(base_addr);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

    /* Check if device is opened in UII mode. */
    if (instance_ptr->open_modes & UII_OPEN_MODE)
    {
        activate_hisr = NU_TRUE;
    }

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

    /* Loop while interrupts are pending */
    while (ints_pending != SD_NO_INTERRUPT)
    {
        /* Check if RX interrupts are pending */
        if ((ints_pending & SD_RX_INTERRUPT) == SD_RX_INTERRUPT)
        {
            /* Clear this RX interrupt */
            Serial_Tgt_Rx_Int_Clear(base_addr);

            /* Read the character from the hardware RX buffer */
            ch = STM32_USART_READ(base_addr);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

            /* Check if we need to reset watchdog. */
            if(activate_hisr == NU_TRUE)
            {
                /* Activate the Receive HISR. */
                NU_Activate_HISR(&(instance_ptr->rx_hisr));

                activate_hisr = NU_FALSE;
            }

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

            /* Get RX status for this byte */
            rx_byte_status = Serial_Tgt_Rx_Err_Get(base_addr);

            /* Check if discard errors in RX character (parity / framing errors) */
            if (rx_byte_status >= SD_RX_PARITY_ERROR)
            {
                /* Increase error status if bad data */
                port->parity_errors += (rx_byte_status == SD_RX_PARITY_ERROR);
                port->frame_errors += (rx_byte_status == SD_RX_FRAME_ERROR);
            }

            else
            {
                /* Update the overrun errors */
                port->overrun_errors += (rx_byte_status == SD_RX_OVERRUN_ERROR);

                /* Ensure the RX buffer is not full */
                if (port->rx_buffer_status != NU_BUFFER_FULL)
                {
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

                    /* If blocking mode and if the buffer is empty */
                    if((port->read_mode != NU_NO_SUSPEND) && (port->rx_buffer_status == NU_BUFFER_EMPTY))
                    {
                        Serial_Set_ISR_Data(port, SD_RX_INTERRUPT);
                    }

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

                    /* Put the character into the RX buffer */
                    port->rx_buffer[port->rx_buffer_write] = ch;

                    /* Move the write pointer */
                    port->rx_buffer_write++;

                    /* Check if the RX write pointer needs to wrap */
                    if (port->rx_buffer_write == port->sd_buffer_size)
                    {
                        /* Wrap the RX write pointer */
                        port->rx_buffer_write = 0UL;
                    }

                    /* Check to see if the buffer is full now */
                    if (port->rx_buffer_write == port->rx_buffer_read)
                    {
                        /* Set RX buffer status to show full */
                        port->rx_buffer_status = NU_BUFFER_FULL;
                    }

                    else
                    {
                        /* Otherwise, set buffer status to show data is in buffer */
                        port->rx_buffer_status = NU_BUFFER_DATA;
                    }
                }

                else
                {
                    /* Buffer is full and more data is available - data lost. */
                    port->busy_errors++;
                }
            }

            /* Enable RX interrupt */
            Serial_Tgt_Rx_Int_Enable(instance_ptr);

        }   /* if RX interrupt */

        /* Check if TX interrupts are pending */
        if ((ints_pending & SD_TX_INTERRUPT) == SD_TX_INTERRUPT)
        {
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

            /* If blocking mode and if the buffer is full */
            if((port->write_mode != NU_NO_SUSPEND) && (port->tx_buffer_status == NU_BUFFER_FULL))
            {
                /* Pass data to HISR */
                Serial_Set_ISR_Data(port, SD_TX_INTERRUPT);
            }

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

            /* Clear this TX interrupt */
            Serial_Tgt_Tx_Int_Clear(base_addr);

            /* Bump the read pointer past the byte that was just
               transmitted. */
            port->tx_buffer_read++;

            /* Check if the TX read pointer needs to wrap */
            if (port->tx_buffer_read == port->sd_buffer_size)
            {
                /* Wrap the TX read pointer */
                port->tx_buffer_read = 0UL;
            }

            /* Check if the TX buffer is empty */
            if (port->tx_buffer_write == port->tx_buffer_read)
            {
                /* Set the TX buffer status to show empty */
                port->tx_buffer_status = NU_BUFFER_EMPTY;

                /* After writing the last data into the USART_DR register, wait until TC = 1. 
                This indicates that the transmission of the last frame is complete. 
                This is required for instance when the USART is disabled or enters the Halt mode 
                to avoid corrupting the last transmission. */
                while (!(ESAL_GE_MEM_READ32 (base_addr + STM32_USART_SR) & STM32_USART_SR_TC));

                /* Perform any hardware necessary tasks when the software buffer is empty. */
                /* Disable the transmitter */
                Serial_Tgt_Tx_Int_Done(instance_ptr);
            }

            else
            {
                /* Transmit the next character in the buffer */
                STM32_USART_WRITE(base_addr, port->tx_buffer[port->tx_buffer_read]);

                /* Update the status of the TX buffer. */
                port->tx_buffer_status = NU_BUFFER_DATA;
            }

            /* Enable TX interrupt */
            Serial_Tgt_Tx_Int_Enable(instance_ptr);

        }   /* if TX interrupt */

        /* Get interrupt status */
        ints_pending = Serial_Tgt_Ints_Pending(base_addr);

    }   /* while interrupts pending */
}




/*************************************************************************
*
* FUNCTION
*
*       _Get_Target_Info
*
* DESCRIPTION
*
*       This local function retrieves serial port target parameters and 
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
static STATUS _Get_Target_Info(const CHAR * key, SERIAL_INSTANCE_HANDLE *inst_info)
{
    STATUS     status;
    UINT32     temp32;
    UINT8      op_id;
    UINT32     ref_freq;


    /* Process Serial Device and set the io address, vector and peripheral clock source 
     * ser_dev:        [1,2,3,4,5,6,7,8] 
     */
    status = REG_Get_UINT32_Value(key, "/tgt_settings/ser_dev", (UINT32*)&temp32);
    
    if (status != NU_SUCCESS)
      return  SERIAL_REGISTRY_ERROR;


    inst_info->serial_tx_irq_type = ESAL_TRIG_LEVEL_LOW;
    inst_info->serial_rx_irq_type = ESAL_TRIG_LEVEL_LOW;
    
    switch(temp32)
    {
      case 1: 
        inst_info->serial_io_addr = USART1_BASE;
        inst_info->serial_tx_vector = ESAL_PR_USART1_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_USART1_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB2CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 2: 
        inst_info->serial_io_addr = USART2_BASE;
        inst_info->serial_tx_vector = ESAL_PR_USART2_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_USART2_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 3: 
        inst_info->serial_io_addr = USART3_BASE;
        inst_info->serial_tx_vector = ESAL_PR_USART3_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_USART3_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 4: 
        inst_info->serial_io_addr = UART4_BASE;
        inst_info->serial_tx_vector = ESAL_PR_UART4_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_UART4_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 5: 
        inst_info->serial_io_addr = UART5_BASE;
        inst_info->serial_tx_vector = ESAL_PR_UART5_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_UART5_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;
        
      case 6: 
        inst_info->serial_io_addr = USART6_BASE;
        inst_info->serial_tx_vector = ESAL_PR_USART6_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_USART6_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB2CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 7: 
        inst_info->serial_io_addr = UART7_BASE;
        inst_info->serial_tx_vector = ESAL_PR_UART7_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_UART7_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;

      case 8: 
        inst_info->serial_io_addr = UART8_BASE;
        inst_info->serial_tx_vector = ESAL_PR_UART8_INT_VECTOR_ID;
        inst_info->serial_rx_vector = ESAL_PR_UART8_INT_VECTOR_ID;
        strncpy(inst_info->serial_ref_clock, CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
        break;
        
      default:  return  NU_INVALID_OPTIONS;
        
    }    

    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value(key, "/tgt_settings/tx_intr_priority", &temp32);
        if (status == NU_SUCCESS)
        {
            inst_info->serial_tx_irq_priority = temp32;
        }
    }

    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value(key, "/tgt_settings/rx_intr_priority", &temp32);
        if (status == NU_SUCCESS)
        {
            inst_info->serial_rx_irq_priority = temp32;
        }
    }
    
    /* Get reference clock */
    status = CPU_Get_Device_Frequency(&op_id, inst_info->serial_ref_clock, &ref_freq);

    /* Check if API call was successful. */
    if (status == NU_SUCCESS)
    {
        inst_info->serial_clock =  ref_freq;
    }
    else
    {
        inst_info->serial_clock =  45e6;
    }

    return status;

}

/*---------------------------------- EOF ------------------------------------*/
