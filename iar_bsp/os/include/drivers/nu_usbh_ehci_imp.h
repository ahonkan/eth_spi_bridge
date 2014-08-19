/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*        nu_usbh_ehci_imp.h
*
* COMPONENT
*
*       Nucleus USB Host software
*
* DESCRIPTION
*
*       This file contains the internal definitions for the EHCI driver
*
* DATA STRUCTURES
*
*       NU_USBH_EHCI        Control Block
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbh_hw_ext.h    All Base hardware dependencies
*       nu_usbh_ehci_cfg.h 
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_EHCI_IMP_H_
#define _NU_USBH_EHCI_IMP_H_

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */


/* ==============  Standard Include Files ============================  */


/* ==================== USB Include Files ============================== */
#include "connectivity/nu_usbh_hw_ext.h"
#include "drivers/nu_usbh_ehci_cfg.h"

/* =====================  Global data ================================  */



/* =====================  #defines ===================================  */

/* QH States */

#define   USBH_EHCI_QH_NOT_READY    0   /* Under Configuration   */
#define   USBH_EHCI_QH_READY        1   /* Configured and in use */
#define   USBH_EHCI_QH_DELETE       2   /* Under deletion   */
#define   USBH_EHCI_QH_IN_MODIFY    3   /* Being Modified   */


#define USBH_EHCI_INTR_ENABLE_MASK     (EHCI_INTR_HOST_ERROR | \
                                        EHCI_INTR_PORT_CHANGE | \
                                        EHCI_INTR_ERR_INTERRUPT | \
                                        EHCI_INTR_INTERRUPT )

/* Hash size    */
#define USBH_EHCI_HASH_SIZE             64

#define USBH_EHCI_MAX_QTDS_PER_BURST    64

#define USBH_EHCI_MAX_BURSTS            2

#define USBH_EHCI_MAX_PEND_IRPS         8

#define USBH_EHCI_QH_TD_ALIGNMENT       32

#define USBH_EHCI_QTD_DATA_SIZE         (16 * 1024)

#define USBH_EHCI_BURST_SIZE            (USBH_EHCI_QTD_DATA_SIZE * \
                                         USBH_EHCI_MAX_QTDS_PER_BURST)

#define USBH_EHCI_ITD_DATA_SIZE         (8 * 1024)


#define USBH_EHCI_BURST_IDLE            0
#define USBH_EHCI_BURST_READY           1
#define USBH_EHCI_BURST_ACTIVE          2

#define USBH_EHCI_ITD_RETIRED           0x02
#define USBH_EHCI_ITD_CANCELLED         0x10

#define USBH_EHCI_STATUS_BASE           -4000
#define USBH_EHCI_ISO_MAX_IRP           USBH_EHCI_STATUS_BASE - 1
#define USBH_EHCI_MAX_ITDS              USBH_EHCI_STATUS_BASE - 2
#define USBH_EHCI_SITD_NOT_RETIRED      USBH_EHCI_STATUS_BASE - 3
#define USBH_EHCI_ITD_NOT_RETIRED       USBH_EHCI_STATUS_BASE - 4


/* Maximum data that can be relieved from a full-speed device in a micro-
 * frame.
 */
#define USBH_EHCI_ISO_IN_SPLIT_MAXP       192

/* Maximum data that can be sent to a full-speed device in a micro-frame. */
#define USBH_EHCI_ISO_OUT_SPLIT_MAXP      188

/* After USBH_EHCI_SITDS_PER_INT number of SITD's are completed an interrupt
 * on completion will be generated.
 */
#define USBH_EHCI_SITDS_PER_INT           2

#define USBH_EHCI_TYPE_SITD                0x04
#define USBH_EHCI_TYPE_ITD                 0x00

#define USBH_EHCI_MAX_ITERATIONS            5000

#define USBH_EHCI_MAX_RST_ITERATIONS        500
/* ====================  Data Types ==================================  */


typedef NU_USBH_HW_DISPATCH NU_USBH_EHCI_DISPATCH;
typedef struct usbh_ehci_iso_qh_info USBH_EHCI_ISO_QH_INFO;
typedef struct usbh_ehci_qh_info USBH_EHCI_QH_INFO;
typedef struct usbh_ehci_qh USBH_EHCI_QH;
typedef struct usbh_ehci_itd USBH_EHCI_ITD;
typedef struct usbh_ehci_sitd USBH_EHCI_SITD;
typedef struct ehci_burst USBH_EHCI_BURST;
typedef struct ehci_iso_IRP USBH_EHCI_ISO_IRP;
typedef struct usbh_ehci_qtd USBH_EHCI_QTD;


typedef struct
{   /* Aligned pool of Memory */
    /* bit map to manage availability status */
    UINT8 bits[MAX_ELEMENTS_PER_POOL / 8];
    /* Ptr returned by NU_Allocate_Aligned_Memory */
    UINT8 *start_ptr;
    /* No. of unallocated elements in the pool */
    UINT8 avail_elements;
    UINT8 pad[3];
}
EHCI_ALIGNED_MEM_POOL;

