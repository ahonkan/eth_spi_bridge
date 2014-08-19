/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME               
*
*       pnet_sr.h              
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       POSIX Networking send, receive and DNS Hosts operations.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*       
*       None.
*
************************************************************************/
#ifndef _PNET_PNET_SR_H
#define _PNET_PNET_SR_H

/* POSIX layer of DNS list */
typedef struct pnet_dns_list
{
    DNS_HOST              *next_host;       /* Next pointer to DNS host     */
    DNS_HOST              *curr_host;       /* Current pointer to DNS host  */
    INT                   nu_fd;            /* Nucleus NET file descriptor  */
    UINT16                stayopen;         /* Stay open condition          */
    UINT16                dnsnum;           /* DNS number                   */
}PNET_DNS;

/* POSIX Recv Control Block for open socket */
typedef struct p_recv
{
	  INT		  			 socket;     /*  Specifies the file descriptor associated
										       with the socket 
									       */
	  VOID       			*buffer;     /*  Points to a buffer where the message
                                               should be stored.
                                           */ 
	  size_t      			 length;     /*  Specifies the length in bytes of the
										       buffer pointed to by the buffer argument. 
									       */ 
	  INT         			 flags;      /*  Specifies the type of message reception:
										       MSG_OOB  (NOT SUPPORTED)
										       MSG_PEEK
										       MSG_WAITALL 
									        */ 
	  struct addr_struct 	*address; 	/* A null pointer, or points to a sockaddr structure
										   	   in which the sending address is to be stored.
										   	   The length and format of the address depend on the
										       address family of the socket. 
										    */
	  struct _msghdr 		*msgheader;   /* Nucleus NET message header structure. 
	  									    */
      INT16                 *addr_length; /* Specifies the length of the sockaddr structure
                                               pointed to by the address argument. */
      UINT16                 reserved_1;  /* reserved data structure value. */
}PNET_DSRECV;

#ifdef __cplusplus
extern "C" {
#endif

ssize_t         pnet_recv(pid_t pid, PPROC_FS_RES *psx_file,PNET_DSRECV *r_tcb);
CHAR            **pnet_gethostentry(pid_t pid, NU_HOSTENT *net_entry, INT type);
DNS_HOST        *pnet_gethostent(UINT16 *stopen);
VOID            pnet_sethostent(pid_t pid,PPROC_FS_RES *psx_file, UINT16 stayopen);
VOID            pnet_endhostent(VOID);
PNET_DNS        *get_pnet_dns(VOID);
DNS_HOST_LIST   *get_host_list(VOID);    
VOID            pnet_set_dns(VOID);
struct hostent* POSIX_SYS_NET_Setup_Hostent(pid_t pid, NU_HOSTENT *net_entry);

#ifdef __cplusplus
}
#endif

#endif /* _PNET_PNET_SR_H */
