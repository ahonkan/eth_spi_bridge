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
*       fsf.c
*
* COMPONENT
*
*       Nucleus Safe File System 
*
* DESCRIPTION
*
*       Contains Safe's standard API functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       fg_getversion                       Returns the file systems 
*                                           version number as a string.
*       fg_init                             Init function for the file system.
*       _fg_setdiscsectors                  Sets sector chains to discard.
*       _fg_copychainintomirror             Copies fat chain into mirror fat.
*       _fg_copymirrorchain                 Copies sector chains from mirror
*                                           fat to normal fat.
*       _fg_copymirrorchainnoremove         Copies a sector chain from mirror
*                                           fat to normal fat.
*       _fg_copydiscmirrorchain             Sets the discarded sector chains in 
*                                           in mirror fat to normal fat.
*       _fg_removemirrorchain               Sets a mirror chain to free.
*       _fg_cleanupfile                     Removes chains from mirrors,
*                                           and if file mode is used set 
*                                           it to abort.
*       _fg_removediscsectors               Release discarded sectors into
*                                           free sectors.
*       _fg_getdename                       Get a direntry into a string.   
*       _fg_removedename                    Remove direntry name. 
*       _fg_setdename                       Set a string into direntry.
*       _fg_flush                           Flush pending information.
*       fg_mountdrive                       Mount a new drive into the 
*                                           file system.
*       fg_unmountdrive                     Unmount a drive.
*       fg_get_drive_count                  Returns number of mounted drives.
*       fg_get_drive_list                   Returns a list of mounted volumes.
*       _fg_getvolumeinfo                   Returns volume info struct, based
*                                           off drive provided.
*       _fg_setfsname                       Convert a string into FS_NAME struct.
*       _fg_findfile                        Finds a file in a direentry.
*       _fg_namecheckwc                     Checks name with wild card in original.
*       _fg_findfilewc                      Used to find files or directory entries
*                                           with wild cards.
*       _fg_findpath                        Determine if a path is valid. 
*       _fg_find                            Find a file that exist in directory 
*                                           entry.
*       _fg_addentry                        Adds a new directory entry into 
*                                           directory list.
*       _fg_storesector                     Calls driver level store sector function.       
*       _fg_getsector                       Gets a sector from the driver levels
*                                           get sector function.
*       fg_getdrive                         Get the current drive number.
*       fg_chdrive                          Change the current drive.
*       fg_getdcwd                          Gets a drive's current working directory.
*       fg_wgetdcwd                         Unicode version of fg_getdcwd.
*       fg_getcwd                           Gets the current working directory
*                                           of the current drive.
*       fg_wgetcwd                          Unicode version of fg_getcwd.
*       fg_mkdir                            Makes a new directory.
*       fg_wmkdir                           Unicode version of fg_mkdir.
*       fg_rmdir                            Remove directory.
*       fg_wrmdir                           Unicode version of fg_rmdir.
*       fg_chdir                            Change current working directory.
*       fg_rename                           Rename a file or directory.
*       fg_wrename                          Unicode version of fg_rename.
*       fg_move                             Moves a file or dirctory.
*       fg_wmove                            Unicode version of fg_move.
*       fg_filelength                       Get the file length.
*       fg_wfilelength                      Unicode version of fg_filelength.
*       fg_delete                           Delete a file.
*       fg_wdelete                          Unicode version of fg_delete.
*       _fg_storefilebuffer                 Store buffer sector into device
*                                           if modified.
*       _fg_fseek                           Subfunction for fg_seek, moves
*                                           file position pointer.
*       _fg_check_handle                    Checks if a file handler is valid.
*       fg_seek                             Moves file position pointer.
*       fg_tell                             Tells current position of file pointer
*                                           of the file opened.
*       fg_eof                              Tells if file position pointer is end 
*                                           of file.
*       fg_rewind                           Resets file postion pointer to top 
*                                           of file.
*       fg_putc                             Writes a character into a file.
*       fg_getc                             Reads a character from a file. 
*       _fg_checkfilelock                   This function is called from xxx_open 
*                                           to check if a file is locked for
*                                           the given mode.
*       _fg_checkappend                     Check if file is open for append.
*       _fg_checksyncpos                    Searches for files with "r" then
*                                           updates the sector.
*       _fg_checkapwithr                    Check if any file has "r".
*       fg_open                             Opens a file for read/write/append.
*       fg_wopen                            Unicode version of fg_open.
*       fg_abortclose                       Aborts and closes a previously opened
*                                           file.
*       fg_close                            Closes previously opened file.
*       fg_flush                            Flush current file's content to 
*                                           physical.
*       fg_settimedate                      Sets a file's time and date.
*       fg_wsettimedate                     Unicode version of fg_setttimedate.
*       fg_gettimedate                      Gets a file's time and date.
*       fg_wgettimedate                     Unicode version of fg_gettimedate.
*       fg_write                            Write data to a file.
*       fg_read                             Read data from a file.
*       fg_getfreespace                     Gets free disk space.
*       fg_format                           Format a volume.
*       fg_findfirst                        Find a file or directory.
*       fg_wfindfirst                       Unicode version of fg_findfirst.
*       fg_findnext                         Find further files or directories.
*       fg_wfindnext                        Unicode version of fg_wfindnext.
*       fg_getpermission                    Get a file's permissions.
*       fg_wgetpermission                   Unicode version of fg_getpermission.
*       fg_setpermission                    Sets the file's permissions.
*       fg_wsetpermission                   Unicode version of fg_setpermission.
*       fg_getlabel                         Get the label of the media.
*       fg_setlabel                         Set the media label.
*       fg_truncate                         Truncate a file to a specific 
*                                           length.
*       fg_wtruncate                        Unicode version of fg_truncate.
*       fg_seteof                           Set EOF at current position.
*       fg_stat                             Get status information on
*                                           file.
*       fg_wstat                            Unicode version of fg_stat.
*       fg_ftruncate                        Truncate a file to a specific
*                                           length.
*       fg_checkvolume                      Deletes a previously installed
*                                           volume.
*       fg_get_oem                          Gets the OEM's name.
*
************************************************************************/

#include "nucleus.h" 
#include "storage/fsf.h"
#include "storage/fsl_defs.h"
   
extern STATUS fsv_get_vnode(UINT16 dh, VOID **fsnode);
extern VOID NUF_Ncpbuf(UINT8 *to, UINT8 *from, INT size);
extern MTE_S* fsl_mte_from_drive_n(INT16 n);
extern UINT16 wr_safe_convert_drvnum_to_dh(INT drivenum);

/****************************************************************************
 *
 * Variables
 *
 ***************************************************************************/

FS_FILESYSTEM fg_filesystem;  /*filesystem structure which contains volume descriptors*/
char truncate_tmp[512];
static const char OEM_name[]="SAFE_FS";
FS_MUTEX_TYPE effs_gmutex;

unsigned long SAFE_Unused_Param; /* Used to resolve compiler warnings. */
#if F_FILE_CHANGED_EVENT && (!FS_CAPI_USED)
F_FILE_CHANGED_EVENTFUNC f_filechangedevent;
#endif
/****************************************************************************
 *
 * fg_getversion
 *
 * returns with the filesystem version string
 *
 * RETURNS
 *
 * string pointer with version number
 *
 ***************************************************************************/

char *fg_getversion(void)
{
#ifdef SAFE_UNICODE
	return (char*)("EFFS_STD_UNI ver:2.54");
#else
	return (char*)("EFFS_STD ver:2.54");
#endif
}

/****************************************************************************
 *
 * fg_init
 *
 * Init function for filesystem, this function has to be called once at
 * startup
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_init(void)
{
	int a;

#if F_FILE_CHANGED_EVENT
	f_filechangedevent=0;
#endif
#if (!FS_CAPI_USED)
	fsm_memset(gl_multi, 0, sizeof(gl_multi));
#endif

#if (!USE_TASK_SEPARATED_CWD)
#if (!FS_CAPI_USED)
	fg_filesystem.fs_curdrive=FS_CURRDRIVE; 
#else
	fg_filesystem.fs_curdrive=0; 
#endif
#endif

	/* reset all drive parameters */
	for (a=0; a<FS_MAXVOLUME; a++)
	{
		fg_filesystem.vd[a].state=FS_VOL_NOTMOUNT;
		fg_filesystem.vd[a].vi = 0;
	}

	if (fs_mutex_create(&effs_gmutex)) return F_ERR_INITFUNC;

	return F_NO_ERROR;
}

/****************************************************************************
 *
 * _fg_setdiscsectors
 *
 * Internal function which will deletes (set to discard) sector chain
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * sector - 1st sector (typically in directory entry) of the chain
 *
 ***************************************************************************/

void _fg_setdiscsectors(FS_VOLUMEINFO *vi,unsigned short sector) 
{
	for (;;) 
	{
		unsigned short nextsector;

		if (sector>=FS_FAT_EOF) return;

		nextsector=vi->_fat[sector];

		_fsm_cacheaddsptr(vi,&vi->_fat[sector],FS_FAT_DISCARD); /* set as discard */

		sector=nextsector;
	}
}

/****************************************************************************
 *
 * _fg_copychainintomirror
 *
 * Internal function which will copy original Fat chain into mirror fat
 * and initiate some file variable, this has to be called only in fg_open
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * f - internal filepointer
 * de - directory entry which is used for file
 *
 ***************************************************************************/

int _fg_copychainintomirror(const FS_VOLUMEINFO *vi,FS_FILEINT *f,FS_DIRENTRY *de) 
{
	long datalen;
	unsigned short *sector;
	long sectorsize=vi->sectorsize;

	f->direntry=de;

	f->sectorstart=de->sector;

	f->len=de->len;

	f->pos=0;
	f->relpos=0;
   	datalen=f->len;

	sector=f->sector;
#if TI_COMPRESS
	if (f->direntry->attr & FS_ATTR_COMPRESSED) 
	{
		sectorsize<<=1;
	}
#endif

	for(;;) 
	{
		unsigned short nextsector=*sector;

		if (nextsector>=FS_FAT_EOF) 
		{
			if (!datalen) return 0;
			return 1; 	/* check eof before end, error! */
		}
		if (!datalen) return 1; /* how ? */

		vi->fatmirror[nextsector]=vi->_fat[nextsector]; /* copy living chain into mirror */
		sector=&vi->fatmirror[nextsector];		   /* goto next	sector */

   		if (datalen>=sectorsize) 
		{
   			datalen-=sectorsize;
   		}
   		else 
		{
			datalen=0;
	   	}
	}
}

/****************************************************************************
 *
 * _fg_copymirrorchainnoremove
 *
 * Internal function which will copy sector chain from mirror fat to normal
 * fat, Only called from fg_flush, it keeps the original chain
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * sector - 1st sector (typically in directory entry) of the chain
 *
 ***************************************************************************/

static void _fg_copymirrorchainnoremove(FS_VOLUMEINFO *vi,unsigned short sector) 
{
	for (;;) 
	{
		unsigned short nextsector;

		if (sector>=FS_FAT_EOF) return;

		nextsector=vi->fatmirror[sector];
		_fsm_cacheaddsptr(vi,&vi->_fat[sector],nextsector);        /* copy it */

		sector=nextsector;
	}
}

/****************************************************************************
 *
 * _fg_copymirrorchain
 *
 * Internal function which will copy sector chain from mirror fat to normal
 * fat, And set discardable sectors from mirror to normal fat
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * sector - 1st sector (typically in directory entry) of the chain
 * noremove - no remove from mirror
 *
 ***************************************************************************/

static void _fg_copymirrorchain(FS_VOLUMEINFO *vi,unsigned short sector,int noremove) 
{
	if (noremove)
	{
		_fg_copymirrorchainnoremove(vi,sector);
		return;
	}

	for (;;) 
	{
		unsigned short nextsector;

		if (sector>=FS_FAT_EOF) return;

		nextsector=vi->fatmirror[sector];
		_fsm_cacheaddsptr(vi,&vi->_fat[sector],nextsector); /* copy it */

		vi->fatmirror[sector]=FS_FAT_FREE; /* set as free in the mirror */

		sector=nextsector;
	}
}

/****************************************************************************
 *
 * _fg_copydiscmirrorchain
 *
 * Internal function which will set discardable sectors chain from mirror to
 * normal fat as discardable
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * sector - 1st sector of the chain
 *
 ***************************************************************************/

static void _fg_copydiscmirrorchain(FS_VOLUMEINFO *vi,unsigned short sector) 
{
	for (;;) 
	{
		unsigned short nextsector;

		if (sector>=FS_FAT_EOF) return;

		nextsector=vi->fatmirror[sector];
		_fsm_cacheaddsptr(vi,&vi->_fat[sector],FS_FAT_DISCARD);        /* set as discardable now */

		vi->fatmirror[sector]=FS_FAT_FREE; /* set as free in the mirror */

		sector=nextsector;
	}
}

/****************************************************************************
 *
 * _fg_removemirrorchain
 *
 * Internal function which will set free a mirror chain. 1st, call with
 * original, then call it with discardable.
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * sector - 1st sector of the chain
 *
 ***************************************************************************/

static void _fg_removemirrorchain(const FS_VOLUMEINFO *vi,unsigned short sector) 
{
	for (;;) 
	{
		unsigned short nextsector;

		if (sector>=FS_FAT_EOF) return;

		nextsector=vi->fatmirror[sector];

		vi->fatmirror[sector]=FS_FAT_FREE; /* set as free in the mirror */

		sector=nextsector;
	}
}


/****************************************************************************
 *
 * _fg_cleanupfile
 *
 * removes chains from mirrors, and set file mode as abort if it used
 * this is called when any error happens on file operations.
 *
 * INPUTS
 *
 * vi - volumeinfo structure where the chain is
 * f - internal file pointer
 *
 ***************************************************************************/

void _fg_cleanupfile(const FS_VOLUMEINFO *vi, FS_FILEINT *f)
{
	int locked=0;
	int a;

	/* check if its locked before cleaning up */
	for (a=0; a<vi->maxfile; a++)
	{
		int mode=vi->files[a].mode;

		if ((f!=&vi->files[a]) && (mode!=FS_FILE_CLOSE) && (mode!=FS_FILE_ABORT) )
		{
			if (vi->files[a].direntry==f->direntry)
			{
				if (f->mode==FS_FILE_RD)
				{
					locked=1;
					break;
				}
				else
				{
					vi->files[a].mode = FS_FILE_ABORT;
				}
			}
		}
	}

	/* if it's not locked (nobody uses it) then remove mirror chains */
	if (!locked)
	{
		_fg_removemirrorchain(vi,f->sectorstart);
		_fg_removemirrorchain(vi,f->discardstart);
		f->sector=&f->sectorstart;
		f->discard=&f->discardstart;
		f->sectorstart=f->discardstart=FS_FAT_EOF;
	}

	if (f->mode != FS_FILE_CLOSE)
	{
		f->mode=FS_FILE_ABORT;
	}
}
/****************************************************************************
 *
 * _fg_getcompatibleattr
 *
 * create FAT compatibe attribute
 *
 * INPUTS
 *
 * de - directory enrty
 *
 * RETURNS
 *
 * compatible attribute
 *
 ***************************************************************************/

#if F_FILE_CHANGED_EVENT
static unsigned char _fg_getcompatibleattr(FS_DIRENTRY *de)
{
	unsigned long  secure=de->secure;
	if (de->attr&FS_ATTR_DIR) 
	{
		secure|=FSSEC_ATTR_DIR;
	}
	else 
	{
		secure&=~FSSEC_ATTR_DIR;
		secure^=FSSEC_ATTR_ARC;
	}

	return (unsigned char)(secure >> (31-6));
}
#endif

/****************************************************************************
 *
 * _fg_removediscsectors
 *
 * Remove discarded sectors, this will release discarded sectors into free
 * sector
 *
 * INPUTS
 *
 * vi - volumeinfo where the discarded sectors need to be released
 *
 ***************************************************************************/

static void _fg_removediscsectors(const FS_VOLUMEINFO *vi) 
{
	long a;

	for (a=0; a<vi->maxsectornum; a++) 
	{
		if (vi->_fat[a]==FS_FAT_DISCARD) 
		{
			vi->_fat[a]=FS_FAT_FREE; /* no caching needed here */
		}
	}
}

/****************************************************************************
 *
 * _fg_getdename
 *
 * getting direntry into string
 *
 * INPUTS
 *
 * s - where to store filename, minimum size is FS_MAXLNAME
 * vi - volumeinfo
 * de - directory entry
 *
 ***************************************************************************/

