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
*       stropts.h              
*
*   COMPONENT
*
*       Nucleus POSIX - file system
*
*   DESCRIPTION
*
*       Contains the STREAMS interface.
*
*   DATA STRUCTURES
*
*       gid_t                   Used for group IDs.
*       uid_t                   Used for user IDs.
*       t_scalar_t              Used for holding flags.
*       t_uscalar_t             Used for holding flags.
*       bandinfo                Defines the bandinfo structure.
*       strbuf                  Defines the strbuf structure.
*       strpeek                 Defines the strpeek structure.
*       strfdinsert             Defines the strfdinsert structure.
*       strioctl                Defines the strioctl structure.
*       strrecvfd               Defines the strrecvfd structure.
*       str_mlist               Defines the str_mlist structure.
*       str_list                Defines the str_list structure.
*
*   DEPENDENCIES
*
*       types.h                 POSIX types.h definitions
*
*************************************************************************/

#include "services/sys/types.h"

#ifndef __STROPTS_H_
#define __STROPTS_H_

/* For DIAB tools */
#ifndef __Istropts
#define __Istropts


/* Macros to be used as a request argument to ioctl. */
#define		I_PUSH			1				/* Push a STREAMS module. */
#define		I_POP			2				/* Pop a STREAMS module. */
#define		I_LOOK			3				/* Get the top module name. */
#define		I_FLUSH			4				/* Flush a STREAM. */
#define		I_FLUSHBAND		5				/* Flush one band of a STREAM. */
#define		I_SETSIG		6				/* Ask for notification signals. */
#define		I_GETSIG		7				/* Retrieve current notification signals. */
#define		I_FIND			8				/* Look for a STREAMS module. */
#define		I_PEEK			9				/* Peek at the top message on a STREAM. */
#define		I_SRDOPT		10				/* Set the read mode. */
#define		I_GRDOPT		11				/* Get the read mode. */
#define		I_NREAD			12				/* Size the top message. */
#define		I_FDINSERT		13				/* Send implementation-defined information
											about another STREAM. */
#define		I_STR			14				/* Send a STREAMS ioctl( ). */
#define		I_SWROPT		15				/* Set the write mode. */
#define		I_GWROPT		16				/* Get the write mode. */
#define		I_SENDFD		17				/* Pass a file descriptor through a
											STREAMS pipe. */
#define		I_RECVFD		18				/* Get a file descriptor sent via I_SENDFD. */
#define		I_LIST			19				/* Get all the module names on a STREAM. */
#define		I_ATMARK		20				/* Is the top message ‘‘marked’’? */
#define		I_CKBAND		21				/* See if any messages exist in a band. */
#define		I_GETBAND		22				/* Get the band of the top message on a STREAM. */
#define		I_CANPUT		23				/* Is a band writable? */
#define		I_SETCLTIME		24				/* Set close time delay. */
#define		I_GETCLTIME		25				/* Get close time delay. */
#define		I_LINK			26				/* Connect two STREAMs. */
#define		I_UNLINK		27				/* Disconnect two STREAMs. */
#define		I_PLINK			28				/* Persistently connect two STREAMs. */
#define		I_PUNLINK		29				/* Dismantle a persistent STREAMS link. */

/* Our own defined request types. */
#define		I_BUFFALLOC		30				/* Allocate a buffer. */
#define		I_BUFFDEALLOC	31				/* Deallocate a buffer. */
#define		I_RTIMEOUT		32				/* Timeout for read operation. */
#define		I_WTIMEOUT		33				/* Timeout for write operation. */
#define		I_ALLOCTIMEOUT	34				/* Timeout for buffer allocation
											operation. */
#define		I_SFLAGS		35				/* Set the user defined flags. */
#define		I_SIFFPTR		36				/* Set the mapping table between the
											name of the physical interface and 
											pointer to the physical interface. */


/* Macro to be used with I_LOOK. */
#define		FMNAMESZ		64				/* The minimum size in bytes of the buffer
											referred to by the arg argument. */

/* Macros to be used with I_FLUSH. */
#define		FLUSHR			0x1				/* Flush read queues. */
#define		FLUSHW			0x2				/* Flush write queues. */
#define		FLUSHRW			0x3				/* Flush read and write queues. */

/* Macros to be used with I_SETSIG */
#define		S_RDNORM		0x1				/* A normal (priority band set to 0)
											message has arrived at the head of a
											STREAM head read queue. */
#define		S_RDBAND		0x2				/* A message with a non-zero priority
											band has arrived at the head of a STREAM
											head read queue. */
#define		S_INPUT			0x4				/* A message, other than a high-priority
											message,has arrived at the head of a
											STREAM head read queue. */
#define		S_HIPRI			0x8				/* A high-priority message is present on
											a STREAM head read queue. */
