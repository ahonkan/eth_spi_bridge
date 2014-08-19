/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       lwspi_tgt.c
*
* COMPONENT
*
*       Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains the SPI Driver specific functions.
*
* FUNCTIONS
*
*       nu_bsp_drvr_lwspi_lm3s_ssi_init
*       LWSPI_TGT_Initialize
*       LWSPI_TGT_Shutdown
*       LWSPI_TGT_Configure
*       LWSPI_TGT_Read
*       LWSPI_TGT_Write
*       LWSPI_TGT_Write_Read
*       LWSPI_TGT_Device_Enable
*       LWSPI_TGT_Device_Disable
*       LWSPI_TGT_Set_Baud_Rate
*       LWSPI_TGT_Intr_Enable
*       LWSPI_TGT_Intr_Disable
*       LWSPI_TGT_LISR
*       LWSPI_TGT_ISR_Read
*       LWSPI_TGT_ISR_Write
*       LWSPI_TGT_ISR_Write_Read
*       LWSPI_TGT_Check_Device_Busy
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       lwspi_tgt.h
*
**************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"
#include "services/nu_services.h"
#include "connectivity/lwspi.h"
#include "bsp/drivers/lwspi/stm32/lwspi_tgt.h"
#include "bsp/drivers/lwspi/stm32/lwspi_tgt_power.h"
#include "bsp/drivers/cpu/stm32f2x/cpu_tgt.h"

#include "drivers/dma.h"
#include "drivers/dma_common.h"
#include "bsp/drivers/dma/dma_tgt.h"
#include <string.h>


#define LWSPI_TGT_LISR_ENTRY            //DEBUG_F6_SET
#define LWSPI_TGT_LISR_EXIT             //DEBUG_F6_CLEAR
#define LWSPI_TGT_Read_ENTRY  
#define LWSPI_TGT_Read_EXIT  
#define LWSPI_TGT_Write_ENTRY           //DEBUG_F7_SET
#define LWSPI_TGT_Write_EXIT            //DEBUG_F7_CLEAR
#define LWSPI_TGT_Write_Read_ENTRY  
#define LWSPI_TGT_Write_Read_EXIT  
#define LWSPI_TGT_ISR_Read_ENTRY  
#define LWSPI_TGT_ISR_Read_EXIT  
#define LWSPI_TGT_ISR_Write_ENTRY       //DEBUG_F7_SET
#define LWSPI_TGT_ISR_Write_EXIT        //DEBUG_F7_CLEAR
#define LWSPI_TGT_ISR_Write_Read_ENTRY  
#define LWSPI_TGT_ISR_Write_Read_EXIT  


/*********************/
/* LOCAL FUNCTIONS   */
/*********************/

static  STATUS  LWSPI_TGT_Get_STM_SPI_Device_Info(const CHAR *key, LWSPI_INSTANCE_HANDLE *spi_inst_ptr);

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_lwspi_stm32_init
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the device
*       and calls the device driver registration function.
*       This function assumes START and STOP are called
*       alternatively for a device.
*
*   CALLED BY
*
*       System Registry
*
*   INPUTS
*
*       key                                 Path to registry.
*       compctrl                            A flag specifying desired
*                                           operation on component.
*                                           Its value will be one of
*                                           these.
*                                            - RUNLEVEL_STOP
*                                            - RUNLEVEL_START
*                                            - RUNLEVEL_HIBERNATE
*                                            - RUNLEVEL_RESUME
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
/* ah terabit radios:  Made provision in the spi device driver platform 
 * specification to specify specific spi device rather than parameters.
 * This way the user does not have to dig through manuals to set 
 * parameters.
 */
