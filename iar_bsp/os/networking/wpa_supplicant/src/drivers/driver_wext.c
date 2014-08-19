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
 * WPA Supplicant - driver interaction with generic Linux Wireless Extensions
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
 *
 * This file implements a driver interface for the Linux Wireless Extensions.
 * When used with WE-18 or newer, this interface can be used as-is with number
 * of drivers. In addition to this, some of the common functions in this file
 * can be used by other driver interface implementations that use generic WE
 * ioctls, but require private ioctls for some of the functionality.
 */

#include "includes.h"

#include "drivers/ethernet.h"
#include "networking/wireless.h"
#include "common.h"
#include "driver.h"
#include "eloop.h"
#include "priv_netlink.h"
#include "driver_wext.h"
#include "ieee802_11_defs.h"
#include "wpa_common.h"

#include "wpa_supplicant/wpa_supplicant_api.h"
#include "networking/net_evt.h"
#include "networking/wl_evt_types.h"

#define WPA_DRIVER_WEXT_L2PKTHDR_SIZE 32

DV_DEV_ID WPA_Supplicant_Devid_By_Handle(int dev_handle);

static int wpa_driver_wext_flush_pmkid(void *priv);
static int wpa_driver_wext_get_range(void *priv);
static void wpa_driver_wext_finish_drv_init(struct wpa_driver_wext_data *drv);

static int wpa_driver_wext_send_oper_ifla(struct wpa_driver_wext_data *drv,
					  int linkmode, int operstate)
{
	DV_REQ      dvreq;
	STATUS      status;

	if (operstate == -1)
		return 0;

	/* Check for ethernet base. */
    if (drv->eth_base == NU_NULL)
    {
		return -1;
	}

    if (operstate == IF_OPER_UP)
		{
        dvreq.dvr_flags = IF_OPER_UP;
        if ((status = DVC_Dev_Ioctl(drv->dev_handle,
                drv->eth_base + ETHERNET_CMD_SEND_LINK_STATUS,
                (VOID*)&dvreq, sizeof(DV_REQ))) < 0)
        {
            if (status != DV_DEV_NOT_REGISTERED)
            {
                wpa_printf(MSG_ERROR, "WEXT: Unable to change link state.");
            }
        }
    }
    else if (operstate == IF_OPER_DORMANT)
    {
        dvreq.dvr_flags = IF_OPER_DORMANT;
        if ((status = DVC_Dev_Ioctl(drv->dev_handle,
                drv->eth_base + ETHERNET_CMD_SEND_LINK_STATUS,
                (VOID*)&dvreq, sizeof(DV_REQ))) < 0)
        {
            if (status != DV_DEV_NOT_REGISTERED)
            {
                wpa_printf(MSG_ERROR, "WEXT: Unable to change link state.");
            }
        }
    }
    else
    {
        status = 0;
		}

	return status;
}


int wpa_driver_wext_set_auth_param(struct wpa_driver_wext_data *drv,
				   int idx, u32 value)
{
	int ret = 0;
	DV_REQ dvreq;

	dvreq.dvr_flags = idx & IW_AUTH_INDEX;
	dvreq.dvr_dvru.dvru_metric = value;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_AUTH,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_get_bssid - Get BSSID, SIOCGIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: Buffer for BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_get_bssid(void *priv, u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	DV_REQ dvreq;

	dvreq.dvr_value_ptr_1 = bssid;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_GET_WAP,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_bssid - Set BSSID, SIOCSIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_bssid(void *priv, const u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	char null_bssid[ETH_ALEN]; 
	DV_REQ dvreq;

	dvreq.dvr_flags = ARPHRD_ETHER;
	if (bssid)
		dvreq.dvr_value_ptr_1 = (void *)bssid;
	else {
		os_memset(null_bssid, 0, sizeof(null_bssid));
		dvreq.dvr_value_ptr_1 = null_bssid;
	}

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_WAP,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_get_ssid - Get SSID, SIOCGIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: Buffer for the SSID; must be at least 32 bytes long
 * Returns: SSID length on success, -1 on failure
 */
int wpa_driver_wext_get_ssid(void *priv, u8 *ssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	DV_REQ dvreq;

	dvreq.dvr_value_ptr_1 = ssid;
	dvreq.dvr_value_1 = 32;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_GET_ESSID,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCGIWESSID]");
		ret = -1;
	} else {
		/* Return new length. */
		ret = dvreq.dvr_value_1;
		if (ret > 32)
			ret = 32;
		/* Some drivers include nul termination in the SSID, so let's
		 * remove it here before further processing. WE-21 changes this
		 * to explicitly require the length _not_ to include nul
		 * termination. */
		if (ret > 0 && ssid[ret - 1] == '\0' &&
		    drv->we_version_compiled < 21)
			ret--;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_ssid - Set SSID, SIOCSIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: SSID
 * @ssid_len: Length of SSID (0..32)
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_ssid(void *priv, const u8 *ssid, size_t ssid_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	char buf[33];
	DV_REQ dvreq;

	if (ssid_len > 32)
		return -1;

	/* flags: 1 = ESSID is active, 0 = not (promiscuous) */
	dvreq.dvr_flags = (ssid_len != 0);
	os_memset(buf, 0, sizeof(buf));
	os_memcpy(buf, ssid, ssid_len);
	dvreq.dvr_value_ptr_1 = buf;

	if (drv->we_version_compiled < 21) {
		/* For historic reasons, set SSID length to include one extra
		 * character, C string nul termination, even though SSID is
		 * really an octet string that should not be presented as a C
		 * string. Some Linux drivers decrement the length by one and
		 * can thus end up missing the last octet of the SSID if the
		 * length is not incremented here. WE-21 changes this to
		 * explicitly require the length _not_ to include nul
		 * termination. */
		if (ssid_len)
			ssid_len++;
	}
	dvreq.dvr_value_1 = ssid_len;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_ESSID,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWESSID]");
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_freq - Set frequency/channel, SIOCSIWFREQ
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @freq: Frequency in MHz
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_freq(void *priv, int freq)
{
	struct wpa_driver_wext_data *drv = priv;
	DV_REQ dvreq;
	struct iw_freq freq_opt;
	int ret = 0;

	/* Set custom parameter in the DV_REQ structure. */
	dvreq.dvr_value_ptr_1 = &freq_opt;
	dvreq.dvr_value_1 = sizeof(freq_opt);

	os_memset(&freq_opt, 0, sizeof(freq_opt));

	freq_opt.m = freq * 100000;
	freq_opt.e = 1;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_FREQ,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWFREQ]");
		ret = -1;
	}

	return ret;
}


/* Task to listen for events from the notifications module. */
VOID wpa_driver_wext_event_task(UNSIGNED argc, VOID *argv)
{
	struct wpa_driver_wext_data *drv;
	DV_DEV_ID dev_id;
	NET_WL_EVTDATA_ALL msg;
	UINT16 msg_len;
	UINT32 notify_type;
	STATUS status;
	void *ctx;
	union wpa_event_data event_data;
	EQM_EVENT_ID event_id = 0;
	EQM_EVENT_HANDLE event_handle;
	WL_EVTDATA_ASSOC_INFO *assoc_info;
	WL_EVTDATA_MICHAEL_MIC_FAILURE *michael_mic_failure;
	WL_EVTDATA_INTERFACE_STATUS *interface_status;
	WL_EVTDATA_PMKID_CANDIDATE *pmkid_candidate;
	WL_EVTDATA_STKSTART *stkstart;

	/* Make sure the task was passed the driver data as a parameter. */
	if (argc != 1 || argv == NULL) {
		wpa_printf(MSG_ERROR, "WEXT: Event task has no wext data.");
		return;
	}

	drv = (struct wpa_driver_wext_data *)argv;
	ctx = drv->ctx;

	while (drv->dev_handle == 0)
		os_sleep(0, 100000);

	wpa_driver_wext_finish_drv_init(drv);

	dev_id = WPA_Supplicant_Devid_By_Handle(drv->dev_handle);
	if (dev_id < 0) {
		wpa_printf(MSG_ERROR, "WEXT: Unable to look-up device ID.");
		return;
	}

	for (;;) {
		msg_len = WPA_DRIVER_WEXT_MAX_EVTMSG_SIZE;
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

		switch (notify_type) {
		case WL_EVENT_ASSOC:
			assoc_info = (WL_EVTDATA_ASSOC_INFO *)&msg.wl_data;

			if (assoc_info != NULL) {
				memset(&event_data, 0, sizeof(&event_data));
				event_data.assoc_info.beacon_ies = assoc_info->wl_beacon_ies;
				event_data.assoc_info.beacon_ies_len = assoc_info->wl_beacon_ies_len;
				event_data.assoc_info.req_ies = assoc_info->wl_req_ies;
				event_data.assoc_info.req_ies_len = assoc_info->wl_req_ies_len;
				event_data.assoc_info.resp_ies = assoc_info->wl_resp_ies;
				event_data.assoc_info.resp_ies_len = assoc_info->wl_resp_ies_len;

			}
			wpa_supplicant_event(ctx, EVENT_ASSOC, NULL);

			break;

		case WL_EVENT_DISASSOC:
			wpa_supplicant_event(ctx, EVENT_DISASSOC, NULL);
			break;

		case WL_EVENT_MICHAEL_MIC_FAILURE:
			michael_mic_failure = (WL_EVTDATA_MICHAEL_MIC_FAILURE *)&msg.wl_data;

			if (michael_mic_failure != NULL) {
				event_data.michael_mic_failure.unicast =
					michael_mic_failure->wl_unicast;

				wpa_supplicant_event(ctx, EVENT_MICHAEL_MIC_FAILURE,
					&event_data);
			}
			break;

		case WL_EVENT_SCAN_RESULTS:
			drv->scan_complete_events = 1;
			eloop_cancel_timeout(wpa_driver_wext_scan_timeout,
					drv, ctx);
			wpa_supplicant_event(ctx, EVENT_SCAN_RESULTS, NULL);
			break;

		case WL_EVENT_ASSOCINFO:
			assoc_info = (WL_EVTDATA_ASSOC_INFO *)&msg.wl_data;

			if (assoc_info != NULL) {
				memset(&event_data, 0, sizeof(&event_data));
				event_data.assoc_info.beacon_ies = assoc_info->wl_beacon_ies;
				event_data.assoc_info.beacon_ies_len = assoc_info->wl_beacon_ies_len;
				event_data.assoc_info.req_ies = assoc_info->wl_req_ies;
				event_data.assoc_info.req_ies_len = assoc_info->wl_req_ies_len;
				event_data.assoc_info.resp_ies = assoc_info->wl_resp_ies;
				event_data.assoc_info.resp_ies_len = assoc_info->wl_resp_ies_len;

				wpa_supplicant_event(ctx, EVENT_ASSOCINFO, &event_data);
			}
			break;

		case WL_EVENT_INTERFACE_STATUS:
			interface_status = (WL_EVTDATA_INTERFACE_STATUS *)&msg.wl_data;

			if (interface_status != NULL) {
				/* Do nothing. This event is handled in the
				* Nucleus-specific code and causes addition/removal
				* of the WPA Supplicant interface. */
			}
			break;

		case WL_EVENT_PMKID_CANDIDATE:
			pmkid_candidate = (WL_EVTDATA_PMKID_CANDIDATE *)&msg.wl_data;

			if (pmkid_candidate != NULL) {
				os_memcpy(event_data.pmkid_candidate.bssid,
					pmkid_candidate->wl_bssid, ETH_ALEN);
				event_data.pmkid_candidate.index =
					pmkid_candidate->wl_index;
				event_data.pmkid_candidate.preauth =
					pmkid_candidate->wl_preauth;

				wpa_supplicant_event(drv->ctx, EVENT_PMKID_CANDIDATE,
					&event_data);
			}
			break;

		case WL_EVENT_STKSTART:
			stkstart = (WL_EVTDATA_STKSTART *)&msg.wl_data;

			if (stkstart != NULL) {
				os_memcpy(event_data.stkstart.peer, stkstart->wl_peer,
					ETH_ALEN);

				wpa_supplicant_event(ctx, EVENT_STKSTART, &event_data);
			}
			break;
		}
	}
}

static int wpa_driver_wext_get_ifflags_ifname(struct wpa_driver_wext_data *drv,
					      const char *ifname, int *flags)
{
	/* This operation has no equivalent IOCTL in Nucleus. So it always
	 * returns zero flags. */
	*flags = 0;
	return 0;
}


/**
 * wpa_driver_wext_get_ifflags - Get interface flags (SIOCGIFFLAGS)
 * @drv: driver_wext private data
 * @flags: Pointer to returned flags value
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_get_ifflags(struct wpa_driver_wext_data *drv, int *flags)
{
	return wpa_driver_wext_get_ifflags_ifname(drv, drv->ifname, flags);
}


static int wpa_driver_wext_set_ifflags_ifname(struct wpa_driver_wext_data *drv,
					      const char *ifname, int flags)
{
	/* This operation has no equivalent IOCTL in Nucleus. So it always
	 * returns success and does nothing. */
	return 0;
}


