/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       sd_imp.c
*
* COMPONENT
*
*       Nucleus SDIO FILE Function Driver's Atheros SDIO Stack Communication
*       layer.
*
* DESCRIPTION
*
*       This file contains SDIO Stack communication layer implementation
*       of Nucleus SDIO FILE Function Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       nu_os_drvr_sdio_func_file_init Calls NU_SDIO_FDR_Imp_Init
*       None
*
* DEPENDENCIES
*
*       sd_imp.h
*       sd_disk.h
*       runlevel_init.h
*
*************************************************************************/

#include "drivers/sd_imp.h"
#include "drivers/sd_disk.h"
#include "services/runlevel_init.h"

/* Common SD driver target specific information */
static SD_TGT SD_Tgt_Info;

SD_PNP_INFO PIds[] = {
                        {
                            0, /* SDIO_ManufacturerCode */
                            0, /* SDIO_ManufacturerID */
                            0, /* SDIO_FunctionNo */
                            0, /* SDIO_FunctionClass */
                            0, /* SDMMC_ManfacturerID */
                            0, /* SDMMC_OEMApplicationID */
                            (CARD_MMC),  /* CardFlags */
                        },
                        {
                            0, /* SDIO_ManufacturerCode */
                            0, /* SDIO_ManufacturerID */
                            0, /* SDIO_FunctionNo */
                            0, /* SDIO_FunctionClass */
                            0, /* SDMMC_ManfacturerID */
                            0, /* SDMMC_OEMApplicationID */
                            (CARD_SD),  /* CardFlags */
                        },

                        {
                            0                                 /* List is null terminated. */
                        }
                    };

SDFUNCTION Sdio_File_Drv_Context= {
                                     CT_SDIO_STACK_VERSION_CODE, /* Version: version code of the SDIO stack */
                                     {0},                        /* SDList */
                                     ("sdio_mem"),               /* pName: name of registering driver */
                                     NU_SDIO_FILE_MAX_DEVICES,   /* MaxDevices: maximum number of devices supported by this function */
                                     0,                          /* NumDevices: number of devices supported by this function */
                                     PIds,                       /* pIds: null terminated table of supported devices*/
                                     NU_SDIO_FDR_Imp_Probe,      /* pProbe: New device inserted */
                                     NU_SDIO_FDR_Imp_Remove,     /* pRemove: Device removed (NULL if not a hot-plug capable driver) */
                                     NULL,                       /* pSuspend */
                                     NULL,                       /* pResume */
                                     NULL,                       /* pWake */
                                     &SD_Tgt_Info,               /* pContext */
                                   };

static VOID NU_SDIO_FDR_Set_IO_Param(SDDEVICE * pDev, SDREQUEST *sdrequest,UINT8* buffer,
                      UINT32 blockno, UINT16 nblocks);

#if (NU_SDIO_FILE_MANUAL_CMD12_13 == NU_TRUE)
static STATUS NU_SDIO_FDR_Imp_StopTrans(SDDEVICE *device,UINT32 count,UINT32 reading);
#endif

static STATUS NU_SDIO_FDR_Imp_Chk_State(SDDEVICE *device, UINT32 shift, UINT32 mask, UINT32 cstate, UINT32 timeout);

/***********************************************************************
*
*   FUNCTION
*
*       nu_os_drvr_sdio_func_sd_init
*
*   DESCRIPTION
*
*       Store SD data to arrays and register the SD component(s).
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       NU_SDIO_FDR_Imp_Init
*
*   INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
VOID nu_os_drvr_sdio_func_sd_init (const CHAR * key, INT startstop)
{
    if ((key != NU_NULL) && (startstop == RUNLEVEL_START))
    {
        /* Save the config path in the target info */
        strncpy(SD_Tgt_Info.config_path, key, sizeof(SD_Tgt_Info.config_path));

        /* Initialize the SDIO layer. */
        (VOID)NU_SDIO_FDR_Imp_Init();
    }
}

/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_Init
*
* DESCRIPTION
*
*       This function is called when FILE function driver is
*       initialized. It registers Function Driver context with
*       SDIO stack.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       STATUS status                  - NU_SUCCESS
*                                           - SDIO_STATUS_ERROR
*
*************************************************************************/
STATUS NU_SDIO_FDR_Imp_Init(VOID)
{
    STATUS sdio_status;
    STATUS status = SDIO_STATUS_ERROR;

    /* Register with bus driver core */
    sdio_status = SDIO_RegisterFunction(&Sdio_File_Drv_Context);

    DBG_ASSERT(SDIO_SUCCESS(sdio_status));

    if (SDIO_SUCCESS(sdio_status))
    {
        status = NU_SUCCESS;
    }

    return(status);
}

