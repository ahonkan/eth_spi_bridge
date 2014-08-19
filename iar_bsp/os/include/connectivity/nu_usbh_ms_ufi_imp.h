/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
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
*       nu_usbh_ms_ufi_imp.h
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver
*       Wrapper for UFI subclass.
*
* DESCRIPTION
*
*       This file contains the definitions for intrfaces
*       exported by UFI wrapper.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_UFI_IMP_H_
#define     _NU_USBH_MS_UFI_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* =============================== Define ============================== */
/*---- UFI command codes ----*/

/* Common Command set. */
#define     NU_UFI_CMD_FORMAT_UNIT     0x04 /* FORMAT UNIT.              */
#define     NU_UFI_CMD_INQUIRY         0x12 /* INQUIRY.                  */
#define     NU_UFI_CMD_STR_STP_UNT     0x1B /* START STOP UNIT.          */
#define     NU_UFI_CMD_MD_SEL10        0x55 /* MODE SELECT(10).          */
#define     NU_UFI_CMD_MD_SNS10        0x5A /* MODE SENSE(10).           */
/* PREVENT-ALLOW MEDIUM REMOVAL. */
#define     NU_UFI_CMD_PRE_ALL_MED_RMV 0x1E
#define     NU_UFI_CMD_READ10          0x28 /* READ(10).                 */
#define     NU_UFI_CMD_READ12          0xA8 /* READ(12).                 */
#define     NU_UFI_CMD_RD_CAP          0x25 /* READ CAPACITY.            */
#define     NU_UFI_CMD_RD_FMT_CAP      0x23 /* READ FORMAT CAPACITIES.   */
#define     NU_UFI_CMD_REQ_SNS         0x03 /* REQUEST SENSE.            */
#define     NU_UFI_CMD_REZERO_UNIT     0x01 /* REZERO UNIT.              */
#define     NU_UFI_CMD_SEEK10          0x2B /* SEEK(10).                 */
#define     NU_UFI_CMD_SND_DGN         0x1D /* SEND DIAGNOSTIC.          */
#define     NU_UFI_CMD_TST_UNT_RDY     0x00 /* TEST UNIT READY.          */
#define     NU_UFI_CMD_VERIFY10        0x2F /* VERIFY(10).               */
#define     NU_UFI_CMD_WRITE10         0x2A /* WRITE(10).                */
#define     NU_UFI_CMD_WRITE12         0xAA /* WRITE(12).                */
#define     NU_UFI_CMD_WR_VFY10        0x2E /* WRITE AND VERIFY(10).     */

/*---- UFI command packet length ----*/

/* Max. size of packet length. */
#define     NU_UFI_CML_MAX             0x0C
#define     NU_UFI_CML_FORMAT_UNIT     0x0C /* FORMAT UNIT.              */
#define     NU_UFI_CML_INQUIRY         0x0C /* INQUIRY.                  */
#define     NU_UFI_CML_STR_STP_UNT     0x0C /* START STOP UNIT.          */
#define     NU_UFI_CML_MD_SEL10        0x0C /* MODE SELECT(10).          */
#define     NU_UFI_CML_MD_SNS10        0x0C /* MODE SENSE(10).           */
/* PREVENT-ALLOW MEDIUM REMOVAL. */
#define     NU_UFI_CML_PRE_ALL_MED_RMV 0x0C
#define     NU_UFI_CML_READ10          0x0C /* READ(10).                 */
#define     NU_UFI_CML_READ12          0x0C /* READ(12).                 */
#define     NU_UFI_CML_RD_CAP          0x0C /* READ CAPACITY.            */
#define     NU_UFI_CML_RD_FMT_CAP      0x0C /* READ FORMAT CAPACITIES.   */
#define     NU_UFI_CML_REQ_SNS         0x0C /* REQUEST SENSE.            */
#define     NU_UFI_CML_REZERO_UNIT     0x0C /* REZERO UNIT.              */
#define     NU_UFI_CML_SEEK10          0x0C /* SEEK(10).                 */
#define     NU_UFI_CML_SND_DGN         0x0C /* SEND DIAGNOSTIC.          */
#define     NU_UFI_CML_TST_UNT_RDY     0x0C /* TEST UNIT READY.          */
#define     NU_UFI_CML_VERIFY10        0x0C /* VERIFY(10).               */
#define     NU_UFI_CML_WRITE10         0x0C /* WRITE(10).                */
#define     NU_UFI_CML_WRITE12         0x0C /* WRITE(12).                */
#define     NU_UFI_CML_WR_VFY10        0x0C /* WRITE AND VERIFY(10).     */

/*---- UFI command data length ----*/
#define     NU_UFI_DL_RD_CAP           0x08

/* ======================= Function Prototypes ========================= */

/* NU_USBH_MS_UFI user API. */
STATUS      NU_USBH_MS_UFI_Request(VOID *, VOID *, UINT32, VOID *,
                                    UINT32, UINT8 );

STATUS      NU_USBH_MS_UFI_Inquiry(VOID *, UINT8 *, VOID *, UINT8);
STATUS      NU_USBH_MS_UFI_Test_Unit_Ready(VOID *, UINT8 *);
STATUS      NU_USBH_MS_UFI_Read_Capacity(VOID *, UINT8 *, UINT8 *);
STATUS      NU_USBH_MS_UFI_Request_Sense(VOID *, UINT8 *, VOID *, UINT8);
STATUS      NU_USBH_MS_UFI_Read10(VOID *, UINT8 *, UINT32, UINT32, VOID *,
                                  UINT32);
STATUS      NU_USBH_MS_UFI_Write10(VOID *, UINT8 *, UINT32, UINT32,
                                    VOID *, UINT32);

/* ===================================================================== */

#endif      /* _NU_USBH_MS_UFI_IMP_H_ */

/* ======================  End Of File  ================================ */