/**
 * wpa_driver_wext_set_ifflags - Set interface flags (SIOCSIFFLAGS)
 * @drv: driver_wext private data
 * @flags: New value for flags
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_ifflags(struct wpa_driver_wext_data *drv, int flags)
{
	return wpa_driver_wext_set_ifflags_ifname(drv, drv->ifname, flags);
}


/**
 * wpa_driver_wext_init - Initialize WE driver interface
 * @ctx: context to be used when calling wpa_supplicant functions,
 * e.g., wpa_supplicant_event()
 * @ifname: interface name, e.g., wlan0
 * Returns: Pointer to private data, %NULL on failure
 */
void * wpa_driver_wext_init(void *ctx, const char *ifname)
{
	struct wpa_driver_wext_data *drv;
	NU_TASK *task_mem;
	STATUS status;

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;

	/* Intialize driver_wext data struture */
	drv->event_sock = NU_NO_SOCKETS;
	drv->ioctl_sock = NU_NO_SOCKETS;
	drv->mlme_sock = NU_NO_SOCKETS;

	drv->ctx = ctx;
	os_strlcpy(drv->ifname, ifname, sizeof(drv->ifname));

	/* Allocate memory for event task control block and stack */
	task_mem = os_zalloc(sizeof(NU_TASK) +
					WPA_DRIVER_WEXT_TASK_STACK_SIZE);

	if (task_mem == NULL) {
		wpa_printf(MSG_ERROR, "WEXT: Unable to allocate task stack.");
		os_free(drv);
		return NULL;
	}

	/* Create the main WPA Supplicant task. */
	status = NU_Create_Task(task_mem,
					"WPA_WEXT",
					wpa_driver_wext_event_task,
					1, drv, (VOID *)(task_mem + 1),
					WPA_DRIVER_WEXT_TASK_STACK_SIZE,
					WPA_DRIVER_WEXT_TASK_PRIORITY,
					WPA_DRIVER_WEXT_TASK_TIME_SLICE,
					WPA_DRIVER_WEXT_TASK_PREEMPT, NU_START);
	if (status != NU_SUCCESS) {
		wpa_printf(MSG_ERROR, "WEXT: Unable to create event task.");
		os_free(drv);
		return NULL;
	}

	drv->evt_task = task_mem;

	return drv;
}


