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
*       plus_core_exp.c
*
*   COMPONENT
*
*       Nuclues Plus core symbols export API for Nucleus Processes
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

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PLUS_CORE_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/nu_kernel.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_KERN_PLUS_CORE);

/**********************************/
/* Export task control functions. */
/**********************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Task);
NU_EXPORT_SYMBOL (NU_Delete_Task);
NU_EXPORT_SYMBOL (NU_Reset_Task);
NU_EXPORT_SYMBOL (NU_Terminate_Task);
NU_EXPORT_SYMBOL (NU_Resume_Task);
NU_EXPORT_SYMBOL (NU_Suspend_Task);
NU_EXPORT_SYMBOL (NU_Relinquish);
NU_EXPORT_SYMBOL (NU_Sleep);
NU_EXPORT_SYMBOL (NU_Change_Priority);
NU_EXPORT_SYMBOL (NU_Change_Time_Slice);
NU_EXPORT_SYMBOL (NU_Create_Auto_Clean_Task);
NU_EXPORT_SYMBOL (NU_Current_Task_Pointer);
NU_EXPORT_SYMBOL (NU_Task_Information);
#if (NU_STACK_CHECKING == NU_TRUE)
NU_EXPORT_SYMBOL (NU_Check_Stack);
#endif /* NU_STACK_CHECKING == NU_TRUE */

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Change_Preemption);
NU_EXPORT_KSYMBOL (NU_Established_Tasks);
NU_EXPORT_KSYMBOL (NU_Task_Pointers);

/**************************************/
/* Export Queue management functions. */
/**************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Queue);
NU_EXPORT_SYMBOL (NU_Delete_Queue);
NU_EXPORT_SYMBOL (NU_Reset_Queue);
NU_EXPORT_SYMBOL (NU_Send_To_Front_Of_Queue);
NU_EXPORT_SYMBOL (NU_Send_To_Queue);
NU_EXPORT_SYMBOL (NU_Broadcast_To_Queue);
NU_EXPORT_SYMBOL (NU_Receive_From_Queue);
NU_EXPORT_SYMBOL (NU_Queue_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Queues);
NU_EXPORT_KSYMBOL (NU_Queue_Pointers);

/******************************************/
/* Export Semaphore management functions. */
/******************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Semaphore);
NU_EXPORT_SYMBOL (NU_Delete_Semaphore);
NU_EXPORT_SYMBOL (NU_Reset_Semaphore);
NU_EXPORT_SYMBOL (NU_Obtain_Semaphore);
NU_EXPORT_SYMBOL (NU_Release_Semaphore);
NU_EXPORT_SYMBOL (NU_Get_Semaphore_Owner);
NU_EXPORT_SYMBOL (NU_Semaphore_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Semaphores);
NU_EXPORT_KSYMBOL (NU_Semaphore_Pointers);

/********************************************/
/* Export Event Group management functions. */
/********************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Event_Group);
NU_EXPORT_SYMBOL (NU_Delete_Event_Group);
NU_EXPORT_SYMBOL (NU_Set_Events);
NU_EXPORT_SYMBOL (NU_Retrieve_Events);
NU_EXPORT_SYMBOL (NU_Event_Group_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Event_Groups);
NU_EXPORT_KSYMBOL (NU_Event_Group_Pointers);

/***********************************************/
/* Export Dynamic memory management functions. */
/***********************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Create_Memory_Pool);
NU_EXPORT_SYMBOL (NU_Delete_Memory_Pool);
NU_EXPORT_SYMBOL (NU_Add_Memory);
NU_EXPORT_SYMBOL (NU_Deallocate_Memory);
NU_EXPORT_SYMBOL (NU_Reallocate_Aligned_Memory);
NU_EXPORT_SYMBOL (NU_Allocate_Aligned_Memory);
NU_EXPORT_SYMBOL (NU_System_Memory_Get);
NU_EXPORT_SYMBOL (NU_Memory_Pool_Information);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Established_Memory_Pools);
NU_EXPORT_KSYMBOL (NU_Memory_Pool_Pointers);

/******************************************/
/* Export Interrupt management functions. */
/******************************************/

/* Interrupt management functions are only
   available to kernel mode processes */
NU_EXPORT_KSYMBOL (NU_Control_Interrupts);
NU_EXPORT_KSYMBOL (ESAL_GE_INT_Global_Set);
NU_EXPORT_KSYMBOL (NU_Register_LISR);
NU_EXPORT_KSYMBOL (NU_Activate_HISR);
NU_EXPORT_KSYMBOL (NU_Create_HISR);
NU_EXPORT_KSYMBOL (NU_Delete_HISR);
NU_EXPORT_KSYMBOL (NU_Current_HISR_Pointer);
NU_EXPORT_KSYMBOL (NU_Established_HISRs);
NU_EXPORT_KSYMBOL (NU_HISR_Pointers);
NU_EXPORT_KSYMBOL (NU_HISR_Information);

/**************************************/
/* Export Timer management functions. */
/**************************************/

/* User exported symbols */
NU_EXPORT_SYMBOL (NU_Retrieve_Clock);
NU_EXPORT_SYMBOL (NU_Retrieve_Clock64);
NU_EXPORT_SYMBOL (NU_Ticks_To_Time);
NU_EXPORT_SYMBOL (NU_Time_To_Ticks);

/* Kernel exported symbols */
NU_EXPORT_KSYMBOL (NU_Create_Timer);
NU_EXPORT_KSYMBOL (NU_Delete_Timer);
NU_EXPORT_KSYMBOL (NU_Control_Timer);
NU_EXPORT_KSYMBOL (NU_Reset_Timer);
NU_EXPORT_KSYMBOL (NU_Established_Timers);
NU_EXPORT_KSYMBOL (NU_Timer_Pointers);
NU_EXPORT_KSYMBOL (NU_Timer_Information);
NU_EXPORT_KSYMBOL (NU_Get_Remaining_Time);
NU_EXPORT_KSYMBOL (NU_Pause_Timer);
NU_EXPORT_KSYMBOL (NU_Resume_Timer);
NU_EXPORT_KSYMBOL (NU_Set_Clock64);

#endif /* CFG_NU_OS_KERN_PLUS_CORE_EXPORT_SYMBOLS == NU_TRUE */
