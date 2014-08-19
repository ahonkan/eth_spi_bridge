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
 * FILE NAME
 *
 *      nu_usb_strm_imp.h 
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *
 *      This file describes control block and related definitions for 
 *      BULK streaming support for USB 3.0 specifications.
 *
 * 
 * DATA STRUCTURES
 *
 *      NU_USB_STREAM                       Control block for USB bulk 
 *                                          stream.
 *      NU_USB_STREAM_GRP                   Control block for USB bulk
 *                                          stream group.
 * 
 * FUNCTIONS 
 *
 *      None 
 * 
 * DEPENDENCIES 
 *
 *      None
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USB_STRM_IMP_H_
#define _NU_USB_STRM_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/* =================== Data Structures ===============================  */

/* Stream state definitions. */
#define STREAM_FREE     0
#define STREAM_BUSY     1

/* USB stream control block. */
typedef struct nu_usb_stream
{
    NU_USB_IRP  irp;
    UINT16      stream_id;
    BOOLEAN     state;
}NU_USB_STREAM;

/* Control block for USB stream group. */
typedef struct nu_usb_stream_grp
{
    NU_USB_STREAM   *streams;
    UINT16          num_streams;
    NU_SEMAPHORE    stream_grp_lock;
}NU_USB_STREAM_GRP;

/* ===================================================================  */
#endif /* #if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* _NU_USB_STRM_IMP_H_ */
/* ====================== end of file ================================  */

