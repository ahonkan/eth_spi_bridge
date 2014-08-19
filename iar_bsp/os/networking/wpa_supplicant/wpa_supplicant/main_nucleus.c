/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*
 * WPA Supplicant / main() function for UNIX like OSes and MinGW
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details. 
 */
#include "includes.h"
#include "common.h"
#include "wpa.h"
#include "eloop.h"
#include "wpa_supplicant_i.h"
#include "wpa_supplicant_cfg.h"
#include "wpa_supplicant_api.h"
#include "drivers/ethernet.h"
#include "networking/wireless.h"
#include "networking/wl_evt_types.h"
#include "services/reg_api.h"
#include "nucleus_gen_cfg.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "storage/pcdisk.h"
#include "services/runlevel_init.h"

/*
 * Global Variables
 */

/* The WPA Supplicant's global data structure. */
struct wpa_global *global;

/* Memory pool for dynamic memory allocations. */
NU_MEMORY_POOL *WPA_Supplicant_Mem = NU_NULL;

/* Main Nucleus task for the WPA Supplicant. */
NU_TASK *WPA_Supplicant_Main_Task = NU_NULL;

/* Suspend/resume Nucleus task for the WPA Supplicant. */
NU_TASK *WPA_Supplicant_Suspend_Task = NU_NULL;

/* Semaphore to protect os_printf() call internals. */
NU_SEMAPHORE WPA_Supplicant_Printf_Semaphore;

/* Buffer for temporarily storing incoming EAPOL packets. */
UINT8 *WPA_Supplicant_Input_Buffer = NU_NULL;

/* Supplicant initialization task control block. */
NU_TASK WPA_Supplicant_Init_Task;
VOID *WPA_Supplicant_Init_Task_Stack = NU_NULL;

/* Registry path for the wpa_supplicant. */
CHAR WPA_Supplicant_Registry_Path[REG_MAX_KEY_LENGTH];

/* List containing mapping of DV_DEV_ID to DV_DEV_HANDLE. */
struct wpa_supplicant_devmap {
	DV_DEV_HANDLE dev_handle;
	DV_DEV_ID dev_id;
	char *dev_name;
	u8 is_suspended;
	u8 padding;
} WPA_Supplicant_Devmap[CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED];

/* External variables. */
extern struct wpa_interface WPA_Supplicant_Ifaces_Config[];
extern NU_MEMORY_POOL System_Memory;
extern CHAR net_registry_path[];

/*
 * Function Prototypes
 */
STATIC VOID WPA_Supplicant_Main_Task_Entry(UNSIGNED argc, VOID *argv);
STATIC VOID WPA_Supplicant_Suspend_Task_Entry(UNSIGNED argc, VOID *argv);


/**
 * nu_os_net_wpa_supplicant_wpa_supplicant_init - Initialization function
 * @path: Path to the component configuration in the registry
 * @startstop: If true then start operation, otherwise stop operation
 *
 * This is the system initialization and deinitialization function
 * which is called by the RTOS. It is called in the specified run-level
 * order.
 */
STATUS nu_os_net_wpa_supp_src_init(CHAR *path,INT startstop)
{
	STATUS status = NU_SUCCESS;
	DV_DEV_LABEL wifi_label = { WIFI_LABEL };
	DV_LISTENER_HANDLE listener_handle;

	if (path != NU_NULL) {
		/* Save a copy locally. */
		os_strncpy(WPA_Supplicant_Registry_Path, path, REG_MAX_KEY_LENGTH);
	}

	if (startstop == RUNLEVEL_START) {
		/* Start requested, so start up the wpa_supplicant. */

		/* Initialize the WPA Supplicant. */
		status = WPA_Supplicant_Initialize(&System_Memory);

		if (status != NU_SUCCESS) {
			wpa_printf(MSG_ERROR, "NU_WPA: Initialization failed.");

			/* Call error handling function */
			ERC_System_Error(status);
		}

		/* Register listener with device manager. */
		status = DVC_Reg_Change_Notify(&wifi_label,
									DV_GET_LABEL_COUNT(wifi_label),
									WIFI_Dev_Register_CB,
									WIFI_Dev_Unregister_CB,
									NU_NULL,
									&listener_handle);
		if (status != NU_SUCCESS) {
			wpa_printf(MSG_ERROR, "NU_WPA: Listener registration failed.");

			/* Call error handling function */
			ERC_System_Error(status);
		}
	}
	else if (startstop == RUNLEVEL_STOP) {
		/* Stop requested, so shutdown the wpa_supplicant. */
		status = WPA_Supplicant_Shutdown();

		if (status != NU_SUCCESS)
			wpa_printf(MSG_ERROR, "NU_WPA: Shutdown operation failed.");
	}

	return (status);

}


