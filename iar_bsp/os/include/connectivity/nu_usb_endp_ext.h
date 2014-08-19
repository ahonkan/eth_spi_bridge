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
************************************************************************/

/************************************************************************ 
* 
*    FILE NAME 
*
*        nu_usb_endp_ext.h
* 
*    COMPONENT
*
*        Nucleus USB Software 
* 
*    DESCRIPTION 
*
*        This file contains the declaration for external interfaces exposed
*        by Nucleus USB software's Alternate setting component.
*
*    DATA STRUCTURES
*
*        None
* 
*    FUNCTIONS 
*
*        None 
* 
*    DEPENDENCIES
*
*        nu_usb_endp_imp.h              Endpoint's Internal definitions
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ENDP_EXT_H_
#define _NU_USB_ENDP_EXT_H_

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_endp_imp.h"

/* ====================  Function Prototypes ========================== */
STATUS NU_USB_ENDP_Get_Desc (NU_USB_ENDP * cb,
                             NU_USB_ENDP_DESC ** endp_desc_out);

/* Services for the endp */
STATUS NU_USB_ENDP_Get_Number (NU_USB_ENDP * cb,
                               UINT8 *number_out);

STATUS NU_USB_ENDP_Get_Sync_Type (NU_USB_ENDP * cb,
                                  UINT8 *sync_type_out);

STATUS NU_USB_ENDP_Get_Usage_Type (NU_USB_ENDP * cb,
                                   UINT8 *usage_type_out);

STATUS NU_USB_ENDP_Get_Max_Packet_Size (NU_USB_ENDP * cb,
                                        UINT16 *max_packet_size_out);

STATUS NU_USB_ENDP_Get_Num_Transactions (NU_USB_ENDP * cb,
                                         UINT8 *num_transactions_out);

STATUS NU_USB_ENDP_Get_Class_Desc (NU_USB_ENDP * cb,
                                   UINT8 **class_desc_out,
                                   UINT32 *length_out);

STATUS NU_USB_ENDP_Get_Interval (NU_USB_ENDP * cb,
                                 UINT8 *interval_out);

STATUS NU_USB_ENDP_Get_Direction (NU_USB_ENDP * cb,
                                  UINT8 *direction_out);

STATUS NU_USB_ENDP_Get_Transfer_Type (NU_USB_ENDP * cb,
                                      UINT8 *type_out);

STATUS NU_USB_ENDP_Get_Status (NU_USB_ENDP * cb,
                               UINT16 *status_out);

/* Services to access associated objects */
STATUS NU_USB_ENDP_Get_Pipe (NU_USB_ENDP * cb,
                             NU_USB_PIPE ** pipe_out);

STATUS NU_USB_ENDP_Get_Alt_Settg (NU_USB_ENDP * cb,
                                  NU_USB_ALT_SETTG ** alt_setting_out);

STATUS NU_USB_ENDP_Get_Device (NU_USB_ENDP * cb,
                               NU_USB_DEVICE ** device_out);

/* SuperSpeed endpoint companion descriptor is defined in USB 3.0. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USB_ENDP_Get_Companion_Desc (
                    NU_USB_ENDP                   *cb,
                    NU_USB_SSEPCOMPANION_DESC    **epcompanion_desc_out);


STATUS NU_USB_ENDP_Get_MaxBurst (NU_USB_ENDP    *cb,
                                 UINT8          *maxburst_out);

STATUS NU_USB_ENDP_Get_Bulk_MaxStreams (NU_USB_ENDP     *cb,
                                        UINT32          *maxstreams_out);

STATUS NU_USB_ENDP_Get_Iso_MaxPktPerIntrvl (NU_USB_ENDP *cb,
                                            UINT8       *max_packets_out);

STATUS NU_USB_ENDP_Get_BytesPerInterval (NU_USB_ENDP *cb,
                                            UINT16   *bytesperinterval_out);

STATUS NU_USB_ENDP_Get_SSEPC_bmAttributes (NU_USB_ENDP *cb,
                                         UINT8   *bmAttributes_out);
                                         
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ==================================================================== */

#endif /* _NU_USB_ENDP_EXT_H_ */

/* ======================  End Of File  =============================== */