static void wpa_driver_wext_finish_drv_init(struct wpa_driver_wext_data *drv)
{
	int flags;

	if (wpa_driver_wext_get_ifflags(drv, &flags) != 0)
		printf("Could not get interface '%s' flags\n", drv->ifname);
	else if (!(flags & IFF_UP)) {
		if (wpa_driver_wext_set_ifflags(drv, flags | IFF_UP) != 0) {
			printf("Could not set interface '%s' UP\n",
			       drv->ifname);
		} else {
			/*
			 * Wait some time to allow driver to initialize before
			 * starting configuring the driver. This seems to be
			 * needed at least some drivers that load firmware etc.
			 * when the interface is set up.
			 */
			wpa_printf(MSG_DEBUG, "Interface %s set UP - waiting "
				   "a second for the driver to complete "
				   "initialization", drv->ifname);
			/* os_sleep(1, 0); */
		}
	}

	/*
	 * Make sure that the driver does not have any obsolete PMKID entries.
	 */
	wpa_driver_wext_flush_pmkid(drv);

	if (wpa_driver_wext_set_mode(drv, 0) < 0) {
		printf("Could not configure driver to use managed mode\n");
	}

	wpa_driver_wext_get_range(drv);

	drv->ifindex = NU_IF_NameToIndex(drv->ifname);

	if (os_strncmp(drv->ifname, "wlan", 4) == 0) {
		/*
		 * Host AP driver may use both wlan# and wifi# interface in
		 * wireless events. Since some of the versions included WE-18
		 * support, let's add the alternative ifindex also from
		 * driver_wext.c for the time being. This may be removed at
		 * some point once it is believed that old versions of the
		 * driver are not in use anymore.
		 */
		char ifname2[IFNAMSIZ + 1];
		os_strlcpy(ifname2, drv->ifname, sizeof(ifname2));
		os_memcpy(ifname2, "wifi", 4);
		wpa_driver_wext_alternative_ifindex(drv, ifname2);
	}

	wpa_driver_wext_send_oper_ifla(drv, 1, IF_OPER_DORMANT);
}


/**
 * wpa_driver_wext_deinit - Deinitialize WE driver interface
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 *
 * Shut down driver interface and processing of driver events. Free
 * private data buffer if one was allocated in wpa_driver_wext_init().
 */
void wpa_driver_wext_deinit(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	int flags;

	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);

	/*
	 * Clear possibly configured driver parameters in order to make it
	 * easier to use the driver after wpa_supplicant has been terminated.
	 */
	(void) wpa_driver_wext_set_bssid(drv,
					 (u8 *) "\x00\x00\x00\x00\x00\x00");

	wpa_driver_wext_send_oper_ifla(priv, 0, IF_OPER_DORMANT);

	/* Teminate the event task, if present. */
	if (drv->evt_task != NULL) {
		NU_Terminate_Task(drv->evt_task);
		NU_Delete_Task(drv->evt_task);

		os_free(drv->evt_task);
		drv->evt_task = NULL;
	}

	if (drv->mlme_sock >= 0)
		eloop_unregister_read_sock(drv->mlme_sock);

	if (wpa_driver_wext_get_ifflags(drv, &flags) == 0)
		(void) wpa_driver_wext_set_ifflags(drv, flags & ~IFF_UP);

	if (drv->mlme_sock >= 0)
		close(drv->mlme_sock);
	os_free(drv->assoc_req_ies);
	os_free(drv->assoc_resp_ies);
	os_free(drv);
}


/**
 * wpa_driver_wext_set_param - Set driver configuration parameters
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @param: Driver specific configuration parameters
 *
 * Returns: 0 on success, -1 on failure
 *
 * Handler for handling configuration parameters of the driver
 * (driver_param).
 */
int wpa_driver_wext_set_param(void *priv, const char *param)
{
	struct wpa_driver_wext_data *drv = priv;
	int dev_handle;
	STATUS status;
	DV_IOCTL0_STRUCT dev_ioctl0;
	DV_DEV_LABEL wifi_label = { WIFI_LABEL };
	DV_DEV_LABEL eth_label = { ETHERNET_LABEL };

	if (param == NULL) {
		wpa_printf(MSG_DEBUG, "%s: no driver parameters specified",
			   __FUNCTION__);
		return -1;
	}

	/* If the device handle is specified. */
	if (os_strncmp(param, "-d ", 3) == 0) {
		dev_handle = NCL_Atoi(param + 3);
		drv->dev_handle = dev_handle;
	} else {
		return -1;
	}

	/* Get the "Ethernet" IOCTL base address. */
	dev_ioctl0.label = eth_label;
	status = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0,
				sizeof(DV_IOCTL0_STRUCT));
	if (status == NU_SUCCESS)
		drv->eth_base = (int)dev_ioctl0.base;

	/* Get the "WIFI" IOCTL base address. */
	dev_ioctl0.label = wifi_label;
	status = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &dev_ioctl0,
				sizeof(DV_IOCTL0_STRUCT));
	if (status == NU_SUCCESS)
		drv->wifi_base = (int)dev_ioctl0.base;

	wpa_driver_wext_get_range(drv);

	return 0;
}


/**
 * wpa_driver_wext_scan_timeout - Scan timeout to report scan completion
 * @eloop_ctx: Unused
 * @timeout_ctx: ctx argument given to wpa_driver_wext_init()
 *
 * This function can be used as registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
void wpa_driver_wext_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	wpa_printf(MSG_DEBUG, "Scan timeout - try to get results");
	wpa_supplicant_event(timeout_ctx, EVENT_SCAN_RESULTS, NULL);
}


/**
 * wpa_driver_wext_scan - Request the driver to initiate scan
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: Specific SSID to scan for (ProbeReq) or %NULL to scan for
 *	all SSIDs (either active scan with broadcast SSID or passive
 *	scan
 * @ssid_len: Length of the SSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_scan(void *priv, const u8 *ssid, size_t ssid_len)
{
	struct wpa_driver_wext_data *drv = priv;
	DV_REQ dvreq;
	struct iw_scan_req req;
	int ret = 0, timeout;

	if (ssid_len > IW_ESSID_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: too long SSID (%lu)",
			   __FUNCTION__, (unsigned long) ssid_len);
		return -1;
	}

	os_memset(&dvreq, 0, sizeof(dvreq));
	os_memset(&req, 0, sizeof(req));

	if (ssid && ssid_len) {
		req.essid_len = ssid_len;
		req.bssid.sa_family = ARPHRD_ETHER;
		os_memset(req.bssid.sa_data, 0xff, ETH_ALEN);
		os_memcpy(req.essid, ssid, ssid_len);

		dvreq.dvr_value_ptr_1 = &req;
		dvreq.dvr_value_1 = sizeof(req);
		dvreq.dvr_flags = IW_SCAN_THIS_ESSID;
	}

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_SCAN,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWSCAN]");
		ret = -1;
	}

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	timeout = 5;
	if (drv->scan_complete_events) {
		/*
		 * The driver seems to deliver SIOCGIWSCAN events to notify
		 * when scan is complete, so use longer timeout to avoid race
		 * conditions with scanning and following association request.
		 */
		timeout = 30;
	}
	wpa_printf(MSG_DEBUG, "Scan requested (ret=%d) - scan timeout %d "
		   "seconds", ret, timeout);
	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);
	eloop_register_timeout(timeout, 0, wpa_driver_wext_scan_timeout, drv,
			       drv->ctx);

	return ret;
}


static u8 * wpa_driver_wext_giwscan(struct wpa_driver_wext_data *drv,
				    size_t *len)
{
	u8 *res_buf;
	size_t res_buf_len;
	DV_REQ dvreq;
	int scan_retries;

	res_buf_len = IW_SCAN_MAX_DATA;
	for (scan_retries = 0; ; scan_retries++) {
		res_buf = os_malloc(res_buf_len);
		if (res_buf == NULL)
			return NULL;

		dvreq.dvr_value_ptr_1 = res_buf;
		dvreq.dvr_value_1 = res_buf_len;

		/* If the scan buffer size isn't sufficient, then this IOCTL
		 * returns the required buffer size in the "dvr_value_1" request
		 * parameter. */ 
		if (DVC_Dev_Ioctl(drv->dev_handle, 
					drv->wifi_base + WIRELESS_CMD_GET_SCAN,
					(VOID*)&dvreq, sizeof(DV_REQ)) == 0)
			break;
		else if (scan_retries >= 9) {
			perror("ioctl[SIOCGIWSCAN]");
			os_free(res_buf);
			return NULL;
		} else {
			os_free(res_buf);
			res_buf = NULL;
			if (dvreq.dvr_value_1 > res_buf_len)
				res_buf_len = dvreq.dvr_value_1;
			else
				res_buf_len *= 2;
			if (res_buf_len > 65535)
				res_buf_len = 65535; /* 16-bit length field */
			wpa_printf(MSG_DEBUG, "Scan results did not fit - "
				   "trying larger buffer (%lu bytes)",
				   (unsigned long) res_buf_len);
		}
	}

	if (dvreq.dvr_value_1 > res_buf_len) {
		os_free(res_buf);
		return NULL;
	}
	*len = dvreq.dvr_value_1;

	return res_buf;
}


