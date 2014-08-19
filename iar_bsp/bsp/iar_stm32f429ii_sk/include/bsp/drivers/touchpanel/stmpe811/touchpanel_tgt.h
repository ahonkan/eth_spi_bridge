/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                      All Rights Reserved.
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
*       touchpanel_tgt.h
*
*   COMPONENT
*
*       STMPE811 - STMPE811 touch panel controller driver
*
*   DESCRIPTION
*
*       This file contains the target specific defines and function
*       prototypes for STMPE811 touch panel controller driver.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       touchpanel_tgt_power.h
*
*************************************************************************/

#ifndef STMPE811_TGT_H
#define STMPE811_TGT_H

/*********************************/
/* TARGET SPECIFIC STRUCTURE     */
/*********************************/
typedef struct _touchpanel_tgt_struct
{
    UINT32      i2c_addr;
    UINT32      i2c_mode;
    I2C_HANDLE  i2c_handle;    
    I2C_NODE    i2c_node;
    VOID        (*setup_func)(VOID);
    VOID        (*cleanup_func)(VOID);
    BOOLEAN     initialized;    /* Initialization status of touch panel controller. */

} TOUCHPANEL_TGT;

/* Register set for STMPE811. */

/* Identification registers */
#define IO_EXPANDER_CHP_ID              0x00
#define IO_EXPANDER_ID_VER              0x02

/* General Control Registers */
#define IO_EXPANDER_SYS_CTRL1           0x03
#define IO_EXPANDER_SYS_CTRL2           0x04
#define IO_EXPANDER_SPI_CFG             0x08 

/* Interrupt System register */
#define IO_EXPANDER_INT_CTRL            0x09
#define IO_EXPANDER_INT_EN              0x0A
#define IO_EXPANDER_INT_STA             0x0B
#define IO_EXPANDER_GPIO_INT_EN         0x0C
#define IO_EXPANDER_GPIO_INT_STA        0x0D
#define IO_EXPANDER_ADC_INT_EN          0x0E
#define IO_EXPANDER_ADC_INT_STA         0x0F

/* GPIO Registers */
#define IO_EXPANDER_GPIO_SET_PIN       0x10
#define IO_EXPANDER_GPIO_CLR_PIN       0x11
#define IO_EXPANDER_GPIO_MP_STA        0x12
#define IO_EXPANDER_GPIO_DIR           0x13
#define IO_EXPANDER_GPIO_ED            0x14
#define IO_EXPANDER_GPIO_RE            0x15
#define IO_EXPANDER_GPIO_FE            0x16
#define IO_EXPANDER_GPIO_AF            0x17

/* ADC Registers */
#define IO_EXPANDER_ADC_CTRL1          0x20
#define IO_EXPANDER_ADC_CTRL2          0x21
#define IO_EXPANDER_ADC_DATA_CH0       0x30
#define IO_EXPANDER_ADC_DATA_CH1       0x32
#define IO_EXPANDER_ADC_DATA_CH2       0x34
#define IO_EXPANDER_ADC_DATA_CH3       0x36
#define IO_EXPANDER_ADC_DATA_CH4       0x38
#define IO_EXPANDER_ADC_DATA_CH5       0x3A
#define IO_EXPANDER_ADC_DATA_CH6       0x3B
#define IO_EXPANDER_ADC_DATA_CH7       0x3C 
#define IO_EXPANDER_ADC_CAPT           0x22

/* TouchScreen Registers */
#define IO_EXPANDER_TSC_CTRL           0x40
#define IO_EXPANDER_TSC_CFG            0x41
#define IO_EXPANDER_WDM_TR_X           0x42 
#define IO_EXPANDER_WDM_TR_Y           0x44
#define IO_EXPANDER_WDM_BL_X           0x46
#define IO_EXPANDER_WDM_BL_Y           0x48
#define IO_EXPANDER_FIFO_TH            0x4A
#define IO_EXPANDER_FIFO_STA           0x4B
#define IO_EXPANDER_FIFO_SIZE          0x4C
#define IO_EXPANDER_TSC_DATA_X         0x4D 
#define IO_EXPANDER_TSC_DATA_Y         0x4F
#define IO_EXPANDER_TSC_DATA_Z         0x51
#define IO_EXPANDER_TSC_DATA_XYZ       0x52 
#define IO_EXPANDER_TSC_FRACT_XYZ      0x56
#define IO_EXPANDER_TSC_DATA           0x57
#define IO_EXPANDER_TSC_I_DRIVE        0x58
#define IO_EXPANDER_TSC_SHIELD         0x59

/* IO_EXPANDER_SYS_CTRL1 bit defines. */
#define IO_EXPANDER_SYS_CTRL1_HIBERNATE         0x01
#define IO_EXPANDER_SYS_CTRL1_SOFT_RESET        0x02

