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
*       nor_fldrv.c
*
* COMPONENT
*
*       Nucleus Safe NOR Driver
*
* DESCRIPTION
*
*       NOR generic flash driver.
*
* DATA STRUCTURES
*
*       None.
*           
* FUNCTIONS
*
*       _chk_preerase                       Check if pre erase is active.
*       StoreFat                            Store fat into flash device.       
*       GetFat                              Get last stored fat from flash.
*       StoreSector                         Store one sector into flash.
*       GetSector                           Get a sector from flash.
*       Format                              Format a volume.
*       fs_mount_flashdrive                 Mount function for driver.
*       fs_getmem_flashdrive                Returns number of bytes 
*                                           required for driver.
*
*************************************************************************/

#include        "drivers/nor_fldrv.h"

/****************************************************************************
 *
 * Driver version
 *
 ***************************************************************************/

#define F_DRVVERSION	0x0101

/****************************************************************************
 *
 * static definitions
 *
 ***************************************************************************/

#define MAXFILE 4    /* number of file could be opened at once */

/****************************************************************************
 *
 * _chk_preerase
 *
 * checking if preerase is active and set pre-eraseable descriptor block(s)
 *
 * INPUTS
 *
 * vi - volume info which fat needs to be written from
 *
 ***************************************************************************/

