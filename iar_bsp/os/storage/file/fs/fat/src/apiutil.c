/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
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
*       apiutil.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Contains support code for user api level source code.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       pc_dskinit                          Mount a disk.
*       pc_idskclose                        Unmount a disk.
*       pc_fd2file                          Map a file descriptor to a
*                                            file structure.
*       pc_allocfile                        Allocate a file structure.
*       pc_freefile                         Release a file structure.
*       pc_free_all_fil                     Release all file structures
*                                            for a drive.
*       pc_log_base_2                       Calculate log2(N).
*       pc_get_cwd                          Determine cwd string from
*                                            current directory inode.
*       pc_upstat                           Copy directory entry info
*                                            to a user's stat buffer.
*************************************************************************/

#include        "storage/fat_defs.h"
#include        "storage/encod_defs.h"
#include        "storage/fd_defs.h"

extern PC_FILE      *mem_file_pool;         /* Memory file pool list.   */

#ifdef FAT_CACHE_TABLE
extern FAT_CS_TE    FAT_CACHE_TABLE[];      /* User-defined setting for
                                               FAT cache sizes per specific
                                               device. */
#endif
/************************************************************************
* FUNCTION
*
*       pc_dskinit
*
* DESCRIPTION
*
*       Given a valid drive number, read block zero and convert its
*       contents to File system drive information.
*
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Mount successful.
*       NUF_FATCORE                         Fat cache table too small.
*       NUF_NO_PARTITION                    No partition in disk.
*       NUF_FORMAT                          Disk is not formatted.
*       NUF_NO_MEMORY                       Can't allocate internal
*                                            buffer.
*       NUF_IO_ERROR                        Driver returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_dskinit(UINT16 dh)
{
STATUS      ret_stat;
UINT8       b[512];
DDRIVE      *pdr;
FATSWAP     *pfr;
UINT16      nblocks;
UINT8 FAR   *pdata;
UINT32      fatoffset;
INT         i;
UINT16      wvalue;
UINT32      ltemp;
FAT_CB      *fs_cb = NU_NULL;

    /* Convert disk handle to a drive number */
    ret_stat = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);
    if (ret_stat != NU_SUCCESS)
        return NUF_INTERNAL;

    /* Find the drive. */
    pdr = fs_cb->ddrive;
    if (pdr)
    {
        /* Get the size of FAT cache table */
        fs_cb->drive_fat_size = fat_get_cache_size(dh);
        if (fs_cb->drive_fat_size < 2)
        {
            /* There must be at least 2 blocks for caching */
            return(NUF_FATCORE);
        }

        /* Move FATSWAP pointer to local. */
        pfr = &pdr->fat_swap_structure;

        /* Set FAT buffer data map size. */
        if (fs_cb->drive_fat_size < 256)
        {
            pfr->data_map_size = 256;
        }
        else
        {
            pfr->data_map_size = fs_cb->drive_fat_size;
        }

        /* Allocate FAT buffer data map. */
        pfr->data_map =
            (UINT16 *)NUF_Alloc(sizeof(UINT16) * pfr->data_map_size);
        if (!pfr->data_map)
        {
            pc_report_error(PCERR_DRVALLOC);
            return(NUF_NO_MEMORY);
        }

        /* Allocate FAT BIT-map of blocks. */
        pfr->pdirty = (UINT8 *)NUF_Alloc(pfr->data_map_size >> 3 );
        if (!pfr->pdirty)
        {
            pc_report_error(PCERR_DRVALLOC);
            NU_Deallocate_Memory(pfr->data_map);
            return(NUF_NO_MEMORY);
        }

        /* Initialize FATSWAP structure. */
        for (i = 0; i < pfr->data_map_size; i++)
        {
            pfr->data_map[i] = 0;
            pfr->pdirty[i >> 3] = 0;
        }
        pfr->block_0_is_valid = 0;
        pfr->base_block = 0;
        pfr->n_blocks_used = 0;
        /* Set drive open count. */
        pdr->opencount = 1;
        pdr->dh = dh;

    }
    else
    {
        return(NUF_INTERNAL);
    }

    /* Grab the device driver. */
    PC_DRIVE_IO_ENTER(dh)

    /* Get 1 block starting at 0 from dh. */
    /* READ */
    if ( fs_dev_io_proc(dh, 0L, &b[0], (UINT16) 1, YES) != NU_SUCCESS )
    {
        PC_DRIVE_IO_EXIT(dh)
        pc_report_error(PCERR_INITREAD);
        NU_Deallocate_Memory(pfr->data_map);
        NU_Deallocate_Memory(pfr->pdirty);
        return(NUF_IO_ERROR);
    }
    /* Release the drive io locks. */
    PC_DRIVE_IO_EXIT(dh)

    /* Verify that we have a good dos formatted disk */
    SWAP16(&wvalue,(UINT16 *)&b[0x1fe]);
    if (wvalue != 0xAA55)
    {
        pc_report_error(PCERR_INITMEDI);
        pdr->opencount = 0;
        NU_Deallocate_Memory(pfr->data_map);
        NU_Deallocate_Memory(pfr->pdirty);
        return(NUF_FORMAT);
    }

    /* This drive is FAT file system */
    pdr->fs_type  = FAT_FILE_SYSTEM;

    /* Now load the structure from the buffer */
    /* OEMNAME. */
    NUF_Copybuff(&pdr->oemname[0],&b[3],8);
    /* Bytes per sector. */
    SWAP16((UINT16 *)&pdr->bytspsector, (UINT16 *)&b[0xb]);
    /* Sector per allocation. */
    pdr->secpalloc = b[0xd];
    /* Check this value to prevent a divide by zero error */
    if(pdr->secpalloc == 0)
    {
        pc_report_error(PCERR_INITDEV);
        pdr->opencount = 0;
        NU_Deallocate_Memory(pfr->data_map);
        NU_Deallocate_Memory(pfr->pdirty);      
        return(NUF_FORMAT);
    }
    /* Reserved sectors. */
    SWAP16((UINT16 *)&pdr->fatblock,(UINT16 *)&b[0xe]);
    /* Number of FATs. */
    pdr->numfats = b[0x10];
    /* Root dir entries. */
    SWAP16((UINT16 *)&pdr->numroot,(UINT16 *)&b[0x11]);
    /* Total # sectors. */
    SWAP16(&wvalue,(UINT16 *)&b[0x13]);
    pdr->numsecs = wvalue;
    /* Media descriptor. */
    pdr->mediadesc = b[0x15];
    /* FAT size. */
    SWAP16(&wvalue,(UINT16 *)&b[0x16]);
    pdr->secpfat = wvalue;
    /* Sectors per track. */
    SWAP16((UINT16 *)&pdr->secptrk,(UINT16 *)&b[0x18]);
    /* Number of heads. */
    SWAP16((UINT16 *)&pdr->numhead,(UINT16 *)&b[0x1a]);
    /* Hidden sectors. */
    SWAP32((UINT32 *)&pdr->numhide,(UINT32 *)&b[0x1c]);
    /* Huge Sectors. */
    SWAP32((UINT32 *)&pdr->bignumsecs,(UINT32 *)&b[0x20]);


    /* Check if running on a DOS (4.0) huge partition */
    /* If traditional total # sectors is zero, use value in extended BPB */
    if (pdr->numsecs == 0L)
        pdr->numsecs = pdr->bignumsecs;


    /* FSINFO
        Set to 0 when the free clusters count is not necessarily correct.
        Set to 1 when the free clusters count is correct. */
	/* NUF_FSINFO_DISABLE */
    pdr->valid_fsinfo = 0;

    /* Check root dir entries. */
    if ( (pdr->numroot != 0) && (pdr->secpfat != 0L) )
    {   /* FAT12/FAT16 */

        /* The first block of the root is just past the fat copies */
        pdr->rootblock = pdr->fatblock + pdr->secpfat * pdr->numfats;

        /* The first block of the cluster area is just past the root */
        /* Round up if we have to */
        pdr->firstclblock = pdr->rootblock +
                            (pdr->numroot + INOPBLOCK - 1)/INOPBLOCK;

        /*  Calculate the largest index in the file allocation table.
            Total # block in the cluster area)/Blockpercluster =='s total
            Number of clusters. Entries 0 & 1 are reserved so the highest
            valid fat index is 1 + total # clusters.
            Note: Max cluster number  = Number of clusters + 1 */
        pdr->maxfindex = (UINT32)
                (1 + (pdr->numsecs - pdr->firstclblock)/pdr->secpalloc);
        
        /* The use of 4086 and 65526 is intentional. This isn't an off by one 
          error as mentioned in the FAT specification. Internally maxfindex is
          used as max number of FAT table entries. The max number of FAT table 
          entries is equal to the total cluster count plus 1
          (pdr->maxfindex = CountofCluster + 1). The reason that 
          CountofCluster + 1 is equal to total number of usable FAT table entries
          is because the usable FAT table entries start at entry 2. FAT table entries
          0 and 1 are reserved by the FAT specification, */

        /* if < 4085 clusters then 12 bit else 16. */
        if (pdr->maxfindex < 4086)
            pdr->fasize = 3/* NU_FAT12_SIG */;
        /* < 65525 clusters 16bit fat. */
        else if (pdr->maxfindex < 65526)
            pdr->fasize = 4/* NU_FAT16_SIG */;
        else
        {
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);
            return(NUF_FORMAT);
        }

        pdr->phys_num = b[0x24];
        pdr->xtbootsig = b[0x26];

        /* Unique number per volume (4.0). */
        SWAP32((UINT32 *)&pdr->volid,(UINT32 *)&b[0x27]);

        /* Volume label (4.0). */
        NUF_Copybuff(&pdr->vollabel[0],&b[0x2b],11);

         /* set the pointer to where to look for free clusters to the contiguous
            area. On the first call to write this will hunt for the real free
            blocks. */
        pdr->free_contig_pointer = 2L;

         /* Keep track of how much free space is on the drive. (This will
            speed up pc_free()) when calculating free space */
        pdr->free_clusters_count = 0L;
    }
    else
    {   /* FAT32 */

        /* FAT size. */
        SWAP32((UINT32 *)&pdr->bigsecpfat,(UINT32 *)&b[0x24]);
        /* FAT flag. */
        SWAP16((UINT16 *)&pdr->fat_flag,(UINT16 *)&b[0x28]);
        /* File system version. */
        SWAP16((UINT16 *)&pdr->file_version,(UINT16 *)&b[0x2a]);

        /* Nucleus FILE can only operate on FAT32 ver0 */
        if ( pdr->file_version != 0 )
        {
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);          
            return(NUF_INTERNAL);
        }


        /* Root dir start cluster. */
        SWAP32((UINT32 *)&pdr->rootdirstartcl,(UINT32 *)&b[0x2c]);
        /* Unique number per volume (4.0). */
        SWAP32((UINT32 *)&pdr->volid,(UINT32 *)&b[0x43]);
        /* Volume label (4.0). */
        NUF_Copybuff(&pdr->vollabel[0],&b[0x47],11);

        /* NU_FAT32_SIG */
        pdr->fasize = 8;

        /* The first block of the root is given. */
        pdr->rootblock = pdr->fatblock + pdr->bigsecpfat * pdr->numfats;
        if (pdr->rootdirstartcl != 2L)
        {
            if (pdr->rootdirstartcl > 2L)
            {
                pdr->rootblock += ( (pdr->rootdirstartcl - 2L) *
                                        pdr->secpalloc );
            }
            else
            {
                pdr->opencount = 0;
		        NU_Deallocate_Memory(pfr->data_map);
		        NU_Deallocate_Memory(pfr->pdirty);
                return(NUF_FORMAT);
            }
        }

        /* The first block of the cluster area is the root dir. */
        pdr->firstclblock = pdr->rootblock;
        /*  Calculate the largest index in the file allocation table.
            Total # block in the cluster area)/Blockpercluster =='s total
            Number of clusters. Entries 0 & 1 are reserved so the highest
            valid fat index is 1 + total # clusters.
            Note: Max cluster number  = Number of clusters + 1 */
        pdr->maxfindex = (UINT32)
                (1 + (pdr->numsecs - pdr->firstclblock)/pdr->secpalloc);

        /* Copy FAT size. */
        pdr->secpfat = pdr->bigsecpfat;

        /* FSINFO block. */
        SWAP16((UINT16 *)&pdr->fsinfo,(UINT16 *)&b[0x30]);

        /* Grab the device driver. */
        PC_DRIVE_IO_ENTER(dh)

        /* Read the File System INFOrmation. */
        /* READ */
        if ( fs_dev_io_proc(dh, (UINT32) pdr->fsinfo,
                                            &b[0], (UINT16) 1, YES) != NU_SUCCESS  )
        {
            /* Release the drive io locks. */
            PC_DRIVE_IO_EXIT(dh)
            pc_report_error(PCERR_INITREAD);
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);        
            return(NUF_IO_ERROR);
        }
        /* Release the drive io locks. */
        PC_DRIVE_IO_EXIT(dh)


        /* Check the signature of the file system information sector.*/
        if ( ((b[0] == 'R') && (b[1] == 'R') && (b[2] == 'a') && (b[3] == 'A') &&
             (b[0x1e4] == 'r') && (b[0x1e5] == 'r') && (b[0x1e6] == 'A') && (b[0x1e7] == 'a')) ||
             ((b[0x1fe] == 0x55) && (b[0x1ff] == 0xAA)) )
        {
            /* The count of free clusters on the drive.
                Note: Set to -1(0xffffffff) when the count is unknown. */
            SWAP32((UINT32 *)&pdr->free_clusters_count,(UINT32 *)&b[0x1e8]);

            /* Check the number of total free cluster  */
            if ( (pdr->free_clusters_count == (UINT32)0xffffffff) ||
                 (pdr->free_clusters_count > pdr->maxfindex) )
                pdr->free_clusters_count = 0L;
            else
                /* The free cluster count is known. */
                pdr->valid_fsinfo = 1/* NUF_FSINFO_ENABLE */;

            /* The cluster number of the cluster that was most recently allocated. */
            SWAP32((UINT32 *)&pdr->free_contig_pointer,(UINT32 *)&b[0x1ec]);

            /* Check the number of next free cluster  */
            if (pdr->free_contig_pointer > pdr->maxfindex)
                pdr->free_contig_pointer = 2L;
        }
        else
        {
            /* FSINFO Error case. */
            pdr->free_clusters_count = 0L;
            pdr->free_contig_pointer = 2L;
        }
    }


    if (pdr->fasize == 3)
    {
        if (pdr->maxfindex > 0x0ff6L)
        {
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);
            return(NUF_FORMAT);
        }
    }
    else if (pdr->fasize == 4)
    {
        if (pdr->maxfindex > 0xfff6L)
        {
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);
            return(NUF_FORMAT);
        }
    }
    else    /* FAT32 */
    {
        if (pdr->maxfindex > 0x0ffffff6L)
        {
            pdr->opencount = 0;
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);
            return(NUF_FORMAT);
        }
    }

    /* Remember how many blocks we allocated. */
    pdr->fat_swap_structure.n_blocks_total = fs_cb->drive_fat_size;
    if (pdr->fat_swap_structure.n_blocks_total >  (INT)pdr->secpfat)
    {
        pdr->fat_swap_structure.n_blocks_total = (INT)pdr->secpfat;
    }

    /* Allocate FAT cache buffer */
    pdr->fat_swap_structure.data_array =
                    (UINT8 FAR *)NUF_Alloc(pdr->fat_swap_structure.n_blocks_total << 9 );

    if (!pdr->fat_swap_structure.data_array)
    {
        pdr->opencount = 0;
        NU_Deallocate_Memory(pfr->data_map);
        NU_Deallocate_Memory(pfr->pdirty);
        return(NUF_NO_MEMORY);
    }

    /* Set FAT buffer flag. */
    if (pdr->fat_swap_structure.n_blocks_total == (INT)pdr->secpfat)
        pdr->use_fatbuf = 0;
    else
        pdr->use_fatbuf = 1;




