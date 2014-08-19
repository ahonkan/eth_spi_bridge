/**************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*     ethernet_tgt.h
*
* DESCRIPTION
*
*     This file contains the macros and data structures for the EMAC
*     Ethernet driver.
*
**************************************************************************/
#ifndef ETHERNET_TGT_H
#define ETHERNET_TGT_H

#include "nucleus.h"
#include "drivers/nu_drivers.h"
#include "kernel/nu_kernel.h"
#include "networking/nu_networking.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#define EMAC_ONLY                               0

/* Minimum frequency (Hz) for PHY to function with 100 MPBS/Full duplex */
#define STM32_EMAC_MIN_FREQUENCY                25000000

/*****************************/
/* USER CONFIGURABLE DEFINES */
/*****************************/

/* Define number of RX descriptors.  Dedicate 60% of Networking buffers to RX (1 per descriptor) */
/* ah terabit radios
 * Dropped the number of descriptors from 60% to 20% to allow enough buffers for other
 * network interfaces.
 */
#define NUM_RX_DESC                             (UINT)(CFG_NU_OS_NET_STACK_MAX_BUFS * 0.2)

/* Define number of TX descriptors.  Only need enough descriptors to TX one full ethernet frame */
#define NUM_TX_DESC                             (UINT)((ETHERNET_MTU/CFG_NU_OS_NET_STACK_BUF_SIZE)+1)

/*****************************/
/* DRIVER INTERFACE DEFINES  */
/*****************************/

/* Size of receive buffer fixed at 128 bytes */
#define STM32_EMAC_RX_BUF_SIZE                  CFG_NU_OS_NET_STACK_BUF_SIZE

/* Macros for 32 bit I/O register access. */
#define STM32_EMAC_OUT32(addr, reg_num, data)   ((*( (volatile UINT32*) (addr + reg_num) ) ) = (UINT32) (data))
#define STM32_EMAC_IN32(addr,reg_num)           (*( (volatile UINT32*) (addr + reg_num) ) )

/***********************/
/* MULTICAST DEFINES   */
/***********************/
#define CRC_POLYNOMIAL              0x04C11DB7
#define STM32_EMAC_MCAST_ENABLE     0xFFFFFFFF

/*********************/
/*  DATA STRUCTURES  */
/*********************/
/* TX buffer descriptor structure. */
typedef struct  STM32_EMAC_TX_DESCRIPTOR_STRUCT
{
    UINT32    tdes0;
    UINT32    tdes1;
    UINT32    tdes2;
    UINT32    tdes3;

} STM32EMAC_TXDESC;

/* RX buffer descriptor structure. */
typedef struct  STM32_EMAC_RX_DESCRIPTOR_STRUCT
{
    UINT32    rdes0;
    UINT32    rdes1;
    UINT32    rdes2;
    UINT32    rdes3;

} STM32EMAC_RXDESC;

/* Structure for storing the EMAC EMAC device specific data for each device instance. */
typedef struct EMAC_XDATA_STRUCT
{
    /* Transmit and Receive HISR Control Blocks */
    NU_HISR   tx_hisr_cb;
    NU_HISR   rx_hisr_cb;
    NU_HISR   phy_hisr_cb; 

    /* Receive Frame copied to memory Counter */
    UINT32    emac_rx_count;

    /* Receive errors. */
    UINT32    emac_rx_overrun;
    UINT32    emac_rx_bna;
    UINT32    emac_rx_error;
    UINT32    emac_rx_data_xfer_error;
    UINT32    emac_rx_read_xfer_error;
    UINT32    emac_rx_write_xfer_error;
    UINT32    emac_rx_desc_access_error;
    UINT32    emac_rx_data_buffer_access_error; 

    /* Good Transmitted Frame Counter */
    UINT32    emac_tx_count;

    /* Transmit errors. */
    UINT32    emac_tx_underrun;
    UINT32    emac_tx_bna;
    UINT32    emac_tx_retry;
    UINT32    emac_tx_collision;
    UINT32    emac_tx_data_xfer_error;
    UINT32    emac_tx_read_xfer_error;
    UINT32    emac_tx_write_xfer_error;
    UINT32    emac_tx_desc_access_error;
    UINT32    emac_tx_data_buffer_access_error; 

} STM32_EMAC_XDATA;

#define	BIT_FLD_SET(bit_pos,value)	(value << bit_pos)

