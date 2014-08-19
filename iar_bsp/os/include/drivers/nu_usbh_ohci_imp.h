/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*   nu_usbh_ohci_imp.h
*
* COMPONENT
*   Nucleus USB Host software.
*
* DESCRIPTION
*   This file contains control block and other structures for
*   NU_USBH_OHCI component(OHCI host controller).
*
* DATA STRUCTURES
*   nu_usbh_ohci        OHCI control block.
*
* FUNCTIONS
*   None.
*
* DEPENDENCIES
*   nu_usbh_hw_ext.h        NU_USBH_HW definitions.
*   nu_usbh_ohci_dat.h      OHCI HW dispatch table definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_OHCI_IMP_H_
#define _NU_USBH_OHCI_IMP_H_
/* ===================================================================== */

/* Configuration defines related to root port settings. Added for PR-367.*/
#define OHCI_PORT_CFG_POWER_GROUP 0
#define OHCI_PORT_CFG_POWER_INDIVIDUAL 1
#define OHCI_PORT_CFG_POWER_ALWAYS_ON 2
#define OHCI_PORT_CFG_OC_GROUP 0
#define OHCI_PORT_CFG_OC_INDIVIDUAL 1
#define OHCI_PORT_CFG_OC_NOT_SUPPORTED 2

/* ==================== USB Include Files ============================== */
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_ohci_cfg.h"
/* ====================  Data Types ==================================== */

#define USBH_OHCI_MAX_PEND_IRPS   10

#define USBH_OHCI_MAX_ISO_TRANS_PER_IRP 5000

#define USBH_OHCI_MAX_DATA_PER_TD 4096
#define USBH_OHCI_MAX_DATA_PER_TD_MASK (USBH_OHCI_MAX_DATA_PER_TD - 1)
/* log(USBH_OHCI_MAX_DATA_PER_TD) / log(2). */
#define USBH_OHCI_MAX_DATA_PER_TD_SHIFT 12

/* Under Configuration. */
#define USBH_OHCI_ED_NOT_READY 0
/* Configured and ready for use.*/
#define USBH_OHCI_ED_READY     1
/* Under deletion. */
#define USBH_OHCI_ED_DELETE    2
#define USBH_OHCI_ED_IN_MODIFY 3

#define OHCI_RH_DESCA_PSM (1 << 8)
#define OHCI_RH_DESCA_NPS (1 << 9)
#define OHCI_RH_DESCA_DT (1 << 10)
#define OHCI_RH_DESCA_OCPM (1 << 11)
#define OHCI_RH_DESCA_NOCP (1 << 12)

#define OHCI_RH_DESC_LENGTH 0xB
#define OHCI_RH_DESC_TYPE 0x29

#define OHCI_RH_DESC_INDEX_LENGTH 0
#define OHCI_RH_DESC_INDEX_TYPE 1
#define OHCI_RH_DESC_INDEX_NDP 2
#define OHCI_RH_DESC_INDEX_HUB_CHAR_0 3
#define OHCI_RH_DESC_INDEX_HUB_CHAR_1 4
#define OHCI_RH_DESC_INDEX_POTPGT 5
#define OHCI_RH_DESC_INDEX_CTRL_CURR 6
#define OHCI_RH_DESC_INDEX_DEV_REMOVABLE_0 7
#define OHCI_RH_DESC_INDEX_DEV_REMOVABLE_1 8
#define OHCI_RH_DESC_INDEX_PWR_CTRL_MASK_0 9
#define OHCI_RH_DESC_INDEX_PWR_CTRL_MASK_1 10

#define OHCI_RH_LENGTH_OFFSET_DEV_DESC 0
#define OHCI_RH_LENGTH_OFFSET_CONFIG_DESC 2
#define OHCI_RH_LENGTH_OFFSET_HUB_DESC 0

#define OHCI_EP_ADDR_MASK 0xF

#define OHCI_RH_STATUS_CHANGE_BMP(port_num) (0x1 << port_num)

#define OHCI_GLOBAL_STATUS_CHANGE_INDEX 0x0

#define USBH_OHCI_HASH_SIZE 16


/* Control transfer states. */
#define USBH_OHCI_CTRL_SETUP 0
#define USBH_OHCI_CTRL_DATA 1
#define USBH_OHCI_CTRL_STATUS 2

#define USBH_OHCI_TD_CANCELLED   0x10
#define USBH_OHCI_TD_RETIRED     2


#define OHCI_ED_TD_ALIGNMENT 16
#define OHCI_ISO_TD_ALIGNMENT 32
#define OHCI_HCCA_ALIGNMENT 256

#define USBH_OHCI_REVISION       0x10        /* OHCI rev 1.0 or higher. */

#define USBH_OHCI_EDCTL_FA_MASK  0x0000007f  /* Function address.      */
#define USBH_OHCI_EDCTL_FA_ROT   0
#define USBH_OHCI_EDCTL_EN_MASK  0x00000780  /* Endpoint number.       */
#define USBH_OHCI_EDCTL_EN_ROT   7
#define USBH_OHCI_EDCTL_DIR_MASK 0x00001800  /* Direction.             */
#define USBH_OHCI_EDCTL_DIR_TD   0x00000000  /* Get direction from TD. */
#define USBH_OHCI_EDCTL_DIR_OUT  0x00000800  /* OUT.                   */
#define USBH_OHCI_EDCTL_DIR_IN   0x00001000  /* Get direction from TD. */
#define USBH_OHCI_EDCTL_SPD_MASK 0x00002000  /* Speed.                 */
#define USBH_OHCI_EDCTL_SPD_FULL 0x00000000  /* Full speed device.     */
#define USBH_OHCI_EDCTL_SPD_LOW  0x00002000  /* Low speed device.      */
#define USBH_OHCI_EDCTL_SKIP     0x00004000  /* Skip ED.               */
#define USBH_OHCI_EDCTL_FMT_MASK 0x00008000  /* TD format.             */
#define USBH_OHCI_EDCTL_FMT_GEN  0x00000000  /* General TDs.           */
#define USBH_OHCI_EDCTL_FMT_ISO  0x00008000  /* ISO TDs.               */
#define USBH_OHCI_EDCTL_MPS_MASK 0x07ff0000  /* Maximum packet size.   */
#define USBH_OHCI_EDCTL_MPS_ROT  16

