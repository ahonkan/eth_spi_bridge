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
*       nu_usbh_ms_8020_imp.h
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver
*       Wrapper for SFF8020i subclass.
*
* DESCRIPTION
*
*       This file contains the definitions for intrfaces
*       exported by SFF8020i wrapper.
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

#ifndef     _NU_USBH_MS_8020_IMP_H_
#define     _NU_USBH_MS_8020_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif



/* ============================= Define ================================ */

/*---- 8020 command codes ----*/

/* Common Command set. */
#define  NU_8020_CMD_INQUIRY         0x12   /* INQUIRY.                  */
#define  NU_8020_CMD_LD_ULD          0xA6   /* LOAD/UNLOAD CD.           */
#define  NU_8020_CMD_MEC_STS         0xBD   /* MECHANISM STATUS.         */
#define  NU_8020_CMD_MD_SEL10        0x55   /* MODE SELECT(10).          */
#define  NU_8020_CMD_MD_SNS10        0x5A   /* MODE SENSE(10).           */
#define  NU_8020_CMD_PAU_RSM         0x4B   /* PAUSE/RESUME.             */
#define  NU_8020_CMD_PLY_AUD10       0x45   /* PLAY AUDIO(10).           */
#define  NU_8020_CMD_PLY_AUD_MSF     0x47   /* PLAY AUDIO MSF.           */
#define  NU_8020_CMD_PLY_CD          0xBC   /* PLAY CD.                  */
/* PREVENT-ALLOW MEDIUM REMOVAL.*/
#define  NU_8020_CMD_PRE_ALL_MED_RMV 0x1E
#define  NU_8020_CMD_READ10          0x28   /* READ(10).                 */
#define  NU_8020_CMD_READ12          0xA8   /* READ(12).                 */
#define  NU_8020_CMD_RD_CAP          0x25   /* READ CAPACITY.            */
#define  NU_8020_CMD_RD_CD           0xBE   /* READ CD.                  */
#define  NU_8020_CMD_RD_CD_MSF       0xB9   /* READ CD MSF.              */
#define  NU_8020_CMD_RD_HDR          0x44   /* READ HEADER.              */
#define  NU_8020_CMD_RD_SUB_CHN      0x42   /* READ SUB-CHANNEL.         */
#define  NU_8020_CMD_RD_TOC          0x43   /* READ TOC.                 */
#define  NU_8020_CMD_REQ_SNS         0x03   /* REQUEST SENSE.            */
#define  NU_8020_CMD_SCN             0xBA   /* SCAN.                     */
#define  NU_8020_CMD_SEEK10          0x2B   /* SEEK(10).                 */
#define  NU_8020_CMD_SET_CD_SPD      0xBB   /* SET CD SPEED.             */
#define  NU_8020_CMD_STP_PLY_SCN     0x4E   /* STOP PLAY/SCAN.           */
#define  NU_8020_CMD_STR_STP_UNT     0x1B   /* START STOP UNIT.          */
#define  NU_8020_CMD_TST_UNT_RDY     0x00   /* TEST UNIT READY.          */

#define  NU_8020_CMD_WRITE6          0x0A   /* WRITE(6).                 */
#define  NU_8020_CMD_WRITE10         0x2A   /* WRITE(10).                */
#define  NU_8020_CMD_WRITE12         0xAA   /* WRITE(12).                */

/*---- SFF-8020i command packet length ----*/

/* Max. size of packet length. */
#define  NU_8020_CML_MAX             0x0C
#define  NU_8020_CML_READ10          0x0C
#define  NU_8020_CML_INQUIRY         0x0C
#define  NU_8020_CML_TST_UNT_RDY     0x0C
#define  NU_8020_CML_RD_CAP          0x0C
#define  NU_8020_CML_REQ_SNS         0x0C
#define  NU_8020_CML_WRITE10         0x0C

/*---- SFF-8020i command data length ----*/
#define  NU_8020_DL_RD_CAP           0x08


/* ======================= Function Prototypes ========================= */

STATUS  NU_USBH_MS_8020_Inquiry(VOID *, UINT8 *, VOID *, UINT8 );
STATUS  NU_USBH_MS_8020_Test_Unit_Ready(VOID *, UINT8 *);
STATUS  NU_USBH_MS_8020_Read_Capacity(VOID *, UINT8 *, UINT8 *);
STATUS  NU_USBH_MS_8020_Request_Sense(VOID *, UINT8 *, VOID *, UINT8 );
STATUS  NU_USBH_MS_8020_Read10(VOID *, UINT8 *, UINT32, UINT32,
                                VOID *, UINT32 );
STATUS  NU_USBH_MS_8020_Write10(VOID *, UINT8 *, UINT32, UINT32,
                                VOID *, UINT32 );
STATUS  NU_USBH_MS_8020_Request(VOID *, VOID *, UINT32 , VOID *,
                                UINT32 , UINT8 );


/* ===================================================================== */

#endif      /* _NU_USBH_MS_8020_IMP_H_ */

/* ======================  End Of File  ================================ */
