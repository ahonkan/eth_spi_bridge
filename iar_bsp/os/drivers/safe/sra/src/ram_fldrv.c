/*************************************************************************/
/*                                                                       */
/*               Copyright 2007 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/************************************************************************
* FILE NAME
*
*       ram_fldrv.c
*
* COMPONENT
*
*       Nucleus Safe Ramdisk Driver
*
* DESCRIPTION
*
*       RAM disk source file. 
*
* DATA STRUCTURES
*
*       None.
*           
* FUNCTIONS
*           
*       StoreFat                            Not Used in RAM disk.                                           
*       StoreSector                         Store a sector into ram.
*       GetSector                           Get a sector from ram.
*       Format                              Format a volume.
*       fs_mount_ramdrive                   Mount function for this 
*                                           driver.
*       fs_getmem_ramdrive
*       SRA_Register                        Registration function
*       nu_os_drvr_safe_sra_init            Init function
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"


/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern NU_MEMORY_POOL       System_Memory;

/****************************************************************************
 *
 * StoreFat
 *
 * Not used in ramdrive
 *
 ***************************************************************************/

static int StoreFat(FS_VOLUMEINFO *vi) 
{
    if (vi) return 0;
    return 1;
}

/****************************************************************************
 *
 * StoreSector
 *
 * Called from higher level to store one sector into ram
 *
 * INPUTS
 *
 * vi - volumeinfo where the free sectors are
 * file - internal file pointer where last sector is
 * data - data pointer where data is
 * len - length of data need to be stored
 *
 * RETURNS
 *
 * 0 - if successfully stored
 * other if any error
 *
 ***************************************************************************/

static int StoreSector(FS_VOLUMEINFO *vi,FS_FILEINT *file,void *data,long len) 
{
    unsigned short a;

    if (!len) return 0; /* nothing need to store */

    for (a=0; a<vi->maxsectornum; a++) 
    {
        if (fsm_checksectorfree(vi,a)) 
        {

            fsm_addsectorchain(vi,file,a);

            {
                char *ptr=vi->ramdrivedata;
                ptr+=(long)a * vi->sectorsize;
                memcpy (ptr,data,len);
            }

            return 0;
        }
    }

    return 1; /* no space */
}

/****************************************************************************
 *
 * GetSector
 *
 * Get sector data back from ram
 *
 * INPUTS
 *
 * vi - volumeinfo which volume to belong it
 * secnum - sector number 
 * data - where to store data
 * offset - relative offset in sector to start reading
 * datalen - length of retrieved data
 *
 * RETURNS
 *
 * 0 - if successfully restored
 * other if any error
 *
 ***************************************************************************/

static int GetSector (const FS_VOLUMEINFO *vi,long secnum,void *data,long offset,long datalen)
{
    char *ptr=vi->ramdrivedata;

    ptr+=secnum * vi->sectorsize;

    memcpy (data,ptr+offset,datalen);

    return 0;
}

/****************************************************************************
 *
 * Format
 *
 * Format a volume
 *
 * INPUTS
 *
 * vi - volumeinfo which volume needed to be formatted
 *
 * RETURNS
 *
 * 0 - if successfully formatted
 * other if any error
 *
 ***************************************************************************/

static int Format(FS_VOLUMEINFO *vi)
{
    long a;
    unsigned int b;

    /* reset FAT */
    for (a=0; a<vi->maxsectornum; a++)
    {
        vi->_fat[a]=FS_FAT_FREE;
        vi->fatmirror[a]=FS_FAT_FREE;
    }

    /* reset directory */
    for (b=0; b<vi->maxdirentry; b++)
    {
        vi->direntries[b].attr=0;
    }

    return 0;
}

/****************************************************************************
 *
 * fs_mount_ramdrive
 *
 * Mount function for this driver, called from fs_mount function
 *
 * INPUTS
 *
 * vd - volumedescriptor of the volume
 * phyfunc - device specific function
 *
 * RETURNS
 *
 * 0 - if successfully mounted
 * other if any error
 *
 ***************************************************************************/

