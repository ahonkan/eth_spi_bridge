/*
 * wpa_supplicant/hostapd - Build time configuration defines
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
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
 * This header file can be used to define configuration defines that were
 * originally defined in Makefile. This is mainly meant for IDE use or for
 * systems that do not have suitable 'make' tool. In these cases, it may be
 * easier to have a single place for defining all the needed C pre-processor
 * defines.
 */

#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

/* Insert configuration defines, e.g., #define EAP_MD5, here, if needed. */

/* Do not modify the below "CONFIG_NUCLEUS_OS" define. */
#define CONFIG_NUCLEUS_OS

/* Configuration settings for Nucleus OS. You can comment out any of
 * the options in this section to disable that module. These settings
 * are only for advanced users.
 */
#ifdef CONFIG_NUCLEUS_OS

#define CONFIG_DRIVER_WEXT
/* #define CONFIG_DRIVER_TEST */
/* #define CONFIG_NO_STDOUT_DEBUG */
#define CONFIG_CTRL_IFACE
#define CONFIG_ANSI_C_EXTRA
#define CONFIG_WPS
#define CONFIG_CTRL_IFACE_UDP
#define CTRL_IFACE_SOCKET
#define IEEE8021X_EAPOL
#define OS_NO_C_LIB_DEFINES

#define EAP_MD5
#define EAP_MSCHAPv2
#define EAP_TLS
#define EAP_TTLS
#define EAP_PEAP
#define EAP_GTC
#define EAP_OTP
#define EAP_LEAP
#define EAP_PSK
#define EAP_SAKE
#define EAP_GPSK
#define EAP_WSC
//#define EAP_FAST
//#define EAP_TNC
//#define EAP_SIM

#endif /* CONFIG_NUCLEUS_OS */

#endif /* BUILD_CONFIG_H */