/*
 * Data structure for collecting WEXT scan results. This is needed to allow
 * the various methods of reporting IEs to be combined into a single IE buffer.
 */
struct wext_scan_data {
	struct wpa_scan_res res;
	u8 *ie;
	size_t ie_len;
	u8 ssid[32];
	size_t ssid_len;
	int maxrate;
};


static void wext_get_scan_mode(struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	if (iwe->u.mode == IW_MODE_ADHOC)
		res->res.caps |= IEEE80211_CAP_IBSS;
	else if (iwe->u.mode == IW_MODE_MASTER || iwe->u.mode == IW_MODE_INFRA)
		res->res.caps |= IEEE80211_CAP_ESS;
}


static void wext_get_scan_ssid(struct iw_event *iwe,
			       struct wext_scan_data *res, char *custom,
			       char *end)
{
	int ssid_len = iwe->u.essid.length;
	if (custom + ssid_len > end)
		return;
	if (iwe->u.essid.flags &&
	    ssid_len > 0 &&
	    ssid_len <= IW_ESSID_MAX_SIZE) {
		os_memcpy(res->ssid, custom, ssid_len);
		res->ssid_len = ssid_len;
	}
}


static void wext_get_scan_freq(struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	int divi = 1000000, i;

	if (iwe->u.freq.e == 0) {
		/*
		 * Some drivers do not report frequency, but a channel.
		 * Try to map this to frequency by assuming they are using
		 * IEEE 802.11b/g.  But don't overwrite a previously parsed
		 * frequency if the driver sends both frequency and channel,
		 * since the driver may be sending an A-band channel that we
		 * don't handle here.
		 */

		if (res->res.freq)
			return;

		if (iwe->u.freq.m >= 1 && iwe->u.freq.m <= 13) {
			res->res.freq = 2407 + 5 * iwe->u.freq.m;
			return;
		} else if (iwe->u.freq.m == 14) {
			res->res.freq = 2484;
			return;
		}
	}

	if (iwe->u.freq.e > 6) {
		wpa_printf(MSG_DEBUG, "Invalid freq in scan results (BSSID="
			   MACSTR " m=%d e=%d)",
			   MAC2STR(res->res.bssid), (int)iwe->u.freq.m,
			   iwe->u.freq.e);
		return;
	}

	for (i = 0; i < iwe->u.freq.e; i++)
		divi /= 10;
	res->res.freq = iwe->u.freq.m / divi;
}


static void wext_get_scan_qual(struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	res->res.qual = iwe->u.qual.qual;
	res->res.noise = iwe->u.qual.noise;
	res->res.level = iwe->u.qual.level;
}


static void wext_get_scan_encode(struct iw_event *iwe,
				 struct wext_scan_data *res)
{
	if (!(iwe->u.data.flags & IW_ENCODE_DISABLED))
		res->res.caps |= IEEE80211_CAP_PRIVACY;
}


static void wext_get_scan_rate(struct iw_event *iwe,
			       struct wext_scan_data *res, char *pos,
			       char *end)
{
	int maxrate;
	char *custom = pos + IW_EV_LCP_LEN;
	struct iw_param p;
	size_t clen;

	clen = iwe->len;
	if (custom + clen > end)
		return;
	maxrate = 0;
	while (clen >= sizeof(struct iw_param)) {
		/* Note: may be misaligned, make a local, aligned copy */
		os_memcpy(&p, custom, sizeof(struct iw_param));
		if (p.value > maxrate)
			maxrate = p.value;
		clen -= sizeof(struct iw_param);
		custom += sizeof(struct iw_param);
	}

	/* Convert the maxrate from WE-style (b/s units) to
	 * 802.11 rates (500000 b/s units).
	 */
	res->maxrate = maxrate / 500000;
}


static void wext_get_scan_iwevgenie(struct iw_event *iwe,
				    struct wext_scan_data *res, char *custom,
				    char *end)
{
	char *genie, *gpos, *gend;
	u8 *tmp;

	if (iwe->u.data.length == 0)
		return;

	gpos = genie = custom;
	gend = genie + iwe->u.data.length;
	if (gend > end) {
		wpa_printf(MSG_INFO, "IWEVGENIE overflow");
		return;
	}

	tmp = os_realloc(res->ie, res->ie_len + gend - gpos);
	if (tmp == NULL)
		return;
	os_memcpy(tmp + res->ie_len, gpos, gend - gpos);
	res->ie = tmp;
	res->ie_len += gend - gpos;
}


static void wext_get_scan_custom(struct iw_event *iwe,
				 struct wext_scan_data *res, char *custom,
				 char *end)
{
	size_t clen;
	u8 *tmp;

	clen = iwe->u.data.length;
	if (custom + clen > end)
		return;

	if (clen > 7 && os_strncmp(custom, "wpa_ie=", 7) == 0) {
		char *spos;
		int bytes;
		spos = custom + 7;
		bytes = custom + clen - spos;
		if (bytes & 1 || bytes == 0)
			return;
		bytes /= 2;
		tmp = os_realloc(res->ie, res->ie_len + bytes);
		if (tmp == NULL)
			return;
		hexstr2bin(spos, tmp + res->ie_len, bytes);
		res->ie = tmp;
		res->ie_len += bytes;
	} else if (clen > 7 && os_strncmp(custom, "rsn_ie=", 7) == 0) {
		char *spos;
		int bytes;
		spos = custom + 7;
		bytes = custom + clen - spos;
		if (bytes & 1 || bytes == 0)
			return;
		bytes /= 2;
		tmp = os_realloc(res->ie, res->ie_len + bytes);
		if (tmp == NULL)
			return;
		hexstr2bin(spos, tmp + res->ie_len, bytes);
		res->ie = tmp;
		res->ie_len += bytes;
	} else if (clen > 4 && os_strncmp(custom, "tsf=", 4) == 0) {
		char *spos;
		int bytes;
		u8 bin[8];
		spos = custom + 4;
		bytes = custom + clen - spos;
		if (bytes != 16) {
			wpa_printf(MSG_INFO, "Invalid TSF length (%d)", bytes);
			return;
		}
		bytes /= 2;
		hexstr2bin(spos, bin, bytes);
		res->res.tsf += WPA_GET_BE64(bin);
	}
}


static int wext_19_iw_point(struct wpa_driver_wext_data *drv, u16 cmd)
{
	return drv->we_version_compiled > 18 &&
		(cmd == SIOCGIWESSID || cmd == SIOCGIWENCODE ||
		 cmd == IWEVGENIE || cmd == IWEVCUSTOM);
}


static void wpa_driver_wext_add_scan_entry(struct wpa_scan_results *res,
					   struct wext_scan_data *data)
{
	struct wpa_scan_res **tmp;
	struct wpa_scan_res *r;
	size_t extra_len;
	u8 *pos, *end, *ssid_ie = NULL, *rate_ie = NULL;

	/* Figure out whether we need to fake any IEs */
	pos = data->ie;
	end = pos + data->ie_len;
	while (pos && pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == WLAN_EID_SSID)
			ssid_ie = pos;
		else if (pos[0] == WLAN_EID_SUPP_RATES)
			rate_ie = pos;
		else if (pos[0] == WLAN_EID_EXT_SUPP_RATES)
			rate_ie = pos;
		pos += 2 + pos[1];
	}

	extra_len = 0;
	if (ssid_ie == NULL)
		extra_len += 2 + data->ssid_len;
	if (rate_ie == NULL && data->maxrate)
		extra_len += 3;

	r = os_zalloc(sizeof(*r) + extra_len + data->ie_len);
	if (r == NULL)
		return;
	os_memcpy(r, &data->res, sizeof(*r));
	r->ie_len = extra_len + data->ie_len;
	pos = (u8 *) (r + 1);
	if (ssid_ie == NULL) {
		/*
		 * Generate a fake SSID IE since the driver did not report
		 * a full IE list.
		 */
		*pos++ = WLAN_EID_SSID;
		*pos++ = data->ssid_len;
		os_memcpy(pos, data->ssid, data->ssid_len);
		pos += data->ssid_len;
	}
	if (rate_ie == NULL && data->maxrate) {
		/*
		 * Generate a fake Supported Rates IE since the driver did not
		 * report a full IE list.
		 */
		*pos++ = WLAN_EID_SUPP_RATES;
		*pos++ = 1;
		*pos++ = data->maxrate;
	}
	if (data->ie)
		os_memcpy(pos, data->ie, data->ie_len);

	tmp = os_realloc(res->res,
			 (res->num + 1) * sizeof(struct wpa_scan_res *));
	if (tmp == NULL) {
		os_free(r);
		return;
	}
	tmp[res->num++] = r;
	res->res = tmp;
}
				      

