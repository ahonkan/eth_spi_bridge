/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME            
*
*       nu_usbh_hid_imp.h    
*                    
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for HID Class Driver.
*
* DATA STRUCTURES
*
*       NU_USBH_HID                         HID Class Driver Control Block.
*       USBH_HID_DEVICE                     This structure maintains
*                                           information about each HID
*                                           Interface served by this class
*                                           driver.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_hid_dat.h                   Dispatch Table Definitions.
*
**************************************************************************/

#ifndef _NU_USBH_HID_IMP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define _NU_USBH_HID_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

#include "connectivity/nu_usbh_ext.h"

/* =====================  #defines ===================================  */

#ifdef NU_USB_ASSERT
    #define NU_USBH_HID_ASSERT(A)   NU_USB_ASSERT(A)
#else
    #define NU_USBH_HID_ASSERT(A)   ((VOID)0)
#endif
/*
 * HID report item format.
 */

#define USBH_HID_ITEM_FORMAT_SHORT          0
#define USBH_HID_ITEM_FORMAT_LONG           1

/*
 * Special tag indicating long items.
 */

#define USBH_HID_ITEM_LONG                  15

/*
 * HID report descriptor item type (prefix bit 2,3).
 */

#define USBH_HID_ITEM_TYPE_MAIN             0
#define USBH_HID_ITEM_TYPE_GLOBAL           1
#define USBH_HID_ITEM_TYPE_LOCAL            2
#define USBH_HID_ITEM_TYPE_RESERVED         3

/*
 * HID report descriptor main item tags.
 */

#define USBH_HID_MAIN_ITEM_INPUT            8
#define USBH_HID_MAIN_ITEM_OUTPUT           9
#define USBH_HID_MAIN_ITEM_FEATURE          11
#define USBH_HID_MAIN_ITEM_BEG_COLL         10
#define USBH_HID_MAIN_ITEM_END_COLL         12


/*
 * HID report descriptor global item tags.
 */

#define USBH_HID_GLOBAL_ITEM_USAGE_PAGE     0
#define USBH_HID_GLOBAL_ITEM_LOG_MIN        1
#define USBH_HID_GLOBAL_ITEM_LOG_MAX        2
#define USBH_HID_GLOBAL_ITEM_PHY_MIN        3
#define USBH_HID_GLOBAL_ITEM_PHY_MAX        4
#define USBH_HID_GLOBAL_ITEM_UNT_EXP        5
#define USBH_HID_GLOBAL_ITEM_UNIT           6
#define USBH_HID_GLOBAL_ITEM_REP_SIZE       7
#define USBH_HID_GLOBAL_ITEM_REP_ID         8
#define USBH_HID_GLOBAL_ITEM_REP_COUNT      9
#define USBH_HID_GLOBAL_ITEM_PUSH           10
#define USBH_HID_GLOBAL_ITEM_POP            11

/*
 * HID report descriptor local item tags.
 */

#define USBH_HID_LOCAL_ITEM_USAGE            0
#define USBH_HID_LOCAL_ITEM_USAGE_MIN        1
#define USBH_HID_LOCAL_ITEM_USAGE_MAX        2


/* HID report type.  */

#define USBH_HID_IN_REPORT                  0x01
#define USBH_HID_OUT_REPORT                 0x02
#define USBH_HID_FEATURE_REPORT             0x03
#define USBH_HID_BEGIN_COLLECTION           0x04
#define USBH_HID_END_COLLECTION             0x05

/* In USAGE TABLE document maximum usage id used is 0x91. */

#define USBH_HID_MAX_USAGE                  0x91

#define NU_USB_MAX_HID_REPORT_SIZE          20
#define NU_USB_MAX_HID_REPORT_DESC_SIZE     200


#define USBH_HID_MAX_COLL_CHILDS            2
#define USBH_HID_MAX_SUBDESCRIPTORS         4
#define USBH_HID_MAX_STACK_SIZE             13

