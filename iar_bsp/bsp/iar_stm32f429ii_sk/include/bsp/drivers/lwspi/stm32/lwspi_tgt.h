/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       lwspi_tgt.h
*
* COMPONENT
*
*       Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the SPI hardware.
*
**************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     LWSPI_TGT_H
#define     LWSPI_TGT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* SPI target info structure. */
typedef struct _lwspi_tgt_struct
{
    VOID            *setup_func;
    VOID            *cleanup_func;
    UINT32          trans_delay;
    UINT32          cs_delay;

} LWSPI_TGT;

/* Offset addresses of SPI Module registers. */
#define     SPI_CR1                  0x00   /* Control register 1 */
#define     SPI_CR2                  0x04   /* Control register 2 */
#define     SPI_SR                   0x08   /* Status register */
#define     SPI_DR                   0x0C   /* Data register */
#define     SPI_CRCPR                0x10   /* CRC polynomial register */
#define     SPI_RXCRCR               0x14   /* RX CRC register */
#define     SPI_TXCRCR               0x18   /* TX CRC register */
#define     SPI_I2SCFGR              0x1C   /* SPI-I2S configuration register */
#define     SPI_I2SPR                0x20   /* SPI-I2S prescaler register */

/* Byte offset constants. */
#define     SPI_BYTE1_OFFSET         1
#define     SPI_BYTE2_OFFSET         2
#define     SPI_BYTE3_OFFSET         3

/* SPI_CR1 register defines. */
#define     SPI_CR1_BIDIMODE         0x8000 /* Bidirectional data mode enable */
#define     SPI_CR1_BIDIOE           0x4000 /* Output enable in bidirectional mode */
#define     SPI_CR1_CRCEN            0x2000 /* Hardware CRC calculation enable */
#define     SPI_CR1_CRCNEXT          0x1000 /* Transmit CRC next */
#define     SPI_CR1_DFF              0x0800 /* Data frame format */
#define     SPI_CR1_RXONLY           0x0400 /* Receive only */
#define     SPI_CR1_SSM              0x0200 /* Software slave management */
#define     SPI_CR1_SSI              0x0100 /* Internal slave select */
#define     SPI_CR1_LSBFIRST         0x0080 /* Frame format */
#define     SPI_CR1_SPE              0x0040 /* SPI enable */
#define     SPI_CR1_BR_MSK           0x0038 /* Baud rate control mask */
#define     SPI_CR1_BR_256           0x0038 /* fPCLK/256 */
#define     SPI_CR1_BR_128           0x0030 /* fPCLK/128 */
#define     SPI_CR1_BR_64            0x0028 /* fPCLK/64 */
#define     SPI_CR1_BR_32            0x0020 /* fPCLK/32 */
#define     SPI_CR1_BR_16            0x0018 /* fPCLK/16 */
#define     SPI_CR1_BR_8             0x0010 /* fPCLK/8 */
#define     SPI_CR1_BR_4             0x0008 /* fPCLK/4 */
#define     SPI_CR1_BR_2             0x0000 /* fPCLK/2 */
#define     SPI_CR1_MSTR             0x0004 /* Master selection */
#define     SPI_CR1_CPOL             0x0002 /* Clock polarity */
#define     SPI_CR1_CPHA             0x0001 /* Clock phase */

/* SPI_CR2 register defines. */
#define     SPI_CR2_TXEIE            0x0080 /* TX buffer enpty interrupt enable */
#define     SPI_CR2_RXNEIE           0x0040 /* RX buffer not enpty interrupt enable */
#define     SPI_CR2_ERRIE            0x0020 /* Error interrupt enable */
#define     SPI_CR2_FRF              0x0010 /* Frame format (0: Motorola, 1: TI) */
#define     SPI_CR2_SSOE             0x0004 /* SS output enable */
#define     SPI_CR2_TXDMAEN          0x0002 /* TX buffer DMA enable */
#define     SPI_CR2_RXDMAEN          0x0001 /* RX buffer DMA enable */
#define     SPI_CR2_SPIDIS           0x0000 /* Disable all interrupts */

/* SPI_SR register defines. */
#define     SPI_SR_TIFRFE            0x0100 /* TI frame format error  */
#define     SPI_SR_BSY               0x0080 /* Busy flag */
#define     SPI_SR_OVR               0x0040 /* Overrun flag */
#define     SPI_SR_MODF              0x0020 /* Mode fault */
#define     SPI_SR_CRCERR            0x0010 /* CRC error flag */
#define     SPI_SR_UDR               0x0008 /* Underrun flag */
#define     SPI_SR_CHSIDE            0x0004 /* Channel side */
#define     SPI_SR_TXE               0x0002 /* Transmit buffer empty */
#define     SPI_SR_RXNE              0x0001 /* Receive buffer not empty */
#define     SPI_SR_ERRORS            0x0178 /* Possible SPI errors */

/* SPI_DR register defines. */
#define     SPI_DR_MSK               0xFFFF /* Received data mask */