#define USBH_OHCI_ED_PTR_TD_TAIL   0xfffffff0 /* TD tail pointer Mask. */
#define USBH_OHCI_ED_PTR_HALTED    0x00000001 /* TD queue is halted.   */
#define USBH_OHCI_ED_PTR_TGL_CARRY 0x00000002 /* Toggle carry.         */
#define USBH_OHCI_ED_PTR_TD_HEAD   0xfffffff0 /* tdHead pointer Mask.  */
#define USBH_OHCI_ED_PTR_NEXT_ED   0xfffffff0 /* nextEd pointer Mask.  */

#define USBH_OHCI_TD_CTL_BFR_RND      0x00040000 /* Buffer rounding.    */
#define USBH_OHCI_TD_CTL_PID_MASK     0x00180000
#define USBH_OHCI_TD_CTL_PID_SETUP    0x00000000 /* SETUP.              */
#define USBH_OHCI_TD_CTL_PID_OUT      0x00080000 /* OUT.                */
#define USBH_OHCI_TD_CTL_PID_IN       0x00100000 /* IN.                 */
#define USBH_OHCI_TD_CTL_DELAY_MASK   0x00e00000 /* Delay Interrupt.    */
/* Data toggle the MSB of this field is "0" data toggle from ED. */
#define USBH_OHCI_TD_CTL_TOGGLE_DATA0 0x02000000 /* Indicates DATA0.    */
#define USBH_OHCI_TD_CTL_TOGGLE_DATA1 0x03000000 /* Indicates DATA1.    */
#define USBH_OHCI_TD_CTL_TOGGLE_USEED 0x00000000 /* Use toggle from ED. */
#define USBH_OHCI_TD_CTL_TOGGLE_USETD 0x02000000 /* Use toggle from TD. */
#define USBH_OHCI_TD_CTL_ERRCOUNT     0x0c000000 /* Error count.        */
#define USBH_OHCI_TD_CTL_CC_MASK      0xf0000000 /* Condition code.     */

#define USBH_OHCI_TD_NEXTTD      0xfffffff0  /* NextTD.  */

#define USBH_OHCI_TICTL_DI_MASK  0x00e00000  /* Delay interrupt. */
#define USBH_OHCI_TICTL_DI_ROT   21
#define USBH_OHCI_TICTL_DI_NONE  7
#define USBH_OHCI_TICTL_FC_MASK  0x07000000  /* Frame count.     */
#define USBH_OHCI_TICTL_FC_ROT   24
#define USBH_OHCI_TICTL_FC_NONE  7
#define USBH_OHCI_TICTL_CC_MASK  0xf0000000  /* Condition code.  */

#define USBH_OHCI_TD_ISO_SF       0x0000ffff /* Starting frame.  */
#define USBH_OHCI_TD_ISO_NEXTTD   0xfffffff0 /* NextTd.          */
#define USBH_OHCI_TD_ISO_PSW_SIZE 0x07ff     /* Size of packet.  */
#define USBH_OHCI_TD_ISO_PSW_CC   0xf000     /* Completion code. */

/* OHCI spec 4.3.3 completion code. */

#define USBH_OHCI_CC_NO_ERROR       0x0      /* No errors detected.    */
#define USBH_OHCI_CC_CRC            0x1      /* CRC error.             */
#define USBH_OHCI_CC_BITSTUFF       0x2      /* Bit stuffing error.    */
#define USBH_OHCI_CC_DATA_TOGGLE    0x3      /* Data toggle mismatch.  */
#define USBH_OHCI_CC_STALL          0x4      /* Endpoint stall.        */
#define USBH_OHCI_CC_NO_RESPONSE    0x5      /* Device not responding. */
#define USBH_OHCI_CC_PID_CHECK      0x6      /* PID Check bits failed. */
#define USBH_OHCI_CC_UNEXPECTED_PID 0x7      /* Unexpected PID.        */
#define USBH_OHCI_CC_DATA_OVERRUN   0x8      /* Packet exceeded MPS.   */
#define USBH_OHCI_CC_DATA_UNDERRUN  0x9      /* Packet less than MPS.  */
#define USBH_OHCI_CC_BFR_OVERRUN    0xc      /* HC buffer overrun.     */
#define USBH_OHCI_CC_BFR_UNDERRUN   0xd      /* HC buffer under run.    */
#define USBH_OHCI_CC_NOT_ACCESSED   0xe      /* Set by software.       */
#define USBH_OHCI_CC_ED_HALTED      0xf      /* The EP is halted.      */

/* This code is placed in PSW7 CC position in non ISO TDs to distinguish
 * them from SIO TDs.
 */
#define USBH_OHCI_CC_NON_ISO  0xA000
#define USBH_OHCI_CC_NON_ISO_MASK  0xF000

/* Total number of TDs for CTRL, BULK and INTR endpoints. */
#define OHCI_TOTAL_GEN_TDS (OHCI_MAX_TD_GEN_POOLS * OHCI_MAX_TDS_PER_POOL)

/* OHCI controller memory mapped I/O region.
 * OHCI spec table 7.1
 * Host Controller Operational Registers.
 * The following values are offsets relative to the base of the memory.
 */

