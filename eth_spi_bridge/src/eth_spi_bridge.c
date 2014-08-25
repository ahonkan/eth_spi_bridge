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

/*---------------------------------*/
/* Copied block from web demo code */
#ifdef CFG_NU_OS_NET_WEB_ENABLE
VOID DEMO_WebServ_Register_Plugins(VOID);
static STATUS DEMO_WebServ_Get_activeDevices(WS_REQUEST * req);
static STATUS DEMO_WebServ_Get_Setting_IP(WS_REQUEST * req);
UINT8 TBDemo_Atoi_New(char* ptr, UINT8* NUM);
#endif

/*extern VOID* g_DHCP_ptr */;
NU_MEMORY_POOL *app_mem;


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

    /* Wait until the NET stack is initialized. */
    status = NETBOOT_Wait_For_Network_Up(NU_SUSPEND);

    /* start web and ssh */
    if (status == NU_SUCCESS)
    {
#ifdef CFG_NU_OS_NET_SSH_ENABLE
    	UM_Add_User("hello", "world", UM_SSH, UM_ADD_MODE);
#endif
        printf("\r\nRun shell commands to read IP\r\n");
    }

#ifdef CFG_NU_OS_NET_WEB_ENABLE
#if (CFG_NU_OS_NET_WEB_INCLUDE_SSL == 1)

    status = NU_WS_SSL_Init(app_mem);
#endif
    if (status == NU_SUCCESS)
    {
        DEMO_WebServ_Register_Plugins();
        printf("\r\n And open the Nucleus website in Browser to read the Target IP\r\n");
    }
