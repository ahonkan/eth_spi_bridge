/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
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
*       o_nucleus.c
*
*   DESCRIPTION
*
*       This file defines functions of an OS compatibility layer
*       for the Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Ioctl_SIOCGIFADDR
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/
#include "e_os.h"
#include "os/kernel/plus/supplement/inc/error_management.h"

NU_MEMORY_POOL *OPENSSL_Mem = &System_Memory;
static NU_SEMAPHORE OPENSSL_Printf_Semaphore;
static NU_SEMAPHORE OPENSSL_Timer_Semaphore;
static INT32 OPENSSL_Last_Tick;
static INT32 OPENSSL_Time = 0x4d08c14e; /* 15th Dec 2010, 1823 GMT+5 */
static INT OPENSSL_initialized = 0;

u32 OPENSSL_errno;

CHAR OPENSSL_Print_Buf[800];
CHAR OPENSSL_Print_Buf2[500];

int cos_program_init(void)
{
    /* If already initialized, return silently with success. */
    if (OPENSSL_initialized) {
        return 0;
    }

    if (NU_Create_Semaphore(&OPENSSL_Printf_Semaphore, "SSLPSEM",
            (UNSIGNED)1, NU_FIFO) != NU_SUCCESS) {
        return -1;
    }

    if (NU_Create_Semaphore(&OPENSSL_Timer_Semaphore, "SSLTSEM",
            (UNSIGNED)1, NU_FIFO) != NU_SUCCESS) {
        return -1;
    }

    OPENSSL_Last_Tick = NU_Retrieve_Clock();

    OPENSSL_initialized = 1;

    return 0;
}

void cos_program_deinit(void)
{
    NU_Delete_Semaphore(&OPENSSL_Printf_Semaphore);
    NU_Delete_Semaphore(&OPENSSL_Timer_Semaphore);
    OPENSSL_initialized = 0;
}

void *cos_zalloc(size_t size)
{
    void *n = cos_malloc(size);
    if (n)
        memset(n, 0, size);
    return n;
}

void *cos_malloc(size_t size)
{
    STATUS status;
    char *mem_ptr = NULL;

    if (size != 0) {
        status = NU_Allocate_Memory(OPENSSL_Mem, (VOID **)&mem_ptr,
                            size + sizeof(u32), NU_SUSPEND);
        if (status == NU_SUCCESS) {
            /* Store the allocation size at the start of the memory
             * allocation. */
            *((u32 *)mem_ptr) = (u32)size;
            mem_ptr = mem_ptr + sizeof(u32);
        } else {
            mem_ptr = NULL;
            printf("cos_malloc: Failed to get memory (size requested = %d)\n",
                size);
        }
    }

    return mem_ptr;
}

void *cos_realloc(void *ptr, size_t size)
{
    char *new_ptr = NULL;
    char *prev_ptr;
    u32 prev_size;

    if (size == 0) {
        if (ptr) cos_free(ptr);
        return NULL;
    } else if (size && ptr == NULL) {
        new_ptr = cos_malloc(size);
        return new_ptr;
    }

    prev_ptr = (char *)ptr - sizeof(u32);
    prev_size = *((u32 *)prev_ptr);

    /* If new size is less than previous size, then do nothing. */
    if (size < prev_size)
        return ptr;

    /* Allocate required memory. */
    new_ptr = cos_malloc(size);

    if (ptr && new_ptr) {
        /* Copy required bytes to the new buffer and then free the
         * original buffer. */
        size = prev_size < size ? prev_size : size;

        memcpy(new_ptr, ptr, size);

        cos_free(ptr);
    } else if (new_ptr == NULL) {
        printf(
            "cos_realloc: Failed to get memory (size requested = %d)\n",
            size);
    }

    return (new_ptr);
}

void cos_free(void *ptr)
{
    char *mem_ptr;

    if (ptr != NULL) {
        /* Move back pointer by 32 bits as space for the size was also
         * allocated at the start of the buffer. */
        mem_ptr = (char *)ptr - sizeof(u32);
        if (NU_Deallocate_Memory(mem_ptr) != NU_SUCCESS)
            printf("cos_free: unable to deallocate memory\n");
    }
}

char *cos_strdup(const char *s)
{
    char *res;
    size_t len;
    if (s == NULL)
        return NULL;
    len = strlen(s);
    res = cos_malloc(len + 1);
    if (res)
        memcpy(res, s, len + 1);
    return res;
}

