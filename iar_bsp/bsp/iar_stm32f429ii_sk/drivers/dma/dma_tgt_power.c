/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
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
*       dma_tgt_power.c
*
*   COMPONENT
*
*       EDMA3                           - EDMA3 controller driver
*
*   DESCRIPTION
*
*       This file contains the EDMA3 Driver specific functions.
*
*   FUNCTIONS
*
*       DMA_Tgt_Pwr_Default_State
*       DMA_Tgt_Pwr_Set_State
*       DMA_Tgt_Pwr_Notify_Park1
*       DMA_Tgt_Pwr_Notify_Park2
*       DMA_Tgt_Pwr_Notify_Resume1
*       DMA_Tgt_Pwr_Notify_Resume2
*       DMA_Tgt_Pwr_Notify_Resume3
*       DMA_Tgt_Pwr_Calc_Baud_Divisor
*       DMA_Tgt_Pwr_Min_OP_Pt_Calc
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h                                             
*       nu_services.h
*       nu_drivers.h
*       dma_common.h
*       dma_tgt.h
*       dma_tgt_power.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "drivers/dma_common.h"
#include "bsp/drivers/dma/dma_tgt.h"
#include "bsp/drivers/dma/dma_tgt_power.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Default_State
*
*   DESCRIPTION
*
*       This function places EDMA in known, low-power state
*
*   INPUTS
*
*       DMA_INSTANCE_HANDLE  *inst_handle - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DMA_Tgt_Pwr_Default_State (DMA_INSTANCE_HANDLE *inst_handle)
{
    /* Supress harmless warnings */
    NU_UNUSED_PARAM(inst_handle);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This function sets the state of the EDMA
*
*   INPUT
*
*       VOID           *instance_handle    - Instance handle
*       PM_STATE_ID    *state              - Power state
*
*   OUTPUT
*
*       STATUS         pm_status           - NU_SUCCESS 
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state)
{
    INT                    int_level;
    STATUS                 pm_status = NU_SUCCESS;
    STATUS                 status = NU_SUCCESS;
    DMA_INSTANCE_HANDLE    *inst_handle = (DMA_INSTANCE_HANDLE *)instance_handle;

#if 0
    PMI_DEV_HANDLE         pmi_dev_ptr = inst_handle->pmi_dev;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

    UINT8     op_pt = 0;

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

    /* Enable DMA only if already OFF */
    if (((*state == DMA_ON) || (*state == POWER_ON_STATE)) && (PMI_STATE_GET(pmi_dev_ptr) == DMA_OFF))
    {
        if ((inst_handle->device_in_use == NU_TRUE) && (PMI_IS_PARKED(pmi_dev_ptr) == NU_FALSE))
        {

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* Determine the min op pt for the device's baud rate */
            status = DMA_Tgt_Pwr_Min_OP_Pt_Calc(inst_handle, &op_pt);

            if(status == NU_SUCCESS)
            {
                /* First ensure that DVFS is at min op before USART is enabled */
                pm_status = PMI_REQUEST_MIN_OP(op_pt, pmi_dev_ptr);
            }

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

            if ((pm_status == NU_SUCCESS) && (status == NU_SUCCESS))
            {
                /* Disable interrupts for critical section */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                /* Configure and enable baud rate */
                DMA_Tgt_Setup (inst_handle);

                /* Enable DMA */
                DMA_Tgt_Enable(inst_handle);                

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                /* Update MPL for USART */
                (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev_ptr, PM_NOTIFY_ON);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

                /* Restore interrupts to original level. */
                (VOID)NU_Local_Control_Interrupts(int_level);
            }
        }

        /* Update the state of the device */
        PMI_STATE_SET(pmi_dev_ptr, DMA_ON);

        /* Set ON flag in event group */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_ON_EVT, status);
    }

    /* Disable USART only if already ON */
    if ((*state == DMA_OFF) && (PMI_STATE_GET(pmi_dev_ptr) == DMA_ON))
    {
        /* Update the state of the device in the State Information structure */
        PMI_STATE_SET(pmi_dev_ptr, DMA_OFF);

        /* Clear if ON event was set */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_OFF_EVT, status);

        /* Disable the DMA */
        DMA_Tgt_Disable(inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Update MPL for DVFS */
        (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev_ptr, PM_NOTIFY_OFF);

        /* Release min request for DVFS OP */
        (VOID)PMI_RELEASE_MIN_OP(pmi_dev_ptr);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }
