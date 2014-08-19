/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains private functions definitions of PMS tick 
*       suppression subcomponent.
*
***********************************************************************/
#ifndef PMS_TICK_SUPPRESSION_H
#define PMS_TICK_SUPPRESSION_H

#ifdef __cplusplus
extern "C" {
#endif

UINT32      PMS_TS_Get_Suppressed_Ticks_Passed(UINT32 last_hw_counter_value, 
                                               UINT32 elapsed_time, 
                                               BOOLEAN this_is_a_tick_flag,
                                               UINT32 single_tick);
VOID        PMS_TS_Adjust_For_Suppressed_Ticks(UINT32 suppressed_ticks_passed);
VOID        PMS_TS_Suppress_Ticks(UINT32 ticks_to_suppress,UINT32 single_tick);
UINT32      PMS_TS_Unsuppress_Ticks(UINT32 current_hw_timer_value,UINT32 single_tick);
UINT32      PMS_TS_Get_Hw_Counter_Timestamp(BOOLEAN hw_counter_was_reset_flag_ptr, UINT32 tick_interval);
VOID        PMS_TS_Wakeup(INT vector_id);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_TICK_SUPPRESSION_H */


