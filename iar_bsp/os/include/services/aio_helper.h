/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME               
*
*       aio_helper.h         
*
* COMPONENT
*
*       Nucleus POSIX - file system
*
* DESCRIPTION
*
*       This file contains the internal routines required by Asynchronous 
*		I/O component.
*
* DATA STRUCTURES
*
*		aio_result_t						Holds result of AIO
*											transaction.
*		aiocb_in_t							Holds internal AIO
*											Control Block.
*
* DEPENDENCIES
*
*		None
*
*************************************************************************/
#ifndef __AIO_HELPER_H_
#define __AIO_HELPER_H_


#define EVENT_AIO_SUSPEND   0x1 


/* AIO Completed Successfully.  */
#define AIO_COMPL       0

/* AIO Not Completed Successfully.  */
#define AIO_UCOMPL      -1

/* Following Macros provide the status of the request.  */

/* The I/O operation is in progress.  */
#define AIO_REQINPROGRESS   0x01

/* The I/O request is on the I/O thread Queue.  */
#define AIO_REQENQ          0x02

/* The I/O request has been completed. ret_value indicates the completion
   status.  */
#define AIO_REQDONE         0x03

/* The I/O request has been canceled by the application. Discard it.  */
#define AIO_REQDISCARD      0x80000000

/* The name of the AIO Thread */
#define AIO_THREAD_NAME     "AIOQUEU"

/* Request Type.  */
#define AIO_READ            0x1
#define AIO_WRITE           0x2
#define AIO_TERMINATE		0x5

typedef struct aio_result_s
{
    unsigned long   status;                 /* Status of the request.  */
    ssize_t         ret_value;              /* Return value.  */
    unsigned long   err_code;               /* POSIX Error Codes.  */
}aio_result_t;

typedef struct aiocb_in_s
{
    int         aio_fildes;                 /* File Descriptor.  */
    off_t       aio_offset;                 /* File Offset.  */
    volatile void *aio_buf;                 /* Pointer to the buffer. */
    size_t      aio_nbytes;                 /* No Of Bytes To Transfer. */
    int         aio_reqprio;                /* Request Priority.  */
#if	(_POSIX_REALTIME_SIGNALS != -1)
	struct sigevent aio_sigevent;           /* Signal No and Value.  */
#endif
    int         aio_lio_opcode;             /* Operation to be
                                               executed.  */
    aio_result_t result;                    /* Result Value.  */
    int         sending_thread;             /* Contains ID of the sending
                                               thread.  */
    int         suspend_thread_id;          /* ID of the thread suspended
                                               on aio_suspend.  */
    char        aio_suspend;                /* AIO suspend flag.  */
    char        aio_request_type;           /* AIO request type.  */                                               
    char		padding[2];					/* Padding for the structure
    										alignment */ 
}aiocb_in_t;

#ifdef __cplusplus
extern "C" {
#endif
    
/* Function Prototypes.  */
int aio_send_request(aiocb_in_t*);  

#ifdef __cplusplus
}
#endif
    

#endif  /*  __AIO_HELPER_H_  */




