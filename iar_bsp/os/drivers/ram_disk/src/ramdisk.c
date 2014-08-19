/*************************************************************************/
/*                                                                       */
/*               Copyright 2010 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ramdisk.c
*
* COMPONENT
*
*       RAM Disk Driver
*
* DESCRIPTION
*
*       Provides a configurable ram drive capability using pages allocated
*       from a Nucleus memory partition.
*
* FUNCTIONS
*
*       nu_os_drvr_fat_rd_init
*       RD_DVM_Read
*       RD_DVM_Write
*       rd_allocate_space
*       rd_alloc_page
*       rd_free_page
*
*******************************************************************/

#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "services/nu_services.h"
#include        "storage/nu_storage.h"
#include        "drivers/nu_drivers.h"


#if (!RAMDISK_FROMPOOL)
/* We will use malloc */
#include        <stdio.h>
#include        <stdlib.h>
#endif  /* !RAMDISK_FROMPOOL */

STATUS  rd_allocate_space(RD_INSTANCE_HANDLE *inst_handle);

static UINT8 *rd_alloc_page(RD_INSTANCE_HANDLE *inst_handle);
static VOID   rd_free_page(UINT8 *page);

/***********************************************************************
*
*   FUNCTION
*
*       nu_os_drvr_fat_rd_init
*
*   DESCRIPTION
*
*       Store Ram Disk data to arrays and register the Ram Disk component(s).
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       RDA_Register
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
VOID nu_os_drvr_fat_rd_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID dev_id;
    RD_INSTANCE_HANDLE *inst_handle;
    NU_MEMORY_POOL     *sys_pool_ptr;
    STATUS status = NU_SUCCESS;


    if (key != NU_NULL)
    {
        /* If we are starting the device */
        if (startstop == RUNLEVEL_START)
        {
            /* Get system memory pool pointer */
            status =  NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the RD instance handle */
                status = NU_Allocate_Memory (sys_pool_ptr, (VOID**)&inst_handle,
                                             sizeof (RD_INSTANCE_HANDLE), NU_NO_SUSPEND);

                /* If we allocated the memory */
                if (status == NU_SUCCESS)
                {
                    /* Zero out allocated space */
                    (VOID)memset (inst_handle, 0, sizeof (RD_INSTANCE_HANDLE));

                    /* Save the config path in the instance handle */
                    strncpy(inst_handle->config_path, key, sizeof(inst_handle->config_path));

                    /* Get the device name and save it in the instance handle */
                    status = REG_Get_String_Value (key, "/tgt_settings/dev_name",
                                                   inst_handle->dev_name, sizeof(inst_handle->dev_name));

                    if (status == NU_SUCCESS)
                    {
                        /* Populate the rest of the instance handle */
                        inst_handle->NUF_RAMDISK_PARTITION.pm_id = 0;
                        inst_handle->rd_pages[0]                 = 0;
                        inst_handle->rd_opencount                = 0;
                        inst_handle->rd_pool_init_completed      = 0;
                        inst_handle->rd_logicals_opened          = 0;
                        inst_handle->rd_sector_size              = 512; /* The size (in bytes) of a physical sector */

                        /* Create the Ram disk "device" */
                        status = rd_allocate_space(inst_handle);

                        if (status == NU_SUCCESS)
                        {
                            /* Call the component registration function */
                            (VOID)RD_Dv_Register(key, inst_handle);

                            /* Save dev_id in instance structure */
                            dev_id = (inst_handle->dev_id);
                        }
                    }

                    if (status != NU_SUCCESS)
                    {
                        /* Deallocate instance memrory */
                        (VOID)NU_Deallocate_Memory((VOID*)inst_handle);
                    }
                }
            }
        }
        else if (startstop == RUNLEVEL_STOP)
        {
            /* If we are stopping an already started device */
            if (dev_id >= 0)
            {
                /* Call the component unregistration function */
                (VOID)RD_Dv_Unregister(key, startstop, dev_id);
            }
        }
    }

}


