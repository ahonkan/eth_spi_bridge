/**************************************************************************
*            Copyright 2007 Mentor Graphics Corporation
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
*       spi_dv_interface.h
*
* COMPONENT
*
*       SPI_DRIVER                          - Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains the function prototypes and data structures
*       for SPI driver.
*
* DATA STRUCTURES
*
*       SPI_INSTANCE_HANDLE
*       SPI_DRV_SESSION
*
**************************************************************************/

/* Check to see if the file has already been included. */
#ifndef     SPI_DV_INTERFACE_H
#define     SPI_DV_INTERFACE_H
#define     NU_SPI_SOURCE_FILE

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* SPI Driver Error codes */
#define SPI_DRV_DEFAULT_ERROR         -1
#define SPI_DRV_TOO_MANY_LABELS       -2
#define SPI_DRV_NO_INSTANCE_AVAILABLE -3
#define SPI_DRV_TGT_SETUP_FAILED      -4
#define SPI_DRV_NO_SESSION_AVAILABLE  -5
#define SPI_DRV_DEV_IN_USE            -6
#define SPI_DEV_ALREADY_REGISTERED    -7
#define SPI_DRV_INVALID_PARAMS        -8
#define SPI_DRV_INVALID_OPEN_MODE     -9
#define SPI_DEV_ALREADY_IN_USE        -10
#define SPI_DEV_REGISTRY_ERROR        -11
#define SPI_PMS_ERROR                 -12

/* SPI instance structure */
typedef struct  _SPI_INSTANCE_HANDLE_struct
{
    BOOLEAN                 device_in_use;
    DV_DEV_ID               dev_id;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    SPI_CB                  *spi_cb;
    UINT32                  spi_io_addr;
    UINT32                  spi_clock;
    INT                     spi_intr_vector;
    INT                     spi_intr_priority;
    ESAL_GE_INT_TRIG_TYPE   spi_intr_type;
    INT                     spi_slave_cnt;
    INT                     spi_master_mode;    /* Flag to indicate whether
                                                   this SPI device is running
                                                   as a master.             */
    INT                     spi_driver_mode;    /* Specifies whether Nucleus
                                                   SPI will handle this
                                                   device in polling,
                                                   interrupt driven or
                                                   interrupt driven with user
                                                   buffering mode .         */
    BOOLEAN                 spi_sim_tx_attribs; /* Flag to indicate if SPI 
                                                   controller would require
                                                   Transfer attribute 
                                                   simulation.              */
    VOID                   *spi_reserved;
    CHAR                    spi_ref_clock[NU_DRVR_REF_CLOCK_LEN];
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE   pmi_dev;
#endif
} SPI_INSTANCE_HANDLE;

/* SPI session structure */
typedef struct  _spi_drv_session_struct
{
    SPI_INSTANCE_HANDLE *spi_inst_ptr;
    UINT32              open_mode;

} SPI_DRV_SESSION;

/* CONFIGURABLE */
#define SPI_DRV_MAX_LABEL_CNT       10
#define SPI_MODE_IOCTL_BASE         (DV_IOCTL0+1)

/* Open Modes */
#define SPI_OPEN_MODE               0x1

/* SPI total power states */
#define SPI_TOTAL_POWER_STATE_COUNT 2
#define SPI_POWER_BASE              (SPI_MODE_IOCTL_BASE + SPI_CLASS_CMD_DELIMITER)

/* Power event bits */
#define SPI_PWR_ON_EVT              0x1
#define SPI_PWR_RESUME_EVT          0x2

/* Power States */
#define SPI_OFF                     0    
#define SPI_ON                      1 

/* Minimum DVFS OP for SPI to perform correctly */
#define SPI_MIN_DVFS_OP             1

/* Helpful macros */
#define SPI_DRV_BA_FROM_I_PTR(inst_ptr)     (inst_ptr->spi_io_addr)
#define SPI_DRV_BA_FROM_S_PTR(ses_ptr)      (ses_ptr->spi_inst_ptr->spi_io_addr)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  SPI_Dv_Register (const CHAR * key, SPI_INSTANCE_HANDLE* spi_inst_ptr);
STATUS  SPI_Dv_Unregister (const CHAR * key, INT startstop, DV_DEV_ID dev_id);
STATUS  SPI_Dv_Open (VOID *inst_ptr, DV_DEV_LABEL label_list[], 
                        INT label_cnt, VOID* *session_handle);
STATUS  SPI_Dv_Close(VOID *sess_handle);
STATUS  SPI_Dv_Ioctl (VOID *sess_ptr, INT ioctl_cmd, VOID *ioctl_data, INT length);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* !SPI_DV_INTERFACE_H */
