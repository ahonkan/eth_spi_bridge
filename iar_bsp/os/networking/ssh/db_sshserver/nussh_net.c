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
*       sshnu_net.c
*
*   DESCRIPTION
*
*       This file defines functions of an OS compatibility layer
*       for the Nucleus Net.
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
*       sshnu_net.h
*
************************************************************************/
#include "nussh_includes.h"

/*
 * Map various networking functions to Nucleus equivalents.
 */

int nussh_socket(int socket_family, int socket_type, int protocol)
{
    STATUS status;
    status = NU_Socket((INT16)socket_family, (INT16)socket_type,
                (INT16)protocol);
    return (int)status;
}

int nussh_bind(int socket, struct sockaddr *addr, socklen_t addr_len)
{
    STATUS status;
    struct addr_struct nu_addr;
    struct sockaddr_in *my_addr;
#if (SSH_ENABLE_IPV6)
    struct sockaddr_in6 *my_addr6;
#endif

    if(addr->sa_family == AF_INET)
    {
        my_addr = (struct sockaddr_in *)addr;
        nu_addr.family = my_addr->sin_family;
        nu_addr.port = ntohs(my_addr->sin_port);
        memcpy(nu_addr.id.is_ip_addrs, &my_addr->sin_addr.s_addr,
                sizeof(my_addr->sin_addr.s_addr));
    }
#if (SSH_ENABLE_IPV6)
    else if(addr->sa_family == AF_INET6)
    {
        my_addr6 = (struct sockaddr_in6 *)addr;
        nu_addr.family = my_addr6->sin6_family;
        nu_addr.port = ntohs(my_addr6->sin6_port);
        memcpy(nu_addr.id.is_ip_addrs, &my_addr6->sin6_addr.s6_addr,
                sizeof(my_addr6->sin6_addr.s6_addr));
    }
#endif
    else
    {
        return NU_INVALID_PARM;
    }

    status = NU_Bind(socket, &nu_addr, sizeof(nu_addr));
    return (int)status;
}

int nussh_connect(int socket, struct sockaddr *addr, socklen_t addr_len)
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

int nussh_listen(int sockfd, int backlog)
{
    STATUS status;
    status = NU_Listen((INT)sockfd, (UINT16)backlog);
    return (int)status;
}

int nussh_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    STATUS status;
    struct addr_struct nu_addr;
    struct sockaddr_in *my_addr = (struct sockaddr_in *)addr;
#if (SSH_ENABLE_IPV6 == NU_TRUE)
    struct sockaddr_in6 *my_addr6 = (struct sockaddr_in6 *)addr;
#endif

    status = NU_Accept(sockfd, &nu_addr, 0);
#if (SSH_ENABLE_IPV6 == NU_TRUE)
    if(nu_addr.family == AF_INET6)
    {
        if(NU_Is_IPv4_Mapped_Addr(nu_addr.id.is_ip_addrs))
        {
            my_addr6->sin6_family = AF_INET;
            my_addr->sin_port = ntohs(nu_addr.port);
            memcpy(&my_addr->sin_addr.s_addr, &nu_addr.id.is_ip_addrs[12],
                    IP_ADDR_LEN);
        }
        else
        {
            my_addr6->sin6_family = AF_INET6;
            my_addr6->sin6_port = ntohs(nu_addr.port);
            memcpy(&my_addr6->sin6_addr.s6_addr, nu_addr.id.is_ip_addrs,
                    IP6_ADDR_LEN);
        }
    }
    else
    {
#endif
        my_addr->sin_family = nu_addr.family;
        my_addr->sin_port = ntohs(nu_addr.port);
        my_addr->sin_addr.s_addr = GET32(nu_addr.id.is_ip_addrs, 0);
        memcpy(&my_addr->sin_addr.s_addr, nu_addr.id.is_ip_addrs,
                sizeof(my_addr->sin_addr.s_addr));
#if (SSH_ENABLE_IPV6 == NU_TRUE)
    }