/**
 * WPA_Supplicant_Initialize - Initializes the %wpa_supplicant
 * @mem_pool: Pointer to the memory pool for dynamic memory allocation
 * Returns: NU_SUCCESS on success and error code on failure
 *
 * This is the Nucleus-specific initialization function for the
 * %wpa_supplicant product. It is either called by the RTOS
 * initialization module or if that is disabled, then it should be called
 * from the user's application.
 */
STATUS WPA_Supplicant_Initialize(NU_MEMORY_POOL *mem_pool)
{
	STATUS status;
	VOID *t1_stack_ptr = NU_NULL;
	NU_SUPERV_USER_VARIABLES

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	/* Set the memory pool. */
	WPA_Supplicant_Mem = mem_pool;

	/* Clear the device map list. */
	os_memset(WPA_Supplicant_Devmap, 0, sizeof(WPA_Supplicant_Devmap));

	/* Create a semaphore for "printf" related functions. */
	status = NU_Create_Semaphore(&WPA_Supplicant_Printf_Semaphore,
								"WPAPRNT", (UNSIGNED)1, NU_FIFO);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to create semaphore.");
		NU_USER_MODE();
		return status;
	}

	/* Allocate memory for the input buffer. */
	status = NU_Allocate_Memory(mem_pool,
					(VOID **)&WPA_Supplicant_Input_Buffer,
					WPA_SUPPLICANT_INPUT_BUFFER_SIZE, NU_NO_SUSPEND);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to allocate input buffer.");
		NU_USER_MODE();
		return status;
	}

	/* Allocate memory for task control block and stack. */
	status = NU_Allocate_Memory(mem_pool,
					(VOID **)&WPA_Supplicant_Main_Task,
					sizeof(NU_TASK) + WPA_SUPPLICANT_MAIN_TASK_STACK_SIZE,
					NU_NO_SUSPEND);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to allocate task memory.");
		NU_USER_MODE();
		return status;
	}

	memset(WPA_Supplicant_Main_Task, 0, sizeof(NU_TASK));
	t1_stack_ptr = (VOID *)(&WPA_Supplicant_Main_Task[1]);

	/* Create the main WPA Supplicant task. */
	status = NU_Create_Task(WPA_Supplicant_Main_Task,
					"WPA_SUP",
					WPA_Supplicant_Main_Task_Entry,
					0, NU_NULL, t1_stack_ptr,
					WPA_SUPPLICANT_MAIN_TASK_STACK_SIZE,
					WPA_SUPPLICANT_MAIN_TASK_PRIORITY,
					WPA_SUPPLICANT_MAIN_TASK_TIME_SLICE,
					WPA_SUPPLICANT_MAIN_TASK_PREEMPT, NU_NO_START);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to create main task.");
		NU_USER_MODE();
		return status;
	}

	if (status == NU_SUCCESS)
	{
		status = NU_Resume_Task(WPA_Supplicant_Main_Task);
		if (status != NU_SUCCESS) {
			wpa_printf(MSG_ERROR, "NU_WPA: Unable to resume main task.");
			NU_USER_MODE();
			return status;
		}
	}

	/* Allocate memory for the suspend/resume task CB and stack. */
	status = NU_Allocate_Memory(mem_pool,
					(VOID **)&WPA_Supplicant_Suspend_Task,
					sizeof(NU_TASK) + WPA_SUPPLICANT_MAIN_TASK_STACK_SIZE,
					NU_NO_SUSPEND);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to allocate suspend stack.");
		NU_USER_MODE();
		return status;
	}

	memset(WPA_Supplicant_Suspend_Task, 0, sizeof(NU_TASK));
	t1_stack_ptr = (VOID *)(&WPA_Supplicant_Suspend_Task[1]);

	/* Create the suspend/resume WPA Supplicant task. */
	status = NU_Create_Task(WPA_Supplicant_Suspend_Task,
					"WPA_SUS",
					WPA_Supplicant_Suspend_Task_Entry,
					0, NU_NULL, t1_stack_ptr,
					WPA_SUPPLICANT_MAIN_TASK_STACK_SIZE,
					WPA_SUPPLICANT_MAIN_TASK_PRIORITY,
					WPA_SUPPLICANT_MAIN_TASK_TIME_SLICE,
					WPA_SUPPLICANT_MAIN_TASK_PREEMPT, NU_NO_START);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to create suspend task.");
		NU_USER_MODE();
		return status;
	}

	if (status == NU_SUCCESS)
	{
		status = NU_Resume_Task(WPA_Supplicant_Suspend_Task);
	}

	NU_USER_MODE();
	return status;
}