#define USBH_HID_BMREQTYPE_GET_DESC         0x81
#define USBH_HID_BMREQTYPE_HID_GET          0xA1
#define USBH_HID_BMREQTYPE_HID_SET          0x21


#define USBH_HID_BREQUEST_GET_DESC          0x06

#define USBH_HID_DESCTYPE_HID               0x21
#define USBH_HID_DESCTYPE_REPORT            0x22
#define USBH_HID_DESCTYPE_PHYSICAL          0x23


#define USBH_HID_BREQUEST_GET_REPORT        0x01
#define USBH_HID_BREQUEST_GET_IDLE          0x02
#define USBH_HID_BREQUEST_GET_PROTOCOL      0x03

#define USBH_HID_BREQUEST_SET_REPORT        0x09
#define USBH_HID_BREQUEST_SET_IDLE          0x0A
#define USBH_HID_BREQUEST_SET_PROTOCOL      0x0B

#define USBH_HID_WVALUE_REPORT_PROTOCOL     0x01

#if (NU_USB_OPTIMIZE_FOR_SIZE)

/* Keyboard user driver requires five usages for its operation.It includes three usages of LED usage page which are Num Lock, Caps Lock and Scroll 
 * Lock and two usages of keyboard that implies that only two keys can be pressed simultaneouly on the keyboard. On the other hand, mouse user driver    
 * requires four usages for its operation. It includes three usages of Generic desktop page which are X, Y and wheel and one usage of Button page       
 * which is button 1 of primary usage.
 */
#define NU_USB_MAX_HID_USAGES               5

#define USBH_HID_MAX_COLLECTIONS            2
#define USBH_HID_MAX_REPORT_IDS             0x05

/* USB HID task stack size required for creation
 * of the class driver.
 */
#define USBH_HID_TASK_STACK_SIZE            6 * NU_MIN_STACK_SIZE
#else
#define NU_USB_MAX_HID_USAGES               150
#define USBH_HID_MAX_COLLECTIONS            24
#define USBH_HID_MAX_REPORT_IDS             0x60

/* USB HID task stack size required for creation
 * of the class driver.
 */
#define USBH_HID_TASK_STACK_SIZE            2048
#endif /* NU_USB_OPTIMIZE_FOR_SIZE */

/* USB HID task priority required for creation
 *  of the class driver for NMI.
 */
#define USBH_HID_TASK_PRIORITY              13

/* HID error codes . */
#define NU_USBH_HID_DUPLICATE_INIT         -100

/* ====================  Data Types ==================================  */
typedef struct nu_usbh_hid_user NU_USBH_HID_USER;

/* ====== Internal Data Structures ===== */

typedef struct hid_report_info
{
    UINT32 report_size;
    UINT32 report_id;
    UINT32 type;
}USBH_HID_REPORT_INFO;


struct hid_sub_descriptor
{
    UINT8   bDescriptorType;
    UINT8   wDescriptorLengthlow;
    UINT8   wDescriptorLengthhigh;
    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[1];
};

typedef struct hid_descriptor
{
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bcdHIDlow;
    UINT8   bcdHIDhigh;
    UINT8   bCountryCode;
    UINT8   bNumDescriptors;
    UINT8   sub_desc[USBH_HID_MAX_SUBDESCRIPTORS][3];

    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[2]; 
}USBH_HID_DESCRIPTOR;

/* Structure for storing ITEM information. */

typedef struct _hid_item_prefix
{
    UINT8   item_prefix;
    UINT8   bSize;
    UINT8   bType;
    UINT8   bTag;
    INT32   item_signed_data;
    UINT32  item_data;
}
USBH_HID_ITEM_PREFIX;


/* Structure for storing State of Global ITEM. */

typedef struct _hid_global_items
{
    UINT32 report_id;
    UINT32 report_size;
    UINT32 report_count;
    UINT32 usage_page;
    INT32 logical_min;
    INT32 logical_max;
    INT32 physical_min;
    INT32 physical_max;
    INT32 unit_exponent;
    INT32 unit;

}
USBH_HID_GLOBAL_ITEMS;


