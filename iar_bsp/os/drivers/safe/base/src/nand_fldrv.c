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
*       nand_fldrv.c
*
* COMPONENT
*
*       Nucleus Safe NAND Driver
*
* DESCRIPTION
*
*       NAND generic flash driver.
*
* DATA STRUCTURES
*
*       None.
*           
* FUNCTIONS
*
*       RestoreChanges                      Restore changes from cache
*                                           area.
*       AllocNewCache                       Allocate new cache area in FAT.
*       StoreDir                            Store a directory.
*       GetDir                              Get last stored directory.
*       SwapFatBitsPrevBitsBlk              Swap new(mirror) block with
*                                           previous block.
*       CopySector                          Copy sector.
*       CopyBB                              Copy bad block information to
*                                           new block.
*       SectorMerge                         Merge not-copied sectors to 
*                                           latest fatbitsblock.
*       StoreFat                            Store fat into flash device.
*       CheckSignature                      Check signature with 1 bit
*                                           error included.
*       GetFat                              Get last stored fat from flash.
*       AllocBlock                          Allocate a new free block.
*       StoreSector                         Store one sector into flash.
*       GetSector                           Get a sector from flash.
*       Format                              Format a volume.
*       fs_mount_nandflashdrive             Mount function for driver.
*       fs_getmem_nflashdrive               Returns number of bytes 
*                                           required for driver.
*
*************************************************************************/

#include        "drivers/nand_fldrv.h"

/****************************************************************************
 *
 * Driver version
 *
 ***************************************************************************/

#define F_DRVVERSION	0x0101

/****************************************************************************
 *
 * Static definitions
 *
 ***************************************************************************/

#define MAXFILE 4    /* number of file could be opened at once */

/****************************************************************************
 *
 * Some definitions for signature data
 *
 ***************************************************************************/

#define SDATA_FAT ((long)0x92ab52edUL)  /* definition for fat signature */
#define SDATA_NR  ((long)0x78987453UL)	/* definition for normal data signature */

/****************************************************************************
 *
 * RestoreChanges
 *
 * restoring changes from cache area, this function is called in mounting
 *
 * INPUTS
 *
 * vi - volumeinfo
 *
 * RETURNS
 *
 * 0 - successfully restored or not restoring needed
 * other if any error happened
 *
 ***************************************************************************/

static int RestoreChanges(FS_VOLUMEINFO *vi) 
{
	if (vi->flash->cacheddescsize) 
	{
		long a,found=-1;

		_fsm_cachereset(vi);

		for (a=0; a<vi->maxsectornum; a++) 
		{
			if (vi->_fat[a]==FS_FAT_CHBLK) 
			{
				found=a;
				break;
			}
		}
		if (found==-1) return 1; /* wrcache not found */

		vi->cache.currdescnum=vi->_blockindex[a/vi->flash->sectorperblock]; /* set physical block value */

		for (a=0; a<vi->flash->cachedpagenum; a++) 
		{
			int ret=fsm_readflash(vi,(unsigned char*)(vi->cache.desc)+a*vi->flash->cachedpagesize,	/* read changes */
				vi->flash->blockstart+vi->cache.currdescnum,a*vi->flash->cachedpagesize,vi->flash->cachedpagesize);

			if (ret==-1) 
			{
#if	SAFE_16BIT_CHAR
				fsm_memset((unsigned char*)(vi->cache.desc)+a*vi->flash->cachedpagesize,
					0x00ffff,vi->flash->cachedpagesize); /* DSP! */
#else
				fsm_memset((unsigned char*)(vi->cache.desc)+a*vi->flash->cachedpagesize,
					0x00ff,vi->flash->cachedpagesize); /* DSP! */
#endif
			}
			else if (ret) 
			{
				vi->cache.status |= FS_STCACHE_FULL; /* no more could be added */
				return 1; /*  cannot be read  */
			}
		}

		for (;;) 
		{
			int ret=_fsm_chacheupdatechanges(vi);
			if (ret==2) break; /*  eof found  */
			if (ret==3) 
			{
				vi->cache.status |= FS_STCACHE_FULL; /* no more could be added */
				return 1; /* invalid */
			}
			if (ret) 
			{
				vi->cache.status |= FS_STCACHE_FULL; /* no more could be added */
				break;	/* any error on last */
			}
		}
	}
	return 0; /* simple ok */
}

/****************************************************************************
 *
 * AllocNewCache
 *
 * function for allocating new cache area in the FAT
 *
 * INPUTS
 *
 * vi - volumeinfo
 *
 * RETURNS
 *
 * 0 - if successfully
 * other if any error
 *
 ***************************************************************************/

static int AllocNewCache(FS_VOLUMEINFO *vi) 
{
	long a;
	unsigned short sector;
	unsigned short oldblock,freeblock;

	if (!vi->flash->cacheddescsize) return 0;	/* not needed */

	if (!fsm_findfreeblock(vi,&sector)) 
	{
		return 1; /* no free block to store */
	}

	oldblock=vi->_blockindex[sector/vi->flash->sectorperblock];

	/* remove previous entry */
	for (a=0; a<vi->maxsectornum; a++) 
	{  
		if (vi->_fat[a]==FS_FAT_CHBLK) 
		{
			_fsm_cacheaddsptr(vi,&vi->_fat[a],FS_FAT_DISCARD);
		}
	}


	for (a=0; a<vi->flash->sectorperblock; a++) 
	{
		_fsm_cacheaddsptr(vi,&vi->_fat[sector+a],FS_FAT_CHBLK);	   /* set as cache entry */
	}

	for (;;) 
	{
		fsm_wearleveling(vi);

		freeblock=vi->_blockindex[vi->flash->maxblock];  /* get free block! */

		_fsm_cacheaddlptr(vi,&vi->_wearlevel[freeblock]);

		if (!vi->flash->EraseFlash (vi->flash->blockstart+freeblock)) 
		{
			/* swap free and old block (old is free now) */
			_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->flash->maxblock],oldblock);  /* swap free and old block (old is free now) */
			_fsm_cacheaddsptr(vi,&vi->_blockindex[sector/vi->flash->sectorperblock],freeblock);

			vi->cache.currdescnum=freeblock;
			return 0;
		}

		{
			unsigned short badsector;
			if (!fsm_findfreeblock(vi,&badsector)) return 1; /* no more free sector */
			fsm_swapbadblock(vi,badsector);
		}
	}

}


