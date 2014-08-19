/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
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
*       inct_common.c
*
*   COMPONENT
*
*       IN - Initialization
*
*   DESCRIPTION
*
*       This file contains initialization and setup routines with
*       target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       OS_Init_Entry                       Common target dependent
*                                           initialization
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       esal.h                              ESAL Internal constants
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/esal.h"
#include        "kernel/proc_extern.h"

/* External function prototypes */
extern          VOID            INC_Initialize(VOID *);
extern          VOID            TCC_Unhandled_Interrupt(INT);
extern          VOID            TCC_Unhandled_Exception(INT, VOID *);
extern          VOID            **TCCT_Dispatch_LISR(INT, VOID *);
extern          VOID            TCCT_Dispatch_Nested_LISR(INT);
extern          VOID            TCCT_Schedule(VOID);

#ifdef CFG_NU_OS_SVCS_PWR_CORE_ENABLE
    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern          VOID            PMS_Hibernate_Initialize(VOID);
extern          BOOLEAN         PMS_Hibernate_Check(VOID);

    #endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE */

#ifdef  CFG_NU_OS_DRVR_SERIAL_ENABLE
extern          INT             NU_SIO_Putchar(INT c);
extern          INT             NU_SIO_Getchar(VOID);
#endif

static BOOLEAN INCT_Hibernate_Wake;

/***********************************************************************
*
*   FUNCTION
*
*       OS_Init_Entry
*
*   DESCRIPTION
*
*       All required target components are initialized by this function.
*       After all initialization is complete, this function calls
*       INC_Initialize to perform all Nucleus PLUS system initialization.
*
*   CALLED BY
*
*       ESAL_TS_RTE_Lowlevel_Initialize
*
*   CALLS
*
*       ESAL_GE_INT_All_Disable             Disables all target interrupts
*       ESAL_GE_ISR_Initialize              Initialize target interrupt
*                                           service routine components
*       ESAL_GE_MEM_Initialize              Initialize target memory
*                                           components
*       ESAL_GE_RTE_Initialize              Initialize run-time
*                                           environment components
*       ESAL_GE_STK_Initialize              Initialize target stack
*                                           components
*       ESAL_GE_TMR_OS_Timer_Start          Initialize target OS timer
*                                           component
*       INC_Initialize                      Nucleus PLUS System init
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  OS_Init_Entry(VOID)
{
    VOID *first_avail_mem;
    INT  (*rte_putchar)(INT)  = NU_NULL;
    INT  (*rte_getchar)(VOID) = NU_NULL;
    
#if (CFG_NU_OS_KERN_PLUS_CORE_EXPORT_SYMBOLS == NU_TRUE)
    /* Keep symbols for nu.os.kern.plus.core */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_KERN_PLUS_CORE);
#endif /* CFG_NU_OS_KERN_PLUS_CORE_EXPORT_SYMBOLS */

#if (CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EXPORT_SYMBOLS == NU_TRUE)
    /* Keep symbols for nu.os.kern.plus.supplement */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_KERN_PLUS_SUPPLEMENT);
#endif /* CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EXPORT_SYMBOLS */

#if (CFG_NU_OS_SVCS_REGISTRY_EXPORT_SYMBOLS == NU_TRUE)
    /* Keep symbols for nu.os.svcs.registry */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_SVCS_REGISTRY);
#endif /* CFG_NU_OS_SVCS_REGISTRY_EXPORT_SYMBOLS */

    INCT_Hibernate_Wake = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_CORE_ENABLE
    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

    INCT_Hibernate_Wake = PMS_Hibernate_Check();

    #endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE */

    /* Disable all architecture interrupts */
    ESAL_GE_INT_FAST_ALL_DISABLE();


#ifdef  CFG_NU_OS_DRVR_SERIAL_ENABLE
    #ifdef CFG_NU_OS_SVCS_DBG_ENABLE
        #if (CFG_NU_OS_SVCS_DBG_COM_PORT_TYPE != 1)
        rte_putchar = NU_SIO_Putchar;
        rte_getchar = NU_SIO_Getchar;
        #endif
    #else
        rte_putchar = NU_SIO_Putchar;
        rte_getchar = NU_SIO_Getchar;
    #endif
#endif

    if (INCT_Hibernate_Wake == NU_FALSE)
    {
        /* Initialize target run-time environment components */
        ESAL_GE_RTE_Initialize(rte_putchar, rte_getchar);

        /* Initialize target stack components */
        ESAL_GE_STK_Initialize(TCCT_Schedule);

#if (NU_STACK_FILL == NU_TRUE)

        /* Fill system stack with pattern before switching to system stack */
        ESAL_GE_MEM_Set(ESAL_GE_STK_System_SP_Start_Get(),
                        NU_STACK_FILL_PATTERN,
                        ESAL_GE_STK_SYSTEM_SIZE);

#endif  /* NU_STACK_FILL == NU_TRUE */

        /* Switch to the target system stack */
        ESAL_GE_STK_SYSTEM_SP_SET();
    }

    /* Initialize target memory components */
    first_avail_mem = ESAL_GE_MEM_Initialize();

    /* Initialize target interrupt service routine components */
    ESAL_GE_ISR_Initialize(TCC_Unhandled_Interrupt,
                           TCC_Unhandled_Exception,
                           TCCT_Dispatch_LISR,
                           TCCT_Dispatch_Nested_LISR,
                           INCT_Hibernate_Wake);

    /* Disable all target interrupt sources */
    ESAL_GE_INT_All_Disable();

    /* Initialize target OS timer component */
    ESAL_GE_TMR_OS_Timer_Start(NU_PLUS_TICKS_PER_SEC);

#ifdef CFG_NU_OS_SVCS_PWR_CORE_ENABLE
    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

    /* Check for waking from hibernate */
    if (INCT_Hibernate_Wake == NU_TRUE)
    {
        /* Calling this function will initialize the system
           to the state that it was in before entering hibernate.
           
           NOTE:  This call will not return. */
        PMS_Hibernate_Initialize();
    }

    #endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE */

    /* Call Nucleus PLUS system initialization
       NOTE:  Control never returns here */
    INC_Initialize(first_avail_mem);

    /* Code should never reach here.  This line ensures compiler doesn't try
       to optimize return from this function and cause a stack pointer
       problem. */
    ESAL_GE_STK_NO_RETURN();
}
