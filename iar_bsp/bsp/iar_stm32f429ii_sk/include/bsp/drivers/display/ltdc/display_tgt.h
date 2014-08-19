/*************************************************************************
*
*            Copyright 2013 Mentor Graphics Corporation
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
*       display_tgt.h
*
*   COMPONENT
*
*       Display - ST LCD-TFT Display Controller driver
*
*   DESCRIPTION
*
*       This file contains defines, data structures and function prototypes
*       for ST LTDC driver.
*
*   DATA STRUCTURES
*
*       LTDC_TGT
*       LTDC_Init
*       LTDC_Layer_Init
*
*   DEPENDENCIES
*
*       None 
*
*************************************************************************/
#ifndef     DISPLAY_TGT_H
#define     DISPLAY_TGT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* LCDC device info structure. */
typedef struct _lcdc_dev_struct
{
    UINT32      lcdc_io_addr;
    UINT32      lcdc_screen_width;
    UINT32      lcdc_screen_height;
    VOID        *lcdc_framebuffer_addr;
    UINT32      lcdc_bits_per_pixel;
    CHAR        lcdc_ref_clock[NU_DRVR_REF_CLOCK_LEN];

    VOID        (*setup_func)(VOID);
    VOID        (*cleanup_func)(VOID);
} LTDC_TGT;

/* LTDC structure definition */
typedef struct
{
    UINT32 LTDC_HSPolarity;         
    UINT32 LTDC_VSPolarity;         
    UINT32 LTDC_DEPolarity;         
    UINT32 LTDC_PCPolarity;         
    UINT32 LTDC_HorizontalSync;     
    UINT32 LTDC_VerticalSync;       
    UINT32 LTDC_AccumulatedHBP;     
    UINT32 LTDC_AccumulatedVBP;     
    UINT32 LTDC_AccumulatedActiveW; 
    UINT32 LTDC_AccumulatedActiveH; 
    UINT32 LTDC_TotalWidth;         
    UINT32 LTDC_TotalHeigh;         
    UINT32 LTDC_BackgroundRedValue; 
    UINT32 LTDC_BackgroundGreenValue;      
    UINT32 LTDC_BackgroundBlueValue;      
} LTDC_Init;

/* LTDC Layer structure definition */
typedef struct
{
    UINT32 LTDC_HorizontalStart;                  
    UINT32 LTDC_HorizontalStop;       
    UINT32 LTDC_VerticalStart;        
    UINT32 LTDC_VerticalStop;         
    UINT32 LTDC_PixelFormat;          
    UINT32 LTDC_ConstantAlpha;        
    UINT32 LTDC_DefaultColorBlue;     
    UINT32 LTDC_DefaultColorGreen;    
    UINT32 LTDC_DefaultColorRed;      
    UINT32 LTDC_DefaultColorAlpha;    
    UINT32 LTDC_BlendingFactor_1;     
    UINT32 LTDC_BlendingFactor_2;     
    UINT32 LTDC_CFBStartAdress;       
    UINT32 LTDC_CFBLineLength;        
    UINT32 LTDC_CFBPitch;                                                              
    UINT32 LTDC_CFBLineNumber;        
} LTDC_Layer_Init;

/* Define if custom colors are used. */
#define     DISPLAY_CUSTOM_SOURCE_COLOR     NU_FALSE

/* Define source color positions and sizes. */

#if (DISPLAY_CUSTOM_SOURCE_COLOR == NU_TRUE)

#define     SRC_RED_POSITION            24
#define     SRC_GREEN_POSITION          16
#define     SRC_BLUE_POSITION           8
#define     SRC_ALPHA_POSITION          0

#define     SRC_RED_LENGTH              8
#define     SRC_GREEN_LENGTH            8
#define     SRC_BLUE_LENGTH             8
#define     SRC_ALPHA_LENGTH            8

#endif /* (DISPLAY_CUSTOM_SOURCE_COLOR == NU_TRUE) */

/* Define to NU_TRUE if DST colors in color_convert.h don't match with the
   hardware supported colors. */
#define     DISPLAY_CUSTOM_TARGET_COLOR     NU_FALSE

/* Define hardware supported color positions and sizes. */

#if (DISPLAY_CUSTOM_TARGET_COLOR == NU_TRUE)