static void _chk_preerase(const FS_VOLUMEINFO *vi) 
{
	long a;
	long currfat;

	if (!vi->flash) return;
	if (!vi->flash->chkeraseblk) return;
	if (!vi->flash->erasedblk) return;

	currfat=vi->currfat;

	for (a=0; a<(vi->maxfat/2); a++) 
	{
		long nextdesc=vi->flash->descblockstart+currfat;

	    vi->flash->chkeraseblk[nextdesc]=1; /* request to preerase */

		currfat++; /* lets try next one */
		if (currfat>=vi->maxfat) currfat=0;
	}
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

static int StoreFat(FS_VOLUMEINFO *vi) {
FS_FATDESC *desc=vi->fatdesc;
long currfat=vi->currfat;
long a;

	*vi->version=F_DRVVERSION;

	if (vi->flash->cacheddescsize) {

		_fsm_cacheupdate(vi);

		if (vi->cache.status & (FS_STCACHE_DESCCHANGE|FS_STCACHE_DIRCHANGE)) {
			if (!(vi->cache.status & FS_STCACHE_FULL)) {
				if (!vi->flash->WriteFlash(vi->cache.sptr,vi->cache.currdescnum,0,(long)vi->cache.sptr[1],vi->flash->descsize+(((long)vi->cache.sptr)-(long)(vi->cache.desc)) )) {
					if (!vi->flash->VerifyFlash(vi->cache.sptr,vi->cache.currdescnum,0,(long)vi->cache.sptr[1],vi->flash->descsize+(((long)vi->cache.sptr)-(long)(vi->cache.desc)))) {
						_fsm_cachenext(vi);
						return 0;
					}
				}
			}
		}

		_fsm_cachereset(vi);
	}


	desc->reference&=1023; /* keep reference in range  */
	desc->crc32=fsm_calccrc32(FS_CRCINIT,(char*)desc+sizeof(desc->crc32),(unsigned long)vi->flash->descsize-sizeof(desc->crc32));

	for (a=0; a<(vi->maxfat/2); a++) {
		long nextdesc=vi->flash->descblockstart+currfat;

		if (!vi->flash->EraseFlash(nextdesc)) 
		{
			if (!vi->flash->WriteFlash(desc,nextdesc,0,vi->flash->descsize,0)) 
			{
         		if (!vi->flash->VerifyFlash(desc,nextdesc,0,vi->flash->descsize,0)) 
				{
					vi->cache.currdescnum=nextdesc;

					desc->reference++;
					desc->reference&=1023;

					vi->currfat=currfat+1; /* set next fat */
					if (vi->currfat>=vi->maxfat) vi->currfat=0; /* keep range */

					_chk_preerase(vi);

					return 0;
				}
			}
		}

		currfat++; /* lets try next one */
		if (currfat>=vi->maxfat) currfat=0;
	}

	return 1; /* error */

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
	long a;
	unsigned long currref=0;
	long currfat=-1;

	for (a=0; a<vi->maxfat; a++) 
	{
		if (vi->flash->ReadFlash(desc,vi->flash->descblockstart+a,0,vi->flash->descsize)) continue;
		if (desc->crc32==fsm_calccrc32(FS_CRCINIT,(char*)desc+sizeof(desc->crc32),(unsigned long)vi->flash->descsize-sizeof(desc->crc32))) 
		{
			unsigned long ref=desc->reference;

			if (currfat==-1) 
			{
				currref=ref;
				currfat=a;
			}
			else 
			{
				if (currref>ref) 
				{
					if (currref-ref > 512) 
					{
						currref=ref;
						currfat=a;
					}
				}
				else 
				{
					if (ref-currref < 512) 
					{
						currref=ref;
						currfat=a;
					}
				}
			}
		}
	}

	if (currfat==-1) return 1; /* cant found */

	if (vi->flash->ReadFlash(desc,vi->flash->descblockstart+currfat,0,vi->flash->descsize)) return 1;

	if (vi->flash->cacheddescsize) {
		vi->cache.currdescnum=vi->flash->descblockstart+currfat;

		_fsm_cachereset(vi);

		if (vi->flash->ReadFlash(vi->cache.desc,vi->flash->descblockstart+currfat,vi->flash->descsize,vi->flash->cacheddescsize)) return 1; /* cant usable */

		for (;;) {
			int ret=_fsm_chacheupdatechanges(vi);
			if (ret==2) break; /*  eof found  */
			if (ret==3) {
				vi->cache.status |= FS_STCACHE_FULL; /* no more could be added */
				return 1; /* invalid */
			}
			if (ret) {
				vi->cache.status |= FS_STCACHE_FULL; /* no more could be added */
				break;	/* any error on last */
			}
		}
	}

	desc->reference=currref+1;
	desc->reference&=1023;

	vi->currfat=currfat+1;
	if (vi->currfat>=vi->maxfat) vi->currfat=0;

	if (*vi->version!=F_DRVVERSION) return 1; /* invalid version number */

	_chk_preerase(vi);

	return 0;
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

static int StoreSector(FS_VOLUMEINFO *vi,FS_FILEINT *file,void *data,long len) {
long a;
long relsector;
unsigned short sector;
unsigned short freeblock;
	if (!len) return 0;

	for (relsector=0; relsector<vi->flash->sectorperblock; relsector++) 
	{
		if (vi->fatbits[(unsigned long)relsector>>5]&(1UL<<(relsector&31))) 
		{
			return _fsm_writenextsector(vi,file,relsector,data,0);
		}
	}

	for (;;) 
	{

		fsm_wearleveling(vi);
		freeblock=vi->_blockindex[vi->flash->maxblock];  /* get free block! */
		_fsm_cacheaddlptr(vi,&vi->_wearlevel[freeblock]);

		if (vi->flash->EraseFlash (vi->flash->blockstart+freeblock)) return 1;

#if (!FSF_MOST_FREE_ALLOC)
		for (sector=0,a=0; a<vi->flash->maxblock; a++) {
			long b;
			for (b=0; b<vi->flash->sectorperblock; b++,sector++) {
				if (fsm_checksectorfree(vi,sector)) {
					int usedbefore=0;
					unsigned short oldblock=vi->_blockindex[a];
					long c;
					vi->fatbitsblock=a; /* store for next write */

					for (c=0; c<vi->flash->sectorperblock; c++) { /* copy used sectors */
						if (!fsm_checksectorfree(vi,vi->flash->sectorperblock*a+c)) { /* check if data there */
							usedbefore=1;
							vi->fatbits[c>>5]&=~(1UL<<(c&31)); /* clear erase bits */

							if (!vi->flash->ReadFlash(vi->rdbuffer,vi->flash->blockstart+oldblock,c*vi->flash->sectorsize,vi->flash->sectorsize)) {
								if (vi->flash->WriteFlash (vi->rdbuffer,vi->flash->blockstart+freeblock,c,vi->flash->sectorsize,0)) goto badblock;
								if (vi->flash->VerifyFlash (vi->rdbuffer,vi->flash->blockstart+freeblock,c,vi->flash->sectorsize,0)) goto badblock;
							}

						}
						else {
							vi->fatbits[c>>5]|=(1UL<<(c&31)); /* set erase bits */
						}
					}

					vi->fatbits[b>>5]&=~(1UL<<(b&31)); /* clear erase bits */

					if (vi->flash->WriteFlash (data,vi->flash->blockstart+freeblock,b,vi->flash->sectorsize,0)) goto badblock;
					if (vi->flash->VerifyFlash(data,vi->flash->blockstart+freeblock,b,vi->flash->sectorsize,0)) goto badblock;

					fsm_addsectorchain(vi,file,sector);

					_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->flash->maxblock],oldblock);  /* swap free and old block (old is free now) */
					_fsm_cacheaddsptr(vi,&vi->_blockindex[a],freeblock);

					if (!usedbefore) return 0;

					return StoreFat(vi); /* we should store the fat if block was used before! */
				}
			}
		}
#else
		{
			int found=0;
			unsigned short sec;
			long secnum=0,b;

			for (sec=0,sector=0,a=0; a<vi->flash->maxblock; a++) {
				long tmpsecnum=-1;
				int cou=0;
				unsigned short tmpsec=0;
				for (b=0; b<vi->flash->sectorperblock; b++,sec++) {
					if (fsm_checksectorfree(vi,sec)) {
						if (tmpsecnum==-1) {
							tmpsecnum=b;
							tmpsec=sec;
						}
						cou++;
					}
				}

				if (cou>found) {
					sector = tmpsec;
					secnum = tmpsecnum;
					found  = cou;
				}
				if (cou==vi->flash->sectorperblock) break;
			}

			if (!found) return 1; /* no more free sector */

			a=sector/vi->flash->sectorperblock;
			b=secnum;

			if (fsm_checksectorfree(vi,sector)) {
				int usedbefore=0;
				unsigned short oldblock=vi->_blockindex[a];
				long c;
				vi->fatbitsblock=(unsigned short)a; /* store for next write */

				for (c=0; c<vi->flash->sectorperblock; c++) { /* copy used sectors */
					if (!fsm_checksectorfree(vi,vi->flash->sectorperblock*a+c)) { /* check if data there */
						usedbefore=1;
						vi->fatbits[(unsigned long)c>>5]&=~(1UL<<(c&31)); /* clear erase bits */

						if (!vi->flash->ReadFlash(vi->rdbuffer,vi->flash->blockstart+oldblock,c*vi->flash->sectorsize,vi->flash->sectorsize)) {
							if (vi->flash->WriteFlash (vi->rdbuffer,vi->flash->blockstart+freeblock,c,vi->flash->sectorsize,0)) goto badblock;
							if (vi->flash->VerifyFlash (vi->rdbuffer,vi->flash->blockstart+freeblock,c,vi->flash->sectorsize,0)) goto badblock;
						}

					}
					else {
						vi->fatbits[(unsigned long)c>>5]|=(1UL<<(c&31)); /* set erase bits */
					}
				}

				vi->fatbits[(unsigned long)b>>5]&=~(1UL<<(b&31)); /* clear erase bits */

				if (vi->flash->WriteFlash (data,vi->flash->blockstart+freeblock,b,vi->flash->sectorsize,0)) goto badblock;
				if (vi->flash->VerifyFlash(data,vi->flash->blockstart+freeblock,b,vi->flash->sectorsize,0)) goto badblock;

				fsm_addsectorchain(vi,file,sector);

		 		_fsm_cacheaddsptr(vi,&vi->_blockindex[vi->flash->maxblock],oldblock);  /* swap free and old block (old is free now) */
		 		_fsm_cacheaddsptr(vi,&vi->_blockindex[a],freeblock);

		 		if (!usedbefore) return 0;

		 		return StoreFat(vi); /* we should store the fat if block was used before! */
		 	}
		}

#endif
		return 1; /* signal error no free sector */

badblock: /* block is bad */

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

	block=vi->_blockindex[block]; /* convert logical to physical  */
	blockrel=blockrel*vi->sectorsize+offset; /* calculate block relative address */

	return vi->flash->ReadFlash(data,vi->flash->blockstart+block,blockrel,datalen);
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

static int Format(FS_VOLUMEINFO *vi) {
long a;
unsigned int b;

	for (a=0; a<vi->maxsectornum; a++) vi->_fat[a]=FS_FAT_FREE;	    /* reset desc */
	for (b=0; b<vi->maxdirentry; b++) vi->direntries[b].attr=0;  /* reset directory */

	vi->fatdesc->nextdesc=vi->flash->descblockstart;

	vi->fatdesc->reference&=1023; /* fat reference is counting upward */

	for (a=0; a<=vi->flash->maxblock; a++) {
		vi->_blockindex[a]=(unsigned short)a;
	}

	if (vi->resetwear) {
		for (a=0; a<=vi->flash->maxblock; a++) {
			vi->_wearlevel[a]=0;
		}
	}

	fsm_memset(vi->fatbits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));

	if (vi->flash->cacheddescsize) {
		_fsm_cachereset(vi);
	}

	for (a=0; a<vi->maxfat; a++) {	/* reset all fat entry */
		if (StoreFat(vi)) return 1; /* store fat1 */
	}

	return 0;
}

/****************************************************************************
 *
 * fs_mount_flashdrive
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

int fs_mount_flashdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc) {
FS_VOLUMEINFO *vi=vd->vi;

	vd->storefat       =StoreFat;
	vd->storesector    =StoreSector;
	vd->getsector	   =GetSector;
	vd->format		   =Format;

/* alloc flashdevice properties */
	vi->flash=(FS_FLASH*)fsm_allocdata(vi,sizeof(FS_FLASH));
	if (!vi->flash) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

	fsm_memset(vi->flash,0,sizeof(FS_FLASH)); /* reset entries */

/* check phy function */
	if (!phyfunc) {
		vd->state=FS_VOL_DRVERROR;	 /* physical function is needed */
		return 1;
	}

	if (phyfunc(vi->flash)) {	/* get flash properties */
		vd->state=FS_VOL_DRVERROR;	/* unknown flash type */
		return 1;
	}

	vi->flash->descsize-=vi->flash->cacheddescsize;

	if ((!vi->flash->ReadFlash) ||
		(!vi->flash->EraseFlash) ||
		(!vi->flash->WriteFlash) ||
		(!vi->flash->VerifyFlash)) {
		vd->state=FS_VOL_DRVERROR;	/* these functions has to be implemented! */
		return 1;
	}

	vi->maxfat=(vi->flash->descblockend-vi->flash->descblockstart)+1;
	vi->currfat=0;

	if (vi->flash->sectorperblock != vi->flash->blocksize/vi->flash->sectorsize) {
		vd->state=FS_VOL_DRVERROR;
		return 1;
	}

/* alloc write buffer */
	if (fsm_setsectorsize(vi,vi->flash->sectorsize)) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
/* alloc files */
	if (fsm_setmaxfile(vi,MAXFILE)) {		  /*  maximum file number  */
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}

/* alloc desc (fat+directory and desc) */
	vi->fatdesc=(FS_FATDESC*)fsm_allocdata(vi,sizeof(FS_FATDESC));
	if (!vi->fatdesc) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	else {

		FS_FATDESC *desc=vi->fatdesc;
		long freesize=vi->flash->descsize;

		vi->_blockindex=(unsigned short*)fsm_allocdata(vi,vi->flash->maxblock*(long)sizeof(unsigned short));
		if (!vi->_blockindex) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->flash->maxblock--;	/* 1 block is kept because of power fail!! */
		vi->maxsectornum=vi->flash->maxblock*vi->flash->sectorperblock;
		vi->_fat=(unsigned short*)fsm_allocdata(vi,vi->maxsectornum*(long)sizeof(unsigned short));
		if (!vi->_fat) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->_wearlevel=(long*)fsm_allocdata(vi,(vi->flash->maxblock+1)*(long)sizeof(long));
		if (!vi->_wearlevel) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->version=(long*)fsm_allocdata(vi,sizeof(long));
		if (!vi->version) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		vi->maxdirentry=(unsigned int)((freesize-((long)vi->buffer-(long)desc))/(long)sizeof(FS_DIRENTRY));
		vi->direntries=(FS_DIRENTRY*)fsm_allocdata(vi,(long)((long)vi->maxdirentry*(long)sizeof(FS_DIRENTRY)));
		if (!vi->direntries) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}

		freesize-=(long)vi->buffer-(long)desc;
		if (!fsm_allocdata(vi,freesize)) {		/* alloc the rest of descriptor (flash->descsize); */
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}
	}

	if (vi->flash->cacheddescsize) {
		vi->cache.desc=(unsigned long *)fsm_allocdata(vi,vi->flash->cacheddescsize);
		if (!vi->cache.desc) {
			vd->state=FS_VOL_NOMEMORY;
			return 1;
		}
		vi->cache.size=vi->flash->cacheddescsize;
		vi->cache.maxde=0;
	}

	vi->fatmirror=(unsigned short*)fsm_allocdata(vi,vi->maxsectornum*(long)sizeof(unsigned short));
	if (!vi->fatmirror) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}


	vi->fatbits=(unsigned long*)fsm_allocdata(vi,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));
	if (!vi->fatbits) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}
	fsm_memset(vi->fatbits,0,(vi->flash->sectorperblock/32+1)*(long)sizeof(long));