/* EHCI ISO IRP structure to translate IRP into TD's */
struct ehci_iso_IRP
{
    NU_USB_ISO_IRP  *irp;
    UINT32 no_tds;       /* Number of TDs */
    UINT32 cnt;          /* Retired TD count */
    UINT32 next_index;   /* Next TD index */
};
struct usbh_ehci_qh
{
    UINT32 next;
    UINT32 ep_controlinfo1;
    UINT32 ep_controlinfo2;
    UINT32 current_qtd;
    UINT32 next_qtd;
    UINT32 alt_next_qtd;
    UINT32 transfer_result;
    UINT32 buffer[5];
    USBH_EHCI_QH_INFO *info;
    USBH_EHCI_QH *prev;
    UINT32 pad[10];

};


struct usbh_ehci_qh_info
{
    CS_NODE list;              /* Used when on rm list */
    USBH_EHCI_QH *hwqh;        /* Address of its QH    */

    USBH_EHCI_QTD  *qtd_head;

    USBH_EHCI_QTD *dummy_qtd; /* for consuming short transfers */


    NU_USB_IRP *pend_irp;     /* Pend the IRPs while bursts happen on
                               * bus
                               */
    UINT32 key;
    UINT32 interval;          /* for Interrupt and isochronous transfer types */
    UINT32 index_per_tbl;     /* Index in the periodic frame list where QH
                               * will be placed. */
    UINT32 next_uframe;       /* Micro frame number */
    UINT32 load;              /* EP B/W  in usecs      */
    UINT32 address;           /* This is used to initialize sITD's
                                * TT info.
                                */
    UINT32 mask;              /* Start split and complete
                               * split mask for SITD's */
    UINT32 ssplit_info;       /* Field containing transaction position
                               * and transaction count information for split
                               * out transfers. */
    UINT16 maxpacket;
    UINT8 func_address;
    UINT8 speed;
    UINT8 bEndpointAddress;   /* field  from USB endpoint descriptor */
    UINT8 type;               /* ED transfer type */
    UINT8 qh_state;
    UINT8 uframe;             /* uframe with minimum load */
};

/* Isochronous queue head information */
struct usbh_ehci_iso_qh_info
{
    USBH_EHCI_QH_INFO qh_info;
    VOID              *td_array_start;
                            /* Array of pointers to the tds
                             * For ISO these are allocated
                             * when the pipe is opened.
                             */
    UINT32 next_td_index;   /* Next TD index,
                             * 0 - USBH_EHCI_MAX_ITDS_PER_IRP -1*/
    USBH_EHCI_ISO_IRP    irps[USBH_EHCI_MAX_PEND_IRPS];
                             /* List of all IRPs submitted */

    UINT32              incoming_irp_index;  /* Index to point next IRP
                                              * submitted by user.
                                              */
    UINT32              next_irp_index;      /* Index to point IRP, which
                                              * will be used in submitting
                                              * new TD.
                                              */
    UINT32              current_irp_index;   /* Index to point IRP, TD
                                              * corresponding to which will
                                              * be retired.
                                              */

    UINT32              last_td_frame_num;   /* SOF Frame number defined in
                                              * last ISO TD.
                                              */

    UINT32              last_irp;            /* Mark the index of last IRP
                                              * submitted.
                                              */

    BOOLEAN             no_more_irp;         /* Flag to indicate, no more
                                              * ISO IRP to submit.
                                              */
    UINT8               free_tds;            /* Number of free TDs slots in
                                              * the list of tds. */
    UINT8               ret_td_index;        /* Index of retired TD */

    /* Flag to mark, ISO Transaction has been started, so now on instead of
     * reading the frame number from hardware use last_td_frame_num in new
     * TD.
     */
    BOOLEAN             trans_start_flag;

};





struct usbh_ehci_itd
{
    /* Hardware part of ITD */
    UINT32                  next;
    UINT32                  trans_info[8];
    UINT32                  buffer[7];
    /* Data specific to the software */

    USBH_EHCI_ISO_QH_INFO   *qhinfo;
    UINT32                  length[8];    /* Data Length */
    UINT32                  pg;
    UINT32                  frame;
    UINT8                   *data[8];
    UINT8                   *non_cache_ptr[8];
    UINT8                   *raw_data[8];
    UINT8                   trans_per_td; /* Transactions in the ITD */
    UINT8                   pad[19];
};

struct usbh_ehci_sitd
{
    /* Hardware part of SITD */
    UINT32                  next;
    UINT32                  ep_controlinfo1;
    UINT32                  ep_controlinfo2;
    UINT32                  results;
    UINT32                  buffer[2];
    UINT32                  backpointer;

    UINT32                  length;    /* Data Length */
    UINT32                  frame;
    UINT8                   *data;
    UINT8                   *non_cache_ptr;
    UINT8                   *raw_data;
    /* Data specific to the software */
    USBH_EHCI_ISO_QH_INFO   *qhinfo;
    UINT8                   pad[12];

};

struct usbh_ehci_qtd
{
    UINT32 next;
    UINT32 alt_next;
    UINT32 token;
    UINT32 buffer[5];
    USBH_EHCI_BURST *burst;
    UINT32 index;
    UINT32 pad[6];
};


