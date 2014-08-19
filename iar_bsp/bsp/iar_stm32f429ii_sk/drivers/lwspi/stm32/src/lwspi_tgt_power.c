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
*       lwspi_tgt_power.c
*
* COMPONENT
*
*       Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains the routine for power services.
*
* FUNCTIONS
*
*       LWSPI_TGT_Pwr_Default_State
*       LWSPI_TGT_Pwr_Set_State
*       SPI_Pwr_Get_Clock_Rate
*       LWSPI_TGT_Pwr_Pre_Park
*       LWSPI_TGT_Pwr_Post_Park
*       LWSPI_TGT_Pwr_Pre_Resume
*       LWSPI_TGT_Pwr_Post_Resume
*       LWSPI_TGT_Pwr_Resume_End
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       lwspi_tgt.h
*       lwspi_tgt_power.h
*
**************************************************************************/
/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/lwspi/stm32/lwspi_tgt.h"
#include "bsp/drivers/lwspi/stm32/lwspi_tgt_power.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
/**************************************************************************
*
* FUNCTION
*
*       LWSPI_TGT_Pwr_Default_State
*
* DESCRIPTION
*
*       This function places the SPI device in lowest-power state.
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_TGT_Pwr_Default_State (LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    /* Disable SPI device. */
    LWSPI_TGT_Device_Disable(spi_inst_ptr);

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Disable SPI controller interrupts. */
    LWSPI_TGT_Intr_Disable(spi_inst_ptr);
#endif

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the SPI.
*
*   INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       state                               New state of SPI device.
*
*   OUTPUT
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Set_State(VOID *spi_inst_ptr, PM_STATE_ID *state)
{
    LWSPI_INSTANCE_HANDLE   *spi_inst_hdl_ptr;
    LWSPI_TGT               *tgt_ptr;
    VOID                    (*setup_fn)(VOID);
    VOID                    (*cleanup_fn)(VOID);
    STATUS                  status;

    /* Initialize local variables. */
    spi_inst_hdl_ptr    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    tgt_ptr             = (LWSPI_TGT *)spi_inst_hdl_ptr->spi_tgt_ptr;
    status              = NU_SUCCESS;

    /* Enable SPI only if already OFF. */
    if (((*state == SPI_ON) || (*state == POWER_ON_STATE)) && (PMI_STATE_GET(spi_inst_hdl_ptr->pmi_dev) == SPI_OFF))
    {
        if (spi_inst_hdl_ptr->device_in_use == NU_TRUE)
        {
            /* Call the target specific setup function. */
            setup_fn = tgt_ptr->setup_func;
            if(setup_fn != NU_NULL)
            {
                setup_fn();
            }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* First ensure that DVFS is at min op before SPI is enabled. */
            status = PMI_REQUEST_MIN_OP(SPI_MIN_DVFS_OP, spi_inst_hdl_ptr->pmi_dev);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

            if (status == NU_SUCCESS)
            {
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                /* Update MPL for DVFS. */
                (VOID)PMI_DVFS_Update_MPL_Value(spi_inst_hdl_ptr, PM_NOTIFY_ON);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

                /* Enable device. */
                LWSPI_TGT_Device_Enable(spi_inst_hdl_ptr);
             }
        }

        /* Update power state. */
        PMI_STATE_SET(spi_inst_hdl_ptr->pmi_dev, *state);

        /* Set ON flag in event group. */
        PMI_CHANGE_STATE_WAIT_CYCLE(spi_inst_hdl_ptr->pmi_dev, PMI_ON_EVT, status);

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
        if (spi_inst_hdl_ptr->device_in_use == NU_TRUE)
        {
            /* Enable all SPI interrupts. */
            LWSPI_TGT_Intr_Enable (spi_inst_hdl_ptr);
        }
#endif
    }

    /* Disable SPI only if already ON. */
    if ((*state == SPI_OFF) && ((PMI_STATE_GET(spi_inst_hdl_ptr->pmi_dev) == SPI_ON) ||
                                (PMI_STATE_GET(spi_inst_hdl_ptr->pmi_dev) == POWER_ON_STATE)))
    {
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
        /* Disable all SPI interrupts. */
        LWSPI_TGT_Intr_Disable (spi_inst_hdl_ptr);
#endif

        /* Update the state of the device in the State Information structure. */
        PMI_STATE_SET(spi_inst_hdl_ptr->pmi_dev, SPI_OFF);

        /* Clear if ON event was set. */
        PMI_CHANGE_STATE_WAIT_CYCLE(spi_inst_hdl_ptr->pmi_dev, PMI_OFF_EVT, status);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Release min request for DVFS OP. */
        (VOID)PMI_RELEASE_MIN_OP(spi_inst_hdl_ptr->pmi_dev);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

        /* Disable device. */
        LWSPI_TGT_Device_Disable(spi_inst_hdl_ptr);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Update MPL for DVFS. */
        (VOID)PMI_DVFS_Update_MPL_Value(spi_inst_hdl_ptr, PM_NOTIFY_OFF);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

        /* Call the target specific cleanup function. */
        cleanup_fn = tgt_ptr->cleanup_func;
        if(cleanup_fn != NU_NULL)
        {
            cleanup_fn();
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_TGT_Pwr_Get_Clock_Rate
*
*   DESCRIPTION
*
*       The function to get clock rate for SPI baud rate calculation.
*
*   INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*       clock_rate                          The output clock rate.
*
*   OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Get_Clock_Rate(LWSPI_INSTANCE_HANDLE *spi_inst_ptr,
                              UINT32                *clock_rate)
{
    STATUS    status;
    UINT8     op_id;
    UINT32    ref_freq;

    /* Get reference clock */
    status = CPU_Get_Device_Frequency(&op_id, spi_inst_ptr->ref_clock, &ref_freq);
    /* Check if API call was successful. */
    if (status == NU_SUCCESS)
    {
        /* Retrieve MCK, as SPI baud rate is based on MCK frequency. */
        *clock_rate = ref_freq;
    }
    else
    {
        *clock_rate = spi_inst_ptr->clock;
    }

    return (status);
}

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))
/*************************************************************************
*
*    FUNCTION
*
*        LWSPI_TGT_Pwr_Pre_Park
*
*    DESCRIPTION
*
*        This function is called before park event.
*
*    INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*    OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Pre_Park(VOID *spi_inst_ptr)
{
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Disable interrupts. */
    LWSPI_TGT_Intr_Disable ((LWSPI_INSTANCE_HANDLE *)spi_inst_ptr);
#endif
    return (NU_SUCCESS);
}