#if (BPP == 16)

#define     DST_RED_POSITION            11
#define     DST_GREEN_POSITION          5
#define     DST_BLUE_POSITION           0
#define     DST_ALPHA_POSITION          0

#define     DST_RED_LENGTH              5
#define     DST_GREEN_LENGTH            6
#define     DST_BLUE_LENGTH             5
#define     DST_ALPHA_LENGTH            0

#else

#define     DST_RED_POSITION            0
#define     DST_GREEN_POSITION          8
#define     DST_BLUE_POSITION           16
#define     DST_ALPHA_POSITION          0

#define     DST_RED_LENGTH              8
#define     DST_GREEN_LENGTH            8
#define     DST_BLUE_LENGTH             8
#define     DST_ALPHA_LENGTH            0

#endif

#endif /* (DISPLAY_CUSTOM_COLOR == NU_TRUE) */

/* Min Op for the dipslay driver */
#define DISPLAY_MIN_DVFS_OP                 0

    /* Contrast levels. */
#define     LCD_CONTRAST_MAX                100

/* LCDC error codes. */
#define LCDC_NO_INSTANCE_AVAILABLE          -1
#define LCDC_SESSION_NOT_FOUND              -2
#define LCDC_REGISTRY_ERROR                 -3
#define LCDC_NOT_REGISTERED                 -4
#define LCDC_PM_ERROR                       -5
#define LCDC_INVALID_CONTRAST_VALUE         -6
#define LCDC_INVALID_CHIP_ID                -7
#define LCDC_INVALID_VALUE                  -8

/* Predefined contrast levels. */
#define LCD_CONTRAST_BRIGHT                 100
#define LCD_CONTRAST_ON                     80
#define LCD_CONTRAST_DIM                    40
#define LCD_CONTRAST_OFF                    0

/* LCD Control pin */ 
#define LCD_NCS_PIN                         GPIO_Pin_2                  
#define LCD_NCS_GPIO_CLK                    RCC_AHB1Periph_GPIOC 

/* LCD Command/data pin */
#define LCD_WRX_PIN                         GPIO_Pin_13                  

/* LTDC and LTDC Layer Base Address */
#define LTDC_BASE_ADDRESS                   (0x40016800)
#define LTDC_LAYER1_BASE_ADDRESS            (LTDC_BASE_ADDRESS + 0x84)

/* LTDC GCR Mask */
#define GCR_MASK                            ((UINT32)0x0FFE888F)  

/* LTDC Register Defines */
#define RESERVED0                           0x00
#define RESERVED1                           0x04
#define SSCR                                0x08
#define BPCR                                0x0C
#define AWCR                                0x10
#define TWCR                                0x14
#define GCR                                 0x18
#define RESERVED2                           0x1C
#define RESERVED3                           0x20
#define SRCR                                0x24
#define RESERVED4                           0x28
#define BCCR                                0x2C
#define RESERVED5                           0x30
#define IER                                 0x34 
#define ISR                                 0x38 
#define ICR                                 0x3C 
#define LIPCR                               0x40 
#define CPSR                                0x44 
#define CDSR                                0x48 

/* LTDC Layer Register Defines */
#define CR                                  0x00
#define WHPCR                               0x04
#define WVPCR                               0x08
#define CKCR                                0x0C
#define PFCR                                0x10
#define CACR                                0x14
#define DCCR                                0x18
#define BFCR                                0x1C
#define RESERVED6                           0x20
#define RESERVED7                           0x24
#define CFBAR                               0x28 
#define CFBLR                               0x2C
#define CFBLNR                              0x30