VOID nu_bsp_drvr_lwspi_stm32_init(const CHAR *key, INT compctrl)
{
    NU_MEMORY_POOL        *sys_pool_ptr;
    LWSPI_INSTANCE_HANDLE *spi_inst_ptr;
    LWSPI_TGT             *tgt_ptr = NU_NULL;
    static DV_DEV_ID       dev_id;
    STATUS                 status;

    if(compctrl == RUNLEVEL_START)
    {
        /* Get system memory pool. */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
        if (status == NU_SUCCESS)
        {
            /* Allocate a new instance. */
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID **)&spi_inst_ptr,
                                         sizeof(LWSPI_INSTANCE_HANDLE), NU_NO_SUSPEND);
        }
        
        if (status == NU_SUCCESS)
        {
            /* Clear instance handle. */
            memset (spi_inst_ptr, 0, sizeof(LWSPI_INSTANCE_HANDLE));

            /* Allocate a new target driver control block. */
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID **)&tgt_ptr,
                                         sizeof(LWSPI_TGT), NU_NO_SUSPEND);
        }
         
        if (status == NU_SUCCESS)
        {
            /* Clear recently allocated memory. */
            memset (tgt_ptr, 0, sizeof(LWSPI_TGT));
    
            /* Save target driver pointer. */
            spi_inst_ptr->spi_tgt_ptr = tgt_ptr;

            /* Set SPI registry path. */
            status = LWSPI_TGT_Get_STM_SPI_Device_Info(key, spi_inst_ptr);
            
        }    
                
        if (status == NU_SUCCESS)
        {
            /* Set SPI registry path. */
            status = LWSPI_Set_Reg_Path(key, spi_inst_ptr);
        }

        /* If everything is OK so far then call SPI generic layer register function. */
        if (status == NU_SUCCESS)
        {
            status = LWSPI_Dv_Register (key, spi_inst_ptr);
        }

        /* In case of error de-allocate memory. */
        if(status != NU_SUCCESS)
        {
            /* De-allocate instance handle memory. */
            NU_Deallocate_Memory(spi_inst_ptr);

            /* De-allocate tgt handle memory, if allocated before. */
            if(tgt_ptr)
            {
                /* De-allocate instance handle memory. */
                NU_Deallocate_Memory(tgt_ptr);
            }
        }
    }
    else if (compctrl == RUNLEVEL_STOP)
    {
        LWSPI_Dv_Unregister (key, compctrl, dev_id);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Initialize
*
*   DESCRIPTION
*
*       This function initializes the SPI controller.
*
*   INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
VOID LWSPI_TGT_Initialize(LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    UINT8      *spi_base_address;
    LWSPI_TGT  *spi_tgt_ptr;
    VOID       (*setup_fn)(VOID);
    UINT16      temp16;

    /* Initialize local variables. */
    spi_base_address    = (UINT8 *)spi_inst_ptr->io_addr;
    spi_tgt_ptr         = (LWSPI_TGT *)spi_inst_ptr->spi_tgt_ptr;

    /* Call the target specific setup function, if any, to enable register access. */
    setup_fn = spi_tgt_ptr->setup_func;
    if(setup_fn != NU_NULL)
    {
        setup_fn();
    }

    /* Select SPI mode in I2SCFGR register. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_I2SCFGR);
    temp16 &= ~SPI_I2SCFGR_I2SMOD;
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_I2SCFGR), temp16);

    /* Set SPI in master mode. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR1);
    temp16 |= SPI_CR1_MSTR;
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR1), temp16);

    /* Enable SSOE in SPI CR2 register to setup a single master configuration. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR2);
    temp16 |= SPI_CR2_SSOE;
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR2), temp16);

}

/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Shutdown
*
*   DESCRIPTION
*
*       This function closes the SPI controller.
*
*   INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID LWSPI_TGT_Shutdown(LWSPI_INSTANCE_HANDLE* spi_inst_ptr)
{
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Configure
*
* DESCRIPTION
*
*       This function configures the SPI bus as required by a particular
*       slave device.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       device                              Device pointer which contains
*                                           SPI bus parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Configure(VOID *spi_inst_ptr, NU_SPI_DEVICE *device)
{
    LWSPI_INSTANCE_HANDLE *spi_dev_inst_ptr;
    STATUS                status;
    UINT8                 *spi_base_address;
    UINT16                temp16;

    /* Initialize local variables. */
    spi_dev_inst_ptr = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    status           = NU_SUCCESS;
    spi_base_address = (UINT8 *) spi_dev_inst_ptr->io_addr;

    /* Disable the device to re-programme control registers. */
    LWSPI_TGT_Device_Disable(spi_dev_inst_ptr);
    
    /* Set baud rate. */
    status = LWSPI_TGT_Set_Baud_Rate(spi_dev_inst_ptr, device->baud_rate);

    /*----------------------- SPI CR1 Register Settings ---------------------*/
    
    /* Read configuration register */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR1);
    
    /* Set transfer size. */
    temp16 &= ~(SPI_DATASIZE_8BIT | SPI_DATASIZE_16BIT);

    if (device->transfer_size == 8)   	  temp16 |= SPI_DATASIZE_8BIT;
    else if (device->transfer_size == 16) temp16 |= SPI_DATASIZE_16BIT;
    else return LWSPI_UNSUPPORTED_TRANSFER_SIZE;

    /* Set SPI Clock Polarity */
    temp16 &= ~(SPI_POLARITY_LOW | SPI_POLARITY_HIGH);

    if (device->spi_config & SPI_CFG_MODE_POL_LO)       temp16 |= SPI_POLARITY_LOW;
    else if (device->spi_config & SPI_CFG_MODE_POL_HI)  temp16 |= SPI_POLARITY_HIGH;
    else return LWSPI_UNSUPPORTED_POLARITY;

    /* Set SPI Clock Phase */
    temp16 &= ~(SPI_PHASE_1EDGE | SPI_PHASE_2EDGE);

    if (device->spi_config & SPI_CFG_MODE_PHA_FIRST_EDGE)       temp16 |= SPI_PHASE_1EDGE;
    else if (device->spi_config & SPI_CFG_MODE_PHA_SECOND_EDGE) temp16 |= SPI_PHASE_2EDGE;
    else return LWSPI_UNSUPPORTED_PHASE;

    /* Set bit order (MSB/LSB). */
    temp16 &= ~(SPI_FIRSTBIT_MSB | SPI_FIRSTBIT_LSB);

    if (device->spi_config & SPI_CFG_BO_LSB_FIRST)      temp16 |= SPI_FIRSTBIT_LSB;
    else if (device->spi_config & SPI_CFG_BO_MSB_FIRST) temp16 |= SPI_FIRSTBIT_MSB;
    else return LWSPI_UNSUPPORTED_BIT_ORDER;

    /* Set SPI Master/Slave */
    temp16 &= ~(SPI_MODE_MASTER | SPI_MODE_SLAVE);

    if (device->spi_config & SPI_CFG_DEV_MASTER)        temp16 |= SPI_MODE_MASTER;
    else if (device->spi_config & SPI_CFG_DEV_SLAVE)    temp16 |= SPI_MODE_SLAVE;
    else return LWSPI_UNSUPPORTED_BIT_ORDER;

    /* Set SPI Chip Select /NSS HW/SW control */
    /* Note: SPI_NSS_HARD_OUTPUT is a 32 bit int defined in the chip vendor 
     * supplied stm32f4xx_hal_spi.h file.
     * Since we're using only the bottom 16 bits, the bitmask is used to block out 
     * the upper 16.
     *
     * Also, there is no SS polarity control in the ic, so that setting is ignored.
     */
    temp16 &= ~(SPI_NSS_SOFT | (SPI_NSS_HARD_OUTPUT & 0xFFFF));

    if ((device->spi_config & SPI_CFG_SS_POL_SW_CONTROL) == SPI_CFG_SS_POL_SW_CONTROL)
    	temp16 |= SPI_NSS_SOFT;
    else
    	temp16 |= (SPI_NSS_HARD_OUTPUT & 0xFFFF);

    /* Write configuration register */
    ESAL_GE_MEM_READ16(spi_base_address + SPI_CR1) = temp16;


    /*----------------------- SPI CR2 Register Settings ---------------------*/

    /* Read configuration register */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR2);
    
    /* Set SPI Protocol type, TI or Motorola. */
    temp16 &= ~(SPI_TIMODE_DISABLED | SPI_TIMODE_ENABLED);

    if (device->spi_config & SPI_CFG_PROT_TI)       temp16 |= SPI_TIMODE_ENABLED;
    else if (device->spi_config & SPI_CFG_PROT_MOT) temp16 |= SPI_TIMODE_DISABLED;
    else return LWSPI_UNSUPPORTED_PROTOCOL;

    /* Set SPI Chip Select / NSS output mode for master mode */
    /* If the output is under software control, then disable the hardware 
     * control by setting code *_HARD_INPUT.  Otherwise set it to 
     * *_HARD_OUTPUT for hardware output control.
     *
     * The shift by 16 is due to the way the macro is defined in the 
     * chip vendor supplied file.
     */
    temp16 &= ~((SPI_NSS_HARD_INPUT | SPI_NSS_HARD_OUTPUT) >> 16);

    if ((device->spi_config & SPI_CFG_SS_POL_SW_CONTROL) == SPI_CFG_SS_POL_SW_CONTROL)
		temp16 |= (SPI_NSS_HARD_INPUT >> 16);
    else
    	temp16 |= (SPI_NSS_HARD_OUTPUT >> 16);

    /* Write configuration register */
    ESAL_GE_MEM_READ16(spi_base_address + SPI_CR2) = temp16;


    /* Enable the device again. */
    LWSPI_TGT_Device_Enable(spi_dev_inst_ptr);

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_DMA_Configure
*
*       Terabit Radio ah  Aug 11 2014
*       Created function to initialize SPI DMA 
*
* DESCRIPTION
*
*       This function configures the SPI DMA and sets appropriate 
*       addresses in the DMA request structures.
*
* INPUTS
*
*       device                              Device pointer which contains
*                                           SPI bus parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_DMA_Configure(NU_SPI_DEVICE *spi_device)
{
  NU_SPI_BUS            *spi_bus  = spi_device->bus;
  LWSPI_INSTANCE_HANDLE *inst_ptr = spi_bus->dev_context;
  UINT16                temp16;

#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE
  /* Set up DMA peripheral addresses */
  spi_device->dma_tx_req.dst_ptr = inst_ptr->io_addr + SPI_DR;
  spi_device->dma_rx_req.src_ptr = inst_ptr->io_addr + SPI_DR;

  /* Enable the DMA interface on the peripheral */
  temp16 = ESAL_GE_MEM_READ16(inst_ptr->io_addr + SPI_CR2);
  temp16 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
  ESAL_GE_MEM_READ16(inst_ptr->io_addr + SPI_CR2) = temp16;

  return NU_SUCCESS;

#else 

  return  LWSPI_UNSUPPORTED_DMA;

#endif  /* CFG_NU_OS_DRVR_DMA_ENABLE */
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Read
*
* DESCRIPTION
*
*       This function reads the received data from the specified Nucleus
*       SPI device.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       polling                             Specifies that if current I/O
*                                           should be done in polling or
*                                           interrupt mode.
*       read_irp                            I/O request packet for this
*                                           particular transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
**************************************************************************/
STATUS LWSPI_TGT_Read(VOID *spi_inst_ptr, BOOLEAN polling, NU_SPI_IRP *spi_irp)
{
    LWSPI_INSTANCE_HANDLE *spi_dev_inst_ptr;
    UINT8                 *spi_base_address;
    STATUS                 status;
    LWSPI_TGT             *tgt_ptr;
    NU_SPI_DEVICE         *spi_device;
    UINT8                 *buffer_8;
    UINT16                *buffer_16;

    LWSPI_TGT_Read_ENTRY;

    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    status              = NU_SUCCESS;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device          = spi_irp->device;
    buffer_8            = spi_irp->buffer;
    buffer_16           = spi_irp->buffer;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE  pmi_dev = (spi_dev_inst_ptr->pmi_dev);

   /* Check current state of the device. If the device is off, suspend on an
      event until the device power state changes to ON. */
    if ((PMI_STATE_GET(pmi_dev) == SPI_OFF)||(PMI_IS_PARKED(pmi_dev) == NU_TRUE))
    {
        /* Wait until the device is available for a read operation. */
        PMI_WAIT_CYCLE(pmi_dev, status);
    }
#endif

    /* Check if transfer is required in polling mode. */
    if(polling)
    {
        while((spi_irp->actual_length) != (spi_irp->length))
        {
            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

           /* Wait for the Tx fifo to get empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

            /* Write out the dummy data to initiate the Rx. */
            ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, 0x0000);

            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

            /* Wait for the Rx fifo to get non-empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_RXNE) != SPI_SR_RXNE){}

            /* Get the received data. */
            if (spi_device->transfer_size == 16)
                *buffer_16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);
            else
                *buffer_8 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);


            /* Increment the number of data units that are
               processed for the current transfer. */
            spi_irp->actual_length++;

            if (spi_device->transfer_size == 16)
              buffer_16++;
            else
              buffer_8++;

            /* Check if transfer delay is required. */
            if (tgt_ptr->trans_delay != 0 &&
               ((spi_irp->actual_length) != (spi_irp->length)))
            {
                ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
            }
        }

        /* Check if chip select delay is required. */
        if (tgt_ptr->cs_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->cs_delay);
        }
    }
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Otherwise it is interrupt driven transfer. */
    else
    {
        /* Save the read IRP and the IO operation we are interested in. */
        spi_dev_inst_ptr->curr_read_irp = spi_irp;
        spi_dev_inst_ptr->io_operation = LWSPI_IO_OPERATION_READ;

        /* Wait for the Tx fifo to get empty. */
        while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

        /* Enable SPI interrupts. */
        LWSPI_TGT_Intr_Enable(spi_dev_inst_ptr);

        /* Write out the dummy data to initiate the Rx. */
        ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, 0x0000);
    }
