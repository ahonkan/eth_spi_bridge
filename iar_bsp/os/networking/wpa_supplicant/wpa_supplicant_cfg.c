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

/*************************************************************************
*
*   FILENAME
*
*       wpa_supplicant_cfg.c
*
*   DESCRIPTION
*
*       This file contains data structures related to the
*		WPA Supplicant build-time configuration.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       includes.h
*       common.h
*       wpa_supplicant_i.h
*       wpa_supplicant_cfg.h
*
*************************************************************************/
#include "includes.h"
#include "common.h"
#include "wpa_supplicant/wpa_supplicant_i.h"
#include "wpa_supplicant_cfg.h"

/* List of wpa_supplicant configurations per interface. This list
 * of interfaces allows the user to specify an interface-specific
 * configuration, which is particularly useful if multiple wireless
 * interfaces are present.
 *
 * One of the configurations (the last in the list) is usually the
 * wildcard configuration, which matches all interface names. This
 * list entry has a value of "*" for the "ifname" field.
 *
 * Following is a description of each configuration field:
 * conf_name:		This option is specified by "-c" in the command-line
 *					client. This is a string specifying the complete path
 *					to the wpa_supplicant configuration file.
 * ctrl_interface:	This option is specified by "-C" in the command-line
 *					client. This is a string specifying the name of the
 *					global control interface path. It should only be used
 *					if the "conf_name" parameter is not specified, because
 *					otherwise, this setting is specified in the
 *					configuration file.
 * driver:			This option is specified by "-D" in the command-line
 *					client. It specifies the driver to use for the
 *					specified interface. The driver name for Nucleus
 *					based interfaces is "nucleus_wext".
 * driver_param:	This option is specified by "-p" in the command-line
 *					client. It specifies an optional string which contains
 *					parameters to the driver. This should be set to NULL
 *					if not specified.
 * ifname:			This option is specified by "-i" in the command-line
 *					client. It should be the WLAN interface name which
 *					has been registered with Nucleus NET using the
 *					NU_Init_Devices() API. It can also be set to a value
 *					of "*" to match all interface names, but this wildcard
 *					entry should occur at the end of the list.  
 * bridge_ifname:	This option is specified by "-b" in the command-line
 *					client. It is a string specifying the bridge interface
 *					name.
 *
 * IMPORTANT: The list of configurations MUST be terminated by an
 *            item which contains all NULL's.
 */
struct wpa_interface WPA_Supplicant_Ifaces_Config[] =
{
	/* Wildcard configuration with ifname = "*". This should be the
	 * second-last entry in this list. All interface-specific
	 * configurations should be entered above this item. */
	{
		"wpa_sup.cfg",				/* conf_name: Path to config file. */
		NULL,						/* ctrl_interface: Control interface.*/
		"wext",						/* driver: Name of supplicant driver.*/
		NULL,						/* driver_param: Parameter to driver.*/
		"*",						/* ifname: Network interface name. */
		NULL,						/* bridge_ifname: Bridge ifname. */
	 },

	/* List terminator. Do NOT remove/modify this item. */
	{
		NULL,						/* conf_name: Path to config file. */
		NULL,						/* ctrl_interface: Control interface.*/
		NULL,						/* driver: Name of supplicant driver.*/
		NULL,						/* driver_param: Parameter to driver.*/
		NULL,						/* ifname: Network interface name. */
		NULL,						/* bridge_ifname: Bridge ifname. */
	 },
};