#define USBH_OHCI_HC_REVISION         0x0
#define USBH_OHCI_HC_CONTROL          0x4
#define USBH_OHCI_HC_COMMAND_STATUS   0x8
#define USBH_OHCI_HC_INT_STATUS       0xc
#define USBH_OHCI_HC_INT_ENABLE       0x10
#define USBH_OHCI_HC_INT_DISABLE      0x14
#define USBH_OHCI_HC_HCCA             0x18
#define USBH_OHCI_HC_PERIOD_CUR_ED    0x1c
#define USBH_OHCI_HC_CONTROL_HEAD_ED  0x20
#define USBH_OHCI_HC_CONTROL_CUR_ED   0x24
#define USBH_OHCI_HC_BULK_HEAD_ED     0x28
#define USBH_OHCI_HC_BULK_CUR_ED      0x2c
#define USBH_OHCI_HC_DONE_HEAD        0x30
#define USBH_OHCI_HC_FM_INTERVAL      0x34
#define USBH_OHCI_HC_FM_REMAINING     0x38
#define USBH_OHCI_HC_FM_NUMBER        0x3c
#define USBH_OHCI_HC_PERIODIC_START   0x40
#define USBH_OHCI_HC_LS_THRESHOLD     0x44
#define USBH_OHCI_HC_RH_DESCR_A       0x48
#define USBH_OHCI_HC_RH_DESCR_B       0x4c
#define USBH_OHCI_HC_RH_STATUS        0x50
#define USBH_OHCI_HC_RH_PORT_STATUS   0x54

/* USBH_OHCI_HC_REVISION register. */
#define USBH_OHCI_RREV_REV_MASK   0x000000ff /* BCD OpenHCI revision. */

/* USBH_OHCI_HC_CONTROL register. */

/* Control/bulk service ratio. */
#define USBH_OHCI_CTL_CBSR_MASK   0x00000003
#define USBH_OHCI_CTL_CBSR_ROT    0
#define USBH_OHCI_CTL_CBSR_1TO1   0x00000000
#define USBH_OHCI_CTL_CBSR_2TO1   0x00000001
#define USBH_OHCI_CTL_CBSR_3TO1   0x00000002
#define USBH_OHCI_CTL_CBSR_4TO1   0x00000003

#define USBH_OHCI_CTL_PLE          0x00000004 /* Periodic list enable.   */
#define USBH_OHCI_CTL_IE           0x00000008 /* Isochronous list enable.*/
#define USBH_OHCI_CTL_CLE          0x00000010 /* Control list enable.    */
#define USBH_OHCI_CTL_BLE          0x00000020 /* Bulk list enable.       */

#define USBH_OHCI_CTL_HCFS_MASK    0x000000c0 /* HC functional state. */
#define USBH_OHCI_CTL_HCFS_RESET   0x00000000 /* UsbReset.            */
#define USBH_OHCI_CTL_HCFS_RESUME  0x00000040 /* UsbResume.           */
#define USBH_OHCI_CTL_HCFS_OP      0x00000080 /* UsbOperational.      */
#define USBH_OHCI_CTL_HCFS_SUSPEND 0x000000c0 /* UsbSuspend.          */

#define USBH_OHCI_CTL_IR           0x00000100 /* Interrupt routing.      */
#define USBH_OHCI_CTL_RWC          0x00000200 /* Remote wakeup connected.*/
#define USBH_OHCI_CTL_RWE          0x00000400 /* Remote wakeup enable.   */

/* HcCommandStatus Register. */
#define USBH_OHCI_CS_HCR         0x00000001  /* Host controller reset.   */
#define USBH_OHCI_CS_CLF         0x00000002  /* Control list filled.     */
#define USBH_OHCI_CS_BLF         0x00000004  /* Bulk list filled.        */
#define USBH_OHCI_CS_OCR         0x00000008  /* Ownership change request.*/
#define USBH_OHCI_CS_SOC_MASK    0x00030000  /* Scheduling overrun count.*/

/* HcInterruptStatus ,Enable Register. */
#define USBH_OHCI_INT_SO   0x00000001        /* Scheduling overrun.      */
#define USBH_OHCI_INT_WDH  0x00000002        /* Write back done head.    */
#define USBH_OHCI_INT_SF   0x00000004        /* Start of frame.          */
#define USBH_OHCI_INT_RD   0x00000008        /* Resume detected.         */
#define USBH_OHCI_INT_UE   0x00000010        /* Unrecoverable error.     */
#define USBH_OHCI_INT_FNO  0x00000020        /* Frame number overflow.   */
#define USBH_OHCI_INT_RHSC 0x00000040        /* Root hub status change.  */
#define USBH_OHCI_INT_OC   0x40000000        /* Ownership change.        */
#define USBH_OHCI_INT_MIE  0x80000000        /* Master interrupt enable. */

/* Frame counters HcFmInterval Register. */
#define USBH_OHCI_FMI_FI_MASK     0x00003fff /* Frame interval.         */
#define USBH_OHCI_FMI_FSMPS_MASK  0x7fff0000 /* FS largest data packet. */
#define USBH_OHCI_FMI_FIT         0x80000000 /* Frame interval toggle.  */

/*HcFmremaining Register. */
#define USBH_OHCI_FMR_FR_MASK  0x00003fff    /* Frame remaining.        */
#define USBH_OHCI_FMR_FRT      0x80000000    /* Frame remaining toggle. */

/* HcFmNumber Register. */
#define USBH_OHCI_FMN_FN_MASK  0x0000ffff    /* Frame number. */
#define USBH_OHCI_FMN_FN_ROT   0

/* HcPeriodic Start Register. */
#define USBH_OHCI_PS_PS_MASK    0x00003fff   /* Periodic start. */

/* HcLsThreshold Register. */
#define USBH_OHCI_LS_LST_MASK  0x000003ff    /* Low Speed threshold. */

#define USBH_OHCI_DEFAULT_FRAMEINTERVAL  0xa7782edf
#define USBH_OHCI_PERIODIC_START         0x2A20

/* Root Hub Partition. */
/* HC RhDescriptorA register. */
#define USBH_OHCI_RHDA_NDP_MASK  0x000000ff  /* No. of downstream ports. */
#define USBH_OHCI_RHDA_PSM       0x00000100  /* Power switching mode.    */
#define USBH_OHCI_RHDA_NPS       0x00000200  /* No power switching.      */
#define USBH_OHCI_RHDA_DT        0x00000400  /* Device type.             */
#define USBH_OHCI_RHDA_OCPM      0x00000800  /* OC protection mode.      */
#define USBH_OHCI_RHDA_NOCP      0x00001000  /* No OC protection mode.   */
/* Power on to Power good time. */
#define USBH_OHCI_RHDA_POTPGT_MASK 0xff000000