/****************************************************************************
 *
 * StoreDir
 *
 * Storing directory, if separated directory entry is used
 *
 * INPUTS
 *
 * vi - volume info which directory needs to be store
 *
 * RETURNS
 *
 * 0 - if successfully stored
 * other if any error
 *
 ***************************************************************************/

static int StoreDir(FS_VOLUMEINFO *vi) 
{
	long a;
	unsigned short sector;
	unsigned short oldblock,freeblock;
	long dcou;

	if (!vi->flash->separatedir) return 0; /* no separated directory */

	for (dcou=0; dcou<vi->flash->separatedir; dcou++) 
	{
		if (!fsm_findfreeblock(vi,&sector)) 
		{
			return 1; /* no free block to store */
		}

		oldblock=vi->_blockindex[sector/vi->flash->sectorperblock];


		/* remove previous entry */
		for (a=0; a<vi->maxsectornum; a++) 
		{  
			if (vi->_fat[a]==FS_FAT_DIR+(unsigned short)dcou) 
			{
				_fsm_cacheaddsptr(vi,&vi->_fat[a],FS_FAT_DISCARD);
			}
		}


		for (a=0; a<vi->flash->sectorperblock; a++) 
		{
			_fsm_cacheaddsptr(vi,&vi->_fat[sector+a],(unsigned short)(FS_FAT_DIR+dcou));	   /* set as directory entry */
		}


		for (;;) 
		{
			fsm_wearleveling(vi);

			freeblock=vi->_blockindex[vi->flash->maxblock];  /* get free block! */

			_fsm_cacheaddlptr(vi,&vi->_wearlevel[freeblock]);

			if (!vi->flash->EraseFlash (vi->flash->blockstart+freeblock)) 
			{
				if (!fsm_writeverifyflash(vi,&vi->direntries[dcou*(long)vi->maxdirentry/(long)vi->flash->separatedir],vi->flash->blockstart+freeblock,0,(long)vi->maxdirentry*(long)sizeof(FS_DIRENTRY)/vi->flash->separatedir,SDATA_NR)) 
				{
					/* swap free and old block (old is free now) */
					_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->flash->maxblock],oldblock);  
					_fsm_cacheaddsptr(vi,&vi->_blockindex[sector/vi->flash->sectorperblock],freeblock);
					goto nextdir;
				}
			}

			{
				unsigned short badsector;
				if (!fsm_findfreeblock(vi,&badsector)) return 1; /* no more free sector */
				fsm_swapbadblock(vi,badsector);
			}
		}

nextdir:;
	}
	return 0;
}

/****************************************************************************
 *
 * GetDir
 *
 * Internal function to get the last stored directory list back from flash
 * if separated block is used for direntries
 *
 * INPUTS
 *
 * vi - volume info which directory needs to be restore
 *
 * RETURNS
 *
 * 0 - if successfully restored
 * other if any error
 *
 ***************************************************************************/

static int GetDir(const FS_VOLUMEINFO *vi) 
{
	if (vi->flash->separatedir) 
	{
		long dcou;
		for (dcou=0; dcou<vi->flash->separatedir; dcou++) 
		{
			long a,block;
			long found=-1;

			/* remove previous entry */
			for (a=0; a<vi->maxsectornum; a++) 
			{  
				if (vi->_fat[a]==FS_FAT_DIR+(unsigned short)dcou) 
				{
					found=a;
					break;
				}
			}
			if (found==-1) return 1; /* directory not found */

			block=vi->_blockindex[a/vi->flash->sectorperblock];

			if (fsm_readflash(vi,&vi->direntries[dcou*(long)vi->maxdirentry/vi->flash->separatedir],vi->flash->blockstart+block,0,(long)vi->maxdirentry*(long)sizeof(FS_DIRENTRY)/vi->flash->separatedir)) 
			{
				return 1;
			}
		}

		if (vi->fatdesc->dircrc32 != fsm_calccrc32(FS_CRCINIT,vi->direntries,(unsigned long)vi->maxdirentry*sizeof(FS_DIRENTRY))) 
		{
			return 1; /* crc error */
		}
	}

	return 0; /* ok */
}


/****************************************************************************
 *
 * SwapFatBitsPrevBitsBlk
 *
 * swapping the new (mirror) block with the previous block physical
 * into lock block 
 *
 * INPUTS
 *
 * vi - volumeinfo where the free sectors are
 *
 * RETURNS
 *
 * zero if no error, other if any error
 *
 ***************************************************************************/