void _fg_getdename(W_CHAR *s, const FS_VOLUMEINFO *vi, const FS_DIRENTRY *de) 
{
	unsigned int a;
	W_CHAR *slfn;
	int nxt;
	unsigned short nlfn;

	for (a=0; a<CFG_NU_OS_STOR_FILE_FS_SAFE_MAX_DIRENTRY_NAME; a++)
	{
		W_CHAR ch=de->lname[a];
		*s++=ch;
		if (!ch) return;
	}

	if (de->attr&FS_ATTR_LFN1NXT) nxt=1;
	else if (de->attr&FS_ATTR_LFN1NXTTOP) nxt=2;
	else nxt=0;

	nlfn=de->nlfn;

	while (nxt) 
	{
		FS_DIRENTRY_LFN *lfn=(FS_DIRENTRY_LFN *)(&vi->direntries[nlfn]);
		if (nxt==1) 
		{
			slfn=(W_CHAR*)lfn;
			slfn+=sizeof(FS_DIRENTRY_LFN);

			if (lfn->attr&FS_ATTR_LFN1NXT) nxt=1;
			else if (lfn->attr&FS_ATTR_LFN1NXTTOP) nxt=2;
			else nxt=0;

			nlfn=lfn->nlfn1;
		}
		else 
		{
			slfn=(W_CHAR*)lfn;
			slfn+=sizeof(FS_DIRENTRY_LFN);
			slfn+=FS_MAXLFN;

			if (lfn->attr&FS_ATTR_LFN2NXT) nxt=1;
			else if (lfn->attr&FS_ATTR_LFN2NXTTOP) nxt=2;
			else nxt=0;

			nlfn=lfn->nlfn2;
		}

		for (a=0; a<FS_MAXLFN; a++) 
		{
			W_CHAR ch=*slfn++;
			*s++=ch;
			if (!ch) return;
		}
	}

	*s=0;
}

/****************************************************************************
 *
 * _fg_removedename
 *
 * removing direntry name
 *
 * INPUTS
 *
 * vi - volumeinfo
 * de - directory entry to be removed
 *
 ***************************************************************************/

void _fg_removedename(FS_VOLUMEINFO *vi, FS_DIRENTRY *de) 
{
	int nxt;
	unsigned short nlfn;

	if (de->attr&FS_ATTR_LFN1NXT) nxt=1;
	else if (de->attr&FS_ATTR_LFN1NXTTOP) nxt=2;
	else nxt=0;

	de->attr&=~FS_ATTR_ALLLFN1; /* removes attr */

	_fsm_cacheaddde(vi,de);

	nlfn=de->nlfn;

	while (nxt) 
	{
		FS_DIRENTRY_LFN *lfn=(FS_DIRENTRY_LFN *)(&vi->direntries[nlfn]);

		_fsm_cacheaddde(vi,lfn);

		if (nxt==1) 
		{
			if (lfn->attr&FS_ATTR_LFN1NXT) nxt=1;
			else if (lfn->attr&FS_ATTR_LFN1NXTTOP) nxt=2;
			else nxt=0;

			nlfn=lfn->nlfn1;

			lfn->attr&=~FS_ATTR_ALLLFN1; /* removes attr */
		}
		else 
		{
			if (lfn->attr&FS_ATTR_LFN2NXT) nxt=1;
			else if (lfn->attr&FS_ATTR_LFN2NXTTOP) nxt=2;
			else nxt=0;

			nlfn=lfn->nlfn2;

			lfn->attr&=~FS_ATTR_ALLLFN2; /* removes attr */
		}
	}
}

/****************************************************************************
 *
 * _fg_setdename
 *
 * setting string into direntry, first it checks if there is enough space, then its try to remove
 * previous entry, and then it creates a new chain
 *
 * INPUTS
 *
 * s - original name
 * vi - volumeinfo
 * de - directory entry where to store
 *
 * RETURNS
 *
 * 0 - if successfull
 * other if no more entry
 *
 ***************************************************************************/

