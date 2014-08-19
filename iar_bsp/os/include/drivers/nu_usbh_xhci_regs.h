/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       nu_usbh_xhci_regs.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains Registers control block for NU_USBH_XHCI component.
*
* DATA STRUCTURES
*
*       nu_usbh_xhci_reg_base               xHC base registers control block.
*       nu_usbh_xhci_cap_regs               xHC Capability Registers.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
**************************************************************************/
#ifndef NU_USBH_XHCI_REGS_H
#define NU_USBH_XHCI_REGS_H

typedef struct nu_usbh_xhci_reg_base        NU_USBH_XHCI_REG_BASE;
typedef struct nu_usbh_xhci_cap_regs        NU_USBH_XHCI_CAP_REGS;
typedef struct nu_usbh_xhci_op_regs         NU_USBH_XHCI_OP_REGS;
typedef struct nu_usbh_xhci_run_regs        NU_USBH_XHCI_RUN_REGS;
typedef struct nu_usbh_xhci_intr_reg        NU_USBH_XHCI_INTR_REG;
typedef struct nu_usbh_xhci_db_array        NU_USBH_XHCI_DB_ARRAY;

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_reg_base
* This structure serves as access point for xHC register interface.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_reg_base
{
    /* Pointer to capability registers offset */
    NU_USBH_XHCI_CAP_REGS       *capability_regs;

    /* Pointer to operational registers offset */
    NU_USBH_XHCI_OP_REGS        *operational_regs;

    /* Pointer to run time registers offset */
    NU_USBH_XHCI_RUN_REGS       *run_time_regs;

    /* Pointer to doorbell regisrers array offset */
    NU_USBH_XHCI_DB_ARRAY       *doorbell_array;

    /* Pointer to current interrupter register set */
    NU_USBH_XHCI_INTR_REG       *interrupt_reg_set;

};

/*
--------------------------------------------------------------------------
* struct nu_usbh_xhci_cap_regs
* xHC Capability Registers.
--------------------------------------------------------------------------
*/
struct nu_usbh_xhci_cap_regs
{
    UINT32  xhci_hc_capbase;
    UINT32  xhci_hcs_params1;
    UINT32  xhci_hcs_params2;
    UINT32  xhci_hcs_params3;
    UINT32  xhci_hcc_params;
    UINT32  xhci_db_off;
    UINT32  xhci_run_regs_off;

};

/*
--------------------------------------------------------------------------
xHCI HCSPARAMS1 register bitmasks
--------------------------------------------------------------------------
*/
#define XHCI_HCS_MAX_DEV_SLOTS_MASK         0xFF
#define XHCI_HCI_VERSION(x)                 (((x) >> 16) & 0xFFFF)
#define XHCI_HCS_MAX_DEV_INTRS(x)           (((x) >> 8) & 0x7FF)
#define XHCI_HCS_MAX_DEV_PORTS(x)           (((x) >> 24) & 0x7F)

/*
--------------------------------------------------------------------------
xHCI HCSPARAMS2 register bitmasks
--------------------------------------------------------------------------
*/

#define XHCI_HCS_MAX_SCRATCHPAD_BUFF(x)     (((x) >> 27) & 0x1F)

/*
--------------------------------------------------------------------------
xHCI HCSPARAMS3 register bitmasks
--------------------------------------------------------------------------
*/
#define XHCI_HCS_U1_LATENCY(x)              (((x) >> 0) & 0xFF)
#define XHCI_HCS_U2_LATENCY(x)              (((x) >> 16) & 0xFFFF)

/*
--------------------------------------------------------------------------
xHCI HCCPARAMS register bitmasks
--------------------------------------------------------------------------
*/

/* If true  then XHCI uses 64-byte Device Context structures .*/
#define XHCI_HCC_64BYTE_CNTXT(x)            ((x) & (1 << 2))

/* Primary stream array size , 2^(n+1)*/
#define XHCI_HCC_MAX_PRI_STREAM_ARR(x)      (1 << ((((x) >> 12) & 0xF) + 1))

/* XHCI HCCPARAMS reg contains the first extended capability pointer */
#define XHCI_HCC_EXT_CAPS_PTR(x)            (((x) >> 16) & 0xFFFF)

/*
--------------------------------------------------------------------------
xHCI Runtime and Doorbell registers offsets.
--------------------------------------------------------------------------
*/
#define XHCI_DB_OFF_MASK                    (~0x7)
#define XHCI_RUN_REGS_OFF_MASK              (~0x1F)


