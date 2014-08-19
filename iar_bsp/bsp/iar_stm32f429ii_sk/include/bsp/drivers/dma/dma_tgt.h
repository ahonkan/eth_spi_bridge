/*
 * Avinash Honkan   Terabit Radios
 * 17 Jul 2014
 *
 * Modified code from Mentor FAE (Stuart) to fit the STM device.
 * Utilizing code from STM Firmware SDK.
 *
 */

/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       dma_tgt.h
*
*   COMPONENT
*
*       DMA Driver                          - EDMA3 device driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the EDMA3 Driver
*
*   DATA STRUCTURES
*
*       DMA_TGT_HANDLE                      - Structure containing device-
*                                             specific attributes
*************************************************************************/
#ifndef DMA_TGT_H
#define DMA_TGT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*
 * The header files stm32f4xx.h and stm32f4xx_hal_dma.h contain definitions
 * for DMA configuration and access.  No need to repeat hardware definitions 
 * in this
 * file
 */

/* Logical peripheral ID taken from STM Ref Manual 
 * RM0090 DOC ID 018909 Rev 6 DMA Chapter Pg 304, 305
 */

#define DMA_PERIPH_MASK   0x00FF        
#define DMA_STREAM_COUNT  16      /* 0-7 is DMA1, 8-15 is DMA2 */
#define DMA_CHANNEL_COUNT 10      /* Channel assignment of device. 
                                     There are 8 channels but some devices
                                     share channels.  */

#define DMA_EXTRACT_CHANNEL(x)    (x >>= 8)     /* Upper 8 bits is the dma channel */   

#define CHAN_MASK   0x0700

#define CHAN_0      0x0000
#define CHAN_1      0x0100
#define CHAN_2      0x0200
#define CHAN_3      0x0300
#define CHAN_4      0x0400
#define CHAN_5      0x0500
#define CHAN_6      0x0600
#define CHAN_7      0x0700

typedef enum
{        
    DMA_NONE = 0,        
        
    DMA_SPI1_TX,
    DMA_SPI1_RX,
    DMA_SPI2_TX,
    DMA_SPI2_RX,
    DMA_SPI3_TX,
    DMA_SPI3_RX,
    DMA_SPI4_TX,
    DMA_SPI4_RX,
    DMA_SPI5_TX,
    DMA_SPI5_RX,
    DMA_SPI6_TX,
    DMA_SPI6_RX,

    DMA_I2C1_TX,
    DMA_I2C1_RX,
    DMA_I2C2_TX,
    DMA_I2C2_RX,
    DMA_I2C3_TX,
    DMA_I2C3_RX,

    DMA_USART1_TX,
    DMA_USART1_RX,
    DMA_USART2_TX,
    DMA_USART2_RX,
    DMA_USART3_TX,
    DMA_USART3_RX,
    DMA_UART4_TX,
    DMA_UART4_RX,
    DMA_UART5_TX,
    DMA_UART5_RX,
    DMA_USART6_TX,
    DMA_USART6_RX,
    DMA_UART7_TX,
    DMA_UART7_RX,
    DMA_UART8_TX,
    DMA_UART8_RX,

    DMA_I2S2_EXT_TX,
    DMA_I2S2_EXT_RX,
    DMA_I2S3_EXT_TX,
    DMA_I2S3_EXT_RX,

    DMA_TIM1_CH1,
    DMA_TIM1_CH2,
    DMA_TIM1_CH3,
    DMA_TIM1_CH4 ,
    DMA_TIM1_COM,
    DMA_TIM1_TRIG,
    DMA_TIM1_UP,

    DMA_TIM2_CH1,
    DMA_TIM2_CH2 ,
    DMA_TIM2_CH3,
    DMA_TIM2_CH4,
    DMA_TIM2_UP ,

    DMA_TIM3_CH1 ,
    DMA_TIM3_CH2,
    DMA_TIM3_CH3,
    DMA_TIM3_CH4 ,
    DMA_TIM3_UP,
    DMA_TIM3_TRIG,

    DMA_TIM4_CH1,
    DMA_TIM4_CH2,
    DMA_TIM4_CH3,
    DMA_TIM4_UP,

    DMA_TIM5_CH1,
    DMA_TIM5_CH2,
    DMA_TIM5_CH3 ,
    DMA_TIM5_CH4 ,
    DMA_TIM5_UP,
    DMA_TIM5_TRIG,

    DMA_TIM6_UP,

    DMA_TIM7_UP,

    DMA_TIM8_CH1,
    DMA_TIM8_CH2,
    DMA_TIM8_CH3,
    DMA_TIM8_CH4 ,
    DMA_TIM8_TRIG ,
    DMA_TIM8_COM,
    DMA_TIM8_UP,

    DMA_DAC1,
    DMA_DAC2,
    DMA_ADC1,
    DMA_ADC2,
    DMA_ADC3,
    DMA_DCMI,
    DMA_SAI1_A,
    DMA_SAI1_B,
    DMA_CRYP_OUT,
    DMA_CRYP_IN,
    DMA_HASH_IN,
    DMA_SDIO,

} Peripheral_DMA_Typdef;