/* alloc blockdata information temporary wr/rd buffers */
	vi->rdbuffer=(char*)fsm_allocdata(vi,vi->flash->sectorsize);
	if (!vi->rdbuffer) {
		vd->state=FS_VOL_NOMEMORY;
		return 1;
	}


/* try to get latest fat */
	if (GetFat(vi)) { /* gets the last fat  */
		vd->state=FS_VOL_NOTFORMATTED;
		return 1;
	}

	vd->state=FS_VOL_OK;

	return 0;
}


/****************************************************************************
 *
 * fs_getmem_flashdrive
 *
 * returns the required memory in bytes for the driver
 *
 * RETURNS
 * required memory
 *
 ***************************************************************************/

long fs_getmem_flashdrive(FS_PHYGETID phyfunc) {
FS_FLASH flash;
long mem=fsm_allocdatasize((long)sizeof(FS_VOLUMEINFO));     /* volumeinfo */

	fsm_memset(&flash,0,(long)sizeof(FS_FLASH)); /* reset entries */

	if (phyfunc(&flash)) return 0; /* get flash properties, if not identified return zero */
	flash.descsize-=flash.cacheddescsize;

	mem+=fsm_allocdatasize((long)sizeof(FS_FLASH));          /*  flash prop  */

	mem+=fsm_allocdatasize((long)sizeof(FS_FILEINT)*MAXFILE); /*  maximum number of files  */
	mem+=fsm_allocdatasize(flash.sectorsize*MAXFILE); /* rd/wrbuffer */

	mem+=fsm_allocdatasize(flash.descsize);             /* descriptor size */
	mem+=fsm_allocdatasize(flash.sectorsize);			/*  tmp rd buffer  */

	mem+=fsm_allocdatasize((flash.maxblock-1)*flash.sectorperblock*(long)sizeof(unsigned short)); /* for fat mirror */

	mem+=fsm_allocdatasize((flash.sectorperblock/32+1)*(long)sizeof(long)); /* fat bits */

	if (flash.cacheddescsize)
	{
		mem+=fsm_allocdatasize(flash.cacheddescsize);  /* descriptor cache size */
	}

   return mem;
}

/****************************************************************************
 *
 * end of flashdrv.c
 *
 ***************************************************************************/
