/***********************************************************************
*
*             Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       arm_vectors.c
*
*   DESCRIPTION
*
*       This file contains the core ARMv7-m architecture vector table
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"

/* External Function Prototypes */
extern  VOID ESAL_Entry(VOID);
extern  VOID ESAL_AR_ISR_Exception_Handler(VOID);
extern  VOID ESAL_AR_ISR_SVC_Handler(VOID);
extern  VOID ESAL_AR_ISR_Interrupt_Handler(VOID);

/* Vector table */
const VOID (*ESAL_AR_ISR_Vector_Table[CFG_NU_OS_ARCH_ARMV7_M_COM_MAX_VECTOR])(VOID) __attribute__ ((section ("esal_vectors"))) =
{
    [0] = (VOID *)CFG_NU_OS_ARCH_ARMV7_M_COM_RESET_SP,                                      /* Reset Stack Pointer  */
    [1] = (VOID *)ESAL_Entry,                                                               /* Reset Entry Address  */
    [2] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                            /* NMI                  */
    [3] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                            /* Hard Fault           */
    [4] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                            /* Memory Manage        */
    [5] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                            /* Bus Fault            */
    [6] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                            /* Usage Fault          */
    [11] = (VOID *)ESAL_AR_ISR_SVC_Handler,                                                 /* SV Call              */
    [12] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                           /* Debug Monitor        */
    [14] = (VOID *)ESAL_AR_ISR_Exception_Handler,                                           /* Pend SV              */
    [15] = (VOID *)ESAL_AR_ISR_Interrupt_Handler,                                           /* SysTick              */
    [16 ... (CFG_NU_OS_ARCH_ARMV7_M_COM_MAX_VECTOR-1)] = (VOID *)ESAL_AR_ISR_Interrupt_Handler  /* Interrupts           */
};