/* ILI9341 LCD (Based on SPI) Registers */
#define LCD_SLEEP_OUT                       0x11   /* Sleep out register */
#define LCD_GAMMA                           0x26   /* Gamma register */
#define LCD_DISPLAY_OFF                     0x28   /* Display off register */
#define LCD_DISPLAY_ON                      0x29   /* Display on register */
#define LCD_COLUMN_ADDR                     0x2A   /* Colomn address register */ 
#define LCD_PAGE_ADDR                       0x2B   /* Page address register */ 
#define LCD_GRAM                            0x2C   /* GRAM register */   
#define LCD_MAC                             0x36   /* Memory Access Control register*/
#define LCD_PIXEL_FORMAT                    0x3A   /* Pixel Format register */
#define LCD_WDB                             0x51   /* Write Brightness Display register */
#define LCD_WCD                             0x53   /* Write Control Display register*/
#define LCD_RGB_INTERFACE                   0xB0   /* RGB Interface Signal Control */
#define LCD_FRC                             0xB1   /* Frame Rate Control register */
#define LCD_BPC                             0xB5   /* Blanking Porch Control register*/
#define LCD_DFC                             0xB6   /* Display Function Control register*/
#define LCD_POWER1                          0xC0   /* Power Control 1 register */
#define LCD_POWER2                          0xC1   /* Power Control 2 register */
#define LCD_VCOM1                           0xC5   /* VCOM Control 1 register */
#define LCD_VCOM2                           0xC7   /* VCOM Control 2 register */
#define LCD_POWERA                          0xCB   /* Power control A register */
#define LCD_POWERB                          0xCF   /* Power control B register */
#define LCD_PGAMMA                          0xE0   /* Positive Gamma Correction register*/
#define LCD_NGAMMA                          0xE1   /* Negative Gamma Correction register*/
#define LCD_DTCA                            0xE8   /* Driver timing control A */
#define LCD_DTCB                            0xEA   /* Driver timing control B */
#define LCD_POWER_SEQ                       0xED   /* Power on sequence register */
#define LCD_3GAMMA_EN                       0xF2   /* 3 Gamma enable register */
#define LCD_INTERFACE                       0xF6   /* Interface control register */
#define LCD_PRC                             0xF7   /* Pump ratio control register */

/* LCD-TFT Display Controller (LTDC) Register Bit Defines */
#define LTDC_SSCR_VSH                       ((UINT32)0x000007FF)             
#define LTDC_SSCR_HSW                       ((UINT32)0x0FFF0000)             
#define LTDC_BPCR_AVBP                      ((UINT32)0x000007FF)             
#define LTDC_BPCR_AHBP                      ((UINT32)0x0FFF0000)             
#define LTDC_AWCR_AAH                       ((UINT32)0x000007FF)             
#define LTDC_AWCR_AAW                       ((UINT32)0x0FFF0000)             
#define LTDC_TWCR_TOTALH                    ((UINT32)0x000007FF)             
#define LTDC_TWCR_TOTALW                    ((UINT32)0x0FFF0000)             
#define LTDC_GCR_LTDCEN                     ((UINT32)0x00000001)             
#define LTDC_GCR_DTEN                       ((UINT32)0x00010000)             
#define LTDC_SRCR_IMR                       ((UINT32)0x00000001)             
#define LTDC_BCCR_BCBLUE                    ((UINT32)0x000000FF)             
#define LTDC_BCCR_BCGREEN                   ((UINT32)0x0000FF00)             
#define LTDC_BCCR_BCRED                     ((UINT32)0x00FF0000)             
#define LTDC_LxCR_LEN                       ((UINT32)0x00000001)             
#define LTDC_LxWHPCR_WHSTPOS                ((UINT32)0x00000FFF)             
#define LTDC_LxWHPCR_WHSPPOS                ((UINT32)0xFFFF0000)             
#define LTDC_LxWVPCR_WVSTPOS                ((UINT32)0x00000FFF)             
#define LTDC_LxWVPCR_WVSPPOS                ((UINT32)0xFFFF0000)             
#define LTDC_LxPFCR_PF                      ((UINT32)0x00000007)             
#define LTDC_LxCACR_CONSTA                  ((UINT32)0x000000FF)             
#define LTDC_LxDCCR_DCBLUE                  ((UINT32)0x000000FF)             
#define LTDC_LxDCCR_DCGREEN                 ((UINT32)0x0000FF00)             
#define LTDC_LxDCCR_DCRED                   ((UINT32)0x00FF0000)             
#define LTDC_LxDCCR_DCALPHA                 ((UINT32)0xFF000000)             
#define LTDC_LxBFCR_BF2                     ((UINT32)0x00000007)             
#define LTDC_LxBFCR_BF1                     ((UINT32)0x00000700)             
#define LTDC_LxCFBAR_CFBADD                 ((UINT32)0xFFFFFFFF)             
#define LTDC_LxCFBLR_CFBLL                  ((UINT32)0x00001FFF)             
#define LTDC_LxCFBLR_CFBP                   ((UINT32)0x1FFF0000)             
#define LTDC_LxCFBLNR_CFBLNBR               ((UINT32)0x000007FF)   
#define LTDC_HSPolarity_AL                  ((UINT32)0x00000000)   
#define LTDC_VSPolarity_AL                  ((UINT32)0x00000000)   
#define LTDC_DEPolarity_AL                  ((UINT32)0x00000000)   
#define LTDC_PCPolarity_IPC                 ((UINT32)0x00000000)   
#define LTDC_IMReload                       LTDC_SRCR_IMR          
#define LTDC_Pixelformat_RGB565             ((UINT32)0x00000002)
#define LTDC_BlendingFactor1_CA             ((UINT32)0x00000400)
#define LTDC_BlendingFactor2_CA             ((UINT32)0x00000005)