/* Structure for storing State of LOCAL ITEM. */

typedef struct _hid_local_prefix
{
    UINT32 usage;
    UINT32 usage_min;
    UINT32 usage_max;
    UINT32 usage_active;

    UINT32 num_usage_ids;
    UINT32 usage_ids[NU_USB_MAX_HID_USAGES - 1];

}
USBH_HID_LOCAL_ITEMS;

typedef struct _nu_usbh_hid_usage
{
    UINT16 usage_page;
    UINT16 usage_id;

}NU_USBH_HID_USAGE;


/* Structure for collection in tree structure.  */
typedef struct collection
{
    UINT8 collection_type;
    UINT8 number_of_child;
    UINT8 max_number_child;
    UINT8 unused;
    UINT32 usage;
    struct collection *parent;

    struct collection *child[USBH_HID_MAX_COLL_CHILDS];
}
USBH_HID_COLLECTION;


/* Structure for storing state of ITEM and used when MAIN ITEM encounter.
 */

typedef struct _hid_item_state_table
{
    INT32 global_storage[10];
    UINT8 global_storage_sign[10];
    UINT8 coll_ptr;
    UINT8 unused1;

    USBH_HID_LOCAL_ITEMS local_items;

    INT32    global_storage_stack[USBH_HID_MAX_STACK_SIZE][10];
    UINT8    global_storage_stack_top;

    USBH_HID_COLLECTION *collection_stack[USBH_HID_MAX_COLLECTIONS];

    struct _hid_device *hid_device;
}
USBH_HID_ITEM_STATE_TABLE;


typedef struct _nu_usbh_hid_item
{
    UINT32 usage_page;

    INT32 logical_min;
    INT32 logical_max;

    INT32 physical_min;
    INT32 physical_max;

    INT32 unit_exponent;
    INT32 unit;

    UINT32 report_size;
    UINT32 report_count;
    UINT32 report_type;
    UINT32 report_id;

    UINT32 usage;
    UINT32 usage_min;
    UINT32 usage_max;
    UINT32 usage_ids[NU_USB_MAX_HID_USAGES - 1];
    UINT32 num_usage_ids;

    UINT32 designator_index;
    UINT32 designator_min;
    UINT32 designator_max;

    UINT32 main_data;
    UINT32 report_offset;

}
NU_USBH_HID_ITEM;


typedef struct _hid_usage
{
    NU_USBH_HID_ITEM    *item;

    USBH_HID_COLLECTION *collection;

    NU_USBH_HID_USER    *user;

}
USBH_HID_USAGE;




/* Each HID device connected to this Class Driver is
 * remembered by this structure.
 */

typedef struct _hid_device
{
    CS_NODE node;
    /* Session identification. */
    NU_USB_DRVR *drvr;
    NU_USB_DEVICE *device;
    NU_USB_STACK *stack;
    NU_USB_INTF *intf;
    NU_USB_ALT_SETTG *alt_settg;
    /* Protect access per drive. */
    NU_SEMAPHORE    hid_lock;
    NU_SEMAPHORE    cntrl_irp_complete;
    /* Pipes and irps required by the driver. */
    NU_USB_PIPE *control_pipe;
    NU_USB_PIPE *interrupt_pipe;
    NU_USB_IRP  interrupt_irp;
    /* report data is stored in this. */
    UINT8 *raw_report;
    UINT8 *control_buffer;
    /* hid descriptor pointer. */
    USBH_HID_DESCRIPTOR  *hid_desc;
    UINT16 num_report_ids;
    UINT16 in_report_size;
    UINT16 out_report_size;
    UINT16 feature_report_size;
    USBH_HID_REPORT_INFO input_report_info[USBH_HID_MAX_REPORT_IDS];
    USBH_HID_REPORT_INFO output_report_info[USBH_HID_MAX_REPORT_IDS];
    USBH_HID_REPORT_INFO feature_report_info[USBH_HID_MAX_REPORT_IDS];
    UINT8 num_items;
    USBH_HID_USAGE usages[NU_USB_MAX_HID_USAGES];
    UINT8 num_of_coll;
    UINT8 int_irp_completed;
    USBH_HID_COLLECTION *collection[USBH_HID_MAX_COLLECTIONS];

    BOOLEAN Connected;
    
    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[3];
}
USBH_HID_DEVICE;