/* IO_EXPANDER_SYS_CTRL2 bit defines. */
#define IO_EXPANDER_SYS_CTRL2_ADC_OFF           0x01
#define IO_EXPANDER_SYS_CTRL2_TOUCH_SCREEN_OFF  0x02
#define IO_EXPANDER_SYS_CTRL2_GPIO_OFF          0x04
#define IO_EXPANDER_SYS_CTRL2_TEMP_SENSOR_OFF   0x08

/* IO_EXPANDER_INT_CTRL bit defines. */
#define IO_EXPANDER_INT_CTRL_GLOB_INT           0x01
#define IO_EXPANDER_INT_CTRL_INT_TYPE           0x02
#define IO_EXPANDER_INT_CTRL_INT_POL            0x04

/* IO_EXPANDER_INT_EN bit defines. */
#define IO_EXPANDER_GPIO_INT_ENA                0x80
#define IO_EXPANDER_ADC_INT_ENA                 0x40
#define IO_EXPANDER_TEMP_TH_INT_ENA             0x20
#define IO_EXPANDER_FIFO_EMPTY_INT_ENA          0x10
#define IO_EXPANDER_FIFO_FULL_INT_ENA           0x08
#define IO_EXPANDER_FIFO_OV_INT_ENA             0x04
#define IO_EXPANDER_FIFO_TH_INT_ENA             0x02
#define IO_EXPANDER_TOUCH_INT_ENA               0x01

/* IO_EXPANDER_INT_STA bit defines. */
#define IO_EXPANDER_GPIO_INT_STA_BIT            0x80
#define IO_EXPANDER_ADC_INT_STA_BIT             0x40
#define IO_EXPANDER_TEMP_TH_INT_STA             0x20
#define IO_EXPANDER_FIFO_EMPTY_INT_STA          0x10
#define IO_EXPANDER_FIFO_FULL_INT_STA           0x08
#define IO_EXPANDER_FIFO_OV_INT_STA             0x04
#define IO_EXPANDER_FIFO_TH_INT_STA             0x02
#define IO_EXPANDER_TOUCH_INT_STA               0x01

#define IO_EXPANDER_TOUCHPANEL_PINS             (0x02 | 0x04 | 0x08 | 0x10)

/* Touch panel intervals in interrupt mode (in timer ticks, timer tick
   is 10ms by default in PLUS).

   The interrupt mode is hybrid. Sampling is done in polling manner but
   it starts only when pen touch interrupt occurs and stops when pen is
   up. */

#define     STMPE811_TGT_INITIAL_INTERVAL    1  /* Delay after the pen down
                                                   event before starting the
                                                   sampling.              */
#define     STMPE811_TGT_SAMPLING_INTERVAL   3  /* Sampling interval.     */

/* Sampling interval in polling mode. */
#define     STMPE811_TGT_POLLING_INTERVAL    5

/* Touch panel driver HISR priority. */
#define     STMPE811_TGT_HISR_PRIORITY       2

/* Touch panel driver task priority. */
#define     STMPE811_TGT_TASK_PRIORITY       31

/* Touch panel driver HISR stack size. */
#define     STMPE811_TGT_HISR_STACK_SIZE     NU_MIN_STACK_SIZE * 2

/* Touch panel driver task stack size. */
#define     STMPE811_TGT_TASK_STACK_SIZE     NU_MIN_STACK_SIZE * 5

/* Driver internal event mask. */

#define     STMPE811_TGT_PEN_DOWN_EVENT      0x1     /* Pen down event.    */

/* Driver I2C Events*/
#define     STMPE811_TGT_I2C_RX_DONE_EVENT   0x00000001
#define     STMPE811_TGT_I2C_TX_DONE_EVENT   0x00000002
#define     STMPE811_TGT_I2C_ERROR_EVENT     0x00000004

/* Noise range for validating X and Y coordinate measurements. */

#define     STMPE811_TGT_X_NOISY_SAMPLE_RANGE       100
#define     STMPE811_TGT_Y_NOISY_SAMPLE_RANGE       100

/* Macros for 32 bit access. */

#define     STMPE811_TGT_OUTDWORD(reg, data) ESAL_GE_MEM_WRITE32(reg, data)
#define     STMPE811_TGT_INDWORD(reg)        ESAL_GE_MEM_READ32(reg)

VOID        Touchpanel_Tgt_Initialize(TOUCHPANEL_INSTANCE_HANDLE *inst_info);
VOID        Touchpanel_Tgt_Shutdown (TOUCHPANEL_INSTANCE_HANDLE *inst_handle);
VOID        Touchpanel_Tgt_Enable(VOID);
VOID        Touchpanel_Tgt_Disable(VOID);

#endif /* STMPE811_TGT_H */