typedef struct ehci_IRP USBH_EHCI_IRP;

struct ehci_burst
{
    USBH_EHCI_IRP *transfer;
    UINT8 *data;
    UINT8 *norm_data;
    UINT8 *raw_data;
    UINT32 length;
    USBH_EHCI_QTD *hwqtd[USBH_EHCI_MAX_QTDS_PER_BURST + 1];
    UINT32 num_qtds;
    UINT8 index;
    UINT8 state;
};


/* Translates IRP to EHCI TDs */

struct ehci_IRP
{
    USBH_EHCI_QH_INFO *qh;                  /* Ptr to ED */

    NU_USB_IRP *irp;

    USBH_EHCI_BURST burst[USBH_EHCI_MAX_BURSTS];
    UINT8 ref_count;
    UINT8 last_burst;

    UINT32 rem_length;
    UINT8 *rem_data;
    UINT8 direction;
};


typedef struct usbh_ehci_hash
{
    CS_NODE list;
    UINT32 key;
    USBH_EHCI_QH_INFO *qh_info;
}
USBH_EHCI_HASH;


struct ehci_snapshot_status
{
    UINT32 hub_status;
    UINT32 port_status[15];
};

/* Root hub status */

typedef struct usbh_ehci_rh_status
{
    UINT32 status_map;
    struct ehci_snapshot_status previous;
    NU_USB_IRP *irp;
}
USBH_EHCI_RH_STATUS;


typedef struct nu_usbh_ehci
{

    /* Parent control block     */

    NU_USBH_HW cb;

    /* Register base addresses  */

    UINT8 *ehci_capreg_base;
    UINT8 *ehci_oprreg_base;

    NU_MEMORY_POOL  *cacheable_pool;
    NU_MEMORY_POOL  *uncachable_pool;

    USBH_EHCI_QH *dummy_qh;

    USBH_EHCI_QH_INFO *reclaim_head;

    /* Async and Periodic Hash tables */

    USBH_EHCI_HASH *qh_async_ht[USBH_EHCI_HASH_SIZE];

    /* Periodic Stuff   */

    /* Load calculation for periodic pipes */

    UINT32 load[USBH_EHCI_FRAME_LIST_SIZE];

    UINT32 *periodic_list_base;

    /* Load in micro seconds on each of the frame */
    UINT32 frame_load[USBH_EHCI_FRAME_LIST_SIZE];
    /* Load in micro seconds on each of the uframe */
    UINT32 uframe_load[USBH_EHCI_FRAME_LIST_SIZE][8];

    USBH_EHCI_QH_INFO *active_qh_periodic[USBH_EHCI_MAX_PERIODIC_PIPES];
    UINT32 num_active_qh_periodic;
    EHCI_ALIGNED_MEM_POOL qh_pool[USBH_EHCI_MAX_QH_POOLS];
    EHCI_ALIGNED_MEM_POOL qtd_pool[USBH_EHCI_MAX_QTD_POOLS];

    /*  for protecting internal lists, rm_list, mod_list */

    NU_SEMAPHORE protect_sem;

    UINT32 intr_status;

    /* Roothub stuff    */

    USBH_EHCI_RH_STATUS rh_status;
    UINT8 rh_hub_desc[20];
    UINT32 int_disable_count; /* counter to keep track of interrupt
                               * disable count. */
    UINT32 next_uframe;

    UINT32  available_current;

    VOID   *ses_handle;

    UINT8   padding[3];
}
NU_USBH_EHCI;

#define USBH_EHCI_PROTECT     ehci_disable_interrupts(ehci);
#define USBH_EHCI_UNPROTECT   ehci_enable_interrupts(ehci);


#define USBH_EHCI_HASH_FUNC(key)  \
  ((((UINT32)key) ^ (((UINT32)key)>> 5)) % (USBH_EHCI_HASH_SIZE))

#define USB_HW_READ16(cb, p, d)     d = HOST_2_LE16( ESAL_GE_MEM_READ16(p) )
#define USB_HW_READ32(cb, p, d)     d = HOST_2_LE32( ESAL_GE_MEM_READ32(p) )
#define USB_HW_WRITE16(cb, p, d)    ESAL_GE_MEM_WRITE16(p, LE16_2_HOST(d))
#define USB_HW_WRITE32(cb, p, d)    ESAL_GE_MEM_WRITE32(p, LE32_2_HOST(d))

#define EHCI_HW_READ32(cb,p,d)      USB_HW_READ32(cb, p, d)
#define EHCI_HW_WRITE32(cb,p,d)     USB_HW_WRITE32(cb, p, d)

#define EHCI_HW_SET_BITS(cb,p,bits)     {   UINT32 temp; \
                                        EHCI_HW_READ32(cb, (p), temp); \
                                        temp |= (bits); \
                                        EHCI_HW_WRITE32 (cb, (p), temp); }

#define EHCI_HW_CLR_BITS(cb,p,bits)     {   UINT32 temp; \
                                        EHCI_HW_READ32(cb, (p), temp); \
                                        temp &= ~(bits); \
                                        EHCI_HW_WRITE32 (cb, (p), temp); }

