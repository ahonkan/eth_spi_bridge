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
*       syslogger.c
*
*   DESCRIPTION
*
*       This file contains function that allow a simple but system level
*       logger driver that logs to one medium (serial, memory, file, etc).
*
*   FUNCTIONS
*
*       nu_os_drvr_syslogger_init
*       SysLogger_Open
*       SysLogger_Close
*       SysLogger_Write
*       SysLogger_Read
*       SysLogger_Ioctl
*
***********************************************************************/

/* Include required header files */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/syslog_extern.h"
#include    "drivers/syslogger_defs.h"
#include    <string.h>
#include    <stdio.h>

/* Local function prototypes */
static 	STATUS SysLogger_Open(VOID *instance_handle,
                              DV_DEV_LABEL labels_list[],
                              INT labels_cnt,
                              VOID ** session_handle);
static 	STATUS SysLogger_Close(VOID *session_handle);
static 	STATUS SysLogger_Write(VOID *session_handle, const VOID *buffer,
                               UINT32 numbyte, OFFSET_T byte_offset,
                               UINT32 *bytes_written_ptr);
static 	STATUS SysLogger_Read(VOID *session_handle, VOID *buffer,
                              UINT32 numbyte, OFFSET_T byte_offset,
                              UINT32 *bytes_read_ptr);
static 	STATUS SysLogger_Ioctl(VOID *session_handle, INT cmd,
                               VOID *data, INT length);

/* Local variables */
static  DV_DEV_ID           SysLogger_Dev_ID;
static  SYSLOGGER_SESSION   SysLogger_Sessions[SYSLOGGER_MAX_SESSIONS];
static  INT                 SysLogger_Open_Count;
static  STATUS              SysLogger_Medium_Status;


/*************************************************************************
*
* FUNCTION
*
*       nu_os_drvr_syslogger_init
*
* DESCRIPTION
*
*       Run-level entry function for the sys logger
*
* INPUTS
*
*       key                                 Root path to registry for this driver
*       startstop                           Start or stop driver
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    nu_os_drvr_syslogger_init(const CHAR * regpath, INT startstop)
{
    DV_DRV_FUNCTIONS    drv_funcs;
    DV_DEV_LABEL        labels[] = {{SYSLOGGER_CLASS_LABEL}};


    /* Check to see if starting or stopping */
    if (startstop == 1)
    {
        /* Set-up driver functions */
        drv_funcs.drv_open_ptr  = SysLogger_Open;
        drv_funcs.drv_close_ptr = SysLogger_Close;
        drv_funcs.drv_read_ptr  = SysLogger_Read;
        drv_funcs.drv_write_ptr = SysLogger_Write;
        drv_funcs.drv_ioctl_ptr = SysLogger_Ioctl;

        /* Register this device with the Device Manager */
        DVC_Dev_Register(NU_NULL, labels,
                         DV_GET_LABEL_COUNT(labels), &drv_funcs,
                         &SysLogger_Dev_ID);
    }
    else
    {
        /* Unregister with device manager */
        DVC_Dev_Unregister(SysLogger_Dev_ID, NU_NULL);
    }
}