int SwapFatBitsPrevBitsBlk(FS_VOLUMEINFO *vi)
{
	if (vi->fatbitsblock==vi->flash->maxblock)
	{
		/* check if lock block is exist*/
		if (vi->lockblock==0xffffu)
		{
			long a;
			long found=-1;

			/* lock block not found try to search for it */
			for (a=0; a<vi->maxsectornum; a++) 
			{
				if (vi->_fat[a]==FS_FAT_LCKBLK) 
				{
					found=a;
					break;
				}
			}

			/* check if lock block is found */
			if (found!=-1) 
			{
				/* set logical block number to lock block */
				vi->lockblock=(unsigned short) (a/vi->flash->sectorperblock);
			}
			else
			{
				unsigned short sector;

				/* try to search a free block for lock block */
				if (!fsm_findfreeblock(vi,&sector)) 
				{
					/* no more free block */
					return 1; 
				}

				/* set as lock block */
				for (a=0; a<vi->flash->sectorperblock; a++) 
				{
					_fsm_cacheaddsptr(vi,&vi->_fat[sector+a],FS_FAT_LCKBLK);
				}

				vi->lockblock=(unsigned short)(sector/vi->flash->sectorperblock);
			}
		}

		/* if lock block exist swap physical */
		if (vi->lockblock!=0xffffu)
		{
			unsigned short newblk=vi->_blockindex[vi->fatbitsblock];
			unsigned short oldblk=vi->_blockindex[vi->prevbitsblock];
			unsigned short lckblk=vi->_blockindex[vi->lockblock];

			_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->prevbitsblock],newblk); /* swap free, old and lock block (old is free now) */
			_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->lockblock],oldblk);
			_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->fatbitsblock],lckblk);  

			vi->fatbitsblock=vi->prevbitsblock; /* safety set it, physical layer is already swapped upper*/

			return 0;
		}
		else
		{
			return 1;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * CopySector
 *
 * Copying into newblock (fatbits) the previously (prevbits) block relsector
 * data
 *
 * INPUTS
 *
 * vi - volumeinfo where the free sectors are
 * log_newblk - logical block where to copy data
 * log_oldblk - logical block where to get the data
 * relsector - relative sector in the block
 *
 * RETURNS
 *
 * 0 - if successfully stored
 * other if any error
 *
 ***************************************************************************/

static int CopySector(const FS_VOLUMEINFO *vi,unsigned short log_newblk, unsigned short log_oldblk, int relsector)
{
	unsigned short newblk=vi->_blockindex[log_newblk];
	unsigned short oldblk=vi->_blockindex[log_oldblk];

	if (!fsm_readflash(vi,vi->rdbuffer,vi->flash->blockstart+oldblk,vi->flash->sectorsize*relsector,vi->flash->sectorsize))
	{
		if (!fsm_writeverifyflash (vi,vi->rdbuffer,vi->flash->blockstart+newblk,relsector,vi->flash->sectorsize,SDATA_NR)) 
		{
			return 0;
		}
	} 
	return 0;
}


/****************************************************************************
 *
 * CopyBB
 *
 * Search a free block, signal it is a BAD block, get its physical as new
 * usable block, then Copy block into a new block with number of sectors.
 * When function returns then last failed operation should be executed
 * again. It works on fatbitsblock only!
 *
 * INPUTS
 *
 * vi - volumeinfo
 * sectornum - number of sectors to copy
 *
 * RETURNS
 *
 * 0 - if successfully
 * other if any error
 *
 ***************************************************************************/

static int CopyBB(FS_VOLUMEINFO *vi, int sectornum)
{
	unsigned short oldblock=vi->_blockindex[vi->fatbitsblock]; /* get its physical*/
	unsigned short freeblock;

	for (;;)
	{
		unsigned short badsector;
		long a;

		/* search a completely free logicalblock */
		if (!fsm_findfreeblock(vi,&badsector)) 
		{
			return 1; /* no more free sector */
		}

		/* set all sector in that block to not usable (bad) */
		for (a=0; a<vi->flash->sectorperblock; a++)
		{
			_fsm_cacheaddsptr(vi,&vi->_fat[badsector+a],FS_FAT_NOTUSED);	   /* set as not used sector for further */
		}

		/* get free blocks physical */
		freeblock=vi->_blockindex[badsector/vi->flash->sectorperblock];

		/* increase wear for freeblock */
		_fsm_cacheaddlptr(vi,&vi->_wearlevel[freeblock]);

		/* erase freeblock */
		if (!vi->flash->EraseFlash (vi->flash->blockstart+freeblock))
		{
			int sector;
			int error=0;

			/* copy all the sectors to the few free block */
			for (sector=0; sector<sectornum; sector++)
			{
				int ret=fsm_readflash(vi,vi->rdbuffer,vi->flash->blockstart+oldblock,vi->flash->sectorsize*sector,vi->flash->sectorsize);

				/* check if it is only just erased but not written */

				/* is there any other read problem */
				if (ret==-1) 
				{
					continue;
				}

				/* is there any other read problem */
				if (ret)
				{
					/* cannot be solved if reading fails */
					/* but skipping sector and read sector will return -1 which will generate read error*/
					continue;
				}

				if (fsm_writeverifyflash (vi,vi->rdbuffer,vi->flash->blockstart+freeblock,sector,vi->flash->sectorsize,SDATA_NR))
				{
					error=1;
					break;
				}
			}

			if (!error)
			{
				/* swap the new block to the original fatbits block */
				_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->fatbitsblock],freeblock);
				_fsm_cacheaddsptr(vi,&vi->_blockindex[badsector/vi->flash->sectorperblock],oldblock);
				/* copying is finished without error, so return back */
				return 0;
			}
		}
	}
}

/****************************************************************************
 *
 * SectorMerge
 *
 * Merge not-copied sectors to the latest fatbitsblock
 *
 * INPUTS
 *
 * vi - volumeinfo where the free sectors are
 *
 * RETURNS
 *
 * 0 - if successfully stored
 * other if any error
 *
 ***************************************************************************/