#define EHCI_HW_READ_REG(ehci,y,z)   EHCI_HW_READ32(ehci,\
                                     ehci->ehci_oprreg_base + (y), (z))


#define EHCI_HW_WRITE_REG(ehci,y,z) EHCI_HW_WRITE32(ehci,\
                                    (ehci->ehci_oprreg_base + (y)), (z))


#define EHCI_CPU_2_BUS(cb,a,b)   b = a
#define EHCI_BUS_2_CPU(cb,a,b)   b = a

#define EHCI_READ_ADDRESS(cb, p, d)     { UINT32 t1,t2; \
                                          EHCI_HW_READ32(cb, (p), t1); \
                                          t2 = t1 & ~0x1F; \
                                          t1 &= 0x1F; \
                                          EHCI_BUS_2_CPU(cb, t2, (d)); \
                                          (d) |= t1; }

#define EHCI_WRITE_ADDRESS(cb, p, d)     { UINT32 t1, t2; \
                                           t2 = (d) & (~0x1F); \
                                           t1 = (d) & (0x1F);  \
                                           EHCI_CPU_2_BUS(cb, t2, t2); \
                                           t2 |= t1; \
                                           EHCI_HW_WRITE32(cb, (p), t2); }

#define USBH_EHCI_RH_GET_BUFFER(irp) NU_USB_IRP_Get_Data(\
(NU_USB_IRP *)irp,&irp_buffer)

#define USBH_EHCI_RH_GET_LENGTH(irp) NU_USB_IRP_Get_Length(\
(NU_USB_IRP *)irp,&irp_length)

#define USBH_EHCI_RH_SET_LENGTH(irp, x) NU_USB_IRP_Set_Actual_Length(\
(NU_USB_IRP *)irp,x)

#define USBH_EHCI_RH_SET_STATUS(irp, x) NU_USB_IRP_Set_Status(\
(NU_USB_IRP *)irp, x)

#define USBH_EHCI_RH_LSB(wValue)    (UINT8)(wValue & 0xFF)
#define USBH_EHCI_RH_MSB(wValue)    (UINT8)((wValue >> 8) & 0xFF)

#define USBH_EHCI_RH_GET_RECIPIENT(bmRequestType)  (bmRequestType & 0x0F)
#define USBH_EHCI_RH_IS_CLASS_SPECIFIC(bmRequestType) \
                          ((bmRequestType & 0x60) == 0x20)

#define RH_GET_RECIPIENT(irp)   (((irp)->setup_data.bmRequestType) & 0x0F)

#define  CAPLENGTH        0x00  /* Capability Register length */
#define  RESERVED         0x01  /* Reserved                   */
#define  HCIVERSION       0x02  /* Interface Version Number   */
#define  HCSPARAMS        0x04  /* Structural Parameters      */
#define  HCCPARAMS        0x08  /* Capability Parameters      */
#define  HCSP_PORTROUTE   0x0c  /* Companion Port Route Description */

/* HCSParams */
#define  HCIVERSION_DEFAULT             0x0100
#define  HCS_DEBUG_PORT_NUMBER(p)       (((p)>>20) & 0x0f)
#define  HCS_PORT_INDICATOR(p)          ((p) & ( 1 << 16 ))
#define  HCS_NUM_CC(p)                  (((p) >> 12 ) & 0x0f)
#define  HCS_NUM_PORT_PER_CC(p)         (((p) >> 8 )  & 0x0f)
#define  HCS_PORT_ROUTING(p)            (( p) & ( 1 << 7 ))
#define  HCS_PORT_POWER_CONTROL(p)      (( p) & ( 1 << 4 ))
#define  HCS_NUM_PORTS(p)               (( p) & 0x0f )

/* HCCParams */
#define  HCC_EXT_CAPS(p)                (( p >> 8 ) & 0xff)
#define  HCC_ISO_THRESHOLD(p)           (( p >> 4 ) & 0x07)
#define  HCC_ISO_CASHE(P)               ((p) & ( 1 << 7))
#define  HCC_ASYNC_PARK(P)              (( p) & (1 << 2))
#define  HCC_FRAME_LIST_FLAG(P)         (( p) & ( 1 << 1))
#define  HCC_64BIT_ADDR(p)              (( p) & 0x01)


/* Host Controller Operational Registers  spec 2.3  */

#define  USBCMD             0x00    /* USB Command      */
#define  USBSTS             0x04    /* USB STATUS       */
#define  USBINTR            0x08    /* USB Interrupt enable */
#define  FRINDEX            0x0c    /* USB Frame Index      */
#define  CTRLDSSEGMENT      0x10    /* 4G segment selector  */
#define  PERIODICLISTBASE   0x14    /* Frame List Base Address */
#define  ASYNCLISTADDR      0x18    /* Next Asynchronous List Address */
#define  CONFIGFLAG         0x40    /* Configured Flag Register   */
#define  PORTSC             0x44    /* Port Status/Control        */