int _fg_setdename(const W_CHAR *s, FS_VOLUMEINFO *vi, FS_DIRENTRY *de) 
{
	FS_DIRENTRY_LFN *lfn;
	unsigned int a;
	int clen;
	int len;
	int prev;
	W_CHAR *slfn;
	unsigned short *prevnlfn;
	unsigned int b;

	for (len=0;;len++) 
	{
		if (!s[len]) 
		{
			break;
		}
	}

	if (len>=(int)(FS_MAXLNAME)) 
	{
		return 1; /* too long name */
	}

	clen=len;
    clen-=CFG_NU_OS_STOR_FILE_FS_SAFE_MAX_DIRENTRY_NAME;

	for (b=0; clen>0 && b<vi->maxdirentry; b++) 
	{
		lfn=(FS_DIRENTRY_LFN *)(&vi->direntries[b]);

		if (lfn->attr&FS_ATTR_DE) continue; /* check if de is used for direntry */
		if (!(lfn->attr&FS_ATTR_LFN1)) clen-=(int)FS_MAXLFN;
		if (!(lfn->attr&FS_ATTR_LFN2)) clen-=(int)FS_MAXLFN;
	}

	if (clen>0) return 1; /* not enough space */

	_fg_removedename(vi,de);

	_fsm_cacheaddde(vi,de);

	for (a=0; a<CFG_NU_OS_STOR_FILE_FS_SAFE_MAX_DIRENTRY_NAME; a++)
	{
		W_CHAR ch=*s++;
		de->lname[a]=ch;
		if (len) len--;
		if (!ch) return 0;
	}
	if (!len) 
	{
		return 0;
	}

	prev=1;
	prevnlfn=&de->nlfn;

	for (b=0; b<vi->maxdirentry; b++) 
	{
		lfn=(FS_DIRENTRY_LFN *)(&vi->direntries[b]);

		if (lfn->attr&FS_ATTR_DE) continue; /* check if de is used for direntry */
		if (!(lfn->attr&FS_ATTR_LFN1)) 
		{
			_fsm_cacheaddde(vi,lfn);

			if (prev==1) de->attr |= FS_ATTR_LFN1NXT;
			else de->attr |= FS_ATTR_LFN2NXT;

			*prevnlfn=(unsigned short)b;

			lfn->attr|=FS_ATTR_LFN1;

			slfn=(W_CHAR*)lfn;
			slfn+=sizeof(FS_DIRENTRY_LFN);

			for (a=0; a<FS_MAXLFN; a++) 
			{
				W_CHAR ch=*s++;
				*slfn++=ch;
				if (len) len--;
				if (!ch) return 0;
			}
			if (!len) return 0;

			de=(FS_DIRENTRY *)lfn;
			prev=1;
			prevnlfn=&lfn->nlfn1;
		}

		if (!(lfn->attr&FS_ATTR_LFN2)) 
		{
			_fsm_cacheaddde(vi,lfn);

			if (prev==1) de->attr |= FS_ATTR_LFN1NXTTOP;
			else de->attr |= FS_ATTR_LFN2NXTTOP;

			*prevnlfn=(unsigned short)b;

			lfn->attr|=FS_ATTR_LFN2;

			slfn=(W_CHAR*)lfn;
			slfn+=sizeof(FS_DIRENTRY_LFN);
			slfn+=FS_MAXLFN;

			for (a=0; a<FS_MAXLFN; a++) 
			{
				W_CHAR ch=*s++;
				*slfn++=ch;
		  		if (len) len--;
				if (!ch) return 0;
			}
			if (!len) return 0;

			de=(FS_DIRENTRY *)lfn;
			prev=2;
			prevnlfn=&lfn->nlfn2;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * _fg_flush
 *
 * Flush pending information to physical. It calls store_directory and
 * store_fat functions to update directory and fat onto physical driver
 * if everything successfully it removes discarded sectors
 *
 * INPUTS
 *
 * vi - volumeinfo, which volume need to be flushed
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int _fg_flush(FS_VOLUMEINFO *vi) 
{
	if (vi->stoperr) return F_ERR_UNUSABLE; /* check state 1st if there was any serious error */

	if (vi->staticcou) vi->staticcou--;	/* decrease counter */

	if (fg_filesystem.vd[vi->drivenum].storefat(vi)) 
	{
		vi->stoperr = 1;	   /* set fatal error flag */
		return F_ERR_UNUSABLE; /* call procedure */
	}

	_fg_removediscsectors(vi);	/* removes discarded sectors now */

	return F_NO_ERROR;
}

/****************************************************************************
 *
 * fg_mountdrive
 *
 * Mounting a new drive into filesystem
 *
 * INPUTS
 *
 * drivenum - which drive number is needed to be mount (0-A, 1-B, 2-C)
 * buffer - buffer address where internal memory allocation does
 * buffersize - size in bytes of the useable buffer
 * mountfunc - which drive to be mounted (e.g. fg_mount_ramdrive)
 * phyfunc - physical flash driver if its needed (RAMDRIVE has no physical!)
 *
 * RETURNS
 *
 * 0 - if no error and successfully mounted
 * other if any error FS_VOL_xxx error code
 *
 ***************************************************************************/

int fg_mountdrive(FS_MULTI *fm,int drivenum,void *buffer,long buffsize,FS_DRVMOUNT mountfunc,FS_PHYGETID phyfunc) 
{
	FS_VOLUMEDESC *vd;

	if (!mountfunc) return FS_VOL_NOTMOUNT;	/* no mount function */
	if (buffsize<(long)sizeof(FS_VOLUMEINFO)) return FS_VOL_NOMEMORY;

	if (drivenum<0 || drivenum>=FS_MAXVOLUME) return FS_VOL_NOTMOUNT;

	vd=&fg_filesystem.vd[drivenum];
	if (vd->state && vd->state!=FS_VOL_NOTFORMATTED) 
	{
		FS_VOLUMEINFO *vi;

		/* passed buffer has to be aligned to long (32bit) */
		if ((unsigned long)buffer & (sizeof(long)-1))
		{
			return FS_VOL_ALLOCATION;			 /* buffer is not aligned to long (odd address) */
		}

		vi=(FS_VOLUMEINFO*)buffer;				 /* set new volumeinfo pointer */

		vd->vi=vi;									/* set volumeinfo pointer into volumedesc */

		vi->freemem=buffsize;					 /* set buffer size */
		vi->usedmem=0;							 /* reset used memory */
		vi->buffer=(char*)buffer;				 /* set buffer info volumeinfo */

		if (!fsm_allocdata(vd->vi,sizeof(FS_VOLUMEINFO))) 
		{ /* alloc itself first */
			vd->state=FS_VOL_NOMEMORY;
			return vd->state;
		}

		vi->flash=0;
		vi->drivenum=drivenum;
		vi->_wearlevel=0;
		vi->laststaticwear=0;
		vi->staticcou=2;						/* after power on counter delay */
		vi->fatbitsblock=(unsigned short)0xffff;/* set to invalid for fsm_findfreeblock */
		vi->prevbitsblock=(unsigned short)0xffff;/* set to invalid for fsm_findfreeblock */
		vi->stoperr = 0;						 /* reset fatal error flag */
		vi->reserved=0;
		vi->dobadblock=0;

		fsm_memset(&vi->cache,0,sizeof(vi->cache));

		/* create semaphore */
		if (fs_mutex_create(&vi->mutex)) 
		{
			return FS_VOL_DRVERROR;
		}

		/* lock it*/
		if (_fsm_checksemaphore(fm,vi))
		{
			return FS_VOL_DRVERROR;
		}

		/* call mount function  */
		if (!mountfunc(vd,phyfunc)) 
		{
			long b;

			for (b=0; b<vi->maxfile; b++) 
			{
				vi->files[b].mode=FS_FILE_CLOSE; /* close all files; */
			}

			for (b=0; b<vi->maxsectornum; b++) 
			{
				vi->fatmirror[b]=FS_FAT_FREE;
			}

#if (!USE_TASK_SEPARATED_CWD)
			vi->cwd[0]=0; 			/* root directory */
#endif

			if (!vd->storefat	) vd->state=FS_VOL_DRVERROR;
			if (!vd->storesector) vd->state=FS_VOL_DRVERROR;
			if (!vd->getsector	) vd->state=FS_VOL_DRVERROR;
			if (!vd->format		) vd->state=FS_VOL_DRVERROR;

			_fg_removediscsectors(vi); /* removing discarded sectors */

			if (vi->flash) 
			{
  				if (vi->flash->chkeraseblk) 
				{
					if (vi->flash->erasedblk) 
					{
						fsm_wearleveling(vi); /* setting up preerase */
					}
					else 
					{ /* chkeraseblk and erasedblk both set to be */
						vd->state=FS_VOL_DRVERROR;
					}
				}
				else 
				{
					if (vi->flash->erasedblk) 
					{ /* chkeraseblk and erasedblk both set to be */
						vd->state=FS_VOL_DRVERROR;
					}
				}
			}
		}

   		return vd->state;
	}

	return FS_VOL_DRVALREADYMNT;
}

/****************************************************************************
 *
 * fg_unmountdrive
 *
 * unmount a drive, it set the status of the volume to FS_VOL_NOTMOUNT
 * memory deallocation task is the hosts task
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - which drive needs to be unmounted
 *
 * RETURN
 *
 * error code or zero if it successful
 *
 ***************************************************************************/

int fg_unmountdrive (FS_MULTI *fm,int drivenum) 
{
	FS_VOLUMEINFO *vi;

	if (drivenum<0 || drivenum>=FS_MAXVOLUME) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);
	vi=fg_filesystem.vd[drivenum].vi;

	if (vi) 
	{
		int ret=_fsm_checksemaphore(fm,vi);
		if (ret) return F_SETLASTERROR(ret);
	}

	fg_filesystem.vd[drivenum].state=FS_VOL_NOTMOUNT;

	if (vi) 
	{
		(void)fs_mutex_delete(&vi->mutex);
		fm->pmutex=0; /* release multi's mutex pointer */
	}

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_get_drive_count
 *
 * Returns the number of mounted volumes
 *
 * RETURN
 *
 * number of mounted volumes
 *
 ***************************************************************************/

int fg_get_drive_count ()
{
	int rc=0;
	int i;

	for (i=0;i<FS_MAXVOLUME;i++)
	{
		if (fg_filesystem.vd[i].state!=FS_VOL_NOTMOUNT) ++rc;
	}

	return rc;
}

/****************************************************************************
 *
 * fg_get_drive_list
 *
 * Returns a list of the mounted volumes
 *
 * INPUT
 *
 * buf - buffer wich must be FS_MAXVOLUME space inside 
 *		 put drivenumber which drive is mounted
 *
 * RETURNS
 *
 * Number of mounted volumes
 *
 ***************************************************************************/

int fg_get_drive_list (int *buf)
{
	int rc=0;
	int i;

	for (i=0;i<FS_MAXVOLUME;i++)
	{
		if (fg_filesystem.vd[i].state!=FS_VOL_NOTMOUNT)
		{
			++rc;
			if (buf) *buf++=i;
		}
	}

	return rc;
}

/****************************************************************************
 *
 * _fg_getvolumeinfo
 *
 * getting back a volume info structure of a given drive
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - which drive volumeinfo needs to be retrieve
 * pvi - pointer of volumeinfo structure where to put the pointer
 *
 * RETURNS
 *
 * 0 - if successfully retrieved
 * other if any error (FS_xxx errorcodes)
 *
 ***************************************************************************/

int _fg_getvolumeinfo(FS_MULTI *fm,int drivenum,FS_VOLUMEINFO **pvi) 
{
	FS_VOLUMEDESC *vd;
	int ret;

	if (drivenum < 0 || drivenum>=FS_MAXVOLUME) return F_ERR_INVALIDDRIVE;

    vd=&fg_filesystem.vd[drivenum];

	if (pvi) *pvi=vd->vi;	/* set volumeinfo */

	if (vd->vi) 
	{
		ret=_fsm_checksemaphore(fm,vd->vi);
		if (ret) return ret;

#if USE_TASK_SEPARATED_CWD
		vd->vi->cwd=fm->fs_vols[drivenum].cwd; /* safety uses of vi->cwd */
#endif
	}

	if (vd->state==FS_VOL_NOTFORMATTED) return F_ERR_NOTFORMATTED;
	if (vd->state) return F_ERR_INVALIDDRIVE;

	return F_NO_ERROR;
}

/****************************************************************************
 *
 * _fg_setfsname
 *
 * convert a single string into FS_NAME structure
 *
 * INPUTS
 *
 * name - combined name with drive,path,filename,extension used for source
 * fsname - where to fill this structure with separated drive,path,name,ext
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if name contains invalid path or name
 *
 ***************************************************************************/

int _fg_setfsname(FS_MULTI *fm,const W_CHAR *name,FS_NAME *fsname) 
{
	unsigned int pathpos=0;
	unsigned int namepos=0;
	unsigned int a;

	if (!name[0]) return 1; /* no name */

	if (name[1]==':') 
	{
		int drv=name[0];

		if (drv>='A' && drv<='Z') 
		{
			fsname->drivenum=drv-'A';
			name+=2; /* skip drive number */
		}
		else if (drv>='a' && drv<='z') 
		{
			fsname->drivenum=drv-'a';
			name+=2; /* skip drive number */
		}
		else return 1; /* invalid drive */
	}
	else 
	{
		fsname->drivenum=fg_getdrive(fm);
		if (fsname->drivenum==-1) return 1;
	}

	if (name[0]!='/' && name[0]!='\\') 
	{
#ifdef SAFE_UNICODE
   		if (fg_wgetdcwd(fm,fsname->drivenum,fsname->path,FS_MAXPATH)) return 1; /* error */
#else
   		if (fg_getdcwd(fm,fsname->drivenum,fsname->path,FS_MAXPATH)) return 1; /* error */
#endif
   		for (pathpos=0; fsname->path[pathpos];) pathpos++;
	}

	for (;;) 
	{
		W_CHAR ch=*name++;

		if (!ch) break;

		if (ch==':') return 1; /* not allowed */

		if (ch=='/' || ch=='\\') 
		{
			if (pathpos) 
			{
				if (fsname->path[pathpos-1]=='/') return 1; /* not allowed double  */
				if (fsname->path[pathpos-1]=='\\') return 1; /* not allowed double  */

				if (pathpos>=FS_MAXPATH-2) return 1; /* path too long */
				fsname->path[pathpos++]=FS_SEPARATORCHAR;
			}

			for (;namepos;) 
			{
				if (fsname->lname[namepos-1]!=' ') break;
				namepos--;		  /* remove end spaces */
			}

			for (a=0; a<namepos; a++) 
			{
				if (pathpos>=FS_MAXPATH-2) return 1; /* path too long */
				fsname->path[pathpos++]=fsname->lname[a];
			}
			namepos=0;
			continue;
		}

		if (ch==' ' && (!namepos)) continue; /* remove start spaces */

		if (namepos>=FS_MAXLNAME-2) return 1; /* name too long */

		fsname->lname[namepos++]=ch;
	}

	fsname->lname[namepos]=0; /* terminates it */
	fsname->path[pathpos]=0;  /* terminates it */

	for (;namepos;) 
	{
   		if (fsname->lname[namepos-1]!=' ') break;
   		fsname->lname[namepos-1]=0; /* remove end spaces */
   		namepos--;
	}

	if (!namepos) return 2; /* no name */
	return 0;
}

/****************************************************************************
 *
 * _fg_findfile
 *
 * internal function to finding file in directory entry
 *
 * INPUTS
 *
 * vi - volumeinfo structure, where to find file
 * name - filename
 * dirnum - directory number of file (FS_DIR_ROOT root directory)
 * pdirnum - pointer where to put current directory entry number of the file
 *
 * RETURNS
 *
 * 0 - if file was not found
 * 1 - if file was found
 *
 ***************************************************************************/

int _fg_findfile(const FS_VOLUMEINFO *vi,const W_CHAR *name,unsigned short dirnum,unsigned short *pdirnum) 
{
	unsigned int a;
	W_CHAR s[FS_MAXLNAME];

	for (a=0; a<vi->maxdirentry; a++) 
	{
		FS_DIRENTRY *de=&vi->direntries[a];
		unsigned int b;

		if (!(de->attr&FS_ATTR_DE)) continue; /* check if de is used for direntry */

		if (de->dirnum!=dirnum) continue;

		_fg_getdename(s,vi,de);

		for (b=0;; b++) 
		{
			W_CHAR ch1=_fsm_toupper(s[b]);
			W_CHAR ch2=_fsm_toupper(name[b]);

			if (ch1!=ch2) break; /* not equal */

			if (!ch1) /* ch2 also equal with 0 so they are equal*/
			{
				if (pdirnum) *pdirnum=(unsigned short)a;
				return 1;
			}
		}
	}

	return 0;
}

/****************************************************************************
 *
 * _fg_namecheckwc
 *
 * function for checking a nem with wild card to original
 *
 * INPUTS
 *
 * wname - wild card name (e.g. *.* or ?a.*
 * name - original name to check
 *
 * RETURNS
 *
 * 0 - if not equal
 * 1 - if equal or match
 *
 ***************************************************************************/

int _fg_namecheckwc (const W_CHAR *wname, const W_CHAR *name) 
{
	for (;;) 
	{
		W_CHAR ch=_fsm_toupper(*name);
		W_CHAR wch=_fsm_toupper(*wname);
		if ((!ch) && (!wch)) 
		{
			return 1;
		}
		if ((!wch) && (ch=='.')) 
		{
			name++;
			continue;
		}
		if (wch==ch) 
		{
			name++;
			wname++;
			continue;
		}
		if (wch=='?') 
		{
			if (ch) name++;
			wname++;
			continue;
		}
		if (wch=='*') 
		{
			wname++;
			wch=_fsm_toupper(*wname++);
			if (!wch) 
			{
				return 1;
			}

			for (;;) 
			{
				ch=_fsm_toupper(*name);
				if (ch==wch) 
				{
					name++;
					break;
				}
				if ((!ch) && (wch=='.')) 
				{
					break;
				}

				if (!ch) 
				{
					return 0;
				}
				name++;
			}
			continue;
		}

		return 0;
	}
}

/****************************************************************************
 *
 * _fg_findfilewc
 *
 * internal function to finding file in directory entry with wild card
 *
 * INPUTS
 *
 * vi - volumeinfo structure, where to find file
 * name - filename
 * dirnum - directory number of file (FS_DIR_ROOT root directory)
 * pdirnum - pointer where to put current directory entry number of the file
 * startpos - where to start searching from
 *
 * RETURNS
 *
 * 0 - if file was not found
 * 1 - if file was found
 *
 ***************************************************************************/

int _fg_findfilewc(const FS_VOLUMEINFO *vi,const W_CHAR *name,unsigned short dirnum,unsigned short *pdirnum,unsigned short startpos) 
{
	unsigned int a;
	W_CHAR s[FS_MAXLNAME];

	for (a=startpos; a<vi->maxdirentry; a++) 
	{
		FS_DIRENTRY *de=&vi->direntries[a];

		if (!(de->attr&FS_ATTR_DE)) continue; /* check if de is used for direntry */

		if (de->dirnum!=dirnum) continue;

		_fg_getdename(s,vi,de);

		if (_fg_namecheckwc(name,s)) 
		{
		 	if (pdirnum) *pdirnum=(unsigned short)a;
			return 1;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * _fg_findpath
 *
 * finding out if path is valid and retrieve directory number in FS_NAME and
 * correct path info with absolute path (removes relatives)
 *
 * INPUTS
 *
 * fg_name - filled structure with path,drive
 *
 * RETURNS
 *
 * 0 - if path was not found or invalid
 * 1 - if path was found
 *
 ***************************************************************************/

int _fg_findpath(const FS_VOLUMEINFO *vi,FS_NAME *fsname) 
{
	unsigned short dirnum;
	W_CHAR *path=fsname->path;
	W_CHAR *mpath=path;

	fsname->dirnum=FS_DIR_ROOT;

	for (;*path;) 
	{
		W_CHAR name[FS_MAXPATH];
		int len;

		name[0]=0;

		for (len=0;;len++) 
		{
			W_CHAR ch=path[len];
			if ((!ch) || (ch=='/') || ch=='\\') 
			{
				name[len]=0;
				break;
			}
			name[len]=ch;
		}

		/* check if is it a dot */
		if (name[0]=='.' && (len==1))
		{
			path+=len;

			if (!(*path)) 
			{
				if (mpath!=fsname->path) 
				{
					mpath--; /* if we are now at the top */
				}
				break;
			}
			path++;
			continue;
		}

		/* check if is it a dotdot */
		if ((name[0]=='.') && (name[1]=='.') && (len==2))
		{
			/* it is a dotdot */

			path+=len;

			if (mpath==fsname->path) return 0; /* we are in the top */

			mpath--; /* no on separator */
			for (;;) 
			{
				if (mpath==fsname->path) break; /* we are now at the top */
				mpath--;
				if (*mpath==FS_SEPARATORCHAR) 
				{
					mpath++;
					break;
				}
			}

			*mpath=0; /* terminate it */
			fsname->dirnum=vi->direntries[fsname->dirnum].dirnum; /* step back */

			if (!(*path)) 
			{
				if (mpath!=fsname->path) mpath--; /* if we are now at the top */
				break;
			}
			path++;
			continue;

		}
		
			/* if there was no dots just step */
			if (path==mpath) 
			{ 
				path+=len;
				mpath+=len;
			}
		else 
		{
			int a=len;
			while (a--) 
			{
				*mpath++=*path++;	 /* copy if in different pos */
			}
		}

		if (!_fg_findfile(vi,name,fsname->dirnum,&dirnum)) 
		{
			return 0;
		}

		if (!(vi->direntries[dirnum].attr & FS_ATTR_DIR ) ) 
		{
			return 0;
		}

		fsname->dirnum=dirnum;

		if (!(*path)) 
		{
			break;
		}
		path++;
		*mpath++=FS_SEPARATORCHAR; /* add separator */
	}

	*mpath=0; /* terminate it */
	return 1;
}

/****************************************************************************
 *
 * _fg_find
 *
 * find an fg_name file exist in directory entry, it checks the path 1st
 * then find the file
 *
 * INPUTS
 *
 * fg_name - filled structure with drive,path,name,ext which file to find
 * pde - FS_DIRENTRY pointer where to store pointer if file is found
 * pdirnum - directory entry number  is stored here if file is found
 *
 * RETURNS
 *
 * 0 - if file was not found
 * 1 - if file was found
 *
 ***************************************************************************/

int _fg_find(const FS_VOLUMEINFO *vi,FS_NAME *fsname,FS_DIRENTRY **pde,unsigned short *pdirnum) 
{
	unsigned short dirnum;

	if (!_fg_findpath(vi,fsname)) return 0;
	if (!_fg_findfile(vi,fsname->lname,fsname->dirnum,&dirnum)) return 0;

	if (pde) *pde=&vi->direntries[dirnum];
	if (pdirnum) *pdirnum=dirnum;

	return 1;
}

/****************************************************************************
 *
 * _fg_isnamedots
 *
 * check whether name is valid or dot/dotdot
 *
 * INPUTS
 *
 * name - incoming name for checking
 *
 * RETURNS
 *
 * 1 - it is dot or dotdot
 * 0 - name is valid
 *
 ***************************************************************************/

static int _fg_isnamedots(const W_CHAR *name)
{
	/* check if is it empty */
	if (!name[0]) 
	{
		return 1;
	}

	/* check if is it a dot */
	if ((name[0]=='.') && (name[1]==0))
	{
		return 1;
	}

	/* check if is it a dotdot */
	if ((name[0]=='.') && (name[1]=='.') && (name[2]==0))
	{
		return 1;
	}

	for (;;)
	{
		W_CHAR ch=*name++;

		if (!ch)
		{
			/* name contains spaces and dots */
			return 1;
		}

		/* if it is dot the continue */
		if (ch=='.')
		{
			continue;
		}

		/* if it is spaces the continue */
		if (ch==0x20)
		{
			continue;
		}

		return 0;
	}
}

/****************************************************************************
 * _fg_addentry
 *
 * Add a new directory entry into directory list
 *
 * INPUTS
 *
 * vi - volume info
 * fsname - filled structure what to add into directory list
 * pde - FS_DIRENTRY pointer where to store the entry where it was added
 *
 * RETURNS
 *
 * 0 - if successfully added
 * other - if any error (see FS_xxx errorcodes)
 *
 ***************************************************************************/

int _fg_addentry(FS_VOLUMEINFO *vi,const FS_NAME *fsname,FS_DIRENTRY **pde) 
{
	unsigned int a;

	if (_fg_isnamedots(fsname->lname)) 
	{
		return F_ERR_INVALIDNAME;
	}

	for (a=0; a<vi->maxdirentry; a++) 
	{
 		FS_DIRENTRY *de=&vi->direntries[a];

		if (!de->attr) 
		{ /* not used */
			de->attr=FS_ATTR_DE;

			if (_fg_setdename(fsname->lname,vi,de))
			{
				return F_ERR_NOMOREENTRY;
			}

			de->dirnum=fsname->dirnum;
			de->len=0;
			de->sector=FS_FAT_EOF;
			de->secure=0; /* nobody allows to access */

			fs_getcurrenttimedate(&de->ctime,&de->cdate);

			_fsm_cacheaddde(vi,de);

			if (pde) *pde=de;
			return F_NO_ERROR;
		}
	}

	return F_ERR_NOMOREENTRY;
}

/****************************************************************************
 *
 * _fg_storesector
 *
 * call store sector function on lower level to store data into sector
 *
 * INPUTS
 *
 * vi - volume info
 * file - FS_FILEINT handler, which file to belong
 * data - buffer pointer where the data is
 * len - length of the buffer
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int _fg_storesector(FS_VOLUMEINFO *vi,FS_FILEINT *file,void *data,long len) 
{
	if (vi->staticcou) vi->staticcou--;	/* decrease counter */

	if (fg_filesystem.vd[file->drivenum].storesector(vi,file,data,len)) return 1; /* call procedure */

	return 0;
}

/****************************************************************************
 *
 * _fg_getsector
 *
 * get a sector from lower level function
 *
 * INPUTS
 *
 * vi - volumeinfo structure which volume is used
 * secnum - sector number which sector is needed
 * data - pointer where to store the data
 * offset - relative position in sector from zero
 * datalen - data length of data
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int _fg_getsector (const FS_VOLUMEINFO *vi,long secnum,void *data,long offset,long datalen) 
{
	if (secnum==FS_FAT_EOF) return 0; /* nothing need to be read! */

	if (secnum>FS_FAT_EOF) return 1; /* invalid sector */

	if (fg_filesystem.vd[vi->drivenum].getsector(vi,secnum,data,offset,datalen)) return 1; /* call procedure */

	return 0;
}

/****************************************************************************
 *
 * fg_getdrive
 *
 * Get current drive number
 *
 * INPUTS
 *
 * fm - multi structure pointer
 *
 * RETURNS
 *
 * with the current drive number (0-A, 1-B,...)
 *
 ***************************************************************************/

int fg_getdrive(const FS_MULTI *fm) 
{
#if((USE_VFS) && (USE_TASK_SEPARATED_CWD))
	return *(fm->fs_curdrive);
#elif((!USE_VFS) && (USE_TASK_SEPARATED_CWD))
    return fm->fs_curdrive;
#else
	fm=fm; /* only for referencing fm variable */
	return fg_filesystem.fs_curdrive;
#endif

}

/****************************************************************************
 *
 * fg_chdrive
 *
 * Change current drive
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - new current drive number (0-A, 1-B, ...)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error (e.g invalid drive number)
 *
 ***************************************************************************/

int fg_chdrive(FS_MULTI *fm,int drivenum) 
{
	if (_fg_getvolumeinfo(fm,drivenum,0)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */
#if((USE_VFS) && (USE_TASK_SEPARATED_CWD))
	*(fm->fs_curdrive)=(short int)drivenum;
#elif((!USE_VFS) && (USE_TASK_SEPARATED_CWD))
    (fm->fs_curdrive)=drivenum;
#else
    fg_filesystem.fs_curdrive=drivenum;    
#endif

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_getdcwd
 *
 * getting a drive current working directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - drive number of which drive current folder needed
 * buffer - where to store current working folder
 * maxlen - buffer length (possible size is FS_MAXPATH)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error (e.g invalid drive number)
 *
 ***************************************************************************/

int fg_getdcwd(FS_MULTI *fm,int drivenum, char *buffer, int maxlen ) 
{
	FS_VOLUMEINFO *vi;	
	int a;

	if (_fg_getvolumeinfo(fm,drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (!maxlen) return F_SETLASTERROR(F_NO_ERROR);

	maxlen--;	/* need for termination */

	for (a=0; a<maxlen; a++) 
	{
		char ch=(char)(vi->cwd[a]);
		buffer[a]=ch;
		if (!ch) break;
	}

	buffer[a]=0;	/* add terminator at the end */

	return F_SETLASTERROR(F_NO_ERROR);
}

#ifdef SAFE_UNICODE
int fg_wgetdcwd(FS_MULTI *fm,int drivenum, W_CHAR *buffer, int maxlen ) 
{
	FS_VOLUMEINFO *vi;
	int a;

	if (_fg_getvolumeinfo(fm,drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (!maxlen) return F_SETLASTERROR(F_NO_ERROR);

	maxlen--;	/* need for termination */

	for (a=0; a<maxlen; a++) 
	{
		W_CHAR ch=vi->cwd[a];
		buffer[a]=ch;
		if (!ch) break;
	}

	buffer[a]=0;	/* add terminator at the end */

	return F_SETLASTERROR(F_NO_ERROR);
}
#endif

/****************************************************************************
 *
 * fg_getcwd
 *
 * getting a current working directory of current drive
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * buffer - where to store current working folder
 * maxlen - buffer length (possible size is FS_MAXPATH)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_getcwd(FS_MULTI *fm,char *buffer, int maxlen ) 
{
	return fg_getdcwd(fm,fg_getdrive(fm),buffer,maxlen);
}

#ifdef SAFE_UNICODE
int fg_wgetcwd(FS_MULTI *fm,W_CHAR *buffer, int maxlen ) 
{
	return fg_wgetdcwd(fm,fg_getdrive(fm),buffer,maxlen);
}
#endif

/****************************************************************************
 *
 * fg_mkdir
 *
 * making a new directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * dirname - new directory name
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_mkdir(FS_MULTI *fm,const char *dirname) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wmkdir(fm,_fsm_towchar(nconv,dirname));
}

int fg_wmkdir(FS_MULTI *fm,const W_CHAR *dirname) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,dirname,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */
	if (fsm_strlen(fsname.lname)+fsm_strlen(fsname.path) >= FS_MAXPATH-4) return F_SETLASTERROR(F_ERR_TOOLONGNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);
	if (!_fg_findpath(vi,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDDIR);
	if (_fg_findfile(vi,fsname.lname,fsname.dirnum,0)) return F_SETLASTERROR(F_ERR_DUPLICATED);		/* already open */

	if (_fg_addentry(vi,&fsname,&de)) return F_SETLASTERROR(F_ERR_NOMOREENTRY); /* couldn't be added */

	de->attr |= FS_ATTR_DIR;		/* set as directory */

	_fsm_cacheaddde(vi,de);
#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		fc.action = FACTION_ADDED;
		fc.flags = FFLAGS_DIR_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
		fc.attr=_fg_getcompatibleattr(de);
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsname.drivenum,fsname.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}
	}
#endif

	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_rmdir
 *
 * Remove directory, only could be removed if empty
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * dirname - which directory needed to be removed
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_rmdir(FS_MULTI *fm,const char *dirname) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wrmdir(fm,_fsm_towchar(nconv,dirname));
}

int fg_wrmdir(FS_MULTI *fm,const W_CHAR *dirname) 
{
#endif
	unsigned short dirnum;
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;
	unsigned int a;

	if (_fg_setfsname(fm,dirname,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);

	 /* check if directory is empty */
	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (!(_fg_find(vi,&fsname,&de,&dirnum))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	if (!(de->attr & FS_ATTR_DIR)) return F_SETLASTERROR(F_ERR_INVALIDDIR);		   /* not a directory */
	if (de->secure&FSSEC_ATTR_READONLY) return F_SETLASTERROR(F_ERR_ACCESSDENIED);

	for (a=0; a<vi->maxdirentry; a++) 
	{
		FS_DIRENTRY *di=&vi->direntries[a];
		if ((di->attr&FS_ATTR_DE) && (di->dirnum == dirnum)) return F_SETLASTERROR(F_ERR_NOTEMPTY); /* something is there */
	}

	_fg_removedename(vi,de);
	de->attr=0;

	_fsm_cacheaddde(vi,de);

#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		fc.action = FACTION_REMOVED;
		fc.flags = FFLAGS_DIR_NAME;
		fc.attr=0;
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsname.drivenum,fsname.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}
	}
#endif
	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_chdir
 *
 * change current working directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * dirname - new working directory name
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_chdir(FS_MULTI *fm,const char *dirname) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wchdir(fm,_fsm_towchar(nconv,dirname));
}

int fg_wchdir(FS_MULTI *fm,const W_CHAR *dirname) 
{
#endif
	FS_VOLUMEINFO *vi;
	FS_NAME fsname;
	int len;
	int a;
	int ret;

 	ret=_fg_setfsname(fm,dirname,&fsname);

	if (ret==1) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	for (len=0;fsname.path[len];) len++;

	if (fsname.lname[0]) 
	{
		if (len && len<FS_MAXPATH) fsname.path[len++]=FS_SEPARATORCHAR;

		for (a=0;a<FS_MAXPATH; a++) 
		{
			W_CHAR ch=fsname.lname[a];
			if (!ch) break;
			if (len<FS_MAXPATH) fsname.path[len++]=ch;
			else return F_SETLASTERROR(F_ERR_TOOLONGNAME);
		}
	}

	if (len<FS_MAXPATH) fsname.path[len]=0; /* terminate it */
	else return F_SETLASTERROR(F_ERR_TOOLONGNAME);

	if (!(_fg_findpath(vi,&fsname))) return F_SETLASTERROR(F_ERR_NOTFOUND);

	for (a=0; a<FS_MAXPATH;a++) vi->cwd[a]=fsname.path[a];

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_rename
 *
 * Rename file or directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - filename or directory name (with or without path)
 * newname - new name of the file or directory (without path)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_rename(FS_MULTI *fm,const char *filename, const char *newname) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	W_CHAR nconv2[FS_MAXPATH];
	return fg_wrename(fm,_fsm_towchar(nconv,filename),_fsm_towchar(nconv2,newname));
}

int fg_wrename(FS_MULTI *fm,const W_CHAR *filename, const W_CHAR *newname) 
{
#endif
	FS_VOLUMEINFO *vi;
	FS_DIRENTRY *de;
	FS_NAME fsname;
	int a,pathlen;
	unsigned short dirnum;
#if F_FILE_CHANGED_EVENT
	FS_NAME fsnameori;
#endif

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	if (de->secure&FSSEC_ATTR_READONLY) return F_SETLASTERROR(F_ERR_ACCESSDENIED);

	for (a=0; a<vi->maxfile; a++) if (vi->files[a].mode!=FS_FILE_CLOSE) 
	{
		if (vi->files[a].direntry==de) return F_SETLASTERROR(F_ERR_BUSY);			/* file is open  */
	}

	dirnum=fsname.dirnum;
	pathlen=fsm_strlen(fsname.path);

#if F_FILE_CHANGED_EVENT
	fsnameori=fsname; /* copy original into temporary*/
#endif
	if (fsm_checknamewc(newname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */
	if (_fg_setfsname(fm,newname,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_strlen(fsname.lname)+ pathlen >= FS_MAXPATH-4) return F_SETLASTERROR(F_ERR_TOOLONGNAME);/* invalid name */


	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);

	if (_fg_findfile(vi,fsname.lname,dirnum,0)) return F_SETLASTERROR(F_ERR_DUPLICATED);

	if (_fg_setdename(fsname.lname,vi,de)) return F_SETLASTERROR(F_ERR_NOMOREENTRY);
#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		/* oldname */
		fc.action = FACTION_RENAMED_OLD_NAME;
   		if (de->attr & FS_ATTR_DIR ) 
		{
			fc.flags = FFLAGS_DIR_NAME;
		}
		else
		{
			fc.flags = FFLAGS_FILE_NAME;
		}

		fc.attr=_fg_getcompatibleattr(de);
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsnameori.drivenum,fsnameori.path,fsnameori.lname))
		{
			f_filechangedevent(&fc);
		}

		/* newname */
		fc.action = FACTION_RENAMED_NEW_NAME;
   		if (de->attr & FS_ATTR_DIR ) 
		{
			fc.flags = FFLAGS_DIR_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
		}
		else
		{
			fc.flags = FFLAGS_FILE_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
		}

		fc.attr=_fg_getcompatibleattr(de);
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsnameori.drivenum,fsnameori.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}

	}	   
#endif

	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_move
 *
 * move file or directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - filename or directory name (with or without path)
 * newname - new name of the file or directory with path
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_move(FS_MULTI *fm,const char *filename, const char *newname) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	W_CHAR nconv2[FS_MAXPATH];
	return fg_wmove(fm,_fsm_towchar(nconv,filename),_fsm_towchar(nconv2,newname));
}

int fg_wmove(FS_MULTI *fm,const W_CHAR *filename, const W_CHAR *newname) 
{
#endif
	FS_VOLUMEINFO *vi;
	FS_DIRENTRY *de;
	FS_DIRENTRY *de2;
	FS_NAME fsname;

#if F_FILE_CHANGED_EVENT
	FS_NAME fsnameorig;
#endif
	int a,pathlen;
	unsigned short dirnum;
	unsigned short dirnum2;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */
	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);


	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* original not exist */

	if (de->secure&FSSEC_ATTR_READONLY) return F_SETLASTERROR(F_ERR_ACCESSDENIED);

	for (a=0; a<vi->maxfile; a++) if (vi->files[a].mode!=FS_FILE_CLOSE) 
	{
		if (vi->files[a].direntry==de) return F_SETLASTERROR(F_ERR_BUSY); 	   /* file is open  */
	}

	dirnum=fsname.dirnum;
    SAFE_Unused_Param = dirnum;
	pathlen=fsm_strlen(fsname.path);

#if F_FILE_CHANGED_EVENT
	/* copy original */
	fsnameorig=fsname; 
#endif
	if (_fg_setfsname(fm,newname,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */
	if (fsm_strlen(fsname.lname)+ pathlen >= FS_MAXPATH-4) return F_SETLASTERROR(F_ERR_TOOLONGNAME);/* invalid name */
	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);

	if ((_fg_find(vi,&fsname,&de2,&dirnum2))) return F_SETLASTERROR(F_ERR_DUPLICATED);   /* target file exists */

	if (!_fg_findpath(vi,&fsname)) return F_SETLASTERROR(F_ERR_NOTFOUND);

	if (_fg_setdename(fsname.lname,vi,de)) return F_SETLASTERROR(F_ERR_NOMOREENTRY);


	if (de->dirnum!=fsname.dirnum) 
	{
		dirnum2=de->dirnum;
		de->dirnum=fsname.dirnum;

		dirnum=fsname.dirnum;

		for (;;) 
		{ /*check recursivity */
			if (dirnum==0xffff) break; /* root */
			dirnum=vi->direntries[dirnum].dirnum;
			if (dirnum==0xffff) break; /* root */

			if (fsname.dirnum==dirnum) 
			{
				de->dirnum=dirnum2;

				_fsm_cacheaddde(vi,de);

				return F_SETLASTERROR(F_ERR_INVALIDDIR); /* recursion */
			}
		}
	}

#if F_FILE_CHANGED_EVENT
		if (f_filechangedevent)
		{
			ST_FILE_CHANGED fcold;
			ST_FILE_CHANGED fcnew;
			char fcvalid;

			fcvalid=1;

			fcold.action = FACTION_RENAMED_OLD_NAME;
			if (de->attr & FS_ATTR_DIR)
			{
				fcold.flags = FFLAGS_DIR_NAME;
			}
			else
			{
				fcold.flags = FFLAGS_FILE_NAME;
			}
			fcold.attr=_fg_getcompatibleattr(de);
			fcold.ctime=de->ctime;
			fcold.cdate=de->cdate;
			fcold.filesize=de->len;

			if (_fsm_createfullname(fcold.filename,sizeof(fcold.filename),fsnameorig.drivenum,fsnameorig.path,fsnameorig.lname))
			{
				fcvalid=0;
			}

			fcnew.action = FACTION_RENAMED_NEW_NAME;
			if (de->attr & FS_ATTR_DIR)
			{
				fcnew.flags = FFLAGS_DIR_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
			}
			else
			{
				fcnew.flags = FFLAGS_FILE_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
			}

			fcnew.attr=_fg_getcompatibleattr(de);
			fcnew.ctime=de->ctime;
			fcnew.cdate=de->cdate;
			fcnew.filesize=de->len;

			if (_fsm_createfullname(fcnew.filename,sizeof(fcnew.filename),fsname.drivenum,fsname.path,fsname.lname))
			{
				fcvalid=0;
			}

			if (fcvalid)
			{
				f_filechangedevent(&fcold);
				f_filechangedevent(&fcnew);
			}
		}
#endif
	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_filelength
 *
 * Get a file length
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - file whose length is needed
 *
 * RETURNS
 *
 * length of the file or -1 if any error
 *
 ***************************************************************************/

long fg_filelength(FS_MULTI *fm,const char *filename) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wfilelength(fm,_fsm_towchar(nconv,filename));
}

long fg_wfilelength(FS_MULTI *fm,const W_CHAR *filename) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&fsname)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return -1; /* invalid name */
	}
	if (fsm_checknamewc(fsname.lname)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return -1;/* invalid name */
	}
	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDDRIVE);
		return -1;
	}
	if (!(_fg_find(vi,&fsname,&de,0))) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTFOUND);
		return -1;	   /* not exist */
	}

	if (de->attr & FS_ATTR_DIR) 
	{
		F_SETLASTERROR_NORET(F_ERR_ACCESSDENIED);
		return -1;				/* directory */
	}

	F_SETLASTERROR_NORET(F_NO_ERROR);
	return (long)de->len; /* length of file is unsigned long but filelength return value is long*/
}

