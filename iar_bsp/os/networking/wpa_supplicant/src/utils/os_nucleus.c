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

/*
 * wpa_supplicant/hostapd / Internal implementation of OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 *
 * This file is an example of operating system specific  wrapper functions.
 * This version implements many of the functions internally, so it can be used
 * to fill in missing functions from the target system C libraries.
 *
 * Some of the functions are using standard C library calls in order to keep
 * this file in working condition to allow the functions to be tested on a
 * Linux target. Please note that OS_NO_C_LIB_DEFINES needs to be defined for
 * this file to work correctly. Note that these implementations are only
 * examples and are not optimized for speed.
 */
#include "includes.h"
#include "common.h"
#include "os.h"

#include "nucleus_gen_cfg.h"
#include "wpa_supplicant/wpa_supplicant_cfg.h"
#include "openssl/rand.h"
#ifdef CFG_NU_OS_DRVR_SERIAL_ENABLE
#include "drivers/serial.h"
#endif
#include "services/runlevel_init.h"

#ifndef OS_NO_C_LIB_DEFINES
#error "Must define OS_NO_C_LIB_DEFINES"
#endif

/* The current Epoch time. */
os_time_t WPA_Supplicant_Epochtime = 0;

/* Externally defined memory pool. */
extern NU_MEMORY_POOL *WPA_Supplicant_Mem;

/* Semaphore to control os_printf() re-entrancy. */
extern NU_SEMAPHORE WPA_Supplicant_Printf_Semaphore;

/* Serial port handle. */
extern DV_DEV_HANDLE COM_Port_Handle;

void os_sleep(os_time_t sec, os_time_t usec)
{
	NU_Sleep((sec * NU_PLUS_TICKS_PER_SEC) + (usec / 10000));
}


/* We cannot use a real-time clock so this function compensates for
 * the missing functionality. */ 
void os_setdatetime(int year, int month, int day, int hour, int min, int sec)
{
	if (os_mktime(year, month, day, hour, min, sec,
				&WPA_Supplicant_Epochtime) != 0)
		wpa_printf(MSG_ERROR, "wpa_supplicant cannot initialize time");
}


int os_get_time(struct os_time *t)
{
	UNSIGNED cur_time;

	cur_time = NU_Retrieve_Clock();
	t->sec = (os_time_t)(cur_time / NU_PLUS_TICKS_PER_SEC);
	t->usec = (os_time_t)((cur_time % NU_PLUS_TICKS_PER_SEC) * 
								((long)1000000/NU_PLUS_TICKS_PER_SEC));

	/* Account for Epoch. */
	t->sec += WPA_Supplicant_Epochtime;
	return 0;
}


int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t)
{
	struct tm tm_i;

	if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
	    hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 ||
	    sec > 60)
		return -1;

	os_memset(&tm_i, 0, sizeof(tm_i));
	tm_i.tm_year = year - 1900;
	tm_i.tm_mon = month - 1;
	tm_i.tm_mday = day;
	tm_i.tm_hour = hour;
	tm_i.tm_min = min;
	tm_i.tm_sec = sec;

#ifdef tm
#undef tm
#endif
	*t = (os_time_t)mktime((struct tm *)&tm_i);
	return 0;
}


int os_daemonize(const char *pid_file)
{
	return -1;
}


void os_daemonize_terminate(const char *pid_file)
{
	return;
}


int os_get_random(unsigned char *buf, size_t len)
{
    if (RAND_bytes(buf, len))
        return 0;

    return -1;
}


unsigned long os_random(void)
{
	return (unsigned long)UTL_Rand();
}


char * os_rel2abs_path(const char *rel_path)
{
	/* The Nucleus implementation does not support relative paths
	 * so just return the path unmodified. */
	return os_strdup(rel_path);
}


int os_program_init(void)
{
	char *seed;

#warning ---> Security Alert! User MUST replace below seed with "dynamically" \
              provided strong random data with good entropy. \
              Please remove these #warnings after substituting your own seed. \
              For more info, please refer to "http://www.openssl.org/docs/crypto/"

	/* Seed the random number generator.
	 * User MUST substitue it with his/her own random data.
	 */
	seed = "This is just a long text to seed the random number generator"
		   "so that it can generate relatively strong random numbers for"
		   "the WPA Supplicant project. Make sure this is long enough";

	RAND_seed(seed, os_strlen(seed));

	return (NU_SUCCESS);
}


void os_program_deinit(void)
{
	os_file_deinit();
}


int os_setenv(const char *name, const char *value, int overwrite)
{
	/* Nucleus does not support the setenv() function. */
	return -1;  
}


int os_unsetenv(const char *name)
{
	/* Nucleus does not support the unsetenv() function. */
	return -1;
}