/* HC RhDescriptorB register. */
#define USBH_OHCI_RHDB_DR_MASK    0x0000ffff /* Device removable.        */
#define USBH_OHCI_RHDB_PPCM_MASK  0xffff0000 /* Port power control mask. */

/* HcRhStatus Register. */
#define USBH_OHCI_RHS_LPS      0x00000001    /* Local power status.      */
#define USBH_OHCI_RHS_CLR_GPWR 0x00000001    /* Clear global power.      */
#define USBH_OHCI_RHS_OCI      0x00000002    /* Over current indicator.  */
#define USBH_OHCI_RHS_DRWE     0x00008000    /* Remote wakeup enable.    */
#define USBH_OHCI_RHS_SET_RWE  0x00008000    /* Set remote wakeup.       */
#define USBH_OHCI_RHS_LPSC     0x00010000    /* Local power status change.
                                              */
#define USBH_OHCI_RHS_SET_GPWR 0x00010000    /* Set global power.        */
#define USBH_OHCI_RHS_OCIC     0x00020000    /* OC indicator change.     */
#define USBH_OHCI_RHS_CRWE     0x80000000    /* Clear remote wakeup
                                              * enable.
                                              */
#define USBH_OHCI_RHS_CLR_RWE  0x80000000    /* Clear wakeup enable.     */

/* HcRhPortStatus register. */
#define USBH_OHCI_RHPS_CCS     0x00000001    /* Current connect status.  */
#define USBH_OHCI_RHPS_CLR_PE  0x00000001    /* Clear port enable.       */
#define USBH_OHCI_RHPS_PES     0x00000002    /* Port enable status.      */
#define USBH_OHCI_RHPS_SET_PE  0x00000002    /* Set port enable.         */
#define USBH_OHCI_RHPS_PSS     0x00000004    /* Port suspend status.     */
#define USBH_OHCI_RHPS_SET_PS  0x00000004    /* Set port suspend.        */
#define USBH_OHCI_RHPS_POCI    0x00000008    /* Port OC indicator.       */
#define USBH_OHCI_RHPS_CLR_PS  0x00000008    /* Clear port suspend.      */
#define USBH_OHCI_RHPS_PRS     0x00000010    /* Port reset status.       */
#define USBH_OHCI_RHPS_SET_PRS 0x00000010    /* Port reset status.       */
#define USBH_OHCI_RHPS_PPS     0x00000100    /* Port power status.       */
#define USBH_OHCI_RHPS_SET_PWR 0x00000100    /* Set port power.          */
#define USBH_OHCI_RHPS_LSDA    0x00000200    /* Low speed device attached.
                                              */
#define USBH_OHCI_RHPS_CLR_PWR 0x00000200    /* Clear port power.        */
#define USBH_OHCI_RHPS_CSC     0x00010000    /* Connect status change.   */
#define USBH_OHCI_RHPS_PESC    0x00020000    /* Port enable status change.
                                              */
#define USBH_OHCI_RHPS_PSSC    0x00040000    /* Port suspend status change.
                                              */
#define USBH_OHCI_RHPS_OCIC    0x00080000    /* Port OC indicator change.*/
#define USBH_OHCI_RHPS_PRSC    0x00100000    /* Port reset status change.*/

#define USBH_OHCI_INT_SCHEDULE_CNT 32

#define USBH_OHCI_LSTHRESH  0x628            /* Low Speed bit threshold. */

#define USBH_RH_RECIPIENT_DEVICE 0
#define USBH_RH_RECIPIENT_INTF   1
#define USBH_RH_RECIPIENT_ENDP   2
#define USBH_RH_RECIPIENT_PORT   3

/* Root Hub Register Access.  */

/* RH Descriptor - A register.  */
#define USBH_RH_DESCA_NUM_PORTS_MASK 0xFF
#define USBH_RH_DESCA_NPS_MASK       0x100
#define USBH_RH_DESCA_PSM_MASK       0x200
#define USBH_RH_DESCA_DT_MASK        0x400
#define USBH_RH_DESCA_OCPM_MASK      0x800
#define USBH_RH_DESCA_NOCP_MASK      0x1000
#define USBH_RH_DESCA_POTPGT_MASK    0xFF000000

/* RH Descriptor - B register.  */
#define USBH_RH_DESCB_DR_MASK        0xFFFF
#define USBH_RH_DESCB_PPCM_MASK      0xFFFF0000

/* RH Status register.  */
#define USBH_RH_HUB_STATUS_MASK      0x00030003

/* RH Port Status Register.  */
#define USBH_RH_PORT_STATUS_MASK     0x001F031F

/* Maximum iteration count for reset to complete. */
#define USBH_OHCI_MAX_RST_ITERATIONS 500

#define OHCI_RH_SPRTD_SET_FEATURE_MAP (               \
                      (0x1 << USBH_HUB_FEATURE_PORT_SUSPEND) | \
                      (0x1 << USBH_HUB_FEATURE_PORT_RESET)     | \
                      (0x1 << USBH_HUB_FEATURE_PORT_POWER))

#define OHCI_RH_SPRTD_CLEAR_FEATURE_MAP (                            \
                     (0x1 << USBH_HUB_FEATURE_PORT_ENABLE        )|\
                     (0x1 << USBH_HUB_FEATURE_PORT_SUSPEND       )|\
                     (0x1 << USBH_HUB_FEATURE_PORT_POWER         )|\
                     (0x1 << USBH_HUB_FEATURE_C_PORT_CONNECTION  )|\
                     (0x1 << USBH_HUB_FEATURE_C_PORT_ENABLE      )|\
                     (0x1 << USBH_HUB_FEATURE_C_PORT_OVER_CURRENT)|\
                     (0x1 << USBH_HUB_FEATURE_C_PORT_RESET       ) )

#define OHCI_INTR_EN_BMP    (USBH_OHCI_INT_SO | \
                    USBH_OHCI_INT_WDH | USBH_OHCI_INT_RD | \
                    USBH_OHCI_INT_UE | USBH_OHCI_INT_RHSC )