/*************************************************************************
*
*   FUNCTION
*
*       RD_Read
*
*   DESCRIPTION
*
*       This function reads from the RD hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       INT          numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - byte offset from zero to start read
*       INT          *bytes_read_ptr        - Pointer to return number of bytes read
*
*   OUTPUTS
*
*       STATUS       sts                    - NU_SUCCESS or error code
*
*************************************************************************/
STATUS RD_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte,
                    OFFSET_T byte_offset, UINT32 *bytes_read_ptr)
{
    STATUS             sts = NU_SUCCESS;
    UINT8              *p;
    UINT16             page_number;
    UINT16             byte_number;
    INT16              i;
    UINT8              *pbuffer;
    RD_SESSION_HANDLE  *sess_handle = (RD_SESSION_HANDLE*)session_handle;
    RD_INSTANCE_HANDLE *inst_handle = (RD_INSTANCE_HANDLE*)(sess_handle->inst_info);
    UINT16             sec_count = (numbyte / (inst_handle->rd_sector_size));
    UINT32             sector_offset = (byte_offset / (inst_handle->rd_sector_size));


    /* Check the buffer pointer */
    if ((buffer == NU_NULL)  || (bytes_read_ptr == NU_NULL))
    {
        /* Return error status */
        sts = NUF_IO_ERROR;
    }
    else
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        PMI_DEV_HANDLE pmi_dev = (inst_handle->pmi_dev);

        /* Check current state of the device. If the device is off, suspend on an 
           event until the device power state changes to ON */
        if ((PMI_STATE_GET(pmi_dev) == RD_OFF))
        {
            /* Wait until the device is available for a read operation */
            PMI_WAIT_CYCLE(pmi_dev, sts);
        }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /* Device state is now ON, so we can read */
        if (sts == NU_SUCCESS)
        {
            *bytes_read_ptr = 0;
            pbuffer = (UINT8 *)buffer;

            while (sec_count)
            {
                /* Get the page number */
                page_number = (UINT16)(sector_offset / RAMDISK_PAGE_SIZE);
        
                /* Check. This shouldn't happen */
                if ( (page_number >= NUM_RAMDISK_PAGES) || !inst_handle->rd_pages[page_number] )
                {
                    sts = NUF_IO_ERROR;
                    break;
                }
        
                /* Get the offset */
                byte_number = (UINT16)((sector_offset % RAMDISK_PAGE_SIZE) * inst_handle->rd_sector_size);
                p = inst_handle->rd_pages[page_number] + byte_number;
                
                for (i = 0; i < inst_handle->rd_sector_size; i++)
                {
                    *pbuffer++ = *p++;
                }
        
                sec_count--;
                sector_offset++;
            }
           
            /* Set the number of bytes read */
            *bytes_read_ptr = numbyte - (sec_count * inst_handle->rd_sector_size);
        }
    }

    return (sts);
}


/*************************************************************************
*
*   FUNCTION
*
*       RD_Write
*
*   DESCRIPTION
*
*       This function writes to the RD hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       INT          numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - byte offset from zero to start write
*       INT          *bytes_written_ptr     - Pointer to return number of bytes written
*
*   OUTPUTS
*
*       INT          bytes_written          - Number of bytes written
*
*************************************************************************/
STATUS RD_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte,
                     OFFSET_T byte_offset, UINT32 *bytes_written_ptr)
{
    STATUS             sts = NU_SUCCESS;
    UINT8              *p;
    UINT16             page_number;
    UINT16             byte_number;
    INT16              i;
    UINT8              *pbuffer;
    RD_SESSION_HANDLE  *sess_handle = (RD_SESSION_HANDLE*)session_handle;
    RD_INSTANCE_HANDLE *inst_handle = (RD_INSTANCE_HANDLE*)(sess_handle->inst_info);
    UINT16             sec_count = numbyte / inst_handle->rd_sector_size;
    UINT32             sector_offset = byte_offset / inst_handle->rd_sector_size;
   

    /* Check the buffer pointer */
    if ((buffer == NU_NULL)  || (bytes_written_ptr == NU_NULL))
    {
        /* Return error status */
        sts = NUF_IO_ERROR;
    }
    else
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        PMI_DEV_HANDLE pmi_dev = (inst_handle->pmi_dev);

        /* Check current state of the device. If the device is off, suspend on an
            event until the device power state changes to ON */
        if ((PMI_STATE_GET(pmi_dev) == RD_OFF))
        {
            /* Wait until the device is available for a read operation */
            PMI_WAIT_CYCLE(pmi_dev, sts);
        }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
    
        /* Device state is now ON, so we can transmit */
        if (sts == NU_SUCCESS)
        {
            /* We don't use disk handle. You could have multiple drives if wanted */
            *bytes_written_ptr = 0;
            pbuffer = (UINT8 *)buffer;

            while (sec_count)
            {
                /* Get the page number */
                page_number = (UINT16)(sector_offset / RAMDISK_PAGE_SIZE);
        
                /* Check. This shouldn't happen */
                if ( (page_number >= NUM_RAMDISK_PAGES) || !inst_handle->rd_pages[page_number] )
                {
                    sts = NUF_IO_ERROR;
                    break;
                }
        
                /* Get the offset */
                byte_number = (UINT16)((sector_offset % RAMDISK_PAGE_SIZE) * inst_handle->rd_sector_size);
                p = inst_handle->rd_pages[page_number] + byte_number;
                
                for (i = 0; i < inst_handle->rd_sector_size; i++)
                {
                    *p++ = *pbuffer++;
                }
        
                sec_count--;
                sector_offset++;
            }
            
            /* Set the number of bytes written */
            *bytes_written_ptr = numbyte - (sec_count * inst_handle->rd_sector_size);
        }
    }
   
    return (sts);
}