char * os_readfile(const char *name, size_t *len)
{
	int f;
	char *buf;

	f = os_file_open((char *)name, PO_RDONLY, PS_IREAD);
	if (f < 0)
		return NULL;

	*len = os_file_length(f);
	if (*len == 0) {
		os_file_close(f);
		return NULL;
	}   

	buf = os_malloc(*len);
	if (buf == NULL) {
		os_file_close(f);
		return NULL;
	}

	os_file_read(f, buf, *len);
	os_file_close(f);

	return buf;
}


void * os_zalloc(size_t size)
{
	void *n = os_malloc(size);
	if (n)
		os_memset(n, 0, size);
	return n;
}


void *os_malloc(size_t size)
{
	STATUS status;
	char *mem_ptr = NULL;

	if (size != 0) {
		status = NU_Allocate_Memory(WPA_Supplicant_Mem, (VOID **)&mem_ptr,
							size + sizeof(u32), NU_SUSPEND);
		if (status == NU_SUCCESS) {
			/* Store the allocation size at the start of the memory
			* allocation. */
			*((u32 *)mem_ptr) = (u32)size;
			mem_ptr = mem_ptr + sizeof(u32);
		} else {
			mem_ptr = NULL;
			wpa_printf(MSG_DEBUG, "os_malloc: Failed to get memory (size requested = %d)\n",
				size);
		}
	}

	return mem_ptr;
}


void *os_realloc(void *ptr, size_t size)
{
	char *new_ptr = NULL;
	char *prev_ptr;
	u32 prev_size;

	if (size == 0) {
		if (ptr) os_free(ptr);
		return NULL;
	} else if (size && ptr == NULL) {
		new_ptr = os_malloc(size);
		return new_ptr;
	}

	prev_ptr = (char *)ptr - sizeof(u32);
	prev_size = *((u32 *)prev_ptr);

	/* If new size is less than previous size, then do nothing. */
	if (size < prev_size)
		return ptr;

	/* Allocate required memory. */
	new_ptr = os_malloc(size);

	if (ptr && new_ptr) {
		/* Copy required bytes to the new buffer and then free the
		* original buffer. */
		size = prev_size < size ? prev_size : size;

		memcpy(new_ptr, ptr, size);

		os_free(ptr);
	} else if (new_ptr == NULL) {
		wpa_printf(MSG_DEBUG,
			"os_realloc: Failed to get memory (size requested = %d)\n",
			size);
	}

	return (new_ptr);
}


void os_free(void *ptr)
{
	char *mem_ptr;

	if (ptr != NULL) {
		/* Move back pointer by 32 bits as space for the size was also
		* allocated at the start of the buffer. */
		mem_ptr = (char *)ptr - sizeof(u32);
		if (NU_Deallocate_Memory(mem_ptr) != NU_SUCCESS)
			wpa_printf(MSG_DEBUG, "os_free: unable to deallocate memory\n");
	}
}


char * os_strdup(const char *s)
{
	char *res;
	size_t len;
	if (s == NULL)
		return NULL;
	len = os_strlen(s);
	res = os_malloc(len + 1);
	if (res)
		os_memcpy(res, s, len + 1);
	return res;
}


size_t os_strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}


int os_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	/* See http://www.ijs.si/software/snprintf/ for portable
	 * implementation of snprintf.
	 */

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	if (size > 0)
		str[size - 1] = '\0';
	return ret;
}

/*
 * Additional porting layer functions required by Nucleus.
 */


/**
 * os_printf - printf() style output function
 * @fmt: Output format string
 * Returns: The number of characters sent to output
 *
 * If the serial driver is enabled, then this function sends
 * output to the serial port. Otherwise it does nothing.
 */