/*
--------------------------------------------------------------------------
 * struct nu_usbh_xhci_op_regs
 * xHC Operational Registers
--------------------------------------------------------------------------
*/

struct nu_usbh_xhci_op_regs
{
    /* USBCMD , xHCI command register. */
    UINT32  xhci_usb_command;

    /* USBSTS , xHCI status register. */
    UINT32  xhci_usb_status;

    /* Page size, 4K is the minimum page size.
     */
    UINT32  xhci_page_size;

    /* Reserved */
    UINT32  xhci_reserved1[2];

    /* Device notification control register.*/
    UINT32  xhci_dev_notif_ctrl;

    /* Command Ring Pointer , 64 bit. */
    UINT32  xhci_cmd_ring_lo;

    UINT32  xhci_cmd_ring_hi;

    /* Reserved */
    UINT32  xhci_reserved2[4];

    /* Pointer to device context base address array pointer. */
    UINT32  xhci_dcbaa_ptr_lo;

    UINT32  xhci_dcbaa_ptr_hi;

    /* Configuration register. */
    UINT32  xhci_config_reg;

    /* Reserved*/
    UINT32  xhci_reserved3[241];

    /* Port status and control register base address. */
    /* port 1 registers, which serve as a base address for other ports. */
    UINT32  xhci_port_status_base;

    /* Power maangment status and control register base adddress. */
    UINT32  xhci_port_power_base;

    /* Port link info base address. */
    UINT32  xhci_port_link_base;

    UINT32  xhci_reserved4;

    /* Ports reserved registers 2-255 */
    UINT32 xhci_reserved5[(4 * 254)];
};

/*
--------------------------------------------------------------------------
USBCMD register bitmasks
--------------------------------------------------------------------------
*/
#define XHCI_USB_CMD_RUN                    (1 << 0)
#define XHCI_USB_CMD_RESET                  (1 << 1)
#define XHCI_USB_CMD_EIE                    (1 << 2)
#define XHCI_USB_CMD_HSEIE                  (1 << 3)
#define XHCI_USB_CMD_EWE                    (1 << 10)
#define XHCI_USB_INT_ENABLE                 (XHCI_USB_CMD_EWE \
                                             | XHCI_USB_CMD_EIE \
                                             | XHCI_USB_CMD_HSEIE)
                                             
/*
--------------------------------------------------------------------------
USB status register bitmasks
--------------------------------------------------------------------------
*/

#define XHCI_STATUS_HALT                    (1 << 0)
#define XHCI_STATUS_EINT                    (1 << 3)

/*
--------------------------------------------------------------------------
Command Ring Control Register(CRCR) bitmask
--------------------------------------------------------------------------
*/

#define XHCI_COMMAND_RING_RSVD_BITS         (0x3F)

/*
--------------------------------------------------------------------------
xHC Port Status and Control Registerbitmasks
--------------------------------------------------------------------------
*/

#define XHCI_PORT_CONNECT                   (1 << 0)
#define XHCI_PORT_PE                        (1 << 1)
#define XHCI_PORT_OC                        (1 << 3)
#define XHCI_PORT_RESET                     (1 << 4)
#define XHCI_SET_PORT_LS(x)                 (((x) & 0xF) << 5)
#define XHCI_GET_PORT_LS(x)                 (((x) >> 5) & 0xF)
#define XHCI_PORT_POWER                     (1 << 9)

#define XHCI_DEVICE_SPEED_MASK              (0xF << 10)
#define XHCI_DEVICE_FS                      (0x1 << 10)
#define XHCI_DEVICE_LS                      (0x2 << 10)
#define XHCI_DEVICE_HS                      (0x3 << 10)
#define XHCI_DEVICE_SS                      (0x4 << 10)

#define XHCI_SLOT_SPEED_FS                  ((0x1 << 10) << 10)
#define XHCI_SLOT_SPEED_LS                  ((0x2 << 10) << 10)
#define XHCI_SLOT_SPEED_HS                  ((0x3 << 10) << 10)
#define XHCI_SLOT_SPEED_SS                  ((0x4 << 10) << 10)

