/* Required include files for C STDIO and Nucleus PLUS kernel services */
#include <stdio.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/nu_networking.h"


/* Define the main task's stack size */
#define HELLO_WORLD_TASK_STACK_SIZE      (NU_MIN_STACK_SIZE * 8)

/* Define the main task's priority */
#define HELLO_WORLD_TASK_PRIORITY   30

/* Define the main task's time slice */
#define HELLO_WORLD_TASK_TIMESLICE  20

/* Statically allocate the main task's control block */
static NU_TASK Task_Control_Block;

/* Prototype for the main task's entry function */
static VOID Main_Task_Entry(UNSIGNED argc, VOID *argv);

/***********************************************************************
 * *
 * *   FUNCTION
 * *
 * *       Application_Initialize
 * *
 * *   DESCRIPTION
 * *
 * *       Demo application entry point - initializes Nucleus Plus
 * *       demonstration application by creating and initializing necessary
 * *       tasks, memory pools, and communication components.
 * *
 * ***********************************************************************/
VOID Application_Initialize(NU_MEMORY_POOL* mem_pool,
                            NU_MEMORY_POOL* uncached_mem_pool)
{
    VOID *pointer;
    STATUS status;

    /* Reference unused parameters to avoid toolset warnings */
    NU_UNUSED_PARAM(uncached_mem_pool);

    /* Allocate memory for the main task */
    status = NU_Allocate_Memory(mem_pool, &pointer,
                                HELLO_WORLD_TASK_STACK_SIZE, NU_NO_SUSPEND);

    /* Check to see if previous operation was successful */
    if (status == NU_SUCCESS)
    {
        /* Create task 0.  */
        status = NU_Create_Task(&Task_Control_Block, "MAIN", Main_Task_Entry,
                                0, NU_NULL, pointer, HELLO_WORLD_TASK_STACK_SIZE,
                                HELLO_WORLD_TASK_PRIORITY, HELLO_WORLD_TASK_TIMESLICE,
                                NU_PREEMPT, NU_START);
    }

    /* Check to see if previous operations were successful */
    if (status != NU_SUCCESS)
    {
        /* Loop forever */
        while(1);
    }
}

/***********************************************************************
 * *
 * *   FUNCTION
 * *
 * *       Main_Task_Entry
 * *
 * *   DESCRIPTION
 * *
 * *       Entry function for the main task. This task prints a hello world
 * *       message.
 * *
 * ***********************************************************************/
static VOID Main_Task_Entry(UNSIGNED argc, VOID *argv)
{
	STATUS status;
	UINT8 dest_addr[] = {192, 168, 2, 3};


    /* Reference all parameters to ensure no toolset warnings */
    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    printf("\r\nHello Nucleus world!\r\n");

    status = NU_Ethernet_Link_Up("ifspi0");
    status = NU_Device_Up("ifspi0");
    if (status == NU_FALSE)
    	printf("ifspi0 is not up\r\n");

    while(1)
    {
    	LED1_SET;
    	NU_Sleep(50);
    	LED1_CLEAR;
    	NU_Sleep(50);
    }

/*
    status = NU_Ethernet_Link_Up("ifspi1");
    status = NU_Device_Up("ifspi1");
    if (status == NU_FALSE)
    	printf("ifspi1 is not up\r\n");
*/

    /* Send an ICMP Echo Request to the destination address and wait
      * on the default timeout for a response.
      */
/*
    status = NU_Ping(dest_addr, 0);
    if (status != NU_SUCCESS)
    {
        printf("Destination address unreachable.\n");
    }
*/
}