/****************************************************************************
 *
 * fg_delete
 *
 * delete a file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - file which wanted to be deleted (with or without path)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_delete(FS_MULTI *fm,const char *filename) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wdelete(fm,_fsm_towchar(nconv,filename));
}

int fg_wdelete(FS_MULTI *fm,const W_CHAR *filename) 
{
#endif
	unsigned short dirnum;
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;
	int a;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (_fg_isnamedots(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);
	if (!(_fg_find(vi,&fsname,&de,&dirnum))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	if (de->attr & FS_ATTR_DIR) return F_SETLASTERROR(F_ERR_INVALIDDIR);		   /* directory */
	if (de->secure&FSSEC_ATTR_READONLY) return F_SETLASTERROR(F_ERR_ACCESSDENIED);

	for (a=0; a<vi->maxfile; a++) if (vi->files[a].mode!=FS_FILE_CLOSE) 
	{
		if (vi->files[a].direntry==de) return F_SETLASTERROR(F_ERR_BUSY);			/* file is open  */
	}

	_fg_removedename(vi,de);

	de->attr=0;	/* removes it lfn */

	_fsm_cacheaddde(vi,de);

	_fg_setdiscsectors(vi,de->sector);

#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		fc.action = FACTION_REMOVED;
		fc.flags = FFLAGS_FILE_NAME;
		fc.attr=0;
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsname.drivenum,fsname.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}
	}
#endif

	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * _fg_storefilebuffer
 *
 * store buffer sector into device if modified
 *
 * INPUTS
 *
 * vi - volume info pointer
 * f - filepointer which file
 *
 * RETURNS
 *
 * error code
 *
 ***************************************************************************/

int _fg_storefilebuffer(FS_VOLUMEINFO *vi,FS_FILEINT *f) 
{
	int ret;

	if (f->modified) 
	{
		f->modified=0; /* modified is cleared even if it may written with error */

		ret=_fg_storesector(vi,f,f->buffer,vi->sectorsize);
		if (ret) 
		{
			_fg_cleanupfile(vi,f);
			return ret;
		}
	}

	return F_NO_ERROR;
}

/****************************************************************************
 *
 * _fg_fseek
 *
 * subfunction for fg_seek it moves position into given offset
 *
 * INPUTS
 *
 * vi - volume info
 * fm - multi structure pointer
 * f - FS_FILEINT structure which file position needed to be modified
 * offset - position from start
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

#if (!TI_COMPRESS)

int _fg_fseek(FS_VOLUMEINFO *vi,FS_MULTI *fm,FS_FILEINT *f, long offset) 
{
	int ret;
	long rem=0;

	if (offset<0) offset=0;
	if (offset>f->len) 
	{
		if (!(f->mode == FS_FILE_WR || f->mode == FS_FILE_RDP || f->mode == FS_FILE_A || f->mode == FS_FILE_AP || f->mode == FS_FILE_WRP)) return F_ERR_NOTOPEN;

		rem=offset-f->len;
		offset=f->len;
	}

	if (f->pos<=offset && offset<f->pos+vi->sectorsize && (!rem)) 
	{
		if (!(f->state&ST_FILE_LOADED)) 
		{	/* check if sector is loaded or not */
			if (_fg_getsector(vi,*f->sector,f->buffer,0,vi->sectorsize)) 
			{ /* if not loaded then load it */
  				_fg_cleanupfile(vi,f);
	  			return F_ERR_ONDRIVE;
  			}
  			f->state |= ST_FILE_LOADED; /* now this is loaded */
		}

		f->relpos=offset-f->pos;
		return F_NO_ERROR; /* no need to read any data */
	}
	else 
	{
		ret=_fg_storefilebuffer(vi,f);
		if (ret) return ret;
	}

	f->sector=&f->sectorstart;
	f->pos=0;
	f->relpos=0;

	while (offset) 
	{
		unsigned short nextsector=*f->sector;

		if (nextsector>=FS_FAT_EOF) 
		{
			return F_ERR_ONDRIVE;	
		}

		if (offset>=vi->sectorsize) 
		{
			offset-=vi->sectorsize;
			f->pos+=vi->sectorsize;
			f->sector=&vi->fatmirror[nextsector];		/* goto next in mirror */
		}
		else 
		{
			f->relpos=offset;
			break;
		}
	}

	ret=_fg_getsector(vi,*f->sector,f->buffer,0,vi->sectorsize);
	f->state|=ST_FILE_LOADED; /* sector is loaded */

	if (ret) 
	{
		_fg_cleanupfile(vi,f);
		return ret;
	}

	if (rem) 
	{ /* check if we seek beyond eof */
		fsm_memset(truncate_tmp,0,sizeof(truncate_tmp));

		while (rem) 
		{
			long size=rem;
			if (size>(long)sizeof(truncate_tmp)) size=(long)sizeof(truncate_tmp);

			if (size!=fg_write(fm,truncate_tmp,1,size,&f->file)) 
			{
				_fg_cleanupfile(vi,f);
				return F_ERR_ONDRIVE;
			}
			rem-=size;
		}
	}

	return F_NO_ERROR;
}