int os_printf(const char *fmt, ...)
{
#ifndef CONFIG_NO_STDOUT_DEBUG
	va_list arg_list;
	INT ret;
	INT size = 800;
	static CHAR outbuf[800];
	static CHAR overflow_msg[] = "\r\nprintf() buffer overflow.\r\n";
	STATUS status;
#ifndef WIN32
	CHAR cr_chr;
	CHAR nl_chr;
#endif

	status = NU_Obtain_Semaphore(&WPA_Supplicant_Printf_Semaphore,
								NU_NO_SUSPEND);
	
	if (status != NU_SUCCESS)
		return 0;

	va_start(arg_list, fmt);
	ret = vsnprintf(outbuf, size, fmt, arg_list);
	va_end(arg_list);

	/* Check for buffer overflow. */
	if (ret > size - 1) {
		ret = os_strlen(overflow_msg);
		os_strncpy(outbuf, overflow_msg, ret);
		outbuf[ret] = '\0';
	}

#ifndef WIN32
	/* Convert trailing "\n" to "\r\n". */
	cr_chr = ret > 1 ? outbuf[ret - 2] : 0;
	nl_chr = ret > 0 ? outbuf[ret - 1] : 0;
	/* Check if ends with "\n", doesn't end with "\r\n" and 
	 * has room in buffer for "\r\n". */
	if ((nl_chr == 0x0a) && (cr_chr != 0x0d) && (ret < (size - 2))) {
		/* Convert "\n" to "\r\n". */
		outbuf[ret - 1] = (CHAR)0x0d;	/* '\r' */
		outbuf[ret]     = (CHAR)0x0a;	/* '\n' */
		outbuf[ret + 1] = (CHAR)0x00;	/* '\0' */
	}
#endif

	outbuf[size - 1] = '\0';

#ifdef CFG_NU_OS_DRVR_SERIAL_ENABLE
	/* Send to serial output. */
	NU_SIO_Puts(outbuf);
#endif

	/* Release the semaphore. */
	NU_Release_Semaphore(&WPA_Supplicant_Printf_Semaphore);

	return ret;
#else /* CONFIG_NO_STDOUT_DEBUG */
	return 0;
#endif /* CONFIG_NO_STDOUT_DEBUG */
}


/* Used by wpa_debug.c for formatted output. */
int os_vprintf(const char *format, va_list ap)
{
	int ret;
	static CHAR str[800];
	size_t size = 800;

	ret = vsnprintf(str, 800, format, ap);
	if (ret > 0) {
		str[size - 1] = '\0';
		os_printf(str);
	}
	return ret;
}


void os_perror(const char *msg)
{
	os_printf("%s: %d\n", msg);
}


int os_select(int nfds, fd_set *readfds, fd_set *writefds,
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


/*
 * File related OS layer functions.
 */

int os_file_setup(void)
{
	CHAR nu_drive[3];

	nu_drive[0] = 'A' + CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE;
	nu_drive[1] = ':';
	nu_drive[2] = '\0';

	/* Open the disk and set the current drive. Do not check error
	 * status as these calls might have been made earlier. */
	NU_Open_Disk(nu_drive);

	NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);

	return 0;
}

void os_file_deinit(void)
{
	INT current_drive;
	char nu_drive[3];
	
	/* Close the default disk. */
	current_drive = NU_Get_Default_Drive();

	nu_drive[0] = 'A' + current_drive;
	nu_drive[1] = ':';
	nu_drive[2] = '\0';

	NU_Close_Disk(nu_drive);
}

int os_file_open(const char *path, int flags, int mode)
{
	if (NU_Check_File_User() != NU_SUCCESS)
		NU_Become_File_User();

	os_file_setup();

	return (int)NU_Open((CHAR *)path, (UINT16)flags, (UINT16)mode);
}

size_t os_file_length(int file_desc)
{
	UINT32 data_length = 0;
	INT32 original_location;

	if (NU_Check_File_User() != NU_SUCCESS) {
		NU_Become_File_User();
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
	}

	/* Save off the current location of the file pointer. */
	original_location = NU_Seek(file_desc, 0, PSEEK_CUR);

	if (original_location >= 0) {
		data_length = (UINT32)NU_Seek(file_desc, 0, PSEEK_END);

		/* Restore the original position of the file pointer. */ 
		NU_Seek(file_desc, original_location, PSEEK_SET);
	}

	return (size_t)data_length;
}

int os_file_close(int file)
{
	if (NU_Check_File_User() != NU_SUCCESS) {
		NU_Become_File_User();
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
	}
	return (int)NU_Close(file);
}

size_t os_file_read(int file, char *buf, size_t size)
{
	if (NU_Check_File_User() != NU_SUCCESS) {
		NU_Become_File_User();
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
	}
	return (size_t)NU_Read(file, (CHAR *)buf, (UINT16)size);
}

size_t os_file_fprintf(int file, char *str)
{
	int length;

	if (NU_Check_File_User() != NU_SUCCESS) {
		NU_Become_File_User();
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
	}
	length = os_strlen(str);
	return (size_t)NU_Write(file, (CHAR *)str, (UINT16)length);
}


/**
 * os_fgets - An "fgets" equivalent for Nucleus FILE
 * Returns: "s" when characters are read and %NULL otherwise
 *
 * Reads at most one less than "size" characters from stream and stores
 * them into the buffer pointed to by "s". Reading stops after an EOF or a
 * newline. If a newline is read, it is stored into the buffer. A '\0' is
 * stored after the last character in the buffer.
 */