#else
    /* If interrupt mode is not supported then it is an error. */
    else
    {
        status = NU_SPI_INVLD_ARG;
    }
#endif /* #if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE) */

    LWSPI_TGT_Read_EXIT;
    return status;
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Write
*
* DESCRIPTION
*
*       This function transmits data to the specified slave of the
*       specified SPI device.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       polling                             Specifies that if current I/O
*                                           should be done in polling or
*                                           interrupt mode.
*       write_irp                           I/O request packet for this
*                                           particular transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Write(VOID *spi_inst_ptr, BOOLEAN polling, NU_SPI_IRP* spi_irp)
{
    LWSPI_INSTANCE_HANDLE *spi_dev_inst_ptr;
    UINT8                 *spi_base_address;
    STATUS                 status;
    LWSPI_TGT             *tgt_ptr;
    NU_SPI_DEVICE         *spi_device;
    UINT8                 **buffer_8;
    UINT16                **buffer_16;

    LWSPI_TGT_Write_ENTRY;

    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    status              = NU_SUCCESS;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device          = spi_irp->device;
    buffer_8            = &(spi_irp->buffer);
    buffer_16           = &(spi_irp->buffer);


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE pmi_dev = (spi_dev_inst_ptr->pmi_dev);

   /* Check current state of the device. If the device is off, suspend on an
       event until the device power state changes to ON. */
    if ((PMI_STATE_GET(pmi_dev) == SPI_OFF)||(PMI_IS_PARKED(pmi_dev) == NU_TRUE))
    {
        /* Wait until the device is available for a write operation. */
        PMI_WAIT_CYCLE(pmi_dev, status);
    }
#endif

    /* Check if transfer is required in polling mode. */
    if (polling)
    {
        while((spi_irp->actual_length) != (spi_irp->length))
        {
            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

            /* Wait for the Tx fifo to get empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

            /* Write out the data. */
            if (spi_device->transfer_size == 16)
                ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_16);
            else
                ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_8);

            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

            /* Wait for the Rx fifo to get non-empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_RXNE) != SPI_SR_RXNE){}

            /* Get the received data and ignore it as we are interested in data Tx only. */
            ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);

            /* Increment the number of data units that are
               processed for the current transfer. */
            spi_irp->actual_length++;

            if (spi_device->transfer_size == 16)
                (*buffer_16)++;
            else
                (*buffer_8)++;


            /* Check if transfer delay is required. */
            if (tgt_ptr->trans_delay != 0 &&
               ((spi_irp->actual_length) != (spi_irp->length)))
            {
                ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
            }
        }

        /* Check if chip select delay is required. */
        if (tgt_ptr->cs_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->cs_delay);
        }
    }
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Otherwise it is interrupt driven transfer. */
    else
    {
        /* Save the write IRP and the IO operation we are interested in. */
        spi_dev_inst_ptr->curr_write_irp = spi_irp;
        spi_dev_inst_ptr->io_operation = LWSPI_IO_OPERATION_WRITE;

        /* Wait for the Tx fifo to get empty. */
        while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

        /* Enable SPI interrupts. */
        LWSPI_TGT_Intr_Enable(spi_dev_inst_ptr);

        /* Write out the data. */
        if (spi_device->transfer_size == 16)
            ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_16);
        else
            ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_8);


    }