#endif

    
    while(1)
    {
/*      status = NU_Ping(dest_addr, 0);*/
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

/*---------------------------------------------------------------------------*/
#ifdef CFG_NU_OS_NET_WEB_ENABLE
VOID DEMO_WebServ_Register_Plugins(VOID)
{
    /* Register plugins used by the demo. */
    WS_Register_Plugin(DEMO_WebServ_Get_activeDevices,      "get_activeDevices",  WS_PRIVATE);
    WS_Register_Plugin(DEMO_WebServ_Get_Setting_IP,         "app_setting_ip",  WS_PRIVATE);
}


/*************************************************************************
* FUNCTION
*
*       DEMO_WebServ_Get_activeDevices
*
* DESCRIPTION
*
*       Plugin that returns the list of active devices.
*
*************************************************************************/
static STATUS DEMO_WebServ_Get_activeDevices(WS_REQUEST * req)
{
    //INT         i;
    CHAR        ubuf[1500];
    //CHAR        temp[50];
    STATUS      status;
    struct if_nameindex *   if_index;
    struct if_nameindex *   if_index_orig;
    NU_IOCTL_OPTION         ioctl_opt;
    DV_DEVICE_ENTRY *       dev;
    ROUTE_NODE *            default_route;
    RTAB4_ROUTE_ENTRY *     default_gateway;
    CHAR                    gateway_ip_addr[4];
    CHAR                    buf[100];


    /* Get pointer to list of interfaces */
    if_index_orig = if_index = NU_IF_NameIndex();

    /* Empty the String. */
    ubuf[0] = '\0';

    /* Loop through interfaces */
    while ((if_index != NU_NULL) && (if_index->if_name != NU_NULL))
    {
        strcat(ubuf, "<tr>");

        /* Device name. */
        strcat(ubuf, "<td>");

        strcat(ubuf, if_index->if_name);

        strcat(ubuf, "</td>");

        /* Device status. */
        strcat(ubuf, "<td>");
        /* Get interface status */
        status = NU_Device_Up(if_index->if_name);
        if (status == NU_TRUE)
        {
            /* Show interface up and hardware address */
            strcat(ubuf, "UP");
        }
        else
        {
            strcat(ubuf, "DOWN");
        }
        strcat(ubuf, "</td>");

        strcat(ubuf, "<td>");

        /* Get a pointer to the device */
        dev = DEV_Get_Dev_By_Name(if_index->if_name);

        /* Put the interface hardware address in buffer */
        sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",dev->dev_mac_addr[0],
                                                                               dev->dev_mac_addr[1],
                                                                               dev->dev_mac_addr[2],
                                                                               dev->dev_mac_addr[3],
                                                                               dev->dev_mac_addr[4],
                                                                               dev->dev_mac_addr[5]);
        strcat(ubuf, buf);
        strcat(ubuf, "</td>");

        strcat(ubuf, "<td>");
        /* Put the interface name in the IOCTL option structure member */
        ioctl_opt.s_optval = (UINT8*)if_index->if_name;

        /* Call NU_Ioctl to get the IPv6 address */
        status = NU_Ioctl(SIOCGIFADDR_IN6, &ioctl_opt, sizeof(ioctl_opt));

        /* Check if we got the IPv6 address */
        if (status == NU_SUCCESS)
        {
            /* Output the IPv6 address */
            sprintf(buf,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                                                                  ioctl_opt.s_ret.s_ipaddr[0],
                                                                  ioctl_opt.s_ret.s_ipaddr[1],
                                                                  ioctl_opt.s_ret.s_ipaddr[2],
                                                                  ioctl_opt.s_ret.s_ipaddr[3],
                                                                  ioctl_opt.s_ret.s_ipaddr[4],
                                                                  ioctl_opt.s_ret.s_ipaddr[5],
                                                                  ioctl_opt.s_ret.s_ipaddr[6],
                                                                  ioctl_opt.s_ret.s_ipaddr[7],
                                                                  ioctl_opt.s_ret.s_ipaddr[8],
                                                                  ioctl_opt.s_ret.s_ipaddr[9],
                                                                  ioctl_opt.s_ret.s_ipaddr[10],
                                                                  ioctl_opt.s_ret.s_ipaddr[11],
                                                                  ioctl_opt.s_ret.s_ipaddr[12],
                                                                  ioctl_opt.s_ret.s_ipaddr[13],
                                                                  ioctl_opt.s_ret.s_ipaddr[14],
                                                                  ioctl_opt.s_ret.s_ipaddr[15]);
            strcat(ubuf, buf);
        }
        else
        {
            /* Output that the IPv6 address is unavailable */
        	strcat(ubuf, "Unavailable");
        }

        strcat(ubuf, "</td>");


        strcat(ubuf, "<td>");
        /* Call NU_Ioctl to get the IPv4 address. */
        status = NU_Ioctl(SIOCGIFADDR, &ioctl_opt, sizeof(ioctl_opt));

        /* Check if we got the IPv4 address */
        if (status == NU_SUCCESS)
        {
            /* Output the IPv4 address */
            sprintf(buf,"%d.%d.%d.%d", ioctl_opt.s_ret.s_ipaddr[0],
                                                                  ioctl_opt.s_ret.s_ipaddr[1],
                                                                  ioctl_opt.s_ret.s_ipaddr[2],
                                                                  ioctl_opt.s_ret.s_ipaddr[3]);
            strcat(ubuf,buf);
        }
        else
        {
            /* Output that IPv4 address is unavailable */
        	strcat(ubuf, "Unavailable");
        }

        strcat(ubuf, "</td>");


        strcat(ubuf, "<td>");
        /* Call NU_Ioctl to get the IP mask */
        status = NU_Ioctl(SIOCGIFNETMASK, &ioctl_opt, sizeof(ioctl_opt));

        /* Check if we got the IP mask */
        if (status == NU_SUCCESS)
        {
            /* Output the net mask */
            sprintf(buf,"%d.%d.%d.%d", ioctl_opt.s_ret.s_ipaddr[0],
                                                                  ioctl_opt.s_ret.s_ipaddr[1],
                                                                  ioctl_opt.s_ret.s_ipaddr[2],
                                                                  ioctl_opt.s_ret.s_ipaddr[3]);
            strcat(ubuf, buf);
        }
        else
        {
            /* Output that the mask is unavailable */
        	strcat(ubuf, "Unavailable");
        }

        strcat(ubuf, "</td>");


        strcat(ubuf, "<td>");
        /* Get the Default route */
        default_route = NU_Get_Default_Route(NU_FAMILY_IP);

        /* Check if we got the Default route */
        if (default_route != NU_NULL)
        {
            /* Get a pointer to the Default Gateway address */
            default_gateway = (RTAB4_ROUTE_ENTRY*)default_route->rt_route_entry_list.rt_entry_head;

            /* Convert gateway to IP address format */
            PUT32(&gateway_ip_addr[0], 0, default_gateway->rt_gateway_v4.sck_addr);

            /* Output the Default Gateway address */
            sprintf(buf,"%d.%d.%d.%d", gateway_ip_addr[0],
                                                                  gateway_ip_addr[1],
                                                                  gateway_ip_addr[2],
                                                                  gateway_ip_addr[3]);
            strcat(ubuf, buf);
        }
        else
        {
            /* Output that the Default Gateway address is unavailable */
        	strcat(ubuf, "Unavailable");
        }
        strcat(ubuf, "</td>");


        /* Go to next interface */
        if_index = (struct if_nameindex *)((CHAR *)if_index +
                    sizeof(struct if_nameindex) + DEV_NAME_LENGTH);
        /* Close the table row */
         strcat(ubuf, "</tr>");
    }   /* while */



        /*  Output statistics. */
        status = WSN_Write_To_Net(req, ubuf, (UINT32)strlen(ubuf), WS_PLUGIN_DATA);

        /* Abort if there's an error */
        if (status != NU_SUCCESS)
        {
            /* Error has occurred. */
            return WS_REQ_ABORTED;
        }

    /* Free name index memory */
    NU_IF_FreeNameIndex(if_index_orig);
    /*  return to Proceed Request */
    return (WS_REQ_PROCEED);
}

