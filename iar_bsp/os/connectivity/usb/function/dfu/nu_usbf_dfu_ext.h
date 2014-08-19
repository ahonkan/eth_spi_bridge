/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   nu_usbf_dfu_ext.h
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver.
*
* DESCRIPTION
*
*   This file contains prototypes for the external interfaces, data
*   structures and macros exposed by DFU Class Driver.
*
* DATA STRUCTURES
*
*   NU_USBF_DFU_USR_CALLBK
*   NU_USBF_DFU
*
* FUNCTIONS
*
*   nu_os_conn_usb_func_dfu_init    This function initializes the 
*                                   device firmware upgrade component.
*   NU_USBF_DFU_Create              Creates an instance of DFU class 
*                                   driver.
*   _NU_USBF_DFU_Delete             Deletes an instance of DFU class 
*                                   driver.
*   NU_USBF_DFU_Register            This API provides and interface to 
*                                   the upper layer user modules, like 
*                                   firmware handler, to be registered 
*                                   with the DFU class diver.
*   NU_USBF_DFU_Unregister          This API provides an interface to 
*                                   deregister the registered user driver.
*   NU_USBF_DFU_Set_Status          Set the status of the DFU from the 
*                                   user driver.
*   NU_USBF_DFU_Get_State           This API provides an interface to the 
*                                   user driver to get the DFU state.
*   NU_USBF_DFU_Get_Status          This API provides an interface to the 
*                                   user driver to get the DFU status.
*   NU_USBF_DFU_DM_Open             This function is called by an 
*                                   application through DM when it want
*                                   to bind DFU interface with device.
*   NU_USBF_DFU_DM_Close            This funciton is called by an 
*                                   application through DM when it want
*                                   to unbind DFU interface with device.
*   NU_USBF_DFU_DM_Read             This function is called by an 
*                                   applicaiton through DM when it want 
*                                   to read data from device.
*   NU_USBF_DFU_DM_Write            This funciton is called by an 
*                                   application through DM when it want 
*                                   to write data to device.
*   NU_USBF_DFU_DM_IOCTL            This funciton is called by an
*                                   application through DM for performing
*                                   I/O control operations.
*
* DEPENDENCIES
*
*   nu_usbf_dfu_imp.h       Internal functions
*
**************************************************************************/

/* =====================================================================*/
#ifndef _NU_USBF_DFU_EXT_H_

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_DFU_EXT_H_

/*========================= Header files ===============================*/

#include "nu_usbf_dfu_imp.h"

/* ================== USB Function DFU User IOCTLS =====================*/
#define USBF_DFU_IOCTL_SET_STATUS       0
#define USBF_DFU_IOCTL_GET_STATE        1
#define USBF_DFU_IOCTL_GET_STATUS       2
#define USBF_DFU_IOCTL_REG_CALLBACK     3
#define USBF_DFU_IOCTL_UNREG_CALLBACK   4

/*=======================Function Prototypes============================*/

STATUS nu_os_conn_usb_func_dfu_init(
                                    CHAR        *path,
                                    INT         startstop);

STATUS NU_USBF_DFU_Create (
                           NU_USBF_DFU          *ctrl_blk,
                           CHAR                 *name);
STATUS _NU_USBF_DFU_Delete (
                           VOID                 *ctrl_blk);
STATUS NU_USBF_DFU_Register(
                           NU_USBF_DFU            *cb,
                           NU_USBF_DFU_USR_CALLBK *dfu_usr_callbk,
                           VOID                   *dfu_fwh_data);
STATUS NU_USBF_DFU_Unregister(
                           NU_USBF_DFU            *cb);
STATUS NU_USBF_DFU_Set_Status(
                           NU_USBF_DFU            *cb,
                           UINT8                  dfu_status);
STATUS NU_USBF_DFU_Get_State(
                           NU_USBF_DFU            *cb,
                           UINT8                  *dfu_state);
STATUS NU_USBF_DFU_Get_Status(
                           NU_USBF_DFU            *cb,
                           UINT8                  *dfu_status);

STATUS NU_USBF_DFU_Get_Handle(
                        VOID                      **handle);

STATUS NU_USBF_DFU_Bind_Interface(USBF_FUNCTION *dfu_func);

STATUS NU_USBF_DFU_Unbind_Interface(USBF_FUNCTION *dfu_func);

STATUS NU_USBF_DFU_DM_Open (VOID* instance_handle, DV_DEV_LABEL label_list[],
                          INT label_cnt, VOID** session_handle);

STATUS NU_USBF_DFU_DM_Close(VOID* instance_handle);

STATUS NU_USBF_DFU_DM_Read(
                           VOID*    dev_handle,
                           VOID*    buffer,
                           UINT32   numbyte,
                           OFFSET_T byte_offset,
                           UINT32   *bytes_read_ptr);

STATUS NU_USBF_DFU_DM_Write(
                            VOID*        dev_handle,
                            const VOID*  buffer,
                            UINT32       numbyte,
                            OFFSET_T     byte_offset,
                            UINT32       *bytes_written_ptr);

STATUS NU_USBF_DFU_DM_IOCTL (
                             VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len);

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif /* _NU_USBF_DFU_EXT_H_ */

/*============================  End Of File  ===========================*/