/* USBcmd */
#define CMD_INTERRUPT_THRESHOLD(p)      (((p)>>16) & 0xFF )
#define DEFAULT_INTERRUPT_THRESHOLD     0x08
#define CMD_ASYNC_PARK_ENABLE           ( 0x800 )
#define CMD_ASYNC_PARK_COUNT(p)         (((p) >> 8 ) & 0x03 )
#define CMD_LHC_RESET                   ( 0x80 )
#define CMD_IAAD                        ( 0x40 )
#define CMD_ASYNC_ENABLE                ( 0x20 )
#define CMD_PERIODIC_ENABLE             ( 0x10 )
#define CMD_FRAME_LIST_SIZE(p)          (( ( p) >> 2) & 0x03)
#define CMD_HC_RESET                    (0x02 )
#define CMD_HC_RUN                      (0x01 )


/* USBStatus */
/*Asynchronous Schedule Status */
#define EHCI_STS_ASYNC                 (0x8000 )
/* Periodic Schedule Status */
#define EHCI_STS_PERIODIC              (0x4000 )
#define EHCI_STS_RECLAMATION           (0x2000 )/* Reclamation */
#define EHCI_STS_HC_HALT               (0x1000 )/* Hc Halted   */
/* Interrupt on Async Advance */
#define EHCI_STS_AAE                   (0x20  )
#define EHCI_STS_HOST_ERROR            (0x10  )/* Host System Error  */
#define EHCI_STS_FRAME_LIST_ROLLOVER   (0x08  )/* Frame List Rollover */
#define EHCI_STS_PORT_CHANGE           (0x04  )
#define EHCI_STS_ERR_INTERRUPT         (0x02  )
/* Completion of USB Transaction */
#define EHCI_STS_INTERRUPT             (0x01  )

/* USB Interrupt Enable Register */
#define EHCI_INTR_AAE                  (0x20 )
#define EHCI_INTR_HOST_ERROR           (0x10 )
#define EHCI_INTR_FRAME_LIST_ROLLOVER  (0x08 )
#define EHCI_INTR_PORT_CHANGE          (0x04 )
#define EHCI_INTR_ERR_INTERRUPT        (0x02 )
#define EHCI_INTR_INTERRUPT            (0x01 )


/* Configure Flag Register  */

#define FLAG_CF                    (0x01 )


/* Hub Class defined values */

#define PORT_CONNECTION         0
#define PORT_ENABLE             1
#define PORT_SUSPEND            2
#define PORT_OVERCURRENT        3
#define PORT_RESET              4
#define PORT_POWER              8

#define C_PORT_CONNECTION       16
#define C_PORT_ENABLE           17
#define C_PORT_SUSPEND          18
#define C_PORT_OVER_CURRENT     19
#define C_PORT_RESET            20

#define EHCI_PORTSC             0x44

#define EHCI_GET_PORT_REG(base, port) ((EHCI_PORTSC + (((port) - 1) << 2)))

#define EHCI_PORT_CONNECT               (0x1 << 0)
#define EHCI_PORT_CONNECT_CHANGE        (0x1 << 1)
#define EHCI_PORT_ENABLED               (0x1 << 2)
#define EHCI_PORT_ENABLE_CHANGE         (0x1 << 3)
#define EHCI_PORT_OVERCURRENT           (0x1 << 4)
#define EHCI_PORT_OVERCURRENT_CHANGE    (0x1 << 5)
#define EHCI_PORT_SUSPEND               (0x1 << 7)
#define EHCI_PORT_FORCE_RESUME          (0x1 << 6)
#define EHCI_PORT_RESET                 (0x1 << 8)
#define EHCI_PORT_LINESTATUS            ((0x1 << 10) | (0x1 << 11))
#define EHCI_PORT_POWER                 (0x1 << 12)
#define EHCI_PORT_OWNER                 (0x1 << 13)

/* itd Transaction Status and Control List */
/*Active Transfer for this slot */
#define EHCI_ITD_ACTIVE         (0x80000000 )
#define EHCI_ITD_BUF_ERR        (0x40000000 )   /* Data Buffer Error  */
#define EHCI_ITD_BABBLE         (0x20000000 )   /* Babble Detected    */
#define EHCI_ITD_TRA_ERR        (0x10000000 )   /* Transaction Error  */
#define EHCI_ITD_TRA_LENGTH(p)  (((p) >> 16 ) & 0x0fff )
#define EHCI_ITD_IOC            ( 0x8000 )  /* Interrupt On Complete */
#define EHCI_ITD_PG_SELECT(p)   (((p) >> 12 ) & 0x07 )  /* Page Select */
#define EHCI_ITD_OFFSET         0x0fff  /* Transaction Offset */


/* itd Buffer Page Pointer List */
#define EHCI_ITD_BUFFER_PTR(p) (((p)>> 12 ) & 0xfffff )
/* End Point Number */
#define EHCI_ITD_EP(p)         (((p)>> 8 ) &  0x0f  )
#define EHCI_ITD_DA            0x7f         /* Device Address   */
#define EHCI_ITD_DIR           ( 0x800 )    /* Direction        */
#define EHCI_ITD_MAX_PKT       0x7ff        /* Max Packet Size  */
/* no of Transaction per micro frame */
#define EHCI_ITD_MULTI         0x03


