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
/*************************************************************************
* FILE NAME
*
*       fstaticw.c
*
* COMPONENT
*
*       Nucleus Safe File System 
*
* DESCRIPTION
*
*       These operations are used to perform the static wear leveling 
*		in Safe.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       fg_staticisblkfree                  Check sectors in a block to
*                                           see if they are free.
*       fg_staticisblkused                  Check sectors in a block to
*                                           see if they are used.
*       fg_staticwear                       Performs static wear 
*                                           leveling.
*       fsm_staticwear                      Performs static wear
*                                           leveling for a multi task
*                                           file system.
*
************************************************************************/

#include "storage/fsf.h"
#include "storage/fstaticw.h"

#if (!FS_CAPI_USED)

/****************************************************************************
 *
 * fg_staticisblkfree
 *
 * static wear leveling subfunction for checking sectors in a block if they
 * are free
 *
 * INPUTS
 *
 * vi - volume info
 * sector - start sector in a block
 *
 * RETURNS
 *
 * 0 - not free
 * 1 - block is free
 *
 ***************************************************************************/

static int fg_staticisblkfree(const FS_VOLUMEINFO *vi,long sector) 
{
	long a;

	for (a=0; a<vi->flash->sectorperblock; a++,sector++) 
	{
		if (vi->_fat[sector]!=FS_FAT_FREE) return 0; /* not free */
		if (vi->fatmirror[sector]!=FS_FAT_FREE) return 0; /* not free */
	}

	return 1;
}

/****************************************************************************
 *
 * fg_staticisblkused
 *
 * static wear leveling subfunction for checking sectors in a block if they
 * are used
 *
 * INPUTS
 *
 * vi - volume info
 * sector - start sector in a block
 *
 * RETURNS
 *
 * 0 - not used
 * 1 - block is used
 *
 ***************************************************************************/

static int fg_staticisblkused(const FS_VOLUMEINFO *vi,long sector) 
{
	long a;
	int used=0;

	for (a=0; a<vi->flash->sectorperblock; a++,sector++) 
	{
		if (vi->_fat[sector]<=FS_FAT_EOF) 
		{
			used=1;
			if (vi->fatmirror[sector]==FS_FAT_FREE) continue;  /*  used and free  */
			if (vi->_fat[sector]==vi->fatmirror[sector]) continue; /* used but the same */
		}
		else if (vi->_fat[sector]==FS_FAT_FREE) continue;

		return 0; /* contains bad sector or locked */
	}

	return used;
}

/****************************************************************************
 *
 * fg_staticwear
 *
 * static wear leveling for normal filesystem
 *
 * INPUTS
 *
 * drvnum - drivenumber to wear level
 *
 * RETURNS
 *
 * 0 - if successful
 * other - error codes
 *
 ***************************************************************************/

static int fg_staticwear(FS_MULTI *fm,int drvnum) 
{
	FS_VOLUMEINFO *vi;
	long lessusedblk=-1;
	long lessusedblkwear=0;
	long mostfreeblk=-1;
	long mostfreeblkwear=0;
	long a;
	int ret,needstorefat=0;

	ret=_fg_getvolumeinfo(fm,drvnum,&vi);
	if (ret) return ret;

	if (!vi->_wearlevel) return F_ERR_NOTUSEABLE; /* no wear level info e.g. RAM drive */
	if (!vi->flash) return F_ERR_NOTUSEABLE; /* no wear level info e.g. RAM drive */
	if (!vi->flash->BlockCopy) return F_ERR_NOTUSEABLE; /* no blockcopy function is defined in phy driver */

	if (vi->staticcou) return F_NO_ERROR; /* not needed yet to check static wear */
	vi->staticcou=FS_STATIC_PERIOD; /* reload counter */

	for (a=0; a<vi->flash->maxblock; a++) 
	{
		long startsector;

		vi->laststaticwear++;
		if (vi->laststaticwear>=vi->flash->maxblock) vi->laststaticwear=0;

		if (vi->laststaticwear==vi->fatbitsblock) continue; /* skip, block is currently used by fs */
		if (vi->laststaticwear==vi->prevbitsblock) continue; /* skip, block is currently used by fs */

		startsector=vi->flash->sectorperblock*vi->laststaticwear; /* get start sector */

		if (fg_staticisblkfree(vi,startsector)) 
		{
			long newwear=vi->_wearlevel[vi->_blockindex[vi->laststaticwear]];

			if (mostfreeblk==-1) 
			{
				mostfreeblk=vi->laststaticwear;
				mostfreeblkwear=newwear;
			}
			else if (newwear>mostfreeblkwear) 
			{
				mostfreeblk=vi->laststaticwear;
				mostfreeblkwear=newwear;
			}
		}
		else if (fg_staticisblkused(vi,startsector)) 
		{
			long newwear=vi->_wearlevel[vi->_blockindex[vi->laststaticwear]];

			if (lessusedblk==-1) 
			{
				lessusedblk=vi->laststaticwear;
				lessusedblkwear=newwear;
			}
			else if (newwear<lessusedblkwear) 
			{
				lessusedblk=vi->laststaticwear;
				lessusedblkwear=newwear;
			}
		}
	}

	{
		long blockwear=vi->_wearlevel[vi->_blockindex[vi->flash->maxblock]];
		if (mostfreeblk==-1 || blockwear>mostfreeblkwear) 
		{
			needstorefat=1;
			mostfreeblk=vi->flash->maxblock;
			mostfreeblkwear=blockwear;
		}
	}

	if ( (mostfreeblk==-1) || (lessusedblk==-1) ) return F_NO_ERROR; /* nothing to change */

	if ( mostfreeblkwear <= lessusedblkwear) return F_NO_ERROR; /* nothing to change */

	if ( (mostfreeblkwear-lessusedblkwear) < FS_STATIC_DISTANCE) return F_NO_ERROR; /* check the distance, not enough */

	/* check whether fat needed to be stored */
	if (needstorefat)
	{
		ret=_fg_flush(vi); /* storing fat */
		if (ret) return ret;
	}

	if ( vi->flash->BlockCopy(vi->flash->blockstart+vi->_blockindex[mostfreeblk],
							  vi->flash->blockstart+vi->_blockindex[lessusedblk])) return F_ERR_ONDRIVE; /* copy block dest<-sou */

	_fsm_cacheaddlptr(vi,&vi->_wearlevel[vi->_blockindex[mostfreeblk]]); /* increase wear info on destination */

	{ /* swap mostfreeblk-lessusedblk physical index  */
		unsigned short tmp=vi->_blockindex[mostfreeblk];
		_fsm_cacheaddsptr(vi,&vi->_blockindex[mostfreeblk],vi->_blockindex[lessusedblk]);
		_fsm_cacheaddsptr(vi,&vi->_blockindex[lessusedblk],tmp);
	}

	return _fg_flush(vi); /* storing fat */
}

/****************************************************************************
 *
 * fsm_staticwear
 *
 * static wear leveling for multitask filesystem
 *
 * INPUTS
 *
 * drvnum - drivenumber to wear level
 *
 * RETURNS
 *
 * 0 - if successful
 * other - error codes
 *
 ***************************************************************************/

int fsm_staticwear(int drvnum) 
{
	int ret;
	FS_MULTI *fm;

	if (_fsm_gettask(&fm)) return F_ERR_TASKNOTFOUND;

	ret=fg_staticwear(fm,drvnum);

	_fsm_releasesemaphore(fm);

	return ret;
}

/****************************************************************************
 *
 * End of fstaticw.c
 *
 ***************************************************************************/
#endif /* (!FS_CAPI_USED) */