char * os_fgets(char *s, int size, int stream)
{
	CHAR *p;

	/* Do nothing if buffer is not large enough. */
	if (s == NULL)
		return NULL;
	else if (size <= 1)
		return NULL;

	if (NU_Check_File_User() != NU_SUCCESS) {
		NU_Become_File_User();
		NU_Set_Default_Drive(CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE);
	}

	/* Leave room for terminating NULL */
	size--;

	/* Read each character, until out of buffer space or EOF. */
	for (p=s; (size > 0) && (NU_Read(stream, p, 1) > 0); p++, size--) {
		/* Done if at the end of the line */
		if (*p == '\n') {
			p++;
			break;
		}
	}
	
	/* Did we read something? */
	if (p > s)
		/* Terminate the string. */
		*p = '\0';
	else
		s = NULL;

	return s;
}

/*
 * Map various networking functions to Nucleus equivalents.
 */

int os_socket(int socket_family, int socket_type, int protocol)
{
	STATUS status;
	status = NU_Socket((INT16)socket_family, (INT16)socket_type,
				(INT16)protocol); 
	return (int)status;
}


int os_bind(int socket, struct sockaddr *addr, socklen_t addr_len)
{
	STATUS status;
	struct addr_struct nu_addr;
	struct sockaddr_in *my_addr = (struct sockaddr_in *)addr;

	nu_addr.family = my_addr->sin_family;
	nu_addr.port = ntohs(my_addr->sin_port);
	os_memcpy(nu_addr.id.is_ip_addrs, &my_addr->sin_addr.s_addr,
			sizeof(my_addr->sin_addr.s_addr));
	status = NU_Bind(socket, &nu_addr, sizeof(nu_addr));
	return (int)status;
}


int os_connect(int socket, struct sockaddr *addr, socklen_t addr_len)
{
	STATUS status;
	struct addr_struct nu_addr;
	struct sockaddr_in *sock_addr = (struct sockaddr_in *)addr;

	nu_addr.family = sock_addr->sin_family;
	nu_addr.port = ntohs(sock_addr->sin_port);
	os_memcpy(nu_addr.id.is_ip_addrs, &sock_addr->sin_addr.s_addr,
			sizeof(sock_addr->sin_addr.s_addr));
	status = NU_Connect(socket, &nu_addr, sizeof(nu_addr));
	return (int)status;
}


ssize_t os_send(int socket, const void *message, size_t length, int flags)
{
	INT32 ret;
	ret = NU_Send(socket, (CHAR *)message, (UINT16)length, (INT16)flags);
	return (ssize_t)ret;
}


ssize_t os_sendto(int socket, const void *message, size_t length,
		int flags, const struct sockaddr *dest, socklen_t destlen)
{
	INT32 ret;
	struct addr_struct nu_addr;
	struct sockaddr_in *dest_addr = (struct sockaddr_in *)dest;

	nu_addr.family = dest_addr->sin_family;
	nu_addr.port = ntohs(dest_addr->sin_port);
	os_memcpy(nu_addr.id.is_ip_addrs, &dest_addr->sin_addr.s_addr,
			sizeof(dest_addr->sin_addr.s_addr));
	ret = NU_Send_To(socket, (CHAR *)message, (UINT16)length,
				(INT16)flags, &nu_addr, sizeof(nu_addr));
	return (ssize_t)ret;
}


ssize_t os_recv(int socket, void *buf, size_t len, int flags)
{
	INT32 ret;
	ret = NU_Recv(socket, (CHAR *)buf, (UINT16)len, (INT16)flags);
	return (ssize_t)ret;
}


ssize_t os_recvfrom(int socket, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen)
{
	INT32 ret;
	INT16 nu_addr_len;
	struct addr_struct nu_addr;
	struct sockaddr_in *from_addr = (struct sockaddr_in *)from;

	ret = NU_Recv_From(socket, buf, len, flags, &nu_addr, &nu_addr_len);
	from_addr->sin_family = nu_addr.family;
	from_addr->sin_port = htons(nu_addr.port);
	os_memcpy(&from_addr->sin_addr.s_addr, nu_addr.id.is_ip_addrs,
			sizeof(from_addr->sin_addr.s_addr));
	*fromlen = (socklen_t)sizeof(struct sockaddr);
	return (ssize_t)ret;
}


char * os_inet_ntoa(struct in_addr in)
{
	char *in_str = (char *)&in.s_addr;
	static char addr_buf[MAX_ADDRESS_SIZE * 4];
	sprintf(addr_buf, "%d.%d.%d.%d", in_str[0], in_str[1], in_str[2],
			in_str[3]);
	return addr_buf;
}

int os_close(int socket)
{
	int ret;
	ret = (int)NU_Close_Socket(socket);
	return ret;
}
