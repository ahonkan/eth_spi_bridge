/*              Copyright 2013 Mentor Graphics Corporation
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
*       nussh_fs.c
*
*   DESCRIPTION
*
*       This file defines functions of an OS compatibility layer
*       for the Nucleus OS File System.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_storage.h
*
************************************************************************/
#include "nussh_includes.h"

/* Define an internal type for the FILE data structure. */
typedef struct {
    int fd;
    int flags;
    int error;
} SSH_FILE;

FILE *nussh_fopen(const char *path, const char *mode)
{
    SSH_FILE *ofp;
    UINT16 flags = 0;
    int fd;
    int do_not_truncate = NU_FALSE;
    int i;

    if (path == NULL || mode == NULL)
        return NULL;
/* MGC: Fake PID file >> */
    if(strcmp(path, DROPBEAR_PIDFILE) == 0)
        return 0;
/* MGC: Fake PID file << */

    ofp = (SSH_FILE *)malloc(sizeof(SSH_FILE));
    if (ofp == NULL) return NULL;

    /* Interpret the 'mode'. */
    for (i = 0; i < strlen(mode); i++) {
        switch (mode[i]) {
            case 'r':
                if (flags & PO_WRONLY)
                    flags = (flags ^ PO_WRONLY) | PO_RDWR;
                else
                    flags |= PO_RDONLY;
                break;
            case 'w':
                if (flags & PO_RDONLY)
                    flags = (flags ^ PO_RDONLY) | PO_RDWR;
                else
                    flags |= PO_WRONLY;
                flags |= PO_CREAT | PO_TRUNC;
                break;
            case 'a':
                flags |= PO_APPEND | PO_CREAT;
                break;
            case 'b':
                flags |= PO_BINARY;
                break;
            case 't':
                flags |= PO_TEXT;
                break;
            case '+':
                do_not_truncate = NU_TRUE;
                break;
        }
    }

    if (do_not_truncate && (flags & PO_TRUNC))
        flags = (flags ^ PO_TRUNC);

    fd = NU_Open((CHAR *)path, flags, PS_IREAD | PS_IWRITE);
    if (fd >= 0) {
        ofp->flags = flags;
        ofp->fd = fd;
        ofp->error = 0;
    }
    else
    {
        free(ofp);
        ofp = NULL;
    }

    return (FILE *)ofp;
}

int nussh_fclose(FILE *fp)
{
    SSH_FILE *ofp = (SSH_FILE *)fp;
    int ret;

    if (ofp == NULL)
        return -1;

    ret = (int)NU_Close(ofp->fd);
    free(ofp);
    return ret;
}

int nussh_feof(FILE *stream)
{
    (void)stream;
    /* No Nucleus equivalant so do nothing. */
    return 0;
}

int nussh_fseek(FILE *stream, long offset, int whence)
{
    SSH_FILE *ofp = (SSH_FILE *)stream;
    int ret;

    if (ofp == NULL)
        return -1;
    ret = (int)NU_Seek(ofp->fd, (INT32)offset, (INT16)whence);
    ofp->error = !(ret >= 0);
    return ofp->error;
}

long nussh_ftell(FILE *stream)
{
    SSH_FILE *ofp = (SSH_FILE *)stream;
    long ret;

    if (ofp == NULL)
        return -1;
    ret = (long)NU_Seek(ofp->fd, 0, PSEEK_CUR);
    ofp->error = !(ret >= 0);
    return ret;
}

int nussh_fflush(FILE *stream)
{
    SSH_FILE *ofp = (SSH_FILE *)stream;
    int ret;

    /* Do nothing if given stream is from RTL library. */
    if ((stream == (FILE *)stdout) || (stream == (FILE *)stderr))
        return 0;

    if (ofp == NULL)
        return -1;
    ret = (int)NU_Flush(ofp->fd);
    ofp->error = !(ret == NU_SUCCESS);
    return ret;
}

