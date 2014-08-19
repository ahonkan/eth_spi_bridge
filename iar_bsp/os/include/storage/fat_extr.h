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
*      fat_extr.h
*
* COMPONENT
*
*      FAT
*
* DESCRIPTION
*
*      Function prototypes for Nucleus FILE FAT file system.
*
* DATA STRUCTURES
*
*      None.
*
* FUNCTIONS
*
*      None.
*
* DEPENDENCIES
*
*      None.
*
*************************************************************************/
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


/* =========== File MOUNT.C ================ */
STATUS          fat_init(VOID);
STATUS          fat_uninit(VOID);
STATUS          fat_mount(UINT16 dh, VOID *config);

/* =========== File UNMOUNT.C ================ */
STATUS          fat_unmount(UINT16 dh);

/* =========== File NU_FILE.C ================ */
STATUS          FAT_Init_FS(CHAR*);
STATUS          fat_disk_abort(UINT16 dh);
STATUS          fat_make_dir(CHAR *name);
VOID            _synch_file_ptrs(PC_FILE *pfile);
STATUS          fat_open(UINT16 dh, CHAR *name, UINT16 flag, UINT16 mode);
INT32           fat_read(INT fd, CHAR *buf, INT32 count);
INT32           fat_write(INT fd, CHAR *buf, INT32 count);
INT32           fat_seek(INT fd, INT32 offset, INT16 origin);
STATUS          fat_truncate(INT fd, INT32 offset);
STATUS          _po_lseek(PC_FILE *pfile, INT32 offset, INT16 origin);
STATUS          _po_flush(PC_FILE *pfile);
STATUS          fat_flush(INT fd);
STATUS          fat_close(INT fd);
STATUS          fat_set_attributes(CHAR *name, UINT8 newattr);
STATUS          fat_get_attributes(UINT8 *attr, CHAR *name);
STATUS          fat_rename(CHAR *name, CHAR *newname);
STATUS          fat_delete(CHAR *name);
STATUS          fat_remove_dir(CHAR *name);
UINT32          pc_fat_size(UINT16 bytepsec, UINT16 nreserved, UINT16 cluster_size,
                        UINT16 n_fat_copies, UINT16 root_entries, UINT32 volume_size, UINT16 fasize);
STATUS          fat_format(UINT16, VOID **);
STATUS          fat_get_format_info(UINT16, VOID **);
STATUS          fat_freespace(UINT16 dh, UINT8 *secpcluster, UINT16 *bytepsec,
                             UINT32 *freecluster, UINT32 *totalcluster);
STATUS          fat_get_first(UINT16 dh, FAT_DSTAT *statobj, CHAR *pattern);
STATUS          fat_get_next(FAT_DSTAT *statobj);
VOID            fat_done(FAT_DSTAT *statobj);
STATUS          fat_utime(DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                          UINT16 update_date,UINT16 update_time,UINT16 create_date,
                          UINT16 create_time);
INT             pc_l_pwd(UINT8 *, DROBJ *);
INT             pc_gm_name(UINT8 *path, DROBJ *pmom, DROBJ *pdotdot);

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
    STATUS          fat_check_disk(UINT16 dh,UINT8 flag,UINT8 mode);
#endif

/* =========== File APIEXT.C ================ */
INT16           pc_cluster_size(UINT8 *drive);
UINT32          po_extend_file(INT fd, UINT32 n_clusters, INT16 method);
UINT32          pc_find_contig_clusters(DDRIVE *pdr, UINT32 startpt,
                                UINT32  *pchain, UINT32 min_clusters, INT16 method);
/* =========== File APIUTIL.C ================ */
STATUS          pc_dskinit(UINT16 dh);
STATUS          pc_idskclose(UINT16 dh);
PC_FILE        *pc_fd2file(INT fd);
INT             pc_allocfile(VOID);
VOID            pc_freefile(INT fd);
VOID            pc_free_all_fil(DDRIVE *pdrive);
INT16           pc_log_base_2(UINT16 n);
DROBJ          *pc_get_cwd(DDRIVE *pdrive);
VOID            pc_upstat(FAT_DSTAT *statobj);
INT             fat_get_cache_size(UINT16 dh);

/* =========== File BLOCK.C ================ */
INT             pc_alloc_blk(BLKBUFF **ppblk, DDRIVE *pdrive, UINT32 blockno);
BLKBUFF        *pc_blkpool(DDRIVE *pdrive);
VOID            pc_free_all_blk(DDRIVE *pdrive);
VOID            pc_free_buf(BLKBUFF *pblk, INT waserr);
STATUS          pc_read_blk(BLKBUFF **pblk, DDRIVE *pdrive, UINT32 blockno);
STATUS          pc_write_blk(BLKBUFF *pblk);
STATUS          pc_init_blk(DDRIVE *pdrive, UINT32 blockno);