/* SPI Defines */
#define SPI5_BASE_ADDRESS                   0x40015000
#define SPI_DEVICE_NAME                     "spi0"
#define SPI_BAUD_RATE                       25000
#define SPI_TRANSFER_SIZE                   8
#define SPI_TRANSFER_MODE                   SPI_MODE_POL_LO_PHA_LO
#define SPI_TRANSFER_BIT_ORDER              SPI_BO_MSB_FIRST
#define SPI_SS_POL_DONT_CARE                2

/* Public function prototypes. */
STATUS      Display_Tgt_Get_Target_Info(const CHAR * key, LTDC_TGT *tgt_info);
STATUS      Display_Tgt_Initialize(DISPLAY_INSTANCE_HANDLE *instance_ptr, VOID *display_framebuffer_addr);
VOID        Display_Tgt_Read_Palette(VOID *palette_ptr);
VOID        Display_Tgt_Write_Palette(VOID *palette_ptr);
VOID        Display_Tgt_Pre_Process_Hook(VOID);
VOID        Display_Tgt_Post_Process_Hook(VOID);
VOID        Display_Tgt_Shutdown(DISPLAY_INSTANCE_HANDLE *instance_ptr);
STATUS      Display_Tgt_Backlight_Set_ON(VOID *tgt_info);
STATUS      Display_Tgt_Backlight_Set_OFF(VOID *tgt_info);
STATUS      Display_Tgt_Backlight_Status(VOID *tgt_info, BOOLEAN  *backlight_status_ptr);
STATUS      Display_Tgt_Contrast_Set(VOID *tgt_info, INT8 contrast);
STATUS      Display_Tgt_Contrast_Get(VOID *tgt_info, INT8 *contrast_ptr);
VOID        *Display_Tgt_Allocate_Framebuffer(VOID *tgt_info);
STATUS      Display_Tgt_Set_State_ON(VOID *tgt_info);
STATUS      Display_Tgt_Set_State_OFF(VOID *tgt_info);

VOID        Display_Tgt_LCD_Configuration();
VOID        Display_Tgt_LCD_Write_Command(UINT8 LCD_Reg);
VOID        Display_Tgt_LCD_Write_Data(UINT8 value);
VOID        Display_Tgt_LCD_Enable_Power(VOID);
VOID        Display_Tgt_LCD_Display_ON(VOID);
VOID        Display_Tgt_LCD_Display_OFF(VOID);
VOID        Display_Tgt_LCD_SPI_Configuration(VOID);
VOID        Display_Tgt_LTDC_Initialization(LTDC_Init* LTDC_InitStruct);
VOID        Display_Tgt_LTDC_Enable(FunctionalState NewState);
VOID        Display_Tgt_LTDC_Dither_Enable(FunctionalState NewState);
VOID        Display_Tgt_LTDC_Layer_Initialization(UINT32 LTDC_Layerx, LTDC_Layer_Init* LTDC_Layer_InitStruct);
VOID        Display_Tgt_LTDC_Enable_Layer(UINT32 LTDC_Layerx, FunctionalState NewState);
VOID        Display_Tgt_SPI_I2S_Send_Data(UINT32 SPIx, UINT8 Data);

#ifdef __cplusplus
}
#endif

#endif      /* DISPLAY_TGT_H */

