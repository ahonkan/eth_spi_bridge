/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   DESCRIPTION
*
*       This file contains generic interface specifications for Power -
*       aware device drivers
*
*************************************************************************/
#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#include "services/power_core.h"

/************************/
/*  MACROS              */
/************************/
#define PMI_STATE_GET(pmi_dev_ptr) ((PMI_DEV*)pmi_dev_ptr)->current_state
#define PMI_STATE_SET(pmi_dev_ptr, state) ((PMI_DEV*)pmi_dev_ptr)->current_state = state

#define PMI_IS_PARKED(pmi_dev_ptr)  ((PMI_DEV*)pmi_dev_ptr)->is_parked
#define PMI_IS_PARKED_SET(pmi_dev_ptr, flag) ((PMI_DEV*)pmi_dev_ptr)->is_parked = flag

#define PMI_REQUEST_MIN_OP(op_pt, pmi_dev_ptr) NU_PM_Request_Min_OP(op_pt, &((PMI_DEV*)pmi_dev_ptr)->min_op_request_handle)

#define PMI_RELEASE_MIN_OP(pmi_dev_ptr) NU_PM_Release_Min_OP(((PMI_DEV*)pmi_dev_ptr)->min_op_request_handle)

/*************************************************************************
*
*   MACRO
*
*       PMI_WAIT_CYCLE
*
*   DESCRIPTION
*
*       This macro retreived the appropriate events that cause suspension
*
*************************************************************************/
#define PMI_WAIT_CYCLE(pmi_dev, status)                                                 \
{                                                                                       \
    UNSIGNED    _retrieved_events;                                                      \
                                                                                        \
    /* Now retrieve the event to cause suspension */                                    \
    status = NU_Retrieve_Events(&(((PMI_DEV*)pmi_dev)->pwr_state_evt_grp),              \
                                  (PMI_ON_EVT | PMI_RESUME_EVT),                        \
                                  NU_AND, &_retrieved_events, NU_SUSPEND);              \
}

/*************************************************************************
*
*   MACRO
*
*       PMI_CHANGE_STATE_WAIT_CYCLE
*
*   DESCRIPTION
*
*       This macro sets the appropriate events to lift suspension
*
*************************************************************************/
#define PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev, state, status)                                 \
{                                                                                           \
    if ((state == PMI_ON_EVT) || (state == PMI_RESUME_EVT))                                 \
    {                                                                                       \
        /* Set resume flag in event group */                                                \
        status = NU_Set_Events(&(((PMI_DEV*)pmi_dev)->pwr_state_evt_grp), state, NU_OR);    \
    }                                                                                       \
                                                                                            \
    if ((state == PMI_OFF_EVT) || (state == PMI_PARKED_EVT))                                \
    {                                                                                       \
        /* Set resume flag in event group */                                                \
        status = NU_Set_Events(&(((PMI_DEV*)pmi_dev)->pwr_state_evt_grp), state, NU_AND);   \
    }                                                                                       \
}

/************************/
/*  POWER Definitions   */
/************************/

/* General error define */
#define PMI_DEV_ERROR                   -1

/* Power Open modes */
#define POWER_OPEN_MODE                 0x2
#define UII_OPEN_MODE                   0x4

/* Power event bits */
#define PMI_ON_EVT                      0x1
#define PMI_RESUME_EVT                  0x2
#define PMI_PARKED_EVT                  ~PMI_RESUME_EVT
#define PMI_OFF_EVT                     ~PMI_ON_EVT

typedef STATUS (*PMI_DRV_SET_STATE_FN)(VOID *instance_handle, PM_STATE_ID *state);
typedef STATUS (*PMI_DVFS_NOTIFY_FN)(VOID *instance_handle, PM_DVFS_NOTIFY_TYPE type);
typedef STATUS    (*PMI_DRV_PARK_RESUME_FN)(VOID *instance_handle);

/* Handle used to pass device structure between device and interface */
typedef VOID*     PMI_DEV_HANDLE;