/* SPI_I2SCFGR register defines. */
#define     SPI_I2SCFGR_I2SMOD       0x0800 /* I2S mode selection */
#define     SPI_I2SCFGR_I2SE         0x0400 /* I2S enable */
#define     SPI_I2SCFGR_I2SCFG_RX_M  0x0300 /* Master receive */
#define     SPI_I2SCFGR_I2SCFG_TX_M  0x0200 /* Master transmit */
#define     SPI_I2SCFGR_I2SCFG_RX_S  0x0100 /* Slave receive */
#define     SPI_I2SCFGR_I2SCFG_TX_S  0x0000 /* Slave transmit */
#define     SPI_I2SCFGR_PCMSYNC      0x0080 /* PCM frame sync */
#define     SPI_I2SCFGR_I2SSTD_PCM   0x0030 /* PCM standard */
#define     SPI_I2SCFGR_I2SSTD_LSB   0x0020 /* LSB justified standard (right justified) */
#define     SPI_I2SCFGR_I2SSTD_MSB   0x0020 /* MSB justified standard (left justified) */
#define     SPI_I2SCFGR_I2SSTD_PLS   0x0000 /* I2S Philips standard */
#define     SPI_I2SCFGR_CKPOL        0x0008 /* Steady state clock polarity  */
#define     SPI_I2SCFGR_DATLEN_RSV   0x0006 /* Not allowed */
#define     SPI_I2SCFGR_DATLEN_32B   0x0004 /* 32-bit data length */
#define     SPI_I2SCFGR_DATLEN_24B   0x0002 /* 24-bit data length */
#define     SPI_I2SCFGR_DATLEN_16B   0x0000 /* 16-bit data length */
#define     SPI_I2SCFGR_DATLEN_MSK   0xFFF9 /* Data length mask */
#define     SPI_I2SCFGR_CHLEN        0x0001 /* Channel length (0:15-bit, 1:32-bit) */

/* SPI_I2SPR register defines. */
#define     SPI_I2SPR_MCKOE          0x0200 /* Master clock output enable */
#define     SPI_I2SPR_ODD            0x0100 /* Odd factor for the prescaler */
#define     SPI_I2SPR_I2SDIV_MSK     0x00FF /* I2S linear prescaler mask */

/* SPI clock polarity and phase mask. */
#define     SPI_CPOL_CPHA_MASK       0x0003

/* Error codes definitions. */
#define     LWSPI_UNSUPPORTED_BAUD_RATE     -4000
#define     LWSPI_UNSUPPORTED_TRANSFER_SIZE -4001
#define     LWSPI_UNSUPPORTED_SS_POLARITY   -4002
#define     LWSPI_UNSUPPORTED_BIT_ORDER     -4003
#define     LWSPI_UNSUPPORTED_POLARITY      -4004
#define     LWSPI_UNSUPPORTED_PHASE         -4005
#define     LWSPI_UNSUPPORTED_PROTOCOL      -4006
#define     LWSPI_UNSUPPORTED_DMA           -4007


/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
VOID    nu_bsp_drvr_lwspi_stm32_init(const CHAR*, INT);
VOID    LWSPI_TGT_Initialize(LWSPI_INSTANCE_HANDLE*);
VOID    LWSPI_TGT_Shutdown(LWSPI_INSTANCE_HANDLE*);
STATUS  LWSPI_TGT_Configure(VOID*, NU_SPI_DEVICE*);
STATUS  LWSPI_TGT_Read(VOID*, BOOLEAN, NU_SPI_IRP*);
STATUS  LWSPI_TGT_Write(VOID*, BOOLEAN, NU_SPI_IRP*);
STATUS  LWSPI_TGT_Write_Read(VOID*, BOOLEAN, NU_SPI_IRP*, NU_SPI_IRP*);
VOID    LWSPI_TGT_Device_Enable(LWSPI_INSTANCE_HANDLE* spi_inst_ptr);
VOID    LWSPI_TGT_Device_Disable(LWSPI_INSTANCE_HANDLE* spi_inst_ptr);

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
VOID    LWSPI_TGT_LISR(INT);
VOID    LWSPI_TGT_Intr_Enable(LWSPI_INSTANCE_HANDLE*);
VOID    LWSPI_TGT_Intr_Disable(LWSPI_INSTANCE_HANDLE*);
BOOLEAN LWSPI_TGT_ISR_Read(VOID*, NU_SPI_IRP*);
BOOLEAN LWSPI_TGT_ISR_Write(VOID*, NU_SPI_IRP*);
BOOLEAN LWSPI_TGT_ISR_Write_Read(VOID*, NU_SPI_IRP*, NU_SPI_IRP*);
#endif /* CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE */

STATUS  LWSPI_TGT_Set_Baud_Rate(LWSPI_INSTANCE_HANDLE*, UINT32);
BOOLEAN LWSPI_TGT_Check_Device_Busy(LWSPI_INSTANCE_HANDLE*);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* !LWSPI_TGT_H */

