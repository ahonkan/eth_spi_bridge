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
*   FILE NAME
*
*       phy.h
*
*   COMPONENT
*
*       PHY - Ethernet PHY Driver
*
*   DESCRIPTION
*
*       This file contains constant definitions and function macros
*       for the PHY Class Driver module.
*
*   DATA STRUCTURES
*
*       PHY_CTRL
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*
*************************************************************************/
#ifndef PHY_H
#define PHY_H

#include "nucleus.h"
#include "networking/externs.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* PHY Link Status Definitions */
#define PHY_LINK_DOWN               0       /* PHY link is down */
#define PHY_LINK_UP                 1       /* PHY link is up */
#define PHY_LINK_INVALID_READ       -1      /* Invalid read of PHY link status from PHY register */

/* Connection speed definitions */
#define __10_MBPS                   10
#define __100_MBPS                  100
#define __1000_MBPS                 1000

/* Duplex mode definitions */
#define HALF_DUPLEX                 11
#define FULL_DUPLEX                 22

/* Define limit for maximum PHY address. */
#define PHY_MAX_PHY_ADDR            32

/* PHY loop constant definitions */
#define PHY_AUTONEGOT_TRIES         10000000

#define PHY_NEGOT_100MBPS           NU_TRUE

#define PHY_NEGOT_FULL_DUPLEX       NU_TRUE

/* Number of ticks between each poll interval */
#define PHY_POLL_INTERVAL           50

/**************************************************************/
/* BASIC PHY TRANCEIVER DEFINES                               */
/**************************************************************/

/* Register offsets */
#define PHY_BMCR           0       /* Basic Mode Control Register */
#define PHY_BMSR           1       /* Basic Mode Status Register */
#define PHY_PHYIDR1        2       /* PHY Identifier Register #1 */
#define PHY_PHYIDR2        3       /* PHY Identifier Register #2 */
#define PHY_ANAR           4       /* Auto-Negotiation Advertisement Register */
#define PHY_ANLPAR         5       /* Auto-Negotiation Link Partner Ability Register (Base Page) */
#define PHY_ANLPARNP       5       /* Auto-Negotiation Link Partner Ability Register (Next Page) */
#define PHY_ANER           6       /* Auto-Negotiation Expansion Register */

/* Register values/bit defines */
#define PHY_BMCR_RESET       0x8000
#define PHY_BMCR_LOOPBACK    0x4000
#define PHY_BMCR_100MB       0x2000
#define PHY_BMCR_AUTO_NEG    0x1000
#define PHY_BMCR_POWER_DOWN  0x0800
#define PHY_BMCR_ISOLATE     0x0400
#define PHY_BMCR_RESTART     0x0200
#define PHY_BMCR_FULL_DUPLEX 0x0100
#define PHY_BMCR_COLL_TEST   0x0080

#define PHY_BMSR_100MBPS_FULLD  0x4000
#define PHY_BMSR_100MBPS_HALFD  0x2000
#define PHY_BMSR_10MBPS_FULLD   0x1000
#define PHY_BMSR_10MBPS_HALFD   0x0800
#define PHY_BMSR_AUTO_NEG       0x0020  
#define PHY_BMSR_LINK_STATUS    0x0004  /* Link Status bit */

#define PHY_PHYIDR1_OUI_MSB    0x2000
#define PHY_PHYIDR2_OUI_LSB    0x5C00 
#define PHY_PHYIDR2_VAL        0xB8A0
#define PHY_INVALID_PHYID      0xFFFF




typedef struct _phy_ctrl_struct
{
    /* MAC - Ethernet device that uses this PHY */
    UINT32         dev_io_addr;
    UINT32         dev_number;
    DV_DEV_ID      dev_id;

    /* PHY */ 
    UINT16         phy_dev;                 /* Different PHYs may be present on the same platform */
    UINT16         phy_addr;                /* PHY address assigned to the MAC */
    UINT32         phy_io_addr;             /* PHY IO address to access PHY registers */
    UINT32         phy_polled_link;         /* PHY uses polled method for link status check */
    NU_TIMER       *phy_timer;              /* Timer for polled link */

    INT            phy_link_status;         /* Up or down */
    UINT16         phy_speed;               /* 10BT, 100BT, .. */
    UINT16         phy_mode;                /* Duplex, Half duplex */

    STATUS         (*mii_read) (DV_DEVICE_ENTRY *, INT, INT, UINT16 *);
    STATUS         (*mii_write) (DV_DEVICE_ENTRY *, INT, INT, UINT16);


} PHY_CTRL;

/* Public function prototypes */
STATUS  PHY_Initialize(PHY_CTRL *phy_ctrl, DV_DEVICE_ENTRY *device);
INT     PHY_Get_Link_Status(PHY_CTRL *phy_ctrl);
STATUS  PHY_Auto_Negotiate (PHY_CTRL *phy_ctrl, DV_DEVICE_ENTRY *device, 
                            UINT32 retries, INT *mode, INT *speed);
STATUS  PHY_Read(UINT32 phy_io_addr, UINT16 phy_addr, UINT16 reg_id, UINT16 *phy_data);
STATUS  PHY_Write(UINT32 phy_io_addr, UINT16 phy_addr, UINT16 reg_id, UINT16 phy_data);
VOID    PHY_Notify_Status_Change (PHY_CTRL *phy_ctrl);
VOID    PHY_HISR(VOID);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif /* PHY_H */
