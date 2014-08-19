/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       serial_tgt.h
*
*   COMPONENT
*
*       SMT32_USART                           - SMT32_USART controller driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the UART hardware
*
*************************************************************************/
#ifndef SERIAL_TGT_H
#define SERIAL_TGT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Standard Label */
/* Controller type */
#define SMT32_USART_1_LABEL {0xEA,0xF3,0x16,0xF4,0x51,0xE4,0x49,0x70,0x86,0xE0,0x0A,0xD5,0x75,0x68,0xC9,0xAD}

/*********************************/
/* MACROS                        */
/*********************************/

/* Open Modes */
#define SERIAL_OPEN_MODE                    0x1
#define POWER_OPEN_MODE                     0x2
#define UII_OPEN_MODE                       0x4


/***********************/
/* Serial Mode defines */
/***********************/

/* Defines for USART data bits, stop bits and parity */
#define DATA_BITS_8                         8UL
#define DATA_BITS_9                         9UL
#define STOP_BITS_1                         1UL
#define STOP_BITS_2                         2UL
#define PARITY_NONE                         0UL
#define PARITY_ODD                          1UL
#define PARITY_EVEN                         2UL

/***************************/
/* USART Register Offsets  */
/***************************/
#define STM32_USART_SR                      0x00
#define STM32_USART_DR                      0x04
#define STM32_USART_BRR                     0x08
#define STM32_USART_CR1                     0x0C
#define STM32_USART_CR2                     0x10
#define STM32_USART_CR3                     0x14
#define STM32_USART_GTPR                    0x18

/**************************************/
/* USART Status register bits         */
/**************************************/
#define STM32_USART_SR_PE                   0x001
#define STM32_USART_SR_FE                   0x002
#define STM32_USART_SR_NE                   0x004
#define STM32_USART_SR_ORE                  0x008
#define STM32_USART_SR_IDLE                 0x010
#define STM32_USART_SR_RXNE                 0x020
#define STM32_USART_SR_TC                   0x040
#define STM32_USART_SR_TXE                  0x080
#define STM32_USART_SR_LBD                  0x100
#define STM32_USART_SR_CTS                  0x200

/**************************************/
/* USART Baud-rate register bits      */
/**************************************/
#define STM32_USART_BRR_DIV_FRACTION_MASK   0x000F
#define STM32_USART_RSR_DIV_MANTISSA_MASK   0xFFF0

/**************************************/
/* USART Control register 1 bits     */
/**************************************/
#define STM32_USART_CR1_SBK                 0x0001
#define STM32_USART_CR1_RWU                 0x0002
#define STM32_USART_CR1_RE                  0x0004
#define STM32_USART_CR1_TE                  0x0008
#define STM32_USART_CR1_IDLEIE              0x0010
#define STM32_USART_CR1_RXNEIE              0x0020
#define STM32_USART_CR1_TCIE                0x0040
#define STM32_USART_CR1_TXEIE               0x0080
#define STM32_USART_CR1_PEIE                0x0100
#define STM32_USART_CR1_PS                  0x0200
#define STM32_USART_CR1_PCE                 0x0400
#define STM32_USART_CR1_WAKE                0x0800
#define STM32_USART_CR1_M                   0x1000
#define STM32_USART_CR1_UE                  0x2000
#define STM32_USART_CR1_M_DATA_8            0x0000
#define STM32_USART_CR1_M_DATA_9            0x1000
#define STM32_USART_CR1_PS_EVEN             0x0000
#define STM32_USART_CR1_PS_ODD              0x0200
#define STM32_USART_ALL_INTERRUPT_MASK     (STM32_USART_CR1_IDLEIE | \
                                            STM32_USART_CR1_RXNEIE | \
                                            STM32_USART_CR1_TCIE |   \
                                            STM32_USART_CR1_TXEIE |  \
                                            STM32_USART_CR1_PEIE)

