/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   DESCRIPTION
*
*       This file contains generic interface specifications for Power -
*       aware device drivers
*
*************************************************************************/

/**********************************/
/* INCLUDE FILES                  */
/**********************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/cpu_interface.h"
#include "services/reg_api.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
#include "services/power_core.h"
#endif

extern STATUS CPU_Tgt_Get_Frequency(UINT32 *master_freq, CHAR *identifer);

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Get_Device_Frequency
*
*   DESCRIPTION
*
*       This function retreives the reference frequency for the device
*
*   INPUTS
*
*       *op_id
*       *ref_clock
*       *ref_freq
*
*   OUTPUTS
*
*       status 
*
*************************************************************************/
STATUS CPU_Get_Device_Frequency(UINT8 *op_id, CHAR *ref_clock, UINT32 *ref_freq)
{
    STATUS status = -1; 
    
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))
    
    STATUS pm_status;
    pm_status = NU_PM_Get_Current_OP(op_id);

    if (pm_status == NU_SUCCESS)
    {
        /* Get additional info */
        pm_status = NU_PM_Get_OP_Specific_Info(*op_id, ref_clock, ref_freq, sizeof(*ref_freq));
        
        if (pm_status == NU_SUCCESS)
        {
            status = NU_SUCCESS;
        }
    }

#else  
    
    /* Call the CPU function to get the constant ref_freq value */
    status = CPU_Tgt_Get_Frequency(ref_freq, ref_clock);
    
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE && CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE */

    /* Suppress warnings */
    NU_UNUSED_PARAM(op_id);

    return (status);
}
