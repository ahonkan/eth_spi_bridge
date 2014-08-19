/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       dma.h
*
* COMPONENT
*
*       DMA Device Interface  - Nucleus DMA device interface file
*
* DESCRIPTION
*
*       This file contains code for Nucleus DMA device interface.
*
*************************************************************************/

/* Check to avoid multiple file inclusion. */
#ifndef     DMA_H
#define     DMA_H

/* Standard GUID's for DMA Driver */
#define DMA_LABEL   {0x00,0xd4,0x4e,0xa7,0x71,0x37,0x46,0x67,0xb7,0xf9,0xe1,0x0f,0xe6,0x8b,0x54,0xf8}

/*******************/
/* IOCTL CMDs      */
/*******************/
#define DMA_ACQUIRE_CHANNEL              10
#define DMA_RELEASE_CHANNEL              11
#define DMA_RESET_CHANNEL                12
#define DMA_DATA_TRANSFER                13
#define DMA_SET_COMP_CALLBACK            14

/***********************/
/* DMA ERROR CODES     */
/***********************/
#define NU_DMA_STATUS_BASE              -120000
#define NU_DMA_ERROR                    NU_DMA_STATUS_BASE-1
#define NU_DMA_DEVICE_NOT_FOUND         NU_DMA_STATUS_BASE-2
#define NU_DMA_INVALID_HANDLE           NU_DMA_STATUS_BASE-3
#define NU_DMA_CHANNEL_ALREADY_OPEN     NU_DMA_STATUS_BASE-4
#define NU_DMA_CHANNEL_MAX_USER_REACHED NU_DMA_STATUS_BASE-5
#define NU_DMA_CHANNEL_MAX_REACHED      NU_DMA_STATUS_BASE-6
#define NU_DMA_INVALID_PARAM            NU_DMA_STATUS_BASE-7
#define NU_DMA_INVALID_COMM_MODE        NU_DMA_STATUS_BASE-8
#define NU_DMA_INVALID_CHANNEL          NU_DMA_STATUS_BASE-9
#define NU_DMA_CHANNEL_USER_EXIST       NU_DMA_STATUS_BASE-10
#define NU_DMA_DEV_NOT_FOUND            NU_DMA_STATUS_BASE-11
#define NU_DMA_IOCTL_INVALID_LENGTH     NU_DMA_STATUS_BASE-12
#define NU_DMA_IOCTL_INVALID_MODE       NU_DMA_STATUS_BASE-13
#define NU_DMA_REGISTRY_ERROR           NU_DMA_STATUS_BASE-14
#define NU_DMA_INVALID_SESSION          NU_DMA_STATUS_BASE-15
#define NU_DMA_DEVICE_NOT_FREE          NU_DMA_STATUS_BASE-16
#define NU_DMA_DRIVER_ERROR             NU_DMA_STATUS_BASE-17
#define NU_DMA_ALREADY_OPEN             NU_DMA_STATUS_BASE-18
#define NU_DMA_RESET_FAIL               NU_DMA_STATUS_BASE-19
#define NU_DMA_INVALID_REQUEST_COUNT    NU_DMA_STATUS_BASE-20

/* Define following CFG_NU_OS defaults */
/*
#ifndef CFG_NU_OS_DRVR_DMA_MAX_DEVICES
#define CFG_NU_OS_DRVR_DMA_MAX_DEVICES  1
#endif

#ifndef CFG_NU_OS_DRVR_DMA_MAX_CHANNELS
#define CFG_NU_OS_DRVR_DMA_MAX_CHANNELS 1
#endif

#ifndef CFG_NU_OS_DRVR_DMA_MAX_CHANNEL_USERS
#define CFG_NU_OS_DRVR_DMA_MAX_CHANNEL_USERS 1
#endif
*/
/* Maximum devices supported. */
#define DMA_MAX_DEVICES                 CFG_NU_OS_DRVR_DMA_MAX_DEVICES

/* Maximum channels supported. */
#define DMA_MAX_CHANNELS                CFG_NU_OS_DRVR_DMA_MAX_CHANNELS

/* Maximum users supported per channel. */
#define DMA_MAX_CHAN_USERS              CFG_NU_OS_DRVR_DMA_MAX_CHANNEL_USERS

/* Maximum users supported per channel. */
#define DMA_CHANNEL_HANDLE_SIGNATURE    0xC3000000

/* Macro to validate channel handle. */
#define DMA_CHECK_VALID_CHAN_HANDLE(chan_handle)  \
        ((chan_handle & 0xFF000000) == DMA_CHANNEL_HANDLE_SIGNATURE)

/* Macro to get device index from handle. */
#define DMA_GET_DEV_CB_INDEX(chan_handle)  \
        ((chan_handle >> 16) & 0xFF)

/* Macro to get channel index from handle. */
#define DMA_GET_CHAN_INDEX(chan_handle)  \
        (chan_handle & 0xFF)

/* Macro to get user index from handle. */
#define DMA_GET_USER_INDEX(chan_handle)  \
        ((chan_handle >> 8) & 0xFF)

/* DMA transfer types. */
typedef enum
{
    DMA_FREE,
    DMA_SYNC_SEND,
    DMA_SYNC_RECEIVE,
    DMA_ASYNC_SEND,
    DMA_ASYNC_RECEIVE,
    DMA_SYNC_MEM_TRANS,
    DMA_ASYNC_MEM_TRANS

} DMA_REQUEST_TYPE;