static STATUS DEMO_WebServ_Get_Setting_IP(WS_REQUEST * req)
{
    CHAR        *pg_string = 0;
    CHAR        url_buf[WS_URL_LEN];

    struct if_nameindex *   if_index;
    struct if_nameindex *   if_index_orig;
    NU_IOCTL_OPTION         ioctl_opt;
    DV_DEVICE_ENTRY *       dev;
    STATUS                  status;
    UINT8                   i;
    UINT8      address_src[4] = {0x00};
    UINT8      subnet_src[4]  = {0x00};
    CHAR *     ip_ptr;

    static UINT8 entry_count = 0x00;

    /* Find out the required element in POST method. */
    pg_string = HTTP_Token_Value_by_Name("dev_lb", req);

    /* Make sure that we have found the required element. */
    if (pg_string)
    {
		/* Get pointer to list of interfaces */
        if_index_orig = if_index = NU_IF_NameIndex();
        /* Obtain the TCP semaphore to protect the stack global variables */

        /* Loop through interfaces */
        while ((if_index != NU_NULL) && (if_index->if_name != NU_NULL))
        {

            if( strcmp (pg_string,if_index->if_name ) == 0x00)
            {
            	pg_string = HTTP_Token_Value_by_Name("dev_ip", req);
            	ip_ptr = pg_string;

                for (i = 0x00; i< 4; i++)
                {
                	ip_ptr += TBDemo_Atoi_New(ip_ptr, &address_src[i]);
                	ip_ptr++;
                }

            	pg_string = HTTP_Token_Value_by_Name("sub_mask", req);
            	ip_ptr = pg_string;
                for (i = 0x00; i< 4; i++)
                {
                	ip_ptr += TBDemo_Atoi_New(ip_ptr, &subnet_src[i]);
                	ip_ptr++;
                }

                /* Put the interface name in the IOCTL option structure member */
                ioctl_opt.s_optval = (UINT8*)if_index->if_name;
                /* Call NU_Ioctl to get the IPv4 address. */
                status = NU_Ioctl(SIOCGIFADDR, &ioctl_opt, sizeof(ioctl_opt));


                if (entry_count == 0x00)
                {
                	entry_count = 0x01;

#ifdef CFG_NU_OS_NET_STACK_INCLUDE_DHCP
                	LED2_SET;
/*
                    status = NU_Dhcp_Release (g_DHCP_ptr,
                		         if_index->if_name);
*/
#else
                //status = NU_Detach_IP_From_Device (if_index->if_name);
                  	status = NU_Remove_IP_From_Device (if_index->if_name,
               			                           (UINT8*)&(ioctl_opt.s_ret.s_ipaddr[0]),
                			                       NU_FAMILY_IP);
#endif
                }
                else
                {
                   	status = NU_Remove_IP_From_Device (if_index->if_name,
                   			                           (UINT8*)&(ioctl_opt.s_ret.s_ipaddr[0]),
                    			                       NU_FAMILY_IP);
                }
                status = NU_Attach_IP_To_Device(
					                            if_index->if_name,
					                            address_src,
					                            subnet_src);
            }
            /* Go to next interface */
            if_index = (struct if_nameindex *)((CHAR *)if_index +
                        sizeof(struct if_nameindex) + DEV_NAME_LENGTH);
        }
    }

    /*  Redirect to ssi.ssi script when completed  */
    HTTPS_Uri_To_Url(req, "settings.ssi", url_buf);
    HTTP_Redirect_Client(req, url_buf);

    /*  return to Proceed Request */
    return (WS_REQ_PROCEED);
}


#define TBD_IS_DIGIT(c) ( (int) (c >= '0' && c <= '9') )

UINT8 TBDemo_Atoi_New(char* ptr, UINT8* NUM)
{
    CHAR  c  = *ptr;
    *NUM = 0x00;
    UINT8 incr = 0x00;
    while ( TBD_IS_DIGIT(c) )
    {
    	*NUM = ( 10 * (*NUM) ) + (unsigned int)( c - '0' );
        c = *++ptr;
        incr++;
    }

    return ( incr );
}
#endif