static int SectorMerge(FS_VOLUMEINFO *vi)
{
	/* check whether merge is needed (not merged already) */
	if (vi->fatbitsblock==vi->flash->maxblock)
	{
		int lastsector=-1;
		int relsector;
		int sector;

		for (relsector=0; relsector<vi->flash->sectorperblock; relsector++)
		{
			if (vi->copybits[(unsigned int)relsector>>5]&(1UL<<((unsigned int)relsector&31)))
			{
				/* store as last sector copy */
				lastsector=relsector;

				for (;;)
				{
					if (!vi->dobadblock)
					{
					if (!CopySector(vi,vi->fatbitsblock,vi->prevbitsblock,relsector))
					{
						break;
						}

						vi->dobadblock=1;
					}

					/* Copy block as badblock */
					if (CopyBB(vi,relsector))
					{
						/* signal error */
						return 1;
					}
					vi->dobadblock=0;
				}


				/* remove copy bits */
				vi->copybits[(unsigned int)relsector>>5]&=~(1UL<<((unsigned int)relsector&31));
			}
		}

		/* set reserved all sectors before last copy sector */
		for (sector=0; sector<=lastsector; sector++)
		{
			/* remove free bits */
			vi->fatbits[(unsigned int)sector>>5]&=~(1UL<<((unsigned int)sector&31));
		}


		if (SwapFatBitsPrevBitsBlk(vi))
		{
			return 1;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * StoreFat
 *
 * store fat into flash device, in normal flash fat+directory+blockindex
 * is written at once
 *
 * INPUTS
 *
 * vi - volume info which fat needs to be written from
 *
 * RETURNS
 *
 * 0 - if successfully written
 * other if any error
 *
 ***************************************************************************/

static int StoreFat(FS_VOLUMEINFO *vi) 
{
	FS_FATDESC *desc=vi->fatdesc;
	long a;
	long *pnextdesc;
	long *pcurrdesc;

	if (SectorMerge(vi)) 
	{
		/* signal error if merge has failed */
		return 1; 
	}

	*vi->version=F_DRVVERSION;

	if (vi->flash->cacheddescsize) 
	{

		_fsm_cacheupdate(vi);

		if (vi->cache.status & (FS_STCACHE_DESCCHANGE|FS_STCACHE_DIRCHANGE)) 
		{
			if (!(vi->cache.status & FS_STCACHE_FULL)) 
			{
				long pagestart=((long)(vi->cache.sptr)-(long)(vi->cache.desc))/vi->flash->cachedpagesize;
				long pagenum=(((long)vi->cache.ptr-(long)vi->cache.sptr)+vi->flash->cachedpagesize-1)/vi->flash->cachedpagesize;
				if (!vi->flash->WriteVerifyPage(vi->cache.sptr,vi->flash->blockstart+vi->cache.currdescnum,pagestart,pagenum,SDATA_NR)) 
				{
					_fsm_cachenext(vi);
					return 0;
				}
			}
		}

		_fsm_cachereset(vi);
	}

	if (vi->flash->separatedir) 
	{
		desc->dircrc32=fsm_calccrc32(FS_CRCINIT,vi->direntries,(unsigned long)vi->maxdirentry*sizeof(FS_DIRENTRY));
	}

	if (AllocNewCache(vi)) 
	{
		return 1; /* cannot allocate */
	}

	/* need directory area! */
	if (StoreDir(vi)) 
	{
		return 1; /* cannot stored */
	}

	/* setting up desciptor physical current and next address*/
	if (vi->pnextdesc==&vi->desc1) 
	{
		/* set current values when next is desc1 */
		pcurrdesc=&vi->desc1;
		pnextdesc=&vi->desc2;
	}
	else if (vi->pnextdesc==&vi->desc2) 
	{
		/* set current values when next is desc2 */
		pcurrdesc=&vi->desc2;
		pnextdesc=&vi->desc1;
	}
	else 
	{
		/* invalid descriptor order */
		return 1;
	}

	for (;;) 
	{
		/* get current descriptor block number */
		long currdesc=*pcurrdesc;
		/* check if it is valid */
		if (currdesc!=-1) 
		{
			long leastblk;
			fsm_wearleveling(vi);

			leastblk=vi->_blockindex[vi->flash->maxblock];

			/* moves fat if too much write <1% */
			if ((vi->_wearlevel[currdesc]>vi->_wearlevel[leastblk]) && (vi->_wearlevel[currdesc]-vi->_wearlevel[leastblk]>=128))  
			{ 
				long found=0;

				for (a=0; a<vi->flash->maxblock; a++) 
				{
					if (vi->_blockindex[a]==(unsigned short)currdesc) 
					{
						unsigned short freeblock=vi->_blockindex[vi->flash->maxblock];  /* get free block! */
						_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->flash->maxblock],(unsigned short)currdesc);   /* swap current */
						_fsm_cacheaddsptr(vi,&vi->_blockindex[a],freeblock); /* swap free */

						/* remove signatures and datas	from old descriptor */
						(void)(vi->flash->EraseFlash(currdesc+vi->flash->blockstart)); /* remove signatures and data	from old FAT */
						_fsm_cacheaddlptr(vi,&vi->_wearlevel[currdesc]); /* increase original because of erase */

						/* set new one */
						currdesc=freeblock; 

						/* store into origin value */
						*pcurrdesc=freeblock;

						found=1;
						break;
					}
				}

				/* this has to be always false */
				if (!found) 
				{
					return 1; 
				}
			}

			/* setting up new wear level value for currdesc */
			_fsm_cacheaddlptr(vi,&vi->_wearlevel[currdesc]);

			/* set next descriptor value in descriptor field */
			desc->nextdesc = *pnextdesc;

			/* calculate CRC on it */
			desc->crc32=fsm_calccrc32(FS_CRCINIT,(char*)desc+sizeof(desc->crc32),(unsigned long)vi->flash->descsize-sizeof(desc->crc32));

			if (!vi->flash->EraseFlash(currdesc+vi->flash->blockstart)) 
			{
				if (!fsm_writeverifyflash(vi,desc,currdesc+vi->flash->blockstart,0,vi->flash->descsize,SDATA_FAT)) 
				{
					/* increase reference */
					desc->reference++;

					/* set next desciptor physical */
					vi->pnextdesc = pnextdesc;

					return 0;
				}
			}

			(void)(vi->flash->EraseFlash(currdesc+vi->flash->blockstart)); /* remove signatures and data	from old FAT */
		}

		/* any error so doing bad block handling, new block allocation */
		{
			unsigned short sector;
			if (!fsm_findfreeblock(vi,&sector)) 
			{
				return 1; /* no more free sector */
			}

			for (a=0; a<vi->flash->sectorperblock; a++) 
			{
				/* set as new FAT entry */
				_fsm_cacheaddsptr(vi,&vi->_fat[sector+a],FS_FAT_NOTUSED);	   /* set as new FAT entry */
			}

			/* get physical and store as current */
			*pcurrdesc=vi->_blockindex[sector/vi->flash->sectorperblock]; 
		}
	}
}

/****************************************************************************
 *
 * CheckSignature
 *
 * checking signature with 1 bit error included
 *
 * INPUTS
 *
 * value - read from nflash signature (could content 1 bit error)
 * mask - mask need to be checked
 *
 * RETURNS
 *
 * 0 - signature is match
 * other - signature is invalid or having 2 bits error
 *
 ***************************************************************************/

static unsigned long CheckSignature(unsigned long value, unsigned long mask) 
{
	value^=mask;
	return (value & ((unsigned long)(value-1)));
}

/****************************************************************************
 *
 * GetFat
 *
 * Internal function to get the last stored fat back from flash
 *
 * INPUTS
 *
 * vi - volume info which fat needs to be restore
 *
 * RETURNS
 *
 * 0 - if successfully restored
 * other if any error
 *
 ***************************************************************************/

static int GetFat(FS_VOLUMEINFO *vi) 
{
	FS_FATDESC *desc=vi->fatdesc;
	unsigned long ref1=0,ref2=0;
	unsigned long crc1=1,crc2=1;
	long use=0;
	long a;

	/* reset descriptors */
	vi->desc1=-1;
	vi->desc2=-1;
	vi->lockblock=0xffffu;

	/* find descriptor blocks */
	for (a=0; a<=vi->flash->maxblock; a++) 
	{
		if (!CheckSignature((unsigned long)(vi->flash->GetBlockSignature(a+vi->flash->blockstart)),(unsigned long)SDATA_FAT)) 
		{
			if (vi->desc1==-1) 
			{
				if (!fsm_readflash(vi,desc,a+vi->flash->blockstart,0,vi->flash->descsize))
				{
					crc1=desc->crc32-fsm_calccrc32(FS_CRCINIT,((char*)desc)+sizeof(desc->crc32),(unsigned long)vi->flash->descsize-sizeof(desc->crc32));

					if (!crc1) 
					{
						vi->desc1=a;
						ref1=desc->reference;
						continue;
					}
				}
			}
			else if (vi->desc2==-1) 
			{
				if (!fsm_readflash(vi,desc,a+vi->flash->blockstart,0,vi->flash->descsize))
				{
					crc2=desc->crc32-fsm_calccrc32(FS_CRCINIT,((char*)desc)+sizeof(desc->crc32),(unsigned long)vi->flash->descsize-sizeof(desc->crc32));

					if (!crc2)	
					{
						vi->desc2=a;
						ref2=desc->reference;
						break;
					}
				}
			}

			/* remove signatures and datas from invalid block*/
			(void)(vi->flash->EraseFlash(a+vi->flash->blockstart)); 
		}
	}

	if (vi->desc1==-1) 
	{
		/* no fat found, not formatted maybe */
		return 1; 
	}

	if (crc1 && crc2) 
	{
		/* no correct fat */
		return 1;
	}

	/* determinate which descriptor ia using */
	if ((!crc1) && (!crc2)) 
	{
		if (ref1==ref2+1) 
		{
			use=1;
		}
		else if (ref2==ref1+1) 
		{
			use=2;
		}
	}
	else if (crc1 && (!crc2)) 
	{
		use=2;
	}
	else if (crc2 && (!crc1)) 
	{
		use=1;
	}

	if (use==1) 
	{
		if (fsm_readflash(vi,desc,vi->desc1+vi->flash->blockstart,0,vi->flash->descsize)) 
		{
			return 1;
		}

		desc->reference++;
		vi->pnextdesc = &vi->desc2;

		/* set other (next) descriptor address */
		if (desc->nextdesc!=vi->desc1)
		{
			vi->desc2=desc->nextdesc;
		}

		if (*vi->version!=F_DRVVERSION) 
		{
			/* invalid version number */
			return 1; 
		}

		return 0;
	}
	else if (use==2) 
	{
		if (fsm_readflash(vi,desc,vi->desc2+vi->flash->blockstart,0,vi->flash->descsize)) 
		{
			return 1;
		}

		desc->reference++;
		vi->pnextdesc = &vi->desc1;

		/* set other (next) descriptor address */
		if (desc->nextdesc!=vi->desc2)
		{
			vi->desc1=desc->nextdesc;
		}

		if (*vi->version!=F_DRVVERSION) 
		{
			return 1; /* invalid version number */
		}

		return 0;
	}
	else 
	{
		return 1; /* none of them */
	}
}

/****************************************************************************
 *
 * AllocBlock
 *
 * function to allocate a new free block for datastoring
 *
 * INPUTS
 *
 * vi - volume info
 *
 * RETURNS
 *
 * 0 - if successfully allocated
 * other if any error
 *
 ***************************************************************************/

static int AllocBlock(FS_VOLUMEINFO *vi)
{
	unsigned char numoffreecritical=0;

	for (;;)
	{
		unsigned short freeblock;
		unsigned short sector;
		long a;

		if (_fs_checkfreeblocks(vi,FS_NAND_RESERVEDBLOCK))
		{
			numoffreecritical=1; /* number of free blocks are critical 2006.03.01*/
		}

		fsm_wearleveling(vi);
		freeblock=vi->_blockindex[vi->flash->maxblock];  /* get free block! */
		_fsm_cacheaddlptr(vi,&vi->_wearlevel[freeblock]);

		/* erase reserved block 1st */
		if (!vi->flash->EraseFlash (vi->flash->blockstart+freeblock))
		{

#if (!FSF_MOST_FREE_ALLOC)
			for (a=0; a<vi->flash->maxblock; a++)
			{
				long b;
				sector=(unsigned short)(a*vi->flash->sectorperblock); /* get current blk sector's start */

				for (b=0; b<vi->flash->sectorperblock; b++,sector++)
				{
					if (fsm_checksectorfree(vi,sector))
					{
						char usedbefore=0;
						int c;

						/*if we are under in critical blk number */
						/* we should skip free blocks to be used for data */
						if (numoffreecritical)
						{
							int cou;
							long sectorstart=a;
							sectorstart*=vi->flash->sectorperblock;

							for (cou=0; cou<vi->flash->sectorperblock; cou++)
							{
								if (!fsm_checksectorfree(vi,sectorstart+cou)) break;
							}

							if (cou==vi->flash->sectorperblock) break; /* don't use this empty block */
						}

						vi->prevbitsblock=(unsigned short)a; /* previously logical block */
						vi->fatbitsblock=(unsigned short)vi->flash->maxblock; /* always free logical block */

						/* collect information about used sectors */
						for (c=0; c<vi->flash->sectorperblock; c++)
						{
							vi->fatbits[c>>5]|=(1UL<<(c&31)); /* set erase bits */

							/* check if data there */
							if (!fsm_checksectorfree(vi,vi->flash->sectorperblock*a+c))
							{
								vi->copybits[c>>5]|=(1UL<<(c&31)); /* set copybitbits */
								usedbefore=1;
							}
						}

						/* totally free we don't need mirroring */
						if (!usedbefore)
						{
							return SwapFatBitsPrevBitsBlk(vi); /* simulated as already merged */
						}

						return 0;
					}
				}
			}

			return 1; /* Signal error no more free sector */
#else
		{
			int found=0;
			unsigned short sec;
			long secnum=0,b;

			for (sec=0,sector=0,a=0; a<vi->flash->maxblock; a++)
			{
				long tmpsecnum=-1;
				int cou=0;
				unsigned short tmpsec=0;
				for (b=0; b<vi->flash->sectorperblock; b++,sec++)
				{
					if (fsm_checksectorfree(vi,sec))
					{
						if (tmpsecnum==-1)
						{
							tmpsecnum=b;
							tmpsec=sec;
						}
						cou++;
					}
				}

				if (numoffreecritical)
				{
					if (cou==vi->flash->sectorperblock) continue; /* keep totaly free block for management*/
				}

				if (cou>found)
				{
					sector = tmpsec;
					secnum = tmpsecnum;
					found  = cou;
				}

				if (cou==vi->flash->sectorperblock) break;
			}

			if (!found) return 1; /* no more free sector */

			a=sector/vi->flash->sectorperblock;
			b=secnum;

			if (fsm_checksectorfree(vi,sector))
			{
				int usedbefore=0;
				int c;

				vi->prevbitsblock=(unsigned short)a; /* previously logical block */
				vi->fatbitsblock=(unsigned short)vi->flash->maxblock; /* always free logical block */


				for (c=0; c<vi->flash->sectorperblock; c++)
				{ /* copy used sectors */
					vi->fatbits[(unsigned int)c>>5]|=(1UL<<(c&31)); /* set erase bits */

					/* check if data there */
					if (!fsm_checksectorfree(vi,vi->flash->sectorperblock*a+c))
					{
						usedbefore=1;
						vi->copybits[(unsigned int)c>>5]|=(1UL<<(c&31)); /* set copybitbits */
					}
				}

				/* totally free we don't need mirroring */
				if (!usedbefore)
				{
					return SwapFatBitsPrevBitsBlk(vi); /* simulated as already merged */
				}

				return 0;
		 	}
			else return 1; /* Signal error no free sector */
		}

#endif

		}

		/* it cannot be erased so swap to a non bad block and alloc again */
		{
			unsigned short badsector;
			fsm_memset(vi->fatbits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long)); /* remove erase bits */
			if (!fsm_findfreeblock(vi,&badsector)) return 1; /* no more free sector */
			fsm_swapbadblock(vi,badsector);
		}

	}
}

