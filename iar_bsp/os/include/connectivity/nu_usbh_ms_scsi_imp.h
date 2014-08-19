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
*       nu_usbh_ms_scsi_imp.h
*
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver
*       Wrapper for SCSI subclass.
*
* DESCRIPTION
*
*       This file contains the definitions for intrfaces
*       exported by SCSI wrapper.
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

#ifndef     _NU_USBH_MS_SCSI_IMP_H_
#define     _NU_USBH_MS_SCSI_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ================================= Define ============================ */

/*---- SCSI command codes ----*/

 /* Common Command set. */
#define  NU_SCSI_CMD_CHN_DEF          0x40  /* CHANGE DEFINITION.        */
#define  NU_SCSI_CMD_COMPARE          0x39  /* COMPARE.                  */
#define  NU_SCSI_CMD_CPY              0x18  /* COPY.                     */
#define  NU_SCSI_CMD_CPY_VFY          0x3A  /* COPY AND VERIFY.          */
#define  NU_SCSI_CMD_ERASE10          0x2C  /* ERASE(10).                */
#define  NU_SCSI_CMD_ERASE12          0xAC  /* ERASE(12).                */
#define  NU_SCSI_CMD_FORMAT_UNIT      0x04  /* FORMAT UNIT.              */
#define  NU_SCSI_CMD_INQUIRY          0x12  /* INQUIRY.                  */
#define  NU_SCSI_CMD_LCK_CAC          0x36  /* LOCK-UNLOCK CACHE.        */
#define  NU_SCSI_CMD_LOG_SEL          0x4C  /* LOG SELECT.               */
#define  NU_SCSI_CMD_LOG_SNS          0x4D  /* LOG SENSE.                */
#define  NU_SCSI_CMD_MED_SCN          0x38  /* MEDIUM SCAN.              */
#define  NU_SCSI_CMD_MD_SEL6          0x15  /* MODE SELECT(6).           */
#define  NU_SCSI_CMD_MD_SEL10         0x55  /* MODE SELECT(10).          */
#define  NU_SCSI_CMD_MD_SNS6          0x1A  /* MODE SENSE(6).            */
#define  NU_SCSI_CMD_MD_SNS10         0x5A  /* MODE SENSE(10).           */
#define  NU_SCSI_CMD_MV_MED           0xA5  /* MOVE MEDIUM.              */
#define  NU_SCSI_CMD_MV_MED_ATA       0xA7  /* MOVE MEDIUM ATTACHED.     */
#define  NU_SCSI_CMD_PER_RSV_IN       0x5E  /* PERSISTENT RESERVE IN.    */
#define  NU_SCSI_CMD_PER_RSV_OUT      0x5F  /* PERSISTENT RESERVE OUT.   */
#define  NU_SCSI_CMD_PRE_FETCH        0x34  /* PRE-FETCH.                */
/* PREVENT-ALLOW MEDIUM REMOVAL. */
#define  NU_SCSI_CMD_PRE_ALL_MED_RMV  0x1E
#define  NU_SCSI_CMD_READ6            0x08  /* READ(6).                  */
#define  NU_SCSI_CMD_READ10           0x28  /* READ(10).                 */
#define  NU_SCSI_CMD_READ12           0xA8  /* READ(12).                 */
#define  NU_SCSI_CMD_RD_BUF           0x3C  /* READ BUFFER.              */
#define  NU_SCSI_CMD_RD_CAP           0x25  /* READ CAPACITY.            */
#define  NU_SCSI_CMD_RD_DEF_D10       0x37  /* READ DEFECT DATA(10).     */
#define  NU_SCSI_CMD_RD_DEF_D12       0xB7  /* READ DEFECT DATA(12).     */
/* READ ELEMENT STATUS ATTACHED. */
#define  NU_SCSI_CMD_RD_ELE_STS_ATA   0xB4
#define  NU_SCSI_CMD_RD_ELE_STS       0xB8  /* READ ELEMENT STATUS.      */
#define  NU_SCSI_CMD_RD_GEN           0x29  /* READ GENERATION.          */
#define  NU_SCSI_CMD_RD_LNG           0x3E  /* READ LONG.                */
#define  NU_SCSI_CMD_RASN_BLK         0x07  /* REASSIGN BLOCKS.          */
#define  NU_SCSI_CMD_REBUILD          0x81  /* REBUILD.                  */
#define  NU_SCSI_CMD_RCV_DGN_RLT      0x1C  /* RECEIVE DIAGNOSTIC RESULT */
#define  NU_SCSI_CMD_REGENERATE       0x82  /* REGENERATE.               */
#define  NU_SCSI_CMD_RELEASE6         0x17  /* RELEASE(6).               */
#define  NU_SCSI_CMD_RELEASE10        0x57  /* RELEASE(10).              */
#define  NU_SCSI_CMD_RPT_LUN          0xA0  /* REPORT LUNS.              */
#define  NU_SCSI_CMD_REQ_SNS          0x03  /* REQUEST SENSE.            */
#define  NU_SCSI_CMD_RESERVE6         0x16  /* RESERVE(6).               */
#define  NU_SCSI_CMD_RESERVE10        0x56  /* RESERVE(10).              */
#define  NU_SCSI_CMD_SEEK6            0x0B  /* SEEK(6).                  */
#define  NU_SCSI_CMD_SEEK10           0x2B  /* SEEK(10).                 */
#define  NU_SCSI_CMD_SND_DGN          0x1D  /* SEND DIAGNOSTIC.          */
#define  NU_SCSI_CMD_SET_LMT10        0x33  /* SET LIMITS(10).           */
#define  NU_SCSI_CMD_SET_LMT12        0xB3  /* SET LIMITS(16).           */
#define  NU_SCSI_CMD_STR_STP_UNT      0x1B  /* START STOP UNIT.          */
#define  NU_SCSI_CMD_SYN_CAC          0x35  /* SYNCHRONIZE CACHE.        */
#define  NU_SCSI_CMD_TST_UNT_RDY      0x00  /* TEST UNIT READY.          */
#define  NU_SCSI_CMD_UPD_BLK          0x3D  /* UPDATE BLOCK .            */
#define  NU_SCSI_CMD_VERIFY10         0x2F  /* VERIFY(10).               */
#define  NU_SCSI_CMD_VERIFY12         0xAF  /* VERIFY(12).               */
#define  NU_SCSI_CMD_WRITE6           0x0A  /* WRITE(6).                 */
#define  NU_SCSI_CMD_WRITE10          0x2A  /* WRITE(10).                */
#define  NU_SCSI_CMD_WRITE12          0xAA  /* WRITE(12).                */
#define  NU_SCSI_CMD_WR_VFY10         0x2E  /* WRITE AND VERIFY(10).     */
#define  NU_SCSI_CMD_WR_VFY12         0xAE  /* WRITE AND VERIFY(12).     */
#define  NU_SCSI_CMD_WR_BUF           0x3B  /* WRITE BUFFER.             */
#define  NU_SCSI_CMD_WR_LNG           0x3F  /* WRITE LONG.               */
#define  NU_SCSI_CMD_WR_SAME          0x41  /* WRITE SAME.               */
#define  NU_SCSI_CMD_XDREAD           0x52  /* XDREAD.                   */
#define  NU_SCSI_CMD_XDWRITE          0x50  /* XDWRITE.                  */
#define  NU_SCSI_CMD_XDWRITE_EX       0x80  /* XDWRITE EXTENDED.         */
#define  NU_SCSI_CMD_XPWRITE          0x51  /* XPWRITE.                  */
#define  NU_SCSI_CMD_RD_FMT_CAP       0x23  /* READ FORMAT CAPACITIES.   */