/************************************************************************
* FUNCTION
*
*       rd_allocate_space
*
* DESCRIPTION
*
*       This function prepares the RAM Disk for use by allocating the
*       pages necessary for usage.
*
* INPUTS
*
*       RD_INSTANCE_HANDLE *inst_handle     - Instance handle
*
* OUTPUTS
*
*       STATUS             status           - NU_SUCCESS  
*                                           - NU_NO_PARTITION 
*                                               Error returned from rd_alloc_page
*
*************************************************************************/
STATUS rd_allocate_space(RD_INSTANCE_HANDLE *inst_handle)
{
    UINT16      i;
    UINT16      j;
    STATUS      sts = NU_SUCCESS;

#if (RAMDISK_FROMPOOL)
    void *pointer;
    NU_MEMORY_POOL     *sys_pool_ptr;
#endif

    if (!inst_handle->rd_pool_init_completed)
    {
#if (RAMDISK_FROMPOOL)
        /* Get system memory pool pointer */
        sts =  NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (sts == NU_SUCCESS)
        {
            /*  Create the RAMDISK Partition. */
            sts = NU_Allocate_Memory(sys_pool_ptr, &pointer,
                                     (POOL_SIZE + ALLOC_SIZE), NU_NO_SUSPEND);
            if (sts == NU_SUCCESS)
            {
                /* Create the partition pool for RAMDISK */
                sts = NU_Create_Partition_Pool(&inst_handle->NUF_RAMDISK_PARTITION, "RAMDISK",
                                               pointer, POOL_SIZE,
                                               NUF_RAMDISK_PARTITION_SIZE,
                                               NU_FIFO);

                /* Return the memory if we failed to create the pool */
                if (sts != NU_SUCCESS)
                {
                    NU_Deallocate_Memory(pointer);
                }
            }
        }

#endif
        /* Update the status variable so we know the pool has already
           been allocated */
        if (sts == NU_SUCCESS)
        {
            inst_handle->rd_pool_init_completed = 1;
        }
    }

    if (sts == NU_SUCCESS)
    {
        /* If the ramdisk is not initialized, do it now */
        if (!inst_handle->rd_pages[0])
        {
            for (i = 0; i < NUM_RAMDISK_PAGES; i++)
            {
                inst_handle->rd_pages[i] = rd_alloc_page(inst_handle);

                if (!inst_handle->rd_pages[i])
                {
                    for (j = 0; j < i; j++)
                    {
                        rd_free_page(inst_handle->rd_pages[j]);
                        inst_handle->rd_pages[j] = 0;
                    }
                    return(NU_NO_PARTITION); /* Error returned from rd_alloc_page */
                }
            }

            /* clear the first sector so the disk will be re-formatted */
            pc_memfill(inst_handle->rd_pages[0], inst_handle->rd_sector_size, 0x00);
        }
    }

    return(sts);
}


/************************************************************************
* FUNCTION
*
*       rd_alloc_page
*
* DESCRIPTION
*
*       This function allocates one page of memory from a Nucleus
*       fixed partition.
*
* INPUTS
*
*       RD_INSTANCE_HANDLE *inst_handle     - Instance handle
*
* OUTPUTS
*
*       UINT8              *mem_addr        - Pointer to the page allocated.
*
*************************************************************************/
UINT8 *rd_alloc_page(RD_INSTANCE_HANDLE *inst_handle)
{
    STATUS  status;

#if (RAMDISK_FROMPOOL)
    UINT8   *mem_address;

    /*  Allocate a page.  */
    status = NU_Allocate_Partition(&inst_handle->NUF_RAMDISK_PARTITION,
                                   (VOID **)&mem_address,
                                    NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        return(mem_address);
    }
    else
    {
        return((UINT8 *)NU_NULL);
    }
#else  /*  not RAMDISK_FROMPOOL  */
    /* Using MALLOC instead of Allocate_Partition */
    UINT8   *mem_address;
    OPTION  preempt_status;

    /*  Don't let anyone interfere with the malloc.
     *  Anticipating DOS non- reentrancy.
     */
    preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);

    /*  Get the memory.  */
    mem_address = (UINT8 *)malloc(NUF_RAMDISK_PARTITION_SIZE);

    /*  Others can take over now since the malloc is complete. */
    NU_Change_Preemption(preempt_status);

    return(mem_address);
#endif /*  RAMDISK_FROMPOOL  */
}

/************************************************************************
* FUNCTION
*
*       rd_freec_page
*
* DESCRIPTION
*
*       This function deallocates one page of memory from a Nucleus
*       fixed partition.
*
* INPUTS
*
*       UINT8    *page                      - Pointer to memory to be
*                                             deallocated.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID   rd_free_page(UINT8 *page)
{
#if (RAMDISK_FROMPOOL)
    /*  If the input parameter is valid, deallocate.  */
    if (page)
    {
        NU_Deallocate_Partition((VOID *)page);
    }
#else
    /* Using free() instead of dealloc */
    OPTION  preempt_status;

    /*  Don't let anyone interfere with the malloc.
     *  Anticipating DOS non-reentrancy.
     */
    preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);

    /*  If the input is valid, deallocate the page.  */
    if (page)
    {
        free(page);
    }

    /*  Others can take over now since the malloc is complete. */
    NU_Change_Preemption(preempt_status);
#endif
}