#endif
/****************************************************************************
 *
 * _fg_check_handle
 *
 * internal function it checks if a file handler is valid and converts it
 * into internal file handler
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which filehandle needs to be checked
 *
 * RETURNS
 *
 * 0 - if filehandle is not correct
 * FS_FILEINT structure pointer if successfully
 *
 ***************************************************************************/

FS_FILEINT *_fg_check_handle(FS_MULTI *fm,FS_FILE *filehandle) 
{
	FS_VOLUMEINFO *vi;
	FS_FILEINT *f;

	if (!filehandle) return 0;						  /* invalid handle */

	f=(FS_FILEINT *)filehandle;

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return 0; /* invalid drive */
	if (filehandle->reference<0 || filehandle->reference>=vi->maxfile) return 0; /* out of range */
	if (f!=(&vi->files[filehandle->reference])) return 0; /* invalid pointer */

	return f;
}

/****************************************************************************
 *
 * fg_seek
 *
 * moves position into given offset in given file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - FS_FILE structure which file position needed to be modified
 * offset - relative position
 * whence - where to calculate position (FS_SEEK_SET,FS_SEEK_CUR,FS_SEEK_END)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_seek(FS_MULTI *fm,FS_FILE *filehandle,long offset,long whence) 
{
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);
	FS_VOLUMEINFO *vi;

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (!(f->mode == FS_FILE_RD ||f->mode == FS_FILE_WR || f->mode == FS_FILE_RDP || f->mode == FS_FILE_A || f->mode == FS_FILE_AP || f->mode == FS_FILE_WRP)) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (f->state & ST_FILE_SYNC) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	switch (whence) 
	{
	case FS_SEEK_CUR:  return F_SETLASTERROR(_fg_fseek(vi,fm,f,f->pos+f->relpos+offset));
	case FS_SEEK_END:  return F_SETLASTERROR(_fg_fseek(vi,fm,f,f->len+offset));
	case FS_SEEK_SET:  return F_SETLASTERROR(_fg_fseek(vi,fm,f,offset));
	default: break;
	}

	return F_SETLASTERROR(F_ERR_NOTUSEABLE);
}

/****************************************************************************
 *
 * fg_tell
 *
 * Tells the current position of opened file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs the position
 *
 * RETURNS
 *
 * position in the file from start
 *
 ***************************************************************************/

long fg_tell(FS_MULTI *fm,FS_FILE *filehandle) 
{
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);

	if (!f) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	if (f->mode==FS_FILE_CLOSE) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	F_SETLASTERROR_NORET(F_NO_ERROR);
	return f->pos+f->relpos;
}

/****************************************************************************
 *
 * fg_eof
 *
 * Tells if the current position is end of file or not
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs the checking
 *
 * RETURNS
 *
 * 0 - if not EOF
 * other - if EOF or invalid file handle
 *
 ***************************************************************************/

int fg_eof(FS_MULTI *fm,FS_FILE *filehandle) 
{
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);  /* if error */

	if (f->appsync)
	{
		f->len=f->appsync->len;
	}

	if (f->pos+f->relpos<f->len) return F_SETLASTERROR(F_NO_ERROR);

	return F_SETLASTERROR(F_ERR_EOF);/* EOF */
}

/****************************************************************************
 *
 * fg_rewind
 *
 * set the fileposition in the opened file to the beginning
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs to be rewound
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_rewind(FS_MULTI *fm,FS_FILE *filehandle) 
{
	return fg_seek(fm, filehandle, 0L, FS_SEEK_SET);
}

/****************************************************************************
 *
 * fg_putc
 *
 * write a character into file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * ch - what to write into file
 * filehandle - file where to write
 *
 * RETURNS
 *
 * with written character, or -1 if any error
 *
 ***************************************************************************/

int fg_putc(FS_MULTI *fm,int ch,FS_FILE *filehandle) 
{
	unsigned char tmpch=(unsigned char)(ch);

	if (fg_write(fm,&tmpch,1,1,filehandle)==1) 
	{
		F_SETLASTERROR_NORET(F_NO_ERROR);
		return tmpch;
	}

	F_SETLASTERROR_NORET(F_ERR_WRITE);
	return -1;
}

/****************************************************************************
 *
 * fg_getc
 *
 * get a character from file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * with the read character or -1 if read was not successfully
 *
 ***************************************************************************/

int fg_getc(FS_MULTI *fm,FS_FILE *filehandle) 
{
	unsigned char ch;

	if (fg_read(fm,&ch,1,1,filehandle)==1) 
	{
		F_SETLASTERROR_NORET(F_NO_ERROR);
		return ch;
	}

	F_SETLASTERROR_NORET(F_ERR_READ);
	return -1;
}

/****************************************************************************
 *
 * _fg_checkfilelock
 *
 * this function is called from xxx_open to check if a file is locked for
 * the given mode. File could be opened for multiple read, but not for
 * multiple write
 *
 * INPUTS
 *
 * vi - volumeinfo
 * de - directory entry to check
 * m_mode - requested mode
 *
 * RETURNS
 *
 * 0 - if not locked
 * other if locked
 *
 ***************************************************************************/

int _fg_checkfilelock(const FS_VOLUMEINFO *vi,const FS_DIRENTRY *de,long m_mode) 
{
	int a;

	if (m_mode==FS_FILE_RD) 
	{
		for (a=0; a<vi->maxfile; a++) 
		{
			int mode=vi->files[a].mode;

			if (mode!=FS_FILE_CLOSE) 
			{
				if (vi->files[a].direntry==de) 
				{
					if (mode==FS_FILE_LOCKED) continue; /* locked entry */
					if (mode==FS_FILE_RDP || mode==FS_FILE_WR || mode==FS_FILE_WRP || mode==FS_FILE_A) return 1;
				}
			}
		}
		return 0;
	}

	if (m_mode==FS_FILE_RDP || m_mode==FS_FILE_WR || m_mode==FS_FILE_WRP || m_mode==FS_FILE_A || m_mode==FS_FILE_AP) 
	{
		for (a=0; a<vi->maxfile; a++) 
		{
			int mode=vi->files[a].mode;

			if (mode!=FS_FILE_CLOSE) 
			{
				if (vi->files[a].direntry==de) 
				{
					if (mode==FS_FILE_LOCKED) continue; /* locked entry */
					if ((m_mode == FS_FILE_AP) && (mode==FS_FILE_RD)) continue; /* skip a+ if r found*/
					return 1; /* cant be any */
				}
			}
		}
		return 0;
	}

	return 1; /* invalid mode */
}

/****************************************************************************
 *
 * _fg_checkappend
 *
 * this function is called from xxx_open to check if a file is already
 * opened for append (a+)
 *
 * INPUTS
 *
 * vi - volumeinfo
 * de - directory entry to check
 * m_mode - requested mode
 *
 * RETURNS
 *
 * 0 - if not found
 * file pointer value if found
 *
 ***************************************************************************/

FS_FILEINT *_fg_checkappend(const FS_VOLUMEINFO *vi,const FS_DIRENTRY *de,long m_mode) 
{
	int a;

  	if (m_mode==FS_FILE_RD) 
	{
		for (a=0; a<vi->maxfile; a++) 
		{
			int mode=vi->files[a].mode;

			if (mode==FS_FILE_AP) 
			{
				if (vi->files[a].direntry==de) 
				{
					vi->files[a].state|=ST_FILE_SYNC; /* set append file's state to sync*/
					return &vi->files[a];
				}
			}
		}
		return 0;
	}
	return 0;
}

/****************************************************************************
 *
 * _fg_checksyncpos
 *
 * this function search files with "r" and then updates sector, state and
 * buffer with "a+" file if it is necessary
 *
 * INPUTS
 *
 * vi - volumeinfo
 * f - filehandler of append file
 * remove - signal off appsync must be disconnected
 *
 * RETURNS
 *
 * if found and update any filehandle
 * no filehandle found
 *
 ***************************************************************************/

int _fg_checksyncpos(const FS_VOLUMEINFO *vi,FS_FILEINT *f,int remove)
{
	int a;
	int ret=0;

	for (a=0; a<vi->maxfile; a++) 
	{
		FS_FILEINT *frd=&vi->files[a];

		if (frd->mode==FS_FILE_RD) 
		{
			if (frd->appsync == f)
			{
				frd->sectorstart=f->sectorstart;
				if (f->pos == frd->pos)
				{
					frd->state &= ~ST_FILE_NEXTPROH;

					if (f->sector == &f->sectorstart)
						frd->sector=&frd->sectorstart;
					else 
						frd->sector = f->sector;


					if 	(f->state&ST_FILE_LOADED)
					{/* sector is loaded */
						fsm_memcpy(frd->buffer,f->buffer,vi->sectorsize);
					}
					else 
					{
						if (_fg_getsector(vi,*frd->sector,frd->buffer,0,vi->sectorsize)) 
						{ /* if exist ! */
							_fg_cleanupfile(vi,frd);
						}
					}

					frd->state |= ST_FILE_LOADED;

				}

				if (remove) 
				{
					frd->appsync=0;
				}

				/* signal r file(s) found 2008.05.09 */
				ret=1;
			}
		}
	}		

	return ret;
}

/****************************************************************************
 *
 * _fg_checkapwithr
 *
 * checking if any file exists with "r" till we are "a+" just opened
 *
 * INPUTS
 *
 * vi - volumeinfo structure
 * f - internal filehandle
 *
 * RETURN
 *
 * zero if there is no file with "r"
 * other if any of find
 *
 ***************************************************************************/

int _fg_checkapwithr(const FS_VOLUMEINFO *vi,FS_FILEINT *f)
{
	int a;
	int ret=0;

	for (a=0; a<vi->maxfile; a++) 
	{
		FS_FILEINT *frd=&vi->files[a];

		if (frd->mode==FS_FILE_RD) 
		{
			if (frd->direntry == f->direntry)
			{
				frd->appsync = f;
				f->state |= ST_FILE_SYNC;
				ret=1;
			}
		}
	}

	return ret;
}

/****************************************************************************
 *
 * f_calcfilecrc
 *
 * calculate file content crc
 *
 * INPUTS
 *
 * vi - volume info
 * f - internal FILE handler
 * pcrc32 - pointer where to store file crc
 *
 * RETURNS
 *
 * 0 - no error
 * other any error
 *
 ***************************************************************************/

#if (CRCONFILES)
static unsigned f_calcfilecrc(FS_VOLUMEINFO *vi,FS_FILEINT *f,unsigned long *pcrc32)
{
	unsigned short sector = f->sectorstart;
	unsigned long size = f->len;
	unsigned long dwcrc=FS_CRCINIT;

	for (;;)
	{
		if (!size) 
		{
			*pcrc32=dwcrc;
			return 0;
		}

		if (_fg_getsector(vi,sector,f->buffer,0,vi->sectorsize)) 
		{
			return 1;
		}

		if (size>=(unsigned long)vi->sectorsize)
		{
			dwcrc=fsm_calccrc32(dwcrc,f->buffer,vi->sectorsize);
			size-=vi->sectorsize;
		}
		else
		{
			dwcrc=fsm_calccrc32(dwcrc,f->buffer,size);
			size=0;
		}

		sector=vi->fatmirror[sector];
	}

}
#endif

/****************************************************************************
 *
 * fg_open
 *
 * open a file for reading/writing/appending
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file need to be opened
 * mode - string how to open ("r"-read, "w"-write, "w+"-overwrite, "a"-append
 *
 * RETURNS
 *
 * FS_FILE pointer if successfully
 * 0 - if any error
 *
 ***************************************************************************/

#if (!TI_COMPRESS)
FS_FILE *fg_open(FS_MULTI *fm,const char *filename,const char *mode) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	W_CHAR nconv2[FS_MAXPATH];
	return fg_wopen(fm,_fsm_towchar(nconv,filename),_fsm_towchar(nconv2,mode));
}

FS_FILE *fg_wopen(FS_MULTI *fm,const W_CHAR *filename,const W_CHAR *mode) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;
	int m_mode=FS_FILE_CLOSE;
	FS_FILEINT *f=0;
	int a,ret;
#if (CRCONFILES)
	int crcon=0;
#endif

	if (mode[0]=='r') 
	{
		if (mode[1]==0) m_mode=FS_FILE_RD;
		else if (mode[1]=='+' && mode[2]==0) m_mode=FS_FILE_RDP;
#if (CRCONFILES)
		else if (mode[1]=='!' && mode[2]==0)
		{
			m_mode=FS_FILE_RD;
			crcon=1;
		}
		else if (mode[1]=='+' && mode[2]=='!' && mode[3]==0)
		{
			m_mode=FS_FILE_RDP;
			crcon=1;
		}
#endif
	}
	else if (mode[0]=='w') 
	{
		if (mode[1]==0) m_mode=FS_FILE_WR;
		else if (mode[1]=='+' && mode[2]==0) m_mode=FS_FILE_WRP;
#if (CRCONFILES)
		else if (mode[1]=='!' && mode[2]==0)
		{
			m_mode=FS_FILE_WR;
			crcon=1;
		}
		else if (mode[1]=='+' && mode[2]=='!' && mode[3]==0)
		{
			m_mode=FS_FILE_WRP;
			crcon=1;
		}
#endif
	}
	else if (mode[0]=='a') 
	{
		if (mode[1]==0) m_mode=FS_FILE_A;
		else if (mode[1]=='+' && mode[2]==0) m_mode=FS_FILE_AP;
#if (CRCONFILES)
		else if (mode[1]=='!' && mode[2]==0)
		{
			m_mode=FS_FILE_A;
			crcon=1;
		}
		else if (mode[1]=='+' && mode[2]=='!' && mode[3]==0)
		{
			m_mode=FS_FILE_AP;
			crcon=1;
		}
#endif
	}

	if (m_mode==FS_FILE_CLOSE) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTUSEABLE);
		return 0; 			/* invalid mode */
	}

	if (_fg_setfsname(fm,filename,&fsname)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return 0; /* invalid name */
	}

	if (fsm_checknamewc(fsname.lname)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return 0;/* invalid name */
	}

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDDRIVE);
		return 0;
	}


	if (fs_mutex_get(&effs_gmutex)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_BUSY);
		return 0;
	}

	for (a=0; a<vi->maxfile; a++) 
	{
		if (vi->files[a].mode==FS_FILE_CLOSE) 
		{
			f=&vi->files[a];
			f->mode = FS_FILE_LOCKED; /* avoiding reentrance for allocation */
			break;
		}
	}

	(void)fs_mutex_put(&effs_gmutex);

	if (!f) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_NOMOREENTRY);
		return 0;		/* no more file could be opened  */
	}

	f->sector=0;
	f->drivenum=fsname.drivenum;
	f->file.reference=a;
	f->pos=0;
	f->relpos=0;
	f->len=0;

	f->discardstart=FS_FAT_EOF;					/* this will be discarded at close */
	f->discard=&f->discardstart;

	f->sectorstart=FS_FAT_EOF;
	f->sector=&f->sectorstart;					/* set sector pointer */
	f->modified=0;
	f->state=0;
	f->appsync=0;

	if (_fg_isnamedots(fsname.lname)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		f->mode=FS_FILE_CLOSE;
		return 0;
	}
#if F_FILE_CHANGED_EVENT
	if (_fsm_createfullname(f->filename,sizeof(f->filename),fsname.drivenum,fsname.path,fsname.lname))
	{
	 	F_SETLASTERROR_NORET(F_ERR_TOOLONGNAME);
	 	f->mode=FS_FILE_CLOSE; /* release fileentry */
		return 0;
	}