#define EHCI_SITD_DIRECTION     (0x80000000 )
#define EHCI_SITD_PORT_NUM(p)   (((p) >> 24 ) & 0x7f )
#define EHCI_SITD_HUB_ADDR(p)   (((p) >> 16 ) & 0x7f )
#define EHCI_SITD_EP(p)         (((p) >> 8 )  & 0x0f )
#define EHCI_SITD_DA            0x7f

/* micro-frame Schedule Control */
#define EHCI_SITD_COMPLETION_MASK(p)   ((( p) >> 8 ) & 0xff)
#define EHCI_SITD_START_MASK           0xff
/* sitd Transfer state */
#define EHCI_SITD_IOC        ( 0x80000000 ) /* Interrupt On complete */
#define EHCI_SITD_PG_SELECT      ( 0x40000000 ) /* Page Select */
#define EHCI_SITD_TOTAL_BYTES(p) (((p) >> 16 ) & 0x3ff)
                                            /* Total bytes to transfer */
/* sitd Transfer Status */
#define EHCI_SITD_ACTIVE     (0x80 )
#define EHCI_SITD_ERR      (0x40 )
#define EHCI_SITD_BUFFER_ERR   (0x20 )
#define EHCI_SITD_BABBLE     (0x10 )
#define EHCI_SITD_TX_ERR     (0x08 )    /* Transaction Error */
#define EHCI_SITD_MISS_FRAME   (0x04 )  /* missed Micro Frame */
#define EHCI_SITD_Tx_STATE     (0x02 )  /* Split transaction state */

/* sitd buffer Pointer list */

#define EHCI_SITD_BUFFER_PTR(p)   (((p)>> 12 ) & 0xfffff )
#define EHCI_SITD_OFFSET          0x0fff
/* Transaction Position */
#define EHCI_SITD_TP(p)           (((p) >> 3 ) & 0x03 )

#define EHCI_SITD_TX_COUNT        0x07  /* Transaction Count */
#define EHCI_SITD_BACK_PTR(p)     (((p) >> 5 ) & 0x07ffffff )
#define EHCI_SITD_TERMINATE       0x01



#define EHCI_QTD_NEXT_PTR(p)     (((p) >> 5 ) & 0x07ffffff )
#define EHCI_QTD_TERMINATE       0x01
#define EHCI_QTD_ALT_NEXT_PTR(p) (((p) >> 5 ) & 0x07ffffff )
#define EHCI_QTD_TOTAL_BYTES(p)  (((p) >> 16) & 0x3ff )
#define EHCI_QTD_IOC             (0x8000)
#define EHCI_QTD_CUR_PG(p)       (((p) >> 12 ) & 0x07 )
#define EHCI_QTD_ERR_COUNT(p)    (((p) >> 10 ) & 0x03 )
#define EHCI_QTD_PID(p)          (((p) >> 8 ) & 0x03 )
#define EHCI_QTD_STS             0xff

#define EHCI_QTD_ACTIVE    (0x80 )
#define EHCI_QTD_HALT      (0x40 )
#define EHCI_QTD_BUFFER_ERR  (0x20 )
#define EHCI_QTD_BABBLE    (0x10 )
#define EHCI_QTD_TX_ERR    (0x08 )
#define EHCI_QTD_MISS_FRAME  (0x04 )
#define EHCI_QTD_SPLIT_STATE (0x02 )
#define EHCI_QTD_PING_STATE  (0x01 )


#define EHCI_QH_LP(p)         (((p) >> 5 ) & 0x07ffffff)
#define EHCI_QH_TYPE(p)       (((p) >> 1 ) & 0x03 )
#define EHCI_QH_TERMINATE     0x01
#define EHCI_QH_NAK_RL(p)     (((p) >> 28 ) & 0x0f )
#define EHCI_QH_EP_FLAG     ( 1 << 27 )
#define EHCI_QH_MAX_PKT(p)    (((p) >>16 ) & 0x7ff )
#define EHCI_QH_HEAD          (1 << 15 )/* Head of Reclamation Flag */
#define EHCI_QH_DTC           (1 << 14 )/* Data Toggle Control */
#define EHCI_QH_SPEED(p)      (((p)>> 12 ) & 0x03 )
#define EHCI_QH_EP_NUM(p)     (((p) >> 8 ) & 0x0f )
/* Inactivate On Next Transaction */
#define EHCI_QH_NEXT_TX       (1 << 7 )
#define EHCI_QH_DA            0x7f  /* Device Address */
/* High-Bandwidth Pipe Multiplier */
#define EHCI_QH_MULT(p)       (((p) >> 30 ) & 0x03 )
#define EHCI_QH_PORT_NUM(p)   (((p) >> 23 ) & 0x7f )
#define EHCI_QH_HUB_ADDR(p)   (((p) >> 16 ) & 0x7f )
 /*Split Completion Mask */
