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
*       device.h
*
* COMPONENT
*
*		Nucleus POSIX - file system
*
* DESCRIPTION
*
*       Contains the device I/O related structures.
*
* DATA STRUCTURES
*
*		POSIX_DEVICE_INFO					Device information structure.
*		POSIX_DEVICE_BLOCK					Device Control Block.
*		POSIX_DVD							Device Descriptor Block.
*
* DEPENDENCIES
*
*		"nucleus.h"							Contains various Nucleus PLUS
*											related definitions.
*		"mqueue.h"							Message queue related
*											definitions.
*       "plus_core.h"
*       "psxio.h"
*
*************************************************************************/
#ifndef __DEVICE_H_
#define __DEVICE_H_


#include "nucleus.h"
#include "kernel/plus_core.h"
#include "services/mqueue.h"
#include "services/psxio.h"

/* Base descriptor for the devices.All the device descriptors start from
 this base.  */
#define DEVICE_DESC_MASK    0x4000

/* Device Types */
#define DEVUNKNOWN	0x0000					/* Unknown device. */
#define DEVNULL		0x0001					/* Null device. */
#define DEVSERIAL	0x0002					/* Serial device. */
#define DEVLINEDIT	0x0003					/* Line editor. */
#define DEVETHERNET 0x0004					/* Ethernet device */
#define DEVPPP		0x0005					/* PPP device */
#define DEVMAX		0x0006					/* Maximum number of device
											   types. */

/* Base drive letter for DOS is 'A' all the way to 'Z' */
#define POSIX_BASE_DRIVE_LTR    'A'

typedef struct
{
    CHAR*          dv_name;
    VOID           *dv_info_ptr;		    /* Driver specific pointer. */
    VOID           (*dv_driver_entry)(struct PSX_DRIVER_STRUCT *, PSX_DRIVER_REQUEST *);

	NU_MEMORY_POOL *system_memory;
	UINT32		   dv_flags;			    /* Simplex, Duplex, Device is
	                                           up etc etc */
	UINT8		   dv_type;				    /* Serial, Ethernet? */
	UINT8		   padding[3];
}POSIX_DEVICE_INFO;

typedef struct
{
    CB_LINK         cb_link;                /* CB linked list. */
    INT             id;						/* Descriptor must be
	                                           unique. */
    CHAR            dev_name[NU_MAX_NAME];  /* Driver Name. */
    PSX_DRIVER*     dev_driver;
	UINT32			dev_flags;				 /* Simplex, Duplex, Device is
	                                           up etc etc */
	NU_MEMORY_POOL *system_memory;
#if (_POSIX_MESSAGE_PASSING	!=	-1)
    mqd_t           aio_queue_id;
#endif

#if (_POSIX_THREADS != -1)
	pthread_t		aio_thread_id;			/* Asynchronous thread id */
#endif

    UINT32          user_defined_1;			/* Available for users for
	                                           anything. */
    UINT32          user_defined_2;			/* Available for users for
	                                           anything. */
    UINT32          system_use_1;			/* Reserved for System use. */
    UINT32          system_use_2;			/* Reserved for System use. */
	UINT8			dev_type;				/* Serial, Ethernet? */
	UINT8			opened;					/* Device open count. */
	UINT8			padding[2];
}POSIX_DEVICE_BLOCK;

typedef struct
{
	long			oflag;					/* Device Open Flags. */
	DATA_ELEMENT	padding[4];
	int				dev_descriptor; 		/* Device Descriptor. */
	int				dcb;					/* Device Control Block
	                                           Index. */
}POSIX_DVD;


#ifdef __cplusplus
extern "C" {
#endif

/*  Function Prototypes. */
int posix_init_devices(const POSIX_DEVICE_INFO * devices,int dev_count);
POSIX_DEVICE_INFO   *dev_entry_get(char *name);
POSIX_DEVICE_BLOCK  *find_module_cb_bydesc(INT desc);
POSIX_DEVICE_BLOCK  *find_module_cb_byname(CHAR *name);
POSIX_DEVICE_BLOCK  *find_module_cb_bytid(pthread_t tid);
STATUS              POSIX_SYS_FS_Open_Disks(VOID);
VOID                POSIX_SYS_FS_Close_Disks(VOID);

#ifdef __cplusplus
}
#endif

#endif  /* __DEVICE_H_  */




