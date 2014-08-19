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
*       defs.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       File system definitions.
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

#ifndef _DEFS_H_
#define _DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define F_FILE FS_FILE
#define F_FIND FS_FIND
#define F_SPACE FS_SPACE
#define F_MAXPATH FS_MAXPATH
#define F_SEEK_SET FS_SEEK_SET
#define F_SEEK_END FS_SEEK_END
#define F_SEEK_CUR FS_SEEK_CUR
#define F_ATTR_DIR FS_ATTR_DIR
#define F_STAT FS_STAT

#define f_getversion fsm_getversion
#define f_init fsm_init
#define f_mountdrive(drivenum,buffer,buffsize,mountfunc,phyfunc) fsm_mountdrive(drivenum,buffer,buffsize,mountfunc,phyfunc)
#define f_unmountdrive(drvnum) fsm_unmountdrive (drvnum)
#define f_format(drivenum) fsm_format(drivenum)
#define f_getfreespace(drivenum,space) fsm_getfreespace(drivenum,space)
#define f_enterFS fsm_enterFS
#define f_releaseFS(ID) fsm_releaseFS(ID)
#define f_get_drive_list(buf) fsm_get_drive_list (buf)
#define f_get_drive_count fsm_get_drive_count
#define f_getdrive fsm_getdrive
#define f_chdrive(drivenum) fsm_chdrive(drivenum)
#define f_getcwd(buffer,maxlen) fsm_getcwd(buffer,maxlen)
#define f_getdcwd(drivenum,buffer,maxlen) fsm_getdcwd(drivenum,buffer,maxlen)
#define f_mkdir(dirname) fsm_mkdir(dirname)
#define f_chdir(dirname) fsm_chdir(dirname)
#define f_rmdir(dirname) fsm_rmdir(dirname)
#define f_setlabel(drivenum,label) fsm_setlabel(drivenum,label)
#define f_getlabel(drivenum,label,len) fsm_getlabel(drivenum,label,len)
#define f_rename(filename,newname) fsm_rename(filename,newname)
#define f_move(filename,newname) fsm_move(filename,newname)
#define f_delete(filename) fsm_delete(filename)
#define f_filelength(filename) fsm_filelength(filename)
#define f_findfirst(filename,find) fsm_findfirst(filename,find)
#define f_findnext(find) fsm_findnext(find)
#define f_open(filename,mode) fsm_open(filename,mode)
#define f_close(filehandle) fsm_close(filehandle)
#define f_abortclose(filehandle) fsm_abortclose(filehandle)
#define f_write(buf,size,size_st,filehandle) fsm_write(buf,size,size_st,filehandle)
#define f_read(buf,size,size_st,filehandle) fsm_read(buf,size,size_st,filehandle)
#define f_seek(filehandle,offset,whence) fsm_seek(filehandle,offset,whence)
#define f_tell(filehandle) fsm_tell(filehandle)
#define f_eof(filehandle) fsm_eof(filehandle)
#define f_seteof(filehandle) fsm_seteof(filehandle)
#define f_rewind(filehandle) fsm_rewind(filehandle)
#define f_putc(ch,filehandle) fsm_putc(ch,filehandle)
#define f_getc(filehandle) fsm_getc(filehandle)
#define f_flush(filehandle) fsm_flush(filehandle)
#define f_settimedate(filename,ctime,cdate) fsm_settimedate(filename,ctime,cdate)
#define f_gettimedate(filename,pctime,pcdate) fsm_gettimedate(filename,pctime,pcdate)
#define f_getpermission(filename,psecure) fsm_getpermission(filename,psecure)
#define f_setpermission(filename,secure) fsm_setpermission(filename,secure)
#define f_truncate(filename,length) fsm_truncate(filename,length)
#define f_stat(filename,stat) fsm_stat(filename,stat)
#define f_ftruncate(filehandle,length) fsm_ftruncate(filehandle,length)
#define f_checkvolume(drvnumber) fsm_checkvolume(drvnumber) 
#define f_get_oem(drivenum,str,maxlen) fsm_get_oem(drivenum,str,maxlen)
#define f_getlasterror fsm_getlasterror

#ifdef SAFE_UNICODE
#define F_WFIND FS_WFIND
#define f_wgetcwd(buffer,maxlen) fsm_wgetcwd(buffer,maxlen)
#define f_wgetdcwd(drivenum,buffer,maxlen) fsm_wgetdcwd(drivenum,buffer,maxlen)
#define f_wmkdir(dirname) fsm_wmkdir(dirname)
#define f_wchdir(dirname) fsm_wchdir(dirname)
#define f_wrmdir(dirname) fsm_wrmdir(dirname)
#define f_wrename(filename,newname) fsm_wrename(filename,newname)
#define f_wmove(filename,newname) fsm_wmove(filename,newname)
#define f_wdelete(filename) fsm_wdelete(filename)
#define f_wfilelength(filename) fsm_wfilelength(filename)
#define f_wfindfirst(filename,find) fsm_wfindfirst(filename,find)
#define f_wfindnext(find) fsm_wfindnext(find)
#define f_wopen(filename,mode) fsm_wopen(filename,mode)
#define f_wsettimedate(filename,ctime,cdate) fsm_wsettimedate(filename,ctime,cdate)
#define f_wgettimedate(filename,ctime,cdate) fsm_wgettimedate(filename,ctime,cdate)
#define f_wgetpermission(filename,psecure) fsm_wgetpermission(filename,psecure)
#define f_wsetpermission(filename,secure) fsm_wsetpermission(filename,secure)
#define f_wtruncate(filename,length) fsm_wtruncate(filename,length)
#define f_wstat(filename,stat) fsm_wstat(filename,stat)
#endif

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * End of defs.h
 *
 ***************************************************************************/

#endif /* _DEFS_H_ */
