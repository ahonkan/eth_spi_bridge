/**************************************************************************
*
*              Copyright 2003 Mentor Graphics Corporation
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
*       nu_usbh_nuf_imp.h

*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains function definitions for Nucleus USB Host FILE
*       Driver.
*
* DATA STRUCTURES
*
*       NUF_USBH_MSC                        Subclass information.
*       NUF_USBH_DEVICE                     Physical device control block.
*       NUF_USBH_LOG_DRIVE                  Logical device control block.
*
* FUNCTION
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_nuf_ext.h                   USB File Driver the external
*                                           interface definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef     _NU_USBH_NUF_IMP_H

#ifdef      __cplusplus
extern  "C" {                               /* C declarations in C++.    */
#endif

#define _NU_USBH_NUF_IMP_H

#include "drivers/nu_usbh_nuf_ext.h"

/* =========================  #defines  ================================ */

/* NU_USBH_NUF_DRVR is using in old version of Nucleus USB. */
#define NU_USBH_NUF_DRVR                NU_USB_USER

#define NUF_USBH_SHIFT_8                8
#define NUF_USBH_SHIFT_16               16
#define NUF_USBH_SHIFT_24               24
#define NUF_DRVR_PRESENT                -1
/* Define device types. */
#define NUF_USBH_DEV_DIRECT             0x0000
#define NUF_USBH_DEV_CDROM              0x0005
#define NUF_USBH_DEV_MASK               0x001F
#define NUF_USBH_DEV_RMB                0x80

/* Define device status. */
#define NUF_USBH_DEV_REMOVED           0x0001
#define NUF_USBH_DEV_MEDIA_READY       0x0002
#define NUF_USBH_DEV_MEDIA_NOT_READY   0x0003

/* Define length of the inquiry command buffer. */
#define NUF_USBH_LEN_INQUIRY            36

/* Define length of the request sense command buffer. */
#define NUF_USBH_LEN_REQSENSE           18

/* Define size of the command packet buffer. */
#define NUF_USBH_SIZE_CMDPKT            32

/* Define size of the command data buffer. */
#define NUF_USBH_SIZE_CMDDATA           512

/* Define size of logical sector(Do not change this value). */
#define NUF_USBH_SIZE_BLOCK             512  /* For FAT file system. */

/* Define Sense Key for NOT READY. */
#define NUF_USBH_SK_NOTREADY            2

/* Define Additional Sense Code for Media Not Present. */
#define NUF_USBH_ASC_MNOTPRST           0x3A

/* Define the number of the subclasses. */
#define NUF_USBH_NSUBCLASS              6

/* Define retry count for device initialization processing. */
#define NUF_USBH_RETRY_INIT             500

/* Define retry count for device initialization processing. */
#define NUF_USBH_RETRY_INIT_SHORT       10

/* Define delay time(ticks) for device initialization processing. */
#define NUF_USBH_TIME_INIT              2

/* Define size of the command packet buffer of Mode Sense(6). */
#define NUF_USBH_M_SENSE6_CMD_LEN       6

/* Define size of the command packet buffer of Mode Sense(10). */
#define NUF_USBH_M_SENSE10_CMD_LEN      12

/* Define size of the data packet buffer of Mode Sense(6). */
#define NUF_USBH_M_SENSE6_DATA_LEN      70

/* Define size of the data packet buffer of Mode Sense(10). */
#define NUF_USBH_M_SENSE10_DATA_LEN     70

/* Define Mode Sense(6) command OPCODE. */
#define NUF_USBH_M_SENSE6_CMD_OPCODE    0x1A

/* Define Mode Sense(10) command OPCODE. */
#define NUF_USBH_M_SENSE10_CMD_OPCODE   0x5A

/* Define Mode Sense(6) command 1C. */
#define NUF_USBH_M_SENSE6_CMD_1C        0x1C