/****************************************************************************
 *
 * StoreSector
 *
 * Called from higher level to store one sector into flash
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
	int relsector;

	if (!len) return 0;

	for (;;)
	{
		/* check if fatbitsblock are valid, if not then start allocate one */
		if (vi->fatbitsblock!=0xffff)
		{
			/* This mechanism will store data in NAND flash by incrementally page order */
			for (relsector=0; relsector<vi->flash->sectorperblock; relsector++)
			{
				/* 1st check if copy is requested */
				if (vi->copybits[(unsigned int)relsector>>5]&(1UL<<(relsector&31)))
				{
					for (;;)
					{
						if (!vi->dobadblock)
						{
							/* copy to new position from old position */
							if (!CopySector(vi,vi->fatbitsblock,vi->prevbitsblock,relsector))
							{
								break; /* Copy was successfully */
							}

							vi->dobadblock=1;
						}

						/* Copy block as badblock */
						if (CopyBB(vi,relsector))
						{
							/* signal error */
							return 1;
						}
						vi->dobadblock=0;
					}

					/* remove copybits */
					vi->copybits[(unsigned int)relsector>>5]&=~(1UL<<(relsector&31));
					/* remove fatbits */
					vi->fatbits[(unsigned int)relsector>>5]&=~(1UL<<(relsector&31));
				}
				else
				{
					/* check if sector position is free, then we can store the new data there */
					if (vi->fatbits[(unsigned int)relsector>>5]&(1UL<<(relsector&31)))
					{
						/* clear erase bit on that sector */
						vi->fatbits[(unsigned int)relsector>>5]&=~(1UL<<(relsector&31));

						for (;;)
						{
							if (!vi->dobadblock)
							{
							unsigned short block=vi->_blockindex[vi->fatbitsblock]; /* get its physical*/

								if (!fsm_writeverifyflash(vi,data,vi->flash->blockstart+block,relsector,vi->flash->sectorsize,SDATA_NR))
								{
									/* chained into the previous physical (same logical block to) */
									fsm_addsectorchain(vi,file,(unsigned short)(relsector+vi->prevbitsblock*vi->flash->sectorperblock));
									return 0;
								}

								vi->dobadblock=1;
							}

							/* Copy block as badblock */
							if (CopyBB(vi,relsector))
							{
								/* signal error */
								return 1;
							}
							vi->dobadblock=0;
						}
					}
				}
			}

			/* swap physical if it is necessary, no more entry inside now */
			if (SwapFatBitsPrevBitsBlk(vi)) 
			{
				return 1;
			}

			/* Storing new log-phy table */
			if (StoreFat(vi))
			{
				return 1;
			}
		}

		/* try to alloc block */
		if (AllocBlock(vi)) 
		{
			return 1;
		}
	}
}