#endif

    return (int)status;
}

ssize_t nussh_send(int socket, const void *message, size_t length)
{
    INT32 ret;
    ret = NU_Send(socket, (CHAR *)message, (UINT16)length, 0);
    return (ssize_t)ret;
}

ssize_t nussh_sendto(int socket, const void *message, size_t length,
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

ssize_t nussh_recv(int socket, void *buf, size_t len)
{
    INT32 ret;
    ret = NU_Recv(socket, (CHAR *)buf, (UINT16)len, 0);

    return (ssize_t)(ret ==  NU_NOT_CONNECTED ? 0 : ret);
}

ssize_t nussh_recvfrom(int socket, void *buf, size_t len, int flags,
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

char *nussh_inet_ntoa(struct in_addr in)
{
    char *in_str = (char *)&in.s_addr;
    static char addr_buf[MAX_ADDRESS_SIZE * 4];
    sprintf(addr_buf, "%d.%d.%d.%d", in_str[0], in_str[1], in_str[2],
            in_str[3]);
    return addr_buf;
}

char *nussh_inet6_ntoa(struct in6_addr in)
{
    char *in_str = (char *)&in.s6_addr;
    static char addr_buf[MAX_ADDRESS_SIZE + MAX_ADDRESS_SIZE/2];
    sprintf(addr_buf, "%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x",
            in_str[0], in_str[1], in_str[2],in_str[3], in_str[4], in_str[5],
            in_str[6], in_str[7], in_str[8],in_str[9], in_str[10], in_str[11],
            in_str[12], in_str[13], in_str[14], in_str[15]);
    return addr_buf;
}

int nussh_inet_aton(const char *cp, struct in_addr *inp)
{

        UNSIGNED_INT    addr,address[4],*addrptr;
        INT16           base_n,dots;
        CHAR            ctemp,ct;
        STATUS          status;

        dots = 0;
        addrptr = address;
        /* Be optimistic */
        status = NU_TRUE;

        /* Parse the IPv4 address using "." as the mark of passing
           from one set to the next */
        for( ; ; )
        {

            /* If starts 0, it can be 0x=hex or 0=octal. Otherwise its
               decimal */
            if (*cp == '0')
            {
                if (*++cp == 'x' || *cp == 'X')
                    base_n = 16, cp++;
                else
                    base_n = 8;
            }
            else
                base_n = 10;
            /* Set the return address as 0 */
            addr = 0;

            while ((ctemp = *cp) != '\0')
            {
                /* Is it decimal digit? */
                if(isdigit((unsigned char) ctemp) > 0)
                {
                    /* Convert to the actual char from ascii number */
                    addr = (ctemp - '0') + (addr * base_n);
                    cp++;
                    continue;
                }
                /* Is it hex? check the digit */
                if((base_n == 16) && (isxdigit((unsigned char) ctemp) > 0))
                {
                    /* Convert to the actual char from ascii number */
                    if(islower((unsigned char) ctemp) > 0)
                        ct = 'a';
                    else
                        ct = 'A';
                    addr = (addr << 4) + (ctemp + 10 - ct);
                    cp++;
                    continue;
                }
                break;
            }

            /* Checking dots. */
            if (*cp == '.')
            {
                /* Can't have other than IPv4
                   format or error*/
                if (addr > 0xff || dots > 3)
                {
                     status = NU_FALSE;
                     break;
                }
                dots++;
                *addrptr++ = addr;
                cp++;
            }
            else
                 break;
        }

        /* Checking for weird characters at the end. */
        while (*cp != '\0')
        {
            if (!isspace((unsigned char) *cp))
            {
                status = NU_FALSE;
                break;
            }
            cp++;
        }

        if(status == NU_TRUE)
        {
            /*
            Specified IP format: a.b.c.d
            a.b.c.d (with d treated as 8-bits)
            a.b.c   (with c treated as 16-bits)
            a.b     (with b treated as 24-bits)
            a       (with a treated as 32-bits)
            */
            switch(dots+1)
            {
                /* (a.b.c.d) (8.8.8.8) format */
                case 4:
                    if (addr > 0xff)
                    {
                         status = NU_FALSE;
                         break;
                    }
                    addr |= (address[0] << 24) | (address[1] << 16) | \
                            (address[2] << 8);
                    break;
                /* (a.b.c) (8.8.16) format */
                case 3:
                    if (addr > 0xffff)
                    {
                         status = NU_FALSE;
                         break;
                    }
                    addr |= (address[0] << 24) | (address[1] << 16);
                    break;
                 /* (a.b) 8.24 format */
                case 2:
                    if (addr > 0xffffff)
                    {
                         status = NU_FALSE;
                         break;
                    }
                    addr |= address[0] << 24;
                    break;
                /* (a) 32 format */
                case 1:
                    break;
                default:
                    status = NU_FALSE;
                    break;
            }
        }

        if (status == NU_TRUE)
            inp->s_addr = htonl(addr);

        return status;
}

int nussh_inet6_aton(const char *cp, struct in6_addr *inp)
{
    char *addr_str = (char *)cp;
    return (NU_Inet_PTON(NU_FAMILY_IP6, addr_str, (VOID*)inp->s6_addr));
}

int nussh_close(int socket)
{
    int ret;
    ret = (int)NU_Close_Socket(socket);
    return ret ==  NU_NOT_CONNECTED ? 0 : ret;
}

struct servent *nussh_getservbyname(const char *name, const char *proto)
{
    /* Always return NULL. */
    return NULL;
}

struct hostent *nussh_gethostbyname(const char *name)
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

struct hostent *nussh_gethostbyaddr(const void *addr, socklen_t len, int type)
{
    NU_HOSTENT nu_hent;
    static struct hostent po_hent;

    if (NU_Get_Host_By_Addr((CHAR *)addr, (INT) len, type, &nu_hent) == NU_SUCCESS) {
        po_hent.h_name = nu_hent.h_name;
        po_hent.h_aliases = NULL;
        po_hent.h_addrtype = nu_hent.h_addrtype;
        po_hent.h_length = nu_hent.h_length;
        po_hent.h_addr_list = nu_hent.h_addr_list;

        return &po_hent;
    }

    return NULL;
}

int nussh_getsockname(int socket, struct sockaddr *address,
                    socklen_t *address_len)
{
    struct sockaddr_struct nu_addr;
    struct sockaddr_in *po_addr = (struct sockaddr_in *)address;
#if (SSH_ENABLE_IPV6 == NU_TRUE)
    struct sockaddr_in6 *po6_addr = (struct sockaddr_in6 *)address;
#endif
    INT16 nu_addr_len = sizeof(struct sockaddr_struct);
    int ret;

    ret = (int)NU_Get_Sock_Name(socket, &nu_addr, &nu_addr_len);
    if (ret == 0) {

#if (SSH_ENABLE_IPV6 == NU_FALSE)
        po_addr->sin_family = AF_INET;
        po_addr->sin_port = ntohs(nu_addr.port_num);
        memcpy(&po_addr->sin_addr.s_addr, nu_addr.ip_num.is_ip_addrs,
                sizeof(po_addr->sin_addr.s_addr));

        *address_len = (socklen_t)nu_addr_len;
#else
        if(nu_addr.family == NU_FAMILY_IP)
        {
            po_addr->sin_family = AF_INET;
            po_addr->sin_port = ntohs(nu_addr.port_num);
            memcpy(&po_addr->sin_addr.s_addr, &nu_addr.ip_num.is_ip_addrs[12],
                    IP_ADDR_LEN);

            *address_len = (socklen_t)sizeof(*po_addr);

        }
        else
        {
            po6_addr->sin6_family = AF_INET6;
            po6_addr->sin6_port = ntohs(nu_addr.port_num);
            memcpy(&po6_addr->sin6_addr.s6_addr, nu_addr.ip_num.is_ip_addrs,
                    IP6_ADDR_LEN);

            *address_len = sizeof(*po6_addr);
        }
#endif
    }

    return ret;
}

int nussh_getpeername(int socket, struct sockaddr *address,
                    socklen_t *address_len)
{
    struct sockaddr_struct nu_addr;
    struct sockaddr_in *po_addr = (struct sockaddr_in *)address;
#if (SSH_ENABLE_IPV6 == NU_TRUE)
    struct sockaddr_in6 *po6_addr = (struct sockaddr_in6 *)address;
#endif
    INT16 nu_addr_len = sizeof(nu_addr);
    int ret;

    ret = (int)NU_Get_Peer_Name(socket, &nu_addr, &nu_addr_len);
    if (ret == 0) {
#if (SSH_ENABLE_IPV6 == NU_FALSE)
            po_addr->sin_family = AF_INET;
            po_addr->sin_port = ntohs(nu_addr.port_num);
            memcpy(&po_addr->sin_addr.s_addr, nu_addr.ip_num.is_ip_addrs,
                    sizeof(po_addr->sin_addr.s_addr));

            *address_len = (socklen_t)nu_addr_len;
#else
        if(nu_addr.family == NU_FAMILY_IP)
        {
            po_addr->sin_family = AF_INET;
            po_addr->sin_port = ntohs(nu_addr.port_num);
            memcpy(&po_addr->sin_addr.s_addr, &nu_addr.ip_num.is_ip_addrs[12],
                    IP_ADDR_LEN);

            *address_len = (socklen_t)sizeof(*po_addr);

        }
        else
        {
            po6_addr->sin6_family = AF_INET6;
            po6_addr->sin6_port = ntohs(nu_addr.port_num);
            memcpy(&po6_addr->sin6_addr.s6_addr, nu_addr.ip_num.is_ip_addrs,
                    IP6_ADDR_LEN);

            *address_len = sizeof(*po6_addr);
        }
#endif
    }

    return ret;
}

int nussh_setsockopt(int s, int level, int  optname, const void *optval,
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

int nussh_getsockopt(int s, int level, int optname, void *optval,
                   socklen_t *optlen)
{
    (void)s;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;

    return (-1);
}

int nussh_shutdown(int s, int how)
{
    int ret;

    ret = (int)NU_Shutdown((INT)s, (INT)how + 1);

    return ret;
}
/* Counts the number of bits set in a UINT32. */
UINT32 SETCOUNT_UINT32(UINT32 num)
{

    num = (num & 0x55555555) + ((num >> 1) & 0x55555555);
    num = (num & 0x33333333) + ((num >> 2) & 0x33333333);
    num = (num & 0x0F0F0F0F) + ((num >> 4) & 0x0F0F0F0F);
    num = (num & 0x00FF00FF) + ((num >> 8) & 0x00FF00FF);
    num = (num & 0x0000FFFF) + ((num >> 16) & 0x0000FFFF);

    return num;
}
int nussh_select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout)
{
    STATUS status;
    UNSIGNED nu_timeout, i;
    UNSIGNED fd_count = 0;

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
    else
    {
        /* On success, select() return the number of file descriptors contained
         * in the three returned descriptor sets. */
        for(i = 0; i < FD_ELEMENTS; i++)
        {
            if (readfds)
                fd_count += SETCOUNT_UINT32(readfds->words[i]);
            if (writefds)
                fd_count += SETCOUNT_UINT32(writefds->words[i]);
            if (exceptfds)
                fd_count += SETCOUNT_UINT32(exceptfds->words[i]);
        }
        return fd_count;
    }
    return (int)status;
}