int cos_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    if (size > 0)
        str[size - 1] = '\0';
    return ret;
}

unsigned cos_alarm(unsigned seconds)
{
    /* Do nothing. */
    return seconds;
}

void *cos_signal(int signum, void *handler)
{
    return NULL;
}

void cos_exit(int status)
{
    if (status != NU_SUCCESS)
    {
        printf("\n!!! Fatal Error !!!\n");
        ERC_System_Error(status);
    }
}

int cos_atexit(void (*function)(void))
{
    return 0;
}

int cos_printf(const char *fmt, ...)
{
    va_list arg_list;
    INT ret;
    INT size = sizeof(OPENSSL_Print_Buf);
    static CHAR overflow_msg[] = "\r\nprintf() buffer overflow.\r\n";
#ifndef WIN32
    CHAR cr_chr;
    CHAR nl_chr;
#endif

    NU_Obtain_Semaphore(&OPENSSL_Printf_Semaphore, NU_SUSPEND);

    va_start(arg_list, fmt);
    ret = vsnprintf(OPENSSL_Print_Buf, size - 10, fmt, arg_list);
    va_end(arg_list);

    /* Check for buffer overflow. */
    if (ret > size - 1) {
        ret = strlen(overflow_msg);
        strncpy(OPENSSL_Print_Buf, overflow_msg, ret);
        OPENSSL_Print_Buf[ret] = '\0';
    }

#ifndef WIN32
    /* Convert trailing "\n" to "\r\n". */
    cr_chr = ret > 1 ? OPENSSL_Print_Buf[ret - 2] : 0;
    nl_chr = ret > 0 ? OPENSSL_Print_Buf[ret - 1] : 0;
    /* Check if ends with "\n", doesn't end with "\r\n" and 
     * has room in buffer for "\r\n". */
    if ((nl_chr == 0x0a) && (cr_chr != 0x0d) && (ret < (size - 2))) {
        /* Convert "\n" to "\r\n". */
        OPENSSL_Print_Buf[ret - 1] = (CHAR)0x0d;	/* '\r' */
        OPENSSL_Print_Buf[ret]     = (CHAR)0x0a;	/* '\n' */
        OPENSSL_Print_Buf[ret + 1] = (CHAR)0x00;	/* '\0' */
    }
#endif

    OPENSSL_Print_Buf[size - 1] = '\0';

#undef printf
    printf(OPENSSL_Print_Buf);
#define printf cos_printf

    NU_Release_Semaphore(&OPENSSL_Printf_Semaphore);

    return ret;
}

int cos_vprintf(const char *format, va_list ap)
{
    int ret;
    size_t size = sizeof(OPENSSL_Print_Buf2);

    ret = vsnprintf(OPENSSL_Print_Buf2, size - 10, format, ap);
    if (ret > 0) {
        OPENSSL_Print_Buf2[size - 1] = '\0';
        printf(OPENSSL_Print_Buf2);
    }
    return ret;
}

void cos_perror(const char *msg)
{
    printf("%s\n", msg);
}

#define DAYS_OF_YR(y) (((y)%4 || ((y)%100==0 && (y)%400)) ? 365 : 366)

struct tm *cos_gmtime(const time_t *timer)
{
    int t;
    int d;
    int year;
    int m;
    char days_of_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static struct tm time_struct;

    /* Break down the timer */
    t = (int)*timer;
    time_struct.tm_sec = t % 60;
    t /= 60;
    time_struct.tm_min = t % 60;
    t /= 60;
    time_struct.tm_hour = t % 24;
    t /= 24;
    time_struct.tm_wday = (t + 4) % 7;

    /* Calculate the year and also account for possible leap years. */
    year = 1970;
    while (t >= (d = DAYS_OF_YR(year)))
    {
        t -= d;
        year++;
    }

    /* Set the year value */
    time_struct.tm_year = year - 1900;

    /* Set up day of the year */
    time_struct.tm_yday = t;

    /* Determine if this is a leap year */
    if (d == 366)
        days_of_month[1] = 29;
    else
        days_of_month[1] = 28;

    /* Figure out month and day */
    for(m = 0; t >= days_of_month[m]; m++)
        t -= days_of_month[m];

    /* Set month and day */
    time_struct.tm_mon = m;
    time_struct.tm_mday = t + 1;

    return &time_struct;
}