/******************************************/
/* REGISTER OFFSETS FOR ETH_MAC           */
/******************************************/
#define ETH_MACCR                   0x00        /* MAC configuration register */
#define ETH_MACFFR                  0x04        /* MAC frame filter register */
#define ETH_MACHTHR                 0x08        /* MAC hash table high register */
#define ETH_MACHTLR                 0x0C        /* MAC hash table low register */
#define ETH_MACMIIAR                0x10        /* MAC MII address register */
#define ETH_MACMIIDR                0x14        /* MAC MII data register */
#define ETH_MACFCR                  0x18        /* MAC flow control register */
#define ETH_MACVLANTR               0x1C        /* MAC VLAN tag register */
#define ETH_MACRWUFFR               0x28        /* MAC remote wakeup frame filter register */
#define ETH_MACPMTCSR               0x2C        /* MAC PMT control and status register */
#define ETH_MACDBGR                 0x34        /* MAC debug register */
#define ETH_MACSR                   0x38        /* MAC interrupt status register */
#define ETH_MACIMR                  0x3C        /* MAC interrupt mask register */
#define ETH_MACA0HR                 0x40        /* MAC address 0 high register */
#define ETH_MACA0LR                 0x44        /* MAC address 0 low register */
#define ETH_MACA1HR                 0x48        /* MAC address 1 high register */ 
#define ETH_MACA1LR                 0x4C        /* MAC address 1 low register */ 
#define ETH_MACA2HR                 0x50        /* MAC address 2 high register */ 
#define ETH_MACA2LR                 0x54        /* MAC address 2 low register */ 
#define ETH_MACA3HR                 0x58        /* MAC address 3 high register */ 
#define ETH_MACA3LR                 0x5C        /* MAC address 3 low register */ 
#define ETH_MMCCR                   0x100       /* MMC control register */
#define ETH_MMCRIR                  0x104       /* MMC receive interrupt register */
#define ETH_MMCTIR                  0x108       /* MMC transmit interrupt register */
#define ETH_MMCRIMR                 0x10C       /* MMC receive interrupt mask register */
#define ETH_MMCTIMR                 0x110       /* MMC transmit interrupt mask register */
#define ETH_MMCTGFSCCR              0x14C       /* MMC transmitted good frames after a single collision counter register */
#define ETH_MMCTGFMCCR              0x150       /* MMC transmitted good frames after more than a single collision counter register */ 
#define ETH_MMCTGFCR                0x168       /* MMC transmitted good fmames counter register */
#define ETH_MMCRFCECR               0x194       /* MMC receive frames with CRC error counter register */
#define ETH_MMCRFAECR               0x198       /* MMC receive frames with alignment error counter register */
#define ETH_MMCRGUFCR               0x1C4       /* MMC received good frames counter register */
#define ETH_PTPTSCR                 0x700       /* PTP time stamp control register */
#define ETH_PTPSSIR                 0x704       /* PTP subsecond increment register */  
#define ETH_PTPTSHR                 0x708       /* PTP time stamp high register */  
#define ETH_PTPTSLR                 0x70C       /* PTP time stamp low register */  
#define ETH_PTPTSHUR                0x710       /* PTP time stamp high update register */  
#define ETH_PTPTSLUR                0x714       /* PTP time stamp low update register */  
#define ETH_PTPTSAR                 0x718       /* PTP time stamp addend register */
#define ETH_PTPTTHR                 0x71C       /* PTP target time high register */  
#define ETH_PTPTTLR                 0x720       /* PTP target time low register */  
#define ETH_PTPTSSR                 0x728       /* PTP time stamp status register */   
#define ETH_PTPPPSCR                0x72C       /* PTP PPS control register */   
#define ETH_DMABMR                  0x1000      /* DMA bus mode register */
#define ETH_DMATPDR                 0x1004      /* DMA transmit poll demand register */
#define ETH_DMARPDR                 0x1008      /* DMA receive poll demand register */
#define ETH_DMARDLAR                0x100C      /* DMA receive descriptor list address register */
#define ETH_DMATDLAR                0x1010      /* DMA transmit descriptor list address register */ 
#define ETH_DMASR                   0x1014      /* DMA status register */
#define ETH_DMAOMR                  0x1018      /* DMA operation mode register */
#define ETH_DMAIER                  0x101C      /* DMA interrupt enable register */
#define ETH_DMAMFBOCR               0x1020      /* DMA missed frame and buffer overflow counter register */
#define ETH_DMARSWTR                0x1024      /* DMA receive status watchdog timer register */
#define ETH_DMACHTDR                0x1048      /* DMA current host transmit descriptor register */
#define ETH_DMACHRDR                0x104C      /* DMA current host receive descriptor register */ 
#define ETH_DMACHTBAR               0x1050      /* DMA current host transmit buffer address register */ 
#define ETH_DMACHRBAR               0x1054      /* DMA current host receive buffer address register */ 