#define EHCI_QH_C_MASK(p)     (((p) >> 8 ) & 0xff )
#define EHCI_QH_S_MASK        0xff  /* Interrupt Schedule Mask */
/* control Bits in Overlay */
#define EHCI_QH_NAK_CNT(p)    (((p) >> 1 ) & 0x0f ) /* Nak counter */
#define EHCI_QH_DATA_TOGGLE   ( 1 << 31 )   /* Data Toggle */
#define EHCI_QH_IOC           ( 1 << 15 )
#define EHCI_QH_C_ERR(p)      (((p) >> 10 ) & 0x03 )
#define EHCI_QH_PING          0x01

    /* Split-transaction Complete-split Progress */
#define EHCI_QH_FRAME_TAG     0x1f  /* Split -Transaction Frame Tag */
#define EHCI_QH_S_BYTES(p)    (((p) >> 5 ) & 0x7f


/* Figure 3.8 Periodic Frame Traversal Node (FSTN) */
#define EHCI_FSTD_NPLP(p)    (((p) >> 5 ) & 0x07ffffff )
#define EHCI_FSTD_TYPE(p)    (((p) >> 1 ) & 0x03 )
#define EHCI_FSTD_TERMINATE  0x01


#define PORT_TEST           (0xF0000)
#define PORT_INDICATOR      (0xC000)

#define NO_CACHING            0
#define FRAME_CACHING         1
#define MICRO_FRAME_CACHING   2

#define LINK_POINTER_TYPE_ITD  0x00
#define LINK_POINTER_TYPE_QH   0x01
#define LINK_POINTER_TYPE_SITD 0x02
#define LINK_POINTER_TYPE_FSTN 0x03


#define EHCI_QTD_CANCELLED   1
#define EHCI_QTD_RETIRED     2

#define EHCI_ITD_CANCELLED   3
#define EHCI_ITD_RETIRED     4


#define USBH_EHCI_RH_RECIPIENT_DEVICE      0
#define USBH_EHCI_RH_RECIPIENT_INTF        1
#define USBH_EHCI_RH_RECIPIENT_ENDP        2
#define USBH_EHCI_RH_RECIPIENT_PORT        3

#define USBH_EHCI_RH_PORT_STATUS_MASK 0xFFFFFFFF


/* ====================  Function Prototypes ========================== */


STATUS ehci_schedule_intr_pipe (NU_USBH_EHCI * ehci,
                                USBH_EHCI_QH_INFO * qh_info);

STATUS ehci_rh_handle_irp (NU_USBH_EHCI * ehci,
                           NU_USB_IRP * irp,
                           UINT8 bEndpointAddress);
STATUS ehci_rh_init (NU_USBH_EHCI * ehci);

VOID ehci_rh_isr (NU_USBH_EHCI * ehci);



UINT32 ehci_qh_key (UINT8 function_address,
                    UINT8 bEndpointAddress);
STATUS ehci_alloc_qh (NU_USBH_EHCI * ehci,
                      USBH_EHCI_QH ** qh_ptr,
                      UNSIGNED suspend);
STATUS ehci_alloc_qtd (NU_USBH_EHCI * ehci,
                       USBH_EHCI_QTD ** qtd_ptr,
                       UNSIGNED suspend);


STATUS ehci_dealloc_qh (NU_USBH_EHCI * ehci,
                        USBH_EHCI_QH * qh_ptr);
STATUS ehci_dealloc_qtd (NU_USBH_EHCI * ehci,
                         USBH_EHCI_QTD * qtd_ptr);

UINT8 ehci_normalize_interval (UINT32 interval);


STATUS hash_add_qh (NU_USBH_EHCI * ehci,
                    USBH_EHCI_HASH ** ht,
                    USBH_EHCI_QH_INFO * qhinfo);
STATUS hash_delete_qh (NU_USBH_EHCI * ehci,
                       USBH_EHCI_QH_INFO * qhinfo);
USBH_EHCI_QH_INFO *hash_find_qh (USBH_EHCI_HASH ** ht,
                                 UINT32 key);
UINT32 ehci_find_best_schedule (NU_USBH_EHCI * ehci,
                                USBH_EHCI_QH_INFO * qhinfo);
UINT8 ehci_find_best_schedule_uframe (NU_USBH_EHCI * ehci,
                                      UINT32 branch,
                                      UINT8 uframe,
                                      USBH_EHCI_QH_INFO * qhinfo);

VOID ehci_interrupt_qh_link (NU_USBH_EHCI * ehci,
                             USBH_EHCI_QH_INFO * qhinfo);
VOID ehci_scheduleAsyncPipe (NU_USBH_EHCI * ehci,
                             USBH_EHCI_QH_INFO * qhinfo);
VOID ehci_schedulePeriodicPipe (NU_USBH_EHCI * ehci);

VOID ehci_unlink_qtds (NU_USBH_EHCI * ehci,
                       USBH_EHCI_QH_INFO * qhinfo);
VOID ehci_hndl_retired_qtd (NU_USBH_EHCI * ehci,
                            USBH_EHCI_QTD * qtd,
                            UINT8 flag);
VOID ehci_remove_non_periodic_qh (NU_USBH_EHCI * ehci,
                                  USBH_EHCI_QH_INFO * qhinfo);
VOID ehci_remove_periodic_qh (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qhinfo);
VOID ehci_hndl_periodic_rm_list (NU_USBH_EHCI * ehci,
                                 UINT8 list_type);
