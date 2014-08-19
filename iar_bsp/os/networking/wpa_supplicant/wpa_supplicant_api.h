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
*	FILENAME
*
*		wpa_supplicant_api.h
*
*	DESCRIPTION
*
*		This is the only file which needs to be included to access
*		the various Nucleus-specific APIs of the WPA Supplicant.
*
*	DATA STRUCTURES
*
*		None
*
*	FUNCTIONS
*
*		None
*
*	DEPENDENCIES
*
*		None
*
*************************************************************************/
#ifndef WPA_SUPPLICANT_API_H
#define WPA_SUPPLICANT_API_H

#ifdef __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* Function prototypes. */
STATUS WPA_Supplicant_Initialize(NU_MEMORY_POOL *mem_pool);
STATUS WPA_Supplicant_Shutdown(VOID);
STATUS WPA_Supplicant_Reload_Config(VOID);
INT WPA_Supplicant_Get_Iface_Count(VOID);

/* Internally used utility functions. */

STATUS WIFI_Dev_Register_CB(DV_DEV_ID, VOID*);
STATUS WIFI_Dev_Unregister_CB(DV_DEV_ID, VOID*);
STATUS WIFI_Dev_Suspend_Iface(DV_DEV_ID device_id);
STATUS WIFI_Dev_Resume_Iface(DV_DEV_ID device_id);

void eloop_handle_signal(int sig);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef WPA_SUPPLICANT_API_H */