int fs_mount_ramdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc)
{
    FS_VOLUMEINFO *vi=vd->vi;

    /* check phy function */
    if (phyfunc)
    {
        vd->state=FS_VOL_DRVERROR;   /* no physical function needed */
        return 1;
    }

    /* set dll functions */
    vd->storefat       =StoreFat;
    vd->storesector    =StoreSector;
    vd->getsector      =GetSector;
    vd->format         =Format;

    /* alloc write buffer  */
    if (fsm_setsectorsize(vi,CFG_NU_OS_DRVR_SAFE_SRA_RAM_SECSIZE))
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* alloc files */
    if (fsm_setmaxfile(vi,CFG_NU_OS_DRVR_SAFE_SRA_MAXFILE))
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* alloc blockdata information temporary wr/rd buffers */
    vi->rdbuffer=(char*)fsm_allocdata(vi,vi->sectorsize);
    if (!vi->rdbuffer)
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* calculate maximum number of direntry and alloc directory */
    vi->maxdirentry=(unsigned int)(vi->freemem / vi->sectorsize);  /* calculate directory entry numbers */
    vi->direntries=(FS_DIRENTRY *)fsm_allocdata(vi,(long)vi->maxdirentry*sizeof(FS_DIRENTRY));
    if (!vi->direntries)
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* alloc FAT */
    vi->maxsectornum= (vi->freemem - ( (vi->freemem / vi->sectorsize)*(long)sizeof(unsigned long) )) / vi->sectorsize;
    vi->_fat=(unsigned short *)fsm_allocdata(vi,vi->maxsectornum*sizeof(unsigned short));    /* allocate FAT */
    if (!vi->_fat)
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* alloc mirror FAT */
    vi->fatmirror=(unsigned short *)fsm_allocdata(vi,vi->maxsectornum*sizeof(unsigned short));   /* allocate FAT */
    if (!vi->fatmirror)
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* alloc sectors */
    vi->ramdrivedata=(char*)fsm_allocdata(vi,vi->maxsectornum * vi->sectorsize);
    if (!vi->ramdrivedata)
    {
        vd->state=FS_VOL_NOMEMORY;
        return 1;
    }

    /* RAM drive always empty at start up */
    if (Format(vi))
    {
        vd->state=FS_VOL_NOTFORMATTED;
        return 1;
    }

    /* set working state */
    vd->state=FS_VOL_OK;

    return 0;
}


/****************************************************************************
 *
 * end of ramdrv_s.c
 *
 ***************************************************************************/
/************************************************************************
* FUNCTION
*
*       fs_getmem_ramdrive
*
* DESCRIPTION
*
*       Used to determine how much memory an instance of the ramdisk driver
*       will require.
*
* INPUTS
*
*       phyfunc                             Device specific function.       
*
* OUTPUTS
*
*       Number of bytes an instance of the ramdisk driver will require.
*       
*************************************************************************/
long fs_getmem_ramdrive(FS_PHYGETID phyfunc)
{
    
    return(SAFE_RAMDISK_SIZE);
}


/***********************************************************************
*
*   FUNCTION
*
*       nu_os_drvr_safe_sra_init 
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the component
*       and calls the component driver registration function.
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       SRA_Register
*       SAFE_Unregister
*
*   INPUTS
*
*       CHAR     *key                       - Key
*       INT      startstop                  - Start or Stop flag
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_os_drvr_safe_sra_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID        sra_dev_id;
    STATUS                  status = NU_NOT_REGISTERED;
    SAFE_INSTANCE_HANDLE    *safe_tgt;

    if (startstop)
    {
        if (key != NU_NULL)
        {
            /* Allocate memory for the SAFE_TGT structure */
            status = NU_Allocate_Memory (&System_Memory, (VOID*)&safe_tgt, sizeof (SAFE_INSTANCE_HANDLE), NU_NO_SUSPEND);

            /* Check if the previous operation was successful */
            if (status == NU_SUCCESS)
            {
                /* Zero out allocated space */
                (VOID)memset (safe_tgt, 0, sizeof (SAFE_INSTANCE_HANDLE));

                /* Fill in the function pointers */
                safe_tgt->mount_func  = &fs_mount_ramdrive;
                safe_tgt->phy_func    = (FS_PHYGETID)NU_NULL;
                safe_tgt->getmem_func = &fs_getmem_ramdrive;

                /* Create the SAFE device and call its setup function */
                SAFE_Init(safe_tgt, key);

                /* Call SAFE_Register function and expect a returned device ID */
                SAFE_Dv_Register(key,
                                 startstop,
                                 &sra_dev_id,
                                 safe_tgt);
            }
            else
            {
                (VOID)NU_Deallocate_Memory(safe_tgt);
            }

        } 
    }
    else
    {
        /* If we are stopping an already started device */
        if (sra_dev_id >= 0)
        {
            /* Call the component unregistration function */
            (VOID)Safe_Dv_Unregister(sra_dev_id);
        }
    }
}

 
