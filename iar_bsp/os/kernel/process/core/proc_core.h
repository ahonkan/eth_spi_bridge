/*************************************************************************
*
*             Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

************************************************************************
*
* FILE NAME
*
*       proc_core.h
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       Internal prototypes, definitions, and data type for the core
*       component of processes
*
* DATA STRUCTURES
*
*       None
*
************************************************************************/
#ifndef PROC_CORE_H
#define PROC_CORE_H

/* Process control block identifier is 'PROC' */
#define PROC_CB_ID                  0x50524F43UL

#define PROC_INDEX_AVAILABLE        0

/* Process state definitions - There are 2 sets of states,
   the first is set by the call that enables the next state
   to occur and the second set is for the states being complete */
typedef enum
{
    PROC_CREATED_STATE =            0,          /* The process is created */
    PROC_LOADING_STATE =            1,          /* The process is set to be loaded by the kernel thread */
    PROC_STARTING_STATE =           2,          /* The process is set to be started by the kernel thread */
    PROC_DEINITIALIZING_STATE =     3,          /* The process is set to start the deinit thread */
    PROC_STOPPING_STATE =           4,          /* The process is set to be stopped by the kernel thread after deinit is complete */
    PROC_UNLOADING_STATE =          5,          /* The process is set to be unloaded by the kernel thread */
    PROC_KILLING_STATE =            6,          /* The process is set to be unloaded by the kernel thread from any complete state */
    PROC_STOPPED_STATE =            7,          /* The process is ready to be started or unloaded */
    PROC_STARTED_STATE =            8,          /* The process is running */
    PROC_FAILED_STATE =             9           /* The process failed during creation */
} PROC_STATE;

/* Define highest priority allowed for process tasks (ie lowest value) */
#if (CFG_NU_OS_KERN_PROCESS_CORE_MIN_USER_TASK_PRIORITY >= CFG_NU_OS_KERN_PLUS_CORE_NUM_TASK_PRIORITIES)
/* Ensure at least 8 priorities available for user process tasks */
#define PROC_TASK_MIN_PRIORITY      (CFG_NU_OS_KERN_PLUS_CORE_NUM_TASK_PRIORITIES - 8)
#else
/* Use configured value */
#define PROC_TASK_MIN_PRIORITY      CFG_NU_OS_KERN_PROCESS_CORE_MIN_USER_TASK_PRIORITY
#endif

/* Root task settings */
#define PROC_ROOT_STACK_SIZE        CFG_NU_OS_KERN_PROCESS_CORE_STACK_SIZE
#define PROC_ROOT_PRIORITY          CFG_NU_OS_KERN_PROCESS_CORE_MIN_USER_TASK_PRIORITY
#define PROC_ROOT_TIMESLICE         10

/* Kernel task settings */
#define PROC_KERNEL_STACK_SIZE      4096
#define PROC_KERNEL_PRIORITY        0
#define PROC_KERNEL_TIMESLICE       0

/* Default heap size */
#define PROC_HEAP_SIZE              CFG_NU_OS_KERN_PROCESS_CORE_HEAP_SIZE
#define PROC_HEAP_MIN_ALLOC         4

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
/* Page size */
#define PROC_PAGE_SIZE              CFG_NU_OS_KERN_PROCESS_CORE_PAGE_SIZE
#else
/* Page size */
#define PROC_PAGE_SIZE              0
#endif

/* Stack alignment required for root stack */
#if     (PROC_PAGE_SIZE < ESAL_TS_REQ_STK_ALIGNMENT)
#define PROC_STACK_ALIGNMENT        ESAL_TS_REQ_STK_ALIGNMENT
#else
#define PROC_STACK_ALIGNMENT        PROC_PAGE_SIZE
#endif

/* Queue settings */
#define PROC_QUEUE_SIZE             1
#define PROC_MSG_SIZE               PROC_QUEUE_SIZE     /* For now the queue will only support 1 message at a time */

/* Data structures and type definitions for Process Support */
typedef VOID (*NU_PROC_ENTRY)(INT, VOID *);