#define USBH_OHCI_PROTECT ohci_master_int_dis(ohci);
#define USBH_OHCI_UNPROTECT ohci_master_int_en(ohci);
/*-----------------------------------------------------------------------*/

/* ====== Internal Data Structures ===== */

typedef struct ohci_irp_info USBH_OHCI_IRP_INFO;
typedef struct ohci_iso_irp_info USBH_OHCI_ISO_IRP_INFO;
typedef struct ohci_ed_info USBH_OHCI_ED_INFO;
typedef NU_USBH_HW_DISPATCH NU_USBH_OHCI_DISPATCH;

/*-----------------------------------------------------------------------*/

typedef struct
{                                            /* Aligned pools of Memory */

    /* Bitmap to manage availability status */
    UINT8 bits[MAX_ELEMENTS_PER_POOL / 8];
    /* Ptr returned by NU_Allocate_Memory */
    UINT8 *start_ptr;
    /* No. of unallocated elements in the pool */
    UINT8 avail_elements;

    UINT8 pad[3];
}
OHCI_ALIGNED_MEM_POOL;

/*-----------------------------------------------------------------------*/

typedef struct usbh_ohci_td
{
    UINT32  controlInfo;                     /* Control word. */
    UINT32  *cbp;                            /* Current buffer pointer. */
    UINT32  nextTd;                          /* Next TD. */
    UINT32  be;                              /* Buffer end. */

    /* Following fields are for HCD use only. */
    USBH_OHCI_ED_INFO  *ed_info;

    /* Buffer pointer of the TD. */
    UINT8              *data;

    /* A copy of data in non cache region. */
    UINT8              *non_cache_ptr;

    /* Data length assigned to this TD. */
    UINT16              length;

    /* For identifying non-ISO TD from ISO TD.*/
    UINT16 psw;

}USBH_OHCI_TD;

/*-----------------------------------------------------------------------*/

/* OHCI_TD_Isochronous Transfer Descriptor should be aligned to 32 byte
 * boundary.
 */
typedef struct usbh_ohci_td_iso
{
    UINT32  controlinfo;                     /* Control word. */
    UINT32  bp0;                             /* Buffer page 0. */
    UINT32  nextTd;                          /* Next TD. */
    UINT32  be;                              /* Buffer end. */
    UINT16  psw[8];                          /* Array of packet status
                                              * words.
                                              */

    USBH_OHCI_ED_INFO  *ed_info;

    /* A copy of data in non cache region. */
    UINT8              *non_cache_ptr;

    UINT8              *data;                /* Data field of the TD. */

    UINT16              index;               /* Index to IRP's private TD
                                              * area.
                                              */
    UINT16             length;

    UINT8               iso_trans_per_td;    /* Number of ISO transactions
                                              * defined in one TD.
                                              */
    UINT8 pad[(3+12)];

} USBH_OHCI_TD_ISO;

/*-----------------------------------------------------------------------*/

/* Translates IRP to OHCI TDs. */
struct ohci_irp_info
{
    NU_USB_IRP         *irp;
    UINT32              next_data_len;       /* Data length that's pending
                                              * translation in to TDs.
                                              */

    UINT8              *next_data;           /* Pointers to the irp->data,
                                              * that still needs to be
                                              * translated in to TD.
                                              */
    UINT16              req_tds;             /* No. tds required for this
                                              * IRP.
                                              */
    UINT16              done_tds;            /* No. of TDs serviced by HC
                                              * so far
                                              */
    UINT16              sch_tds;             /* No to TDs scheduled so far
                                              * for this IRP.
                                              */
    UINT8                pad[2];
};

/*-----------------------------------------------------------------------*/

/* Translates IRP to OHCI TDs. */
struct ohci_iso_irp_info
{
    USBH_OHCI_ED_INFO  *ed;                  /* Pointer to ED. */
    NU_USB_IRP         *irp;
    UINT32              next_data_len;       /* Data length that's pending
                                              * translation in to TDs.
                                              */
    UINT8              *next_data;           /* Pointers to the irp->data,
                                              * that still needs to be
                                              * translated in to TD.
                                              */
    UINT16              sch_trans;           /* Number of transactions
                                              * scheduled so far.
                                              */
    UINT16              req_trans;           /* No. packets defined by this
                                              * IRP
                                              */
    UINT16              done_trans;          /* No. of TDs serviced by HC
                                              * so far
                                              */
    UINT8               pad[2];
};

/*-----------------------------------------------------------------------*/

/* USBH_OHCI_HCCA - Host Controller Communications Area be aligned to a
 * 256 byte boundary. Ref: OHCI spec 4.4.1.
 */
typedef struct usbh_ohci_hcca
{
    VOID   *periodicEdTable[32];             /* Pointers to periodic EDs.*/
    UINT16  frameNo;                         /* Frame no in low 16-bits. */
    UINT16  pad1;                            /* Set 0 for updating Frame
                                              * number.
                                              */
    VOID   *doneHead;                        /* Pointer to first completed
                                              * TD.
                                              */
    UINT8   hcReserved[120];                 /* Reserved by HC. */
}USBH_OHCI_HCCA;

/*-----------------------------------------------------------------------*/

/* USBH_OHCI_ED - OHCI Endpoint Descriptor must be aligned to a 16-byte
 * boundary, Table 4.1 OHCI spec.
 */
typedef struct usbh_ohci_ed
{
    UINT32  controlinfo;                     /* Control word. */
    UINT32 *tdTail;                          /* TD queue tail pointer. */
    UINT32 *tdHead;                          /* TD queue head pointer. */
    UINT32  nextEd;                          /* Next USBH_OHCI_ED. */
}USBH_OHCI_ED;

/*-----------------------------------------------------------------------*/

struct ohci_ed_info
{
    CS_NODE node;
    USBH_OHCI_ED   *hwEd;                    /* Address of ED in HCCA.   */

    USBH_OHCI_IRP_INFO  irp_info;
    NU_USB_IRP *pend_irp; /* List for pending IRPs. */

    UINT32          load;                    /* EP BW in micro-secs.     */
    UINT16          interval;                /* For periodic transfers.  */
    UINT16          key;                     /* Key in to the hash table.*/