/**********************************************/
/* REGISTER DEFINITION FOR ETH_MAC            */
/**********************************************/
#define	ETH_MACMIIAR_CR_PRE16       BIT_FLD_SET(2,0x02)
#define	ETH_MACMIIAR_CR_PRE62       BIT_FLD_SET(2,0x01)
#define	ETH_MACMIIAR_CR_PRE102      BIT_FLD_SET(2,0x04)
#define	ETH_MACCR_CSTF              BIT_FLD_SET(25,0x01)
#define	ETH_MACCR_WD                BIT_FLD_SET(23,0x01)
#define	ETH_MACCR_JD                BIT_FLD_SET(22,0x01)
#define ETH_MACCR_IFG(x)            BIT_FLD_SET(17,x)
#define	ETH_MACCR_CSD               BIT_FLD_SET(16,0x01)
#define	ETH_MACCR_FES               BIT_FLD_SET(14,0x01)
#define	ETH_MACCR_LM                BIT_FLD_SET(12,0x01)
#define	ETH_MACCR_DM                BIT_FLD_SET(11,0x01)
#define	ETH_MACCR_IPCO              BIT_FLD_SET(10,0x01)
#define	ETH_MACCR_RD                BIT_FLD_SET(9,0x01)
#define	ETH_MACCR_APCS              BIT_FLD_SET(7,0x01)
#define	ETH_MACCR_TE                BIT_FLD_SET(3,0x01)
#define	ETH_MACCR_RE                BIT_FLD_SET(2,0x01)
#define ETH_MACFFR_RX_ALL           BIT_FLD_SET(31,0x01)
#define ETH_MACFFR_HPF              BIT_FLD_SET(10,0x01)
#define ETH_MACFFR_SAF              BIT_FLD_SET(9,0x01)
#define ETH_MACFFR_SAIF             BIT_FLD_SET(8,0x01)
#define ETH_MACFFR_BFD              BIT_FLD_SET(5,0x01)
#define ETH_MACFFR_PAM              BIT_FLD_SET(4,0x01)
#define ETH_MACFFR_DAIF	            BIT_FLD_SET(3,0x01)
#define ETH_MACFFR_HM               BIT_FLD_SET(2,0x01)
#define ETH_MACFFR_HU               BIT_FLD_SET(1,0x01)
#define ETH_MACFFR_PM               BIT_FLD_SET(0,0x01)
#define ETH_MACFCR_ZQPD             BIT_FLD_SET(7,0x01)
#define ETH_MACFCR_TFCE             BIT_FLD_SET(1,0x01)
#define ETH_DMAOMR_DTCEFD           BIT_FLD_SET(26,0x01)
#define ETH_DMAOMR_RSF              BIT_FLD_SET(25,0x01)
#define ETH_DMAOMR_DFRF	            BIT_FLD_SET(24,0x01)
#define ETH_DMAOMR_TSF              BIT_FLD_SET(21,0x01)
#define ETH_DMAOMR_FTF              BIT_FLD_SET(20,0x01)
#define ETH_DMAOMR_ST               BIT_FLD_SET(13,0x01)
#define ETH_DMAOMR_FEF              BIT_FLD_SET(7,0x01)
#define ETH_DMAOMR_FUGF             BIT_FLD_SET(6,0x01)
#define ETH_DMAOMR_OSF              BIT_FLD_SET(2,0x01)
#define ETH_DMAOMR_SR               BIT_FLD_SET(1,0x01)
#define ETH_DMABMR_MB               BIT_FLD_SET(26,0x01)
#define ETH_DMABMR_AAB              BIT_FLD_SET(25,0x01)
#define ETH_DMABMR_FPM              BIT_FLD_SET(24,0x01)
#define ETH_DMABMR_USP              BIT_FLD_SET(23,0x01)
#define ETH_DMABMR_RDP(x)           BIT_FLD_SET(17,x)
#define ETH_DMABMR_FB               BIT_FLD_SET(16,0x01)
#define ETH_DMABMR_PBL(x)           BIT_FLD_SET(8,x)
#define ETH_DMABMR_EDFE             BIT_FLD_SET(7,0x01)
#define ETH_DMABMR_DSL              BIT_FLD_SET(2,0x01)
#define ETH_DMABMR_DA               BIT_FLD_SET(1,0x01)
#define ETH_DMABMR_SR               BIT_FLD_SET(0,0x01)
#define ETH_DMASR_EBS_DA            BIT_FLD_SET(25,0x01)
#define ETH_DMASR_EBS_RD            BIT_FLD_SET(24,0x01)
#define ETH_DMASR_EBS_TXDMA         BIT_FLD_SET(23,0x01)
#define ETH_DMASR_NIS               BIT_FLD_SET(16,0x01)
#define ETH_DMASR_AIS               BIT_FLD_SET(15,0x01)
#define ETH_DMASR_FBES              BIT_FLD_SET(13,0x01)
#define ETH_DMASR_ETS               BIT_FLD_SET(10,0x01)
#define ETH_DMASR_RBUS              BIT_FLD_SET(7,0x01)
#define ETH_DMASR_RS                BIT_FLD_SET(6,0x01)
#define ETH_DMASR_ROS               BIT_FLD_SET(4,0x01)
#define ETH_DMASR_TBUS              BIT_FLD_SET(2,0x01)
#define ETH_DMASR_TS                BIT_FLD_SET(0,0x01)
#define ETH_DMAIER_NISE             BIT_FLD_SET(16,0x01)
#define ETH_DMAIER_RIE              BIT_FLD_SET(6,0x01)
#define ETH_DMAIER_TIE              BIT_FLD_SET(0,0x01)
#define ETH_MACMIIAR_CR(x)          BIT_FLD_SET(2,x)
#define ETH_MACMIIAR_MII_READ       BIT_FLD_SET(0,0x01)
#define ETH_MACMIIAR_MII_WRITE      BIT_FLD_SET(1,0x01)
#define ETH_MACMIIAR_MII_BUSY       BIT_FLD_SET(0,0x01)