/**
 * WPA_Supplicant_Shutdown - Shuts down the %wpa_supplicant
 * Returns: NU_SUCCESS on success and error code on failure
 *
 * This function shuts down the WPA Supplicant daemon and deinitializes
 * the supplicant. The WPA Supplicant can be re-initialized by calling
 * the WPA_Supplicant_Initialize() API function.
 *
 * Note that this is a non-blocking function which returns immediately.
 * The WPA Supplicant may take a few seconds to terminate its task and
 * deinitialize its data structures after this function returns.
 */
STATUS WPA_Supplicant_Shutdown(VOID)
{
	STATUS status;
	NU_SUPERV_USER_VARIABLES

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	/* Terminate the main processing loop of the WPA Supplicant. */
	eloop_terminate();
	if (WPA_Supplicant_Input_Buffer != NU_NULL) {
		NU_Deallocate_Memory(WPA_Supplicant_Input_Buffer);
		WPA_Supplicant_Input_Buffer = NU_NULL;
	}

	/* Wait for the main WPA Supplicant task to terminate. */
	NU_Sleep(2 * NU_PLUS_TICKS_PER_SEC);

	if (WPA_Supplicant_Main_Task != NU_NULL) {
		/* Forcefully terminate the task. */
		NU_Terminate_Task(WPA_Supplicant_Main_Task);

		/* Delete the task. */
		NU_Delete_Task(WPA_Supplicant_Main_Task);

		/* Deallocate the task's control block and stack. */
		NU_Deallocate_Memory(WPA_Supplicant_Main_Task);
		WPA_Supplicant_Main_Task = NU_NULL;
	}

	if (WPA_Supplicant_Suspend_Task != NU_NULL) {
		/* Forcefully terminate the task. */
		NU_Terminate_Task(WPA_Supplicant_Suspend_Task);

		/* Delete the task. */
		NU_Delete_Task(WPA_Supplicant_Suspend_Task);

		/* Deallocate the task's control block and stack. */
		NU_Deallocate_Memory(WPA_Supplicant_Suspend_Task);
		WPA_Supplicant_Suspend_Task = NU_NULL;
	}

	/* Delete the display output semaphore. */
	status = NU_Delete_Semaphore(&WPA_Supplicant_Printf_Semaphore);
	if (status != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "NU_WPA: Unable to delete a semaphore.");

	/* Switch back to user mode. */
	NU_USER_MODE();

	return NU_SUCCESS;
}


/**
 * WPA_Supplicant_Reload_Config - Reloads the configuration file
 * Returns: NU_SUCCESS on success and error code on failure
 *
 * This function makes WPA Supplicant reload its configuration file
 * without re-starting the service.
 */
STATUS WPA_Supplicant_Reload_Config(VOID)
{
	NU_SUPERV_USER_VARIABLES

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	/* Signal the "eloop" module to reload the configuration file. */
	eloop_handle_signal(SIGHUP);

	/* Switch back to user mode. */
	NU_USER_MODE();

	return NU_SUCCESS;
}


/**
 * WPA_Supplicant_Get_Iface_Count - Returns number if interfaces
 * Returns: Number of interfaces registered with the %wpa_supplicant
 *
 * This function returns the number if interfaces registered with
 * the %wpa_supplicant.
 */
INT WPA_Supplicant_Get_Iface_Count(VOID)
{
	struct wpa_supplicant *wpa_s;
	INT count = 0;
	NU_SUPERV_USER_VARIABLES

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		count++;
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

	return count;
}