time_t cos_time(time_t* t)
{
    UNSIGNED diff;
    UINT32 current_tick;

    /* Make all calculations atomic */
    
    NU_Obtain_Semaphore(&OPENSSL_Timer_Semaphore, NU_SUSPEND);
    
    current_tick = NU_Retrieve_Clock();

    /* Since all quantities are unsigned, wrap around will also generate
       correct absolute difference. */
    diff = current_tick - OPENSSL_Last_Tick;
    OPENSSL_Time += (diff / NU_PLUS_TICKS_PER_SEC);
    OPENSSL_Last_Tick = current_tick;

    NU_Release_Semaphore(&OPENSSL_Timer_Semaphore);

    if (t)
        *t = OPENSSL_Time;

    return ((time_t) OPENSSL_Time);
}

clock_t cos_clock(void)
{
    return ((clock_t) NU_Retrieve_Clock());
}

int cos_isdigit(int c)
{
    /* always asuming C-locale */
    
    return ( ((c >= '0') && (c <= '9')) ? 1 : 0 );
}

int cos_isxdigit(int c)
{
    /* always asuming C-locale */
    
    if ( ((c >= '0') && (c <= '9')) ||
         ((c >= 'a') && (c <= 'f')) ||
         ((c >= 'A') && (c <= 'F')) )
    {
       return 1;
    }

    return 0;
}

int cos_isascii(int c)
{
    /* always asuming C-locale */
    
    return ( ((c >= 0) && (c <= 127)) ? 1 : 0 );
}

/*
 * Map various networking functions to Nucleus equivalents.
 */

int cos_socket(int socket_family, int socket_type, int protocol)
{
    STATUS status;
    status = NU_Socket((INT16)socket_family, (INT16)socket_type,
                (INT16)protocol); 
    return (int)status;
}

int cos_bind(int socket, struct sockaddr *addr, socklen_t addr_len)
{
    STATUS status;
    struct addr_struct nu_addr;
    struct sockaddr_in *my_addr = (struct sockaddr_in *)addr;

    nu_addr.family = my_addr->sin_family;
    nu_addr.port = ntohs(my_addr->sin_port);
    memcpy(nu_addr.id.is_ip_addrs, &my_addr->sin_addr.s_addr,
            sizeof(my_addr->sin_addr.s_addr));
    status = NU_Bind(socket, &nu_addr, sizeof(nu_addr));
    return (int)status;
}

int cos_connect(int socket, struct sockaddr *addr, socklen_t addr_len)
{
    STATUS status;
    struct addr_struct nu_addr;
    struct sockaddr_in *sock_addr = (struct sockaddr_in *)addr;

    nu_addr.family = sock_addr->sin_family;
    nu_addr.port = ntohs(sock_addr->sin_port);
    memcpy(nu_addr.id.is_ip_addrs, &sock_addr->sin_addr.s_addr,
            sizeof(sock_addr->sin_addr.s_addr));
    status = NU_Connect(socket, &nu_addr, sizeof(nu_addr));
    return (int)status;
}

int cos_listen(int sockfd, int backlog)
{
    STATUS status;
    status = NU_Listen((INT)sockfd, (UINT16)backlog);
    return (int)status;
}

int cos_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    STATUS status;
    struct addr_struct nu_addr;
    struct sockaddr_in *my_addr = (struct sockaddr_in *)addr;

    status = NU_Accept(sockfd, &nu_addr, 0);
    my_addr->sin_family = nu_addr.family;
    my_addr->sin_port = ntohs(nu_addr.port);
    my_addr->sin_addr.s_addr = GET32(nu_addr.id.is_ip_addrs, 0);
    memcpy(&my_addr->sin_addr.s_addr, nu_addr.id.is_ip_addrs,
            sizeof(my_addr->sin_addr.s_addr));
    return (int)status;
}

ssize_t cos_send(int socket, const void *message, size_t length, int flags)
{
    INT32 ret;
    ret = NU_Send(socket, (CHAR *)message, (UINT16)length, (INT16)flags);
    return (ssize_t)ret;
}

ssize_t cos_sendto(int socket, const void *message, size_t length,
                   int flags, const struct sockaddr *dest, socklen_t destlen)
{
    INT32 ret;
    struct addr_struct nu_addr;
    struct sockaddr_in *dest_addr = (struct sockaddr_in *)dest;

    nu_addr.family = dest_addr->sin_family;
    nu_addr.port = ntohs(dest_addr->sin_port);
    memcpy(nu_addr.id.is_ip_addrs, &dest_addr->sin_addr.s_addr,
            sizeof(dest_addr->sin_addr.s_addr));
    ret = NU_Send_To(socket, (CHAR *)message, (UINT16)length,
                (INT16)flags, &nu_addr, sizeof(nu_addr));
    return (ssize_t)ret;
}