/* Structure to store device specific information */
typedef struct  _dma_tgt_handle_struct
{
    VOID        (*setup_func)(VOID  *);   /* Pointer type is structure */
    VOID        (*cleanup_func)(VOID);
    UINT32      dma_stream_io_addr;      /* DMA stream base address. */

    HAL_DMA_StateTypeDef  dmaState;   /* Stores state of low level driver code for DMA */

    UINT32  DMA_SxCR_copy;            /* local copy of the dma stream cr register */

  UINT32 PeriphDataAlignment;  /*!< Specifies the Peripheral data width.
                                      This parameter can be a value of @ref DMA_Peripheral_data_size                 */   

  UINT32 MemDataAlignment;     /*!< Specifies the Memory data width.
                                      This parameter can be a value of @ref DMA_Memory_data_size                     */
                               
  UINT32 Mode;                 /*!< Specifies the operation mode of the DMAy Streamx.
                                      This parameter can be a value of @ref DMA_mode
                                      @note The circular buffer mode cannot be used if the memory-to-memory
                                            data transfer is configured on the selected Stream                        */ 

  UINT32 Priority;             /*!< Specifies the software priority for the DMAy Streamx.
                                      This parameter can be a value of @ref DMA_Priority_level                       */

    
} DMA_TGT_HANDLE;

/* DMA IOCTL Base */
#define IOCTL_DMA_BASE                 (DV_IOCTL0+1)

/* DMA Power Base */
#define DMA_POWER_BASE                 (IOCTL_DMA_BASE + DMA_CMD_DELIMITER)

/* Power States */
#define DMA_OFF                        0
#define DMA_ON                         1

/* DMA total power states */
#define DMA_TOTAL_POWER_STATE_COUNT    2

/* DMA Open Mode */
#define DMA_OPEN_MODE                  1

/* Total labels; DMA and Power Labels */
#define DMA_TOTAL_LABELS               2

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

/* External Functions */
VOID    nu_bsp_drvr_dma_edma_init(const CHAR * key, INT startorstop);

/* Function prototypes for DMA target specific driver */
VOID      DMA_Tgt_Enable (DMA_INSTANCE_HANDLE *inst_handle); 
VOID      DMA_Tgt_Disable (DMA_INSTANCE_HANDLE *inst_handle);
VOID      DMA_Tgt_Setup(DMA_INSTANCE_HANDLE  *inst_ptr);
STATUS    DMA_Tgt_Configure_Chan(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan);
STATUS    DMA_Tgt_Reset_Chan(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan);
VOID      DMA_Tgt_HISR (VOID);
STATUS    DMA_Tgt_Data_Trans(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan);

#ifdef __cplusplus
}
#endif

#endif /* !DMA_TGT_H */

