/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       rnl_init.c
*
*   DESCRIPTION
*
*       This file contains the run-level code for Nucleus.
*
*   FUNCTIONS
*
*       NU_RunLevel_Init
*       RunLevel_Init_Task
*       RunLevel_Start
*       NU_RunLevel_Component_Control
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus definitions
*       tc_extr.h                           PLUS Task external API
*       runlevel.h                           Run-Level internal API
*       runlevel.h                           Run-Level external API
*       reg_api.h                           Registry API
*       string.h                            Standard string functions
*       stdio.h                             Standard IO functions
*
*************************************************************************/

/* Include necessary services */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "os/kernel/plus/core/inc/thread_control.h"
#include    "os/services/init/inc/runlevel.h"
#include    "services/runlevel_init.h"
#include    "services/reg_api.h"
#include    <string.h>
#include    <stdio.h>

/* Global variables */
NU_EVENT_GROUP              RunLevel_Started_Event;
NU_EVENT_GROUP              RunLevel_Finished_Event;

#ifdef CFG_NU_OS_SVCS_DBG_ENABLE

/* Local variables */
static  NU_TASK             RunLevel_Init_Task_Cb;

#endif

/* Local Function prototypes */
static  VOID                RunLevel_Init_Task_Entry(UNSIGNED argc, VOID * argv);