/**
 * wpa_driver_wext_get_scan_results - Fetch the latest scan results
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * Returns: Scan results on success, -1 on failure
 */
struct wpa_scan_results * wpa_driver_wext_get_scan_results(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	size_t len;
	int first;
	u8 *res_buf;
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end, *custom;
	struct wpa_scan_results *res;
	struct wext_scan_data data;

	res_buf = wpa_driver_wext_giwscan(drv, &len);
	if (res_buf == NULL)
		return NULL;
	
	first = 1;

	res = os_zalloc(sizeof(*res));
	if (res == NULL) {
		os_free(res_buf);
		return NULL;
	}

	pos = (char *) res_buf;
	end = (char *) res_buf + len;
	os_memset(&data, 0, sizeof(data));

	while (pos + IW_EV_LCP_LEN <= end) {
		/* Event data may be unaligned, so make a local, aligned copy
		 * before processing. */
		os_memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
		if (iwe->len <= IW_EV_LCP_LEN)
			break;

		custom = pos + IW_EV_POINT_LEN;
		if (wext_19_iw_point(drv, iwe->cmd)) {
			/* WE-19 removed the pointer from struct iw_point */
			char *dpos = (char *) &iwe_buf.u.data.length;
			int dlen = dpos - (char *) &iwe_buf;
			os_memcpy(dpos, pos + IW_EV_LCP_LEN,
				  sizeof(struct iw_event) - dlen);
		} else {
			os_memcpy(&iwe_buf, pos, sizeof(struct iw_event));
			custom += IW_EV_POINT_OFF;
		}

		switch (iwe->cmd) {
		case SIOCGIWAP:
			if (!first)
				wpa_driver_wext_add_scan_entry(res, &data);
			first = 0;
			os_free(data.ie);
			os_memset(&data, 0, sizeof(data));
			os_memcpy(data.res.bssid,
				  iwe->u.ap_addr.sa_data, ETH_ALEN);
			break;
		case SIOCGIWMODE:
			wext_get_scan_mode(iwe, &data);
			break;
		case SIOCGIWESSID:
			wext_get_scan_ssid(iwe, &data, custom, end);
			break;
		case SIOCGIWFREQ:
			wext_get_scan_freq(iwe, &data);
			break;
		case IWEVQUAL:
			wext_get_scan_qual(iwe, &data);
			break;
		case SIOCGIWENCODE:
			wext_get_scan_encode(iwe, &data);
			break;
		case SIOCGIWRATE:
			wext_get_scan_rate(iwe, &data, pos, end);
			break;
		case IWEVGENIE:
			wext_get_scan_iwevgenie(iwe, &data, custom, end);
			break;
		case IWEVCUSTOM:
			wext_get_scan_custom(iwe, &data, custom, end);
			break;
		}

		pos += iwe->len;
	}
	os_free(res_buf);
	res_buf = NULL;
	if (!first)
		wpa_driver_wext_add_scan_entry(res, &data);
	os_free(data.ie);

	wpa_printf(MSG_DEBUG, "Received %lu bytes of scan results (%lu BSSes)",
		   (unsigned long) len, (unsigned long) res->num);

	return res;
}


static int wpa_driver_wext_get_range(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	DV_REQ dvreq;
	struct iw_range *range;
	int minlen;
	size_t buflen;

	os_memset(&dvreq, 0, sizeof(dvreq));

	/*
	 * Use larger buffer than struct iw_range in order to allow the
	 * structure to grow in the future.
	 */
	buflen = sizeof(struct iw_range) + 500;
	range = os_zalloc(buflen);
	if (range == NULL)
		return -1;

	/* Set custom parameter in the DV_REQ structure. */
	dvreq.dvr_value_ptr_1 = range;
	dvreq.dvr_value_1 = buflen;

	minlen = ((char *) &range->enc_capa) - (char *) range +
		sizeof(range->enc_capa);

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_GET_RANGE,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCGIWRANGE]");
		os_free(range);
		return -1;
	} else if (dvreq.dvr_value_1 >= minlen &&
		   range->we_version_compiled >= 18) {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: WE(compiled)=%d "
			   "WE(source)=%d enc_capa=0x%x",
			   range->we_version_compiled,
			   range->we_version_source,
			   (unsigned int)range->enc_capa);
		drv->has_capability = 1;
		drv->we_version_compiled = range->we_version_compiled;
		if (range->enc_capa & IW_ENC_CAPA_WPA) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK;
		}
		if (range->enc_capa & IW_ENC_CAPA_WPA2) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
		}
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_WEP40 |
			WPA_DRIVER_CAPA_ENC_WEP104;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_TKIP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_TKIP;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_CCMP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_CCMP;
		if (range->enc_capa & IW_ENC_CAPA_4WAY_HANDSHAKE)
			drv->capa.flags |= WPA_DRIVER_FLAGS_4WAY_HANDSHAKE;

		wpa_printf(MSG_DEBUG, "  capabilities: key_mgmt 0x%x enc 0x%x "
			   "flags 0x%x",
			   drv->capa.key_mgmt, drv->capa.enc, drv->capa.flags);
	} else {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: too old (short) data - "
			   "assuming WPA is not supported");
	}

	os_free(range);
	return 0;
}


static int wpa_driver_wext_set_wpa(void *priv, int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	return wpa_driver_wext_set_auth_param(drv, IW_AUTH_WPA_ENABLED,
					      enabled);
}


static int wpa_driver_wext_set_psk(struct wpa_driver_wext_data *drv,
				   const u8 *psk)
{
	struct iw_encode_ext *ext;
	int ret;
	DV_REQ dvreq;

	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	if (!(drv->capa.flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE))
		return 0;

	if (!psk)
		return 0;

	ext = os_zalloc(sizeof(*ext) + PMK_LEN);
	if (ext == NULL)
		return -1;

	dvreq.dvr_value_ptr_1 = ext;
	dvreq.dvr_value_1 = sizeof(*ext) + PMK_LEN;
	ext->key_len = PMK_LEN;
	os_memcpy(&ext->key, psk, ext->key_len);
	ext->alg = IW_ENCODE_ALG_PMK;

	ret = DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_ENCODEEXT,
				(VOID*)&dvreq, sizeof(DV_REQ));
	if (ret < 0)
		perror("ioctl[SIOCSIWENCODEEXT] PMK");
	os_free(ext);

	return ret;
}


