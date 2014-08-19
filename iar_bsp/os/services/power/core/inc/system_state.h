/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains private data structure definitions and  
*       constants of PMS system state services subcomponent.
*
***********************************************************************/
#ifndef PMS_SYSTEM_STATE_H
#define PMS_SYSTEM_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PM_MAX_SYSTEM_STATES  10
#define INVALID_SYSTEM_STATE  0xFF

/* Global System-wide State structure */
typedef struct PM_SYSTEM_GLOBAL_STATE_STRUCT
{    
    CS_NODE      *request_list_ptr;
    UINT8        system_current_state;
    UINT8        latest_system_set_state;
    UINT8        system_state_count;       
    UINT32       total_state_requests;
    UINT32       total_emergency_requests;
    NU_PROTECT   request_list_protect;  
}PM_SYSTEM_GLOBAL_STATE;

/* Request structure for the System */
typedef struct PM_SYSTEM_REQUEST_STRUCT
{
    CS_NODE      system_request_list;
    UINT8        requested_min_state;
}PM_SYSTEM_REQUEST;

/* Request structure for the System */
typedef struct PM_EMERGENCY_REQUEST_STRUCT
{
    CS_NODE      emergency_request_list;
    UINT8        requested_max_state;
}PM_EMERGENCY_REQUEST;

/* Structure used by each system state*/
typedef struct PM_SYSTEM_STATE_STRUCT
{
    CS_NODE      *map_list_ptr;
    UINT8        total_devices;
    NU_PROTECT   map_list_protect;
}PM_SYSTEM_STATE;

/* System State Map Table */
typedef struct PM_SYSTEM_STATE_MAP_STRUCT
{
    CS_NODE      system_map_list;
    DV_DEV_ID    device;
    PM_STATE_ID  device_state;
}PM_SYSTEM_STATE_MAP;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_SYSTEM_STATE_H */