/**
 * WPA_Supplicant_Get_Instance - Gets an instance using dest-address
 * @dest_addr: Destination MAC address
 * Returns: Pointer to the %wpa_supplicant instance
 *
 * This function returns a pointer to the %wpa_supplicant instance using
 * the destination MAC address.
 */
struct wpa_supplicant * WPA_Supplicant_Get_Instance(CHAR *dest_addr)
{
	struct wpa_supplicant *wpa_s;
	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp((const char *)wpa_s->own_addr, dest_addr) == 0) {
			return wpa_s;
		}
	}
	return NULL;
}


/**
 * WPA_Supplicant_Copy_Chain_To_Buffer - Copies a chain to a single buffer
 * @src_buf: The source buffer chain
 * @dest_buf: The destination buffer
 * @dest_len: Length of the destination buffer
 * Returns: Number of bytes copied
 *
 * Copies a chain of NET Buffers into a single contiguous buffer.
 */
INT WPA_Supplicant_Copy_Chain_To_Buffer(NET_BUFFER *src_buf,
					UINT8 *dest_buf, INT dest_len)
{
	INT32		bytes_copied = 0;
	INT32		bytes_to_copy;

	/* Loop through the chain if needed and copy all buffers until
	 * the maximum number of bytes the user can accept has been copied or
	 * the end of data is reached.
	 */
	while ((src_buf != NU_NULL) && (bytes_copied < dest_len)) {
		/* Determine how many bytes to copy from the first buffer */
		if (src_buf->data_len <= (UINT32)(dest_len - bytes_copied))
			bytes_to_copy = (INT32)src_buf->data_len;
		else
			bytes_to_copy = dest_len - bytes_copied;

		if (bytes_to_copy != 0) {
			/* Copy the data. */
			NU_BLOCK_COPY((dest_buf + bytes_copied), src_buf->data_ptr,
						  (unsigned int)bytes_to_copy);
		}

		/* Update the bytes copied. */
		bytes_copied += bytes_to_copy;

		/* Move to the next buffer in the chain */
		src_buf = src_buf->next_buffer;
	}

	return bytes_copied;
}


/**
 * WPA_Supplicant_EAPOL_Rx - EAPOL packet input function
 * @src_addr: Source MAC address of the packet
 * @dst_addr: Destination MAC address of the packet
 * @buf_ptr: Pointer to the NET Buffer chain
 * @device: Pointer to the input device structure
 * 
 * This function is called by NET in context of the "NET_Demux" task
 * when an incoming EAPOL packet is received.
 */
VOID WPA_Supplicant_EAPOL_Rx(CHAR *src_addr, CHAR *dst_addr,
						NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device)
{
	struct wpa_supplicant *wpa_s;
	INT data_len;
	CHAR disk[4] = "A:\\";

	disk[0] += CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE;

	/* Remove compiler warning for an unused parameter. */
	(void)device;

	/* Make sure the input buffer is currently allocated. */
	if (WPA_Supplicant_Input_Buffer == NU_NULL)
		return;

	if (NU_Check_File_User())
	{
		NU_Become_File_User();
		NU_Open_Disk(disk);
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
		NU_Set_Current_Dir(disk);
	}

	/* Recalculate the source address. */
	dst_addr = (CHAR *)buf_ptr->data_ptr - (DADDLEN * 2) - 2 - 8;
	src_addr = (CHAR *)buf_ptr->data_ptr - DADDLEN - 2 - 8;

	wpa_s = WPA_Supplicant_Get_Instance(dst_addr);
	if (wpa_s != NULL) {
		data_len = WPA_Supplicant_Copy_Chain_To_Buffer(buf_ptr,
									WPA_Supplicant_Input_Buffer,
									WPA_SUPPLICANT_INPUT_BUFFER_SIZE);

		/* Release the internal NET semaphore. This is obtained by the
		 * caller (NET Demux task). Releasing the semaphore isn't
		 * recommended, but it should be done due to design restrictions.
		 */
		NU_Release_Semaphore(&TCP_Resource);

		wpa_supplicant_rx_eapol(wpa_s, (const u8 *)src_addr,
								WPA_Supplicant_Input_Buffer, data_len);

		/* Re-obtain the semaphore which was released above. */
		if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS) {
			wpa_printf(MSG_ERROR, "NU_WPA: Unable to obtain semaphore.");
		}
	}

	/* Place the packet back on the "buffer freelist". */
	MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
}


