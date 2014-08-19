/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       stdio.h
*
*   COMPONENT
*
*       RTL - RunTime Library.
*
*   DESCRIPTION
*
*       This file contains the standard buffered input / output routines.
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       FILE                A structure containing information
*                               about a file.
*       fpos_t              A non-array type containing all
*                           information needed to specify
*                           uniquely every position within
*                           a file.
*       size_t              An unsigned integer type.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       stddef.h            POSIX stddef.h definitions
*       errno.h             POSIX errno.h definitions
*       stdarg.h            Tools variable-argument support
*
*************************************************************************/

#ifndef NU_PSX_STDIO_H
#define NU_PSX_STDIO_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/sys/time.h"
#include "services/signal.h"
#include "services/sys/types.h"
#include "services/stddef.h"
#ifdef H83
#include "services/errno.h"
#endif
#include <stdarg.h>
#include "nucleus.h"

/* For Metaware Metrowerks and KMC GNU Tools. */
#ifndef _STDIO_H
#define _STDIO_H

/* For ADS Tools */
#ifndef __stdio_h
#define __stdio_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _STDIO
#define _STDIO

/* For Paradigm Tools and Microtec Tools. */
#ifndef __STDIO_H
#define __STDIO_H

/* For Microsoft Visual C */
#ifndef _INC_STDIO
#define _INC_STDIO

#ifndef __STDIO_H_
#define __STDIO_H_

/* For Code Sourcery ARM GNU */
#ifndef _STDIO_H_
#define _STDIO_H_

/* For DIAB tools */
#ifndef __Istdio
#define __Istdio

#ifdef __cplusplus
extern "C" {
#endif

#define BUFSIZ          256         /* Default buffer size */

#define _IOFBF          0x01        /* Input/output fully buffered */

#define _IOLBF          0x02        /* Input/output line buffered. */

#define _IONBF          0x04        /* Input/output unbuffered */

#define  L_ctermid      256         /* Maximum size of character array to
                                       hold ctermid( ) output   */

#ifndef SEEK_SET
#define SEEK_SET        0           /* Seek relative to start-of-file. */
#endif /* SEEK_SET */

#ifndef SEEK_CUR
#define SEEK_CUR        1           /* Seek relative to current position. */
#endif /* SEEK_CUR */

#ifndef SEEK_END
#define SEEK_END        2           /* Seek relative to end-of-file. */
#endif /* SEEK_END */

/* Maximum size in bytes of the longest filename string that the
   implementation guarantees can be opened. */
#define FILENAME_MAX    256

/* Number of streams which the implementation guarantees can be open
   simultaneously. The value is at least eight. */
#define FOPEN_MAX       16

/* Minimum number of unique filenames generated by tmpnam( ).*/
#define TMP_MAX         25

/* Temporary file prefix */
#define P_tmpdir        "/tmp"

#define L_tmpnam        (sizeof(P_tmpdir)+15)

/* End-of-file return value. */
#define  EOF            (-1)

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

/* A structure containing information about a file. */
struct _iobuf
{
    char            *base;                  /*  Pointer to the start of
                                                        buffer  */
    int             file;                   /*  File descriptor */
    long            next;                   /*  Offset to next character
                                                        in the buffer  */
    unsigned long   flag;                   /*  Miscellaneous flags */
    unsigned long   cnt;                    /*  No of characters in the
                                                        buffer  */
    unsigned long   bufsize;                /*  Size of the file buffer */
};

typedef struct _iobuf FILE;

/* Standard IO file streams. */
FILE* _iob(unsigned short type);

extern FILE *       __stdin;
#define stdin       __stdin

extern FILE *       __stdout;
#define stdout      __stdout

extern FILE *       __stderr;
#define stderr      __stderr

/* fio_flags */

#define FIO_READ 1                          /*  Reading is allowed */
#define FIO_WRITE 2                         /*  Writing is allowed */
#define FIO_ASCII 4                         /*  Expand \n to \r\n,
                                                control-Z is EOF */
#define FIO_LINEBUF 8                       /*  Line buffered, so flush
                                                after \n */
#define FIO_EOF 16                          /*  End-of-file has occurred */
#define FIO_UNBUF 32                        /*  Stream is not buffered */
#define FIO_MYBUF 64                        /*  User-allocated buffer */
#define FIO_MYFILE 128                      /*  Statically allocated 
                                                FILE structure */
#define FIO_FLUSH 256                       /*  Buffer contains data to 
                                                be flushed */
#define FIO_ERROR 512                       /*  Error has occurred */

/* A non-array type containing all information needed to specify uniquely
   every position within a file.    */
typedef  long fpos_t;           

/* Function Prototypes */
void    clearerr(FILE *);
int     fclose(FILE *);
FILE*   fdopen(int, const char *);
int     feof(FILE *);
int     ferror(FILE *);
int     fflush(FILE *);
int     fgetc(FILE *);
int     fgetpos(FILE *, fpos_t *);
char*   fgets(char *, int, FILE *);
int     fileno(FILE *);
FILE*   fopen(const char *, const char *);
int     fprintf(FILE *, const char *, ...);
int     fputc(int, FILE *);
int     fputs(const char *, FILE *);
size_t  fread(void *, size_t, size_t, FILE *);
FILE*   freopen(const char *, const char *,FILE *);
int     fscanf(FILE *, const char *, ...);
int     fseek(FILE *, long, int);
int     fseeko(FILE *, off_t, int);
int     fsetpos(FILE *, const fpos_t *);
long    ftell(FILE *);
off_t   ftello(FILE *);
size_t  fwrite(const void *, size_t, size_t, FILE *);
int     getc(FILE *);
int     getchar(void);
char*   gets(char *);
void    perror(const char *);
int     printf(const char *, ...);
int     putc(int, FILE *);
int     putchar(int);
int     puts(const char *);
int     remove(const char *);
int     rename(const char *, const char *);
void    rewind(FILE *);
int     scanf(const char *, ...);
void    setbuf(FILE *, char *);
int     setvbuf(FILE *, char *, int, size_t);
int     snprintf(char *, size_t, const char *, ...);
int     sprintf(char *, const char *, ...);
int     sscanf(const char *, const char *,...);
FILE*   tmpfile(void);
char*   tmpnam(char *);
int     ungetc(int, FILE *);
int     vfprintf(FILE *, const char *, va_list);
int     vprintf(const char *, va_list);
int     vsnprintf(char *, size_t, const char *, va_list);
int     vsprintf(char *, const char *, va_list);
void     flockfile(FILE *);
int      ftrylockfile(FILE *);
void     funlockfile(FILE *);
int      getc_unlocked(FILE *);
int      getchar_unlocked(void);
int      putc_unlocked(int, FILE *);
int      putchar_unlocked(int);

#ifdef __cplusplus
}
#endif

#endif  /* __Istdio */
#endif  /* _STDIO_H_ */
#endif  /* __STDIO_H_ */
#endif  /* _INC_STDIO */
#endif  /* __STDIO_H */
#endif  /* _STDIO */
#endif  /* __stdio_h */
#endif  /* _STDIO_H */

#endif /* NU_PSX_STDIO_H */