#define		S_OUTPUT		0x10			/* The write queue for normal data 
											(priority band 0)just below the STREAM
											head is no longer full. This notifies 
											the process that there is room on the
											queue for sending (or writing) normal
											data downstream. */
#define		S_WRNORM		0x20			/* Equivalent to S_OUTPUT. */
#define		S_WRBAND		0x40			/* The write queue for a non-zero priority
											band just below the STREAM head is no
											longer full. */
#define		S_MSG			0x80			/* A STREAMS signal message that contains
											the SIGPOLL signal reaches the front of the
											STREAM head read queue. */
#define		S_ERROR			0x100			/* Notification of an error condition
											reaches the STREAM head. */
#define		S_HANGUP		0x200			/* Notification of a hangup reaches the
											STREAM head. */
#define		S_BANDURG		0x400			/* When used in conjunction with S_RDBAND,
											SIGURG is generated instead of SIGPOLL
											when a priority message reaches the front
											of the STREAM head read queue. */


/* Macros to be used with I_PEEK. */
#define		RS_HIPRI		0x1				/* Only look for high-priority messages. */


/* Macros to be used with I_SRDOPT. */
#define		RNORM			0x1				/* Byte-STREAM mode, the default. */
#define		RMSGD			0x2				/* Message-discard mode. */
#define		RMSGN			0x4				/* Message-non-discard mode. */
#define		RPROTNORM		0x8				/* Fail read( ) with [EBADMSG] if a message
											containing a control part is at the front of
											the STREAM head read queue. */
#define		RPROTDAT		0x10			/* Deliver the control part of a message as
											data when a process issues a read(). */
#define		RPROTDIS		0x20			/* Discard the control part of a message,
											delivering any data part, when a process
											issues a read(). */


/* Macro to be used with I_SWOPT. */
#define		SNDZERO			0x1				/* Send a zero-length message downstream
											when a write( ) of 0 bytes occurs. */


/* Macros to be used with I_ATMARK. */
#define		ANYMARK			0x1				/* Check if the message is marked. */
#define		LASTMARK		0x2				/* Check if the message is the last one
											marked on the queue. */


/* Macros to be used with I_UNLINK. */
#define		MUXID_ALL		0x1				/* Unlink all STREAMs linked to the STREAM
											associated with fildes. */

/* Macros to be used by getmsg(), getpmsg(), putmsg() and putpmsg(). */
#define		MSG_ANY			0x1				/* Receive any message. */
#define		MSG_BAND		0x2				/* Receive message from specified band. */
#define		MSG_HIPRI		0x4				/* Send/receive high-priority message. */
#define		MORECTL			0x10			/* More control information is left in
											message. */
#define		MOREDATA		0x20			/* More data is left in message. */

#ifndef __T_SCALAR_T_
#define __T_SCALAR_T_

typedef long t_scalar_t;

#endif	/* __T_SCALAR_T_  */

#ifndef __T_USCALAR_T_
#define __T_USCALAR_T_

typedef unsigned long t_uscalar_t;

#endif	/* __T_USCALAR_T_  */


struct bandinfo
{

	unsigned char	bi_pri;					/* Priority Band. */
	int				bi_flag;				/* Flushing type. */

};

struct strbuf
{

	int		maxlen;							/* Maximum Buffer Length. */
	int		len;							/* Length of data. */
	char	*buf;							/* Pointer to buffer. */

};

struct strpeek
{

	struct strbuf	ctlbuf;					/* The control portion of the
											message. */
	struct strbuf	databuf;				/* The data portion of the 
											message. */
	t_uscalar_t		flags;					/* RS_HIPRI or 0. */

};

struct strfdinsert
{

	struct strbuf	ctlbuf;					/* The control portion of the 
											message. */
	struct strbuf	databuf;				/* The data portion of the
											message. */
	t_uscalar_t		flags;					/* RS_HIPRI or 0. */
	int				fildes;					/* File descriptor of the other
											stream. */
	int				offset;					/* Relative location of the
											stored value. */
};

struct strioctl
{

	int		ic_cmd;							/* ioctl() command. */
	int		ic_timeout;						/* Timeout for response. */
	int		ic_len;							/* Length of data. */
	char	*ic_dp;							/* Pointer to buffer. */

};

struct strrecvfd
{

	int		fda;							/* Received file descriptor. */
	uid_t	uid;							/* UID of sender. */
	gid_t	gid;							/* GID of sender. */

};


struct str_mlist
{

	char	l_name[FMNAMESZ + 1];			/* A STREAMS module name. */
};

struct str_list
{

	int					sl_nmods;			/* Number of STREAMS module names. */
	struct str_mlist	*sl_modlist;		/* STREAMS module names. */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Control a STREAMS device. */
int ioctl(int fildes, int request, ... /* arg */);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __Istropts 	*/
#endif	/* #ifndef __STROPTS_H_ */