#if (CFG_NU_OS_NET_WPA_SUPP_SRC_ENABLE_FALLBACK_CONF)
/**
 * WPA_Supplicant_Write_Fallback_Conf - Write the "fall-back" config file
 * @conf_name: Name of the configuration file
 *
 * This function checks whether the specified configuration file exists.
 * If it is not found, it creates the file using pre-defined contents.
 */
int WPA_Supplicant_Write_Fallback_Conf(const char *conf_name)
{
	int fd;

	/* Check if the file already exists. */
	fd = os_file_open(conf_name, PO_RDONLY, PS_IREAD);
	
	if (fd >= 0) {
		os_file_close(fd);
		return 0;
	}

	/* Write the "fall-back" configuration file. */
	fd = os_file_open(conf_name, PO_CREAT | PO_WRONLY, PS_IWRITE);

	if (fd >= 0) {
		os_file_fprintf(fd, CFG_NU_OS_NET_WPA_SUPP_SRC_FALLBACK_CONF_DATA);
		os_file_close(fd);
	} else {
		return -1;
	}

	return 0;
}
#endif /* (CFG_NU_OS_NET_WPA_SUPP_SRC_ENABLE_FALLBACK_CONF) */


/**
 * WPA_Supplicant_Devid_By_Handle - Returns DEV_ID of specified handle
 * @dev_handle: Handle of the device
 *
 * This is a utility function which returns the DEV_ID corresponding to
 * a device handle.
 */
DV_DEV_ID WPA_Supplicant_Devid_By_Handle(int dev_handle)
{
	int i;

	/* Search for the device handle in the "devmap" list. */
	for (i = 0; i < CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED; i++) {
		if (WPA_Supplicant_Devmap[i].dev_handle == dev_handle)
			return WPA_Supplicant_Devmap[i].dev_id;
	}

	return -1;
}


/* WPA_Supplicant_Device_Discovery - WLAN device discovery function
 *
 * This function discoverys new WLAN devices and adds them to the
 * %wpa_supplicant interface list. It also detects removal of devices
 * and processes them accordingly. This function is repeatedly called
 * by the main "eloop".
 */
STATUS WIFI_Dev_Register_CB(DV_DEV_ID device_id, VOID *context)
{
	STATUS status;
	int j;
	NU_DEVICE eth_mw;
	DV_IOCTL0_STRUCT dev_ioctl0;
	DV_DEV_HANDLE dev_handle;
	DV_DEV_LABEL wifi_label = { WIFI_LABEL };
	DV_DEV_LABEL eth_label = { ETHERNET_LABEL };
	struct wpa_interface iface;
	struct wpa_interface *iface_ptr;
	char drv_param[15];

	/* Open the wireless device. */
	status = DVC_Dev_ID_Open(device_id, &wifi_label, 1,
				&dev_handle);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Error in DVC_Dev_ID_Open()");
		return (status);
	}

	/* Get the "Ethernet" IOCTL base address. */
	dev_ioctl0.label = eth_label;
	status = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0,
				sizeof(DV_IOCTL0_STRUCT));

	if (status == NU_SUCCESS) {
		/* Get the "NU_DEVICE" structure from the device. */
		status = DVC_Dev_Ioctl(dev_handle, 
					dev_ioctl0.base + ETHERNET_CMD_GET_DEV_STRUCT,
					(VOID*)&eth_mw, sizeof(NU_DEVICE));
	}
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "NU_WPA: Error in DVC_Dev_Ioctl()");
		DVC_Dev_Close(dev_handle);
		return (status);
	}

	/* Store device handle and name in the device map list. */
	for (j = 0; j < CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED; j++) {
		if (WPA_Supplicant_Devmap[j].dev_id == 0) {
			WPA_Supplicant_Devmap[j].dev_id = device_id;
			WPA_Supplicant_Devmap[j].dev_handle = dev_handle;

			WPA_Supplicant_Devmap[j].dev_name =
				os_malloc(os_strlen(eth_mw.dv_name) + 1);
			os_strncpy(WPA_Supplicant_Devmap[j].dev_name,
				eth_mw.dv_name, os_strlen(eth_mw.dv_name) + 1);
			break;
		}
	}

	/* Try to find the new interface in the configuration list. */
	iface_ptr = &WPA_Supplicant_Ifaces_Config[0];

	while (iface_ptr->ifname != NULL) {
		if (os_strcmp(iface_ptr->ifname, eth_mw.dv_name) == 0) {
			iface = *iface_ptr;
			break;
		} else if (os_strcmp(iface_ptr->ifname, "*") == 0) {
			/* If this is a wildcard match, then override the
			 * interface name. */
			iface = *iface_ptr;
			iface.ifname = eth_mw.dv_name;
			break;
		}

		iface_ptr++;
	}
	/* If a valid configuration was found. */
	if (iface_ptr->ifname != NULL) {
#if (CFG_NU_OS_NET_WPA_SUPP_SRC_ENABLE_FALLBACK_CONF)
		WPA_Supplicant_Write_Fallback_Conf(iface.confname);
#endif

	/* Pass the device handle as a driver parameter. */
	os_strncpy(drv_param, "-d **********", sizeof(drv_param));
	NCL_Itoa(dev_handle, drv_param + 3, 10);
	iface.driver_param = drv_param;

	if (wpa_supplicant_add_iface(global, &iface) == NULL)
		wpa_printf(MSG_DEBUG,
			"NU_WPA: Unable to add new iface");
	} else {
		wpa_printf(MSG_INFO,
			"NU_WPA: No config found for new iface");
		DVC_Dev_Close(dev_handle);
	}

	return (status);
}