#else
    /* If interrupt mode is not supported then it is an error. */
    else
    {
        status = NU_SPI_INVLD_ARG;
    }
#endif /* #if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE) */

    LWSPI_TGT_Write_EXIT;
    
    return status;
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Write_Read
*
* DESCRIPTION
*
*       This function transmits and receives data to the specified slave
*       of the specified SPI device.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       polling                             Specifies that if current I/O
*                                           should be done in polling or
*                                           interrupt mode.
*       write_irp                           I/O request packet for the
*                                           Tx transfer.
*       read_irp                            I/O request packet for the
*                                           Rx transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Write_Read(VOID *spi_inst_ptr, BOOLEAN polling,
                            NU_SPI_IRP *write_irp, NU_SPI_IRP *read_irp)
{
    LWSPI_INSTANCE_HANDLE *spi_dev_inst_ptr;
    UINT8                 *spi_base_address;
    STATUS                 status;
    LWSPI_TGT             *tgt_ptr;
    NU_SPI_DEVICE         *spi_device;
    UINT8				  *wr_buffer_8;
    UINT16				  *wr_buffer_16;
    UINT8				  *rd_buffer_8;
    UINT16				  *rd_buffer_16;

    LWSPI_TGT_Write_Read_ENTRY;
    
    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    status              = NU_SUCCESS;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device			= write_irp->device;
    wr_buffer_8			= write_irp->buffer;
    wr_buffer_16		= write_irp->buffer;
    rd_buffer_8			= read_irp->buffer;
    rd_buffer_16		= read_irp->buffer;


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE pmi_dev = (spi_dev_inst_ptr->pmi_dev);

   /* Check current state of the device. If the device is off, suspend on an
       event until the device power state changes to ON. */
    if ((PMI_STATE_GET(pmi_dev) == SPI_OFF)||(PMI_IS_PARKED(pmi_dev) == NU_TRUE))
    {
        /* Wait until the device is available for a write operation. */
        PMI_WAIT_CYCLE(pmi_dev, status);
    }
#endif
  

    /* Check if transfer is required in polling mode. */
    if (polling)
    {
        while((read_irp->actual_length != read_irp->length) &&
              (write_irp->actual_length != write_irp->length))
        {
            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

            /* Wait for the Tx fifo to get empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

            /* Write out the data. */
            if (spi_device->transfer_size == 16)
            	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_16);
            else
            	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_8);


            /* Increment the number of data units that are
               processed for the current transfer. */
            write_irp->actual_length++;

            if (spi_device->transfer_size == 16)
                wr_buffer_16++;
            else
                wr_buffer_8++;

            /* Wait for the SPI bus to get free. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == 1){}

            /* Wait for the Rx fifo to get non-empty. */
            while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_RXNE) != SPI_SR_RXNE){}

            /* Get the received data. */
            if (spi_device->transfer_size == 16)
                *rd_buffer_16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);
            else
                *rd_buffer_8 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);

            /* Increment the number of data units that are
               processed for the current transfer. */
            read_irp->actual_length++;

            if (spi_device->transfer_size == 16)
            	rd_buffer_16++;
            else
            	rd_buffer_8++;


            /* Check if transfer delay is required. */
            if (tgt_ptr->trans_delay != 0 &&
               ((read_irp->actual_length) != (read_irp->length)))
            {
                ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
            }
        }

        /* Check if chip select delay is required. */
        if (tgt_ptr->cs_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->cs_delay);
        }
    }
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Otherwise it is interrupt driven transfer. */
    else
    {
        /* Save the write IRP and the IO operation we are interested in. */
        spi_dev_inst_ptr->curr_read_irp = read_irp;
        spi_dev_inst_ptr->curr_write_irp = write_irp;
        spi_dev_inst_ptr->io_operation = LWSPI_IO_OPERATION_WRITE_READ;

        /* Wait for the Tx fifo to get empty. */
        while ((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_TXE) != SPI_SR_TXE){}

        /* Enable SPI interrupts. */
        LWSPI_TGT_Intr_Enable(spi_dev_inst_ptr);

        /* Write out the data. */
        if (spi_device->transfer_size == 16)
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_16);
        else
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_8);
    }