ssize_t cos_recv(int socket, void *buf, size_t len, int flags)
{
    INT32 ret;
    ret = NU_Recv(socket, (CHAR *)buf, (UINT16)len, (INT16)flags);
    return (ssize_t)ret;
}

ssize_t cos_recvfrom(int socket, void *buf, size_t len, int flags,
                     struct sockaddr *from, socklen_t *fromlen)
{
    INT32 ret;
    INT16 nu_addr_len;
    struct addr_struct nu_addr;
    struct sockaddr_in *from_addr = (struct sockaddr_in *)from;

    ret = NU_Recv_From(socket, buf, len, flags, &nu_addr, &nu_addr_len);
    from_addr->sin_family = nu_addr.family;
    from_addr->sin_port = htons(nu_addr.port);
    memcpy(&from_addr->sin_addr.s_addr, nu_addr.id.is_ip_addrs,
            sizeof(from_addr->sin_addr.s_addr));
    *fromlen = (socklen_t)sizeof(struct sockaddr);
    return (ssize_t)ret;
}

char *cos_inet_ntoa(struct in_addr in)
{
    char *in_str = (char *)&in.s_addr;
    static char addr_buf[MAX_ADDRESS_SIZE * 4];
    sprintf(addr_buf, "%d.%d.%d.%d", in_str[0], in_str[1], in_str[2],
            in_str[3]);
    return addr_buf;
}

int cos_close(int socket)
{
    int ret;
    ret = (int)NU_Close_Socket(socket);
    return ret;
}

struct servent *cos_getservbyname(const char *name, const char *proto)
{
    /* Always return NULL. */
    return NULL;
}

struct hostent *cos_gethostbyname(const char *name)
{
    NU_HOSTENT nu_hent;
    static struct hostent po_hent;

    if (NU_Get_Host_By_Name((CHAR *)name, &nu_hent) == NU_SUCCESS) {
        po_hent.h_name = nu_hent.h_name;
        po_hent.h_aliases = NULL;
        po_hent.h_addrtype = nu_hent.h_addrtype;
        po_hent.h_length = nu_hent.h_length;
        po_hent.h_addr_list = nu_hent.h_addr_list;

        return &po_hent;
    }

    return NULL;
}

int cos_getsockname(int socket, struct sockaddr *address,
                    socklen_t *address_len)
{
    struct sockaddr_struct nu_addr;
    struct sockaddr_in *po_addr = (struct sockaddr_in *)address;
    INT16 nu_addr_len;
    int ret;

    ret = (int)NU_Get_Sock_Name(socket, &nu_addr, &nu_addr_len);
    if (ret == 0) {
        po_addr->sin_family = AF_INET; 
        po_addr->sin_port = ntohs(nu_addr.port_num);
        memcpy(&po_addr->sin_addr.s_addr, nu_addr.ip_num.is_ip_addrs,
                sizeof(po_addr->sin_addr.s_addr));

        *address_len = (socklen_t)nu_addr_len;
    }

    return ret;
}

int cos_getpeername(int socket, struct sockaddr *address,
                    socklen_t *address_len)
{
    struct sockaddr_struct nu_addr;
    struct sockaddr_in *po_addr = (struct sockaddr_in *)address;
    INT16 nu_addr_len;
    int ret;

    ret = (int)NU_Get_Peer_Name(socket, &nu_addr, &nu_addr_len);
    if (ret == 0) {
        po_addr->sin_family = AF_INET; 
        po_addr->sin_port = ntohs(nu_addr.port_num);
        memcpy(&po_addr->sin_addr.s_addr, nu_addr.ip_num.is_ip_addrs,
                sizeof(po_addr->sin_addr.s_addr));

        *address_len = (socklen_t)nu_addr_len;
    }

    return ret;
}

int cos_setsockopt(int s, int level, int  optname, const void *optval,
                   socklen_t optlen) 
{
    int ret = -1;
    int ival;

    if (level == SOL_SOCKET) {
        if (optname == SO_REUSEADDR) {
            ival = *(int *)optval;
            ret = (int)NU_Setsockopt_SO_REUSEADDR(s, (INT16)ival);
        }
    } else if (level == IPPROTO_TCP) {
        if (optname == TCP_NODELAY) {
            ival = *(int *)optval;
            ret = (int)NU_Setsockopt_TCP_NODELAY(s, (UINT8)ival);
        }
    } else if (level == IPPROTO_IP) {
        if (optname == IP_TOS) {
            ival = *(int *)optval;
            ret = (int)NU_Setsockopt_IP_TOS(s, (UINT8)ival);
        }
    }

    return ret;
}