STATUS WIFI_Dev_Unregister_CB(DV_DEV_ID device_id, VOID *context)
{
	int j;
	DV_DEV_HANDLE dev_handle;
	struct wpa_supplicant *wpa_s;
	NET_WL_EVTDATA_ALL evt_data;

	/* Search for device ID in the device map list. */
	for (j = 0; j < CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED; j++) {
		/* If a match is found. */
		if (WPA_Supplicant_Devmap[j].dev_id == device_id) {
			dev_handle = WPA_Supplicant_Devmap[j].dev_handle;

			for (wpa_s = global->ifaces; wpa_s;
					wpa_s = wpa_s->next) {
				if (os_strcmp(wpa_s->ifname,
						WPA_Supplicant_Devmap[j].dev_name) == 0) {
					if (wpa_supplicant_remove_iface(global, wpa_s)
							!= 0) {
						wpa_printf(MSG_DEBUG,
							"NU_WPA: Unable to remove interface");
					}
					break;
				}
			}

			/* Close the device now. */
			DVC_Dev_Close(dev_handle);

			/* Remove entry from the device map list. */
			WPA_Supplicant_Devmap[j].dev_id = 0;
			WPA_Supplicant_Devmap[j].dev_handle = 0;
			os_free(WPA_Supplicant_Devmap[j].dev_name);
			WPA_Supplicant_Devmap[j].dev_name = NULL;
			WPA_Supplicant_Devmap[j].is_suspended = NU_FALSE;

			break;
		}
	}

	memset(&evt_data, 0, sizeof(NET_WL_EVTDATA_ALL));
	evt_data.wl_event_type = WL_EVENT_INTERFACE_REM;
	NU_EQM_Post_Event(&NET_WL_Event_Queue, (EQM_EVENT *)&evt_data,
					  sizeof(NET_WL_EVTDATA_ALL), NU_NULL);

	return (NU_SUCCESS);
}


/* WIFI_Dev_Suspend_Iface - Suspend the WPA Supplicant interface.
 *
 * This function brings the specified WPA Supplicant interface into
 * a suspended state by removing the interface from the WPA Supplicant
 * interface list. But some details of the interface are kept intact
 * so that the interface can later be re-added when it resumes.
 */