/* Define Mode Sense(6) command 3F. */
#define NUF_USBH_M_SENSE6_CMD_3F        0x3F

/* Define Mode Sense(10) command 3F. */
#define NUF_USBH_M_SENSE10_CMD_3F       0x3F

/* Define Connect Status. */
#define NU_USBH_NUF_DRVR_CONNECT        0   /* Connect.                  */
#define NU_USBH_NUF_DRVR_DISCONNECT     1   /* Disconnect.               */

/* The number of messages. */
#define NU_USBH_NUF_EVENT_MSG_NUM       (NU_USBH_NUF_MAX_DRIVES * 2)

/* Message size in terms of UNSIGNED elements. */
#define NU_USBH_NUF_EVENT_MSG_SIZE      2

#define NUF_USBH_BYTES_PER_SECTOR       255
#define NUF_USBH_CYLENDERS_COUNT        31
#define NUF_USBH_HEAD_COUNT             255
#define NUF_USBH_SECTORS_PER_TRACK      0x3F /* The upper two bits of the 
                                                byte field are part of the 
                                                cylinders value" */

/* =====================  Data Structures  ============================= */

/* Mass Storage Class Driver function table. */
typedef struct usbh_mscd_func
{
    /* Request routine. */
    STATUS  (*request)(
            VOID          *p_handle,
            VOID          *p_command,
            UINT32         cmd_len,
            VOID          *p_buffer,
            UINT32         buf_len,
            UINT8          direction);

    /* Inquiry routine. */
    STATUS  (*inquiry)(
            VOID          *p_handle,
            UINT8         *p_command,
            VOID          *p_buffer,
            UINT8          buf_len);

    /* Unit read routine. */
    STATUS  (*unit_ready)(
            VOID          *p_handle,
            UINT8         *p_command);

    /* Read capacity routine. */
    STATUS  (*read_capacity)(
            VOID          *p_handle,
            UINT8         *p_command,
            UINT8         *p_buffer);

    /* Request sense routine. */
    STATUS  (*request_sense)(
            VOID          *p_handle,
            UINT8         *p_command,
            VOID          *p_buffer,
            UINT8          buf_len);

    /* Read routine. */
    STATUS  (*read)(
            VOID          *p_handle,
            UINT8         *p_command,
            UINT32         lba_addr,
            UINT32         count,
            VOID          *p_buffer,
            UINT32         buf_len);

    /* Write routine. */
    STATUS  (*write)(
            VOID          *p_handle,
            UINT8         *p_command,
            UINT32         lba_addr,
            UINT32         count,
            VOID          *p_buffer,
            UINT32         buf_len);

    /* Mode sense routine. */
    STATUS  (*mode_sense)(
            VOID          *p_handle,
            UINT8         *p_command,
            UINT32         lba_addr,
            UINT32         count,
            VOID          *p_buffer,
            UINT32         buf_len);

} USBH_MSCD_FUNC;

/* Device subclass information structure. */
typedef struct usbh_nuf_msc
{
    UINT8            *cmd_pkt;              /* Command packet buffer.    */
    UINT8            *cmd_data;             /* Command data buffer.      */

    /* Pointer to an memory pool. */


    /* Mass Storage Class driver functions.   */
    USBH_MSCD_FUNC    func;
    UINT8             subclass;             /* Subclass code.            */

    DATA_ELEMENT      cs_padding[3];

} NUF_USBH_MSC;

/* Device information structure. */
typedef struct usbh_nuf_device
{
    CS_NODE            dev_link;
    /* Physical device name.   */
    CHAR               dev_name[FPART_MAX_PHYS_NAME];
    PM_STATE_ID        pm_state;
    /* Device handle registered mass storage driver.  */
    VOID               *dev_handle;
    VOID               *data_buf;           /* Data buffer               */

    UINT32              block_len;          /* Block length in sector.   */
    UINT32              last_lba;           /* Last LBA.                 */

    struct usbh_nuf_device    *device;
    NUF_USBH_MSC              *dev_msc;
    NU_SEMAPHORE               rw_lock;
    UINT16                     dev_type;    /* Device type.              */
    UINT16                     media_status;       /* Device media status.      */
    UINT8                      medium;
    UINT8                     *rw_buffer;  /* Buffer used for uncached read/write. */
                                           /* Size of buffer 'USB_NUF_RW_BUFF_SIZE' */
    DATA_ELEMENT               cs_padding[2];
} NUF_USBH_DEVICE;

