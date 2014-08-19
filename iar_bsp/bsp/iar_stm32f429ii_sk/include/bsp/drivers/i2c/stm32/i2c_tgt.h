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
*       i2c_tgt.h
*
*   COMPONENT
*
*       TWI                               - TWI controller driver
*
*   DESCRIPTION
*
*       This file contains target specific constants / defines / etc
*       for the UART hardware
*
*   DATA STRUCTURES
*
*       TWI_TGT                           - Structure to hold target
*                                             specific information.
*
*   DEPENDENCIES
*
*       nucleus.h                         - PLUS
*       device_manager.h                  - Device Manager
*       power_core.h                      - PMS
*
*************************************************************************/
#ifndef I2C_TGT_H
#define I2C_TGT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* I2C target info structure. */
typedef struct _i2c_tgt_struct
{
    VOID        (*setup_func)(VOID);
    VOID        (*cleanup_func)(VOID);
    UINT16      slave_addr;
} I2C_TGT;

#define     I2C_DRIVER_MODE_MASTER   1
#define     I2C_DRIVER_MODE_SLAVE    2

/* Register offsets for various I2C registers in
   I2C module memory space. */

#define I2C_CR1                     0x00
#define I2C_CR2                     0x04
#define I2C_OAR1                    0x08
#define I2C_OAR2                    0x0C
#define I2C_DR                      0x10
#define I2C_SR1                     0x14
#define I2C_SR2                     0x18
#define I2C_CCR                     0x1C
#define I2C_TRISE                   0x20

/* I2C_CR1 register bits. */
#define I2C_CR1_PE                  0x0001
#define I2C_CR1_SMBUS               0x0002
#define I2C_CR1_SMB_TYPE            0x0008
#define I2C_CR1_ENARP               0x0010
#define I2C_CR1_ENPEC               0x0020
#define I2C_CR1_ENGC                0x0040
#define I2C_CR1_NO_STRETCH          0x0080
#define I2C_CR1_START               0x0100
#define I2C_CR1_STOP                0x0200
#define I2C_CR1_ACK_ENABLED         0x0400
#define I2C_CR1_POS                 0x0800
#define I2C_CR1_PEC                 0x1000
#define I2C_CR1_ALERT               0x2000
#define I2C_CR1_SWRST               0x8000

/* I2C_CR2 register bits. */
#define I2C_CR2_FREQ_MASK           0x003F
#define I2C_CR2_ITERREN             0x0100
#define I2C_CR2_ITEVFEN             0x0200
#define I2C_CR2_ITBUFEN             0x0400
#define I2C_CR2_DMAEN               0x0800
#define I2C_CR2_LAST                0x1000
#define I2C_ADD_SHIFT               0x01

/* I2C_SR1 register bit masks. */
#define I2C_SR1_SB                  0x0001
#define I2C_SR1_ADDR                0x0002
#define I2C_SR1_BTF                 0x0004
#define I2C_SR1_ADD10               0x0008
#define I2C_SR1_STOPF               0x0010
#define I2C_SR1_RXNE                0x0040
#define I2C_SR1_TXE                 0x0080
#define I2C_SR1_BERR                0x0100
#define I2C_SR1_ARLO                0x0200
#define I2C_SR1_AF                  0x0400
#define I2C_SR1_OVR                 0x0800
#define I2C_SR1_PEC_ERR             0x1000
#define I2C_SR1_TIME_OUT            0x4000
#define I2C_SR1_SMB_ALERT           0x8000

/* I2C_SR2 register bit masks. */
#define I2C_SR2_MSL                 0x0001
#define I2C_SR2_BUS_BUSY            0x0002
#define I2C_SR2_TRA                 0x0004
#define I2C_SR2_GEN_CALL            0x0010
#define I2C_SR2_SBM_DEFAULT         0x0020
#define I2C_SR2_SBM_HOST            0x0040
#define I2C_SR2_DUALF               0x0080

/* I2C_CCR register bit masks. */
#define I2C_CCR_DUTY                0x4000
#define I2C_CCR_FM                  0x8000

/* Frequency divider values for generating specific I2C baud rates. */

/* Values of ICCL field in ICCLKL register. These values determine the
   amount of time for which I2C clock is low. */

#define I2C_ICCWGR_VALUE_400K_BAUD  400000
#define I2C_ICCWGR_VALUE_250K_BAUD  250000
#define I2C_ICCWGR_VALUE_200K_BAUD  200000
#define I2C_ICCWGR_VALUE_125K_BAUD  125000
#define I2C_ICCWGR_VALUE_100K_BAUD  100000
#define I2C_ICCWGR_VALUE_50K_BAUD   50000
#define I2C_ICCWGR_VALUE_25K_BAUD   25000


/*******************************************************
              Driver operation MACROS
*******************************************************/


/* Macro to send I2C STOP signal. */
#define     I2C_DRIVER_SEND_STOP_SIGNAL(i2c_base_address)               \
            (ESAL_GE_MEM_WRITE16(i2c_base_address + I2C_CR1,            \
                (ESAL_GE_MEM_READ16(i2c_base_address + I2C_CR1) | I2C_CR1_STOP)))

/* Macro to write data to I2C data register. */
#define     I2C_DRIVER_SEND_DATA(i2c_base_address, data)                \
            (ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_DR), data))

/* Macro to read data from I2C data register. */
#define     I2C_DRIVER_GET_DATA(i2c_base_address)                       \
            (ESAL_GE_MEM_READ16(i2c_base_address + I2C_DR))

/* Macro to send I2C START signal. */
#define     I2C_DRIVER_SEND_START_SIGNAL(i2c_base_address)              \
            (ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_ICCR), I2C_ICCR_MSEN | I2C_ICCR_START))

/* Macro to check the STOP signal. */
#define     I2C_DRIVER_STOP_DETECTED(i2c_base_address)                  \
            (ESAL_GE_MEM_READ16(i2c_base_address + I2C_ICSTR) & I2C_ICSTR_SCD_MASK)

/* Macro to set the target slave address with which to communicate. */
#define     I2C_DRIVER_SET_TARGET_ADDRESS(i2c_base_address, address)    \
            (ESAL_GE_MEM_WRITE16((i2c_base_address + I2C_ICMMR), \
             (ESAL_GE_MEM_READ16(i2c_base_address + I2C_ICMMR) & ~I2C_ICMMR_DADR_MASK) | (address << I2C_ICMMR_DADR_SHIFT)))

/* Macro to check Data ACK. */
#define     I2C_DRIVER_ACK_DETECTED(i2c_base_address)                  \
            (!((ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1) & I2C_SR1_AF) == I2C_SR1_AF))

/* Macro to check if bus is busy. */
#define     I2C_DRIVER_CHECK_BUS_FREE(i2c_base_address)                 \
            (!((ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR2) & I2C_SR2_BUS_BUSY) == I2C_SR2_BUS_BUSY))

#define     I2C_TGT_OR_OUT(i2c_base_address, value)                         \
            ESAL_GE_MEM_WRITE16 ((i2c_base_address),                        \
                (ESAL_GE_MEM_READ16 (i2c_base_address) | (value)));

#define     I2C_TGT_AND_OUT(i2c_base_address, value)                        \
            ESAL_GE_MEM_WRITE16 ((i2c_base_address),                        \
                (ESAL_GE_MEM_READ16 (i2c_base_address) & (value)));

/* Macro to send acknowledgment. */
#define     I2C_DRIVER_SEND_ACK(i2c_base_address)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

VOID I2C_Tgt_Shutdown (I2C_INSTANCE_HANDLE *i_handle);

#endif /* !I2C_TGT_H */
