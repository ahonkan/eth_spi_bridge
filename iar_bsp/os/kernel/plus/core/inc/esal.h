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
*       This file provides the external interface to the Embedded
*       Software Abstraction Layer components
*
***********************************************************************/

#ifndef             ESAL_H
#define             ESAL_H

#ifdef              __cplusplus

/* C declarations in C++     */
extern              "C" {
#endif

/* Include configuration header files */
#include <string.h>

/* Define all memory related function prototypes */
VOID                *ESAL_GE_MEM_Initialize(VOID);
VOID                *ESAL_CO_MEM_Cache_Enable(VOID *avail_mem);
VOID                ESAL_PR_MEM_Initialize(VOID);
VOID                *ESAL_PR_MEM_Cache_Enable(VOID *avail_mem);
/* Define all debugging related function prototypes */
VOID                ESAL_GE_DBG_Initialize(VOID **(*breakpoint_handler)(VOID *), VOID **(*hardware_step_handler)(VOID *), VOID **(*data_abort_handler)(VOID *));
VOID                ESAL_AR_DBG_Initialize(VOID);

/**********************************************************************************/
/*                  Interrupt Service                                             */
/**********************************************************************************/

/* Define all externally accessible, interrupt related function prototypes */
VOID                ESAL_GE_ISR_Initialize(VOID (*default_isr)(INT),
                                           VOID (*default_except)(INT, VOID *),
                                           VOID **(*os_isr_entry)(INT, VOID *),
                                           VOID (*os_nested_isr_entry)(INT),
                                           BOOLEAN hibernate_wake);
VOID                ESAL_AR_ISR_Initialize(VOID);
VOID                ESAL_AR_ISR_Vector_Table_Install(VOID);
VOID                ESAL_DP_ISR_Initialize(VOID);
VOID                ESAL_PR_ISR_Initialize(VOID);


/**********************************************************************************/
/*                  Run-time Environment                                          */
/**********************************************************************************/

/* Map generic APIs to lower-level component */
#define             ESAL_GE_RTE_Cxx_System_Objects_Initialize   ESAL_TS_RTE_Cxx_System_Objects_Initialize
#define             ESAL_GE_RTE_Cxx_Exceptions_Initialize       ESAL_TS_RTE_Cxx_Exceptions_Initialize
#define             ESAL_GE_RTL_Initialize                      ESAL_TS_RTE_Initialize

/* Define all run-time environment related function prototypes */
VOID                ESAL_GE_RTE_Initialize(INT (*os_write)(INT), INT (*os_read)(VOID));
VOID                ESAL_TS_RTE_Initialize(VOID);
VOID                ESAL_TS_RTE_Lowlevel_Initialize(VOID);
VOID                ESAL_TS_RTE_Initialize(VOID);
VOID                ESAL_TS_RTE_Cxx_Region_Objects_Initialize(VOID *   region_start,
                                                              VOID *   region_end);
VOID                ESAL_TS_RTE_Cxx_System_Objects_Initialize(VOID);
VOID                ESAL_TS_RTE_Cxx_Exceptions_Initialize(VOID);

/**********************************************************************************/
/*                  Stack                                                         */
/**********************************************************************************/
/* Define all externally accessible, stack related function prototypes */
VOID                ESAL_GE_STK_Initialize(VOID (*unsol_stk_switch_entry)(VOID));


#ifdef              __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif      /* ESAL_H */