#endif

	switch (m_mode) 
	{
		case FS_FILE_RD: /* r */
		case FS_FILE_RDP: /* rp */
   			if (!(_fg_find(vi,&fsname,&de,0))) 
			{
			 	F_SETLASTERROR_NORET(F_ERR_NOTFOUND);
				f->mode=FS_FILE_CLOSE;
				return 0;	   /* not exist */
			}
			if (de->attr & FS_ATTR_DIR) 
			{
				F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
				f->mode=FS_FILE_CLOSE;
				return 0;		   /* directory */
			}

			if (m_mode==FS_FILE_RDP) 
			{
				if (de->secure&FSSEC_ATTR_READONLY) 
				{
					F_SETLASTERROR_NORET(F_ERR_ACCESSDENIED);
					f->mode=FS_FILE_CLOSE;
					return 0;
				}
			}

			if (_fg_checkfilelock(vi,de,m_mode)) 
			{
			 	F_SETLASTERROR_NORET(F_ERR_LOCKED);
				f->mode=FS_FILE_CLOSE;
				return 0;    /* locked */
			}

			/* check whether there is an open appended file */
			f->appsync=_fg_checkappend(vi,de,m_mode);
			if (!f->appsync)
			{
				if (_fg_copychainintomirror(vi,f,de)) 
				{
					_fg_cleanupfile(vi,f);
					F_SETLASTERROR_NORET(F_ERR_READ);
					f->mode=FS_FILE_CLOSE;
					return 0;
				}
			}
			else
			{
				f->direntry=de;
				f->sectorstart=f->appsync->sectorstart;
				f->len=f->appsync->len;
			}
#if (CRCONFILES)
			if (crcon)
			{
				unsigned long dwcrc;

				if (f_calcfilecrc(vi,f,&dwcrc))
				{
					_fg_cleanupfile(vi,f);
					f->mode=FS_FILE_CLOSE;
					F_SETLASTERROR_NORET(F_ERR_CRCERROR);
					return 0;
				}

				if (dwcrc != de->crc32)
				{
					_fg_cleanupfile(vi,f);
					f->mode=FS_FILE_CLOSE;
					F_SETLASTERROR_NORET(F_ERR_CRCERROR);
					return 0;
				}
			}
#endif

			if (_fg_getsector(vi,*f->sector,f->buffer,0,vi->sectorsize)) 
			{
				_fg_cleanupfile(vi,f);
				F_SETLASTERROR_NORET(F_ERR_READ);
				f->mode=FS_FILE_CLOSE;
				return 0; /* remove chain!! */
			}

			f->state|=ST_FILE_LOADED; /* sector is loaded */

	   		break;


   		case FS_FILE_A: /* a */
	   	case FS_FILE_AP: /* a+ */
			if (_fg_find(vi,&fsname,&de,0))  
			{
		   		if (de->attr & FS_ATTR_DIR) 
				{
					F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
					f->mode=FS_FILE_CLOSE;
					return 0;				/* directory */
				}
				if (de->secure&FSSEC_ATTR_READONLY) 
				{
					F_SETLASTERROR_NORET(F_ERR_ACCESSDENIED);
					f->mode=FS_FILE_CLOSE;
					return 0;
				}

				if (_fg_checkfilelock(vi,de,m_mode)) 
				{
				 	F_SETLASTERROR_NORET(F_ERR_LOCKED);
					f->mode=FS_FILE_CLOSE;
					return 0;    /* locked */
				}

			f->direntry=de; /* de must be set before calling _fg_checkapwithr */
				/* check if "r" opened and set the link */
				if (!_fg_checkapwithr(vi,f))
				{
					if (_fg_copychainintomirror(vi,f,de)) 
					{
						_fg_cleanupfile(vi,f);
					 	F_SETLASTERROR_NORET(F_ERR_READ);
						f->mode=FS_FILE_CLOSE;
						return 0;
					}
				}
				else 
				{
					/* f->direntry=de; */ /* de is already set */
					f->sectorstart=de->sector;
					f->len=de->len;
				}
#if (CRCONFILES)
				if (crcon)
				{
					unsigned long dwcrc;

					if (f_calcfilecrc(vi,f,&dwcrc))
					{
						_fg_cleanupfile(vi,f);
						f->mode=FS_FILE_CLOSE;
						F_SETLASTERROR_NORET(F_ERR_CRCERROR);
						return 0;
					}

					if (dwcrc != de->crc32)
					{
						_fg_cleanupfile(vi,f);
						f->mode=FS_FILE_CLOSE;
						F_SETLASTERROR_NORET(F_ERR_CRCERROR);
						return 0;
					}
				}
#endif

				f->pos=-vi->sectorsize; /* forcing seek to read 1st sector! abspos=0; */
				ret=_fg_fseek(vi,fm,f,de->len);
				if (ret)
				{
					_fg_cleanupfile(vi,f);
				 	F_SETLASTERROR_NORET(ret);
					f->mode=FS_FILE_CLOSE;
					return 0;
				}
			}
			else 
			{  /* if not exist creates it! */
				if (!_fg_findpath(vi,&fsname)) 
				{
				 	F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
					f->mode=FS_FILE_CLOSE;
					return 0; /* invalid path */
				}

	   			ret=_fg_addentry(vi,&fsname,&de); 
				if (ret)
				{
				 	F_SETLASTERROR_NORET(ret);
					f->mode=FS_FILE_CLOSE;
					return 0; /* couldn't be added	 */
				}
#if F_FILE_CHANGED_EVENT
				if (f_filechangedevent)
				{
					ST_FILE_CHANGED fc;

					fc.action = FACTION_ADDED;
					fc.flags = FFLAGS_FILE_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
					fc.attr=_fg_getcompatibleattr(de);
					fc.ctime=de->ctime;
					fc.cdate=de->cdate;
					fc.filesize=de->len;
					fsm_memcpy(fc.filename,f->filename,sizeof(fc.filename));

					f_filechangedevent(&fc);
				}
#endif
			}

   			break;

	   	case FS_FILE_WR: /* w */
   		case FS_FILE_WRP: /* w+ */
	   		if (_fg_find(vi,&fsname,&de,0)) 
			{		    /* exist can overwrite */
   				if (de->attr & FS_ATTR_DIR) 
				{
					F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
					f->mode=FS_FILE_CLOSE;
					return 0;				/* directory */
				}
				if (de->secure&FSSEC_ATTR_READONLY) 
				{
					F_SETLASTERROR_NORET(F_ERR_ACCESSDENIED);
					f->mode=FS_FILE_CLOSE;
					return 0;
				}

				if (_fg_checkfilelock(vi,de,m_mode)) 
				{
				 	F_SETLASTERROR_NORET(F_ERR_LOCKED);
					f->mode=FS_FILE_CLOSE;
					return 0;    /* locked */
				}

		   		if (de->sector!=FS_FAT_EOF) 
				{	/* if not eof */
   					_fg_setdiscsectors(vi,de->sector);		/* set sector as discarded */
   				}
				de->len=0;
				de->sector=FS_FAT_EOF;

				_fsm_cacheaddde(vi,de);

   			}
			else 
			{
				if (!_fg_findpath(vi,&fsname)) 
				{
					F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
					f->mode=FS_FILE_CLOSE;
					return 0; /* invalid path */
				}

   				ret=_fg_addentry(vi,&fsname,&de);
				if (ret) 
				{
					F_SETLASTERROR_NORET(ret);
					f->mode=FS_FILE_CLOSE;
					return 0; /* couldn't be added */
				}
#if F_FILE_CHANGED_EVENT
				if (f_filechangedevent)
				{
					ST_FILE_CHANGED fc;

					fc.action = FACTION_ADDED;
					fc.flags = FFLAGS_FILE_NAME | FFLAGS_ATTRIBUTES | FFLAGS_SIZE | FFLAGS_LAST_WRITE;
					fc.attr=_fg_getcompatibleattr(de);
					fc.ctime=de->ctime;
					fc.cdate=de->cdate;
					fc.filesize=de->len;

					fsm_memcpy(fc.filename,f->filename,sizeof(fc.filename));

					f_filechangedevent(&fc);
				}
#endif
			}

   			break;

		default: 
			F_SETLASTERROR_NORET(F_ERR_NOTUSEABLE);
			f->mode=FS_FILE_CLOSE;
			return 0; /* invalid mode */
	}

#if (CRCONFILES)
	if (crcon)
	{
		f->state|=ST_FILE_CRCON; 
	}
#endif
	f->direntry=de;
	f->mode=m_mode; /* set proper mode to file */

	F_SETLASTERROR_NORET(F_NO_ERROR);
	return &f->file;
}

#endif
/****************************************************************************
 *
 * fg_abortclose
 *
 * abort and close a previously opened file
 * last changing (from open or from the last flush operation) will be aborted
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs to be aborted
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_abortclose(FS_MULTI *fm,FS_FILE *filehandle)
{
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);
	FS_VOLUMEINFO *vi;

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	_fg_cleanupfile(vi,f);

	f->mode=FS_FILE_CLOSE;

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_close
 *
 * close a previously opened file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs to be closed
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_close(FS_MULTI *fm,FS_FILE *filehandle)
{
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);
	FS_VOLUMEINFO *vi;
	int ret;

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	switch (f->mode)
	{
		case FS_FILE_ABORT:
				f->mode=FS_FILE_CLOSE;
				ret=_fg_flush(vi);
				if (ret) return F_SETLASTERROR(ret);
				return F_SETLASTERROR(F_ERR_ONDRIVE); 	   /* it was aborted before */

		case FS_FILE_CLOSE:
				return F_SETLASTERROR(F_ERR_NOTOPEN);		   /* it was not open */

		case FS_FILE_RD:
				_fg_cleanupfile(vi,f);
				f->mode=FS_FILE_CLOSE;
				return F_SETLASTERROR(F_NO_ERROR);

		case FS_FILE_RDP:
		case FS_FILE_WR:
		case FS_FILE_WRP:
		case FS_FILE_A:
		case FS_FILE_AP:

				ret=_fg_storefilebuffer(vi,f);

				if (f->state & ST_FILE_SYNC)
				{
					if (!_fg_checksyncpos(vi,f,1)) 
					{
						f->state &= ~ST_FILE_SYNC;
					}
				}

				if (ret) 
				{
					f->mode=FS_FILE_CLOSE;
					return F_SETLASTERROR(ret);
				}

#if (CRCONFILES)
				if (f->state&ST_FILE_CRCON)
				{
					if (f_calcfilecrc(vi,f,&f->direntry->crc32))
					{
						f->mode=FS_FILE_CLOSE;
						return F_SETLASTERROR(F_ERR_CRCERROR); /* cannot be calculated */
					}
				}
#endif
				_fg_copymirrorchain(vi,f->sectorstart,f->state & ST_FILE_SYNC);  /* install chain */
				_fg_copydiscmirrorchain(vi,f->discardstart); /* set discardable if exist */

				f->direntry->sector=f->sectorstart; /* set start pos */
				f->direntry->len=f->len; /* set the size */

				fs_getcurrenttimedate(&f->direntry->ctime,&f->direntry->cdate);

				_fsm_cacheaddde(vi,f->direntry);
#if F_FILE_CHANGED_EVENT
				if (f_filechangedevent)
				{
					ST_FILE_CHANGED fc;
					FS_DIRENTRY *de=f->direntry;

					fc.action = FACTION_MODIFIED;

					fc.flags = FFLAGS_FILE_NAME | FFLAGS_LAST_WRITE | FFLAGS_SIZE;

					fc.attr=_fg_getcompatibleattr(de);
					fc.ctime=de->ctime;
					fc.cdate=de->cdate;
					fc.filesize=de->len;

					fsm_memcpy(fc.filename,f->filename,sizeof(fc.filename));

					f_filechangedevent(&fc);
				}
#endif

				f->mode=FS_FILE_CLOSE;
				return F_SETLASTERROR(_fg_flush(vi));
		default: 
				break;
	}

	f->mode=FS_FILE_CLOSE;
	return F_SETLASTERROR(F_ERR_NOTOPEN);
}

/****************************************************************************
 *
 * fg_flush
 *
 * flushing current content a file into physical.
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file needs to be flushed
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fg_flush(FS_MULTI *fm,FS_FILE *filehandle) 
{
	int ret;
	FS_VOLUMEINFO *vi;
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	switch (f->mode) 
	{
	case FS_FILE_RDP:
	case FS_FILE_WR:
	case FS_FILE_WRP:
	case FS_FILE_A:
	case FS_FILE_AP:
			ret=_fg_storefilebuffer(vi,f);
			if (ret) return F_SETLASTERROR(ret);

			_fg_copymirrorchainnoremove(vi,f->sectorstart);  /* install chain */
			_fg_copydiscmirrorchain(vi,f->discardstart); /* set discardable if exist */

			f->discard=&f->discardstart;
			f->discardstart=FS_FAT_EOF;   /* free up discard chain */

			f->direntry->sector=f->sectorstart; /* set start pos */
			f->direntry->len=f->len; /* set the size */

			fs_getcurrenttimedate(&f->direntry->ctime,&f->direntry->cdate);

			_fsm_cacheaddde(vi,f->direntry);

			ret=_fg_flush(vi);
			if (ret) 
			{
				_fg_cleanupfile(vi,f);
				return F_SETLASTERROR(ret);
			}

#if F_FILE_CHANGED_EVENT
			if (f_filechangedevent)
			{
				ST_FILE_CHANGED fc;
				FS_DIRENTRY *de=f->direntry;

				fc.action = FACTION_MODIFIED;

				fc.flags = FFLAGS_FILE_NAME | FFLAGS_LAST_WRITE | FFLAGS_SIZE;

				fc.attr=_fg_getcompatibleattr(de);
				fc.ctime=de->ctime;
				fc.cdate=de->cdate;
				fc.filesize=de->len;

				fsm_memcpy(fc.filename,f->filename,sizeof(fc.filename));

				f_filechangedevent(&fc);
			}
#endif
			return F_SETLASTERROR(F_NO_ERROR);
	default: 
			return F_SETLASTERROR(F_ERR_NOTOPEN);
	}

}

/****************************************************************************
 *
 * fg_settimedate
 *
 * set a file time and date
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file time and date wanted to be set
 * ctime - new ctime of the file
 * cdate - new cdate of the file
 *
 * RETURNS
 *
 * 0 - if successfully set
 * other - if any error
 *
 ***************************************************************************/

int fg_settimedate(FS_MULTI *fm,const char *filename,unsigned short ctime,unsigned short cdate) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wsettimedate(fm,_fsm_towchar(nconv,filename),ctime,cdate);
}

int fg_wsettimedate(FS_MULTI *fm,const W_CHAR *filename,unsigned short ctime,unsigned short cdate) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	de->ctime=ctime;
	de->cdate=cdate;

	_fsm_cacheaddde(vi,de);

#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		fc.action = FACTION_MODIFIED;

		if (de->attr & FS_ATTR_DIR)
		{
			fc.flags = FFLAGS_DIR_NAME | FFLAGS_LAST_WRITE;
		}
		else
		{
			fc.flags = FFLAGS_FILE_NAME | FFLAGS_LAST_WRITE;
		}

		fc.attr=_fg_getcompatibleattr(de);
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsname.drivenum,fsname.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}
	}
#endif
	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_gettimedate
 *
 * get a file time and date
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file time and date wanted to be retrieve
 * pctime - ctime of the file where to store
 * pcdate - cdate of the file where to store
 *
 * RETURNS
 *
 * 0 - if successfully get
 * other - if any error
 *
 ***************************************************************************/

int fg_gettimedate(FS_MULTI *fm,const char *filename,unsigned short *pctime,unsigned short *pcdate) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wgettimedate(fm,_fsm_towchar(nconv,filename),pctime,pcdate);
}

int fg_wgettimedate(FS_MULTI *fm,const W_CHAR *filename,unsigned short *pctime,unsigned short *pcdate) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (filename[0]=='.') 
	{
		if (filename[1]==0 || (filename[1]=='.' && filename[2]==0)) 
		{
			if (!_fg_findpath(vi,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDDIR);
			if (fsname.dirnum!=FS_DIR_ROOT) 
			{
				if (pctime) *pctime=vi->direntries[fsname.dirnum].ctime;
				if (pcdate) *pcdate=vi->direntries[fsname.dirnum].cdate;
				return F_SETLASTERROR(F_NO_ERROR);
			}
			return F_SETLASTERROR(F_ERR_INVALIDNAME);
		}
	}

	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

   if (pctime) *pctime=de->ctime;
   if (pcdate) *pcdate=de->cdate;

   return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_write
 *
 * write data into file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * buf - where the writing data is
 * size - size of items to be written
 * size_st - number of items need to be written
 * filehandle - file where to write
 *
 * RETURNS
 *
 * with the number of written items
 *
 ***************************************************************************/
#if (!TI_COMPRESS)

long fg_write(FS_MULTI *fm,const void *buf,long size,long size_st,FS_FILE *filehandle) 
{
	FS_VOLUMEINFO *vi;
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);
	long wrsize=0;
	long secfree;
	char *buffer=(char*)buf;
	long ws=size;

	size*=size_st;
	if (size<=0) 
	{
		F_SETLASTERROR_NORET(F_ERR_UNKNOWN);
		return 0; /* what to write? */
	}

	if (!f) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	if (!(f->mode == FS_FILE_WR || f->mode == FS_FILE_RDP || f->mode == FS_FILE_A || f->mode == FS_FILE_AP || f->mode == FS_FILE_WRP)) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDDRIVE);
		return 0;
	}

	secfree=vi->sectorsize-f->relpos;

	if (size<=secfree) 
	{
		fsm_memcpy (f->buffer+f->relpos,buffer,size);
		f->modified=1;
		f->relpos+=size;

		if (f->pos+f->relpos>f->len) f->len=f->pos+f->relpos;

		F_SETLASTERROR_NORET(F_NO_ERROR);
		return size/ws;
	}
	else 
	{
		if (secfree) 
		{
			fsm_memcpy (f->buffer+f->relpos,buffer,secfree);
			f->modified=1;
			size-=secfree;
			buffer+=secfree;
			wrsize+=secfree;
			f->relpos+=secfree;
			if (f->pos+f->relpos>f->len) f->len=f->pos+f->relpos;
		}


		for (;;) 
		{
			if (!size) 
			{
				F_SETLASTERROR_NORET(F_NO_ERROR);
				return wrsize/ws;
			}

			if (_fg_storefilebuffer(vi,f)) 
			{
				F_SETLASTERROR_NORET(F_ERR_WRITE);
				return 0;
			}

			if (*f->sector==FS_FAT_EOF) 
			{ /* error! */
				_fg_cleanupfile(vi,f);
				F_SETLASTERROR_NORET(F_ERR_READ);
				return 0;
			}

			if (f->state & ST_FILE_SYNC)
			{
				/* if sync then check if any other file has the same sector */
				(void)_fg_checksyncpos(vi,f,0);
			}

			f->sector=&vi->fatmirror[*f->sector]; 	/* goto next in mirror, but don't load at this time  */

			f->pos+=f->relpos;
			f->relpos=0;
			if (f->pos+f->relpos>f->len) f->len=f->pos+f->relpos;

			if (size>=vi->sectorsize) 
			{
				if ( !(((long)buffer) & 3) ) 
				{ /* if buffer is aligned, then it can be passed to lower level */
					int ret=_fg_storesector(vi,f,buffer,vi->sectorsize);  /* pass user buffer directly */
					f->modified=0;
					if (ret) 
					{
						_fg_cleanupfile(vi,f);
						F_SETLASTERROR_NORET(F_ERR_WRITE);
						return 0;
					}
					f->state &= ~ST_FILE_LOADED; /* signal that sector is not loaded */
				}
				else 
				{
					fsm_memcpy (f->buffer,buffer,vi->sectorsize); /* copy data into file buffer */
					f->modified=1;
					f->state|=ST_FILE_LOADED; /* sector is loaded <has current value> */
				}

				size-=vi->sectorsize;
				buffer+=vi->sectorsize;
				wrsize+=vi->sectorsize;

				f->relpos=vi->sectorsize;
				if (f->pos+f->relpos>f->len) f->len=f->pos+f->relpos;
			}
			else 
			{
				if (_fg_getsector(vi,*f->sector,f->buffer,0,vi->sectorsize)) 
				{ /* if exist ! */
					_fg_cleanupfile(vi,f);
					F_SETLASTERROR_NORET(F_ERR_READ);
					return 0;
				}
				f->state|=ST_FILE_LOADED; /* sector is loaded */


				fsm_memcpy (f->buffer,buffer,size);
				f->modified=1;

				f->relpos=size;
				if (f->pos+f->relpos>f->len) f->len=f->pos+f->relpos;

				wrsize+=size;
				F_SETLASTERROR_NORET(F_NO_ERROR);
				return wrsize/ws;
			}
		}
	}
}