/****************************************************************************
 *
 * GetSector
 *
 * Get sector data back from flash
 *
 * INPUTS
 *
 * vi - volumeinfo which volume to belong it
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
	long block =secnum / vi->flash->sectorperblock;  /* get logical block */
	long blockrel=secnum - block * vi->flash->sectorperblock;  /* get relative position in block */

	if (block>=vi->flash->maxblock) return 1; /* invalid block address */

	/* check if it is a mirror block */
	if (block==vi->prevbitsblock)
	{
		/* check whether sector is used or not in mirror block */
		if (!(vi->fatbits[(unsigned long)blockrel>>5]&(1UL<<(blockrel&31))))
		{
			/* use this new block in this case */
			block=vi->fatbitsblock;
		}
	}

	block=vi->_blockindex[block]; /* convert logical to physical  */
	blockrel=blockrel*vi->sectorsize+offset; /* calculate block relative address */

	return fsm_readflash(vi,data,vi->flash->blockstart+block,blockrel,datalen);
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
	long cou;
	unsigned int c;

	/* reset all FAT entry */
	for (a=0; a<vi->maxsectornum; a++) 
	{
		vi->_fat[a]=FS_FAT_FREE;	    
	}
	/* reset directory */
	for (c=0; c<vi->maxdirentry; c++) 
	{
		vi->direntries[c].attr=0;  
	}

	/* find good and bad blocks */
	for (a=0; a<=vi->flash->maxblock; a++) 
	{
		if (!vi->flash->CheckBadBlock(a+vi->flash->blockstart)) 
		{
			vi->zerosector[a]=0xff;/* reset entry, good block */
		}
		else 
		{
			vi->zerosector[a]=0x0;/* bad block */
		}
	}

	cou=0;

	/* check bad block info and leave bad blocks out */
	for (a=0; a<=vi->flash->maxblock; a++) 
	{
		if (vi->zerosector[a]==0xff) 
		{
			vi->_blockindex[cou]=(unsigned short)a;
			cou++;
				}

			}

	/* check whether it has enough space */
	if (cou<FS_NAND_RESERVEDBLOCK) 
	{
		/* not enough good block */
		return 1; 
	}

	/* decrease counter to back (lastblock has to be only useable block) */
	cou--; 
	/* set last useable block to the last position */
	vi->_blockindex[vi->flash->maxblock]=vi->_blockindex[cou];

	/* put physical badblocks to the end */
	for (a=0; a<=vi->flash->maxblock; a++) 
	{
		if (!vi->zerosector[a]) 
		{
			long b;
			vi->_blockindex[cou]=(unsigned short)a;
			for (b=0; b<vi->flash->sectorperblock; b++) 
			{
				vi->_fat[cou*vi->flash->sectorperblock+b]=FS_FAT_NOTUSED;	   /* set as nout used (bad) */
			}
			cou++;
		}
	}

	if (vi->resetwear) 
	{
		for (a=0; a<=vi->flash->maxblock; a++) 
		{
			vi->_wearlevel[a]=0;
		}
	}

	for (a=0; a<=vi->flash->maxblock; a++) 
	{
		if (!CheckSignature((unsigned long)(vi->flash->GetBlockSignature(a+vi->flash->blockstart)),(unsigned long)SDATA_FAT)) 
		{
			vi->_wearlevel[a]++;

			/* remove signatures and datas from any signatured */
			(void)(vi->flash->EraseFlash(a+vi->flash->blockstart)); /* remove signatures and data from any signature */
		}
	}

	/* reset blocking allocation */
	fsm_memset(vi->fatbits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));
	fsm_memset(vi->copybits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));
	vi->prevbitsblock=0xffff;
	vi->fatbitsblock=0xffff;
	vi->lockblock=0xffffu;

	/* reset reference counter */
	vi->fatdesc->reference=0;
	/* reset descriptors */
	vi->desc1=-1;
	vi->desc2=-1;
	vi->pnextdesc=&vi->desc1;

	/* write descriptors 2 times */
	/* store fat1 */
	_fsm_cachereset(vi);
	if (StoreFat(vi)) 
	{
		return 1; 
	}

	/* store fat2 */
	_fsm_cachereset(vi);
	if (StoreFat(vi)) 
	{
		return 1; 
	}

	return 0;
}