/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_Close
*
* DESCRIPTION
*
*       This function is called when function driver unloads.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_SDIO_FDR_Imp_Close(VOID)
{
    /* Unregister with bus driver core */
    SDIO_UnregisterFunction(&Sdio_File_Drv_Context);
}

/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_Probe
*
* DESCRIPTION
*
*       This function is called when card (Device) inserted.
*       It verifies the card compatibility and media.*
*
* INPUTS
*
*       SDFUNCTION *function                - Function pointer
*       SDDEVICE   *handle                  - Device handle
*
* OUTPUTS
*
*       BOOLEAN    ret_val                  - 0 Card (device) is accepted.
*                                           - 1 Card (device) is rejected.
*
*************************************************************************/
BOOLEAN NU_SDIO_FDR_Imp_Probe(SDFUNCTION *function, SDDEVICE *handle)
{

    STATUS              status = NU_SUCCESS;
    SD_PHYS_DISK_CTRL   pdcb;
    UINT8               *pcardCSD;
    UINT32              c_size;
    UINT32              *val;
    UINT32              index;

    /* Clear disk control structure block. */
    memset(&pdcb, (UINT8)0, sizeof(SD_PHYS_DISK_CTRL));

    pdcb.p_SDDevice = handle;
    pdcb.p_SDFunction = function;
    
    /* Check if bus mode is not spi. */
    if (!(SDDEVICE_IS_BUSMODE_SPI(handle)))
    {
        /* Get CSD data starting location, the CSD data starts from 1 index. */
        pcardCSD = (SDDEVICE_GET_CARDCSD(handle) + 1) ;
    }
    else
    {
        /* Get CSD data starting location */
        pcardCSD = (SDDEVICE_GET_CARDCSD(handle)) ;
    }

    val = (UINT32*)pcardCSD;

    /* Check if bus mode is not spi. */
    if (!(SDDEVICE_IS_BUSMODE_SPI(handle)))
    {
        /* Reverse the data as the data received is not arranged. */
        for(index = 0; index <4; index++)
        {
            val[index] = ((val[index] & 0xFF000000) >> 24)   |
                         ((val[index] & 0x00FF0000) >> 8)    |
                         ((val[index] & 0x0000FF00) << 8)    |
                         ((val[index] & 0x000000FF) << 24);
        }
    }

    /* If we got an empty CSD, exit */
    if (*val == 0x0)
    {
        status = NUF_SD_CSD_STRUCTURE_ERROR;
    }
    else
    {
        pdcb.media_type = GET_CARD_TYPE(SDDEVICE_GET_CARD_FLAGS(handle));

        pdcb.media_wp = SDDEVICE_IS_CARD_WP_ON(handle);

        pdcb.rca = (UINT16)SDDEVICE_GET_CARD_RCA(handle);
       
        /* Check if bus mode is spi. */
        if (SDDEVICE_IS_BUSMODE_SPI(handle))
        {
            /* Save CSD parameters, assume SD card and version 1.0 of the spec. */
            pdcb.structure_ver = (pcardCSD[15] & CSD_STRUCTURE) >> 6;

            /* Maximum data transfer rate(TRAN_SPEED). */
            pdcb.tran_speed = pcardCSD[12];

            /* Maximum read data block length(READ_BL_LEN). */
            pdcb.read_bl_len = pcardCSD[10] & CSD_READ_BL_LEN;

            /* Device size(C_SIZE). */
            c_size = ((pcardCSD[9] & CSD_C_SIZE_HGH) << 10) |
                                    (pcardCSD[8] << 2) |
                                    ((pcardCSD[7] & CSD_C_SIZE_LOW) >> 6);

            /* Device size multiplier(C_SIZE_MULT). */
            pdcb.c_size_mult = (UINT8)(((pcardCSD[6] & CSD_C_SIZE_MULT_HGH) << 1) |
                                           ((pcardCSD[5] & CSD_C_SIZE_LOW) >> 7));
        }
        else
        {
            /* Save CSD parameters, assume SD card and version 1.0 of the spec. */
            pdcb.structure_ver = (pcardCSD[0] & CSD_STRUCTURE) >> 6;

            /* Maximum data transfer rate(TRAN_SPEED). */
            pdcb.tran_speed = pcardCSD[3];

            /* Maximum read data block length(READ_BL_LEN). */
            pdcb.read_bl_len = pcardCSD[5] & CSD_READ_BL_LEN;

            /* Device size(C_SIZE). */
            c_size = ((pcardCSD[6] & CSD_C_SIZE_HGH) << 10) |
                                    (pcardCSD[7] << 2) |
                                    ((pcardCSD[8] & CSD_C_SIZE_LOW) >> 6);

            /* Device size multiplier(C_SIZE_MULT). */
            pdcb.c_size_mult = (UINT8)(((pcardCSD[9] & CSD_C_SIZE_MULT_HGH) << 1) |
                                           ((pcardCSD[10] & CSD_C_SIZE_LOW) >> 7));

        }
        
        pdcb.card_cap = c_size + 1;
        pdcb.card_cap *= 1<<(pdcb.c_size_mult + 2);

        /* If large read block (=> 1024) SD card, adjust for standard 512 byte block. */
        pdcb.card_cap *= (1 << pdcb.read_bl_len) / NU_SDIO_FILE_STD_BLKSIZE;

        if (pdcb.media_type == CARD_SD)
        {
            /* Initialize high capacity support flag */
            handle->DeviceInfo.AsSDMMCInfo.hcs = NU_FALSE;

            /* SD Card Physical Specification Version */
            if (pdcb.structure_ver == SD_CSD_STR_VERSION_2_0)
            {
                /* Set high capacity support flag */
                handle->DeviceInfo.AsSDMMCInfo.hcs = NU_TRUE;

                if (SDDEVICE_IS_BUSMODE_SPI(handle))
                {
                    /* Device size(C_SIZE). */
                    c_size = (  (pcardCSD[8] & 0x3F) << 16) |
                                (pcardCSD[7] << 8)          |
                                 pcardCSD[6];
                }
                else
                {
                    /* Device size(C_SIZE). */
                    c_size = (  (pcardCSD[7] & 0x3F) << 16) |
                                (pcardCSD[8] << 8)          |
                                 pcardCSD[9];
                }

                /* Card capacity in sectors. */
                pdcb.card_cap = (c_size + 1) * 1024;

                /* No multiplier in this version. */
                pdcb.c_size_mult = 0;
            }
            else if (pdcb.structure_ver != SD_CSD_STR_VERSION_1_0)
            {
                /* Invalid SD Card Physical Specification Version. */
                status = NUF_SD_CSD_STRUCTURE_ERROR;
            }
        }

        /* If we got invalid card info, exit */
        if (c_size == 0x0)
        {
            status = NUF_SD_CSD_STRUCTURE_ERROR;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Register this device/card with Device Manager. Nucleus File mounts the device
           and searches for logical drives on disk. */
        status = SD_Register(&pdcb);
    }

    /* Change Nucleus status  to SDIO Stack status. */
    if (status == NU_SUCCESS)
    {
        /* This will enable SDIO Stack to know that
           this device is accepted by Function Driver. */
        return 1;
    }
    else
    {
        /* This will enable SDIO Stack to know that
           this device is rejected by Function Driver. */
        return 0;
    }
}

/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_Remove
*
* DESCRIPTION
*
*       This function is called when card (Device) removed.
*
* INPUTS
*
*       SDFUNCTION *function                - Function pointer
*       SDDEVICE   *handle                  - Device handle
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_SDIO_FDR_Imp_Remove(SDFUNCTION *function, SDDEVICE *handle)
{
    SD_INSTANCE_HANDLE *inst_handle;

    /* Retrieve a pointer to the instance handle from the
       device handler */
    inst_handle = (SD_INSTANCE_HANDLE *) handle->inst_handle;

    /* Unregister the device */
    (VOID)SD_Unregister (inst_handle->dev_id);
}

/*************************************************************************
* FUNCTION
*
*       sd_set_io_params
*
* DESCRIPTION
*
*       This function sets the Read/Write commands parameter.
*       (Read   : Read single block, Read multiple block command.
*        Write  : Write single block, Write multiple block command.)
*
* INPUTS
*
*       SDDEVICE  *PDev                     - Pointer device control block
*       SDREQUEST *sdrequest                - SDREQUEST structure.
*       UINT8     *buffer                   - Destination address for the data
*       UINT32    blockno                   - Block number to read/write
*       UINT16    nblocks                   - Number of blocks
*                                             (legal range: 1-256)
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID NU_SDIO_FDR_Set_IO_Param(SDDEVICE * pDev, SDREQUEST *sdrequest,UINT8* buffer,
                      UINT32 blockno, UINT16 nblocks)
{
    /* If card does not support high capacity */
    if (pDev->DeviceInfo.AsSDMMCInfo.hcs == NU_FALSE)
    {
        /* Convert sector address to byte address. */
        sdrequest->Argument = blockno << 9;
    }
    else
    {
        /* Send sector address. */
        sdrequest->Argument = blockno;
    }

}
/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_RDWR
*
* DESCRIPTION
*
*       This function Reads or Writes data to an MMC/SD memory card
*       relative to the physical start of the card's memory.
*
*       Command list:
*           CMD17  - READ_SINGLE_BLOCK
*           CMD18  - READ_MULTIPLE_BLOCK
*           CMD24  - WRITE_BLOCK
*           CMD25  - WRITE_MULTIPLE_BLOCK
* INPUTS
*
*       SDDEVICE *device                    - Pointer to device structure
*       UINT32   offset                     - Sector/byte offset to read/write
*       UINT8    *buffer                    - Read/Write buffer
*       UINT32   count                      - Number of sectors
*                                             (legal range : 1-256)
*       UINT32   request                    - YES : Read  NO : Write
*
* OUTPUTS
*
*       NU_SUCCESS
*       SDIO stack API error codes.
*
*************************************************************************/
#if (NU_SDIO_SUPPORT_MULTIBLOCK == NU_TRUE)
STATUS NU_SDIO_FDR_Imp_RDWR(SDDEVICE *device,
                            UINT32 offset,
                            UINT8 *buffer,
                            UINT32 count,
                            UINT32 request)
{
#else
STATUS NU_SDIO_FDR_Imp_RDWR(SDDEVICE *device,
                            UINT32 in_offset,
                            UINT8 *in_buffer,
                            UINT32 in_count,
                            UINT32 request)
{
    UINT32 offset;
    UINT8 *buffer;
    UINT16 count;
#endif
    SDREQUEST *sdrequest;
    SDDEVICE  *handle;
    STATUS    status = NU_SUCCESS;

    sdrequest = SDDeviceAllocRequest(device);
    if(sdrequest != NU_NULL)
    {
        /* Block size. */
        sdrequest->BlockLen = NU_SDIO_FILE_STD_BLKSIZE;

        /* Set parameters for synchronous transactions. */
        sdrequest->Flags = SDREQ_FLAGS_RESP_R1 |
                           SDREQ_FLAGS_DATA_TRANS;

        sdrequest->pCompleteContext = NU_NULL;
        sdrequest->pCompletion      = NU_NULL;

#if (NU_SDIO_FILE_MANUAL_CMD12_13 != NU_TRUE)
        /* Issue CMD13: card status until card programming is complete. */
        sdrequest->Flags |= SDREQ_FLAGS_AUTO_TRANSFER_STATUS;
#endif

        /* Set the command we want to execute. */
#if (NU_SDIO_SUPPORT_MULTIBLOCK != NU_TRUE)
        buffer = (UINT8 *)in_buffer;
        offset = in_offset;
        count = NU_SDIO_SINGLE_BLOCK;
        do
        {
#endif
            /* Set the buffer where we want to read data to or write data from. */
            sdrequest->pDataBuffer = buffer;

            /* Single block or multiple block request command. */
            if (count > NU_SDIO_SINGLE_BLOCK)
            {
                /* reading? */
                if (request)
                {
                    /* READ_MULTIPLE_BLOCK. */
                    sdrequest->Command = SD_CMD18;
                }
                else
                {
                    /* WRITE_MULTIPLE_BLOCK. */
                    sdrequest->Command = SD_CMD25;

                    sdrequest->Flags |= SDREQ_FLAGS_DATA_WRITE;
                }
                sdrequest->BlockCount = count;

#if (NU_SDIO_FILE_MANUAL_CMD12_13 != NU_TRUE)
                /* Issue CMD12: stop transmission after data transfer. */
                sdrequest->Flags |= SDREQ_FLAGS_AUTO_CMD12;
                sdrequest->RetryCount = 3;
#endif
            }
            else
            {
                /* Read/Write single block. */
                if (request)
                {
                    /* READ_SINGLE_BLOCK. */
                    sdrequest->Command = SD_CMD17;
                }
                else
                {
                    /* WRITE_SINGLE_BLOCK. */
                    sdrequest->Command = SD_CMD24;

                    sdrequest->Flags |= SDREQ_FLAGS_DATA_WRITE;

                }

                /* Set the number of block to a single block. */
                sdrequest->BlockCount = NU_SDIO_SINGLE_BLOCK;
            }

            /* Set up the additional read/write parameters. */
            NU_SDIO_FDR_Set_IO_Param(device,sdrequest,(UINT8*)buffer, offset, count);

            /* Get the SDIO handle (SDDEVICE instance). */
            handle = device;

            /* Post a Request. */
            status = SDDEVICE_CALL_REQUEST_FUNC(handle, sdrequest);

            /*  
                Some cards may require long and unpredictable times to write a block of data. After receiving a block of 
                data and completing the CRC check, the card will begin writing and hold the DAT0 line low if its write 
                buffer is full and unable to accept new data from a new WRITE_BLOCK command. The host may poll 
                the status of the card with a SEND_STATUS command (CMD13) at any time, and the card will respond 
                with its status. The status bit READY_FOR_DATA indicates whether the card can accept new data or 
                whether the write process is still in progress) 
            */
            if (status == NU_SUCCESS)
            {
                do 
                {
                    /* Check MMC/SD card whether it is ready to accept new data/buffer or write process is still in progress */
                    status = NU_SDIO_FDR_Imp_Chk_State(device, SD_CS_READY_FOR_DATA_SHIFT, SD_CS_READY_FOR_DATA_MASK, SD_CS_READY_FOR_DATA, 100);
                }    while (status != NU_SUCCESS);
            }
            
#if (NU_SDIO_FILE_MANUAL_CMD12_13 == NU_TRUE)
            /* Check to see if operation successful for multiblock transfer? */
            if( status == NU_SUCCESS && count > NU_SDIO_SINGLE_BLOCK)
            {
                /* Check if bus mode is not spi. */
                if (!(SDDEVICE_IS_BUSMODE_SPI(device)))
                {
                    if(request)
                    {
                        status = NU_SDIO_FDR_Imp_Chk_State(device, SD_CS_CURRENT_STATE_SHIFT, SD_CS_CURRENT_STATE_MASK, SD_CS_STATE_DATA, 100);
                    }
                    else
                    {
                        status = NU_SDIO_FDR_Imp_Chk_State(device, SD_CS_CURRENT_STATE_SHIFT, SD_CS_CURRENT_STATE_MASK, SD_CS_STATE_RCV, 100);
                    }
                }

                /* Check to see if previous operation successful. */
                if (status == NU_SUCCESS)
                {
                    status = NU_SDIO_FDR_Imp_StopTrans(device,count,request);
                }
            }

            /* Check to see if previous operation successful. */
            if (status == NU_SUCCESS)
            {
                /* Wait for MMC/SD CSTATE_TRAN state. */
                NU_SDIO_FDR_Imp_Chk_State(device, SD_CS_CURRENT_STATE_SHIFT, SD_CS_CURRENT_STATE_MASK, SD_CS_STATE_TRANS, 100);
            }
#endif

#if (NU_SDIO_SUPPORT_MULTIBLOCK != NU_TRUE)
            buffer += NU_SDIO_FILE_STD_BLKSIZE;
            offset++;
        }
        while ((status == NU_SUCCESS) && --in_count);
#endif

        /* Free the request */
        SDDeviceFreeRequest(device, sdrequest);
    }
    else
    {
        /* allocation of request failed! */
        status = SDIO_STATUS_NO_RESOURCES;
    }

    return status;
}

#if (NU_SDIO_FILE_MANUAL_CMD12_13 == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_StopTrans
*
* DESCRIPTION
*
*       This function is called after multiblock data transfer. It issues
*       CMD12.
*
* INPUTS
*
*       SDDEVICE *device                    - Pointer to device structure
*       UINT32   count                      - Number of sectors
*                                             (legal range : 1-256)
*       UINT32   reading                    - YES : Read  NO : Write
*
* OUTPUTS
*
*       STATUS   status                     - NU_SUCCESS
*                                           - SDIO stack API error codes.
*
*************************************************************************/
static STATUS NU_SDIO_FDR_Imp_StopTrans(SDDEVICE *device,UINT32 count, UINT32 reading)
{
    SDREQUEST *sdrequest;
    STATUS    status;

    sdrequest = SDDeviceAllocRequest(device);
    if(sdrequest != NU_NULL)
    {
        /* Block size. */
        sdrequest->BlockLen = NU_SDIO_FILE_STD_BLKSIZE;

        sdrequest->Command = SD_CMD12;

        sdrequest->Argument = 0;
        sdrequest->Flags = SDREQ_FLAGS_RESP_R1B ;

        /* Set parameters for synchronous transactions. */
        sdrequest->pCompleteContext = NU_NULL;
        sdrequest->pCompletion      = NU_NULL;
        sdrequest->RetryCount = 3;

        /* Post a Request. */
        status = SDDEVICE_CALL_REQUEST_FUNC(device, sdrequest);

        /* Free the request */
        SDDeviceFreeRequest(device, sdrequest);
    }
    else
    {
        /* allocation of request failed! */
        status = SDIO_STATUS_NO_RESOURCES;
    }

    return status;

}

#endif

/*************************************************************************
* FUNCTION
*
*       NU_SDIO_FDR_Imp_Chk_State
*
* DESCRIPTION
*
*       This function is called after data transfer. It checks for card
*       state by issuing CMD13 and returns on either a valid match of
*       retries timeout.
*
* INPUTS
*
*       SDDEVICE *device                    - Pointer to device structure
*       UINT32   cstate                     - State value to be matched
*                                             (legal range : 1-256)
*       UINT32   timeout                    - Timeout value in OS ticks
*       UINT32   shift                      - Shift position 
*       UINT32   mask                       - Mask value 
*
* OUTPUTS
*
*       STATUS   status                     - NU_SUCCESS
*                                           - SDIO stack API error codes.
*
*************************************************************************/
static STATUS NU_SDIO_FDR_Imp_Chk_State(SDDEVICE *device, UINT32 shift, UINT32 mask, UINT32 cstate, UINT32 timeout)
{
    SDREQUEST *sdrequest;
    STATUS    status, cmdstatus;
    UINT32    current_tick;
    UINT32    rollover;
    UINT32    response;

    sdrequest = SDDeviceAllocRequest(device);
    if(sdrequest != NU_NULL)
    {
        current_tick = NU_Retrieve_Clock();

        if ((UINT32) (current_tick + timeout) < current_tick)
        {
            rollover = ((~0x0 - current_tick) + 1);
            current_tick = timeout;
        }
        else
        {
            rollover = 0UL;
            current_tick += timeout;
        }

        /* Initialize status. */
        status = NUF_SD_CSTATE_ERROR;
        /* Status check loop. */
        while (status == NUF_SD_CSTATE_ERROR)
        {
            /* Check the timeout. */
            if ((NU_Retrieve_Clock() + rollover) > current_tick)
            {
                break;
            }

            /* Set request parameters. */
            sdrequest->Command = SD_CMD13;

            /* Check if bus mode is spi. */
            if (SDDEVICE_IS_BUSMODE_SPI(device))
            {
                sdrequest->Flags = SDREQ_FLAGS_RESP_R2;
                sdrequest->Argument = 0;
            }
            else
            {
                sdrequest->Flags = SDREQ_FLAGS_RESP_R1;
                sdrequest->Argument |= SDDEVICE_GET_CARD_RCA(device) << 16;
            }

            /* Set parameters for synchronous transactions. */
            sdrequest->pCompleteContext = NU_NULL;
            sdrequest->pCompletion      = NU_NULL;

            /* Post a Request. */
            cmdstatus = SDDEVICE_CALL_REQUEST_FUNC(device, sdrequest);

            /* Check to see if previous operation successful. */
            if (cmdstatus == NU_SUCCESS)
            {
                /* Check if bus mode is spi. */
                if (SDDEVICE_IS_BUSMODE_SPI(device))
                {
                    /* Check card status if any error occurred. */
                    response = SD_R1_GET_CARD_STATUS(sdrequest->Response);

                    if ((response & SD_CS_TRANSFER_ERRORS) == 0)
                    {
                        /* If no error occurred return success status.*/
                        status = NU_SUCCESS;
                    }
                }
                else
                {
                    response = SD_R1_GET_CARD_STATUS(sdrequest->Response);
                    response = ((response >> shift) & mask);
                    
                    if (response == cstate)
                    {
                        /* Current state is required state.*/
                        status = NU_SUCCESS;
                    }
                }
            }
            else
            {
                /* Command error. */
                status = cmdstatus;
            }
        }

        /* Free the request */
        SDDeviceFreeRequest(device, sdrequest);
    }
    else
    {
        /* allocation of request failed! */
        status = SDIO_STATUS_NO_RESOURCES;
    }

    return status;
}