static int wpa_driver_wext_set_key_ext(void *priv, wpa_alg alg,
				       const u8 *addr, int key_idx,
				       int set_tx, const u8 *seq,
				       size_t seq_len,
				       const u8 *key, size_t key_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	struct iw_encode_ext *ext;
	DV_REQ dvreq;

	if (seq_len > IW_ENCODE_SEQ_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: Invalid seq_len %lu",
			   __FUNCTION__, (unsigned long) seq_len);
		return -1;
	}

	ext = os_zalloc(sizeof(*ext) + key_len);
	if (ext == NULL)
		return -1;

	dvreq.dvr_flags = key_idx + 1;
	dvreq.dvr_flags |= IW_ENCODE_TEMP;
	if (alg == WPA_ALG_NONE)
		dvreq.dvr_flags |= IW_ENCODE_DISABLED;
	dvreq.dvr_value_ptr_1 = ext;
	dvreq.dvr_value_1 = sizeof(*ext) + key_len;

	if (addr == NULL ||
		os_memcmp(addr, "\xff\xff\xff\xff\xff\xff", ETH_ALEN) == 0)
		ext->ext_flags |= IW_ENCODE_EXT_GROUP_KEY;
	if (set_tx)
		ext->ext_flags |= IW_ENCODE_EXT_SET_TX_KEY;

	ext->addr.sa_family = ARPHRD_ETHER;
	if (addr)
		os_memcpy(ext->addr.sa_data, addr, ETH_ALEN);
	else
		os_memset(ext->addr.sa_data, 0xff, ETH_ALEN);
	if (key && key_len) {
		os_memcpy(ext + 1, key, key_len);
		ext->key_len = key_len;
	}
	switch (alg) {
	case WPA_ALG_NONE:
		ext->alg = IW_ENCODE_ALG_NONE;
		break;
	case WPA_ALG_WEP:
		ext->alg = IW_ENCODE_ALG_WEP;
		break;
	case WPA_ALG_TKIP:
		ext->alg = IW_ENCODE_ALG_TKIP;
		break;
	case WPA_ALG_CCMP:
		ext->alg = IW_ENCODE_ALG_CCMP;
		break;
	case WPA_ALG_PMK:
		ext->alg = IW_ENCODE_ALG_PMK;
		break;
#ifdef CONFIG_IEEE80211W
	case WPA_ALG_IGTK:
		ext->alg = IW_ENCODE_ALG_AES_CMAC;
		break;
#endif /* CONFIG_IEEE80211W */
	default:
		wpa_printf(MSG_DEBUG, "%s: Unknown algorithm %d",
			   __FUNCTION__, alg);
		os_free(ext);
		return -1;
	}

	if (seq && seq_len) {
		ext->ext_flags |= IW_ENCODE_EXT_RX_SEQ_VALID;
		os_memcpy(ext->rx_seq, seq, seq_len);
	}

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_ENCODEEXT,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		ret = -1;
		perror("ioctl[SIOCSIWENCODEEXT]");
	}

	os_free(ext);
	return ret;
}


/**
 * wpa_driver_wext_set_key - Configure encryption key
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @priv: Private driver interface data
 * @alg: Encryption algorithm (%WPA_ALG_NONE, %WPA_ALG_WEP,
 *	%WPA_ALG_TKIP, %WPA_ALG_CCMP); %WPA_ALG_NONE clears the key.
 * @addr: Address of the peer STA or ff:ff:ff:ff:ff:ff for
 *	broadcast/default keys
 * @key_idx: key index (0..3), usually 0 for unicast keys
 * @set_tx: Configure this key as the default Tx key (only used when
 *	driver does not support separate unicast/individual key
 * @seq: Sequence number/packet number, seq_len octets, the next
 *	packet number to be used for in replay protection; configured
 *	for Rx keys (in most cases, this is only used with broadcast
 *	keys and set to zero for unicast keys)
 * @seq_len: Length of the seq, depends on the algorithm:
 *	TKIP: 6 octets, CCMP: 6 octets
 * @key: Key buffer; TKIP: 16-byte temporal key, 8-byte Tx Mic key,
 *	8-byte Rx Mic Key
 * @key_len: Length of the key buffer in octets (WEP: 5 or 13,
 *	TKIP: 32, CCMP: 16)
 * Returns: 0 on success, -1 on failure
 *
 * This function uses SIOCSIWENCODEEXT by default, but tries to use
 * SIOCSIWENCODE if the extended ioctl fails when configuring a WEP key.
 */
int wpa_driver_wext_set_key(void *priv, wpa_alg alg,
			    const u8 *addr, int key_idx,
			    int set_tx, const u8 *seq, size_t seq_len,
			    const u8 *key, size_t key_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	DV_REQ dvreq;

	wpa_printf(MSG_DEBUG, "%s: alg=%d key_idx=%d set_tx=%d seq_len=%lu "
		   "key_len=%lu",
		   __FUNCTION__, alg, key_idx, set_tx,
		   (unsigned long) seq_len, (unsigned long) key_len);

	ret = wpa_driver_wext_set_key_ext(drv, alg, addr, key_idx, set_tx,
					  seq, seq_len, key, key_len);
	if (ret == 0)
		return 0;

	if (ret == -2 &&
	    (alg == WPA_ALG_NONE || alg == WPA_ALG_WEP)) {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT, trying SIOCSIWENCODE");
		ret = 0;
	} else {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT");
		return ret;
	}

	dvreq.dvr_flags = key_idx + 1;
	dvreq.dvr_flags |= IW_ENCODE_TEMP;
	if (alg == WPA_ALG_NONE)
		dvreq.dvr_flags |= IW_ENCODE_DISABLED;
	dvreq.dvr_value_ptr_1 = (void *)key;
	dvreq.dvr_value_1 = key_len;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_ENCODE,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	if (set_tx && alg != WPA_ALG_NONE) {
		dvreq.dvr_flags = key_idx + 1;
		dvreq.dvr_flags |= IW_ENCODE_TEMP;
		dvreq.dvr_value_ptr_1 = NULL;
		dvreq.dvr_value_1 = 0;
		if (DVC_Dev_Ioctl(drv->dev_handle, 
					drv->wifi_base + WIRELESS_CMD_SET_ENCODE,
					(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
			perror("ioctl[SIOCSIWENCODE] (set_tx)");
			ret = -1;
		}
	}

	return ret;
}


static int wpa_driver_wext_set_countermeasures(void *priv,
					       int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	return wpa_driver_wext_set_auth_param(drv,
					      IW_AUTH_TKIP_COUNTERMEASURES,
					      enabled);
}


static int wpa_driver_wext_set_drop_unencrypted(void *priv,
						int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	drv->use_crypt = enabled;
	return wpa_driver_wext_set_auth_param(drv, IW_AUTH_DROP_UNENCRYPTED,
					      enabled);
}


static int wpa_driver_wext_mlme(struct wpa_driver_wext_data *drv,
				const u8 *addr, int cmd, int reason_code)
{
	struct iw_mlme mlme;
	int ret = 0;
	DV_REQ dvreq;

	os_memset(&mlme, 0, sizeof(mlme));
	mlme.cmd = cmd;
	mlme.reason_code = reason_code;
	mlme.addr.sa_family = ARPHRD_ETHER;
	os_memcpy(mlme.addr.sa_data, addr, ETH_ALEN);
	dvreq.dvr_value_ptr_1 = &mlme;
	dvreq.dvr_value_1 = sizeof(mlme);

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_MLME,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWMLME]");
		ret = -1;
	}

	return ret;
}


static void wpa_driver_wext_disconnect(struct wpa_driver_wext_data *drv)
{
	const u8 null_bssid[ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
	u8 ssid[32];
	int i;

	/*
	 * Clear the BSSID selection and set a random SSID to make sure the
	 * driver will not be trying to associate with something even if it
	 * does not understand SIOCSIWMLME commands (or tries to associate
	 * automatically after deauth/disassoc).
	 */
	wpa_driver_wext_set_bssid(drv, null_bssid);

	for (i = 0; i < 32; i++)
		ssid[i] = rand() & 0xFF;
	wpa_driver_wext_set_ssid(drv, ssid, 32);
}


static int wpa_driver_wext_deauthenticate(void *priv, const u8 *addr,
					  int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	wpa_driver_wext_disconnect(drv);
	ret = wpa_driver_wext_mlme(drv, addr, IW_MLME_DEAUTH, reason_code);
	return ret;
}


static int wpa_driver_wext_disassociate(void *priv, const u8 *addr,
					int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	ret = wpa_driver_wext_mlme(drv, addr, IW_MLME_DISASSOC, reason_code);
	wpa_driver_wext_disconnect(drv);
	return ret;
}


static int wpa_driver_wext_set_gen_ie(void *priv, const u8 *ie,
				      size_t ie_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	DV_REQ dvreq;

	dvreq.dvr_value_ptr_1 = (void *)ie;
	dvreq.dvr_value_1 = ie_len;

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_GENIE,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWGENIE]");
		ret = -1;
	}

	return ret;
}


int wpa_driver_wext_cipher2wext(int cipher)
{
	switch (cipher) {
	case CIPHER_NONE:
		return IW_AUTH_CIPHER_NONE;
	case CIPHER_WEP40:
		return IW_AUTH_CIPHER_WEP40;
	case CIPHER_TKIP:
		return IW_AUTH_CIPHER_TKIP;
	case CIPHER_CCMP:
		return IW_AUTH_CIPHER_CCMP;
	case CIPHER_WEP104:
		return IW_AUTH_CIPHER_WEP104;
	default:
		return 0;
	}
}