    UINT8           speed;
    UINT8           next_state;
    UINT8           index2perdTbl;           /* For Interrupt only;
                                              * describe start index in
                                              * schedule table (0-31).
                                              */
    UINT8           bEndpointAddress;        /* From USB EP descriptor.  */

    UINT8           type;                    /* ED transfer type.        */
    UINT8           on_global_pending;

    UINT8           pad[2];
};

/*-----------------------------------------------------------------------*/

/* This structure extends usbh_ohci_ed_info structure for ISO. */
typedef struct usbh_ohci_iso_ed_info
{
    USBH_OHCI_ED_INFO   ed_info;

    USBH_OHCI_TD_ISO *td_array_start;

    UINT32              next_td_index;       /* Next HW TD index, 0-31. */
    USBH_OHCI_ISO_IRP_INFO  irp_info_array [USBH_OHCI_MAX_PEND_IRPS];
                                             /* List of all the IRPs
                                              * submitted.
                                              */
    UINT32              incoming_irp_index;  /* Index to point next IRP
                                              * submitted by user.
                                              */
    UINT32              sch_irp_index;      /* Index to point IRP, which
                                              * will be used in submitting
                                              * new TD.
                                              */
    UINT32              done_irp_index;   /* Index to point IRP, TD
                                              * corresponding to which will
                                              * be retired.
                                              */
    UINT32              last_td_frame_num;   /* SOF Frame number defined in
                                              * last ISO TD.
                                              */
    UINT32              last_irp;            /* Mark the index of last IRP
                                              * submitted.
                                              */
    /* Flag to mark, ISO Transaction has been started, so now on instead of
     * reading the frame number from hardware use last_td_frame_num in new
     * TD.
     */
    BOOLEAN             trans_start_flag;
    BOOLEAN             no_more_irp;         /* Flag to indicate, no more
                                              * ISO IRP to submit.
                                              */
    UINT8               free_tds;            /* Number of free TDs slots in
                                              * the list of tds.
                                              */
    UINT8               pad[1];
}USBH_OHCI_ISO_ED_INFO;

/*-----------------------------------------------------------------------*/

/* USBH_OHCI_ Transfer Descriptor - General: Control/Bulk/Interrupt
 * be aligned to 16 byte boundary. Table 4.2 ohci spec.
 */


typedef struct usbh_ohci_hash
{
    CS_NODE list;
    UINT32  key;                             /* Key to the hash table. */
    VOID   *hcca_addr;                       /* HCCA addr of TD/ED.  */
    USBH_OHCI_ED_INFO   *descriptor;         /* USBH_OHCI_ED_INFO pointer.
                                              */
}
USBH_OHCI_HASH;

/*-----------------------------------------------------------------------*/

struct snapshot_status
{
    UINT32  hub_status;
    UINT32  port_status[OHCI_MAX_PORTS];
};

/* Root hub status. */
typedef struct usbh_rh_status
{
    UINT32                  status_map;
    struct snapshot_status  previous;
    NU_USB_IRP             *irp;
}
USBH_RH_STATUS;

/*-----------------------------------------------------------------------*/

/* ============= Control Block ============= */

typedef struct nu_usbh_ohci
{
    NU_USBH_HW              cb;
    USBH_OHCI_HCCA         *hcca;            /* 256 byte aligned pointers
                                              * of HCCA.
                                              */
    UINT32                  load[32];        /* BW consumed in each of the
                                              * schedules.
                                              */
    USBH_OHCI_ED_INFO      *last_cntrl;      /* Last Control ED. */
    USBH_OHCI_ED_INFO      *last_bulk;       /* Last Bulk ED */
    USBH_OHCI_HASH         *ed_hash_table[USBH_OHCI_HASH_SIZE];

    NU_MEMORY_POOL          *cacheable_pool;
    NU_MEMORY_POOL          *uncachable_pool;
    /* List of EDs that cant find memory resources for their transfers. */
    USBH_OHCI_ED_INFO *global_pend;
    USBH_RH_STATUS          rh_status;
    UINT8                   rh_hub_desc[20];

    NU_SEMAPHORE protect_sem;

    OHCI_ALIGNED_MEM_POOL   td_gen_pool[OHCI_MAX_TD_GEN_POOLS];
                                             /* Pool of 16 byte aligned
                                              * memory of TD elements.
                                              */
    UINT32                  intr_status;
    USBH_OHCI_TD           *done_q;
    UINT32                  int_disable_count;
    UINT32                  available_current;

    VOID                   *ses_handle;
}
NU_USBH_OHCI;



/* ====================  Function Prototypes ========================== */

/* Macro to place an element on CS_NODE list. */
#define OHCI_ADD_TO_LIST(list,node) {\
                                ((CS_NODE *)node)->cs_next = NU_NULL; \
                                ((CS_NODE *)node)->cs_previous = NU_NULL; \
                                 CSC_Place_On_List( \
                                 (CS_NODE**)&(list),(CS_NODE *)(node)); }

/* Macro to remove an element from CS_NODE list. */
#define OHCI_REMOVE_FROM_LIST(list,node) CSC_Remove_From_List( \
                                      (CS_NODE**)&(list),(CS_NODE*)(node))

#define USBH_RH_GET_BUFFER(irp) \
                   NU_USB_IRP_Get_Data((NU_USB_IRP *)irp, &irp_buffer)

#define USBH_RH_GET_LENGTH(irp) \
                   NU_USB_IRP_Get_Length((NU_USB_IRP *)irp, &irp_length)

#define USBH_RH_SET_LENGTH(irp, x) \
                   NU_USB_IRP_Set_Actual_Length((NU_USB_IRP *)irp, x)

#define USBH_RH_SET_STATUS(irp, x) \
                   NU_USB_IRP_Set_Status((NU_USB_IRP *)irp, x)

#define USBH_RH_BREQUEST(irp) \
                   NU_USBH_CTRL_IRP_Get_bRequest(irp, &bRequest)