struct PROC_CB_STRUCT
{
    CS_NODE         created;                /* List of processes */
    UNSIGNED        valid;                  /* Internal CB ID */
    CHAR            name[PROC_NAME_LENGTH]; /* Process name */
    UNSIGNED        id;                     /* Process ID / Address space identifier ASID */
    VOID           *translation;            /* Level 1 page address */
    CS_NODE        *owned_regions;          /* Head of list of owned regions */
    UNSIGNED        owned_total;            /* Number of owned regions */
    CS_NODE        *tasks;                  /* Head of list of process tasks */
    UNSIGNED        total_tasks;            /* Number of tasks bound to process */
    NU_MEMORY_POOL *pool;                   /* Region specified for memory pools */
    UNSIGNED        heap_size;              /* Size of heap to allocate for the process */
    PROC_STATE      state;                  /* Current state of the process */
    PROC_STATE      prev_state;             /* Previous state of the process */
    NU_SYMBOL_ENTRY*symbols;                /* Pointer to process linker symbol table */
    NU_SYMBOL_ENTRY*ksymbols;               /* Pointer to process kernel mode linker symbol table
                                               Used only when mode switching or MMU support is enabled */
    VOID           *load_addr;              /* Load address of process */
    NU_PROC_ENTRY   entry_addr;             /* Process entry address */
    NU_TASK         root_task;              /* Root thread used for entry/exit calls in process context */
    UNSIGNED        stack_size;             /* Size of root task's stack */
    NU_SEMAPHORE    semaphore;              /* Mutex used for process syncronization */
    NU_QUEUE        queue;                  /* Queue used to recieve messages back from kernel thread/state machine */
    UNSIGNED        buffer[PROC_QUEUE_SIZE];/* Buffer for messages */
    VOID           *text_start;             /* Start address of text section (offset of load address) */
    VOID           *rodata_start;           /* Start address of read only data section (offset of load address) */
    VOID           *data_bss_start;         /* Start address of data section containing bss and data (offset of load address) */
    VOID           *initdata_start;         /* Start address of read-only copy of data section to be used as initdata
                                               to initialize the data section each time the process is started from
                                               the stopped state */
    VOID           *data_start;             /* Start address of the data section */
    UINT32          data_size;              /* Size of the data section */
    VOID           *bss_start;              /* Start address of BSS section */
    UINT32          bss_size;               /* Size of BSS section */

    /* Symbol use tracking */
    UINT32          sym_using[((CFG_NU_OS_KERN_PROCESS_CORE_MAX_PROCESSES - 1) / 32) + 1];  /* Array of processes with symbols used by this process. */
    UNSIGNED        sym_using_count;        /* Number of processes with symbols used by this process. */
    UNSIGNED        sym_used_count;         /* Number of processes using symbols of this process. */
    INT             exit_code;              /* Exit code returned by main() or exit() */
    INT             abort_flag;             /* Flag indicating if abort() was called */
    BOOLEAN         kernel_mode;            /* Flag indicating whether this is a kernel mode process */
    BOOLEAN         exit_protect;           /* Marks the process as entering exit() which disallows specific functionality */
};

typedef struct PROC_CB_STRUCT  PROC_CB;

extern PROC_CB     *PROC_Scheduled_CB;
extern PROC_CB     *PROC_Kernel_CB;

/* Internal process list traversal */
extern CS_NODE *                    PROC_Created_List;
#define PROC_GET_FIRST()            (PROC_CB *)PROC_Created_List
#define PROC_GET_NEXT(process)      (PROC_CB *)(process -> created.cs_next)

#if (CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT == NU_TRUE)

/* Development support */
extern VOID (*PROC_Load_Notify_Ptr)(CHAR *, UINT, UINT);
extern VOID (*PROC_Unload_Notify_Ptr)(UINT);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT */

/* Internal process function prototypes */
STATUS PROC_Initialize(PROC_CB **process, CHAR *name);
STATUS PROC_Delete(PROC_CB *process);
PROC_CB *PROC_Get_Pointer(INT id);
STATUS PROC_Release_CB(PROC_CB *process);

STATUS PROC_Alloc(VOID **pointer, UINT32 size, UINT32 alignment);
STATUS PROC_Free(VOID *pointer);
STATUS PROC_Get_Name(INT id, CHAR ** name);
STATUS PROC_Transition(INT id, PROC_STATE trans_state, INT trans_val, UNSIGNED suspend);
VOID   PROC_Kernel_Task_Entry(UNSIGNED argc, VOID * argv);

STATUS PROC_Load(PROC_CB *process);
STATUS PROC_Start(PROC_CB *process);
STATUS PROC_Stop(PROC_CB *process);
STATUS PROC_Unload(PROC_CB *process);

STATUS PROC_Symbols_Use(PROC_CB * sym_owner, PROC_CB * sym_user);
STATUS PROC_Symbols_Unuse(PROC_CB * sym_owner, PROC_CB * sym_user);
STATUS PROC_Symbols_Unuse_All(PROC_CB * sym_user);
BOOLEAN PROC_Symbols_In_Use(PROC_CB * sym_owner);
VOID    PROC_AR_Exception(INT exception_num, VOID *stack_frame);
UNSIGNED PROC_Exception(VOID *exception_address, UNSIGNED exception_type, VOID *return_address, VOID *exception_information);

/* Section symbols for ARM GNU compilers */
#ifdef __GNUC__

extern char _ld_text_start[];
extern char _ld_text_end[];

extern char _ld_rtl_start[];
extern char _ld_rtl_end[];

extern char _ld_rodata_start[];
extern char _ld_rodata_end[];

extern char _ld_nutramp_start[];
extern char _ld_nutramp_end[];

#define TEXT_START      _ld_text_start
#define TEXT_END        _ld_text_end

#define DATA_START      (VOID *)&_ld_ram_data_start
#define BSS_END         (VOID *)&_ld_bss_end

#define RTL_START       _ld_rtl_start
#define RTL_END         _ld_rtl_end

#define RODATA_START    _ld_rodata_start
#define RODATA_END      _ld_rodata_end

#endif  /*  ARMGNU    */

#endif /* PROC_CORE_H */