/*---- SCSI command packet length ----*/

/* Max. size of packet length. */
#define  NU_SCSI_CML_MAX              0x0C
#define  NU_SCSI_CML_READ10           0x0A
#define  NU_SCSI_CML_WRITE10          0x0A
#define  NU_SCSI_CML_INQUIRY          0x06
#define  NU_SCSI_CML_TST_UNT_RDY      0x06
#define  NU_SCSI_CML_RD_CAP           0x0A
#define  NU_SCSI_CML_REQ_SNS          0x06

/*---- SCSI command data length ----*/
#define  NU_SCSI_DL_RD_CAP            0x08

/* ====================  Macro Function  =============================== */


STATUS  NU_USBH_MS_SCSI_Request(VOID *, VOID *, UINT32 , VOID *,          \
                                UINT32 , UINT8 );


/* ====================  Function Prototypes  ========================== */

STATUS      _NU_USBH_MS_SCSI_Delete(VOID *);
STATUS      NU_USBH_MS_SCSI_Inquiry(VOID *, UINT8 *, VOID *, UINT8);
STATUS      NU_USBH_MS_SCSI_Test_Unit_Ready(VOID *, UINT8 *);
STATUS      NU_USBH_MS_SCSI_Read_Capacity(VOID *, UINT8 *, UINT8 *);
STATUS      NU_USBH_MS_SCSI_Request_Sense(VOID *, UINT8 *, VOID *, UINT8);
STATUS      NU_USBH_MS_SCSI_Read10(VOID *, UINT8 *,
                                    UINT32, UINT32, VOID *, UINT32);
STATUS      NU_USBH_MS_SCSI_Write10(VOID *, UINT8 *, UINT32, UINT32,
                                    VOID *, UINT32);

/* ===================================================================== */

#endif      /* _NU_USBH_MS_SCSI_IMP_H_ */

/* ====================  End Of File  ================================== */