char *nussh_fgets(char *s, int size, FILE *stream)
{
    CHAR *p;
    SSH_FILE *ofp = (SSH_FILE *)stream;

    /* Do nothing if buffer is not large enough. */
    if (ofp == NULL)
        return NULL;

    if (size <= 1)
    {
        ofp->error = 1;
        return NULL;
    }

    if (s == NULL)
    {
        ofp->error = 1;
        return NULL;
    }

    /* Leave room for the null terminator. */
    size--;

    /* Read characters one by one until EOF or buffer runs out. */
    for (p=s; (size > 0) && (NU_Read(ofp->fd, p, 1) > 0); p++, size--) {
        if (*p == '\n') {
            p++;
            break;
        }
    }

    if (p > s)
    {
        /* Terminate the string. */
        *p = '\0';
        ofp->error = 0;
    }
    else
    {
        s = NULL;
        ofp->error = 1;
    }

    return s;
}

int nussh_ferror(FILE *stream)
{
    return (((SSH_FILE*)stream)->error);
}

size_t nussh_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    SSH_FILE *ofp = (SSH_FILE *)stream;
    size_t ret;

    if (ofp == NULL || ptr == NULL || (size * nmemb <= 0))
        return 0;
    ret = (size_t)NU_Read(ofp->fd, (CHAR *)ptr, (INT32)size * nmemb);
    ofp->error = !(ret > 0);
    return ret;
}

size_t nussh_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    SSH_FILE *ofp = (SSH_FILE *)stream;
    size_t ret;

    if (ofp == NULL || ptr == NULL || (size * nmemb <= 0))
        return 0;
    ret = (size_t)NU_Write(ofp->fd, (CHAR *)ptr, (INT32)size * nmemb);
    ofp->error = !(ret > 0);
    return ret;
}

int nussh_fprintf (FILE * fp, const char * fmt, ...)
{
    /* Fake files ... */
    if(fp == (FILE *)stderr)
    {
        if(strchr(fmt, '%'))
        {
            va_list param;
            va_start(param, fmt);
            vprintf(fmt, param);
            va_end(param);
        }
        else
            printf(fmt);
    }

    return 0;
}
/************************************************************************
* FUNCTION
*
*     SSHF_Write_File_System
*
* DESCRIPTION
*
*     Function that used to write to an external file system.
*
* INPUTS
*
*     fname                       File name to be written under.
*     filemem                     The pointer to the file in memory.
*     length                      The length of the file.
*
* OUTPUTS
*
*     Returns NU_SUCCESS if Successful.
*     Returns SSH_FAILURE if unsuccessful.
*
************************************************************************/
INT SSHF_Write_File_System(void)
{
    CHAR HUGE*     filemem;
    UINT32         length;
    INT         fd;
    STATUS      status = DROPBEAR_SUCCESS;
    SSH_FS_FILE *file_info;
    INT            i = 0;

    TRACE(("Loading Initial SSH files in FS.\r\n"));

    file_info = &Embed_Fs[0];
    while(file_info->ssh_name[0] != '\0' && status != DROPBEAR_FAILURE)
    {
        length = file_info->ssh_length;
        if((fd = NU_Open(file_info->ssh_name, PO_WRONLY|PO_CREAT, PS_IWRITE)) >= 0)
        {
            while(length && status != DROPBEAR_FAILURE)
            {
                if(length > USHRT_MAX)
                {
                    if((NU_Write(fd, (CHAR*)file_info->ssh_addr, USHRT_MAX)) != USHRT_MAX)
                        status = DROPBEAR_FAILURE;

                    length -= USHRT_MAX;
                    filemem += USHRT_MAX;
                }
                else
                {
                    if((NU_Write(fd, (CHAR*)file_info->ssh_addr, length)) != length)
                        status = DROPBEAR_FAILURE;

                    length = 0;
                }
            }

            status = NU_Close(fd);
            if(status != DROPBEAR_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            else {
                i++;
                file_info = &Embed_Fs[i];
            }
        }
        else
        {
            status = DROPBEAR_FAILURE;
            break;
        }
    }

    if(status != DROPBEAR_SUCCESS)
    {
        TRACE(("Failed to create file %s in FS.\r\n", file_info->ssh_addr));
    }
    else
    {
        TRACE(("SSH loaded %d files in FS.\r\n", Embed_Fs_Size));
    }

    return status;
}