#endif
/****************************************************************************
 *
 * fg_read
 *
 * read data from file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * buf - where the store data
 * size - size of items to be read
 * size_st - number of items need to be read
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * with the number of read items
 *
 ***************************************************************************/

#if (!TI_COMPRESS)

long fg_read(FS_MULTI *fm,void *buf,long size,long size_st,FS_FILE *filehandle) 
{
	FS_VOLUMEINFO *vi;
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);
	long rdsize=0;
	long secfree;
	char *buffer=(char*)buf;
	long rs=size;

	if (!f) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	if (!(f->mode == FS_FILE_RD || f->mode == FS_FILE_RDP || f->mode == FS_FILE_AP || f->mode == FS_FILE_WRP)) 
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	if (f->state & ST_FILE_SYNC)
	{
		F_SETLASTERROR_NORET(F_ERR_NOTOPEN);
		return 0;
	}

	size*=size_st;
	if (size<=0) 
	{
		F_SETLASTERROR_NORET(F_ERR_UNKNOWN);
		return 0;	/* if nothing need to read return */
	}

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) 
	{
		F_SETLASTERROR_NORET(F_ERR_INVALIDDRIVE);
		return 0;
	}

	if (f->appsync)
	{
		f->len=f->appsync->len;
	}

	if (f->relpos+f->pos+size >= f->len) 
	{   /* read len longer than the file */
		size=(f->len)-((f->relpos)+(f->pos));  /* calculate new size */
	}

	if (size<=0) 
	{
		F_SETLASTERROR_NORET(F_ERR_UNKNOWN);
		return 0;	/* if nothing need to read return */
	}

	secfree=vi->sectorsize-f->relpos;

	/* if read size is in filebuffer */
	if (size<=secfree) 
	{
		if (!f->appsync)
		{
			fsm_memcpy(buffer,f->buffer+f->relpos,size);
		}
		else 
		{
			if (f->appsync->pos==f->pos)
			{
				fsm_memcpy(buffer,f->appsync->buffer+f->relpos,size);
			}
			else
			{
				fsm_memcpy(buffer,f->buffer+f->relpos,size);
			}
		}

		f->relpos+=size;
		F_SETLASTERROR_NORET(F_NO_ERROR);
		return size/rs;
	}
	else 
	{
		/* if filepointer is in the middle of filebuffer */
		if (secfree) 
		{
			if (!f->appsync)
			{
				fsm_memcpy(buffer,f->buffer+f->relpos,secfree);
			}
			else 
			{
				if (f->appsync->pos==f->pos)
				{
					fsm_memcpy(buffer,f->appsync->buffer+f->relpos,secfree);
				}
				else
				{
					fsm_memcpy(buffer,f->buffer+f->relpos,secfree);
				}
			}

			rdsize+=secfree;
			buffer+=secfree;
			size-=secfree;
		}

		if (f->modified) 
		{ /* because of write */
			f->modified=0; /* modified is cleared even if it may written with error */
			if (_fg_storesector(vi,f,f->buffer,vi->sectorsize)) 
			{
				_fg_cleanupfile(vi,f);
				F_SETLASTERROR_NORET(F_ERR_WRITE);
				return 0;
			}
		}

		if (f->state&ST_FILE_NEXTPROH)
		{
		 	F_SETLASTERROR_NORET(F_ERR_READ);
		 	return 0; /* how to read next? */
		}

		while (size) 
		{
			f->pos+=vi->sectorsize;
			f->relpos=0;

			if (*f->sector>=FS_FAT_EOF) 
			{
				F_SETLASTERROR_NORET(F_ERR_READ);
				return rdsize/rs; /* how to read next? */
			}

			f->sector=&vi->fatmirror[*f->sector]; 	/* goto next in mirror */

			if (*f->sector>=FS_FAT_EOF) 
			{
				if (f->appsync)
				{
					if (f->pos==f->appsync->pos)
					{
						fsm_memcpy(buffer,f->appsync->buffer,size);
						f->relpos=size;
						rdsize+=size;
						f->state |= ST_FILE_NEXTPROH;
		
						F_SETLASTERROR_NORET(F_NO_ERROR);
						return rdsize/rs;
					}
				}
				F_SETLASTERROR_NORET(F_ERR_READ);
				return rdsize/rs; /* how to read current? */
			}


			if (size>=vi->sectorsize) 
			{
				/* check if size is bigger then sector size, in this case use user buffer to load directly */
				if (_fg_getsector(vi,*f->sector,buffer,0,vi->sectorsize)) 
				{
					_fg_cleanupfile(vi,f);
					F_SETLASTERROR_NORET(F_ERR_READ);
					return 0;
				}

				f->state &=~ST_FILE_LOADED; /* signal sector was not loaded */
				rdsize+=vi->sectorsize;
				buffer+=vi->sectorsize;
				f->relpos=vi->sectorsize;   /* we are at the end of sector */
				size-=vi->sectorsize;

				continue;
			}

			if (_fg_getsector(vi,*f->sector,f->buffer,0,vi->sectorsize)) 
			{
				_fg_cleanupfile(vi,f);
				F_SETLASTERROR_NORET(F_ERR_READ);
				return rdsize/rs;
			}
			f->state|=ST_FILE_LOADED;

			if (size>=vi->sectorsize) 
			{
				fsm_memcpy(buffer,f->buffer,vi->sectorsize);

				rdsize+=vi->sectorsize;
				buffer+=vi->sectorsize;
				f->relpos=vi->sectorsize;
				size-=vi->sectorsize;
			}
			else 
			{
				fsm_memcpy(buffer,f->buffer,size);

				rdsize+=size;
				f->relpos=size;

				F_SETLASTERROR_NORET(F_NO_ERROR);
				return rdsize/rs;
			}
		}
	}

	F_SETLASTERROR_NORET(F_NO_ERROR);
	return rdsize/rs;
}

#endif
/****************************************************************************
 *
 * fg_getfreespace
 *
 * get free diskspace
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - which drive free space is requested (0-A, 1-B, 2-C)
 * space - structure to fill space information
 *
 * RETURNS
 * 0 - if successful
 * other - see error codes
 *
 ***************************************************************************/

int fg_getfreespace(FS_MULTI *fm,int drivenum, FS_SPACE *space) 
{
	FS_VOLUMEINFO *vi;
	unsigned long freespace=0;
	unsigned long bad=0;
	unsigned long used=0;
	long a;
	int ret;

	ret=_fg_getvolumeinfo(fm,drivenum,&vi);
	if (ret) return F_SETLASTERROR(ret);

	for (a=0; a<vi->maxsectornum; a++) 
	{
		if (fsm_checksectorfree(vi,a)) freespace+=(unsigned long)vi->sectorsize;
		else if (fsm_checksectorbad(vi,a)) bad+=(unsigned long)vi->sectorsize;
		else used+=(unsigned long)vi->sectorsize;
	}

	space->total=(unsigned long)(vi->maxsectornum*vi->sectorsize-vi->reserved);


	if (freespace<vi->reserved)	space->free=0;
	else space->free=freespace-vi->reserved;

	space->used=used;
	space->bad=bad;

	/* add some correction because of system used space */
	if (space->bad+space->used+space->free!=space->total)
	{
		space->used=space->total-(space->free+space->bad);
	}

#if(USE_VFS)

    /* Get sector size for wrapper layer.
      vi->sectorsize is set by the devices mount function. */
    space->sec_size = vi->sectorsize;

    /* vi-flash is by default set to zero, by fg_mountdrive. It is 
       only reassigned by physical device initialization functions. */
    if(vi->flash == NU_NULL)
    {
        /* Using a ramdisk device. File system assumes a one to one 
          relationship between blocks and sectors when using ramdisk. */
        space->sec_per_blk = SAFE_RD_SEC_PER_CL;
    }
    else
    {
        /* Using a flash device. */
        space->sec_per_blk = vi->flash->sectorperblock;
    }

#endif

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_format
 *
 * format a volume (deletes every data)
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - which drive need to be formatted
 *
 * RETURNS
 *
 * 0 - if successfully formatted
 * other - if any error
 *
 ***************************************************************************/

int fg_format(FS_MULTI *fm,int drivenum) 
{
	FS_VOLUMEINFO *vi;
	int ret;

	ret=_fg_getvolumeinfo(fm,drivenum,&vi);

	if (ret==F_NO_ERROR || ret==F_ERR_NOTFORMATTED) 
	{
        long b;

		for (b=0; b<vi->maxfile; b++) 
		{
			vi->files[b].mode=FS_FILE_CLOSE; /* close all files; */
		}

		for (b=0; b<vi->maxsectornum; b++) 
		{
			vi->fatmirror[b]=FS_FAT_FREE;
		}


		if (ret==F_ERR_NOTFORMATTED) vi->resetwear=1; /* reset wear leveling if not useful */
		else vi->resetwear=0;					   /* keep wear leveling if it was used with not error */

		fg_filesystem.vd[drivenum].state=FS_VOL_NOTFORMATTED;

		if (fg_filesystem.vd[drivenum].format(vi)) return F_SETLASTERROR(F_ERR_NOTFORMATTED); /* call procedure */

		vi->cwd[0]=0; /* set working folder to root; */
		fg_filesystem.vd[drivenum].state=FS_VOL_OK;

		return F_SETLASTERROR(F_NO_ERROR);
	}

	return F_SETLASTERROR(F_ERR_INVALIDDRIVE);
}

/****************************************************************************
 *
 * fg_findfirst
 *
 * find a file(s) or directory(s) in directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - filename (with or without wildcards)
 * find - where to store found file information
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error or not found
 *
 ***************************************************************************/

int fg_findfirst(FS_MULTI *fm,const char *filename,FS_FIND *find) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	FS_WFIND find2;
	int ret;

	ret=fg_wfindfirst(fm,_fsm_towchar(nconv,filename),&find2);
	if (!ret) 
	{
		find->attr=find2.attr;
		_fsm_fromwchar(find->filename,find2.filename);

		find->ctime=find2.ctime;
		find->cdate=find2.cdate;

		find->filesize=find2.filesize;

		find->secure=find2.secure;

		find->findpos=find2.findpos;
		find->findfsname=find2.findfsname;
	}
	return ret;
}

