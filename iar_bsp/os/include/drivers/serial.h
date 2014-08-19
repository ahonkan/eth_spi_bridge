/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       serial.h
*
*   COMPONENT
*
*       SERIAL                              - Serial Middleware
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Serial Middleware Driver module.
*
*   DATA STRUCTURES
*
*       SERIAL_SESSION                      - Serial session structure
*
*   DEPENDENCIES
*
*       nucleus.h                           - PLUS
*       dv_extr.h                           - Device Manager
*       reg_api.h                           - Registry System
*
*************************************************************************/
#ifndef SERIAL_H
#define SERIAL_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*******************/
/* GUID            */
/*******************/
/* Standard GUID's for Serial Middleware */
#define SERIAL_LABEL   {0xe3,0xd4,0x4e,0xa7,0x71,0x37,0x46,0x67,0xb7,0xf9,0xe1,0x0f,0xe6,0x8b,0x54,0xf8}

/*******************/
/* IOCTL CMDs      */
/*******************/
#define SERIAL_COMP_CONFIG_CMD              1
#define SERIAL_COMP_GET_ATTR_CMD            2
#define SERIAL_COMP_SET_ATTR_CMD            3
#define SERIAL_DEV_SET_MW_CB                4
#define SERIAL_DEV_ENABLE                   5
#define SERIAL_COMP_IS_TXBSY_CMD            6
#define SERIAL_GET_MW_SETTINGS_PATH         7
#define SERIAL_COMP_GET_TX_MODE             8
#define SERIAL_COMP_GET_RX_MODE             9 
#define SERIAL_PWR_HIB_RESTORE              10

/* Ioctl command delimiter */
#define SERIAL_CLASS_CMD_DELIMITER          11

/*******************/
/* ERROR CODES     */
/*******************/
/* SERIAL error values */
#define NU_SERIAL_SUCCESS                 0
#define NU_SERIAL_ERROR                  -1
#define NU_SERIAL_INVALID_PARITY         -2
#define NU_SERIAL_INVALID_DATA_BITS      -3
#define NU_SERIAL_INVALID_STOP_BITS      -4
#define NU_SERIAL_INVALID_BAUD           -5
#define NU_SERIAL_INVALID_COM_PORT       -6
#define NU_SERIAL_INVALID_DATA_MODE      -7
#define NU_SERIAL_INVALID_COMM_MODE      -8
#define NU_SERIAL_UART_LIST_FULL         -9
#define NU_SERIAL_INVALID_TX_MODE        -10
#define NU_SERIAL_DEV_NOT_FOUND          -11
#define NU_SERIAL_SESSION_UNAVAILABLE    -12
#define NU_SERIAL_IOCTL_INVALID_LENGTH   -13
#define NU_SERIAL_IOCTL_INVALID_MODE     -14
#define NU_SERIAL_REGISTRY_ERROR         -15
#define NU_SERIAL_INVALID_SESSION        -16
#define NU_SERIAL_NO_CHAR_AVAIL          -17

/*******************/
/* OTHER DEFINES   */
/*******************/
/* Buffer status values */
#define NU_BUFFER_FULL                  1
#define NU_BUFFER_DATA                  2
#define NU_BUFFER_EMPTY                 3

/* Pending interrupt defines */
#define SD_NO_INTERRUPT                 0
#define SD_TX_INTERRUPT                 1
#define SD_RX_INTERRUPT                 2

/* RX error defines */
#define SD_RX_NO_ERROR                  0
#define SD_RX_OVERRUN_ERROR             1
#define SD_RX_PARITY_ERROR              2
#define SD_RX_FRAME_ERROR               3

/* Defines for using polled or interrupt TX/RX */
#define USE_IRQ                         0UL
#define USE_POLLED                      1UL

/* getchar End Of File return value */
#define NU_EOF                          (-1)

/* End Of File Character, ^C = 0x03 = End Of Text */
#define NU_SERIAL_EOF_CHAR              0x03

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

/* Size of serial HISR defines. */
#define SERIAL_HISR_PRIORITY            2
#define SERIAL_HISR_STK_SIZE            (NU_MIN_STACK_SIZE * 4)

/* There is 1 HISR activation for RX or 1 HISR activation for TX. 
   The 1st RX interrupt will cause the HISR to activate, 
   but the 2nd, 3rd, etc, will not. The HISR has to run before the logic 
   will allow another RX interrupt to cause a HISR activation. The same 
   is true is for TX.  So, since we can have 1 RX and 1 TX activation of 
   the HISR, we get 1+1=2.  This single ring buffer and HISR supports 
   every serial device available, we multiply this by the max number of devices. */
#define SERIAL_ISR_DATA_BUF_SIZE        (2 * CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED)

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/*******************/
/* DATA STRUCTS    */
/*******************/