/**************************************/
/* USART Control register 2 bits      */
/**************************************/
#define STM32_USART_CR2_ADD                 0x000F
#define STM32_USART_CR2_LBDL                0x0020
#define STM32_USART_CR2_LBDIE               0x0040
#define STM32_USART_CR2_LBCL                0x0100
#define STM32_USART_CR2_CPHA                0x0200
#define STM32_USART_CR2_CPOL                0x0400
#define STM32_USART_CR2_CLKEN               0x0800
#define STM32_USART_CR2_STOP                0x3000
#define STM32_USART_CR2_LINEN               0x4000
#define STM32_USART_CR2_STOP_1              0x0000
#define STM32_USART_CR2_STOP_0_5            0x1000
#define STM32_USART_CR2_STOP_2              0x2000
#define STM32_USART_CR2_STOP_1_5            0x3000

/**************************************/
/* USART Control register 3 bits      */
/**************************************/
#define STM32_USART_CR3_EIE                 0x001
#define STM32_USART_CR3_IREN                0x002
#define STM32_USART_CR3_IRLP                0x004
#define STM32_USART_CR3_HDSEL               0x008
#define STM32_USART_CR3_NACK                0x010
#define STM32_USART_CR3_SCEN                0x020
#define STM32_USART_CR3_DMAR                0x040
#define STM32_USART_CR3_DMAT                0x080
#define STM32_USART_CR3_RTSE                0x100
#define STM32_USART_CR3_CTSE                0x200
#define STM32_USART_CR3_CTSIE               0x400

/**************************************/
/* USART Guard time and prescalar     */
/* register bits                      */
/**************************************/
#define STM32_USART_GTPR_PSC                0x00FF
#define STM32_USART_GTPR_GT                 0xFF00

/**************************************/
/* Macros to Read/Write Data register */
/**************************************/
#define STM32_USART_READ(base_addr)         (ESAL_GE_MEM_READ16(base_addr + STM32_USART_DR))
#define STM32_USART_WRITE(base_addr, ch)    (ESAL_GE_MEM_WRITE16(base_addr + STM32_USART_DR, ch))

/*********************************/
/* TARGET SPECIFIC STRUCTURE     */
/*********************************/
typedef struct  _serial_tgt_struct
{
    VOID(*setup_func)(VOID);
    VOID(*cleanup_func)(VOID);

} SERIAL_TGT_HANDLE;

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
VOID nu_bsp_drvr_serial_stm32_usart_init(const CHAR * key, INT startorstop);

INT       Serial_Tgt_Ints_Pending (UINT32 base_addr);
INT       Serial_Tgt_Rx_Err_Get (UINT32 base_addr);
VOID      Serial_Tgt_Rx_Int_Clear (UINT32 base_addr);
VOID      Serial_Tgt_Tx_Int_Clear (UINT32 base_addr);
VOID      Serial_Tgt_Tx_Int_Done (SERIAL_INSTANCE_HANDLE *sess_handle);
VOID      Serial_Tgt_Tx_Int_Enable (SERIAL_INSTANCE_HANDLE *inst_handle);

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/
VOID    Serial_Tgt_Setup(SERIAL_INSTANCE_HANDLE *instance_handle, SERIAL_ATTR *attrs);
VOID    Serial_Tgt_Enable(SERIAL_INSTANCE_HANDLE *instance_handle);
VOID    Serial_Tgt_Rx_Int_Enable(SERIAL_INSTANCE_HANDLE *instance_handle);
VOID    Serial_Tgt_Disable(SERIAL_INSTANCE_HANDLE *instance_handle);
STATUS  Serial_Tgt_Baud_Rate_Set(SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate);
INT     Serial_Tgt_Tx_Busy(SERIAL_INSTANCE_HANDLE *instance_handle);
STATUS  Serial_Tgt_Read (VOID* sess_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read);
STATUS  Serial_Tgt_Write (VOID* sess_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written);
VOID    Serial_Tgt_LISR (INT vector);

#ifdef __cplusplus
}
#endif

#endif /* !SERIAL_TARGET_H */