#else
    /* If interrupt mode is not supported then it is an error. */
    else
    {
        status = NU_SPI_INVLD_ARG;
    }
#endif /* #if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE) */

    LWSPI_TGT_Write_Read_EXIT;

    return status;
}

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Intr_Enable
*
*   DESCRIPTION
*
*       If interrupt mode, enables controller interrupts.
*
*   INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID LWSPI_TGT_Intr_Enable(LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    UINT8   *spi_base_address;
    UINT16   temp16;

    /* Get base address if SPI hardware controller. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Enable required interrupts at module level. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR2);
    temp16 |= (SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR2), temp16);
}

/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Intr_Disable
*
*   DESCRIPTION
*
*       If interrupt mode, mask controller interrupts.
*
*   INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID LWSPI_TGT_Intr_Disable(LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    UINT8   *spi_base_address;
    UINT16  temp16;

    /* Get base address if SPI hardware controller. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Turn off interrupts. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR2);
    temp16 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR2), temp16);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_ISR_Read
*
* DESCRIPTION
*
*       This function handles ISR read operations.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       read_irp                            I/O request packet for this
*                                           particular transfer.
*
* OUTPUTS
*
*       NU_TRUE                             I/O is complete
*       NU_FALSE                            I/O incomplete
*
*************************************************************************/
BOOLEAN LWSPI_TGT_ISR_Read(VOID *spi_inst_ptr, NU_SPI_IRP *spi_irp)
{
    LWSPI_INSTANCE_HANDLE   *spi_dev_inst_ptr;
    UINT8                 *spi_base_address;
    BOOLEAN                  io_complete;
    LWSPI_TGT               *tgt_ptr;
    NU_SPI_DEVICE           *spi_device;
    UINT8                   *buffer_8;
    UINT16                  *buffer_16;

    LWSPI_TGT_ISR_Read_ENTRY;

    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device          = spi_irp->device;
    buffer_8            = spi_irp->buffer;
    buffer_16           = spi_irp->buffer;

    /* Get the received data. */
    if (spi_device->transfer_size == 16)
        *buffer_16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);
    else
        *buffer_8 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);

    /* Increment the number of data units that are
       processed for the current transfer. */
    spi_irp->actual_length++;

    if (spi_device->transfer_size == 16)
      buffer_16++;
    else
      buffer_8++;


    /* Continue data read operation. */
    if(spi_irp->actual_length < spi_irp->length)
    {
        /* Check if transfer delay is required. */
        if (tgt_ptr->trans_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
        }

        /* Write out the dummy data. */
        ESAL_GE_MEM_WRITE16(spi_dev_inst_ptr->io_addr + SPI_DR, 0x0000);
        io_complete = NU_FALSE;
    }
    else
    {
        /* Requested operation is complete. */
        io_complete = NU_TRUE;
    }

    LWSPI_TGT_ISR_Read_EXIT;

    /* Return result of I/O operation. */
    return (io_complete);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_ISR_Write