/* =========== File DROBJ.C ================ */
STATUS          pc_fndnode(UINT16 dh, DROBJ **pobj, UINT8 *path);
STATUS          pc_get_inode(DROBJ **pobj, DROBJ *pmom, UINT8 *filename);
STATUS          pc_next_inode(DROBJ *pobj, DROBJ *pmom, UINT8 *filename, INT attrib);
UINT8           chk_sum(UINT8 *sname);
INT             pc_cre_longname(UINT8 *filename, LNAMINFO *linfo);
INT             pc_cre_shortname(UINT8 *filename, UINT8  *fname, UINT8 *fext);
VOID            lnam_clean(LNAMINFO *linfo, BLKBUFF *rbuf);
STATUS          pc_findin(DROBJ *pobj, UINT8 *filename);
STATUS          pc_get_mom(DROBJ **pmom, DROBJ *pdotdot);
DROBJ          *pc_mkchild(DROBJ *pmom);
STATUS          pc_mknode(DROBJ **pobj, DROBJ *pmom, UINT8 *filename, UINT8 *fileext, UINT8 attributes);
STATUS          pc_gen_sfn(UINT8 *sfn_fname, UINT8 *lfn, DATESTR *time_date, UINT32 free_cl_cnt);

STATUS          pc_insert_inode(DROBJ *pobj, DROBJ *pmom, UINT8 *filename, INT longfile);
VOID            pc_del_lname_block(DROBJ *pobj);
STATUS          pc_renameinode(DROBJ *pobj, DROBJ *poldmom, DROBJ *pnewmom, UINT8 *fnambuf,
                                    UINT8 *fextbuf, UINT8 *newfilename, INT longdest);
STATUS          pc_rmnode(DROBJ *pobj);
STATUS          pc_update_inode(DROBJ *pobj, INT setdate);
DROBJ          *pc_get_root(DDRIVE *pdrive);
UINT32          pc_firstblock(DROBJ *pobj);
STATUS          pc_next_block(DROBJ *pobj);
STATUS          pc_l_next_block(UINT32 *nxt, DDRIVE *pdrive, UINT32 curblock);
VOID            pc_marki(FINODE *pfi, DDRIVE *pdrive, UINT32 sectorno, INT16  index);
FINODE         *pc_scani(DDRIVE *pdrive, UINT32 sectorno, INT16 index);
DROBJ          *pc_allocobj(VOID);
FINODE         *pc_alloci(VOID);
VOID            pc_free_all_i(DDRIVE *pdrive);
VOID            pc_freei(FINODE *pfi);
VOID            pc_freeobj(DROBJ *pobj);
VOID            pc_dos2inode (FINODE *pdir, DOSINODE *pbuff, UINT16 dh);
VOID            pc_init_inode(FINODE *pdir, UINT8 *filename, UINT8 *fileext,
                                UINT8 attr, UINT32 cluster, UINT32 size, DATESTR *crdate);
VOID            pc_ino2dos (DOSINODE *pbuff, FINODE *pdir, UINT16 dh);
INT             pc_isavol(DROBJ *pobj);
INT             pc_isadir(DROBJ *pobj);
INT             pc_isroot(DROBJ *pobj);


/* =========== File PC_ATA.C ================
INT             pc_get_ataparams(INT16 driveno, FMTPARMS *pfmt);
*/

/* =========== File LOWL.C ================ */
STATUS          pc_alloc_chain(UINT32 *, DDRIVE *pdr, UINT32 *pstart_cluster, UINT32 n_clusters);
STATUS          pc_find_free_cluster(UINT32 *, DDRIVE *pdr, UINT32 startpt, UINT32 endpt);
STATUS          pc_clalloc(UINT32 *clno, DDRIVE *pdr);
STATUS          pc_clgrow(UINT32 *nxt,DDRIVE *pdr, UINT32  clno);
STATUS          pc_clnext(UINT32 *nxt, DDRIVE *pdr, UINT32  clno);
STATUS          pc_clrelease(DDRIVE   *pdr, UINT32  clno);
STATUS          pc_faxx(DDRIVE *pdr, UINT32 clno, UINT32 *pvalue);
STATUS          pc_flushfat(DDRIVE *pdr);
STATUS          pc_freechain(DDRIVE *pdr, UINT32 cluster);
INT32           pc_get_chain(DDRIVE *pdr,
                            UINT32 start_cluster,
                            UINT32 *pnext_cluster,
                            UINT32 n_clusters);