/*************************************************************************
*
* FUNCTION
*
*       SysLogger_Open
*
* DESCRIPTION
*
*       Syslogger device "open" function
*
* INPUTS
*
*       VOID          *instance_handle      - Instance handle of the driver
*       DV_DEV_LABEL  labels_list[]         - Access mode (label) of open
*       INT           labels_cnt            - Number of labels
*       VOID*         *session_handle       - Session handle
*
* OUTPUTS
*
*       None
*
*************************************************************************/
static 	STATUS SysLogger_Open(VOID *instance_handle, 
                              DV_DEV_LABEL labels_list[], 
                              INT labels_cnt,
                              VOID ** session_handle)
{
    INT     session_num = 0;
    INT     int_level;
    STATUS  status = NU_SUCCESS;


    /* Ignore the labels count, labels list, and instance handle - avoids compiler warnings */
    (VOID)labels_list;
    (VOID)labels_cnt;
    (VOID)instance_handle;

    /* Find available session */
    while (session_num < SYSLOGGER_MAX_SESSIONS)
    {
        /* Lock-out interrupts for critical section */
        int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Check if this session is available */
        if (SysLogger_Sessions[session_num].inuse != NU_TRUE)
        {
            /* Set the state to in use */
            SysLogger_Sessions[session_num].inuse = NU_TRUE;

            /* Restore interrupt level */
            NU_Local_Control_Interrupts(int_level);

            /* Break from loop */
            break;
        }

        /* Restore interrupt level */
        NU_Local_Control_Interrupts(int_level);

        /* Move to next session */
        session_num++;
    }

    /* Check to see if a free slot was found */
    if (session_num != SYSLOGGER_MAX_SESSIONS)
    {
        /* Set session handle */
        *session_handle = (VOID *)&SysLogger_Sessions[session_num];

        /* Set default logmask */
        SysLogger_Sessions[session_num].logmask = SYSLOGGER_DEFAULT_LOGMASK;

        /* Lock-out interrupts for critical section */
        int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Increment the open count */
        SysLogger_Open_Count++;

        /* Open medium if 1st open */
        if (SysLogger_Open_Count == 1)
        {
            /* Restore interrupt level */
            NU_Local_Control_Interrupts(int_level);

            /* Open the logging medium */
            SysLogger_Medium_Status = SysLogger_Open_Medium();
        }
        else
        {
            /* Restore interrupt level */
            NU_Local_Control_Interrupts(int_level);
        }

        /* Set return status to medium status */
        status = SysLogger_Medium_Status;
    }
    else
    {
        /* Set unsuccessful return status */
        status = -1;
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID   *handle_ptr                       - Session handle of the device
*
*   OUTPUTS
*
*       STATUS status                            - NU_SUCCESS or error code
*
*************************************************************************/
static 	STATUS SysLogger_Close(VOID *session_handle)
{
    INT                 int_level;
    SYSLOGGER_SESSION * session = (SYSLOGGER_SESSION *)session_handle;
    STATUS              status = NU_SUCCESS;


    /* Lock-out interrupts for critical section */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Free this session */
    session -> inuse = NU_FALSE;

    /* Clear mask */
    session -> logmask = 0;

    /* Decrement the open count */
    SysLogger_Open_Count--;

    /* Check to see if all closed */
    if ((SysLogger_Open_Count == 0) && (SysLogger_Medium_Status == NU_SUCCESS));
    {
        /* Close the medium */
        status = SysLogger_Close_Medium();
    }

    /* Restore interrupt level */
    NU_Local_Control_Interrupts(int_level);

    /* Return status to caller */
    return (status);   
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Write
*
*   DESCRIPTION
*
*       This function writes to the selected medium
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       INT          numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - byte offset from zero to start write
*       UINT32       *bytes_written_ptr     - Number of bytes written
*
*   OUTPUTS
*
*       INT          bytes_written          - Number of bytes written
*
*************************************************************************/
static 	STATUS SysLogger_Write(VOID *session_handle, const VOID *buffer,
                               UINT32 numbyte, OFFSET_T byte_offset,
                               UINT32 *bytes_written_ptr)
{
    SYSLOGGER_SESSION * session = (SYSLOGGER_SESSION *)session_handle;
    CHAR                temp_buf[50];
    INT                 prefix_len = 0;
    INT                 msg_len = 0;
    UINT32              ms_time;


    /* Check to see if this message should be written
       NOTE: numbyte used as mask since the logger supports NULL terminated strings */
    if (numbyte & (session -> logmask))
    {
        /* Get system clock */
        ms_time = NU_Retrieve_Clock();

#if (NU_TICKS_PER_SEC <= 1000)
        /* Convert to ms - supports 1 to 1000 ticks per second */
        ms_time = (ms_time * (1000/NU_PLUS_TICKS_PER_SEC)) ;
#else
        /* Convert to ms - supports 1001 to 10000 ticks per second */
        ms_time = (ms_time * (10000/NU_PLUS_TICKS_PER_SEC))/10 ;
#endif

        /* Create message pre-fix */
        sprintf(temp_buf, "[%s][0x%08x][%010ums]: \"", session -> name, (UINT)numbyte, (UINT)ms_time);

        /* Get length of prefix */
        prefix_len = strlen(temp_buf);

        /* Write the pre-fix */
        SysLogger_Write_Medium(temp_buf, prefix_len);

        /* Get length of message */
        msg_len = strlen((CHAR *)buffer);

        /* Now write the message */
        SysLogger_Write_Medium(buffer, msg_len);

        /* Write carriage return and line feed */
        SysLogger_Write_Medium("\"\r\n", 3);
        msg_len += 3;
    }

    /* Return number of bytes transmitted */
    return (prefix_len + msg_len);
}


/**************************************************************************
*   FUNCTION
*
*       SysLogger_Read
**   DESCRIPTION
*
*       This function reads from the selected medium
*
*   INPUTS*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       INT          numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - byte offset from zero to start write*
*   OUTPUTS
*
*       NU_SUCCESS                           Read success
*       other                                Read failed
**************************************************************************/
static 	STATUS SysLogger_Read(VOID *session_handle, VOID *buffer,
                              UINT32 numbyte, OFFSET_T byte_offset,
                              UINT32 *bytes_read_ptr)
{
    STATUS              status;


    /* Read from medium */
    status = SysLogger_Read_Medium((CHAR *)buffer, numbyte, bytes_read_ptr);

    /* Return status of read */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the SysLogger driver.
*
*   INPUTS
*       VOID          *session_handle       - Session handle of the driver
*       INT           cmd                   - Ioctl command
*       VOID          *data                 - Ioctl data pointer
*       INT           length                - Ioctl length
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS
*
*************************************************************************/
static 	STATUS SysLogger_Ioctl(VOID *session_handle, INT cmd,
                               VOID *data, INT length)
{
    SYSLOGGER_SESSION * session = (SYSLOGGER_SESSION *)session_handle;
    STATUS              status = NU_SUCCESS;
    DV_IOCTL0_STRUCT *  ioctl0 = (DV_IOCTL0_STRUCT *)data;


    /* Execute based on IOCTL command */
    switch (cmd)
    {
        case DV_IOCTL0:

            /* Make sure IOCTL data length appropriate */
            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0->base = SYSLOG_CMD_BASE;
            }

        break;

        case (SYSLOG_CMD_BASE + SYSLOG_SET_NAME_CMD):

            /* Check if length equals or exceeds maximum length */
            if (length >= SYSLOGGER_MAX_NAME_LENGTH)
            {
                /* Adjust length to maximum - 1 (for NULL termination) */
                length = SYSLOGGER_MAX_NAME_LENGTH - 1;

                /* Ensure string is null-terminated */
                session -> name[length] = '\0';                
            }

            /* Put name in session structure */
            strncpy(session -> name, (const CHAR *)data, length);

        break;

        case (SYSLOG_CMD_BASE + SYSLOG_SET_MASK_CMD):
        
            /* Check if logmask is valid */
            if (data != NU_NULL)
            {
                /* Set logmask in session structure */
                session -> logmask = *(UINT32 *)data;
            }

        break;

        default:

            /* Return invalid IOCTL command */
            status = DV_IOCTL_INVALID_CMD;

        break;
    }

    /* Return status to caller */
    return (status);
}