/* Device Polling task, that pools removable(RMB bit set) devices. */
typedef struct usbh_nuf_dev_poll_task
{
    VOID                *stack;         /* Device Polling task's stack pointer. */
    NU_TASK             task;
}USBH_NUF_DEV_POLL_TASK;

/* Points to FILE driver list. */
typedef struct
{
  /* Points to device list head. */
    NUF_USBH_DEVICE        *head_device;

    /* Semaphore for device list lock. */
    NU_SEMAPHORE           dev_list_lock;

    /* Device polling task. */
    USBH_NUF_DEV_POLL_TASK poll_task;

    /* Number of RM devices attached. */
    UINT8                  rm_dev_count;
    DATA_ELEMENT           cs_padding[3];
        
} NUF_USBH_DRVR;

/* ====================  Function Prototypes  ========================== */
STATUS NUF_USBH_Connect(
       VOID             *p_handle,
       UINT8             subclass);

STATUS NUF_USBH_Disconnect(
       VOID             *p_handle,
       UINT8             subclass);


STATUS NUF_USBH_Media_Init_Device(
       NUF_USBH_DEVICE  *pcb_device,
       VOID             *p_handle);

STATUS NUF_USBH_Media_Ready_Device(
       NUF_USBH_DEVICE  *pcb_device,
       VOID             *p_handle);

STATUS NUF_USBH_Event_2_Drives(
       VOID             *p_handle,
       INT               event_code);

STATUS NUF_USBH_Media_Io(
       NUF_USBH_DEVICE *pcb_device,
       UINT32           sector,
       VOID            *p_buffer,
       UINT16           count,
       INT              reading);

STATUS NUF_USBH_Allocate_Device_Info(
       VOID             *p_handle,
       UINT8             subclass,
       NUF_USBH_DEVICE **ppcb_device);

STATUS NUF_USBH_Deallocate_Device_Info(
       NUF_USBH_DEVICE  *dev_ptr);


STATUS NU_USBH_NUF_Set_Dev_Name(NUF_USBH_DEVICE* pcb_input_device);


STATUS NUF_USBH_Init_Polling_Task(VOID);


VOID NUF_USBH_RM_Dev_Poll_Task(
       UNSIGNED          argc,
       VOID              *param);

STATUS NUF_USBH_Aligned_Read_Uncached(
       NUF_USBH_DEVICE   *dev_ptr,
       UINT32            offset_sector,
       UINT32            size_sector,
       VOID              *buffer);

STATUS NUF_USBH_Aligned_Write_Uncached(
        NUF_USBH_DEVICE  *dev_ptr,
        UINT32           offset_sector,
        UINT32           size_sector,
        VOID             *buffer);

STATUS NUF_USBH_Init_File_Driver(VOID);

STATUS NUF_USBH_UnInit_File_Driver(VOID);

STATUS NUF_USBH_Get_Task_Status(
                                NU_TASK *taskp,
                                DATA_ELEMENT *task_status);
    
extern NUF_USBH_DRVR                  *pUSBH_File_Drvr;

extern NU_USBH_MS_APP_CALLBACKS app_callbacks;    
    
#include "drivers/nu_usbh_nuf_cfg.h"
    
/* ===================================================================== */
#ifdef      __cplusplus
}                                           /* End of C declarations.    */
#endif

#endif      /*_NU_USBH_NUF_IMP_H         */
/* =====================  End Of File  ================================= */
