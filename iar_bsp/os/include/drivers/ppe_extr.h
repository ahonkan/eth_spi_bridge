/*************************************************************************
*
*               Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
****************************************************************************
* FILE NAME                                                 
*
*       ppe_extr.h                                        
*
* COMPONENT
*
*       PPE - PPPoE Driver Declarations
*
* DESCRIPTION
*
*       Function declarations for use with the PPPoE driver library.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef PPE_EXTR_H
#define PPE_EXTR_H

#include    "drivers/ppe_defs.h"


/* Function declarations for PPE.C */
STATUS              PPE_Create_Session(DV_DEVICE_ENTRY*, UINT16);
DV_DEVICE_ENTRY    *PPE_Find_Virtual_Device(UINT16, UINT16);
UINT16              PPE_Get_Unique_Id(UINT16);
STATUS              PPE_Initialize(DV_DEVICE_ENTRY*);
VOID                PPE_Input(UINT16);
STATUS              PPE_Process_PADT(PPE_FRAME*);
DV_DEVICE_ENTRY    *PPE_Reserve_Device(UINT32);
VOID                PPE_Reset_State(DV_DEVICE_ENTRY*, UINT32);
VOID                PPE_Return_Device(DV_DEVICE_ENTRY*);
STATUS              PPE_Send(DV_DEVICE_ENTRY*, UINT8, NET_BUFFER*);
STATUS              PPE_Session_Terminate(LINK_LAYER*, UINT8);
STATUS              PPE_Transmit(DV_DEVICE_ENTRY*, NET_BUFFER*);
STATUS              PPE_Output(UINT8, UINT8*, UINT16, 
                                DV_DEVICE_ENTRY*, NET_BUFFER*);


/* Function declarations for PPEA.C */
STATUS              PPEA_Process_PADI(PPE_FRAME*);
STATUS              PPEA_Process_PADR(PPE_FRAME*);
UINT32              PPEA_Accept(DV_DEVICE_ENTRY*);
STATUS              PPEA_Wait_For_Client(DV_DEVICE_ENTRY*);


/* Function declarations for PPEH.C */
NET_BUFFER         *PPEH_Build_PADI_Packet(DV_DEVICE_ENTRY*);
VOID                PPEH_Clean_RBuf(PPE_LAYER*);
STATUS              PPEH_Control_Timer(DV_DEVICE_ENTRY*, UINT32);
STATUS              PPEH_Discovery(CHAR*, DV_DEVICE_ENTRY*);
DV_DEVICE_ENTRY    *PPEH_Find_Client_Device(PPE_FRAME*);
STATUS              PPEH_Process_PADO(PPE_FRAME*);
STATUS              PPEH_Process_PADS(PPE_FRAME*);
STATUS              PPEH_Resend(DV_DEVICE_ENTRY*, NET_BUFFER*);
STATUS              PPEH_Send_PADI(DV_DEVICE_ENTRY*);
#if (NET_VERSION_COMP > NET_4_5)
VOID                PPEH_Timer_Expire(TQ_EVENT, UNSIGNED, UNSIGNED);
#else
STATUS              PPEH_Timer_Expire(DV_DEVICE_ENTRY*, UNSIGNED);
#endif
STATUS              PPEH_Validate_ACName(PPE_LAYER*, UINT8*, UINT16);


/* Function declarations for PPEU.C */
VOID                PPEU_Add_Header(NET_BUFFER*, UINT8, UINT16);
UINT16              PPEU_Add_Tag(NET_BUFFER*, UINT16, UINT16, UINT16, UINT8*);
VOID                PPEU_Adjust_Header(NET_BUFFER*, INT16);
UINT16              PPEU_Append_Tag(NET_BUFFER*, UINT16, UINT16, UINT8 HUGE*);
UINT16              PPEU_Get_Tag(NET_BUFFER*, PPE_TAG*);
NET_BUFFER         *PPEU_New_Buffer(DV_DEVICE_ENTRY*);
NET_BUFFER         *PPEU_NextBuffer(NET_BUFFER*);
VOID                PPEU_Adjust_Checksum(UINT8 HUGE *, UINT8 HUGE *, UINT32, UINT8 HUGE *,UINT32);
VOID                PPEU_Fix_TCP_MSS(NET_BUFFER *);


/* Function declarations for PPEC.C */
UINT16              PPEC_Decode_Cookie(PPE_TAG*);
VOID                PPEC_Encode_Cookie(UINT8*, UINT16, UINT8*);
STATUS              PPEC_VerifyCookie(PPE_TAG*);


/* Broadcast address defined in NET.C */
extern const UINT8  NET_Ether_Broadaddr[DADDLEN];

#if (NET_VERSION_COMP > NET_4_5)
extern TQ_EVENT     PPE_Event;
#else
#define PPP_Memory  &System_Memory
#endif

#endif