STATUS WIFI_Dev_Suspend_Iface(DV_DEV_ID device_id)
{
	int j;
	struct wpa_supplicant *wpa_s;
	STATUS status = NU_INVALID_SUSPEND;

	/* Search for device ID in the device map list. */
	for (j = 0; j < CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED; j++) {
		/* If a match is found. */
		if (WPA_Supplicant_Devmap[j].dev_id == device_id) {
			/* If the device is already suspended then do nothing. */
			if (WPA_Supplicant_Devmap[j].is_suspended == NU_TRUE) {
				wpa_printf(MSG_DEBUG,
					"NU_WPA: Device already suspended. Ignoring suspend.");
				status = NU_UNAVAILABLE;
				break;
			}

			/* Search for device in the device map and remove it from the
			 * WPA Supplicant's interface list. */
			for (wpa_s = global->ifaces; wpa_s;
					wpa_s = wpa_s->next) {
				if (os_strcmp(wpa_s->ifname,
						WPA_Supplicant_Devmap[j].dev_name) == 0) {
					if (wpa_supplicant_remove_iface(global, wpa_s)
							!= 0) {
						wpa_printf(MSG_DEBUG,
							"NU_WPA: Unable to remove interface");
					} else {
						WPA_Supplicant_Devmap[j].is_suspended = NU_TRUE;
						status = NU_SUCCESS;
					}
					break;
				}
			}
			break;
		}
	}

	return (status);
}


/* WIFI_Dev_Resume_Iface - Resume the WPA Supplicant interface.
 *
 * This function brings the specified WPA Supplicant interface into
 * a resumed state by adding back the interface to the WPA Supplicant
 * interface list. The interface should have previously been suspended
 * using the suspend counterpart of this function.
 */
STATUS WIFI_Dev_Resume_Iface(DV_DEV_ID device_id)
{
	STATUS status = NU_INVALID_RESUME;
	int j;
	struct wpa_interface iface;
	struct wpa_interface *iface_ptr;
	char drv_param[15];

	/* Store device handle and name in the device map list. */
	for (j = 0; j < CFG_NU_OS_NET_STACK_MAX_DEVS_SUPPORTED; j++) {
		if (WPA_Supplicant_Devmap[j].dev_id == device_id) {
			/* If the device is not suspended then return an error. */
			if (WPA_Supplicant_Devmap[j].is_suspended == NU_FALSE) {
				wpa_printf(MSG_DEBUG,
					"NU_WPA: Device not suspended. Ignoring resume.");
				break;
			}

			status = NU_SUCCESS;
			break;
		}
	}

	/* If a matching entry in the device map was found. */
	if (status == NU_SUCCESS) {
		/* Try to find the new interface in the configuration list. */
		iface_ptr = &WPA_Supplicant_Ifaces_Config[0];

		while (iface_ptr->ifname != NULL) {
			if (os_strcmp(iface_ptr->ifname,
						  WPA_Supplicant_Devmap[j].dev_name) == 0) {
				iface = *iface_ptr;
				break;
			} else if (os_strcmp(iface_ptr->ifname, "*") == 0) {
				/* If this is a wildcard match, then override the
				 * interface name. */
				iface = *iface_ptr;
				iface.ifname = WPA_Supplicant_Devmap[j].dev_name;
				break;
			}

			iface_ptr++;
		}

		/* If a valid configuration was found. */
		if (iface_ptr->ifname != NULL) {

			/* Pass the device handle as a driver parameter. */
			os_strncpy(drv_param, "-d **********", sizeof(drv_param));
			NCL_Itoa(WPA_Supplicant_Devmap[j].dev_handle, drv_param+3, 10);
			iface.driver_param = drv_param;

			if (wpa_supplicant_add_iface(global, &iface) == NULL) {
				wpa_printf(MSG_DEBUG,
					"NU_WPA: Unable to add interface after resume");
				status = NU_INVALID_RESUME;
			}
			WPA_Supplicant_Devmap[j].is_suspended = NU_FALSE;
		} else {
			wpa_printf(MSG_INFO,
				"NU_WPA: No config found for resumed iface");
		}
	}

	return (status);
}


/**
 * WPA_Supplicant_Suspend_Task_Entry - The suspend/resume task entry.
 * @argc: Parameter count for this task
 * @argv: Parameter vector for this task
 *
 * This is the suspend/resume task for the %wpa_supplicant which monitors
 * the suspending and resuming of WLAN interfaces being managed by the
 * %wpa_supplicant.
 */
