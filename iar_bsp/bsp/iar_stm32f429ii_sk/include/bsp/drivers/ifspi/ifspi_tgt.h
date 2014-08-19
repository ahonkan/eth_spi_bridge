/*
 * Avinash Honkan   Terabit Radios
 * 6 Aug 2014
 *
 * These device routines sets up spi and protocol to create a 
 * spi-based network interface.  This code interfaces to the ethernet
 * driver to connect to the net stack.  This code is based on the _Tgt 
 * functions found in the bsp/driver/ethernet directory.
 * There will be lots of references to ETHERNET_* as the stack assumes
 * only an ethernet device will be connected to it.  
 *
 */


#ifndef IFSPI_TGT_H
#define IFSPI_TGT_H

#include "nucleus.h"
#include "drivers/nu_drivers.h"
#include "kernel/nu_kernel.h"
#include "networking/nu_networking.h"
#include "connectivity/nu_connectivity.h"

/* Need to include lwspi plus other interface files for the driver */

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

        
#ifndef NU_SPI_BUS_NAME_LEN
#define NU_SPI_BUS_NAME_LEN         8
#endif

/* Minimum frequency (Hz) for SPI to function */
#define IF_SPI_MIN_FREQUENCY                45000000

/* SPI Configuration Parameters */
#define IF_SPI_CONFIG       (SPI_CFG_SS_POL_LO | SPI_CFG_BO_LSB_FIRST | \
                             SPI_CFG_MODE_POL_LO | SPI_CFG_MODE_PHA_FIRST_EDGE | \
                             SPI_CFG_PROT_TI)

/*****************************/
/* DRIVER INTERFACE DEFINES  */
/*****************************/
#define IF_SPI_BAUD_RATE                    45000000

/* Size of receive buffer fixed at 128 bytes */
#define IF_SPI_BUF_SIZE                  CFG_NU_OS_NET_STACK_BUF_SIZE

/*********************/
/*  DATA STRUCTURES  */
/*********************/

typedef struct IF_SPI_Frame_struct
{
    UINT32      sof;      
    UINT16      len;
    UINT8       data[IF_SPI_BUF_SIZE];
    UINT32      eof;      

} IF_SPI_Frame;


/* 
 * Structure for storing target-specific data
 */
typedef struct IF_SPI_TARGET_STRUCT
{
    CHAR            spi_bus_name[NU_SPI_BUS_NAME_LEN + 1];
    UINT32          spi_dev_ctrl;
    UINT32          spi_baud_rate;

    /* IFSPI Internal data */
    NU_SPI_HANDLE   spiHandle;

    IF_SPI_Frame    tx_frame;
    IF_SPI_Frame    rx_frame;


    NU_TASK         tcb;


} IF_SPI_TARGET_DATA;


/* Public function prototypes */
VOID        nu_bsp_drvr_enet_stm32_emac_init (const CHAR * key, INT startstop);
INT         IF_SPI_Tgt_Xmit_Packet (DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr);
STATUS      IF_SPI_Tgt_MII_Read (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 *data);
STATUS      IF_SPI_Tgt_MII_Write (DV_DEVICE_ENTRY *device, INT phy_addr, INT reg_addr, UINT16 data);

/*****************************************************/
/* FUNCTION PROTOTYPES available to the DV_Interface */
/*****************************************************/
STATUS      IF_SPI_Tgt_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read);
STATUS      IF_SPI_Tgt_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written);
VOID        IF_SPI_Tgt_Enable (ETHERNET_INSTANCE_HANDLE *inst_handle);
VOID        IF_SPI_Tgt_Disable (ETHERNET_INSTANCE_HANDLE *inst_handle);
STATUS      IF_SPI_Tgt_Create_Extended_Data (DV_DEVICE_ENTRY *device);
VOID        IF_SPI_Tgt_Target_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);
STATUS      IF_SPI_Tgt_Controller_Init (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device, UINT8 *ether_addr);
VOID        IF_SPI_Tgt_Get_ISR_Info (ETHERNET_SESSION_HANDLE *ses_handle, ETHERNET_ISR_INFO *isr_info);
VOID        IF_SPI_Tgt_Set_Phy_Dev_ID (ETHERNET_INSTANCE_HANDLE *inst_handle);
STATUS      IF_SPI_Tgt_Get_Link_Status (ETHERNET_INSTANCE_HANDLE *inst_handle, INT *link_status);
STATUS      IF_SPI_Tgt_Phy_Initialize (ETHERNET_INSTANCE_HANDLE *inst_handle, DV_DEVICE_ENTRY *device);
VOID        IF_SPI_Tgt_Notify_Status_Change (ETHERNET_INSTANCE_HANDLE *inst_handle);
VOID        IF_SPI_Tgt_Update_Multicast (DV_DEVICE_ENTRY *device);
STATUS  	IF_SPI_Tgt_Get_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr);
STATUS      IF_SPI_Tgt_Set_Address (ETHERNET_INSTANCE_HANDLE *inst_handle, UINT8 *ether_addr);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef IFSPI_TGT_H */