#endif
    return (pm_status);
}

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Notify_Park1
*
*   DESCRIPTION
*
*       This function will notify the EDMA when to park 
*
*   INPUT
*
*       VOID             *instance_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS                               - NU_SUCCESS
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Notify_Park1(VOID *instance_handle)
{
    DMA_INSTANCE_HANDLE    *inst_handle = (DMA_INSTANCE_HANDLE *) instance_handle;
    DMA_TGT_HANDLE         *tgt_ptr     = (DMA_TGT_HANDLE      *) inst_handle->dma_tgt_handle;

#if 0
    UINT32                 base_address = inst_handle -> dma_io_addr + tgt_ptr -> shadow_region_offset_address;
    
    /* Disable interrupts */
    ESAL_GE_MEM_WRITE32(base_address + EDMA_IECR, 0);

#if (EDMA_MAX_CHANNELS == 64)

    /* Disable interrupts */
    ESAL_GE_MEM_WRITE32(base_address + EDMA_IECRH, 0);    
#endif 

#endif

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Notify_Park2
*
*   DESCRIPTION
*
*       This function will notify the EDMA when to park 
*
*   INPUT
*
*       VOID             *instance_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS                               - NU_SUCCESS 
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Notify_Park2(VOID *instance_handle)
{
    /* Supress harmless warnings */
    NU_UNUSED_PARAM(instance_handle);
    
    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Notify_Resume1
*
*   DESCRIPTION
*
*       This function will notify the EDMA when to resume
*
*   INPUT
*
*       VOID            *instance_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS                              - NU_SUCCESS
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Notify_Resume1(VOID *instance_handle)
{
    /* Supress harmless warnings */
    NU_UNUSED_PARAM(instance_handle);
    
    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Notify_Resume2
*
*   DESCRIPTION
*
*       This function will notify the EDMA when to resume
*
*   INPUT
*
*       VOID            *instance_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS                              - NU_SUCCESS
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Notify_Resume2(VOID *instance_handle)
{
    /* Supress harmless warnings */
    NU_UNUSED_PARAM(instance_handle);
    
    /* Return success*/
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Notify_Resume3
*
*   DESCRIPTION
*
*       This function will notify the EDMA when to resume
*
*   INPUT
*
*       VOID             *instance_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS                               - NU_SUCCESS
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Notify_Resume3(VOID *instance_handle)
{
    DMA_INSTANCE_HANDLE    *inst_handle = (DMA_INSTANCE_HANDLE *) instance_handle;
    DMA_TGT_HANDLE         *tgt_ptr     = (DMA_TGT_HANDLE      *) inst_handle->dma_tgt_handle;

#if 0
    UINT32                 base_address = inst_handle -> dma_io_addr + tgt_ptr -> shadow_region_offset_address;

    /* Disable interrupts */
    ESAL_GE_MEM_WRITE32(base_address + EDMA_IESR, 0xFFFFFFFF);
    
#if (EDMA_MAX_CHANNELS == 64)
    
    /* Disable interrupts */
    ESAL_GE_MEM_WRITE32(base_address + EDMA_IESRH, 0xFFFFFFFF);    
#endif 

#endif

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Pwr_Min_OP_Pt_Calc
*
*   DESCRIPTION
*
*       Computes the minimum DVFS operating point required for
*       proper operation at the given baud-rate
*
*   INPUTS
*
*       DMA_INSTANCE_HANDLE *inst_handle     - Instance handle pointer
*       UINT8*  min_op_pt                    - Min Op Pt returned here
*
*   OUTPUTS
*
*       STATUS  status                       - NU_SUCCESS if min op pt found
*                                            - SERIAL_ERROR if min op pt not found
*
*************************************************************************/
STATUS DMA_Tgt_Pwr_Min_OP_Pt_Calc (DMA_INSTANCE_HANDLE *inst_handle, UINT8* min_op_pt)
{
    STATUS    pm_status;
    STATUS    status = ~NU_SUCCESS;
    UINT8     max_op_pt_cnt, highest_op_pt;
    INT       curr_op_pt;
    UINT32    ref_freq;

#if 0
    /* Get the maximum number of operating points */
    pm_status = NU_PM_Get_OP_Count (&max_op_pt_cnt);

    if(pm_status == NU_SUCCESS)
    {
        /* The highest op pt is one less than the maximum number of op pts */
        highest_op_pt = max_op_pt_cnt - 1;

        /* Loop from highest op pt to lowest */
        for(curr_op_pt=highest_op_pt; curr_op_pt>=0; curr_op_pt--)
        {
            /* Get reference clock */
            pm_status = NU_PM_Get_OP_Specific_Info(curr_op_pt, inst_handle->dma_ref_clock,
                                                   &ref_freq, sizeof(ref_freq));

            if (pm_status == NU_SUCCESS)
            {
                /* Update min op pt */
                *min_op_pt = curr_op_pt;

                status = NU_SUCCESS;
            }
        }
    }
#endif
    return status;
}
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
