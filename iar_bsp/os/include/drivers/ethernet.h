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
*       ethernet.h
*
*   COMPONENT
*
*       ETHERNET - Ethernet Class Driver
*
*   DESCRIPTION
*
*       This file contains constant definitions and function macros
*       for the Ethernet Class Driver module.
*
*   DATA STRUCTURES
*
*       ETHERNET_CONFIG_PATH
*       ETHERNET_ISR_INFO
*
*   DEPENDENCIES
*
*       externs.h
*       reg_api.h
*
*************************************************************************/
#ifndef ETHERNET_H
#define ETHERNET_H

#include "networking/externs.h"
#include "services/reg_api.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*******************/
/* Configurable    */
/*******************/
/* Maximum number of Ethernet sessions/instances */
#define ETHERNET_MAX_INSTANCES          2
#define ETHERNET_MAX_SESSIONS          (1 + ETHERNET_MAX_INSTANCES)

/* Standard GUID's for Ethernet class */
#define ETHERNET_LABEL       {0xa7,0x34,0x5a,0x9d,0xda,0x39,0x48,0x8e,0xb4,0xdc,0xa8,0x1a,0x97,0x38,0x94,0x83}

/****************************************/
/* ETHERNET FRAME SIZE DEFINES          */
/****************************************/

#define ETHERNET_MAC_ADDR_SIZE          6       /* Six-byte MAC address. */
#define ETHERNET_MTU                    1500    /* Max size of ethernet frame. */
#define ETHERNET_HDR_SIZE               14      /* Size of ethernet header. */
#define ETHERNET_TRL_SIZE               4       /* Size of ethernet trailer (FCS). */
#define ETHERNET_MAX_FRAME_SIZE         (ETHERNET_MTU + ETHERNET_HDR_SIZE + ETHERNET_TRL_SIZE)

#define ETHERNET_HISR_MEM_SIZE          CFG_NU_OS_DRVR_ETH_HISR_STACK_SIZE
#define ETHERNET_HISR_PRIORITY          0

typedef struct _ethernet_config_path_struct
{
    CHAR    config_path[REG_MAX_KEY_LENGTH];
    UINT32  max_path_len;

} ETHERNET_CONFIG_PATH;

typedef struct _ethernet_isr_info_struct
{
    /* HISR */
    NU_HISR       *tx_hisr_cb;
    VOID          (*tx_hisr_func)(VOID);
    NU_HISR       *rx_hisr_cb;
    VOID          (*rx_hisr_func)(VOID);
    NU_HISR       *er_hisr_cb;
    VOID          (*er_hisr_func)(VOID);
    NU_HISR       *phy_hisr_cb;
    VOID          (*phy_hisr_func)(VOID);

    /* LISR */
    INT           tx_irq;
    VOID          (*tx_lisr_func)(INT);
    INT           rx_irq;
    VOID          (*rx_lisr_func)(INT);
    INT           er_irq;
    VOID          (*er_lisr_func)(INT);
    INT           phy_irq;
    VOID          (*phy_lisr_func)(INT);

} ETHERNET_ISR_INFO;

/* Ethernet Ioctl commands */
#define IOCTL_ETHERNET_BASE             (DV_IOCTL0+1)
#define ETHERNET_CMD_GET_XDATA          0 
#define ETHERNET_CMD_SET_DEV_HANDLE     1 
#define ETHERNET_CMD_GET_DEV_STRUCT     2 
#define ETHERNET_CMD_TARGET_INIT        3 
#define ETHERNET_CMD_PHY_INIT           4 
#define ETHERNET_CMD_CTRL_INIT          5 
#define ETHERNET_CMD_GET_ISR_INFO       6
#define ETHERNET_CMD_GET_CONFIG_PATH    7
#define ETHERNET_CMD_CTRL_ENABLE        8
#define ETHERNET_CMD_SEND_LINK_STATUS   9
#define ETHERNET_CMD_GET_LINK_STATUS    10
#define ETHERNET_CMD_PWR_HIB_RESTORE    11
#define ETHERNET_CMD_SET_HW_ADDR        12
#define ETHERNET_CMD_GET_HW_ADDR        13
#define TOTAL_ETHERNET_IOCTLS           14

/* Net Ioctl commands */
#define IOCTL_NET_BASE                  (IOCTL_ETHERNET_BASE + TOTAL_ETHERNET_IOCTLS)
#define ETHERNET_CMD_DEV_ADDMULTI       0
#define ETHERNET_CMD_DEV_DELMULTI       1
#define ETHERNET_CMD_OFFLOAD_CTRL       2
#define ETHERNET_CMD_OFFLOAD_CAP        3
#define ETHERNET_CMD_REMDEV             4
#define ETHERNET_CMD_SET_VLAN_TAG       5 
#define ETHERNET_CMD_SET_VLAN_RX_MODE   6 
#define ETHERNET_CMD_GET_VLAN_TAG       7
#define TOTAL_NET_IOCTLS                8

/* Error Codes */
#define NU_ETHERNET_SUCCESS                 0
#define NU_ETHERNET_SESSION_UNAVAILABLE    -1

/* Public function prototypes */
DV_DEV_HANDLE ETHERNET_Open (CHAR *regpath, DV_DEV_ID eth_dev_id, UINT32 eth_flags);
STATUS        ETHERNET_Initialize (DV_DEVICE_ENTRY *device);
VOID          ETHERNET_Create_Name (CHAR *name);


#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif /* ETHERNET_H */