#define USBH_RH_WLENGTH(irp) \
                   NU_USBH_CTRL_IRP_Get_wLength(irp, &wLength);\
                   wLength = LE16_2_HOST(wLength)

#define USBH_RH_LSB(wValue)    (UINT8)(wValue & 0xFF)

#define USBH_RH_MSB(wValue)    (UINT8)((wValue >> 8) & 0xFF)

#define USBH_RH_IS_CLASS_SPECIFIC(bmRequestType) \
                          ((bmRequestType & 0x60) == 0x20)

#define USBH_RH_GET_RECIPIENT(bmRequestType) (bmRequestType & 0x0F)


#define OHCI_GET_PORTSTS_REG_OFFSET(port_num) ((port_num - 1) * 4)

#define OHCI_HW_READ32(cb,p,d) \
            ohci_read32(cb,(UINT32*)(p), (UINT32*)&(d))

#define OHCI_HW_WRITE32(cb,p,d) \
            ohci_write32(cb, (UINT32*)(p), (UINT32)(d))

#define OHCI_HW_READ16(cb,p,d) \
            ohci_read16(cb,(UINT16*)(p), (UINT16*)&(d))

#define OHCI_HW_WRITE16(cb,p,d) \
            ohci_write16(cb, (UINT16*)(p), (UINT16)(d))

#define OHCI_READ_ADDRESS(cb, p, d)     OHCI_HW_READ32(cb,p,d)
#define OHCI_WRITE_ADDRESS(cb, p, d)    OHCI_HW_WRITE32(cb,p,d)

/* Macros to format/retrieve control.FA (function address) field. */
#define USBH_OHCI_EDCTL_FA(x) \
            (((x) & USBH_OHCI_EDCTL_FA_MASK) >> USBH_OHCI_EDCTL_FA_ROT)

#define USBH_OHCI_EDCTL_FA_FMT(x) \
            (((x) << USBH_OHCI_EDCTL_FA_ROT) & USBH_OHCI_EDCTL_FA_MASK)

/* Macros to format/retrieve control.EN (endpoint number) field. */
#define USBH_OHCI_EDCTL_EN(x) \
            (((x) & USBH_OHCI_EDCTL_EN_MASK) >> USBH_OHCI_EDCTL_EN_ROT)

#define USBH_OHCI_EDCTL_EN_FMT(x) \
            (((x) << USBH_OHCI_EDCTL_EN_ROT) & USBH_OHCI_EDCTL_EN_MASK)

/* Macros to format/retrieve control.MPS (max packet size) field. */
#define USBH_OHCI_EDCTL_MPS(x) \
            (((x) & USBH_OHCI_EDCTL_MPS_MASK) >> USBH_OHCI_EDCTL_MPS_ROT)

#define USBH_OHCI_EDCTL_MPS_FMT(x) \
            (((x) << USBH_OHCI_EDCTL_MPS_ROT) & USBH_OHCI_EDCTL_MPS_MASK)

/* Macro to retrieve USBH_OHCI_FMN_FN (frame number) field. */
#define USBH_OHCI_FMN_FN(x) \
            (((x) & USBH_OHCI_FMN_FN_MASK) >> USBH_OHCI_FMN_FN_ROT)

#define OHCI_TD_FC_SET(X) \
             (((X) & USBH_OHCI_TICTL_FC_NONE) << USBH_OHCI_TICTL_FC_ROT)

#define OHCI_TD_DI_SET(X) \
             (((X) & USBH_OHCI_TICTL_DI_NONE) << USBH_OHCI_TICTL_DI_ROT)

/* Take function_addr [2:1] and endpoint addr [1:0]. */
#define USBH_OHCI_ED_HASH_FUNC(key)  \
 (((key << 1) & 0xC) | ((key >> 7) & 0x3))


#define ohci_alloc_ed(ohci, ed_ptr, suspend)  \
        NU_Allocate_Aligned_Memory(ohci->cb.pool, \
                                            (VOID **)ed_ptr, \
                                            sizeof(USBH_OHCI_ED), \
                                            OHCI_ED_TD_ALIGNMENT, \
                                            NU_NO_SUSPEND);


#define ohci_dealloc_ed(ohci, ed_ptr) NU_Deallocate_Memory((VOID *)ed_ptr);

#define OHCI_MOD_INC(a, b) \
               a++; \
               if (a == b) \
               { \
                   a = 0; \
               }

#define OHCI_MOD_DEC(a, b) \
               if (a == 0) \
               { \
                   a = (b -1); \
               } \
               else \
               { \
                   a--; \
               }


/* =================== Internal function prototypes  =================== */

STATUS              ohci_alloc_gen_td(NU_USBH_OHCI *ohci,
                                      USBH_OHCI_TD **td_ptr,
                                      USBH_OHCI_ED_INFO *ed_info,
                                      UNSIGNED suspend);

STATUS              ohci_dealloc_gen_td(NU_USBH_OHCI *ohci,
                                        USBH_OHCI_TD *td_ptr);

STATUS              ohci_alloc_iso_td(NU_USBH_OHCI *ohci,
                                      USBH_OHCI_TD_ISO **td_ptr,
                                      UNSIGNED suspend);

STATUS              ohci_dealloc_iso_td(NU_USBH_OHCI *ohci,
                                        USBH_OHCI_TD_ISO *td_ptr);

UINT8               ohci_normalize_interval(UINT32 interval);

STATUS              ohci_hash_add_ed(NU_USBH_OHCI *ohci,
                                USBH_OHCI_ED_INFO *ed_info);

STATUS              ohci_hash_delete_ed(NU_USBH_OHCI *ohci,
                                   USBH_OHCI_ED_INFO *ed_info);

USBH_OHCI_ED_INFO  *ohci_hash_find_ed(NU_USBH_OHCI *ohci, UINT32 key);

VOID ohci_dealloc_all_tds(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ED_INFO *ed_info);

VOID                ohci_scheduleControlPipe(NU_USBH_OHCI *ohci,
                                             USBH_OHCI_ED_INFO *ed_info);

VOID                ohci_scheduleBulkPipe(NU_USBH_OHCI *ohci,
                                          USBH_OHCI_ED_INFO *ed_info);