#define XHCI_DEVICE_SPEED(x)                ((x)& XHCI_DEVICE_SPEED_MASK)
#define XHCI_PIC(x)                         (((x) & 0x3) <<  14)
#define XHCI_PORT_LINK_STROBE               (1 << 16)
#define XHCI_PORT_CSC                       (1 << 17)
#define XHCI_PORT_PEC                       (1 << 18)
#define XHCI_PORT_WRC                       (1 << 19)
#define XHCI_PORT_OCC                       (1 << 20)
#define XHCI_PORT_RC                        (1 << 21)
#define XHCI_PORT_PLC                       (1 << 22)
#define XHCI_PORT_OFFSET(x)                 (0x10*((x)-1))

/*
--------------------------------------------------------------------------
Port Power Management Status and Control bitmasks
--------------------------------------------------------------------------
*/

#define XHCI_PORT_U1_TIMEOUT(x)             ((x) & 0xFF)

#define XHCI_PORT_U2_TIMEOUT(x)             (((x) & 0xFF) << 8)

/*
--------------------------------------------------------------------------
* struct nu_usbh_xhci_intr_reg
* Interrupt Register Set.
--------------------------------------------------------------------------
*/

struct nu_usbh_xhci_intr_reg
{
    /* Interrupt Management Register.*/
    UINT32  xhci_iman;

    /* Interrupt Moderation Register. */
    UINT32  xhci_imod;

    /* Number of event ring segments */
    UINT32  xhci_erst_size;
    UINT32  xhci_rsvd;

    /* ERST base address. */
    UINT32  xhci_erst_base_lo;
    UINT32  xhci_erst_base_hi;

    /* Event ring dequeue pointer. */
    UINT32  xhci_erst_dequeue_lo;
    UINT32  xhci_erst_dequeue_hi;

};

/*
--------------------------------------------------------------------------
* struct nu_usbh_xhci_run_regs
* Host Controller Runtime Registers
--------------------------------------------------------------------------
*/

struct nu_usbh_xhci_run_regs
{
    UINT32                 microframe_index;
    UINT32                 rsvd[7];
    NU_USBH_XHCI_INTR_REG  ir_set[128];
};

/*
--------------------------------------------------------------------------
* Interrupt Management Register bitmasks
--------------------------------------------------------------------------
*/
#define XHCI_IR_INTE                        (1 << 1)
#define XHCI_IMAN_IRQ_PENDING               (0x1)
#define XHCI_IRQ_CLEAR(x)                   ((x) & 0xFFFFFFFF)
#define XHCI_IMAN_IRQ_ENABLE(x)             ((XHCI_IRQ_CLEAR(x)) | 0x2)
#define XHCI_IMAN_IRQ_DISABLE(x)            ((XHCI_IRQ_CLEAR(x)) & ~(0x3))

/*
--------------------------------------------------------------------------
Event Ring Segment Table Size Register bitmasks
--------------------------------------------------------------------------
*/
#define XHCI_EVT_RING_SEG_SIZE_MASK         ((UINT32)0xFFFF << 16)
#define XHCI_EVT_RING_SEG_EHB               (1 << 3)
#define XHCI_EVT_RING_SEG_PTR_MASK          (0xF)

/*
--------------------------------------------------------------------------
* nu_usbh_xhci_db_array
* Doorbell Registers Array
--------------------------------------------------------------------------
*/

struct nu_usbh_xhci_db_array
{
    UINT32  db_array[256];
};

/*
--------------------------------------------------------------------------
 Doorbell Register bitmasks
--------------------------------------------------------------------------
*/

#define XHCI_DB_TGT_HOST                    0x0
#define XHCI_DB_STRM_ID_HOST                0x0
#define XHCI_DB_SET_ENDPOINT_INDEX(x)       (((x) + 1) & 0xFF)
#define XHCI_DB_SET_STREAM_ID(x)            (((x) & 0xFFFF) << 16)


/* Extended capability register fields */
#define XHCI_EXTENDED_CAPS_ID(x)            ((x) & 0xff)
#define XHCI_EXTENDED_CAPS_NEXT(x)          (((x) >> 8 ) & 0xff)

/* Extended capability ID. */
#define XHCI_EXTENDED_CAPS_PROTOCOL         2

/* Root Hub parameters*/
#define XHCI_ROOT_HUB_PRT_OFFSET(x)        ((x) & 0xFF)
#define XHCI_ROOT_HUB_PRT_COUNT(x)         (((x) >> 8) & 0xFF)
#define XHCI_ROOT_HUB_PRT_MAJ_REV(x)       ((x) >> 24)


#endif  /* NU_USBH_XHCI_REGS_H */

/* ======================  End Of File.  =============================== */
