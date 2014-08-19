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
*       ethernet_dv_interface.h
*
*   COMPONENT
*
*       ETHERNET                            - Ethernet Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Ethernet Library Driver module.
*
*************************************************************************/
#ifndef ETHERNET_DV_INTERFACE_H
#define ETHERNET_DV_INTERFACE_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Incomplete typedef for functions to be included in the 
 * TGT_FUNCTIONS
 */
struct _ethernet_instance_handle_struct;
typedef struct _ethernet_instance_handle_struct ETHERNET_INSTANCE_HANDLE;

struct _ethernet_session_handle_struct;
typedef struct _ethernet_session_handle_struct ETHERNET_SESSION_HANDLE;


/* These target functions are used to abstract out the low level hardware calls
 * required to perform the requests.  This allows for various target routines
 * from different types of interfaces to be used to enable network traffic over
 * that specifc interface.  This could be spi, i2c, serial, etc.
 */
typedef struct _tgt_functions
{
    DV_DRV_READ_FUNCTION  Tgt_Read;
    DV_DRV_WRITE_FUNCTION Tgt_Write;
    
    VOID    (*Tgt_Enable)(ETHERNET_INSTANCE_HANDLE *inst_handle);

    VOID    (*Tgt_Disable)(ETHERNET_INSTANCE_HANDLE *inst_handle);

    STATUS  (*Tgt_Create_Extended_Data)(DV_DEVICE_ENTRY *device);

    VOID    (*Tgt_Target_Initialize)(ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);

    STATUS  (*Tgt_Controller_Init)(ETHERNET_INSTANCE_HANDLE *inst_handle, 
                                   DV_DEVICE_ENTRY *device, UINT8 *ether_addr);

    VOID    (*Tgt_Get_ISR_Info)(ETHERNET_SESSION_HANDLE *ses_handle, ETHERNET_ISR_INFO *isr_info);

    VOID    (*Tgt_Set_Phy_Dev_ID)(ETHERNET_INSTANCE_HANDLE *inst_handle);

    STATUS  (*Tgt_Get_Link_Status)(ETHERNET_INSTANCE_HANDLE *inst_handle, INT *link_status);

    VOID    (*Tgt_Update_Multicast)(DV_DEVICE_ENTRY *device);

    STATUS  (*Tgt_Phy_Initialize)(ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);

    VOID    (*Tgt_Notify_Status_Change)(ETHERNET_INSTANCE_HANDLE *inst_handle);

    STATUS  (*Tgt_Get_Address)(ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr);

    STATUS  (*Tgt_Set_Address)(ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    VOID        (*Tgt_Pwr_Default_State)(VOID *inst_handle);
    PMI_DRV_SET_STATE_FN  Tgt_Pwr_Set_State;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

    STATUS      (*Tgt_Pwr_Hibernate_Restore)(ETHERNET_SESSION_HANDLE * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif


} TGT_FUNCTIONS; 


typedef struct _ethernet_instance_handle_struct
{
    CHAR                   config_path[REG_MAX_KEY_LENGTH];
    CHAR                   name[10];
    UINT32                 io_addr;
    INT                    irq;
    UINT32                 irq_priority;
    ESAL_GE_INT_TRIG_TYPE  irq_type;
    INT                    phy_irq;
    UINT32                 phy_irq_priority;
    ESAL_GE_INT_TRIG_TYPE  phy_irq_type;
    UINT32                 number;
    VOID                  *phy_ctrl;
    CHAR                   ref_clock[NU_DRVR_REF_CLOCK_LEN];
    BOOLEAN                device_in_use;
    DV_DEV_ID              dev_id;
    VOID                   *ethernet_reserved; 
    VOID                   *tgt_ptr;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE         pmi_dev;
#endif

  TGT_FUNCTIONS            tgt_fn;

} ETHERNET_INSTANCE_HANDLE;

typedef struct _ethernet_session_handle_struct
{
    NU_DEVICE              eth_mw;
    DV_DEVICE_ENTRY        *device;
    UINT32                 open_modes;
    ETHERNET_INSTANCE_HANDLE   *inst_info;

} ETHERNET_SESSION_HANDLE;



/* ETHERNET IOCTL base command number */
#define IOCTL_ETHERNET_BASE             (DV_IOCTL0+1)

/* ETHERNET Power Base command number*/
#define ETHERNET_POWER_BASE               (IOCTL_ETHERNET_BASE + TOTAL_ETHERNET_IOCTLS + TOTAL_NET_IOCTLS)

/* ETHERNET Power States */
#define ETHERNET_OFF                      0
#define ETHERNET_ON                       1

/* ETHERNET total power states */
#define ETHERNET_TOTAL_STATE_COUNT        2

/* Open Mode */
#define ETHERNET_OPEN_MODE              0x1

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  Ethernet_Dv_Register (const CHAR *key, ETHERNET_INSTANCE_HANDLE *inst_handle);
STATUS  Ethernet_Dv_Unregister (DV_DEV_ID dev_id);
STATUS  Ethernet_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                          INT labels_cnt,VOID* *session_handle);
STATUS  Ethernet_Dv_Close(VOID *handle_ptr);
STATUS  Ethernet_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !ETHERNET_DV_INTERFACE_H */