VOID ehci_hndl_async_rm_list (NU_USBH_EHCI * ehci,
                              UINT8 list_type);

VOID ehci_process_modify_list (NU_USBH_EHCI * ehci);
VOID ehci_schedule (NU_USBH_EHCI * ehci);
VOID ehci_scan_async_list (NU_USBH_EHCI * ehci);
VOID scan_periodic_list (NU_USBH_EHCI * ehci);
STATUS ehci_qh_handle_irp (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           NU_USB_IRP * irp);

VOID ehci_normalize_status (NU_USBH_EHCI * ehci,
                            USBH_EHCI_QTD * qtd,
                            STATUS *tx_status_out);

VOID ehci_check_retire_transfer (NU_USBH_EHCI * ehci,
                                 USBH_EHCI_QH_INFO * qhinfo);

STATUS ehci_retire_fs_iso_transfer (NU_USBH_EHCI * ehci,
                                 USBH_EHCI_QH_INFO * qhinfo,
                                 USBH_EHCI_ISO_IRP *irp,USBH_EHCI_SITD *psitd,
                                 UINT32 flag);



STATUS ehci_retire_hs_iso_transfer (NU_USBH_EHCI * ehci,
                                    USBH_EHCI_QH_INFO* qhinfo,
                                    USBH_EHCI_ISO_IRP *irp_priv,
                                    USBH_EHCI_ITD *,
                                    UINT32 flag);


VOID ehci_retire_transfer (NU_USBH_EHCI * ehci,
                           USBH_EHCI_IRP * transfer,
                           USBH_EHCI_BURST * burst,
                           STATUS tx_status);

VOID ehci_retire_qh (NU_USBH_EHCI * ehci,
                     USBH_EHCI_QH_INFO * qhinfo,
                     STATUS tx_status);

STATUS ehci_schedule_periodic_qh (NU_USBH_EHCI * ehci,
                                  USBH_EHCI_QH_INFO * qh_info,
                                  UINT32 interval);

VOID ehci_scan_periodic_list (NU_USBH_EHCI * ehci);

VOID ehci_unlink_periodic_qh (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qhinfo);

VOID ehci_unlink_async_qh (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qh_info);

VOID ehci_link_async_qh (NU_USBH_EHCI * ehci,
                         USBH_EHCI_QH_INFO * qh_info);

STATUS ehci_prepare_burst (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           USBH_EHCI_IRP * transfer,
                           USBH_EHCI_BURST * burst);
STATUS ehci_alloc_sitd( NU_USBH_EHCI * ehci,
                        USBH_EHCI_SITD ** sitd_ptr,
                        UNSIGNED suspend);

STATUS ehci_dealloc_sitd (NU_USBH_EHCI * ehci,
                         USBH_EHCI_SITD * sitd_ptr);



STATUS ehci_handle_iso_irp(NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           NU_USB_IRP *irp);


VOID ehci_normalize_itd_trans_status (NU_USBH_EHCI * ehci,
                            UINT32 status,
                            STATUS *tx_status_out,UINT8 direction);

VOID ehci_normalize_sitd_trans_status (NU_USBH_EHCI * ehci,
                            UINT32 status,
                            STATUS *tx_status_out,
                            UINT8 direction);


STATUS ehci_schedule_iso_sitd (NU_USBH_EHCI * ehci,
                               USBH_EHCI_QH_INFO * qh_info,
                               USBH_EHCI_SITD *sitd,
                               UINT32 interval,
                               UINT8 direction);

STATUS ehci_schedule_iso_itd (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qh_info,
                              USBH_EHCI_ITD *pitd,
                              UINT32 frame);

UINT32 ehci_fill_sitd (NU_USBH_EHCI *ehci,
                      USBH_EHCI_SITD *sitd,
                      UINT8 *buf,
                      UINT32 length,
                      UINT8 ioc,
                      UINT8 direction,
                      UINT32 mask,
                      UINT32 ssplit_info,
                      UINT32 address);

UINT32 ehci_fill_itd (NU_USBH_EHCI * ehci,
                      USBH_EHCI_ITD * itd,
                      UINT8 *buf,
                      UINT32 length,
                      UINT16 uframe,
                      UINT8 ioc,
                      UINT32 address,
                      UINT8 direction);

VOID ehci_delete_iso_tds(USBH_EHCI_ISO_QH_INFO* qhinfo);

VOID ehci_disable_interrupts(NU_USBH_EHCI   *ehci);

VOID ehci_enable_interrupts(NU_USBH_EHCI    *ehci);

VOID ehci_scan_isochronous_tds (NU_USBH_EHCI * ehci);

STATUS ehci_transfer_done(NU_USBH_EHCI*, UINT8*, UINT8*, UINT8*, UINT32, BOOLEAN);
STATUS ehci_normalize_buffer(NU_USBH_EHCI*, UINT8*, UINT32, UINT32, BOOLEAN, UINT8**, UINT8**);

/* ==================================================================== */

#include "drivers/nu_usbh_ehci_dat.h"

/* ==================================================================== */

#endif /* _NU_USBH_EHCI_IMP_H_ */

/* =======================  End Of File  ============================== */