int wpa_driver_wext_keymgmt2wext(int keymgmt)
{
	switch (keymgmt) {
	case KEY_MGMT_802_1X:
	case KEY_MGMT_802_1X_NO_WPA:
		return IW_AUTH_KEY_MGMT_802_1X;
	case KEY_MGMT_PSK:
		return IW_AUTH_KEY_MGMT_PSK;
	default:
		return 0;
	}
}


static int
wpa_driver_wext_auth_alg_fallback(struct wpa_driver_wext_data *drv,
				  struct wpa_driver_associate_params *params)
{
	int ret = 0;
	DV_REQ dvreq;

	wpa_printf(MSG_DEBUG, "WEXT: Driver did not support "
		   "SIOCSIWAUTH for AUTH_ALG, trying SIOCSIWENCODE");

	/* Just changing mode, not actual keys */
	dvreq.dvr_flags = 0;
	dvreq.dvr_value_ptr_1 = NULL;
	dvreq.dvr_value_1 = 0;

	/*
	 * Note: IW_ENCODE_{OPEN,RESTRICTED} can be interpreted to mean two
	 * different things. Here they are used to indicate Open System vs.
	 * Shared Key authentication algorithm. However, some drivers may use
	 * them to select between open/restricted WEP encrypted (open = allow
	 * both unencrypted and encrypted frames; restricted = only allow
	 * encrypted frames).
	 */

	if (!drv->use_crypt) {
		dvreq.dvr_flags |= IW_ENCODE_DISABLED;
	} else {
		if (params->auth_alg & AUTH_ALG_OPEN_SYSTEM)
			dvreq.dvr_flags |= IW_ENCODE_OPEN;
		if (params->auth_alg & AUTH_ALG_SHARED_KEY)
			dvreq.dvr_flags |= IW_ENCODE_RESTRICTED;
	}

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_ENCODE,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	return ret;
}


int wpa_driver_wext_associate(void *priv,
			      struct wpa_driver_associate_params *params)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	int allow_unencrypted_eapol;
	int value;

	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	/*
	 * If the driver did not support SIOCSIWAUTH, fallback to
	 * SIOCSIWENCODE here.
	 */
	if (drv->auth_alg_fallback &&
	    wpa_driver_wext_auth_alg_fallback(drv, params) < 0)
		ret = -1;

	if (!params->bssid &&
	    wpa_driver_wext_set_bssid(drv, NULL) < 0)
		ret = -1;

	/* TODO: should consider getting wpa version and cipher/key_mgmt suites
	 * from configuration, not from here, where only the selected suite is
	 * available */
	if (wpa_driver_wext_set_gen_ie(drv, params->wpa_ie, params->wpa_ie_len)
	    < 0)
		ret = -1;
	if (params->wpa_ie == NULL || params->wpa_ie_len == 0)
		value = IW_AUTH_WPA_VERSION_DISABLED;
	else if (params->wpa_ie[0] == WLAN_EID_RSN)
		value = IW_AUTH_WPA_VERSION_WPA2;
	else
		value = IW_AUTH_WPA_VERSION_WPA;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_WPA_VERSION, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->pairwise_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_PAIRWISE, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->group_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_GROUP, value) < 0)
		ret = -1;
	value = wpa_driver_wext_keymgmt2wext(params->key_mgmt_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_KEY_MGMT, value) < 0)
		ret = -1;
	value = params->key_mgmt_suite != KEY_MGMT_NONE ||
		params->pairwise_suite != CIPHER_NONE ||
		params->group_suite != CIPHER_NONE ||
		params->wpa_ie_len;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_PRIVACY_INVOKED, value) < 0)
		ret = -1;

	/* Allow unencrypted EAPOL messages even if pairwise keys are set when
	 * not using WPA. IEEE 802.1X specifies that these frames are not
	 * encrypted, but WPA encrypts them when pairwise keys are in use. */
	if (params->key_mgmt_suite == KEY_MGMT_802_1X ||
	    params->key_mgmt_suite == KEY_MGMT_PSK)
		allow_unencrypted_eapol = 0;
	else
		allow_unencrypted_eapol = 1;

	if (wpa_driver_wext_set_psk(drv, params->psk) < 0)
		ret = -1;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_RX_UNENCRYPTED_EAPOL,
					   allow_unencrypted_eapol) < 0)
		ret = -1;
#ifdef CONFIG_IEEE80211W
	switch (params->mgmt_frame_protection) {
	case NO_MGMT_FRAME_PROTECTION:
		value = IW_AUTH_MFP_DISABLED;
		break;
	case MGMT_FRAME_PROTECTION_OPTIONAL:
		value = IW_AUTH_MFP_OPTIONAL;
		break;
	case MGMT_FRAME_PROTECTION_REQUIRED:
		value = IW_AUTH_MFP_REQUIRED;
		break;
	};
	if (wpa_driver_wext_set_auth_param(drv, IW_AUTH_MFP, value) < 0)
		ret = -1;
#endif /* CONFIG_IEEE80211W */
	if (params->freq && wpa_driver_wext_set_freq(drv, params->freq) < 0)
		ret = -1;
	if (wpa_driver_wext_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		ret = -1;
	if (params->bssid &&
	    wpa_driver_wext_set_bssid(drv, params->bssid) < 0)
		ret = -1;

	return ret;
}


static int wpa_driver_wext_set_auth_alg(void *priv, int auth_alg)
{
	struct wpa_driver_wext_data *drv = priv;
	int algs = 0, res;

	if (auth_alg & AUTH_ALG_OPEN_SYSTEM)
		algs |= IW_AUTH_ALG_OPEN_SYSTEM;
	if (auth_alg & AUTH_ALG_SHARED_KEY)
		algs |= IW_AUTH_ALG_SHARED_KEY;
	if (auth_alg & AUTH_ALG_LEAP)
		algs |= IW_AUTH_ALG_LEAP;
	if (algs == 0) {
		/* at least one algorithm should be set */
		algs = IW_AUTH_ALG_OPEN_SYSTEM;
	}

	res = wpa_driver_wext_set_auth_param(drv, IW_AUTH_80211_AUTH_ALG,
					     algs);
	drv->auth_alg_fallback = res == -2;
	return res;
}


/**
 * wpa_driver_wext_set_mode - Set wireless mode (infra/adhoc), SIOCSIWMODE
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @mode: 0 = infra/BSS (associate with an AP), 1 = adhoc/IBSS
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_mode(void *priv, int mode)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = -1;
	unsigned int new_mode = mode ? IW_MODE_ADHOC : IW_MODE_INFRA;
	DV_REQ dvreq;

	dvreq.dvr_value_1 = new_mode;
	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_MODE,
				(VOID*)&dvreq, sizeof(DV_REQ)) == 0) {
		ret = 0;
	}

	return ret;
}


static int wpa_driver_wext_pmksa(struct wpa_driver_wext_data *drv,
				 u32 cmd, const u8 *bssid, const u8 *pmkid)
{
	struct iw_pmksa pmksa;
	int ret = 0;
	DV_REQ dvreq;

	os_memset(&pmksa, 0, sizeof(pmksa));
	pmksa.cmd = cmd;
	pmksa.bssid.sa_family = ARPHRD_ETHER;
	if (bssid)
		os_memcpy(pmksa.bssid.sa_data, bssid, ETH_ALEN);
	if (pmkid)
		os_memcpy(pmksa.pmkid, pmkid, IW_PMKID_LEN);
	dvreq.dvr_value_ptr_1 = &pmksa;
	dvreq.dvr_value_1 = sizeof(pmksa);

	if (DVC_Dev_Ioctl(drv->dev_handle, 
				drv->wifi_base + WIRELESS_CMD_SET_PMKSA,
				(VOID*)&dvreq, sizeof(DV_REQ)) < 0) {
		ret = -1;
	}

	return ret;
}


static int wpa_driver_wext_add_pmkid(void *priv, const u8 *bssid,
				     const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_ADD, bssid, pmkid);
}


static int wpa_driver_wext_remove_pmkid(void *priv, const u8 *bssid,
		 			const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_REMOVE, bssid, pmkid);
}


static int wpa_driver_wext_flush_pmkid(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_FLUSH, NULL, NULL);
}


static const u8 * wpa_driver_wext_get_mac_addr(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	STATUS status;
    DV_DEVICE_ENTRY *device;

	wpa_printf(MSG_DEBUG, "%s", __func__);

	if ((drv->own_addr == NULL) || (drv->ifname == NULL)) {
		wpa_printf(MSG_DEBUG, "Invalid pointers in get_mac_addr");
		return NULL;
	}

	/* Obtain the TCP semaphore of the NET stack to protect the
	 * stack global variables. This function uses NET internals to
	 * map the device name to an IP address so this semaphore must
	 * be obtained. This operation is not possible using just the
	 * user APIs.
	 */
	status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

	if (status == NU_SUCCESS) {
		/* Get a pointer to the device structure. */
		device = DEV_Get_Dev_By_Name(drv->ifname);

		if (device != NU_NULL)
			os_memcpy(drv->own_addr, device->dev_mac_addr, ETH_ALEN);
		else
			wpa_printf(MSG_DEBUG, "Unable to find specified device");

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WEXT: Unable to release TCP semaphore");
	}
	else
		wpa_printf(MSG_ERROR, "WEXT: Unable to obtain TCP semaphore");

	return drv->own_addr;
}