*
* DESCRIPTION
*
*       This function handles ISR write operations.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       write_irp                           I/O request packet for this
*                                           particular transfer.
*
* OUTPUTS
*
*       NU_TRUE                             I/O is complete.
*       NU_FALSE                            I/O incomplete.
*
*************************************************************************/
BOOLEAN LWSPI_TGT_ISR_Write(VOID *spi_inst_ptr, NU_SPI_IRP *spi_irp)
{
    LWSPI_INSTANCE_HANDLE   *spi_dev_inst_ptr;
    UINT8                   *spi_base_address;
    BOOLEAN                  io_complete;
    LWSPI_TGT               *tgt_ptr;
    NU_SPI_DEVICE           *spi_device;
    UINT8                   **buffer_8;
    UINT16                  **buffer_16;

    LWSPI_TGT_ISR_Write_ENTRY;

    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device          = spi_irp->device;
    buffer_8            = &(spi_irp->buffer);
    buffer_16           = &(spi_irp->buffer);


    /* Clear the Rx FIFO. */
    ESAL_GE_MEM_READ16(spi_dev_inst_ptr->io_addr + SPI_DR);

    /* Increment the number of data units that are
       processed for the current transfer. */
    spi_irp->actual_length++;

    if (spi_device->transfer_size == 16)
      (*buffer_16)++;
    else
      (*buffer_8)++;

    /* Continue data write operation. */
    if(spi_irp->actual_length < spi_irp->length)
    {
        /* Check if transfer delay is required. */
        if (tgt_ptr->trans_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
        }

        /* Write out the data. */
        if (spi_device->transfer_size == 16)
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_16);
        else
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, **buffer_8);

        io_complete = NU_FALSE;
    }
    else
    {
        /* Requested operation is complete. */
        io_complete = NU_TRUE;
    }

    LWSPI_TGT_ISR_Write_EXIT;

    /* Return result of I/O operation. */
    return (io_complete);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_ISR_Write_Read
*
* DESCRIPTION
*
*       This function handles ISR write/read operations.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       write_irp                           I/O request packet for the
*                                           Tx transfer.
*       read_irp                            I/O request packet for the
*                                           Rx transfer.
*
* OUTPUTS
*
*       NU_TRUE                             I/O is complete.
*       NU_FALSE                            I/O incomplete.
*
*************************************************************************/
BOOLEAN LWSPI_TGT_ISR_Write_Read(VOID *spi_inst_ptr, NU_SPI_IRP *write_irp, NU_SPI_IRP *read_irp)
{
    LWSPI_INSTANCE_HANDLE   *spi_dev_inst_ptr;
    BOOLEAN                  io_complete;
    UINT8                 *spi_base_address;
    LWSPI_TGT             *tgt_ptr;
    NU_SPI_DEVICE         *spi_device;
    UINT8				          *wr_buffer_8;
    UINT16				        *wr_buffer_16;
    UINT8				          *rd_buffer_8;
    UINT16				        *rd_buffer_16;

    LWSPI_TGT_ISR_Write_Read_ENTRY;
    
    /* Initialize local variables. */
    spi_dev_inst_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    spi_base_address    = (UINT8 *)spi_dev_inst_ptr->io_addr;
    tgt_ptr             = (LWSPI_TGT *)spi_dev_inst_ptr->spi_tgt_ptr;
    spi_device			    = write_irp->device;
    wr_buffer_8			    = write_irp->buffer;
    wr_buffer_16		    = write_irp->buffer;
    rd_buffer_8			    = read_irp->buffer;
    rd_buffer_16		    = read_irp->buffer;

    

    /* Get the received data. */
    if (spi_device->transfer_size == 16)
        *rd_buffer_16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);
    else
        *rd_buffer_8 = ESAL_GE_MEM_READ16(spi_base_address + SPI_DR);

    /* Increment the number of data units that are
       processed for the current transfer. */
    write_irp->actual_length++;
    read_irp->actual_length++;

    if (spi_device->transfer_size == 16)
    { rd_buffer_16++; wr_buffer_16++;  }
    else
    { rd_buffer_8++; wr_buffer_8++;  }


    /* Continue data write/read operation. Lengths for write/read operations
     * should be equal. Both read and write IRPs can be used to compare lengths.
     */
    if(read_irp->actual_length < read_irp->length)
    {
        /* Check if transfer delay is required. */
        if (tgt_ptr->trans_delay != 0)
        {
            ESAL_PR_Delay_USec(tgt_ptr->trans_delay);
        }

        /* Write out the data. */
        /* Write out the data. */
        if (spi_device->transfer_size == 16)
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_16);
        else
        	ESAL_GE_MEM_WRITE16(spi_base_address + SPI_DR, *wr_buffer_8);

        io_complete = NU_FALSE;
    }
    else
    {
        /* Requested operation is complete. */
        io_complete = NU_TRUE;
    }

    LWSPI_TGT_ISR_Write_Read_EXIT;

    /* Return result of I/O operation. */
    return (io_complete);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_LISR
