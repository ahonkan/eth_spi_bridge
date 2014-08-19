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
*       nu_usb_strm_ext.h
*
* COMPONENT
*
*       Nucleus USB Software
*
* DESCRIPTION
*
*       This file contains the exported function names for operating 
*       BULK streaming.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS 
*       None 
*
* DEPENDENCIES 
*
*       nu_usb_strm_imp.h          Bulk streaming internal definitions.
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USB_STRM_EXT_H
#define _NU_USB_STRM_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  USB Include Files =================================  */

#include        "connectivity/nu_usb_strm_imp.h"

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/* ====================  Function Prototypes ========================== */

STATUS NU_USB_STRM_GRP_Create(NU_USB_STREAM_GRP *stream_grp,
                              UINT16            num_streams);

STATUS NU_USB_STRM_GRP_Delete(NU_USB_STREAM_GRP *stream_grp);

STATUS NU_USB_STRM_GRP_Acquire_Arbitrate(
                                        NU_USB_STREAM_GRP   *stream_grp,
                                        UINT16              *stream_id_out,
                                        NU_USB_STREAM       **stream_out);

STATUS NU_USB_STRM_GRP_Acquire_Forceful(
                                        NU_USB_STREAM_GRP   *stream_grp,
                                        UINT16              stream_id,
                                        NU_USB_STREAM       **stream_out);

STATUS NU_USB_STRM_GRP_Release_Strm(    
                                    NU_USB_STREAM_GRP   *stream_grp,
                                    UINT16              stream_id);

STATUS NU_USB_STRM_Create(NU_USB_STREAM       *cb,
                          UINT32              length,
                          UINT8               *data,
                          NU_USB_IRP_CALLBACK callback);

STATUS NU_USB_STRM_Set_State(NU_USB_STREAM  *cb,
                            UINT8           state);

STATUS NU_USB_STRM_Get_State(NU_USB_STREAM  *cb,
                            UINT8           *state_out);

STATUS NU_USB_STRM_Set_Stream_ID(NU_USB_STREAM  *cb,
                                UINT8           stream_id);

STATUS NU_USB_STRM_Get_Stream_ID(   NU_USB_STREAM   *cb,
                                    UINT8           *stream_id_out);

/* ==================================================================== */

#endif /* #if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* _NU_USB_STRM_EXT_H  */

/* ======================  End Of File  =============================== */