static int wpa_driver_wext_send_eapol(void *priv, const u8 *dest,
					u16 proto, const u8 *data, size_t data_len)
{
	struct wpa_driver_wext_data *drv = priv;
	STATUS status;
	DV_DEVICE_ENTRY *dev;
	NET_BUFFER *net_buf_ptr;
	ARP_MAC_HEADER mac_info;
	UINT32 dev_flags;

	wpa_hexdump(MSG_MSGDUMP, "wext_send_eapol TX frame", data, data_len);

	/* Obtain the TCP semaphore of the NET stack to protect the
	 * stack global variables. This function uses NET internals
	 * so this semaphore must be obtained.
	 */
	status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

	if (status == NU_SUCCESS) {
		/* Get a pointer to the device structure. */
		dev = DEV_Get_Dev_By_Name(drv->ifname);

		if (dev != NU_NULL) {
			/* Initializing the pseudo-MAC header structure. */

			/* Set the size of this MAC layer information structure. */
			mac_info.ar_len = sizeof(ARP_MAC_HEADER);

			/* Set the family type to be unspecified, so that the MAC layer
			 * send function knows that this is not an IP datagram and
			 * there is no need to resolve the hardware address.
			 */
			mac_info.ar_family = SK_FAM_UNSPEC;

			/* Copy the destination hardware address. */
			NU_BLOCK_COPY(mac_info.ar_mac.ar_mac_ether.dest,
						  dest, ETH_ALEN);

			/* Copy the source hardware address. */
			NU_BLOCK_COPY(mac_info.ar_mac.ar_mac_ether.me,
						  dev->dev_mac_addr, ETH_ALEN);

			/* Type of this packet. */
			mac_info.ar_mac.ar_mac_ether.type = proto;

			/* Allocate NET buffers for the new packet. */
			net_buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
											   data_len + dev->dev_hdrlen);

			if (net_buf_ptr != NU_NULL) {
				/* Set the data pointer to the correct location. */
				net_buf_ptr->data_ptr =
					net_buf_ptr->mem_parent_packet + dev->dev_hdrlen;

				/* Copy the EAPOL packet on the buffer chain. */
				if (MEM_Copy_Data(net_buf_ptr,
								 (const CHAR *)data, data_len, 0)
								 == data_len) {
					/* Set the deallocation list pointer. */
					net_buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

					/* Back-up the old device flags and then temporarily
					 * set the DV_UP and DV_RUNNING flags because we
					 * need to send EAPOL packets before the device is
					 * fully up. */
					dev_flags = dev->dev_flags;
					dev->dev_flags |= (DV_UP | DV_RUNNING);

					/* Send out the packet to the Authenticator. */
					status = (*dev->dev_output)(net_buf_ptr, dev,
											   (VOID *)&mac_info,
												NU_NULL);

					/* Restore the device flags. */
					dev->dev_flags = dev_flags;

					if (status != NU_SUCCESS)
						/* Free up the buffers. */
						MEM_Multiple_Buffer_Chain_Free(net_buf_ptr);
				} else {
					/* Free up the buffers. */
					MEM_Multiple_Buffer_Chain_Free(net_buf_ptr);

					/* Set the error status. */
					status = -1;
				}
			} else {
				/* Set the error status. */
				status = -1;
			}
		} else {
			wpa_printf(MSG_DEBUG, "Interface %s device not found",
				drv->ifname);
		}

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WEXT: Failed to release TCP semaphore");
	} else {
		wpa_printf(MSG_ERROR, "WEXT: Unable to obtain TCP semaphore");
	}

	return 0;
}


int wpa_driver_wext_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct wpa_driver_wext_data *drv = priv;
	if (!drv->has_capability)
		return -1;
	os_memcpy(capa, &drv->capa, sizeof(*capa));
	return 0;
}


int wpa_driver_wext_alternative_ifindex(struct wpa_driver_wext_data *drv,
					const char *ifname)
{
	if (ifname == NULL) {
		drv->ifindex2 = -1;
		return 0;
	}

	drv->ifindex2 = NU_IF_NameToIndex(ifname);
	if (drv->ifindex2 <= 0)
		return -1;

	wpa_printf(MSG_DEBUG, "Added alternative ifindex %d (%s) for "
		   "wireless events", drv->ifindex2, ifname);

	return 0;
}


int wpa_driver_wext_set_operstate(void *priv, int state)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_DEBUG, "%s: operstate %d->%d (%s)",
		   __func__, drv->operstate, state, state ? "UP" : "DORMANT");
	drv->operstate = state;
	return wpa_driver_wext_send_oper_ifla(
		drv, -1, state ? IF_OPER_UP : IF_OPER_DORMANT);
}


int wpa_driver_wext_get_version(struct wpa_driver_wext_data *drv)
{
	return drv->we_version_compiled;
}


const struct wpa_driver_ops wpa_driver_wext_ops = {
	"wext", /* name */
	"Linux wireless extensions (generic)", /* desc */
	wpa_driver_wext_get_bssid, /* get_bssid */
	wpa_driver_wext_get_ssid, /* get_ssid */
	wpa_driver_wext_set_wpa, /* set_wpa */
	wpa_driver_wext_set_key, /* set_key */
	wpa_driver_wext_init, /* init */
	wpa_driver_wext_deinit, /* deinit */
	wpa_driver_wext_set_param, /* set_param */
	wpa_driver_wext_set_countermeasures, /* set_countermeasures */
	wpa_driver_wext_set_drop_unencrypted, /* set_drop_unencrypted */
	wpa_driver_wext_scan, /* scan */
	NULL,/* get_scan_results */
	wpa_driver_wext_deauthenticate, /* deauthenticate */
	wpa_driver_wext_disassociate, /* disassociate */
	wpa_driver_wext_associate, /* associate */
	wpa_driver_wext_set_auth_alg, /* set_auth_alg */
	wpa_driver_wext_add_pmkid, /* add_pmkid */
	wpa_driver_wext_remove_pmkid, /* remove_pmkid */
	wpa_driver_wext_flush_pmkid, /* flush_pmkid */
	wpa_driver_wext_get_capa, /* get_capa */
	NULL, /* poll */
	NULL, /* get_ifname */
	wpa_driver_wext_get_mac_addr, /* get_mac_addr */
	wpa_driver_wext_send_eapol, /* send_eapol */
	wpa_driver_wext_set_operstate, /* set_operstate */
	NULL, /* mlme_setprotection */
	NULL, /* get_hw_feature_data */
	NULL, /* set_channel */
	NULL, /* set_ssid */
	NULL, /* set_bssid */
	NULL, /* send_mlme */
	NULL, /* mlme_add_sta */
	NULL, /* mlme_remove_sta */
	NULL, /* update_ft_ies */
	NULL, /* send_ft_action */
	wpa_driver_wext_get_scan_results, /* get_scan_results2 */
	NULL, /* set_probe_req_ie */
	wpa_driver_wext_set_mode, /* set_mode */
};