*
* DESCRIPTION
*
*       This is the entry function for the ISR (Interrupt Service Routine)
*       that services the SPI module interrupts.
*
* INPUTS
*
*       vector                              Interrupt vector.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID LWSPI_TGT_LISR(INT vector)
{
    LWSPI_INSTANCE_HANDLE    *spi_inst_ptr;
    UINT8                    *spi_base_address;

    LWSPI_TGT_LISR_ENTRY;

    /* Get a pointer to the instance handle for this vector. */
    spi_inst_ptr = (LWSPI_INSTANCE_HANDLE *) ESAL_GE_ISR_VECTOR_DATA_GET (vector);

    /* Get base address of the SPI module. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Process further if it is a valid (Rx) interrupt. */
    if (ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & (SPI_SR_RXNE))
    {
        /* Disable SPI interrupts. */
        LWSPI_TGT_Intr_Disable(spi_inst_ptr);

        /* Push data to a HISR for processing. */
        LWSPI_ISR_Data_Set(spi_inst_ptr);
    }

    LWSPI_TGT_LISR_EXIT;
}

#endif /* CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE */

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Device_Enable
*
* DESCRIPTION
*
*       This function enables the controller.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID LWSPI_TGT_Device_Enable(LWSPI_INSTANCE_HANDLE* spi_inst_ptr)
{
    UINT8   *spi_base_address;
    UINT16  temp16;

    /* Get base address if SPI hardware controller. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Enable the SPI module. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR1);
    temp16 |= SPI_CR1_SPE;
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR1), temp16);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Device_Disable
*
* DESCRIPTION
*
*       This function disables the controller.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID LWSPI_TGT_Device_Disable(LWSPI_INSTANCE_HANDLE* spi_inst_ptr)
{
    UINT8   *spi_base_address;
    UINT16  temp16;

    /* Get base address if SPI hardware controller. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Disable the SPI module. */
    temp16 = ESAL_GE_MEM_READ16(spi_base_address + SPI_CR1);
    temp16 &= ~SPI_CR1_SPE;
    ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR1), temp16);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Set_Baud_Rate
*
* DESCRIPTION
*
*       This function sets the baud rate for SPI transfers.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       baud_rate                           Desired baud rate.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*       LWSPI_UNSUPPORTED_BAUD_RATE         The specified baud rate is not
*                                           supported.
*
**************************************************************************/
STATUS LWSPI_TGT_Set_Baud_Rate(LWSPI_INSTANCE_HANDLE *spi_inst_ptr,
                         UINT32                      baud_rate)
{
    UINT8      *spi_base_address;
    STATUS      status;
    UINT16      tmp16;
    UINT32      baud_div;
    UINT32      clock_rate = ESAL_PR_TMR_OS_CLOCK_RATE;

    /* Get base address of SPI hardware controller. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Initialize return status with success value. */
    status = NU_SUCCESS;

    /* Get current value of SPI_CR1. */
    tmp16 = ESAL_GE_MEM_READ16 (spi_base_address + SPI_CR1);

    /* Clear current baud rate divisor. */
    tmp16 &= ~SPI_CR1_BR_MSK;

    /* Get the hardware settings for the specified baud rate.
     * System frequency is XMHz.
     */

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    LWSPI_TGT_Pwr_Get_Clock_Rate(spi_inst_ptr, &clock_rate);
#else
    clock_rate = spi_inst_ptr->clock;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    baud_div = clock_rate / (baud_rate);

    if(baud_div ==2)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_2;
    }

    else if(baud_div >=3 && baud_div <=4)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_4;
    }

    else if(baud_div >=5 && baud_div <=8)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_8;
    }

    else if(baud_div >=9 && baud_div <=16)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_16;
    }

    else if(baud_div >=17 && baud_div <=32)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_32;
    }

    else if(baud_div >=33 && baud_div <=64)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_64;
    }

    else if(baud_div >=65 && baud_div <=128)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_128;
    }

    else if(baud_div >=129 && baud_div <=256)
    {   /* Divide by 2. */
        tmp16 |= SPI_CR1_BR_256;
    }
    else
    {
        /* Otherwise indicate that the specified baud rate is not
         * supported.
         */
        status = LWSPI_UNSUPPORTED_BAUD_RATE;
    }

    /* Proceed to apply new baud rate if above operations were
     * successful.
     */
    if (status == NU_SUCCESS)
    {
        /* Write new baud rate divisor to the SPI_CR1 register. */
        ESAL_GE_MEM_WRITE16((spi_base_address + SPI_CR1), tmp16);
    }

    /* Return the completion status of the service. */
    return (status);
}


/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Check_Device_Busy
*
* DESCRIPTION
*
*       This function checks if there is an I/O operation pending on
*       device.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
* OUTPUTS
*
*       NU_FALSE                            Device is free. There is no
*                                           active I/O on device.
*       NU_TRUE                             Device is busy. There is
*                                           active I/O on device.
*
*************************************************************************/
BOOLEAN LWSPI_TGT_Check_Device_Busy(LWSPI_INSTANCE_HANDLE  *spi_inst_ptr)
{
    UINT8   *spi_base_address;
    BOOLEAN  device_busy = NU_FALSE;

    /* Initialize local variables. */
    spi_base_address = (UINT8 *) spi_inst_ptr->io_addr;

    /* Check if SPI is enabled? */
    if(ESAL_GE_MEM_READ32 (spi_base_address + SPI_CR1) & SPI_CR1_SPE)
    {
        /* If enabled, check the busy bit. */
        if((ESAL_GE_MEM_READ16(spi_base_address + SPI_SR) & SPI_SR_BSY) == SPI_SR_BSY)
        {
            device_busy = NU_TRUE;
        }
    }

    return (device_busy);
}