VOID WPA_Supplicant_Suspend_Task_Entry(UNSIGNED argc, VOID *argv)
{
	NET_WL_EVTDATA_ALL msg;
	UINT32 notify_type;
	STATUS status;
	EQM_EVENT_ID event_id = 0;
	EQM_EVENT_HANDLE event_handle;
	WL_EVTDATA_INTERFACE_STATUS *interface_status;

	/* Reference unused parameters to avoid toolset warnings */
	NU_UNUSED_PARAM(argc);
	NU_UNUSED_PARAM(argv);

	for (;;) {
		status = NU_EQM_Wait_Event(&NET_WL_Event_Queue,
								0xffffffff,
								&notify_type,
								&event_id,
								&event_handle);

		if (status == NU_SUCCESS)
		{
			status = NU_EQM_Get_Event_Data(&NET_WL_Event_Queue,
										event_id,
										event_handle,
										(EQM_EVENT *)&msg);
		}
		
		if (status != NU_SUCCESS)
		{
			os_sleep(0, 250000);
			continue;
		}

		if (notify_type == WL_EVENT_INTERFACE_STATUS) {
			interface_status = (WL_EVTDATA_INTERFACE_STATUS *)&msg.wl_data;

			if (interface_status != NULL) {
				/* Do nothing for interface add/remove because these
				 * events are handled in the Nucleus-specific code and
				 * cause addition/removal of the WPA Supplicant interface.
				 * Only handle interface "dormant" and "up" here since
				 * these are used to bring the interface into a suspend
				 * and resume state by the power management support of the
				 * WLAN driver.
				 */
				if (interface_status->wl_ievent == WL_EVENT_INTERFACE_DORMANT)
					WIFI_Dev_Suspend_Iface(interface_status->wl_dev_id);
				else if (interface_status->wl_ievent == WL_EVENT_INTERFACE_UP)
					WIFI_Dev_Resume_Iface(interface_status->wl_dev_id);
			}
		}
	}
}


/**
 * WPA_Supplicant_Main_Task_Entry - Main task entry point
 * @argc: Parameter count for this task
 * @argv: Parameter vector for this task
 *
 * This is the main %wpa_supplicant task which carries out all the
 * processing for the %wpa_supplicant. It runs the main eloop and
 * waits for WLAN interfaces to be added on which to perform the
 * connection sequence.
 */
VOID WPA_Supplicant_Main_Task_Entry(UNSIGNED argc, VOID *argv)
{
	int exitcode = -1;
	struct wpa_params params;

	/* Reference unused parameters to avoid toolset warnings */
	NU_UNUSED_PARAM(argc);
	NU_UNUSED_PARAM(argv);

	/* printf("\r\n\r\nwpa_supplicant starting.\n"); */

	/* This routine sets the WPA clock with refrence to Epoch. */
	os_setdatetime(2010,11,1,0,0,0);

	if (os_program_init())
		wpa_printf(MSG_ERROR, "NU_WPA: os_program_init() failed.");

	/* Set default params - See main_nucleus_cfg.h */
	os_memset(&params, 0, sizeof(params));
	params.daemonize            = 0;
	params.wpa_debug_level      = WPA_SUPPLICANT_PARAM_DEBUG_LEVEL;
	params.wpa_debug_file_path  = WPA_SUPPLICANT_PARAM_DEBUG_FILE_PATH;
	params.wpa_debug_show_keys  = WPA_SUPPLICANT_PARAM_DEBUG_SHOW_KEYS;
	params.pid_file             = NULL;
	params.wpa_debug_timestamp  = WPA_SUPPLICANT_PARAM_DEBUG_SHOW_TIMESTAMP;
	params.dbus_ctrl_interface  = 0;
	params.wait_for_monitor     = WPA_SUPPLICANT_PARAM_WAIT_FOR_MONITOR;
	if (CFG_NU_OS_NET_WPA_SUPP_SRC_CTRL_IFACE == NULL ||
				os_strlen(CFG_NU_OS_NET_WPA_SUPP_SRC_CTRL_IFACE) == 0)
		params.ctrl_interface   = NULL;
	else
		params.ctrl_interface   = CFG_NU_OS_NET_WPA_SUPP_SRC_CTRL_IFACE;

	exitcode = 0;
	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		wpa_printf(MSG_ERROR, "wpa_supplicant_init() failed.");
		exitcode = -1;
	}

	if (exitcode == 0)
		exitcode = wpa_supplicant_run(global);

	wpa_supplicant_deinit(global);
	os_program_deinit();

	os_free(params.pid_file);

	printf("\nwpa_supplicant exiting with status %d\n", exitcode);
	return;
}