/* Define data structure for management of a SERIAL port. */
typedef struct _serial_session_struct
{
    NU_SEMAPHORE            tx_semaphore;       /* Semaphore so that strings between threads do not get mixed up */

    /* Buffering */
    UINT32                  sd_buffer_size;
    UINT8                   *rx_buffer;
    UINT32                  rx_buffer_read;
    UINT32                  rx_buffer_write;
    volatile INT            rx_buffer_status;
    UINT8                   *tx_buffer;
    UINT32                  tx_buffer_read;
    UINT32                  tx_buffer_write;
    volatile INT            tx_buffer_status;
    
    /* Errors */
    UINT32                  parity_errors;
    UINT32                  frame_errors;
    UINT32                  overrun_errors;
    UINT32                  busy_errors;
    UINT32                  general_errors;
    
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    /* Middleware configuration */
    UNSIGNED                read_mode;                      /* NU_NO_SUSPEND or NU_SUSPEND or (1-4,294,967,293) */
    UNSIGNED                write_mode;                     /* NU_NO_SUSPEND or NU_SUSPEND or (1-4,294,967,293) */

    /* Event group to track buffer usage */
    /* RX Event set when buffer status changes from NU_BUFFER_EMPTY to NU_BUFFER_DATA */
    /* TX Event set when buffer status changes from NU_BUFFER_FULL to NU_BUFFER_DATA */
    NU_EVENT_GROUP          ser_buffer_event;               

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

    /* Device atttributes */
    UINT32                  tx_mode;                        /* Tx intr/polled mode */
    UINT32                  rx_mode;                        /* Rx intr/polled mode */
    UINT32                  baud_rate;
    UINT32                  data_bits;
    UINT32                  stop_bits;
    UINT32                  parity;
    UINT32                  flow_ctrl;

    /* Device Manager */
    DV_DEV_HANDLE           comp_dev_handle;                /* Serial device handle */

} SERIAL_SESSION;

/* Define data structure used for passing data from LISR to HISR */
typedef struct
{
    SERIAL_SESSION *       port;
    INT                    tx_or_rx;

} SERIAL_ISR_DATA;

/*******************/
/* External API    */
/*******************/
STATUS  nu_os_drvr_serial_init (const CHAR * key, INT startorstop);
STATUS  NU_Serial_Open (DV_DEV_LABEL* name, SERIAL_SESSION* *port);
STATUS  NU_Serial_Close (SERIAL_SESSION *port);
INT     NU_Serial_Getchar (SERIAL_SESSION *port);
INT     NU_Serial_Putchar (SERIAL_SESSION *port, INT c);
INT     NU_Serial_Puts (SERIAL_SESSION *port, const CHAR *s);
STATUS  NU_Serial_Set_Configuration (SERIAL_SESSION *port, UINT32 baud_rate,
                                     UINT32 tx_mode, UINT32 rx_mode, UINT32 data_bits,
                                     UINT32 stop_bits, UINT32 parity, UINT32 flow_ctrl);
STATUS  NU_Serial_Get_Configuration (SERIAL_SESSION *port, UINT32 *baud_rate,
                                     UINT32 *tx_mode, UINT32 *rx_mode, UINT32 *data_bits,
                                     UINT32 *stop_bits, UINT32 *parity, UINT32 *flow_ctrl);
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

STATUS  NU_Serial_Get_Read_Mode(SERIAL_SESSION *port, UNSIGNED* mode);
STATUS  NU_Serial_Set_Read_Mode(SERIAL_SESSION *port, UNSIGNED mode, UNSIGNED timeout);
STATUS  NU_Serial_Get_Write_Mode(SERIAL_SESSION *port, UNSIGNED* mode);
STATUS  NU_Serial_Set_Write_Mode(SERIAL_SESSION *port, UNSIGNED mode, UNSIGNED timeout);
VOID    Serial_Set_ISR_Data(SERIAL_SESSION *port, INT tx_or_rx);

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/************************************************/
/* Legacy API and Constants                     */
/************************************************/
INT     SDC_Put_String (CHAR* str, SERIAL_SESSION* port);
INT     SDC_Put_Stringn (CHAR* str, SERIAL_SESSION* port, UNSIGNED count);
STATUS  SDC_Data_Ready (SERIAL_SESSION* port);
INT     SDC_Put_Char (UINT8 ch, SERIAL_SESSION* port, BOOLEAN caller_putstring);
INT     SDC_Get_Char (SERIAL_SESSION* port);
VOID    SDC_Reset (SERIAL_SESSION* port);
#define NU_SD_Put_Char      SDC_Put_Char
#define NU_SD_Put_String    SDC_Put_String
#define NU_SD_Put_Stringn   SDC_Put_Stringn
#define NU_SD_Data_Ready    SDC_Data_Ready

#define NU_SERIAL_PORT      SERIAL_SESSION
 
/****************************************************************************
 Note: All definitions are generic and should not require any changes
 ****************************************************************************/

/* SIO GUID */
#define     STDIO_LABEL   {0x1b,0x21,0x75,0x17,0x30,0x5f,0x42,0x1c,0x9c,0xc0,0x2e,0x2d,0x0e,0x7d,0x3c,0xfe}

/* Define externally available functions - K&R style serial IO */
STATUS          NU_SIO_Start        (INT startorstop);
INT             NU_SIO_Getchar      (VOID);
INT             NU_SIO_Putchar      (INT c);
INT             NU_SIO_Puts         (const CHAR* s);
NU_SERIAL_PORT* NU_SIO_Get_Port     (VOID);
STATUS          NU_Serial_Write     (SERIAL_SESSION *port, CHAR* buff, UINT16 size, UINT16* bytes_written);
#define         NU_SIO_Data_Ready() NU_SD_Data_Ready(NU_SIO_Get_Port())

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !SERIAL_H */