int cos_getsockopt(int s, int level, int optname, void *optval,
                   socklen_t *optlen)
{
    (void)s;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;

    return (-1);
}

int cos_shutdown(int s, int how)
{
    int ret;

    ret = (int)NU_Shutdown((INT)s, (INT)how + 1);

    return ret;
}

int cos_select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout)
{
    STATUS status;
    UNSIGNED nu_timeout;

    if (timeout) {
        nu_timeout = (timeout->tv_sec * NU_PLUS_TICKS_PER_SEC) +
                ((timeout->tv_usec * NU_PLUS_TICKS_PER_SEC) / 1000000);
    } else {
        nu_timeout = NU_SUSPEND;
    }

    status = NU_Select(nfds, readfds, writefds, exceptfds, nu_timeout);

    /* On failure, clear all the descriptor sets to make the behavior
     * consistent with the *NIX implementation. */
    if (status != NU_SUCCESS) {
        if (readfds)
            FD_ZERO(readfds);
        if (writefds)
            FD_ZERO(writefds);
        if (exceptfds)
            FD_ZERO(exceptfds);

        /* If the error is due to no sockets being specified, then sleep
         * for the specified duration to be consistent with the *NIX
         * implementation. */
        if (status == NU_NO_SOCKETS) {
            if (nu_timeout != 0)
                NU_Sleep(nu_timeout);
            status = 0;
        }
    }
    return (int)status;
}

FILE *cos_fopen(const char *path, const char *mode)
{
    OPENSSL_FILE *ofp;
    UINT16 flags = 0;
    int fd;
    int do_not_truncate = NU_FALSE;
    int i;

    if (path == NULL || mode == NULL)
        return NULL;

    ofp = (OPENSSL_FILE *)cos_malloc(sizeof(OPENSSL_FILE));
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
        cos_free(ofp);
        ofp = NULL;
    }

    return (FILE *)ofp;
}

int cos_fclose(FILE *fp)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)fp;
    int ret;

    if (ofp == NULL)
        return -1;
    ret = (int)NU_Close(ofp->fd);
    cos_free(ofp);
    return ret;
}

int cos_feof(FILE *stream)
{
    (void)stream;
    /* No Nucleus equivalant so do nothing. */
    return 0;
}

int cos_fseek(FILE *stream, long offset, int whence)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;
    int ret;

    if (ofp == NULL)
        return -1;
    ret = (int)NU_Seek(ofp->fd, (INT32)offset, (INT16)whence);
    ofp->error = !(ret >= 0);
    return ofp->error;
}

long cos_ftell(FILE *stream)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;
    long ret;

    if (ofp == NULL)
        return -1;
    ret = (long)NU_Seek(ofp->fd, 0, PSEEK_CUR);
    ofp->error = !(ret >= 0);
    return ret;
}

int cos_fflush(FILE *stream)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;
    int ret;

    /* Do nothing if given stream is from RTL library. */
    if ((stream == stdout) || (stream == stderr))
        return 0;

    if (ofp == NULL)
        return -1;
    ret = (int)NU_Flush(ofp->fd);
    ofp->error = !(ret == NU_SUCCESS);
    return ret;
}

char *cos_fgets(char *s, int size, FILE *stream)
{
    CHAR *p;
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;

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

int cos_ferror(FILE *stream)
{
    return (((OPENSSL_FILE*)stream)->error);
}

size_t cos_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;
    size_t ret;

    if (ofp == NULL || ptr == NULL || (size * nmemb <= 0))
        return 0;
    ret = (size_t)NU_Read(ofp->fd, (CHAR *)ptr, (INT32)size * nmemb);
    ofp->error = !(ret > 0);
    return ret;
}

size_t cos_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    OPENSSL_FILE *ofp = (OPENSSL_FILE *)stream;
    size_t ret;

    if (ofp == NULL || ptr == NULL || (size * nmemb <= 0))
        return 0;
    ret = (size_t)NU_Write(ofp->fd, (CHAR *)ptr, (INT32)size * nmemb);
    ofp->error = !(ret > 0);
    return ret;
} 