#ifdef DEBUG0

        DEBUG_PRINT ("Oem NAME  %8s\n",pdr->oemname);
        DEBUG_PRINT ("Bytspsec  %d\n",pdr->bytspsector);
        DEBUG_PRINT ("secpallc  %d\n",pdr->secpalloc);
        DEBUG_PRINT ("secres    %d\n",pdr->fatblock);
        DEBUG_PRINT ("numfat    %d\n",pdr->numfats);
        DEBUG_PRINT ("numrot    %d\n",pdr->numroot);
        DEBUG_PRINT ("numsec    %d\n",pdr->numsecs);
        DEBUG_PRINT ("mediac    %d\n",pdr->mediadesc);
        DEBUG_PRINT ("secfat    %d\n",pdr->secpfat);
        DEBUG_PRINT ("sectrk    %d\n",pdr->secptrk);
        DEBUG_PRINT ("numhed    %d\n",pdr->numhead);
        DEBUG_PRINT ("numhide   %d\n",pdr->numhide);
#endif

    pdr->bytespcluster = 512 * pdr->secpalloc;

    /* bits to mask in to calculate byte offset in cluster from file pointer.
        AND file pointer with this to get byte offset in cluster a shift right
        9 to get block offset in cluster */
    pdr->byte_into_cl_mask = pdr->bytespcluster;
    pdr->byte_into_cl_mask -= 1L;

    /* save away log of sectors per alloc */
    pdr->log2_secpalloc = pc_log_base_2((UINT16)pdr->secpalloc);

    /* Number of maximum file clusters */
    pdr->maxfsize_cluster = MAXFILE_SIZE >> pdr->log2_secpalloc;

    /* Initialize the fat management code */
    if (pdr->use_fatbuf)
    {
        /* Swap in item 0. ( read the first page of the FAT) */
        ret_stat = pc_pfswap(&pdata, pdr, (UINT32)0, NO);
        if (ret_stat != NU_SUCCESS)
        {
            pc_report_error(PCERR_FATREAD);
            NU_Deallocate_Memory((VOID *)pdr->fat_swap_structure.data_array);
	        NU_Deallocate_Memory(pfr->data_map);
	        NU_Deallocate_Memory(pfr->pdirty);
            pdr->opencount = 0;
            return(ret_stat);
        }
    }
    else
    {
        /* No fat swapping. We use drive.fat_swap_structure elements
           to implement an in memory caching scheme. data_map[255] is used
           to indicate which blocks are dirty and data_array points to
           the buffer to hold the in memory fat */

        ltemp = pdr->secpfat;
        pdata = pdr->fat_swap_structure.data_array;
        fatoffset = pdr->fatblock;
        while (ltemp)
        {
            if (ltemp > MAXSECTORS)
                nblocks = MAXSECTORS;
            else
                nblocks = (UINT16) ltemp;

            /* Grab the device driver. */
            PC_DRIVE_IO_ENTER(dh)

            /* The dirty blocks table data_map[255] was zeroed when we
               zeroed the drive structure. */
            /* Now read the fat. */
            /* READ */
            if ( fs_dev_io_proc(dh, (UINT32)fatoffset,
                                                pdata, nblocks, YES) != NU_SUCCESS )
            {
                /* Release the drive io locks. */
                PC_DRIVE_IO_EXIT(dh)
                pc_report_error(PCERR_FATREAD);
                NU_Deallocate_Memory((VOID *)pdr->fat_swap_structure.data_array);
                pdr->opencount = 0;
		        NU_Deallocate_Memory(pfr->data_map);
		        NU_Deallocate_Memory(pfr->pdirty);
                return(NUF_IO_ERROR);
            }
            /* Release the drive io locks. */
            PC_DRIVE_IO_EXIT(dh)

            ltemp -= nblocks;
            fatoffset += nblocks;
            pdata += (nblocks << 9);
        }
    }

    return(NU_SUCCESS);
}