VOID                ohci_schedulePeriodicPipe(NU_USBH_OHCI *ohci,
                                              USBH_OHCI_ED_INFO *ed_info);

UINT8               ohci_find_best_schedule(NU_USBH_OHCI *ohci,
                                            USBH_OHCI_ED_INFO *ed_info);

VOID                ohci_hndl_td_done_q(NU_USBH_OHCI *ohci,
                                        USBH_OHCI_TD *td);

VOID                ohci_hndl_retired_td(NU_USBH_OHCI *ohci,
                                         USBH_OHCI_TD *td);

VOID                ohci_hndl_retired_iso_td(NU_USBH_OHCI *ohci,
                                             USBH_OHCI_TD_ISO *iso_hwTd);

VOID                ohci_close_periodic_ed(NU_USBH_OHCI *ohci,
                                           USBH_OHCI_ED_INFO *ed_info);

VOID                ohci_close_non_periodic_ed(NU_USBH_OHCI *ohci,
                                               USBH_OHCI_ED_INFO *ed_info);

STATUS ohci_make_td (NU_USBH_OHCI      *ohci,
                     UINT32             info,
                     UINT8             *data,
                     UINT32             len,
                     USBH_OHCI_IRP_INFO *iso_irp_info,
                     USBH_OHCI_ED_INFO *ed_info,
                     USBH_OHCI_TD **filled_hwTd,
                     USBH_OHCI_TD *previous_hwTd);

VOID ohci_fill_td(NU_USBH_OHCI *ohci,
                    UINT32 info,
                    UINT8 *irp_buf,
                    UINT8 *noncache_buf,
                    UINT16 len,
                    USBH_OHCI_IRP_INFO *irp_info,
                    USBH_OHCI_ED_INFO *ed_info,
                    USBH_OHCI_TD *hwTd);

UINT32 ohci_fill_iso_td(NU_USBH_OHCI *ohci,
                    UINT32 info,
                    UINT8 *data,
                    UINT32 len,
                    USBH_OHCI_ED_INFO *ed,
                    UINT32 index,
                    USBH_OHCI_TD_ISO *hwTd);


STATUS              ohci_find_td_count(NU_USBH_OHCI *ohci,
                                       USBH_OHCI_ED_INFO *ed_info,
                                       NU_USB_IRP *irp,
                                       UINT32 maxPacket,
                                       UINT32 *count);

STATUS              ohci_handle_irp(NU_USBH_OHCI *ohci,
                                    USBH_OHCI_ED_INFO *ed_info,
                                    NU_USB_IRP *irp,
                                    BOOLEAN update_ed_info);

STATUS              ohci_handle_iso_irp(NU_USBH_OHCI *ohci,
                                        USBH_OHCI_ED_INFO *ed_info,
                                        NU_USB_IRP *irp);

STATUS              ohci_rh_handle_irp(NU_USBH_OHCI *ohci,
                                             NU_USB_IRP *irp,
                                             UINT8 bEndpointAddress);

UINT8               ohci_check_iso_trans_per_td(NU_USBH_OHCI *ohci,
                                  USBH_OHCI_ISO_IRP_INFO *irp_info,
                                  NU_USB_ISO_IRP *irp,
                                  UINT16  *td_length);

STATUS              ohci_rh_init(NU_USBH_OHCI *ohci);

VOID                ohci_rh_isr(NU_USBH_OHCI *ohci);

UINT32              ohci_ed_key(UINT8 function_address,
                                UINT8 bEndpointAddress);

VOID                ohci_unlink_tds(NU_USBH_OHCI *ohci,
                                    USBH_OHCI_ED_INFO *ed_info,
                                    BOOLEAN remove_dummy_td);

STATUS ohci_rh_invalid_cmd(NU_USBH_OHCI *ohci,
                      NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_nothing_to_do_cmd(NU_USBH_OHCI *ohci,
                            NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_set_hub_feature(NU_USBH_OHCI *ohci,
                          UINT8 feature);

STATUS ohci_rh_set_port_feature(NU_USBH_OHCI *ohci,
                           UINT8 port,
                           UINT8 feature);

STATUS ohci_rh_clear_hub_feature(NU_USBH_OHCI *ohci,
                            UINT8 feature);

STATUS ohci_rh_clear_port_feature(NU_USBH_OHCI *ohci,
                             UINT8 port,
                             UINT8 feature);

STATUS ohci_rh_clear_feature(NU_USBH_OHCI *ohci,
                        NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_set_feature(NU_USBH_OHCI *ohci,
                      NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_get_state(NU_USBH_OHCI *ohci,
                    NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_get_descriptor(NU_USBH_OHCI *ohci,
                         NU_USBH_CTRL_IRP *irp);

STATUS ohci_rh_get_status(NU_USBH_OHCI *ohci,
                     NU_USBH_CTRL_IRP *irp);

VOID ohci_master_int_en(NU_USBH_OHCI *ohci);
VOID ohci_master_int_dis(NU_USBH_OHCI *ohci);

STATUS ohci_transfer_done(NU_USBH_OHCI*, UINT8*, UINT8*, UINT8*, UINT32, BOOLEAN);
STATUS ohci_normalize_buffer(NU_USBH_OHCI*, UINT8*, UINT32, UINT32, BOOLEAN, UINT8**, UINT8**);

VOID ohci_read32(NU_USBH_OHCI    *ohci, 
                UINT32          *address, 
                UINT32          *data_out);

VOID ohci_read16(NU_USBH_OHCI    *ohci, 
                        UINT16   *address, 
                        UINT16   *data_out);

VOID ohci_write32(NU_USBH_OHCI    *ohci, 
                        UINT32   *address, 
                        UINT32   data);
                        
VOID ohci_write16(NU_USBH_OHCI       *ohci, 
                        UINT16      *address, 
                        UINT16      data);

/* ==================== USB Include Files ============================== */

#include "drivers/nu_usbh_ohci_dat.h"

/* ===================================================================== */

#endif /* _NU_USBH_OHCI_IMP_H_ */

/* ======================  End Of File  ================================ */