/****************************************************************************
 *
 * fs_mount_nandflashdrive
 *
 * Mount function for this driver, called from fs_mount function
 *
 * INPUTS
 *
 * vd - volumedescriptor of the volume
 *
 * RETURNS
 *
 * 0 - if successfully mounted
 * other if any error
 *
 ***************************************************************************/

int fs_mount_nandflashdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc)
{
	FS_VOLUMEINFO *vi=vd->vi;

	vd->storefat       =StoreFat;
	vd->storesector    =StoreSector;
	vd->getsector	   =GetSector;
	vd->format		   =Format;

/* alloc flashdevice properties */
	vi->flash=(FS_FLASH*)fsm_allocdata(vi,sizeof(FS_FLASH));
	if (!vi->flash)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	fsm_memset(vi->flash,0,sizeof(FS_FLASH)); /* reset entries */

/* check phy function */
	if (!phyfunc)
	{
		vd->state=FS_VOL_DRVERROR;	 /* physical function is needed */
		return 1;
	}

	/* get flash properties */
	if (phyfunc(vi->flash))
	{
		vd->state=FS_VOL_DRVERROR;	/* unknown flash type */
		return 1;
	}

	if ((!vi->flash->ReadFlash) ||
		(!vi->flash->EraseFlash) ||
		(!vi->flash->WriteFlash) ||
		(!vi->flash->VerifyFlash) ||
		(!vi->flash->CheckBadBlock) ||
		(!vi->flash->GetBlockSignature))
	{
		vd->state=FS_VOL_DRVERROR;	/* these functions has to be implemented! */
		return 1;
	}

	vi->reserved = FS_NAND_RESERVEDBLOCK * vi->flash->blocksize;

	/* maximum 4 is allowed */
	if (vi->flash->separatedir>4)
	{
		vd->state=FS_VOL_DRVERROR;
		return 1;
	}

	vi->flash->cacheddescsize=0; /* this will be set automatically if config is correct */

	if (vi->flash->WriteVerifyPage)
	{
		if (vi->flash->cachedpagenum*vi->flash->cachedpagesize==vi->flash->blocksize)
		{
			vi->cache.desc=(unsigned long *)fsm_allocdata(vi,vi->flash->blocksize);
			if (!vi->cache.desc) 
			{
				vd->state=FS_VOL_NOMEMORY;
				return 1;
			}

			vi->flash->cacheddescsize=vi->flash->blocksize;
			vi->cache.size=vi->flash->cacheddescsize;
			vi->cache.maxde=0;
		}
		else
		{
			vd->state=FS_VOL_DRVERROR;	/* cachedpagesize*cachedpagenum!=blocksize */
			return 1;
		}
	}
	else if (vi->flash->cachedpagenum || vi->flash->cachedpagesize)
	{
		vd->state=FS_VOL_DRVERROR;	/* no writeverifypage function defined! */
		return 1;
	}

	vi->zerosector=(unsigned char*)fsm_allocdata(vi,vi->flash->maxblock);
	if (!vi->zerosector)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

	if (vi->flash->sectorperblock != vi->flash->blocksize/vi->flash->sectorsize)
	{
		vd->state=FS_VOL_DRVERROR;
		return 1;
	}

/* alloc write buffer */
	if (fsm_setsectorsize(vi,vi->flash->sectorsize))
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
/* alloc files */
	if (fsm_setmaxfile(vi,MAXFILE))
	{		  /*  maximum file number  */
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

/* alloc fatbits */
	vi->fatbits=(unsigned long*)fsm_allocdata(vi,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));
	if (!vi->fatbits)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	fsm_memset(vi->fatbits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));

