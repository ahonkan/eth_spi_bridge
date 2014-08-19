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
*       This file contains private data structure definitions, 
*       constants and functions of PMS peripheral state services 
*       subcomponent.
*
***********************************************************************/
#ifndef PMS_PERIPHERAL_H
#define PMS_PERIPHERAL_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

/* Internal error value for device handle */
#define PMS_EMPTY_DEV_HANDLE  -1

/* Device Information Structure */
typedef struct PM_DEVICE_INFO_STRUCT
{
    CS_NODE         *request_list_ptr;
    DV_DEV_ID       dev_id;
    PM_STATE_ID     current_state;
    PM_STATE_ID     latest_set_state;
    UINT32          max_state_count;
    DV_DEV_HANDLE   dev_handle;
    INT             dev_power_base;
    NU_PROTECT      request_list_protect;
    UINT32          total_state_requests;
}PM_DEVICE_INFO;

/* Request List Structure for each device */
typedef struct PM_REQUEST_STRUCT
{
    CS_NODE      state_request_list;
    PM_STATE_ID  requested_min_state;
}PM_REQUEST;


STATUS PMS_Peripheral_Get_Device_Index(DV_DEV_ID dev_id, INT *periph_index_ptr);

#ifdef          __cplusplus

}
#endif /* _cplusplus */

#endif /* PMS_PERIPHERAL_H */