int fg_wfindfirst(FS_MULTI *fm,const W_CHAR *filename,FS_WFIND *find) 
{
#endif
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&find->findfsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checkname(find->findfsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name, wildcard is ok */

	if (_fg_getvolumeinfo(fm,find->findfsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (!_fg_findpath(vi,&find->findfsname)) return F_SETLASTERROR(F_ERR_INVALIDDIR); /* search for path */

	find->findpos=0;

	if (find->findfsname.dirnum!=FS_DIR_ROOT) 
	{
#ifdef SAFE_UNICODE
		W_CHAR dots[3];
		dots[0]='.';
		dots[1]=0;

		if (_fg_namecheckwc(find->findfsname.lname,dots)) 
#else
		if (_fg_namecheckwc(find->findfsname.lname,(char*)".")) 
#endif
		{
			find->attr=FS_ATTR_DIR; /* set dir */

			find->ctime=vi->direntries[find->findfsname.dirnum].ctime;
			find->cdate=vi->direntries[find->findfsname.dirnum].cdate;

			find->filesize=0;
			find->secure=0;

			find->filename[0]='.';
			find->filename[1]=0;

			if (find->attr&FS_ATTR_DIR) find->secure|=FSSEC_ATTR_DIR;
			else 
			{
			   find->secure&=~FSSEC_ATTR_DIR;
			   find->secure^=FSSEC_ATTR_ARC;
			}

			find->findpos=0xffff; /* signal .. is needed*/

			return F_SETLASTERROR(F_NO_ERROR);
		}

	 	find->findpos=0xffff; /* signal .. is needed*/
	}

#ifdef SAFE_UNICODE
	return fg_wfindnext(fm,find);
#else
	return fg_findnext(fm,find);
#endif
}

/****************************************************************************
 *
 * fg_findnext
 *
 * find further file(s) or directory(s) in directory
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * find - where to store found file information (findfirst should call 1st)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error or not found
 *
 ***************************************************************************/

int fg_findnext(FS_MULTI *fm,FS_FIND *find) 
{
#ifdef SAFE_UNICODE
	FS_WFIND find2;
	int ret;

	find2.findpos=find->findpos;
	find2.findfsname=find->findfsname;

	ret=fg_wfindnext(fm,&find2);
	if (!ret) 
	{
		find->attr=find2.attr;
		_fsm_fromwchar(find->filename,find2.filename);

		find->ctime=find2.ctime;
		find->cdate=find2.cdate;

		find->filesize=find2.filesize;

		find->secure=find2.secure;

		find->findpos=find2.findpos;
		find->findfsname=find2.findfsname;

		if (find->attr&FS_ATTR_DIR) find->secure|=FSSEC_ATTR_DIR;
		else 
		{
		   find->secure&=~FSSEC_ATTR_DIR;
		   find->secure^=FSSEC_ATTR_ARC;
		}
	}
	return ret;
}

int fg_wfindnext(FS_MULTI *fm,FS_WFIND *find) 
{
#endif
	unsigned short dirnum;
	FS_VOLUMEINFO *vi;
	FS_DIRENTRY *de;

	if (_fg_getvolumeinfo(fm,find->findfsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (find->findpos==0xffff) 
	{
#ifdef SAFE_UNICODE
		W_CHAR dots[3];
		dots[0]='.';
		dots[1]='.';
		dots[2]=0;
		if (_fg_namecheckwc(find->findfsname.lname,dots)) 
#else
		if (_fg_namecheckwc(find->findfsname.lname,(char*)"..")) 
#endif
		{
			find->attr=FS_ATTR_DIR; /* set dir */

			find->ctime=vi->direntries[find->findfsname.dirnum].ctime;
			find->cdate=vi->direntries[find->findfsname.dirnum].cdate;

			find->filesize=0;
			find->secure=0;

			find->filename[0]='.';
			find->filename[1]='.';
			find->filename[2]=0;

			if (find->attr&FS_ATTR_DIR) find->secure|=FSSEC_ATTR_DIR;
			else 
			{
			   find->secure&=~FSSEC_ATTR_DIR;
			   find->secure^=FSSEC_ATTR_ARC;
			}

			find->findpos=0; /* signal .. is not needed*/
			return F_SETLASTERROR(F_NO_ERROR);
		}

		find->findpos=0; /* signal .. is not needed*/
	}

	if (!_fg_findfilewc(vi,find->findfsname.lname,find->findfsname.dirnum,&dirnum,find->findpos)) return F_SETLASTERROR(F_ERR_NOTFOUND);

	de=&vi->direntries[dirnum];

	_fg_getdename(find->filename,vi,de);

	find->attr=(char)(de->attr & FS_ATTR_DIR);
	find->cdate=de->cdate;
	find->ctime=de->ctime;
	find->filesize=de->len;
	find->secure=de->secure;

	find->findpos=(unsigned short)(dirnum+1);

	if (find->attr&FS_ATTR_DIR) find->secure|=FSSEC_ATTR_DIR;
	else 
	{
	   find->secure&=~FSSEC_ATTR_DIR;
	   find->secure^=FSSEC_ATTR_ARC;
	}

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_getpermission
 *
 * get a file permission
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file/directory permission wanted to be retrieve
 * psecure - pointer where to store permission
 *
 * RETURNS
 *
 * 0 - if successfully get
 * other - if any error
 *
 ***************************************************************************/

int fg_getpermission(FS_MULTI *fm,const char *filename, unsigned long *psecure) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wgetpermission(fm,_fsm_towchar(nconv,filename),psecure);
}

int fg_wgetpermission(FS_MULTI *fm,const W_CHAR *filename, unsigned long *psecure) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */
	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	if (psecure) 
	{
		*psecure=de->secure;
		if (de->attr&FS_ATTR_DIR) *psecure|=FSSEC_ATTR_DIR;       
		else 
		{
			*psecure&=~FSSEC_ATTR_DIR;
			*psecure^=FSSEC_ATTR_ARC;
		}
	}

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_setpermission
 *
 * set a file permission
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file/directory permission wanted to be retrieve
 * secure - permission what to set for file
 *
 * RETURNS
 *
 * 0 - if successfully get
 * other - if any error
 *
 ***************************************************************************/

int fg_setpermission(FS_MULTI *fm,const char *filename, unsigned long secure) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wsetpermission(fm,_fsm_towchar(nconv,filename),secure);
}

int fg_wsetpermission(FS_MULTI *fm,const W_CHAR *filename, unsigned long secure) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */
	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	secure&=FSSEC_ATTR_HIDDEN|FSSEC_ATTR_READONLY|FSSEC_ATTR_ARC|FSSEC_ATTR_SYSTEM|FSSEC_ATTR_USER; /* keep only valid bits */
	if (!(de->attr & FS_ATTR_DIR))
	{
		secure^=FSSEC_ATTR_ARC;
	}

	de->secure=secure;

	_fsm_cacheaddde(vi,de);

#if F_FILE_CHANGED_EVENT
	if (f_filechangedevent)
	{
		ST_FILE_CHANGED fc;

		fc.action = FACTION_MODIFIED;

		if (de->attr & FS_ATTR_DIR)
		{
			fc.flags = FFLAGS_DIR_NAME | FFLAGS_ATTRIBUTES;
		}
		else
		{
			fc.flags = FFLAGS_FILE_NAME | FFLAGS_ATTRIBUTES;
		}

		fc.attr=_fg_getcompatibleattr(de);
		fc.ctime=de->ctime;
		fc.cdate=de->cdate;
		fc.filesize=de->len;

		if (!_fsm_createfullname(fc.filename,sizeof(fc.filename),fsname.drivenum,fsname.path,fsname.lname))
		{
			f_filechangedevent(&fc);
		}
	}
#endif
	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_getlabel
 *
 * get a label of a media
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - drive number which labels is needed
 * label - char pointer where to store label
 * len - length of label buffer
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_getlabel(FS_MULTI *fm,int drivenum, char *label, long len) 
{
	FS_VOLUMEINFO *vi;
	char *l;
	long pos;
	unsigned int a;

	if (_fg_getvolumeinfo(fm,drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	l="NO LABEL";
	for (a=0; a<vi->maxdirentry; a++) 
	{
		FS_DIRENTRY *de=&vi->direntries[a];

		if (!(de->attr&FS_ATTR_DE)) continue; /* check if de is used for direntry */

		if (de->dirnum!=FS_DIR_LABEL) continue;

		l=(char*)(de->lname);
		break;
	}

	len--;
	for (pos=0; pos<len; pos++) 
	{
		char ch=*l++;
		if (!ch) break;
		*label++=ch;
	}

	*label=0; /* terminate it */
	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_setlabel
 *
 * set a label of a media
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - drive number which babel's need to be set
 * label - new label for the media
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_setlabel(FS_MULTI *fm,int drivenum, const char *label) 
{
	unsigned int a;
	FS_VOLUMEINFO *vi;
	char *l=0;
	int pos;
	int len=CFG_NU_OS_STOR_FILE_FS_SAFE_MAX_DIRENTRY_NAME;

	if (_fg_getvolumeinfo(fm,drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	for (a=0; a<vi->maxdirentry; a++) 
	{
		FS_DIRENTRY *de=&vi->direntries[a];

		if (!(de->attr&FS_ATTR_DE)) continue; /* check if de is used for direntry */

		if (de->dirnum!=FS_DIR_LABEL) continue;

		l=(char*)(de->lname);
		break;
	}

	if (l==0) 
	{ /* there is no existing entry */
		for (a=0; a<vi->maxdirentry; a++) 
		{
			FS_DIRENTRY *de=&vi->direntries[a];
			if (!de->attr) 
			{ /* not used */
				de->attr=FS_ATTR_DE;

				de->dirnum=FS_DIR_LABEL;
				de->len=0;
				de->sector=FS_FAT_EOF;
				de->secure=0; /* nobody allows to access */

				_fsm_cacheaddde(vi,de);

				l=(char*)(de->lname);
				break;
			}
		}
	}

	if (l==0) return F_SETLASTERROR(F_ERR_NOMOREENTRY);

	len--;
	for (pos=0; pos<len; pos++) 
	{
		char ch=*label++;
		if (!ch) break;
		*l++=ch;
	}
	*l=0; /* terminate it */
	return F_SETLASTERROR(_fg_flush(vi));
}

/****************************************************************************
 *
 * fg_truncate
 *
 * truncate a file to a specified length, filepointer will be set to the
 * end
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file need to be truncated
 * length - length of new file
 *
 * RETURNS
 *
 * filepointer or zero if fails
 *
 ***************************************************************************/

FS_FILE *fg_truncate(FS_MULTI *fm,const char *filename, unsigned long length) 
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wtruncate(fm,_fsm_towchar(nconv,filename),length);
}

FS_FILE *fg_wtruncate(FS_MULTI *fm,const W_CHAR *filename,unsigned long length) 
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;
	int m_mode=FS_FILE_WRP;
	FS_FILEINT *f=0;
	int a;
#ifndef SAFE_UNICODE
	if (length==0) 
	{
		return fg_open(fm,filename,(char*)"w+");
	}
#else
	if (length==0) 
	{
		W_CHAR m[3];
		m[0]='w';
		m[1]='+';
		m[2]=0;
		return fg_wopen(fm,filename,m);
	}
#endif

	if (_fg_setfsname(fm,filename,&fsname)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return 0; /* invalid name */
	}
	if (fsm_checknamewc(fsname.lname)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		return 0;/* invalid name */
	}

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDDRIVE);
		return 0; /* cant open any */
	}

	if (fs_mutex_get(&effs_gmutex)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_BUSY);
		return 0;
	}

	for (a=0; a<vi->maxfile; a++) 
	{
		if (vi->files[a].mode==FS_FILE_CLOSE) 
		{
			f=&vi->files[a];
			f->mode = FS_FILE_LOCKED; /* avoiding reentrance for allocation */
			break;
		}
	}

	(void)fs_mutex_put(&effs_gmutex);

	if (!f) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_NOMOREENTRY);
		return 0;		/* no more file could be opened  */
	}

	f->drivenum=fsname.drivenum;
	f->relpos=0;
	f->pos=0;
	f->len=0;
	f->modified=0;

	f->discardstart=FS_FAT_EOF;					/* this will be discarded at close */
	f->discard=&f->discardstart;

	f->sectorstart=FS_FAT_EOF;
	f->sector=&f->sectorstart;					/* set sector pointer */
	f->state=0;
	f->appsync=0;

	if (_fg_isnamedots(fsname.lname)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDNAME);
		f->mode = FS_FILE_CLOSE; 
		return 0;
	}

	if (!(_fg_find(vi,&fsname,&de,0))) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
		f->mode = FS_FILE_CLOSE; 
		return 0;	   /* not exist */
	}
#if F_FILE_CHANGED_EVENT
	if (_fsm_createfullname(f->filename,sizeof(f->filename),fsname.drivenum,fsname.path,fsname.lname))
	{
	 	F_SETLASTERROR_NORET(F_ERR_TOOLONGNAME);
	 	f->mode=FS_FILE_CLOSE; /* release fileentry */
		return 0;
	}
#endif

  	if (de->attr & FS_ATTR_DIR) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_INVALIDDIR);
		f->mode = FS_FILE_CLOSE; 
		return 0;		   /* directory */
	}

	if (de->secure&FSSEC_ATTR_READONLY) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_ACCESSDENIED);
		f->mode = FS_FILE_CLOSE; 
		return 0;
	}

	if (_fg_checkfilelock(vi,de,m_mode)) 
	{
	 	F_SETLASTERROR_NORET(F_ERR_LOCKED);
		f->mode = FS_FILE_CLOSE; 
		return 0;    /* locked */
	}

	if (_fg_copychainintomirror(vi,f,de)) 
	{
		_fg_cleanupfile(vi,f);
	 	F_SETLASTERROR_NORET(F_ERR_READ);
		f->mode = FS_FILE_CLOSE; 
		return 0;
	}

	if (f->len) 
	{
		unsigned long slen;

		f->pos=-vi->sectorsize; /* forcing seek to read 1st sector! abspos=0; */

		if (length>(unsigned long)f->len) slen=(unsigned long)f->len;
		else slen=length;

		if (_fg_fseek(vi,fm,f,(long)slen)) 
		{
			_fg_cleanupfile(vi,f);
		 	F_SETLASTERROR_NORET(F_ERR_READ);
			f->mode = FS_FILE_CLOSE; 
			return 0;
		}
	}
	else 
	{
		f->pos=0;
	}

	if (length<(unsigned long)f->len) 
	{
		if (*f->sector!=FS_FAT_EOF) 
		{
			if (vi->fatmirror[*f->sector]!=FS_FAT_EOF) 
			{
				f->discardstart=vi->fatmirror[*f->sector];
				f->discard=&vi->fatmirror[*f->sector];
				for (;;) 
				{
					unsigned short nextsec=*f->discard;
					if (nextsec==FS_FAT_EOF) break;
					f->discard=&vi->fatmirror[nextsec];
				}
				vi->fatmirror[*f->sector]=FS_FAT_EOF;
			}
		}

		f->len=(long)length;
	}
	else if (length>(unsigned long)f->len) 
	{
		long rem=(long)(length-(unsigned long)f->len);

		f->mode=m_mode; /*  lock it  */
	    fsm_memset(truncate_tmp,0,sizeof(truncate_tmp));

		while (rem) 
		{
			long size=rem;
			if (size>(long)sizeof(truncate_tmp)) size=(long)sizeof(truncate_tmp);

			if (size!=fg_write(fm,truncate_tmp,1,size,&f->file)) 
			{
				_fg_cleanupfile(vi,f);
			 	F_SETLASTERROR_NORET(F_ERR_WRITE);
				f->mode = FS_FILE_CLOSE; 
				return 0;
			}
			rem-=size;
		}
	}

	f->direntry=de;
	f->mode=m_mode; /*  set proper mode for lock it  */

 	F_SETLASTERROR_NORET(F_NO_ERROR);
	return &f->file;
}

/****************************************************************************
 *
 * fg_seteof
 *
 * set end of file at the current position
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_seteof(FS_MULTI *fm,FS_FILE *filehandle) 
{
	FS_VOLUMEINFO *vi;
	FS_FILEINT *f=_fg_check_handle(fm,filehandle);

	if (!f) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (!(f->mode == FS_FILE_RDP || f->mode == FS_FILE_WR || f->mode == FS_FILE_WRP)) return F_SETLASTERROR(F_ERR_NOTOPEN);

	if (_fg_getvolumeinfo(fm,f->drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE);

	if (*f->sector!=FS_FAT_EOF) 
	{
		if (vi->fatmirror[*f->sector]!=FS_FAT_EOF) 
		{
			*f->discard=vi->fatmirror[*f->sector];
			f->discard=&vi->fatmirror[*f->sector];
			for (;;) 
			{
				unsigned short nextsec=*f->discard;
				if (nextsec==FS_FAT_EOF) break;
				f->discard=&vi->fatmirror[nextsec];
			}
			vi->fatmirror[*f->sector]=FS_FAT_EOF;
		}
	}

	f->len=f->relpos+f->pos;

	if (!f->len) 
	{
		if (f->sectorstart!=FS_FAT_EOF) 
		{
			*f->discard=f->sectorstart;
			f->discard=&vi->fatmirror[f->sectorstart];
			vi->fatmirror[f->sectorstart]=FS_FAT_EOF;
		}
		f->sectorstart=FS_FAT_EOF; /* this will be copied to direntry at close! */
		f->sector=&f->sectorstart;
		f->modified=0;	 /* no need to store buffer if len=0 */
	}

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_stat
 *
 * get status information on a file
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filename - which file time and date wanted to be retrieve
 * stat - pointer where to store information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_stat(FS_MULTI *fm,const char *filename,FS_STAT *stat)
{
#ifdef SAFE_UNICODE
	W_CHAR nconv[FS_MAXPATH];
	return fg_wstat(fm,_fsm_towchar(nconv,filename),stat);
}

int fg_wstat(FS_MULTI *fm,const W_CHAR *filename,FS_STAT *stat)
{
#endif
	FS_DIRENTRY *de;
	FS_NAME fsname;
	FS_VOLUMEINFO *vi;

	fsm_memset(stat,0,sizeof(FS_STAT));

	if (_fg_setfsname(fm,filename,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDNAME); /* invalid name */
	if (fsm_checknamewc(fsname.lname)) return F_SETLASTERROR(F_ERR_INVALIDNAME);/* invalid name */

	if (_fg_getvolumeinfo(fm,fsname.drivenum,&vi)) return F_SETLASTERROR(F_ERR_INVALIDDRIVE); /* invalid drive */

	if (filename[0]=='.') 
	{
		if (filename[1]==0 || (filename[1]=='.' && filename[2]==0)) 
		{
			if (!_fg_findpath(vi,&fsname)) return F_SETLASTERROR(F_ERR_INVALIDDIR);
			if (fsname.dirnum!=FS_DIR_ROOT) 
			{
				stat->createtime=vi->direntries[fsname.dirnum].ctime;
				stat->createdate=vi->direntries[fsname.dirnum].cdate;
				stat->secure|=FSSEC_ATTR_DIR;
				stat->drivenum=fsname.drivenum;
				return F_SETLASTERROR(F_NO_ERROR);
			}
			return F_SETLASTERROR(F_ERR_INVALIDNAME);
		}
	}

	if (!(_fg_find(vi,&fsname,&de,0))) return F_SETLASTERROR(F_ERR_NOTFOUND);	   /* not exist */

	stat->secure=de->secure;
	if (de->attr&FS_ATTR_DIR) stat->secure|=FSSEC_ATTR_DIR;
	else 
	{
		stat->secure&=~FSSEC_ATTR_DIR;
		stat->secure^=FSSEC_ATTR_ARC;
		stat->filesize=(unsigned long)de->len;
	}

	stat->drivenum=fsname.drivenum;

	stat->createtime=de->ctime;
	stat->createdate=de->cdate;

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * fg_ftruncate
 *
 * truncate a file to a specified length, filepointer will be set to the
 * end
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * filehandle - which file need to be truncated
 * length - length of new file
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int	fg_ftruncate(FS_MULTI *fm,FS_FILE *filehandle,unsigned long length)
{
	int ret=fg_seek(fm,filehandle,(long)length,FS_SEEK_SET);
	if (ret) return ret;
	return fg_seteof(fm,filehandle);
}

/****************************************************************************
 *
 * fg_checkvolume
 *
 * Deletes a previously initialized volume.
 *
 * INPUTS
 *
 * fm - multi structure
 * drvnumber - number of drive to check
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fg_checkvolume(FS_MULTI *fm,int drvnumber) 
{
	return F_SETLASTERROR(_fg_getvolumeinfo(fm,drvnumber,0));
}

/****************************************************************************
 *
 * fg_get_oem
 *
 * Get OEM name
 *
 * INPUTS
 *
 * fm - multi structure pointer
 * drivenum - drivenumber
 * str - where to store information
 * maxlen - length of the buffer
 *
 * RETURN
 *
 * errorcode or zero if successful
 *
 ***************************************************************************/

int fg_get_oem (FS_MULTI *fm, int drivenum, char *str, long maxlen)
{
	FS_VOLUMEINFO *vi;
	int ret;
	int i;

	ret=_fg_getvolumeinfo(fm,drivenum,&vi);
	if (ret) return F_SETLASTERROR(ret);

	if (maxlen)
	{
		if ((int)maxlen>(int)sizeof(OEM_name)) maxlen=sizeof(OEM_name);
		else maxlen--;

		for (i=0;i<maxlen;i++)
		{
			str[i]=OEM_name[i];
		}
		str[i]=0;
	}

	return F_SETLASTERROR(F_NO_ERROR);
}

/****************************************************************************
 *
 * End of fsf.c
 *
 ***************************************************************************/