/*************************************************************************
*
*    FUNCTION
*
*        LWSPI_TGT_Pwr_Post_Park
*
*    DESCRIPTION
*
*        This function is called before park event.
*
*    INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*    OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Post_Park(VOID *spi_inst_ptr)
{
    /* Wait if device is busy. */
    while(LWSPI_TGT_Check_Device_Busy(spi_inst_ptr));

    /* Disable device. */
    LWSPI_TGT_Device_Disable(spi_inst_ptr);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*    FUNCTION
*
*        LWSPI_TGT_Pwr_Pre_Resume
*
*    DESCRIPTION
*
*        This function is called before park event.
*
*    INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*    OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Pre_Resume(VOID *spi_inst_ptr)
{
    LWSPI_INSTANCE_HANDLE     *spi_inst_handle;
    LWSPI_TGT                 *tgt_ptr;
    STATUS                     status = NU_SUCCESS;

    /* Initialize local variables. */
    spi_inst_handle    = (LWSPI_INSTANCE_HANDLE *)spi_inst_ptr;
    tgt_ptr            = (LWSPI_TGT *)spi_inst_handle->spi_tgt_ptr;
    status             = ~NU_SUCCESS;

/* TODO: Figure out how to access baudrate from tgt_ptr.  Parameters are defined
 * at high level but needs to be passed down to low level.
 */
#if 0
    /* Reset baud rate as system clock may have been changed. */
    if(tgt_ptr->baud_rate != 0)
    {
        /* SSE bit needs to be disabled before changing configuration register. */
        LWSPI_TGT_Device_Disable(spi_inst_ptr);

        status = LWSPI_TGT_Set_Baud_Rate(spi_inst_handle, 
                        spi_inst_handle->curr_write_irp->device->baud_rate);
        /* Set SSE bit to enable the device again. */
        LWSPI_TGT_Device_Enable(spi_inst_ptr);
    }
#endif

    return (status);
}

/*************************************************************************
*
*    FUNCTION
*
*        LWSPI_TGT_Pwr_Post_Resume
*
*    DESCRIPTION
*
*        This function is called before park event.
*
*    INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*    OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Post_Resume(VOID *spi_inst_ptr)
{
    /* Enable device. */
    LWSPI_TGT_Device_Enable((LWSPI_INSTANCE_HANDLE *)spi_inst_ptr);

    /* Update MPL for DVFS after frequency change. */
    (VOID)PMI_DVFS_Update_MPL_Value(spi_inst_ptr, PM_NOTIFY_ON);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*    FUNCTION
*
*        LWSPI_TGT_Pwr_Resume_End
*
*    DESCRIPTION
*
*        This function is called before park event.
*
*    INPUT
*
*       spi_inst_ptr                        Pointer to SPI device instance.
*
*    OUTPUT
*
*        NU_SUCCESS                         Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_TGT_Pwr_Resume_End(VOID *spi_inst_ptr)
{
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    /* Enable interrupts. */
    LWSPI_TGT_Intr_Enable (spi_inst_ptr);
#endif

    return (NU_SUCCESS);
}

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

