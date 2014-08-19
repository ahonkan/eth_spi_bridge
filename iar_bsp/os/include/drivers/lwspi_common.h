/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       lwspi_common.h
*
*   COMPONENT
*
*       Lightweight SPI generic driver.
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the lightweight SPI driver module.
*
*************************************************************************/
#ifndef LWSPI_COMMON_H
#define LWSPI_COMMON_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Definitions for LWSPI ISR handling */
#define LWSPI_HISR_STACK_SIZE           512
#define LWSPI_HISR_PRIORITY             2

/*  The size of the ISR queue is 2x the number of SPI buses to
    allow both TX and RX data to be put in the queue for each bus */
#define LWSPI_ISR_QUEUE_SIZE            (LWSPI_NUM_BUSES * 2)
#define LWSPI_ISR_QUEUE_FULL            1
#define LWSPI_ISR_QUEUE_EMPTY           2
#define LWSPI_ISR_QUEUE_DATA            3

/* Function prototypes */
STATUS  LWSPI_Get_Target_Info(const CHAR*, LWSPI_INSTANCE_HANDLE*);
STATUS  LWSPI_Set_Reg_Path(const CHAR *, LWSPI_INSTANCE_HANDLE*);
STATUS  LWSPI_PR_Intr_Enable(LWSPI_SESSION_HANDLE*);
STATUS  LWSPI_ISR_Data_Set(LWSPI_INSTANCE_HANDLE *);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
VOID    LWSPI_Check_Power_State(VOID*);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !LWSPI_COMMON_H */
