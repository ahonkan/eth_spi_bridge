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
*       fsmf.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       Safe's standard API multi-thread wrapper.
*
* DATA STRUCTURES
*
*       None.       
*           
* FUNCTIONS
*
*       None.
*
*************************************************************************/

#ifndef _FSMF_H_
#define _FSMF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* //////////////////////////////////////////////////////////////////////////////////// */
/*  */
/* 	Init Functions	 */
/*  */
/* //////////////////////////////////////////////////////////////////////////////////// */

#define fsm_getversion fg_getversion
#define fsm_init fg_init
extern int fsm_mountdrive(int drivenum,void *buffer,long buffsize,FS_DRVMOUNT mountfunc,FS_PHYGETID phyfunc);
extern int fsm_unmountdrive(int drivenum);
extern int fsm_getfreespace(int drivenum, FS_SPACE *space);
extern int fsm_enterFS(void);
extern void fsm_releaseFS(long ID);

extern int fsm_format(int drivenum);
extern int fsm_get_drive_list(int *buf);
extern int fsm_get_drive_count(void);

/* //////////////////////////////////////////////////////////////////////////////////// */
/*  */
/* 	Directory handler functions	 */
/*  */
/* //////////////////////////////////////////////////////////////////////////////////// */

extern int fsm_getdrive(void);
extern int fsm_chdrive(int drivenum);

extern int fsm_getcwd(char *buffer, int maxlen );
extern int fsm_getdcwd(int drivenum, char *buffer, int maxlen );

extern int fsm_mkdir(const char *dirname);
extern int fsm_chdir(const char *dirname);
extern int fsm_rmdir(const char *dirname);

/* //////////////////////////////////////////////////////////////////////////////////// */
/*  */
/* 	files functions	 */
/*  */
/* //////////////////////////////////////////////////////////////////////////////////// */

extern int fsm_rename(const char *filename,const char *newname);
extern int fsm_move(const char *filename, const char *newname);
extern int fsm_delete(const char *filename);

extern long fsm_filelength(const char *filename);

extern int fsm_findfirst(const char *filename,FS_FIND *find);
extern int fsm_findnext(FS_FIND *find);

extern int fsm_getpermission(const char *filename, unsigned long *psecure);
extern int fsm_setpermission(const char *filename, unsigned long secure);

/* //////////////////////////////////////////////////////////////////////////////////// */
/*  */
/* 	file read/write functions	 */
/*  */
/* //////////////////////////////////////////////////////////////////////////////////// */

extern FS_FILE *fsm_open(const char *filename,const char *mode);
extern int fsm_close(FS_FILE *filehandle);
extern int fsm_abortclose(FS_FILE *filehandle);
extern long fsm_write(const void *buf,long size,long size_st,FS_FILE *filehandle);
extern long fsm_read (void *buf,long size,long size_st,FS_FILE *filehandle);
extern int fsm_seek (FS_FILE *filehandle,long offset,long whence);
extern long fsm_tell (FS_FILE *filehandle);
extern int fsm_eof(FS_FILE *filehandle);
extern int fsm_rewind(FS_FILE *filehandle);
extern int fsm_putc(int ch,FS_FILE *filehandle);
extern int fsm_getc(FS_FILE *filehandle);
extern int fsm_flush(FS_FILE *filehandle);
extern int fsm_seteof(FS_FILE *filehandle);


extern int fsm_settimedate(const char *filename,unsigned short ctime,unsigned short cdate);
extern int fsm_gettimedate(const char *filename,unsigned short *ctime,unsigned short *cdate);
extern int fsm_getlabel(int drivenum, char *label, long len);
extern int fsm_setlabel(int drivenum, const char *label);
extern FS_FILE *fsm_truncate(const char *filename,unsigned long length);
extern int fsm_stat(const char *filename,FS_STAT *stat);
extern int fsm_ftruncate(FS_FILE *filehandle,unsigned long length);
extern int fsm_checkvolume(int drvnumber);
extern int fsm_get_oem (int drivenum, char *str, long maxlen);
extern int fsm_getlasterror(void); 


#ifdef SAFE_UNICODE
extern int fsm_wgetcwd(W_CHAR *buffer, int maxlen );
extern int fsm_wgetdcwd(int drivenum, W_CHAR *buffer, int maxlen );
extern int fsm_wmkdir(const W_CHAR *dirname);
extern int fsm_wchdir(const W_CHAR *dirname);
extern int fsm_wrmdir(const W_CHAR *dirname);
extern int fsm_wrename(const W_CHAR *filename,const W_CHAR *newname);
extern int fsm_wmove(const W_CHAR *filename,const W_CHAR *newname);
extern int fsm_wdelete(const W_CHAR *filename);
extern long fsm_wfilelength(const W_CHAR *filename);
extern int fsm_wfindfirst(const W_CHAR *filename,FS_WFIND *find);
extern int fsm_wfindnext(FS_WFIND *find);
extern int fsm_wgetpermission(const W_CHAR *filename, unsigned long *psecure);
extern int fsm_wsetpermission(const W_CHAR *filename, unsigned long secure);
extern FS_FILE *fsm_wopen(const W_CHAR *filename,const W_CHAR *mode);
extern int fsm_wsettimedate(const W_CHAR *filename,unsigned short ctime,unsigned short cdate);
extern int fsm_wgettimedate(const W_CHAR *filename,unsigned short *ctime,unsigned short *cdate);
extern FS_FILE *fsm_wtruncate(const W_CHAR *filename,unsigned long length);
extern int fsm_wstat(const W_CHAR *filename,FS_STAT *stat);
#endif

/****************************************************************************
 *
 * internal common functions for reentrance
 *
 ***************************************************************************/

extern int _fsm_checksemaphore(FS_MULTI *fm,FS_VOLUMEINFO *vi);
extern void _fsm_releasesemaphore(FS_MULTI *fm);
extern int _fsm_gettask(FS_MULTI **fm);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * End of fsmf.h
 *
 ***************************************************************************/

#endif /* _FSMF_H_ */