/************************************************************************
* FUNCTION
*
*       pc_idskclose
*
* DESCRIPTION
*
*       Given a valid disk handle. Flush the file allocation table and
*       purge any buffers or objects associated with the drive.
*
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Unmount successful.
*       NUF_NOT_OPENED                      Drive not opened.
*       NUF_IO_ERROR                        Driver returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_idskclose(UINT16 dh)
{
DDRIVE      *pdr;
STATUS      ret_val;
FAT_CB      *fs_cb;

    /* Get the drive structure using the fat control block. */
    ret_val = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);

    /* Find the drive. */
    if (ret_val == NU_SUCCESS)
        pdr = fs_cb->ddrive;
    else
        pdr = NU_NULL;

    /* Check drive number */
    if (pdr && pdr->opencount)
    {
        /* Grab exclusive access to the FAT. */
        PC_FAT_ENTER(dh)
        /* Flush the file allocation table. */
        ret_val = pc_flushfat(pdr);
        /* Release non-exclusive use of FAT. */
        PC_FAT_EXIT(dh)

        if (ret_val == NU_SUCCESS)
        {
            /* Release the drive if opencount == 1 */
            ret_val = pc_dskfree(dh, NO);

        }
    }
    else
    {
        /* Could not get the DDRIVE structure for this drive,
           or open count is zero. */
        ret_val = NUF_NOT_OPENED;
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_fd2file
*
* DESCRIPTION
*
*       Map a file descriptor to a file structure. Return null if the
*       file is not open. If an error has occurred on the file return
*       NU_NULL unless allow_err is true.
*
*
* INPUTS
*
*       fd                                  File descriptor
*
* OUTPUTS
*
*       File representation.
*       NU_NULL                             Invalid FD.
*
*************************************************************************/
PC_FILE *pc_fd2file(INT fd)
{
PC_FILE     *pfile;
PC_FILE     *ret_val = NU_NULL;

    /* Is this descriptor value in correct range ? */
    if (0 <= fd && fd <= gl_VFS_MAX_OPEN_FILES)
    {
        /* Get PFILE memory pool pointer */
        pfile = mem_file_pool+fd;
        if (pfile && !pfile->is_free)
        {
            ret_val = pfile;
        }
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_allocfile
*
* DESCRIPTION
*
*       Allocate PC_FILE structure.
*
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       File descriptor.
*       -1                                  Over the CFG_NU_OS_STOR_FILE_VFS_MAX_OPEN_FILES.
*
*************************************************************************/
INT pc_allocfile(VOID)
{
PC_FILE     *pfile;
INT         i, retval = -1;


    /* Get PC_FILE memory pool pointer. */
    pfile = mem_file_pool;

    for (i = 0; i < gl_VFS_MAX_OPEN_FILES; i++, pfile++)
    {
        /* Check free flag. */
        if (pfile->is_free)
        {
            /* Initialize file descriptor. */
            NUF_Memfill(pfile, sizeof(PC_FILE), (UINT8) 0);

            /* Set retval = file descriptor number, break. */
            retval = i;
            break;
        }
    }
    return(retval);
}


/************************************************************************
* FUNCTION
*
*       pc_freefile
*
* DESCRIPTION
*
*       Free all core associated with a file descriptor and make the
*       descriptor available for future calls to allocfile.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_freefile(INT fd)
{
PC_FILE     *pfile;


    /* Get file descriptor. */
    if ((pfile = pc_fd2file(fd)) != NU_NULL)
    {
        /* Release a DROBJ. */
        if (pfile->pobj)
            pc_freeobj(pfile->pobj);
        /* Set free flag. */
        pfile->is_free = YES;
    }
}


/************************************************************************
* FUNCTION
*
*       pc_free_all_fil
*
* DESCRIPTION
*
*       Release all file descriptors associated with a drive and free
*       up all core associated with the files called by dsk_close
*
*
* INPUTS
*
*       pdrive                              Drive management structure
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_free_all_fil(DDRIVE *pdrive)
{
PC_FILE     *pfile;
INT         i;


    for (i = 0; i < gl_VFS_MAX_OPEN_FILES; i++)
    {
        /* Get file descriptor. */
        pfile = pc_fd2file(i);
        if (pfile != NU_NULL)
        {
            if ( (pfile->pobj) && (pfile->pobj->pdrive == pdrive) )
            {
                /* print a debug message since in normal operation
                   all files should be close closed before closing the drive */
                pc_report_error(PCERR_FSTOPEN);
                /* Release a file structure. */
                pc_freefile(i);
            }
        }
    }
}