/**********************************************/
/* DESCRIPTOR DEFINITION FOR ETH_MAC          */
/**********************************************/
/* EMAC receive descriptor related defines. */
#define STM32_EMAC_RX_DESC_OWN_BIT  0x80000000  /* OWN bit */ 
#define STM32_EMAC_RX_DESC_FL_MSK   0x3FFF0000  /* Frame length */ 
#define STM32_EMAC_RX_DESC_FS_BIT   0x00000200  /* First descriptor */ 
#define STM32_EMAC_RX_DESC_LS_BIT   0x00000100  /* Last descriptor */ 
#define STM32_EMAC_RX_DESC_RER_BIT  0x00008000  /* Receive end of ring */ 
#define STM32_EMAC_RX_DESC_RCH_BIT  0x00004000  /* Second address chained */ 

/* EMAC transmit descriptor related defines. */
#define STM32_EMAC_TX_DESC_OWN_BIT  0x80000000  /* OWN bit */
#define STM32_EMAC_TX_DESC_IC_BIT   0x40000000  /* Interrupt on completion */ 
#define STM32_EMAC_TX_DESC_LS_BIT   0x20000000  /* Last segment */ 
#define STM32_EMAC_TX_DESC_FS_BIT   0x10000000  /* First segment */ 
#define STM32_EMAC_TX_DESC_TER_BIT  0x00200000  /* Transmit end of ring */ 
#define STM32_EMAC_TX_DESC_TCH_BIT  0x00100000  /* Second address chained */ 
#define STM32_EMAC_TX_DESC_ES_BIT   0x00008000  /* Error summary bit */ 
#define STM32_EMAC_TX_DESC_EC_BIT   0x00000100  /* Excessive collision bit */ 
#define STM32_EMAC_TX_DESC_CC_MSK   0x00000078  /* Collision count mask */ 
#define STM32_EMAC_TX_DESC_UF_BIT   0x00000002  /* Underflow bit */ 

/* Definition for general failure code */
#define STM32_EMAC_FAILURE          -1

/* Public function prototypes */
VOID        nu_bsp_drvr_enet_stm32_emac_init (const CHAR * key, INT startstop);
INT         Ethernet_Tgt_Xmit_Packet (DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr);
STATUS      Ethernet_Tgt_MII_Read (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 *data);
STATUS      Ethernet_Tgt_MII_Write (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 data);

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/
STATUS      Ethernet_Tgt_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read);
STATUS      Ethernet_Tgt_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written);
VOID        Ethernet_Tgt_Enable (ETHERNET_INSTANCE_HANDLE *inst_handle);
VOID        Ethernet_Tgt_Disable (ETHERNET_INSTANCE_HANDLE *inst_handle);
STATUS      Ethernet_Tgt_Create_Extended_Data (DV_DEVICE_ENTRY *device);
VOID        Ethernet_Tgt_Target_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);
STATUS      Ethernet_Tgt_Controller_Init (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device, UINT8 *ether_addr);
VOID        Ethernet_Tgt_Get_ISR_Info (ETHERNET_SESSION_HANDLE *ses_handle, ETHERNET_ISR_INFO *isr_info);
VOID        Ethernet_Tgt_Set_Phy_Dev_ID (ETHERNET_INSTANCE_HANDLE *inst_handle);
STATUS      Ethernet_Tgt_Get_Link_Status (ETHERNET_INSTANCE_HANDLE *inst_handle, INT *link_status);
STATUS      Ethernet_Tgt_Phy_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);
VOID        Ethernet_Tgt_Notify_Status_Change (ETHERNET_INSTANCE_HANDLE *inst_handle);
VOID        Ethernet_Tgt_Update_Multicast (DV_DEVICE_ENTRY *device);
STATUS      Ethernet_Tgt_Get_Address (DV_DEVICE_ENTRY *device, UINT8 *ether_addr);
STATUS      Ethernet_Tgt_Set_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef ETHERNET_TGT_H */
