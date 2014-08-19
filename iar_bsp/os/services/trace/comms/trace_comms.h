/***********************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains internal data structures and definitions of
*       TRACE communication.
*
***********************************************************************/

#ifndef TRACE_COMMS_H_
#define TRACE_COMMS_H_

#ifdef          __cplusplus
extern          "C" {   /* C declarations in C++     */
#endif

#include    <stdio.h>
/********************************/
/* Trace communications enabled */
/********************************/
#define     DBG_INTERFACE                   0
#define     SERIAL_INTERFACE                1
#define     ETHERNET_INTERFACE              2
#define     FILE_INTERFACE                  3
#define     USB_INTERFACE                   4

#define     NU_TRACE_COMMS_HEADER           0xEF56A55A
#define     NU_TRACE_COMMS_HEADER_SIZE      4
#define     NU_TRACE_COMMS_HEADER_CHKSUM    (0xEF + 0x56 + 0xA5 + 0x5A)
#define     NU_TRACE_COMMS_CKSUM_SIZE       4

#ifdef CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE

#define     TRACE_COMMS_STACK_TIME_SLICE    0

#define     NU_TRACE_COMMS_OPEN_ERROR       (NU_TRACE_ERROR_CODE_DELIMITER - 0)
#define     NU_TRACE_COMMS_CLOSE_ERROR      (NU_TRACE_ERROR_CODE_DELIMITER - 1)
#define     NU_TRACE_COMMS_TX_ERROR         (NU_TRACE_COMMS_OPEN_ERROR - 2)

STATUS      NU_Trace_Comms_Open(VOID);
STATUS      NU_Trace_Comms_Transmit(CHAR* buff, UINT16 size);
STATUS      NU_Trace_Comms_Close(VOID);

/*********************************/
/* Serial Communications enabled */
/*********************************/

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == SERIAL_INTERFACE)

#define     TRACE_COMMS_STACK_SIZE          1024

STATUS      Serial_Trace_Comms_Open(VOID);
STATUS      Serial_Trace_Comms_Transmit(CHAR* buff, UINT16 size);
STATUS      Serial_Trace_Comms_Close(VOID);
BOOLEAN     Serial_Trace_Comms_Is_Ready(VOID);

#define     NU_Trace_Comms_Open             Serial_Trace_Comms_Open
#define     NU_Trace_Comms_Transmit         Serial_Trace_Comms_Transmit
#define     NU_Trace_Comms_Close            Serial_Trace_Comms_Close
#define     NU_Trace_Comms_Is_Ready         Serial_Trace_Comms_Is_Ready

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == SERIAL_INTERFACE) */



/***********************************/
/* Ethernet Communications enabled */
/***********************************/

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == ETHERNET_INTERFACE)

#define     TRACE_COMMS_STACK_SIZE          2048

STATUS      Ethernet_Trace_Comms_Open(VOID);
STATUS      Ethernet_Trace_Comms_Transmit(CHAR* buff, UINT16 size);
STATUS      Ethernet_Trace_Comms_Close(VOID);
BOOLEAN     Ethernet_Trace_Comms_Is_Ready(VOID);

#define     NU_Trace_Comms_Open             Ethernet_Trace_Comms_Open
#define     NU_Trace_Comms_Transmit         Ethernet_Trace_Comms_Transmit
#define     NU_Trace_Comms_Close            Ethernet_Trace_Comms_Close
#define     NU_Trace_Comms_Is_Ready         Ethernet_Trace_Comms_Is_Ready

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == ETHERNET_INTERFACE) */

/****************************************/
/* File IO based communications enabled */
/****************************************/

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE)

#define     TRACE_COMMS_UP_TSK_PRIORITY     2
#define     TRACE_COMMS_UP_TSK_STK_SZ       (NU_MIN_STACK_SIZE * 8)
#define     FLUSH_DEAMON_PRIORITY           CFG_NU_OS_SVCS_TRACE_CORE_COMMS_FLUSH_DEFAULT_PRIORITY
#define     FLUSH_DEAMON_TIME_SLICE         0
#define     FLUSH_DEAMON_STACK_SIZE         (NU_MIN_STACK_SIZE * 32)
#define     FLUSH_HISR_PRIORITY             0
#define     FLUSH_HISR_STACK_SIZE           (NU_MIN_STACK_SIZE * 8)
#define     FILE_BUFFER_FLUSH_THRESHOLD     ((CFG_NU_OS_SVCS_TRACE_CORE_TRACE_BUFFER_SIZE * \
                                             CFG_NU_OS_SVCS_TRACE_COMMS_BUFFER_FLUSH_THRESHOLD) / 100)
#define     FILE_ENABLE_LOGGING_WITH_FLUSH   NU_FALSE
#define     STR_VALUE(x)                     #x
#define     ENDIANESS                        STR_VALUE(endianess)

STATUS      FileIO_Trace_Comms_Open(VOID);
STATUS      FileIO_Trace_Comms_Close(VOID);
VOID        FileIO_Start_Trace_Buffer_Flush(VOID);
BOOLEAN     FileIO_Trace_Comms_Is_Ready(VOID);
STATUS      FileIO_Flush_Buffer_To_File(VOID);
STATUS      FileIO_Get_File_System_Info(CHAR ** buff);

#define     NU_Trace_Comms_Open             FileIO_Trace_Comms_Open
#define     NU_Trace_Comms_Close            FileIO_Trace_Comms_Close
#define     NU_Trace_Comms_Is_Ready         FileIO_Trace_Comms_Is_Ready
#define     NU_Trace_Buffer_Flush           FileIO_Flush_Buffer_To_File

extern char    *Get_Trace_Buffer( void );
extern void    Set_Read_Head( unsigned long read_head );
extern void    Get_Buffer_Read_Write_Head( unsigned long *read_head, unsigned long *write_head );
extern unsigned long Get_Used_Buffer_Space( void );
extern unsigned long Get_Buffer_Size( void );

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE) */

#endif /* CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE */

#ifdef          __cplusplus
}   /* End of C declarations */
#endif  /* __cplusplus */

#endif /* TRACE_COMMS_H_ */