typedef struct _pmi_dvfs_struct
{
    PM_DVFS_HANDLE              pmi_dvfs_handle;      /* Device specific DVFS Handle */
    INT                         mpl_ref_frequency;    /* Reference MPL frequency     */
    INT                         mpl_ref_park;         /* Duration in ns for device park   */
    INT                         mpl_ref_resume;       /* Duration in ns for devcie resume */
    INT                         mpl_ref_duration;     /* Duration in ns that device can be parked */
    PMI_DRV_PARK_RESUME_FN      park_pre_evt_set;     /* Device specific function that will run before the event group is changed */
    PMI_DRV_PARK_RESUME_FN      park_post_evt_set;     /* Device specific function that will run after the event group is changed */
    PMI_DRV_PARK_RESUME_FN      res_pre_evt_set;   /* Device specific function that will run before the event group and mpl update is changed */
    PMI_DRV_PARK_RESUME_FN      res_post_evt_set;   /* Device specific function that will run after the mpl is changed */
    PMI_DRV_PARK_RESUME_FN      res_complete;   /* Device specific function that will run after the event group and mpl is changed */
    VOID                        *instance_handle;

} PMI_DVFS;

typedef struct _pmi_uii_struct
{
    PM_WD_HANDLE    wd_handle;
    UINT16          wd_timeout;
    INT             uii_base;
    VOID            *uii_misc;

} PMI_UII;

/* Define USART device info structure */
typedef struct  _pmi_dev_struct
{
    PM_MIN_REQ_HANDLE     min_op_request_handle;
    PM_STATE_ID           def_pwr_state;
    PM_STATE_ID           current_state;
    BOOLEAN               is_parked;
    NU_EVENT_GROUP        pwr_state_evt_grp;
    DV_DEV_ID             *dev_id;
    INT                   power_base;
    INT                   max_power_states;
    PMI_DRV_SET_STATE_FN  dev_set_state;
    PMI_DVFS              pmi_dvfs;
    PMI_UII               pmi_uii;

} PMI_DEV;

/**********************************/
/* LOCAL FUNCTION PROTOTYPES      */
/**********************************/
VOID        PMI_Device_Setup(PMI_DEV_HANDLE pmi_dev, PMI_DRV_SET_STATE_FN set_state_func,
                             INT power_base, INT max_power_states, DV_DEV_ID *dev_id,
                             VOID *instance_handle);
STATUS      PMI_DVFS_Setup(PMI_DEV_HANDLE pmi_dev, const CHAR *key, VOID *instance_handle,
                           PMI_DRV_PARK_RESUME_FN park_pre_event_set,
                           PMI_DRV_PARK_RESUME_FN park_post_event_set,
                           PMI_DRV_PARK_RESUME_FN resume_pre_event_set,
                           PMI_DRV_PARK_RESUME_FN resume_post_event_set,
                           PMI_DRV_PARK_RESUME_FN resume_complete);
STATUS      PMI_Device_Initialize(PMI_DEV_HANDLE *pmi_dev, const CHAR *key,
                                  DV_DEV_LABEL all_device_labels[], INT *total_label_cnt, INT uii_base);
STATUS      PMI_Device_Unregister(PMI_DEV_HANDLE pmi_dev);
STATUS      PMI_Device_Open(UINT32 *mode, DV_DEV_LABEL labels_list[], INT labels_cnt);
STATUS      PMI_Device_Close(PMI_DEV_HANDLE pmi_dev);
STATUS      PMI_Device_Ioctl(PMI_DEV_HANDLE pmi_dev, INT cmd, VOID *data, INT length,
                             VOID *inst_handle, UINT32 open_modes);
STATUS      PMI_DVFS_Update_MPL_Value(PMI_DEV_HANDLE pmi_dev, PM_DVFS_NOTIFY notification);
STATUS      PMI_Min_OP_Pt_Calc(PMI_DEV_HANDLE pmi_dev, CHAR *ref_clock, UINT32 min_freq, UINT8 *min_op_pt);
STATUS      PMI_Reset_Watchdog(PMI_DEV_HANDLE pmi_dev);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif /* !POWER_INTERFACE_H */