/************************************************************************
* FUNCTION
*
*       pc_log_base_2
*
* DESCRIPTION
*
*       Calculate log2(N).
*
*
* INPUTS
*
*       n                                   Sector per cluster
*
* OUTPUTS
*
*       log2(N)
*
*************************************************************************/
INT16 pc_log_base_2(UINT16 n)                                   /*__fn__*/
{
INT16       log;
INT16       ret_val = 0;

    log = 0;
    if ( n > 1 )
    {
        while (n)
        {
            log += 1;
            n >>= 1;
        }
        ret_val = (INT16)(log-1);
    }
    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_get_cwd
*
* DESCRIPTION
*
*       Return the current directory inode for the drive represented
*       by ddrive.
*
*
* INPUTS
*
*       pdr                                 Drive management structure.
*
* OUTPUTS
*
*       DROBJ pointer
*       Return NU_NULL on error.
*
*************************************************************************/
DROBJ *pc_get_cwd(DDRIVE *pdrive)
{
STATUS      sts;
DROBJ       *pcwd;
DROBJ       *pobj;
DROBJ       *ret_val;



    /* Get the current working directory. */
    sts = fsv_get_vnode(pdrive->dh, (void **)&pcwd);
    if (sts != NU_SUCCESS)
        pcwd = NU_NULL;

    if (pcwd)
    {
        /* Allocate an empty DROBJ and FINODE. */
        pobj = pc_allocobj();
        if (!pobj)
            return(NU_NULL);
        /* Free the inode that comes with allocobj */
        pc_freei(pobj->finode);
        NUF_Copybuff(pobj, pcwd, sizeof(DROBJ));
        pobj->finode->opencount += 1;

        ret_val = pobj;
    }
    else    /* If no cwd is set error */
    {
        ret_val = NU_NULL;
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_upstat
*
* DESCRIPTION
*
*       Given a pointer to a DSTAT structure that contains a pointer to
*       an initialized DROBJ structure, load the public elements of
*       DSTAT with name, filesize, date of modification, et al.
*
*
* INPUTS
*
*       statobj                             Caller's buffer to put file
*                                            info.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_upstat(FAT_DSTAT *statobj)
{
DROBJ       *pobj;
FINODE      *pi;
INT         ii, jj;
UINT8       *sfn_ptr;
UINT8       *lfn_ptr;
UINT8       *ext_ptr;


    /* Move FINODE pointer to local. */
    pobj = statobj->pobj;
    pi = pobj->finode;

    /* Copy file name and file extension. */
    NUF_COPYBUFF(statobj->sfname, pi->fname, 8);
    sfn_ptr = (UINT8*)&(statobj->sfname);
    /* Get last character */
    for(ii = 0; ii < 8 && *sfn_ptr != 0x00; ++ii)
    {
        NUF_NEXT_CHAR(sfn_ptr);
    }
    /* Set last character to NULL. */
    *(sfn_ptr) = '\0';
    
    NUF_COPYBUFF(statobj->fext, pi->fext, 3);
    sfn_ptr = (UINT8*)&(statobj->fext);
    for(ii = 0; ii < 3 && *sfn_ptr != 0x00; ++ii)
    {
        NUF_NEXT_CHAR(sfn_ptr);
    }
    /* Set last character to NULL. */
    *(sfn_ptr) = '\0';
    
    /* Initialize long file name. */
    statobj->lfname[0] = '\0';

    /* Set file attributes. */
    statobj->fattribute = pi->fattribute;
    if (statobj->pobj->linfo.lnament) /* long file name */
    {
        pc_cre_longname((UINT8 *)statobj->lfname, &statobj->pobj->linfo);
    }
    else  /* Copy short file name to long name field */
    {
        /* No copy on a volume label */
        if(!(statobj->fattribute & AVOLUME))
        {
            sfn_ptr = (UINT8*)&statobj->sfname[0];
            lfn_ptr = (UINT8*)&statobj->lfname[0];
            ii = 0;
            while( *sfn_ptr != '\0' && *sfn_ptr != ' ')
            {

                NUF_COPYBUFF(lfn_ptr,sfn_ptr,1);
                NUF_NEXT_CHAR(sfn_ptr);
                NUF_NEXT_CHAR(lfn_ptr);
                ii++;
            }

            if( statobj->fext[0] != ' ' && statobj->fext[0] != 0)
            {
                *lfn_ptr = (UINT8)'.';
                NUF_NEXT_CHAR(lfn_ptr);
                ii++;

                ext_ptr = (UINT8*)&statobj->fext[0];

                for( jj = 0; jj<3; jj++)
                {
                    if( *ext_ptr == ' ')
                    {
                        *lfn_ptr = (UINT8)0;
                    }
                    else
                    {
                        NUF_COPYBUFF(lfn_ptr,ext_ptr,1);
                    }
                    NUF_NEXT_CHAR(lfn_ptr);
                    NUF_NEXT_CHAR(ext_ptr);
                    ii++;
                }

                *lfn_ptr = (UINT8)0;
            }
            else
            {
                *lfn_ptr = (UINT8)0;
            }
        }
    }


    statobj->fcrcmsec = pi->fcrcmsec;
    statobj->fcrtime = pi->fcrtime;
    statobj->fcrdate = pi->fcrdate;

    statobj->faccdate = pi->faccdate;

    statobj->fuptime = pi->fuptime;
    statobj->fupdate = pi->fupdate;

    statobj->fsize = pi->fsize;
    
    /* Set cluster for data file. */
    statobj->fclusterhigh = (UINT16) (pi->fcluster >> 16);
    statobj->fclusterlow  = (UINT16) (pi->fcluster & 0x0000ffff);
    
}

/************************************************************************
* FUNCTION
*
*       fat_get_cache_size
*
* DESCRIPTION
*
*       Given a valid drive handle, look up the size of the FAT cache.
*       If a specific value wasn't specified, a default value is
*       returned.
*
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       INT                                 Size of FAT cache
*
*************************************************************************/
INT fat_get_cache_size(UINT16 dh)
{
INT     cache_size = FAT_DEFAULT_CACHE_SIZE;
CHAR    devname[FS_DEV_MAX_DEVNAME];
#ifdef FAT_CACHE_TABLE
UINT16  i;
#endif


    /* Resolve the device name for the disk handle */
    if (NU_SUCCESS == fsdh_dh_to_devname(dh, devname))
    {
#ifdef FAT_CACHE_TABLE
        /* Look in the FAT cache size table for a device name match */
        i = 0;
        while(FAT_CACHE_TABLE[i].size != FAT_CACHE_TABLE_END_SIZE)
        {
            if (0 == NUF_Strncmp(FAT_CACHE_TABLE[i].devname, devname))
            {
                /* Found the first matching device name entry */
                cache_size = FAT_CACHE_TABLE[i].size;
                break;
            }
            ++i;
        }
#endif
    }

    return (cache_size);

}