/* ============= Control Block ============= */

/* HID Class Driver Control Block. */

typedef struct nu_usbh_hid
{
    NU_USB_DRVR     cb;

    NU_TASK         task;

    USBH_HID_DEVICE *dev_list_head;

    NU_SEMAPHORE    ev_lock;

    NU_MEMORY_POOL  *pool;

    /* This semaphore is used to synchronize interrupt
     * and control IRP submission thread.
     */

    NU_SEMAPHORE    intr_irp_complete;

}
NU_USBH_HID;

/* ====================  Function Prototypes ========================== */

/* Class Driver API implementation prototypes. */

STATUS USBH_HID_Connect_Users ( NU_USBH_HID     *cb,
                                USBH_HID_DEVICE *hid_device);

VOID USBH_HID_Task (UNSIGNED    argc,
                    VOID        *argv);

STATUS USBH_HID_Get_HID_Desc  ( NU_USBH_HID         *cb,
                                NU_USB_ALT_SETTG    *alt_stg,
                                USBH_HID_DEVICE     *hidDev);

VOID USBH_HID_Intr_Complete (NU_USB_PIPE    *pipe,
                             NU_USB_IRP     *irp);

STATUS USBH_HID_Get_Report_Desc(NU_USBH_HID     *cb,
                                USBH_HID_DEVICE *hidDev);

VOID USBH_HID_Cntrl_Complete (  NU_USB_PIPE     *pipe,
                                NU_USB_IRP      *irp);

STATUS  USBH_HID_Decode_Report_Desc (USBH_HID_DEVICE *hid_device ,
                                     UINT8           *report_dscr,
                                     UINT32          report_dscr_length);

STATUS USBH_HID_Set_Report_Protocol(NU_USBH_HID *cb,
                                    USBH_HID_DEVICE *hidDev);

STATUS USBH_HID_Delete (VOID *cb);

STATUS Usbh_HID_Init_Item_St_Table (
                        USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                        USBH_HID_DEVICE             *hid_device);

UINT8 *USBH_HID_Extract_Item_Prefix(
                          UINT8                 *raw_report_dscr,
                          UINT32                *report_dscr_length,
                          USBH_HID_ITEM_PREFIX  *item_prefix);

STATUS USBH_HID_Decode_Global_Item(
                            USBH_HID_ITEM_STATE_TABLE *item_state_table,
                            USBH_HID_ITEM_PREFIX *item_prefix );

STATUS USBH_HID_Decode_Local_Item(
                    USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                    USBH_HID_ITEM_PREFIX        *item_prefix);

STATUS USBH_HID_Decode_Main_Item(
                    USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                    USBH_HID_ITEM_PREFIX        *item_prefix);

STATUS USBH_HID_Hnd_Main_Non_Data_Item(
                    USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                    UINT32                      main_data,
                    UINT8                       begin_end_col);

STATUS USBH_HID_Handle_Main_Data_Item (
                        USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                        UINT32                      main_data,
                        UINT8                       report_type);

USBH_HID_DEVICE* USBH_HID_Find_Device_For_IN_IRP(NU_USBH_HID    *cb);

/* ==================================================================== */

#include "connectivity/nu_usbh_hid_dat.h"

#ifdef          __cplusplus
}                                           /* End of C declarations.  */
#endif

/* ==================================================================== */

#endif /* _NU_USBH_HID_IMP_H_ */

/* ======================  End Of File  =============================== */