STATUS          pc_pfaxx(DDRIVE *pdr, UINT32  clno, UINT32  value);
STATUS          pc_pfswap(UINT8 FAR **, DDRIVE *pdr, UINT32 index, INT for_write);
STATUS          pc_pfpword(DDRIVE *pdr, UINT16 index, UINT16 *pvalue);
STATUS          pc_pfgword(DDRIVE *pdr, UINT16 index, UINT16 *value);
STATUS          pc_pfflush(DDRIVE *pdr);
STATUS          pc_clzero(DDRIVE *pdrive, UINT32 cluster);
STATUS          pc_dskfree(UINT16 dh, INT  unconditional);
UINT32          pc_ifree(UINT16 dh);
UINT32          pc_sec2cluster(DDRIVE *pdrive, UINT32 blockno);
UINT16          pc_sec2index(DDRIVE *pdrive, UINT32 blockno);
UINT32          pc_cl2sector(DDRIVE *pdrive, UINT32 cluster);
STATUS          pc_update_fsinfo(DDRIVE *pdr);

/* =========== File PC_LOCKS.C ================ */
UINT16          pc_fs_enter(VOID);
VOID            pc_fs_exit(UINT16 restore_state);

VOID            fat_release(UINT16 dh);
VOID            fat_reclaim(UINT16 dh);
STATUS          pc_alloc_lock(WAIT_HANDLE_TYPE *wh);
VOID            pc_dealloc_lock(WAIT_HANDLE_TYPE wh);

#if (LOCK_METHOD == 2)
    VOID            fs_suspend_task(VOID);
    INT             fs_lock_task(VOID);
    VOID            fs_unlock_task(INT state_var);
    VOID            pc_wait_lock(LOCKOBJ *plock);
    VOID            pc_wake_lock(LOCKOBJ *plock);
    VOID            pc_drive_enter(UINT16 dh, INT exclusive);
    VOID            pc_drive_exit(UINT16 dh);
    VOID            pc_inode_enter(FINODE *pinode, INT exclusive);
    VOID            pc_inode_exit(FINODE *pinode);
    VOID            pc_fat_enter(UINT16 dh);
    VOID            pc_fat_exit(UINT16 dh);
    VOID            pc_drive_io_enter(UINT16 dh);
    VOID            pc_drive_io_exit(UINT16 dh);
    VOID            pc_generic_enter(LOCKOBJ *plock, INT exclusive);
    VOID            pc_generic_exit(LOCKOBJ *plock);
#endif

/* =========== File PC_MEMRY.C ================ */
INT             pc_memory_init(VOID);
INT             pc_memory_close(VOID);
DROBJ          *pc_memory_drobj(DROBJ *pobj);
FINODE         *pc_memory_finode(FINODE *pinode);

/* =========== File PC_PART.C ================ */
STATUS NU_Create_FAT_Partition(CHAR *dev_name, UINT16 part_type, UINT32 size, UINT32 offset, UINT8 fat_type);

/* =========== File FILEUTIL.C ================ */
INT             pc_allspace(UINT8 *p, INT i);
VOID            pc_cppad(UINT8 *to, UINT8 *from, INT size);
INT             pc_isdot(UINT8 *fname, UINT8 *fext);
INT             pc_isdotdot(UINT8 *fname, UINT8 *fext);
INT             pc_parsenewname(DROBJ *pobj, UINT8 *name,
                            UINT8 *ext, VOID **new_name,
                            VOID **new_ext, UINT8 *fname);
INT             pc_next_fparse(UINT8 *filename);
INT             pc_fileparse(UINT8 *filename, UINT8 *fileext, VOID *pfname, VOID *pfext);
UINT8          *pc_nibbleparse(UINT8 *topath);
STATUS          pc_parsepath(VOID **topath, VOID **pfname, VOID **pfext, UINT8 *path);
INT             pc_patcmp(UINT8 *disk_fnam, UINT8 *in_fnam);
VOID            pc_strcat(UINT8 *to, UINT8 *from);
INT             pc_use_wdcard(UINT8 *code);
INT             pc_use_upperbar(UINT8 *code);
INT             pc_checkpath(UINT8 *code, INT vol);

/* =========== File PC_UNICD.C ================ */
UINT8           uni2asc(UINT8 *ptr);
UINT8           asc2uni(UINT8 *ptr, UINT8 ascii);

/* =========== File dir_wr.c ================ */
STATUS fat_wr_get_first(DSTAT *statobj, CHAR *pattern);
STATUS fat_wr_get_next (DSTAT *statobj);
STATUS fat_wr_done (DSTAT *statobj);
STATUS fat_dealloc_fat_dstat_for_dh(UINT16 dh);

/* =========== File vnode.c ================= */
STATUS fat_allocate_fsnode(UINT16 dh, CHAR *path, VOID **fsnode);
STATUS fat_deallocate_fsnode(UINT16 dh, VOID *fsnode);
STATUS fat_fsnode_to_string(UINT16 dh, VOID *fsnode, CHAR *string);
#ifdef          __cplusplus
}

#endif /* _cplusplus */
