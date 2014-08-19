/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
*    FILE NAME 
*
*        nu_usb_device_imp.h 
*
*    COMPONENT 
*
*        Nucleus USB Base
*
*    DESCRIPTION 
*
*        This file contains the control block and other data structures
*        for the NU_USB_DEVICE class.
*
*    DATA STRUCTURES 
*
*        _nu_usb_device      Device Control Block
*
*    FUNCTIONS
*
*        None 
*
*    DEPENDENCIES
*
*        None
* 
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_DEVICE_IMP_H
#define _NU_USB_DEVICE_IMP_H
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ====================  Data Structures ============================== */
struct _nu_usb_device
{
    /* Linked list for maintaining devices */
    CS_NODE node;

    /* USB device descriptor */
    NU_USB_DEVICE_DESC device_descriptor;

    /* Device Qualifier descriptor  */
    NU_USB_DEV_QUAL_DESC device_qualifier;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    /* Device power attributes */
    NU_USB_DEV_PW_ATTRIB pw_attrib;

    /* Variable for holding raw Binary Device Object Store descriptor. */
    UINT8 *raw_bos_descriptors;
                                  
    /* BOS descriptors will be saved in this variable after parsing. */
    NU_USB_BOS *bos;

    /* Holds current state of a link. Used in Power Management. */
    UINT8 link_state;
	/* Holds route string for the device -  used for routing packet to
	*  USB3.0 device.
	*/
    UINT32 route_string;
#endif

    /* array of descriptors fetched from the device. Each index represents a specific speed 
	  * configuration. Following is the detail.
	  * 	0:	USB Speed Unknown, should be set to NU_NULL.
	  * 	1:	USB Speed Low, should be set NU_NULL if not used.
	  * 	2:	USB Speed Full, should be set NU_NULL if not used.
	  * 	3:	USB Speed High, should be set NU_NULL if not used.
	  * 	4:	USB Speed Super, should be set NU_NULL if not used.
	  */
    UINT8 *raw_descriptors[NU_USB_MAX_CONFIGURATIONS][NU_USB_MAX_SPEEDS];

    /* array of configuration descriptor pointers */
    NU_USB_CFG *config_descriptors[NU_USB_MAX_CONFIGURATIONS];

    /* number of string descriptors */
    UINT8 num_string_descriptors;

    /* array of String descriptors  */
    NU_USB_STRING *string_descriptors[NU_USB_MAX_STRINGS];

    /* Active configuration of the device */
    UINT8 active_cnfg_num;

    /* USB speed of the device */
    UINT8 speed;

    /* Class driver managing this device */
    NU_USB_DRVR *driver;

    /* Control pipe for this device */
    NU_USB_PIPE ctrl_pipe;
    NU_USB_DEVICE *parent;

    /* port # on the parent port */
    UINT8 port_number;

    /* Device's USB function address */
    UINT8 function_address;

    /* The associated stack */
    NU_USB_STACK *stack;

    /* The associated controller */
    NU_USB_HW *hw;

    /* OTG support status */
    /* This contains the following information in the 8 bits:
     * 
     *   Bit b0 indicates SRP support by the device.
     *   Bit b1 indicates HNP support by the device.
     *   Bits b2, b3 are reserved.
     *   Bit b4 indicates if SRP is enabled on the device.
     *   Bit b5 indicates if HNP is enabled on the device.
     *   Bit b6 indicates if HNP is supported on the host(a_hnp_support).
     *   Bit b7 indicates if HNP is supported on some other port on the
     *   host (a_alt_hnp_support).
     *   bit 6 & 7 only makes sense for Function stack. Bit 5 takes
     *   precedence over bit 6 & 7. 
     */ 
    UINT8   otg_status;

    /* for exclusive access to the device */
    NU_SEMAPHORE lock;

    /* device states in Table 9-1 of USB specification */
    UINT32       state;

	/* Amount of current which this device will draw. */
	UINT32       current_drawn;
};

/* ====================  Function Prototypes ========================== */
STATUS usb_string_desc_2_ascii (NU_USB_STRING * string_desc,
                                CHAR * string);

/* ==================================================================== */

#endif /* _NU_USB_DEVICE_IMP_H_ */

/* ======================  End Of File  =============================== */

