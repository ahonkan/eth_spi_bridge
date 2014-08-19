/**************************************************************************
*            Copyright 2007 Mentor Graphics Corporation
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
*       spi_common.c
*
* COMPONENT
*
*       SPI_DRIVER                  SPI library
*
* DESCRIPTION
*
*       This file contains the generic SPI library functions.
*
* FUNCTIONS
*
*       SPI_Get_Target_Info
*       SPI_PR_Intr_Enable
*       SPI_Set_Reg_Path
*       SPI_Call_Setup_Func
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/**********************************/
/* EXTERNAL FUNCTION PROTOTYPES   */
/**********************************/
extern VOID SPI_Tgt_LISR(INT vector);
extern VOID SPI_Tgt_Intr_Enable(SPI_INSTANCE_HANDLE* spi_inst_ptr);

/**************************************************************************
* FUNCTION
*
*       SPI_Get_Target_Info
*
* DESCRIPTION
*
*       This function gets target attributes from Registry
*
* INPUTS
*
*       key                                 - Registry key
*       spi_inst_ptr                        - Pointer to spi instance handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*       SPI_DEV_REGISTRY_ERROR              Error occurred while reading target
*                                           information.
*
**************************************************************************/
STATUS SPI_Get_Target_Info(const CHAR * key, SPI_INSTANCE_HANDLE* spi_inst_ptr)
{
    STATUS reg_status;
    STATUS status;

    /* Get the base io address */
    reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/io_addr", &(spi_inst_ptr->spi_io_addr));
    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }
    else
    {
        status = SPI_DEV_REGISTRY_ERROR;
    }

    if(status == NU_SUCCESS)
    {
        /* Get the clock */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/clock", &(spi_inst_ptr->spi_clock));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get the interrupt vector */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32 *)&(spi_inst_ptr->spi_intr_vector));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get the interrupt priority */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_priority", (UINT32 *)&(spi_inst_ptr->spi_intr_priority));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get the interrupt type */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_type", (UINT32 *)&(spi_inst_ptr->spi_intr_type));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get the slave count */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/slave_cnt", (UINT32 *)&(spi_inst_ptr->spi_slave_cnt));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get master/slave mode */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/master_mode", (UINT32*)&(spi_inst_ptr->spi_master_mode));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }
    
    if(status == NU_SUCCESS)
    {
        /* Get the transfer attributes simulation flag. */
        reg_status = REG_Get_Boolean_Value (key, "/tgt_settings/simulate_transfer_attributes", (BOOLEAN*)&(spi_inst_ptr->spi_sim_tx_attribs));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }
    
    if(status == NU_SUCCESS)
    {
        /* Get interrupt/polled mode */
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/driver_mode", (UINT32*)&(spi_inst_ptr->spi_driver_mode));
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Get the reference clock */
        reg_status = REG_Get_String_Value (key, "/tgt_settings/ref_clock", &(spi_inst_ptr->spi_ref_clock[0]), NU_DRVR_REF_CLOCK_LEN);
        if(reg_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*       SPI_PR_Intr_Enable
*
* DESCRIPTION
*
*       This function initializes processor-level interrupt
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
STATUS SPI_PR_Intr_Enable(SPI_DRV_SESSION* spi_ses_ptr)
{
    SPI_INSTANCE_HANDLE *spi_inst_ptr = (spi_ses_ptr->spi_inst_ptr);
    VOID                (*old_lisr)(INT);
    STATUS              status = NU_SUCCESS;
    
    /* Check if interrupt-driven mode is specified for this device. */
    if (spi_inst_ptr->spi_driver_mode & SPI_INTERRUPT_MODE)
    {
        /* Register the interrupt service routine for transfer complete
           flag interrupt. */
        status = NU_Register_LISR (spi_inst_ptr->spi_intr_vector,
                                       SPI_Tgt_LISR, &old_lisr);

        /* Check if ISR registration was successful. */
        if (status == NU_SUCCESS)
        {
            /* Save session */
            ESAL_GE_ISR_VECTOR_DATA_SET (spi_inst_ptr->spi_intr_vector, (VOID*) spi_ses_ptr);

            SPI_Tgt_Intr_Enable (spi_inst_ptr);

            /* Enable the SPI interrupt */
            (VOID)ESAL_GE_INT_Enable(spi_inst_ptr->spi_intr_vector,
                                     spi_inst_ptr->spi_intr_type, spi_inst_ptr->spi_intr_priority);
        }
    }
    
    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       SPI_Set_Reg_Path
*
*   DESCRIPTION
*
*       This function copies registry path to device handle.
*
*   INPUTS
*
*       key                                 - Registry path
*       spi_inst_ptr                        - pointer to spi instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SPI_DEV_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS SPI_Set_Reg_Path(const CHAR * key, SPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    STATUS     status = NU_SUCCESS;

    /********************************/
    /* COPY REG PATH */
    /********************************/
    if((strlen(key)+1) <= REG_MAX_KEY_LENGTH)
    {
        strcpy(spi_inst_ptr->reg_path, key);
        status = NU_SUCCESS;
    }
    else
    {
        status = SPI_DEV_REGISTRY_ERROR;
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       SPI_Call_Setup_Func
*
*   DESCRIPTION
*
*       This function gets setup function from Registry and calls it.
*
*   INPUTS
*
*       key                                 - Registry path
*       spi_inst_ptr                        - pointer to spi instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SPI_DEV_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS SPI_Call_Setup_Func(const CHAR * key, SPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[REG_MAX_KEY_LENGTH];
    STATUS     (*setup_func)(VOID*);

    /* Get setup function if exists */
    strcpy(reg_path, key);
    strcat(reg_path,"/setup");
    if (REG_Has_Key (reg_path))
    {
        status = REG_Get_UINT32 (reg_path, (UINT32*)&setup_func);
        if (status == NU_SUCCESS && setup_func != NU_NULL)
        {
            (VOID)setup_func(spi_inst_ptr);
        }
    }

    return status;
}
