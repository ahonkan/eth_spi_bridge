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
* FILE NAME 
*
*        nu_usb_irp_ext.h
*
* COMPONENT
*
*        Nucleus USB Device Software
*
* DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_IRP class.
*
*
* DATA STRUCTURES
*       None 
*
* FUNCTIONS
*
*       None 
*
* DEPENDENCIES 
*
*               nu_usb_irp_imp.h    IRP's Internal definitions
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_IRP_EXT_H
#define _NU_USB_IRP_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include        "connectivity/nu_usb_irp_imp.h"

/* ====================  Function Prototypes ========================== */

/* Create and Delete    */

STATUS NU_USB_IRP_Create (NU_USB_IRP * cb,
                          UINT32 length,
                          UINT8 *data,
                          BOOLEAN accept_short_packets,
                          BOOLEAN use_empty_packets,
                          NU_USB_IRP_CALLBACK callback,
                          VOID *context,
                          UINT32 interval);
STATUS NU_USB_IRP_Delete (NU_USB_IRP * cb);

STATUS NU_USB_IRP_Get_Length (NU_USB_IRP * cb,
                              UINT32 *length_out);
STATUS NU_USB_IRP_Set_Length (NU_USB_IRP * cb,
                              UINT32 length);

STATUS NU_USB_IRP_Get_Data (NU_USB_IRP * cb,
                            UINT8 **data_out);
STATUS NU_USB_IRP_Set_Data (NU_USB_IRP * cb,
                            UINT8 *data);

STATUS NU_USB_IRP_Get_Status (NU_USB_IRP * cb,
                              STATUS *status_out);
STATUS NU_USB_IRP_Set_Status (NU_USB_IRP * cb,
                              STATUS status);

STATUS NU_USB_IRP_Get_Actual_Length (NU_USB_IRP * cb,
                                     UINT32 *length_out);
STATUS NU_USB_IRP_Set_Actual_Length (NU_USB_IRP * cb,
                                     UINT32 length);

STATUS NU_USB_IRP_Get_Accept_Short_Packets (NU_USB_IRP * cb,
                                            BOOLEAN * accept_short_packets_out);
STATUS NU_USB_IRP_Set_Accept_Short_Pkt (NU_USB_IRP * cb,
                                        BOOLEAN accept_short_packets);

STATUS NU_USB_IRP_Get_Use_Empty_Pkt (NU_USB_IRP * cb,
                                     BOOLEAN * use_empty_packet_out);
STATUS NU_USB_IRP_Set_Use_Empty_Pkt (NU_USB_IRP * cb,
                                     BOOLEAN use_empty_packet);

STATUS NU_USB_IRP_Get_Callback (NU_USB_IRP * cb,
                                NU_USB_IRP_CALLBACK * callback_out);
STATUS NU_USB_IRP_Set_Callback (NU_USB_IRP * cb,
                                NU_USB_IRP_CALLBACK callback);

STATUS NU_USB_IRP_Get_Context (NU_USB_IRP * cb,
                               VOID **context_out);
STATUS NU_USB_IRP_Set_Context (NU_USB_IRP * cb,
                               VOID *context);

STATUS NU_USB_IRP_Get_Interval (NU_USB_IRP * cb,
                                UINT32 *interval_out);
STATUS NU_USB_IRP_Set_Interval (NU_USB_IRP * cb,
                                UINT32 interval);

STATUS NU_USB_IRP_Get_Pipe (NU_USB_IRP * cb,
                            NU_USB_PIPE ** pipe_out);
STATUS NU_USB_IRP_Set_Pipe (NU_USB_IRP * cb,
                            NU_USB_PIPE * pipe);
							
STATUS NU_USB_IRP_Set_Buffer_Type_Cachable (NU_USB_IRP * cb,
                            	   BOOLEAN buffer_type);						 
STATUS NU_USB_IRP_Get_Buffer_Type_Cachable (NU_USB_IRP * cb,
                            	   BOOLEAN *buffer_type);							
/* ==================================================================== */

#endif /* _NU_USB_IRP_EXT_H    */

/* =======================  End Of File  ============================== */