/* DMA peripheral address types. */
typedef enum
{
    DMA_ADDRESS_INCR,
    DMA_ADDRESS_DECR,
    DMA_ADDRESS_FIXED,
    DMA_ADDRESS_DOUBLE_BUFFER = 0x80000000,     /* Bit Or to select double buffer mode */
    
} DMA_ADDRESS_TYPE;

#define DMA_ADDRESS_MASK  (DMA_ADDRESS_INCR | DMA_ADDRESS_DECR | DMA_ADDRESS_FIXED)

/* DMA request structure for data transfer. */
/* For double-buffer address types, these pointers will point to memory
 * of size 2x <length>.  So the first <length> values will be first buffer
 * and second <length> values will be the second.  For example, 
 * it will be src_ptr[2][<length>] for source double buffer
 */
typedef struct _dma_req_struct
{
    VOID *src_ptr;
    VOID *dst_ptr;
    UINT32 length;
    
    DMA_ADDRESS_TYPE  src_add_type;
    DMA_ADDRESS_TYPE  dst_add_type;

    VOID *req_reserve;

} DMA_REQ;


/* This define is used to indicate the tranfer is continuous, the dma channel is blocked 
 * forever, transferring data.  The callbacks will occur without freeing the channel.
 */
#define DMA_LENGTH_CONTINUOUS  (0xFFFFFFFF)

/* DMA channel handle. Least significant byte in channel handle holds channel
   index in channel array. LSB+1 byte holds user index of the specified channel.
   LSB+2 byte holds device index in device array. */
typedef UINT32 DMA_CHAN_HANDLE;

/* Structure to hold DMA channel information. */
 typedef struct _dma_channel_struct
{
    VOID             *next;                 /* Link to next channel on transfer requests channel list. */
    VOID              (*comp_callback)();   /* Completion call back function. */
    DMA_REQ          *cur_req_ptr;          /* Current request pointer on the channel. */
    UINT32            cur_req_length;       /* Current request length on the channel. */
    DMA_CHAN_HANDLE   chan_handle;          /* Channel handle. */
    DMA_REQUEST_TYPE  cur_req_type;         /* Current request type on the channel. */
    UINT8             hw_chan_id;           /* DMA hardware channel id. This channel id would 
                                               be used by underlying DMA device driver. */
    UINT8             peri_id;              /* Peripheral id. This id would be used by underlying DMA device driver. */
    BOOLEAN           enabled;              /* Flag to indicate the channel is enabled. */

    UINT8             free_buffer_idx;      /* Valid only in double-buffer mode, this index points to 
                                               the buffer that is not currently being written to or read from by the
                                               DMA engine. */ 

    UINT8             pad[2];

} DMA_CHANNEL;

#define NU_DMA_NAME_LEN         8

/* Structure to hold DMA device information. */
typedef struct _dma_device_struct
{
    CHAR            name[NU_DMA_NAME_LEN + 1];
    DV_DEV_HANDLE   dev_handle;                                        /* Device Handle. */
    NU_EVENT_GROUP  chan_comp_evt[(DMA_MAX_CHANNELS + 15) / 16];       /* Events to manage transfer completion and
                                                                          errors on channels (2 events per channel). */
    DMA_CHANNEL     channels[DMA_MAX_CHANNELS][DMA_MAX_CHAN_USERS]; /* DMA channel structures. */
    NU_SEMAPHORE    chan_semaphore[DMA_MAX_CHANNELS];                  /* Semaphore for exclusive access of each channel. */
    UINT32          channels_inuse;                                    /* Flags to track channels in use (For optimized access). */
    UINT16          user_count;                                        /* User count of the device. */
    UINT8           dma_dev_id;                                        /* DMA device handle as mentioned in the platform file. */
    UINT8           pad[1];

} DMA_DEVICE;

/* DMA device handle. */
typedef DMA_DEVICE *DMA_DEVICE_HANDLE;


/* DMA generic driver interface functions. */
STATUS NU_DMA_Open(UINT8 dma_device_index,
                   DMA_DEVICE_HANDLE *dma_handle_ptr);
STATUS NU_DMA_Data_Transfer(DMA_CHAN_HANDLE chan_handle,
                            DMA_REQ * dma_req_ptr,
                            UINT32 total_requests,
                            UINT8 is_cached,
                            DMA_REQUEST_TYPE req_type,
                            UNSIGNED suspend);
STATUS NU_DMA_Acquire_Channel(DMA_DEVICE_HANDLE dma_handle,
                              DMA_CHAN_HANDLE *chan_handle_ptr,
                              UINT8 hw_chan_id,
                              UINT8 peri_id,
                              VOID (*compl_callback)(DMA_CHAN_HANDLE , DMA_REQ *, UINT32 , STATUS ));
STATUS NU_DMA_Release_Channel(DMA_CHAN_HANDLE chan_handle);
STATUS NU_DMA_Reset_Channel(DMA_CHAN_HANDLE chan_handle);
STATUS NU_DMA_Close(DMA_DEVICE_HANDLE dma_handle);

#endif      /* !DMA_H */

