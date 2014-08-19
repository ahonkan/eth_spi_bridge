/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       psxio.h
*
*   DESCRIPTION
*
*       This file contains data structure definitions and constants for
*       the Input/Output Driver component.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
***********************************************************************/

/* Check to see if the file has been included already.  */

#ifndef _PSXIO
#define _PSXIO

#ifdef __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* Define I/O driver constants.  */
typedef enum
{
    PSX_IO_INITIALIZE   = 1,
    PSX_IO_ASSIGN       = 2,
    PSX_IO_RELEASE      = 3,
    PSX_IO_INPUT        = 4,
    PSX_IO_OUTPUT       = 5,
    PSX_IO_STATUS       = 6,
    PSX_IO_TERMINATE    = 7
} PSX_IO_FUNCTION;

/* Define I/O driver request structures.  */

typedef struct PSX_DRIVER_REQUEST_STRUCT
{
    PSX_IO_FUNCTION             psx_function;               /* I/O request function */
    STATUS                      psx_status;                 /* Status of request */
    UNSIGNED                    psx_request_size;           /* Requested size */
    UNSIGNED                    psx_actual_size;            /* Actual size */
    VOID                       *psx_buffer_ptr;             /* Buffer pointer */
} PSX_DRIVER_REQUEST;

typedef struct PSX_DRIVER_STRUCT
{
    VOID                        (*psx_driver_entry)(struct PSX_DRIVER_STRUCT *,
                                                    PSX_DRIVER_REQUEST *);
} PSX_DRIVER;

/* Core processing functions.  */
STATUS PSX_IO_Initialize_Driver(PSX_DRIVER *driver, VOID (*driver_entry)(PSX_DRIVER *, PSX_DRIVER_REQUEST *));
STATUS PSX_IO_Request_Driver(PSX_DRIVER *driver, PSX_DRIVER_REQUEST *request);

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