/* alloc copybits, copy bits is for determining if the next sector space is a copyable sector only */
	vi->copybits=(unsigned long*)fsm_allocdata(vi,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));
	if (!vi->copybits)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	fsm_memset(vi->copybits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));

	/* reset block allocation*/
	vi->prevbitsblock=0xffff;
	vi->fatbitsblock=0xffff;

/* alloc desc (fat+directory and desc) */
	vi->fatdesc=(FS_FATDESC*)fsm_allocdata(vi,(long)sizeof(FS_FATDESC));
	if (!vi->fatdesc)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	else
	{
		FS_FATDESC *desc=vi->fatdesc;
		long freesize=vi->flash->descsize;

		vi->_blockindex=(unsigned short*)fsm_allocdata(vi,vi->flash->maxblock*(long)sizeof(unsigned short));
		if (!vi->_blockindex)
		{
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->flash->maxblock--;	/* 1 block is kept because of power fail!! */
		vi->maxsectornum=vi->flash->maxblock*vi->flash->sectorperblock;
		vi->_fat=(unsigned short*)fsm_allocdata(vi,vi->maxsectornum*(long)sizeof(unsigned short));
		if (!vi->_fat)
		{
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->_wearlevel=(long*)fsm_allocdata(vi,(vi->flash->maxblock+1)*(long)sizeof(long));
		if (!vi->_wearlevel)
		{
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->version=(long*)fsm_allocdata(vi,(long)sizeof(long));
		if (!vi->version)
		{
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		if (!vi->flash->separatedir)
		{	/* only if this is set to 0 */
			vi->maxdirentry=(unsigned int)((freesize-((long)vi->buffer-(long)desc))/(long)sizeof(FS_DIRENTRY));
			vi->direntries=(FS_DIRENTRY*)fsm_allocdata(vi,(long)vi->maxdirentry*(long)sizeof(FS_DIRENTRY));
			if (!vi->direntries)
			{
				vd->state=FS_VOL_NOMEMORY;
				return 1;
			}
		}

		freesize-=(long)vi->buffer-(long)desc;
		if (!fsm_allocdata(vi,freesize))
		{		/* alloc the rest of descriptor (flash->descsize); */
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}
	}

	/* only if this is set  */
	if (vi->flash->separatedir)
	{
		vi->maxdirentry=(unsigned int)(vi->flash->blocksize/(long)sizeof(FS_DIRENTRY));
		vi->maxdirentry*=(unsigned int)vi->flash->separatedir;
		vi->direntries=(FS_DIRENTRY*)fsm_allocdata(vi,(long)vi->maxdirentry*(long)sizeof(FS_DIRENTRY));
		if (!vi->direntries)
		{
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}
	}


/* alloc fatmirror */
	vi->fatmirror=(unsigned short*)fsm_allocdata(vi,vi->maxsectornum*(long)sizeof(unsigned short));
	if (!vi->fatmirror)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

/* alloc blockdata information temporary rd buffers */
	vi->rdbuffer=(char*)fsm_allocdata(vi,vi->flash->sectorsize);
	if (!vi->rdbuffer)
	{
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

/* try to get latest fat */
	if (GetFat(vi))
	{ /* gets the last fat  */
		vd->state=FS_VOL_NOTFORMATTED;
		return 1;
	}

	if (GetDir(vi))
	{ /* gets the last directory entries  */
		vd->state=FS_VOL_NOTFORMATTED;
		return 1;
	}

	if (RestoreChanges(vi))
	{ /* recovering from wr cache */
		vd->state=FS_VOL_NOTFORMATTED;
		return 1; /* invalid */
	}

	vd->state=FS_VOL_OK;

	return 0;
}

/****************************************************************************
 *
 * fs_getmem_nflashdrive
 *
 * returns the required memory in bytes for the driver
 *
 * RETURNS
 * required memory
 *
 ***************************************************************************/

long fs_getmem_nandflashdrive(FS_PHYGETID phyfunc)
{
	FS_FLASH flash;
	long mem=fsm_allocdatasize(sizeof(FS_VOLUMEINFO));     /* volumeinfo */

	if (phyfunc(&flash)) return 0; /* get flash properties, if not identified return zero */

	if (flash.separatedir>4) return 0; /* maximum 4 is allowed */

	if (flash.WriteVerifyPage)
	{
		if (flash.cachedpagenum*flash.cachedpagesize==flash.blocksize)
		{
			mem+=fsm_allocdatasize(flash.blocksize); /* for cached area */
		}
		else
		{
		   flash.cachedpagenum=flash.cachedpagesize=0;
		   flash.WriteVerifyPage=0;	 /* disable cache if pagesize*pagenum!=blocksize */
		}
	}

	mem+=fsm_allocdatasize((long)sizeof(FS_FLASH));           /*  flash prop  */
	mem+=fsm_allocdatasize(flash.maxblock);      /* zerosectors */

	mem+=fsm_allocdatasize((long)sizeof(FS_FILEINT)*MAXFILE); /*  maximum number of files  */
	mem+=fsm_allocdatasize(flash.sectorsize*MAXFILE); /* rd/wrbuffer */
	mem+=fsm_allocdatasize(flash.descsize);            /* descriptor size */

	if (flash.separatedir)
	{	/* only if this is set  */
		long maxdirentry=flash.blocksize/(long)sizeof(FS_DIRENTRY);
		maxdirentry*=flash.separatedir;
		mem+=fsm_allocdatasize(maxdirentry*(long)sizeof(FS_DIRENTRY));
	}

	mem+=fsm_allocdatasize((flash.maxblock-1)*flash.sectorperblock*(long)sizeof(unsigned short)); /* for fat mirror */

	mem+=fsm_allocdatasize(flash.sectorsize);       /*  tmp rd buffer  */
	mem+=fsm_allocdatasize((flash.sectorperblock/32+1)*(long)sizeof(long)); /* fatbits */
	mem+=fsm_allocdatasize((flash.sectorperblock/32+1)*(long)sizeof(long)); /* copybits */

	return mem;
}

/****************************************************************************
 *
 * end of nflshdrv.c
 *
 ***************************************************************************/
