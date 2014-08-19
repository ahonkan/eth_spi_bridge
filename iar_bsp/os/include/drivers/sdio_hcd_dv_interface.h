/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       sdio_hcd_dv_interface.h
*
*   COMPONENT
*
*       SDIO                              - SDIO Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the SDIO Library Driver module.
*
*************************************************************************/
#ifndef SDIO_HCD_DV_INTERFACE_H
#define SDIO_HCD_DV_INTERFACE_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


/*********************************/
/* Data Structures               */
/*********************************/

typedef struct _hcd_spisdio_instance_struct
{
    INT                     mode;
    const CHAR              *key;
    BOOLEAN                 device_in_use;
    VOID                    *target_info_ptr;
    INT                     open_cnt;
    DV_DEV_ID               dev_id;
    VOID                    *sdio_hcd_reserved; 
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

} SDIO_HCD_INSTANCE, SDIO_HCD_SESSION;

/*********************************/
/* MACROS                        */
/*********************************/
#define SDIO_HCD_MAX_MODE_NUM           3

/* Power States */
#define SDIO_HCD_OFF                    0
#define SDIO_HCD_ON                     1

/* Open Modes */
#define SDIO_HCD_BASE                   (DV_IOCTL0 + 1)

/* SDIO base defines  */
#define SDIO_HCD_POWER_BASE             (SDIO_HCD_BASE + SDIO_CMD_DELIMITER)
#define SDIO_HCD_UII_BASE               (SDIO_HCD_POWER_BASE + POWER_IOCTL_TOTAL)

/* SDIO total power states */
#define SDIO_TOTAL_STATE_COUNT          2

#define SDIO_HCD_OPEN_MODE              0x1

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS Sdio_Hcd_Dv_Register(const CHAR *key, SDIO_HCD_INSTANCE *instance_handle, DV_DEV_ID *dev_id_ptr);
STATUS Sdio_Hcd_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS Sdio_Hcd_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                       INT label_cnt, VOID* *session_handle);
STATUS Sdio_Hcd_Dv_Close(VOID *sess_handle);
STATUS Sdio_Hcd_Dv_Ioctl(VOID *session_ptr, INT ioctl_cmd, VOID *data, INT length);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !SDIO_HCD_DV_INTERFACE_H */