/*************************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Init
*
*   DESCRIPTION
*
*       This function initializes the various run-level items necessary
*       for the application interfaces to function.
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       mem_pool                            Pointer to memory pool that
*                                           can be used for alloc
*
*   OUTPUTS
*
*       Various                             Errors returned by other
*                                           kernel services
*       NU_SUCCESS                          Successful init of run-level
*
*************************************************************************/
STATUS  NU_RunLevel_Init(NU_MEMORY_POOL * mem_pool)
{
    STATUS  status;

#ifdef CFG_NU_OS_SVCS_DBG_ENABLE

    VOID *  stack_mem;

    /* Allocate memory for system initialization task stack */
    status = NU_Allocate_Memory(mem_pool, (VOID *)&stack_mem,
                                RUNLEVEL_INIT_TASK_STACK_SIZE,
                                NU_NO_SUSPEND);

    /* Ensure memory successfully allocated */
    if (status == NU_SUCCESS)
    {
        /* Create system initialization task as a normal task */
        status = NU_Create_Task(&RunLevel_Init_Task_Cb, "RUNLVL",
                                RunLevel_Init_Task_Entry, 0, NU_NULL,
                                stack_mem, RUNLEVEL_INIT_TASK_STACK_SIZE,
                                RUNLEVEL_INIT_TASK_PRIORITY, 0,
                                NU_PREEMPT, NU_START);
    }

#else

    /* Create system initialization task as an auto-clean task */
    status = NU_Create_Auto_Clean_Task(NU_NULL, "RUNLVL",
                                       RunLevel_Init_Task_Entry, 0,
                                       NU_NULL, mem_pool,
                                       RUNLEVEL_INIT_TASK_STACK_SIZE,
                                       RUNLEVEL_INIT_TASK_PRIORITY, 0,
                                       NU_PREEMPT, NU_START);

#endif

    /* Create "started" event group */
    if (status == NU_SUCCESS)
    {
        status = NU_Create_Event_Group(&RunLevel_Started_Event, "RLSTART");
    }

    /* Ensure "started" event group successfully created */
    if (status == NU_SUCCESS)
    {
        /* Create "finished" event group */
        status = NU_Create_Event_Group(&RunLevel_Finished_Event, "RLFINSH");
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       RunLevel_Init_Task_Entry
*
*   DESCRIPTION
*
*       This function is the entry point for the run-level initialization
*       task.
*
*   CALLED BY
*
*       Kernel
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       argc                                unsigned value set at
*                                           task creation
*       argv                                void pointer set at task
*                                           creation
*
*   OUTPUTS
*
*       none
*
*************************************************************************/
static  VOID    RunLevel_Init_Task_Entry(UNSIGNED argc, VOID * argv)
{
    INT     run_level;

    /* Suppress warnings */
    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    /* Get the starting run-level*/
    run_level = RUNLEVEL_FIRST;

    /* Loop through all system run-levels */
    do
    {
        /* Initialize the run-level */
        RunLevel_Start(run_level);

        /* Move to next run-level */
        run_level++;

    } while (run_level < RUNLEVEL_APP);

    /* Transition to application */
    (VOID)TCS_Change_Group_ID(NU_Current_Task_Pointer(),
                              TC_GRP_ID_APP);

    /* Loop through all application run-levels */
    do
    {
        /* Initialize the run-level */
        RunLevel_Start(run_level);

        /* Move to next run-level */
        run_level++;

    /* Check if need to keep looping */
    } while (run_level <= RUNLEVEL_INIT_MAX_RUNLEVEL);

    return;
}


/*************************************************************************
*
*   FUNCTION
*
*       RunLevel_Start
*
*   DESCRIPTION
*
*       This internal function starts the specified run-level
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       runlevel                            Runlevel to start
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID        RunLevel_Start(INT runlevel)
{
    STATUS  status;
    CHAR    keypath[REG_MAX_KEY_LENGTH];
    CHAR    comppath[REG_MAX_KEY_LENGTH];
    INT     runlevelcomplete = NU_FALSE;
    INT     entrynum = 1;


    /* Set start bit for this run-level */
    NU_Set_Events(&RunLevel_Started_Event, (1UL << runlevel), NU_OR);

    /* Loop until no more components found at the given run-level */
    do
    {
        /* Create registry path for this entry */
        sprintf(keypath, RUNLEVEL_ROOT "%d/%d", runlevel, entrynum);

        /* Get registry entry for this string */
        status = REG_Get_String((const CHAR *)keypath, comppath, REG_MAX_KEY_LENGTH);

        /* Ensure this entry exits
           NOTE: This shouldn't fail, but continue trying other entries for this
                 run-level even if one entry is missing information */
        if (status == NU_SUCCESS)
        {
            /* Start the run-level component */
            (VOID)NU_RunLevel_Component_Control(comppath, 1);

            /* Go to next entry number */
            entrynum++;
        }
        else
        {
            /* Done with this run-level */
            runlevelcomplete = NU_TRUE;
        }

    /* Check if runlevel is complete */
    } while (!runlevelcomplete);

    /* Set finished bit for this run-level */
    NU_Set_Events(&RunLevel_Finished_Event, (1UL << runlevel), NU_OR);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Component_Control
*
*   DESCRIPTION
*
*       Can be used to manually initialize any run-level 0 component
*       or all run-level 0 components from an application.
*
*       NOTE: Current implementation cannot check if a start request is
*             being made on an already started component (same with stop request).
*             This functionality will be implemented when a writable registry
*             is supported and the component status will be updated in the
*             registry.
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       compregpath                         Component registry path
*                                           If NULL, initializes all
*                                           components in Run-Level 0
*       compctrl                            0 to stop, 1 to start,
*                                           2 to hibernate and 3 to
*                                           resume component
*
*   OUTPUTS
*
*       NU_SUCCESS                          Component control operation
*                                           successfully completed
*       NU_NOT_PRESENT                      The specified component is not found
*
*************************************************************************/
STATUS  NU_RunLevel_Component_Control(const CHAR * compregpath, INT compctrl)
{
    STATUS                  status;
    RUNLEVEL_COMP_ENTRY     comp_entry;
    CHAR                    keypath[REG_MAX_KEY_LENGTH];


    /* Create the key registry path for the component's entrypoint */
    strcpy(keypath, compregpath);
    strcat(keypath, RUNLEVEL_ENTRY_KEY);

    /* Get component entry-point function address from the registry */
    status = REG_Get_UINT32((const CHAR *)keypath, (UINT32 *)&comp_entry);

    /* Make sure an entry was found */
    if ((status == NU_SUCCESS) && (comp_entry != NU_NULL))
    {
        /* Call the component entry function with registry path and specified start/stop value */
        status = comp_entry(compregpath, compctrl);

        /* PLACEHOLDER - Log component status to registry once registry is read/write */
    }
    else
    {
        /* Set return value to show component not present */
        status = NU_NOT_PRESENT;
    }

    /* Return results of operation */
    return (status);
}
