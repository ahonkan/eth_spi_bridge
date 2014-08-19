/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       nussh_fs.h
*
*   DESCRIPTION
*
*       This file defines an OS compatibility layer for Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_storage.h
*
************************************************************************/
#ifndef NUSSH_FS_H_
#define NUSSH_FS_H_
#include "nucleus.h"
#include "storage/nu_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File API definitions and mappings to Nucleus equivalents.
 */

#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE
/* This is a dummy implementation to get code compiled */
struct stat {
	long st_size;
};
#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */


#if SSH_INCLUDE_INITIAL_FILES
typedef struct _SSH_FS_FILE{

    CHAR            ssh_name[32];            /* name */
    CHAR            *ssh_addr;               /* starting address */
    INT16           ssh_type;                /* mode (SSH_COMPRESSED or SSH_COMPILED) */
    INT32           ssh_length;              /* real length */
    INT32           ssh_clength;             /* compressed length */
    struct _SSH_FS_FILE * ssh_next;          /* next file in chain */
    UINT8           padding[2];
}SSH_FS_FILE;

extern SSH_FS_FILE Embed_Fs[];

extern const size_t Embed_Fs_Size;
#endif
/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 */
#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */

/* Map FILE to the void type. */
#define  FILE VOID

/*
 * File API definitions and mappings to Nucleus equivalents.
 */


/* Map file functions to the OS layer. */
#define fopen nussh_fopen
#define fclose nussh_fclose
#ifdef  feof
#undef  feof
#endif /* feof */
#define feof nussh_feof
#define fseek nussh_fseek
#define ftell nussh_ftell
#define fflush nussh_fflush
#define fgets nussh_fgets
#ifdef  ferror
#undef  ferror
#endif /* ferror */
#define ferror nussh_ferror
#define fread nussh_fread
#define fwrite nussh_fwrite
#define fprintf(fp, ...) nussh_fprintf((FILE *)fp, __VA_ARGS__)

FILE *nussh_fopen(const char *path, const char *mode);
int nussh_fclose(FILE *fp);
int nussh_feof(FILE *stream);
int nussh_fseek(FILE *stream, long offset, int whence);
long nussh_ftell(FILE *stream);
int nussh_fflush(FILE *stream);
char *nussh_fgets(char *s, int size, FILE *stream);
int nussh_ferror(FILE *stream);
size_t nussh_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t nussh_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int nussh_fprintf (FILE * fp, const char * fmt, ...);
INT SSHF_Write_File_System(void);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_FS_H_ */