/**************************************************************************
* FUNCTION
*
*       LWSPI_TGT_Get_STM_SPI_Device_Info
*
* DESCRIPTION
*       ahonkan: terabit radios
*
*       This function gets the physical spi device number from the 
*       registry and sets the address, vector, clock and other hardware
*       specific parameters.
*
*       Note:  This routine is not generic to STM hardware. Specific to 
*       devices with 6 SPI devices.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successfully retrieved 
*                                           active I/O on device.
*       NU_FAIL                             Device is busy. There is
*                                           active I/O on device.
*
*************************************************************************/
#define SPI_INSTANCE_ASSIGN(spi_num, __instance_ptr)                                \
        do                                                                          \
        {                                                                           \
            __instance_ptr->io_addr = SPI ## spi_num ## _BASE;                      \
            __instance_ptr->intr_vector = ESAL_PR_SPI ## spi_num ## _INT_VECTOR_ID; \
                                                                                    \
            if ( (spi_num == 2) || (spi_num == 3))                                  \
              strncpy(&(__instance_ptr->ref_clock[0]),                              \
                        CPU_APB1CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);                   \
            else                                                                    \
              strncpy(&(__instance_ptr->ref_clock[0]),                              \
                        CPU_APB2CLK_FREQ, NU_DRVR_REF_CLOCK_LEN);                   \
        } while(0)

static  STATUS  LWSPI_TGT_Get_STM_SPI_Device_Info(const CHAR *key, LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    STATUS    reg_status = NU_SUCCESS;
    UINT32    temp;
    LWSPI_TGT *tgt_ptr = (LWSPI_TGT*)spi_inst_ptr->spi_tgt_ptr;

    /* Go through the registry and extract the STM SPI device configuration parameters.  These are:
     
      spi_dev:        [1,6]
      intr_priority:  0x1007
      trans_delay:    4
      cs_delay:       4

    */      

    /* Process SPI Device and set the io address, vector and peripheral clock source 
     * spi_dev:        [1,6]  
     */
    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/spi_dev", (UINT32*)&temp);
        
    if(reg_status != NU_SUCCESS)
        return reg_status;            

    switch(temp)
    {
        case 1: SPI_INSTANCE_ASSIGN(1, spi_inst_ptr); break;
        case 2: SPI_INSTANCE_ASSIGN(2, spi_inst_ptr); break;
        case 3: SPI_INSTANCE_ASSIGN(3, spi_inst_ptr); break;
        case 4: SPI_INSTANCE_ASSIGN(4, spi_inst_ptr); break;
        case 5: SPI_INSTANCE_ASSIGN(5, spi_inst_ptr); break;
        case 6: SPI_INSTANCE_ASSIGN(6, spi_inst_ptr); break;
        default: return NU_INVALID_OPTIONS;
    }

    /* Set up dma peri ID based on spi dev setting */
    switch(temp)
    {
        case 1: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI1_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI1_RX;
                break;  
        case 2: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI2_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI2_RX;
                break;  
        case 3: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI3_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI3_RX;
                break;  
        case 4: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI4_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI4_RX;
                break;  
        case 5: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI5_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI5_RX;
                break;  
        case 6: spi_inst_ptr->dma_intf.tx_dma_peri_id = DMA_SPI6_TX;  
                spi_inst_ptr->dma_intf.rx_dma_peri_id = DMA_SPI6_RX;
                break;  

        default: return NU_INVALID_OPTIONS;
    }

    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/dma_enable", (UINT32*)&temp);
            
    if(reg_status != NU_SUCCESS)
      temp = 0;            

    spi_inst_ptr->dma_intf.dma_enable = temp;


    /* Get the defined peripheral clock frequency used for baud calculation. */
    reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/clock",
                                            &(spi_inst_ptr->clock));

    if(reg_status != NU_SUCCESS)
    	spi_inst_ptr->clock = 90e6;
    
    /*  intr_priority:  0x1007 */
    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/intr_priority", (UINT32 *)&(spi_inst_ptr->intr_priority));
        
    if(reg_status != NU_SUCCESS)
        return reg_status;            

    /* Get and save transfer delay. */
    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/trans_delay", (UINT32*)&tgt_ptr->trans_delay);
    if(reg_status != NU_SUCCESS)
    	tgt_ptr->trans_delay = 0;

    /* Get and save chip select delay. */
    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/cs_delay", (UINT32*)&tgt_ptr->cs_delay);
    if(reg_status != NU_SUCCESS)
    	tgt_ptr->cs_delay = 0;


    /* Get and save tx and rx dma device ids assocated with this spi interface. */
    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/tx_dma_dev_id", (UINT32*)&temp);
    if(reg_status != NU_SUCCESS)
        return reg_status;

    spi_inst_ptr->dma_intf.tx_dma_dev_id = (UINT8)temp;

    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/rx_dma_dev_id", (UINT32*)&temp);
    if(reg_status != NU_SUCCESS)
        return reg_status;

    spi_inst_ptr->dma_intf.rx_dma_dev_id = (UINT8)temp;

    /* Get and save setup function. */
    reg_status = REG_Get_UINT32_Value(key, "/setup", (UINT32*)&tgt_ptr->setup_func);
    if(reg_status != NU_SUCCESS)
        return reg_status;            

    /* Get and save cleanup function. */
    reg_status = REG_Get_UINT32_Value(key, "/cleanup", (UINT32*)&tgt_ptr->cleanup_func);
    if(reg_status != NU_SUCCESS)
        return reg_status;            

    return reg_status;
}



