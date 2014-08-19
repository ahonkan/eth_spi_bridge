/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       plus_supplement_exp.c
*
*   COMPONENT
*
*       Nuclues Plus supplement symbols API export for Nucleus Processes
*
*   DESCRIPTION
*
*       Export symbols for Plus Core component.  Each section is broken
*       into 2 categories, API available for both kernel and user process
*       use and API available only to kernel processes.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/nu_kernel.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_KERN_PLUS_SUPPLEMENT);

/****************************************/
/* Export Mailbox management functions. */
/****************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Mailbox);
NU_EXPORT_SYMBOL (NU_Delete_Mailbox);
NU_EXPORT_SYMBOL (NU_Reset_Mailbox);
NU_EXPORT_SYMBOL (NU_Send_To_Mailbox);
NU_EXPORT_SYMBOL (NU_Broadcast_To_Mailbox);
NU_EXPORT_SYMBOL (NU_Receive_From_Mailbox);
NU_EXPORT_SYMBOL (NU_Mailbox_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Mailboxes);
NU_EXPORT_KSYMBOL (NU_Mailbox_Pointers);

/*************************************/
/* Export Pipe management functions. */
/*************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Pipe);
NU_EXPORT_SYMBOL (NU_Delete_Pipe);
NU_EXPORT_SYMBOL (NU_Reset_Pipe);
NU_EXPORT_SYMBOL (NU_Send_To_Front_Of_Pipe);
NU_EXPORT_SYMBOL (NU_Send_To_Pipe);
NU_EXPORT_SYMBOL (NU_Broadcast_To_Pipe);
NU_EXPORT_SYMBOL (NU_Receive_From_Pipe);
NU_EXPORT_SYMBOL (NU_Pipe_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Pipes);
NU_EXPORT_KSYMBOL (NU_Pipe_Pointers);

/***************************************/
/* Export Signal processing functions. */
/***************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Control_Signals);
NU_EXPORT_SYMBOL (NU_Receive_Signals);
NU_EXPORT_SYMBOL (NU_Register_Signal_Handler);
NU_EXPORT_SYMBOL (NU_Send_Signals);

/* There are currently no kernel exported
   symbols for signal processing */

/*************************************************/
/* Export Partition memory management functions. */
/*************************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Partition_Pool);
NU_EXPORT_SYMBOL (NU_Delete_Partition_Pool);
NU_EXPORT_SYMBOL (NU_Allocate_Partition);
NU_EXPORT_SYMBOL (NU_Deallocate_Partition);
NU_EXPORT_SYMBOL (NU_Partition_Pool_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Partition_Pools);
NU_EXPORT_KSYMBOL (NU_Partition_Pool_Pointers);

/*****************************************/
/* Export Development support functions. */
/*****************************************/

NU_EXPORT_SYMBOL (NU_Get_Release_Version);

#endif /* CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EXPORT_SYMBOLS == NU_TRUE */



