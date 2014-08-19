/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       serial.c
*
*   COMPONENT
*
*       SERIAL                              - Serial Middleware
*
*   DESCRIPTION
*
*       This file contains the Serial Middleware specific functions.
*
*   FUNCTIONS
*
*       nu_os_drvr_serial_init
*       NU_Serial_Open
*       NU_Serial_Close
*       NU_Serial_Getchar
*       NU_Serial_Putchar
*       NU_Serial_Puts
*       NU_Serial_Get_Configuration
*       NU_Serial_Set_Configuration
*       NU_Serial_Get_Read_Mode
*       NU_Serial_Set_Read_Mode
*       NU_Serial_Get_Write_Mode
*       NU_Serial_Set_Write_Mode
*       SDC_Init_Session
*       SDC_Deinit_Session
*       SDC_Reset
*       SDC_Put_Char
*       SDC_Get_Char
*       SDC_Put_String
*       SDC_Put_Stringn
*       SDC_Data_Ready
*       SIO_Device_Register_Callback
*       SIO_Device_Unregister_Callback
*       NU_SIO_Getchar
*       NU_SIO_Putchar
*       NU_SIO_Puts
*       NU_SIO_Get_Port
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"


/*********************************/
/* MACROS                        */
/*********************************/
#define RX_BUFFER_EMPTY_TO_DATA_EVT     0x1UL
#define TX_BUFFER_FULL_TO_DATA_EVT      0x2UL


/*******************************/
/* LOCAL VARIABLE DECLARATIONS */
/*******************************/
static NU_SEMAPHORE     NU_Serial_Open_Semaphore;
static CHAR             Serial_MW_Reg_Path[REG_MAX_KEY_LENGTH];
static NU_SERIAL_PORT * NU_SIO_Port;
static BOOLEAN          NU_SIO_Initialized = NU_FALSE;
static INT              Serial_IOCTL_Base;

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

static NU_HISR          SDC_HISR_Cb;
static UINT8            SDC_HISR_Stack[SERIAL_HISR_STK_SIZE];
static SERIAL_ISR_DATA  Serial_ISR_Buffer[SERIAL_ISR_DATA_BUF_SIZE];
static INT              Serial_ISR_Read;
static INT              Serial_ISR_Write;

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/*********************************/
/* INTERNAL FUNCTION PROTOTYPES  */
/*********************************/
static STATUS Serial_ID_Open(DV_DEV_ID dev_id, SERIAL_SESSION* *port);
static STATUS SDC_Init_Session (SERIAL_SESSION *port);
static STATUS SDC_Deinit_Session (SERIAL_SESSION *port);
static VOID   NU_SIO_Deinitialize (VOID);
static STATUS SIO_Device_Register_Callback(DV_DEV_ID device_id, VOID *context);
static STATUS SIO_Device_Unregister_Callback(DV_DEV_ID device_id, VOID *context);

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

static VOID             SDC_Serial_HISR (VOID);

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/*********************************/
/* FUNCTION DEFINITIONS          */
/*********************************/

/*************************************************************************
*
* FUNCTION
*
*       nu_os_drvr_serial_init
*
* DESCRIPTION
*
*       This function initializes serial middleware
*
* INPUTS
*
*       CHAR    *key                        - Path to registry
*       INT     startstop                   - Option to Register or Unregister
*
* OUTPUTS
*
*       STATUS  status                      - NU_SUCCESS for success Or
*                                             Error status from
*                                             NU_Create_Semaphore or NU_Delete_Semaphore
*
*************************************************************************/
STATUS nu_os_drvr_serial_init (const CHAR * key, INT startstop)
{
    STATUS             status = NU_SERIAL_ERROR;
    CHAR               sem_name[8];
    DV_DEV_LABEL       device_type_label[] = {{SERIAL_LABEL}, {STDIO_LABEL}};
    DV_LISTENER_HANDLE listener_id;

    if(key != NU_NULL)
    {
        if(startstop == RUNLEVEL_START)
        {
            /* Zeroize the semaphore control block */
            (VOID)memset((VOID*)&NU_Serial_Open_Semaphore, 0, sizeof(NU_SEMAPHORE));

            /* Build the name. */
            sem_name[0] = 's';
            sem_name[1] = 'e';
            sem_name[2] = 'r';
            sem_name[3] = 'o';
            sem_name[4] = 'p';
            sem_name[5] = 'e';
            sem_name[6] = 'n';
            sem_name[7] = '\0';

            /* Create the semaphore */
            status = NU_Create_Semaphore (&NU_Serial_Open_Semaphore, sem_name,
                                          (UNSIGNED)1, (OPTION)NU_FIFO);

            if (status == NU_SUCCESS)
            {
                /* Copy MW registry Path */
                strncpy(Serial_MW_Reg_Path, key, REG_MAX_KEY_LENGTH);

                /* Register callback for serial devices. */
                status =  DVC_Reg_Change_Notify(device_type_label,
                                                DV_GET_LABEL_COUNT(device_type_label),
                                                &SIO_Device_Register_Callback,
                                                &SIO_Device_Unregister_Callback,
                                                NU_NULL,
                                                &listener_id);
            }

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)
            if (status == NU_SUCCESS)
            {
                /* This HISR unblocks threads waiting to send or receive serial data. */
                status = NU_Create_HISR(&SDC_HISR_Cb, "SDHISR", SDC_Serial_HISR, SERIAL_HISR_PRIORITY,
                                        SDC_HISR_Stack, sizeof(SDC_HISR_Stack) );
            }
#endif
        }
        else if (startstop == RUNLEVEL_STOP)
        {
            /* Delete the semaphore */
            status = NU_Delete_Semaphore(&NU_Serial_Open_Semaphore);

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)
            if (status == NU_SUCCESS)
            {
                /* Delete the HISR */
                status = NU_Delete_HISR(&SDC_HISR_Cb);
            }
#endif

            if (status == NU_SUCCESS)
            {
                /* Clear the registry path */
                (VOID)memset((VOID*)Serial_MW_Reg_Path, 0, sizeof(Serial_MW_Reg_Path));
            }
        }
        else
        {
            status = NU_SUCCESS;
        }
    }

    return status;
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Serial_Open
*
* DESCRIPTION
*
*       This function opens a serial device and returns a serial middleware
*       session handle.
*
* INPUTS
*
*       DV_DEV_LABEL    *name               - Label associated with a port
*                                           - Pass null for arbitrary port
*       SERIAL_SESSION* *port               - Port for device access returned
*
* OUTPUTS
*
*       STATUS       status                 - If successful, Serial Middleware Session Handle,
*                                             a value greater equal to zero
*                                           - If error state; negative value returned
*                                             from one of following API's:
*                                              NU_Obtain_Semaphore, NU_Register_LISR,
*                                              DVC_Dev_ID_Open, DVC_Dev_ID_Get or DVC_Dev_Ioctl.
*                                             Or one of these Serial middleware errors:
*                                              NU_SERIAL_DEV_NOT_FOUND
*                                              NU_SERIAL_SESSION_UNAVAILABLE
*
*************************************************************************/
STATUS NU_Serial_Open(DV_DEV_LABEL* name, SERIAL_SESSION* *port)
{
    INT              dev_index;
    DV_DEV_LABEL     labels[2] = {{SERIAL_LABEL}};
    INT              label_cnt = 1;
    DV_DEV_ID        serial_dev_id_list[CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED];
    INT              dev_id_cnt = CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED;
    STATUS           status;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If no name was passed-in, then we will look for one label only i.e. SERIAL */
    if(name == NU_NULL)
    {
        label_cnt = 1;
    }
    /* If a name was passed-in, copy that name as second entry in label list */
    else
    {
        /* Copy user label */
        memcpy(&labels[1], name, sizeof(DV_DEV_LABEL));
        label_cnt = 2;
    }

    /* Get all devices with these labels; <SERIAL ,name>  */
    status = DVC_Dev_ID_Get (labels, label_cnt, serial_dev_id_list, &dev_id_cnt);

    if(status == NU_SUCCESS)
    {
        /* If some devices were found */
        if(dev_id_cnt > 0)
        {
            /* Attempt to open one of the matching serial devices */
            for(dev_index = 0; dev_index < dev_id_cnt; dev_index++)
            {
                status = Serial_ID_Open(serial_dev_id_list[dev_index], port);
            }
        }
        else
        {
            /* ERROR No device with that name found */
            status = NU_SERIAL_DEV_NOT_FOUND;
        }
    }

    /* Switch to user mode. */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       Serial_ID_Open
*
* DESCRIPTION
*
*       This function opens a serial device and returns a serial middleware
*       session handle.
*
* INPUTS
*
*       DV_DEV_ID       dev_id              - Serial device id
*       SERIAL_SESSION* *port               - Port for device access returned
*
* OUTPUTS
*
*       STATUS       status                 - If successful, Serial Middleware Session Handle,
*                                             a value greater equal to zero
*                                           - If error state; negative value returned
*                                             from one of following API's:
*                                             NU_Obtain_Semaphore, NU_Register_LISR,
*                                             DVC_Dev_ID_Open, DVC_Dev_ID_Get or DVC_Dev_Ioctl.
*                                             Or one of these Serial middleware errors:
*                                             NU_SERIAL_DEV_NOT_FOUND
*                                             NU_SERIAL_SESSION_UNAVAILABLE
*
*************************************************************************/
STATUS Serial_ID_Open(DV_DEV_ID dev_id, SERIAL_SESSION* *port)
{
    DV_IOCTL0_STRUCT ioctl0;
    DV_DEV_LABEL     serial_label = {SERIAL_LABEL};
    DV_DEV_HANDLE    dev_handle = DV_INVALID_HANDLE;
    STATUS           status;
    CHAR             mw_config_path[REG_MAX_KEY_LENGTH];
    STATUS           reg_status;
    SERIAL_SESSION   *serial_cb;
    NU_MEMORY_POOL   *sys_pool_ptr;


    /* Serialize access to this function */
    status = NU_Obtain_Semaphore(&NU_Serial_Open_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get system memory pool pointer */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a new session port */
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID**)&serial_cb,
                                         sizeof(SERIAL_SESSION), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                memset(serial_cb, 0, sizeof(SERIAL_SESSION));

                /* Open device in SERIAL mode */
                status = DVC_Dev_ID_Open (dev_id, &serial_label, 1, &dev_handle);

                /* If a device was opened successfully */
                if (status == NU_SUCCESS)
                {
                    /* Save device information in session */
                    serial_cb->comp_dev_handle = dev_handle;

                    /* Init ioct0 structure with SERIAL 'mode' label */
                    memcpy(&ioctl0.label, &serial_label, sizeof(DV_DEV_LABEL));

                    /* Do DV_IOCTL0 operation to get SERIAL ioctl base of device */
                    status = DVC_Dev_Ioctl (dev_handle,
                                            DV_IOCTL0, &ioctl0, sizeof(DV_IOCTL0_STRUCT));

                    if(status == NU_SUCCESS)
                    {
                        /* Save DV_IOCTL0 base in session structure */
                        Serial_IOCTL_Base = ioctl0.base;

                        /* Get the middleware config settings registry path */
                        status = DVC_Dev_Ioctl (dev_handle, (Serial_IOCTL_Base)+SERIAL_GET_MW_SETTINGS_PATH,
                                                &(mw_config_path[0]), sizeof(mw_config_path));

                        if (status == NU_SUCCESS)
                        {
                            /* Get Buffer Size from Registry */
                            reg_status = REG_Get_UINT32_Value(&(mw_config_path[0]), "/buffer_size", &(serial_cb->sd_buffer_size));
                            if(reg_status == NU_SUCCESS)
                            {
                                status = NU_SUCCESS;
                            }
                            else
                            {
                                /* Get Buffer Size from Registry using default path */
                                reg_status = REG_Get_UINT32_Value(&(Serial_MW_Reg_Path[0]), "/buffer_size", &(serial_cb->sd_buffer_size));
                                if(reg_status == NU_SUCCESS)
                                {
                                    status = NU_SUCCESS;
                                }
                                else
                                {
                                    status = NU_SERIAL_REGISTRY_ERROR;
                                }
                            }

                            if(status == NU_SUCCESS)
                            {
                                /* Get tx mode */
                                status = DVC_Dev_Ioctl (dev_handle,
                                                        (Serial_IOCTL_Base+SERIAL_COMP_GET_TX_MODE),
                                                        &serial_cb->tx_mode, sizeof(serial_cb->tx_mode));

                                    if(status == NU_SUCCESS)
                                    {
                                        /* Get rx mode */
                                        status = DVC_Dev_Ioctl (dev_handle,
                                                                (Serial_IOCTL_Base+SERIAL_COMP_GET_RX_MODE),
                                                                &serial_cb->rx_mode, sizeof(serial_cb->rx_mode));

                                        if(status == NU_SUCCESS)
                                        {
                                            /* Set MW CB in the driver */
                                            status = DVC_Dev_Ioctl (dev_handle,
                                                                    (Serial_IOCTL_Base+SERIAL_DEV_SET_MW_CB),
                                                                    serial_cb, sizeof(SERIAL_SESSION));

                                            if(status == NU_SUCCESS)
                                            {
                                                /* Enable the device */
                                                status = DVC_Dev_Ioctl (dev_handle,
                                                                        (Serial_IOCTL_Base+SERIAL_DEV_ENABLE),
                                                                        NU_NULL, NU_NULL);

                                                if (status == NU_SUCCESS)
                                                {
                                                    /* Initialize session; semaphore & buffers */
                                                    status = SDC_Init_Session(serial_cb);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        /* Everything successfully done */
                        if(status == NU_SUCCESS)
                        {
                            /* Return the session index */
                            *port = serial_cb;
                        }
                        else
                        {
                            /* Close the device */
                            (VOID)DVC_Dev_Close(dev_handle);

                            /* De-Initialize session; semaphore & buffers */
                            (VOID)SDC_Deinit_Session(serial_cb);
                        }
                    }
                    else
                    {
                        /* Deallocate memory */
                        (VOID)NU_Deallocate_Memory((VOID*)serial_cb);
                    }
                }
            }

        /* Allow other threads to use this service */
        NU_Release_Semaphore(&NU_Serial_Open_Semaphore);

    }
    else
    {
        /* ERROR No more sessions available */
        status = NU_SERIAL_SESSION_UNAVAILABLE;
    }

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Serial_Close
*
* DESCRIPTION
*
*       This function closes a serial port.
*
* INPUTS
*
*       SERIAL_SESSION *port                - Port associated with a CB
*
* OUTPUTS
*
*       STATUS       status                 - If successful, NU_SUCCESS
*                                           - If error state; negative value returned
*                                             from one of following API's:
*                                              NU_Obtain_Semaphore,
*                                              DVC_Dev_Close or DVC_Dev_Ioctl.
*                                             Or one of these Serial middleware errors:
*                                              NU_SERIAL_INVALID_SESSION
*
*************************************************************************/
STATUS NU_Serial_Close(SERIAL_SESSION *port)
{
    STATUS         status = NU_SERIAL_INVALID_SESSION;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Serialize access to this function */
    status = NU_Obtain_Semaphore(&NU_Serial_Open_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Close the device */
        (VOID)DVC_Dev_Close(port->comp_dev_handle);

        /* De-Initialize session; semaphore & buffers */
        status = SDC_Deinit_Session(port);

        /* Allow other threads to use this service */
        NU_Release_Semaphore(&NU_Serial_Open_Semaphore);
    }

    /* Switch to user mode. */
    NU_USER_MODE();

    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Getchar
*
*   DESCRIPTION
*
*       This function reads a character from the selected serial port.
*       It can be used to read in binary data as no special character
*       checking (like End Of File) is done on the data stream.
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port to get the
*                                             char from.
*
*   OUTPUTS
*
*       INT     Character                     - Character read from hardware
*               SDC_Get_Char status           - On error from SDC_Get_Char
*               NU_SERIAL_SESSION_UNAVAILABLE - If port parameter is NULL
*
***********************************************************************/
INT NU_Serial_Getchar(SERIAL_SESSION *port)
{
    INT c = NU_SERIAL_SESSION_UNAVAILABLE;

    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        /* Attempt to get character from serial driver */
        c = SDC_Get_Char (port);
    }

    /* Return character or error status */
    return (c);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Putchar
*
*   DESCRIPTION
*
*       This function abstracts out the serial driver specific
*       components for the K&R-like putchar() method
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       NU_SD_Put_Char                      - Sends a character to the serial
*                                             driver
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       INT           c                     - Character to transmit
*
*   OUTPUTS
*
*       INT           ret_val               - Character transmitted
*                                             or NU_EOF if transmit failed
*
***********************************************************************/
INT NU_Serial_Putchar(SERIAL_SESSION *port, INT c)
{
    INT ret_val = NU_EOF;

    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        /* Send character to serial driver */
        ret_val = SDC_Put_Char ((UINT8) c, port, NU_FALSE);
    }

    /* Return character written or EOF for error */
    return (ret_val);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Puts
*
*   DESCRIPTION
*
*       This function abstracts out the serial driver specific
*       components for the K&R-like puts() method
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       NU_SD_Put_String                    - Sends a string to the serial
*                                             driver
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       CHAR           *s                   - Pointer to NULL terminated
*                                             string
*
*   OUTPUTS
*
*       INT            retval               - NU_EOF if transmit failed
*                                             Otherwise a non-negative
*
***********************************************************************/
INT  NU_Serial_Puts(SERIAL_SESSION *port, const CHAR *s)
{
    INT             retval = NU_EOF;


    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        /* Send string to serial driver */
        retval = SDC_Put_String ((CHAR*) s, port);
    }

    /* Return EOF on error, non-negative otherwise */
    return (retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Get_Configuration
*
*   DESCRIPTION
*
*       This function returns the serial port configuration attributes
*
*   CALLED BY
*
*       Applications
*
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UINT32         *baud_rate           - Serial baud rate value
*       UINT32         *tx_mode             - Serial Tx mode value
*       UINT32         *rx_mode             - Serial Rx mode value
*       UINT32         *data_bits           - Serial data bits value
*       UINT32         *stop_bits           - Serial stop bits value
*       UINT32         *parity              - Serial parity value
*       UINT32         *flow_ctrl           - Serial flow control value
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS if the
*                                             configuration IOCTL call
*                                             was successful
*                                             Error returned by
*                                             DVC_Dev_Ioctl
*
***********************************************************************/
STATUS  NU_Serial_Get_Configuration (SERIAL_SESSION *port, UINT32 *baud_rate,
                             UINT32 *tx_mode, UINT32 *rx_mode, UINT32 *data_bits,
                             UINT32 *stop_bits, UINT32 *parity, UINT32 *flow_ctrl)
{
    STATUS           status;

    /* Call the serial set attribute IOCTL. */
    status =  DVC_Dev_Ioctl (port->comp_dev_handle,
                           (Serial_IOCTL_Base+SERIAL_COMP_GET_ATTR_CMD),
                           port, sizeof(SERIAL_SESSION *));

    /* Save new serial port configuration parameters. */
    *baud_rate = port->baud_rate;
    *tx_mode   = port->tx_mode;
    *rx_mode   = port->rx_mode;
    *data_bits = port->data_bits;
    *stop_bits = port->stop_bits;
    *parity    = port->parity;
    *flow_ctrl = port->flow_ctrl;

    /* Return status */
    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Set_Configuration
*
*   DESCRIPTION
*
*       This function configures the serial port
*
*   CALLED BY
*
*       Applications
*
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UINT32         baud_rate            - Serial baud rate value
*       UINT32         tx_mode              - Serial Tx mode value
*       UINT32         rx_mode              - Serial Rx mode value
*       UINT32         data_bits            - Serial data bits value
*       UINT32         stop_bits            - Serial stop bits value
*       UINT32         parity               - Serial parity value
*       UINT32         flow_ctrl            - Serial flow control value
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS if the
*                                             configuration IOCTL call
*                                             was successful
*                                             Error returned by 
*                                             DVC_Dev_Ioctl
*
***********************************************************************/
STATUS  NU_Serial_Set_Configuration (SERIAL_SESSION *port, UINT32 baud_rate,
                             UINT32 tx_mode, UINT32 rx_mode, UINT32 data_bits,
                             UINT32 stop_bits, UINT32 parity, UINT32 flow_ctrl)
{
    STATUS           status;

    /* Save new serial port configuration parameters. */
    port->baud_rate = baud_rate;
    port->tx_mode = tx_mode;
    port->rx_mode = rx_mode;
    port->data_bits = data_bits;
    port->stop_bits = stop_bits;
    port->parity = parity;
    port->flow_ctrl = flow_ctrl;

    /* Call the serial set attribute IOCTL. */
    status =  DVC_Dev_Ioctl (port->comp_dev_handle,
                           (Serial_IOCTL_Base+SERIAL_COMP_SET_ATTR_CMD),
                            port, sizeof(SERIAL_SESSION *));

    /* Return status */
    return status;

}

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Get_Read_Mode
*
*   DESCRIPTION
*
*       This API returns the read mode configuration
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UNSIGNED       *mode                - Pointer where mode is returned
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS for success
*                                             NU_SERIAL_SESSION_UNAVAILABLE for invalid handle
*                                             NU_SERIAL_ERROR if device in polled mode
*
***********************************************************************/
STATUS  NU_Serial_Get_Read_Mode(SERIAL_SESSION *port, UNSIGNED* mode)
{
    STATUS           status = NU_SERIAL_SESSION_UNAVAILABLE;


    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        if (port->rx_mode == USE_IRQ)
        {
            /* Copy mode */
            *mode = port->read_mode;

            /* Update status */
            status = NU_SUCCESS;
        }
        else
        {
            /* Device in polled mode; read_mode does not apply */
            status = NU_SERIAL_ERROR;
        }
    }

    /* Return status */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Set_Read_Mode
*
*   DESCRIPTION
*
*       This API sets the read mode configuration.
*       Depending on the timeout, this service may block.
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UNSIGNED       mode                 - Read Mode
*       UNSIGNED       timeout              - NU_NO_SUSPEND or NU_SUSPEND or (1-4,294,967,293)
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS for success
*                                             NU_SERIAL_SESSION_UNAVAILABLE for invalid handle
*                                             NU_SERIAL_ERROR if device in polled mode
*
***********************************************************************/
STATUS NU_Serial_Set_Read_Mode (SERIAL_SESSION *port, UNSIGNED mode, UNSIGNED timeout)
{
    INT         int_level;
    STATUS      status = NU_SUCCESS;


    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        /* If device in interrupt mode */
        if (port->rx_mode == USE_IRQ)
        {
            /* Disable interrupts for critical section */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Set mode */
            port->read_mode = mode;

            /* Restore interrupts to original level. */
            (VOID)NU_Local_Control_Interrupts (int_level);
        }
        else
        {
            /* Device in polled mode; read_mode does not play any role in this mode */
            status = NU_SERIAL_ERROR;
        }
    }
    else
    {
        status = NU_SERIAL_SESSION_UNAVAILABLE;
    }

    /* Return status */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Get_Write_Mode
*
*   DESCRIPTION
*
*       This API returns the write mode configuration
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UNSIGNED       *mode                - Pointer where mode is returned
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS for success
*                                             NU_SERIAL_SESSION_UNAVAILABLE for invalid handle
*                                             NU_SERIAL_ERROR if device in polled mode
*
***********************************************************************/
STATUS  NU_Serial_Get_Write_Mode(SERIAL_SESSION *port, UNSIGNED* mode)
{
    STATUS           status = NU_SERIAL_SESSION_UNAVAILABLE;


    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        if (port->tx_mode == USE_IRQ)
        {
            /* Copy mode */
            *mode = port->write_mode;

            /* Update status */
            status = NU_SUCCESS;
        }
        else
        {
            /* Device in polled mode; write_mode does not apply */
            status = NU_SERIAL_ERROR;
        }
    }

    /* Return status */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Serial_Set_Write_Mode
*
*   DESCRIPTION
*
*       This API sets the write mode configuration.
*       Depending on the timeout, this service may block.
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       SERIAL_SESSION *port                - Serial port
*       UNSIGNED       mode                 - Write Mode
*       UNSIGNED       timeout              - NU_NO_SUSPEND or NU_SUSPEND or (1-4,294,967,293)
*
*   OUTPUTS
*
*       STATUS         status               - NU_SUCCESS for success
*                                             NU_SERIAL_SESSION_UNAVAILABLE for invalid handle
*                                             NU_SERIAL_ERROR if device in polled mode
*
***********************************************************************/
STATUS NU_Serial_Set_Write_Mode (SERIAL_SESSION *port, UNSIGNED mode, UNSIGNED timeout)
{
    INT         int_level;
    STATUS      status = NU_SUCCESS;


    /* Ensure handle is valid */
    if(port != NU_NULL)
    {
        /* If device in interrupt mode */
        if (port->tx_mode == USE_IRQ)
        {
            /* Disable interrupts for critical section */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Set mode */
            port->write_mode = mode;

            /* Restore interrupts to original level. */
            (VOID)NU_Local_Control_Interrupts (int_level);
        }
        else
        {
            /* Device in polled mode; write_mode does not play any role in this mode */
            status = NU_SERIAL_ERROR;
        }
    }
    else
    {
        status = NU_SERIAL_SESSION_UNAVAILABLE;
    }

    /* Return status */
    return (status);
}

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       SDC_Init_Session
*
* DESCRIPTION
*
*       This function verifies validity of class structure values and
*       initializes the serial session structure.
*
* INPUTS
*
*       SERIAL_SESSION *port                - Serial class structure.
*
* OUTPUTS
*
*       STATUS         status               - Returns NU_SUCCESS if
*                                             successful initialization,else
*                                             a negative value is returned.
*
*************************************************************************/
static STATUS  SDC_Init_Session (SERIAL_SESSION *port)
{
    STATUS status = NU_SUCCESS;
    CHAR   sem_name[8];
    VOID   *ptr;
    NU_MEMORY_POOL *sys_pool_ptr;
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Create a semaphore that allows only one thread to use the buffers at a time and
       to keep the NU_Serial_Putchar/Puts API's are thread safe. */
    if (port != NU_NULL)
    {
        /* Zeroize the semaphore control block in the UART structure */
        (VOID)memset((VOID*)&(port->tx_semaphore), 0, sizeof(NU_SEMAPHORE));

        /* Build a name for the semaphore */
        sem_name[0] = 't';
        sem_name[1] = 'x';
        sem_name[2] = '_';
        sem_name[3] = 's';
        sem_name[4] = 'e';
        sem_name[5] = 'm';
        sem_name[6] = 'a';
        sem_name[7] = 0;

        /* Create the semaphore */
        status = NU_Create_Semaphore (&(port->tx_semaphore), sem_name,
                                      (UNSIGNED)1, (OPTION)NU_PRIORITY_INHERIT);

        /* Ensure the above operations completed successfully */
        if (status == NU_SUCCESS)
        {
            /* Get system memory pool pointer */
            status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the TX / RX data buffers. */
                status = NU_Allocate_Memory (sys_pool_ptr,
                                             &ptr,
                                             (UNSIGNED)(2UL * port->sd_buffer_size),
                                             (UNSIGNED)NU_NO_SUSPEND);
                if (status == NU_SUCCESS)
                {
                    port->tx_buffer = ptr;

                    /* Set the RX buffer to just past the TX buffer. */
                    port->rx_buffer = (UINT8*) (port->tx_buffer + port->sd_buffer_size);

                    /* Setup the RX buffer pointers and status. */
                    port->rx_buffer_read = port->rx_buffer_write = 0UL;
                    port->rx_buffer_status = NU_BUFFER_EMPTY;

                    /* Setup the TX buffer pointers and status. */
                    port->tx_buffer_read = port->tx_buffer_write = 0UL;
                    port->tx_buffer_status = NU_BUFFER_EMPTY;

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

                    /* Create event group for buffer status signaling */
                    status = NU_Create_Event_Group (&(port->ser_buffer_event), "SEREVT");

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

                    /* Initialize the error counters. */
                    SDC_Reset (port);
                }

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

               /* Initialize default read (blocking) and write (blocking) modes */
               port->read_mode = NU_SUSPEND;
               port->write_mode = NU_SUSPEND;

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */
            }
        }
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Deinit_Session
*
* DESCRIPTION
*
*       This function de-initializes the SERIAL port.
*
* INPUTS
*
*       SERIAL_SESSION *port                - Serial class structure.
*
* OUTPUTS
*
*       STATUS         status               - If successful, NU_SUCCESS
*                                           - If error state; negative value returned
*                                             from one of following API's:
*                                              NU_Deallocate_Memory,
*                                              or NU_Delete_Semaphore.
*
*************************************************************************/
static STATUS  SDC_Deinit_Session (SERIAL_SESSION *port)
{
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    /* Delete buffer event */
    (VOID)NU_Delete_Event_Group(&(port->ser_buffer_event));

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

    /* De-allocate memory for the TX / RX data buffers. */
    (VOID)NU_Deallocate_Memory (port->tx_buffer);

    /* Delete semaphore */
    (VOID)NU_Delete_Semaphore(&(port->tx_semaphore));

     /* Zeroize session structure */
     (VOID)NU_Deallocate_Memory((VOID*)port);

    /* Return status to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Reset
*
* DESCRIPTION
*
*       This function initializes the data variables associated with a UART
*
* INPUTS
*
*       SERIAL_SESSION *port                - Serial port to reset
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SDC_Reset (SERIAL_SESSION *port)
{

    /* Initialize the error counters. */
    port->frame_errors   = 0UL;
    port->overrun_errors = 0UL;
    port->parity_errors  = 0UL;
    port->busy_errors    = 0UL;
    port->general_errors = 0UL;
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Put_Char
*
* DESCRIPTION
*
*       This function writes a character out to the Serial port.
*       If device in polled mode, the function loops till the transmitter is
*       available for transmission.
*       If device in interrupt mode, both blocking and non-blocking behaviors
*       are available. The behavior provided depends on middleware configuration.
*
* INPUTS
*
*       UINT8          ch                   - Character to to be written to
*                                             the serial port.
*       SERIAL_SESSION *port                - Serial port to send the char to.
*
* OUTPUTS
*
*       INT           retval                - Character written or
*                                           - NU_EOF if write failed
*
*************************************************************************/
INT  SDC_Put_Char (UINT8 ch, SERIAL_SESSION *port, BOOLEAN caller_putstring)
{
    INT       retval = NU_EOF;
    UINT16    bytes_written;

    /* Reference unused parameters to avoid toolset warnings */
    NU_UNUSED_PARAM(caller_putstring);

    /* Transmit char */
    (VOID)NU_Serial_Write(port, (CHAR*)&ch, 1, &bytes_written);

    if (bytes_written > 0)
    {
        retval = (INT)ch;
    }

    return retval;
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Get_Char
*
* DESCRIPTION
*
*       This function reads the last received character from the Serial port.
*       Both blocking and non-blocking behaviors are available. The behavior
*       provided depends on middleware configuration.
*
* INPUTS
*
*       SERIAL_SESSION *port                - Serial port to get the
                                              char from.
*
* OUTPUTS
*
*       INT           ch                         - Character read
*                     NU_SERIAL_NO_CHAR_AVAIL    - If no characters are available
*                     DVC_Dev_Read status        - On error from DVC_Dev_Read
*                     NU_Obtain_Semaphore status - On error from NU_Obtain_Semaphore
*                     NU_Retrieve_Events status  - On error from NU_Retrieve_Events
*
*************************************************************************/
INT  SDC_Get_Char (SERIAL_SESSION *port)
{
    INT         retval;
    UINT8       ch;
    INT         int_level;
    UINT32      bytes_read = 0;
    STATUS      data_ready = NU_FALSE;
    STATUS      status = NU_SUCCESS;
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    UNSIGNED    retrieved_events;

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If polled mode */
    if (port->rx_mode == USE_POLLED)
    {
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)
        do
        {
            /* Read directly from hardware */
            status = DVC_Dev_Read (port->comp_dev_handle, &ch, 1, 0, &bytes_read);

          /* Loop here until a character is available if we are in blocking mode */
        } while ((status == NU_SUCCESS) && (bytes_read != 1));
#else
        /* Read directly from hardware */
        status = DVC_Dev_Read (port->comp_dev_handle, &ch, 1, 0, &bytes_read);

        /* If no character was received, return error */
        if ((status == NU_SUCCESS) && (bytes_read != 1))
        {
            status = NU_SERIAL_NO_CHAR_AVAIL;
        }
#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */
    }

    /* If interrupt mode */
    else
    {
        /* Disable interrupts for critical section */
        int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

        /* Check if a character is available */
        if(port->rx_buffer_status != NU_BUFFER_EMPTY)
        {
            data_ready = NU_TRUE;
        }

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

        /* If character not available and blocking mode */
        if((data_ready != NU_TRUE) && (port->read_mode != NU_NO_SUSPEND))
        {
           do
           {
               /* Restore interrupts to original level. */
               (VOID)NU_Local_Control_Interrupts (int_level);

                /* Block on the event, until the RX buffer status is empty */
                status = NU_Retrieve_Events(&(port->ser_buffer_event), RX_BUFFER_EMPTY_TO_DATA_EVT,
                                            NU_AND_CONSUME, &retrieved_events, port->read_mode);

                /* Disable interrupts for critical section */
                int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

            } while ((status == NU_SUCCESS) && (port->rx_buffer_status == NU_BUFFER_EMPTY));
        }

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

        /* If either of these conditions is true, that means data is present in buffer */
        if ((data_ready == NU_TRUE) || (status == NU_SUCCESS))
        {
            /* Check if UART is not empty */
            if (port->rx_buffer_status != NU_BUFFER_EMPTY)
            {
                /* Store the character to be returned. */
                ch = (UINT8)port->rx_buffer[port->rx_buffer_read];

                /* Move the read pointer */
                port->rx_buffer_read++;

                /* Check if the RX read pointer needs to wrap. */
                if (port->rx_buffer_read == port->sd_buffer_size)
                {
                    /* Wrap the RX read pointer */
                    port->rx_buffer_read = 0UL;
                }

                /* Check if the RX buffer is empty */
                if (port->rx_buffer_write == port->rx_buffer_read)
                {
                    /* Set the RX buffer status to empty */
                    port->rx_buffer_status = NU_BUFFER_EMPTY;
                }
                else
                {
                    /* Set the RX buffer status to show data */
                    port->rx_buffer_status = NU_BUFFER_DATA;
                }
            }
            else
            {
                /* No character was received */
                status = NU_SERIAL_NO_CHAR_AVAIL;
            }
        }

        /* Restore interrupts to original level. */
        (VOID)NU_Local_Control_Interrupts (int_level);
    }

    /* Return the character read, otherwise return the error status */
    if (status == NU_SUCCESS)
    {
        retval = (INT)ch;
    }
    else
    {
        retval = (INT)status;
    }

    /* Return to user mode. */
    NU_USER_MODE();

    /* Return character to caller */
    return (retval);
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Put_String
*
* DESCRIPTION
*
*       This function writes a null-terminated string out to the serial port.
*
* INPUTS
*
*       CHAR           *str                 - String to be written to the
*                                             serial port.
*       SERIAL_SESSION *port                - Serial port to send the
*                                             string to.
*
* OUTPUTS
*
*       INT           ret_val               - Entire string transmitted
*                                             or NU_EOF if transmit failed
*
*************************************************************************/
INT SDC_Put_String (CHAR str[], SERIAL_SESSION *port)
{
    INT retval = 0;

    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore so that strings between threads do not get mixed. */
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    if (NU_Obtain_Semaphore (&(port->tx_semaphore), port->write_mode) == NU_SUCCESS)
#else
    if (NU_Obtain_Semaphore (&(port->tx_semaphore), NU_SUSPEND) == NU_SUCCESS)

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

    {
        /* Loop through the string until the NULL terminator is found. */
        for (; (*str != (CHAR)0) && (retval != NU_EOF); str++)
        {
            /* Transmit each character of the string */
            retval = SDC_Put_Char ((UINT8)*str, port, NU_TRUE);
        }

        /* Allow other threads to use this service. */
        (VOID)NU_Release_Semaphore (&(port->tx_semaphore));
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return retval;
}


/*************************************************************************
* FUNCTION
*
*       SDC_Put_Stringn
*
* DESCRIPTION
*       This function writes a n number of characters in a string
*       out to the serial port.
*
* INPUTS
*
*       CHAR           str[]                - String to be written to the
*                                             serial port.
*       SERIAL_SESSION *port                - Serial port to send the
*                                             string to.
*       UNSIGNED       count                - Number of bytes in a string
*                                             to be written.
*
* OUTPUTS
*
*       INT           ret_val               - Entire buffer transmitted
*                                             or NU_EOF if transmit failed
*
****************************************************************************/
INT SDC_Put_Stringn (CHAR str[], SERIAL_SESSION *port, UNSIGNED count)
{
    UNSIGNED    i;
    INT         retval = 0;
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore so that strings between threads do not get mixed. */
#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    if (NU_Obtain_Semaphore (&(port->tx_semaphore), port->write_mode) == NU_SUCCESS)
#else
    if (NU_Obtain_Semaphore (&(port->tx_semaphore), NU_SUSPEND) == NU_SUCCESS)

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

    {
        /* Loop through the specified number of characters in the string. */
        for (i=0; (i<count) && (retval != NU_EOF); i++)
        {
            /* Transmit each character */
            retval = SDC_Put_Char ((UINT8)str[i], port, NU_TRUE);
        }

        /* Allow other threads to use this service. */
        (VOID)NU_Release_Semaphore (&(port->tx_semaphore));
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return retval;
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Data_Ready
*
* DESCRIPTION
*
*       This function checks to see if there are any characters in the
*       receive buffer.  A status value is returned indicating whether
*       characters are present in the receive buffer.
*
* INPUTS
*
*       SERIAL_SESSION *port                - Serial port to check for data.
*
* OUTPUTS
*
*       STATUS         data_ready           - The status indicates the
*                                             presence of characters.
*
*************************************************************************/
STATUS SDC_Data_Ready (SERIAL_SESSION *port)
{
    INT     data_ready = NU_FALSE;
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure we have a valid SERIAL_SESSION structure */
    if (port != NU_NULL)
    {
        /* Set the data ready flag based on if data is in the RX buffer */
        data_ready = (port->rx_buffer_status != NU_BUFFER_EMPTY);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    /* Return the status to caller */
    return ((STATUS)data_ready);
}

/***************** SIO Support ****************************/

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       Serial_Set_ISR_Data
*
* DESCRIPTION
*
*       This functions puts ISR data into a circular buffer and
*       activates the serial HISR to process the data
*
* INPUTS
*
*       port                                Pointer to port structure
*       tx_or_rx                            TX or RX data
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Set_ISR_Data(SERIAL_SESSION * port, INT tx_or_rx)
{
    INT       int_level;

    /* Disable interrupts for critical section */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Put the data into the circular buffer */
    Serial_ISR_Buffer[Serial_ISR_Write].port = port;
    Serial_ISR_Buffer[Serial_ISR_Write].tx_or_rx = tx_or_rx;

    /* Update write value */
    Serial_ISR_Write++;

    /* Check if write value needs to wrap */
    if (Serial_ISR_Write == SERIAL_ISR_DATA_BUF_SIZE)
    {
        /* Reset write value back to 0 */
        Serial_ISR_Write = 0;
    }

    /* Activate the serial HISR */
    NU_Activate_HISR(&SDC_HISR_Cb);

    /* Restore interrupts to original level. */
    (VOID)NU_Local_Control_Interrupts(int_level);
}


/*************************************************************************
*
* FUNCTION
*
*       Serial_Get_ISR_Data
*
* DESCRIPTION
*
*       This functions gets ISR data from a circular buffer and
*       returns the data to the caller
*
* INPUTS
*
*       port                                Pointer to storage for port
*       tx_or_rx                            Pointer to storage for tx_or_rx
*
* OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Serial_Get_ISR_Data(SERIAL_SESSION ** port, INT * tx_or_rx)
{
    INT       int_level;

    /* Disable interrupts for critical section */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get the data from the circular buffer */
    *port = Serial_ISR_Buffer[Serial_ISR_Read].port;
    *tx_or_rx = Serial_ISR_Buffer[Serial_ISR_Read].tx_or_rx;

    /* Update read value */
    Serial_ISR_Read++;

    /* Check if read value needs to wrap */
    if (Serial_ISR_Read == SERIAL_ISR_DATA_BUF_SIZE)
    {
        /* Reset read value back to 0 */
        Serial_ISR_Read = 0;
    }

    /* Restore interrupts to original level. */
    (VOID)NU_Local_Control_Interrupts (int_level);
}


/*************************************************************************
*
* FUNCTION
*
*       SDC_Serial_HISR
*
* DESCRIPTION
*
*       This HISR is called to unblock threads from TX or RX blocking
*
* INPUTS
*
*       VOID
*
* OUTPUTS
*
*       VOID
*
*************************************************************************/
static VOID SDC_Serial_HISR (VOID)
{
    SERIAL_SESSION* port;
    INT             tx_or_rx;


    /* Get ISR data */
    Serial_Get_ISR_Data(&port, &tx_or_rx);

    /* Check if data is for a TX or RX */
    if (tx_or_rx == SD_RX_INTERRUPT)
    {
        /* If port configured in blocking mode */
        if(port->read_mode != NU_NO_SUSPEND)
        {
            /* Set event as one character has been received and added to RX buffer */
            NU_Set_Events(&(port->ser_buffer_event), RX_BUFFER_EMPTY_TO_DATA_EVT, NU_OR);
        }
    }
    else
    {
        /* If port configured in blocking mode */
        if(port->write_mode != NU_NO_SUSPEND)
        {
            /* Set event as one character has been transmitted and removed from TX buffer */
            NU_Set_Events(&(port->ser_buffer_event), TX_BUFFER_FULL_TO_DATA_EVT, NU_OR);
        }
    }
}

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

/***********************************************************************
*
*   FUNCTION
*
*       SIO_Device_Register_Callback
*
*   DESCRIPTION
*
*       Callback function for new serial/sdio device addition event.
*
*   CALLED BY
*
*       DM Device Discovery Task
*
*   CALLS
*
*       Serial_ID_Open
*
*   INPUTS
*
*       device_id                   Device ID of newly registered serial
*                                   device.
*       context                     Context information for this callback.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_SERIAL_DEV_NOT_FOUND
*       NU_SERIAL_SESSION_UNAVAILABLE
*
***********************************************************************/
static STATUS SIO_Device_Register_Callback(DV_DEV_ID device_id, VOID *context)
{
    STATUS status = NU_SUCCESS;

    /* Suppress warnings */
    NU_UNUSED_PARAM(context);

    /* Proceed only iff a STDIO serial port hasn't already been opened. */
    if( NU_SIO_Initialized == NU_FALSE)
    {
        /* Open serial stdout device. */
        status = Serial_ID_Open(device_id, &NU_SIO_Port);

        /* If device was successfully opened. */
        if(status == NU_SUCCESS)
        {
            /* Set internal flag to indicate port is initialize. */
            NU_SIO_Initialized = NU_TRUE;
        }
    }

    /* Suppress warnings */
    NU_UNUSED_PARAM(device_id);

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       SIO_Device_Unregister_Callback
*
*   DESCRIPTION
*
*       Callback function for previous serial/sdio device removal event.
*
*   CALLED BY
*
*       DM Device Discovery Task
*
*   CALLS
*
*       NU_SIO_Deinitialize
*
*   INPUTS
*
*       device_id                   Device ID of newly unregistered serial
*                                   device.
*       context                     Context information for this callback.
*                                   Unused (null) for this component.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_NOT_REGISTERED
*
***********************************************************************/
static STATUS SIO_Device_Unregister_Callback(DV_DEV_ID device_id, VOID *context)
{
    STATUS status = NU_NOT_REGISTERED;

    /* Suppress warnings */
    NU_UNUSED_PARAM(device_id);
    NU_UNUSED_PARAM(context);

    /* Proceed only iff a STDIO serial port has already been opened .*/
    if( NU_SIO_Initialized == NU_TRUE)
    {
        /* Deinitialize serial stdout device. */
        NU_SIO_Deinitialize();

        /* Set internal flag to indicate port is deinitialized. */
        NU_SIO_Initialized = NU_FALSE;

        /* Set status to success. */
        status = NU_SUCCESS;
    }

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_SIO_Deinitialize
*
*   DESCRIPTION
*
*       This function sets the SIO flag to false when there are no STDIO
*       labeled Serial devices in the system.
*
*   CALLED BY
*
*       SIO
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID NU_SIO_Deinitialize (VOID)
{
    STATUS status;


    /* Proceed only iff a STDIO serial port has already been opened */
    if( NU_SIO_Initialized == NU_TRUE)
    {
        NU_SIO_Initialized = NU_FALSE;

        /* Ensure handle is valid */
        if (NU_SIO_Port != NU_NULL)
        {
            /* Serialize access to this function */
            status = NU_Obtain_Semaphore(&NU_Serial_Open_Semaphore, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* De-Initialize session; semaphore & buffers */
                (VOID)SDC_Deinit_Session(NU_SIO_Port);

                /* Allow other threads to use this service */
                NU_Release_Semaphore(&NU_Serial_Open_Semaphore);
            }
        }
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_SIO_Getchar
*
*   DESCRIPTION
*
*       This function is like the Standard I/O getchar() function
*       where the serial data is read from STDIN. It can not be used
*       to read in binary data as End Of File character checking
*       is done on the data stream.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       INT    Character                    - Character read
*              NU_EOF                       - End Of File found
*                                             or any error
*
***********************************************************************/
INT NU_SIO_Getchar(VOID)
{
    INT c;

    /* Get character from serial driver */
    c = NU_Serial_Getchar (NU_SIO_Port);

    /* Check for the End Of File character or an error status */
    if ((c == NU_SERIAL_EOF_CHAR) || (c < 0))
    {
        /* Return NU_EOF */
        c = NU_EOF;
    }

    /* Return character or NU_EOF */
    return (c);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_SIO_Putchar
*
*   DESCRIPTION
*
*       This function abstracts out the serial driver specific
*       components for the K&R-like putchar() method
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       NU_SD_Put_Char                      - Sends a character to the serial
*                                             driver
*
*   INPUTS
*
*       INT  c                              - Character to transmit
*
*   OUTPUTS
*
*       INT  c                              - Character transmitted
*                                             or NU_EOF if transmit failed
*
***********************************************************************/
INT NU_SIO_Putchar(INT c)
{
    INT retval = NU_EOF;


    /* NOTE: NU_SD_Put_Char() does not indicate errors */

    /* Do nothing unless SIO is initialized */
    if (NU_SIO_Initialized)
    {
        /* Send character to serial driver */
        retval = NU_SD_Put_Char ((UINT8)c, NU_SIO_Port, NU_FALSE);
    }

    /* Return character written or EOF for error */
    return (retval);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_SIO_Puts
*
*   DESCRIPTION
*
*       This function abstracts out the serial driver specific
*       components for the K&R-like puts() method
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       NU_SD_Put_String                    - Sends a string to the serial
*                                             driver
*
*   INPUTS
*
*       CHAR *s                             - Pointer to NULL terminated
*                                             string
*
*   OUTPUTS
*
*       INT  retval                         - NU_EOF if transmit failed
*                                             Otherwise a non-negative
*
***********************************************************************/
INT  NU_SIO_Puts(const CHAR* s)
{
    /* NOTE: NU_SD_xxx() do not indicate errors */

    INT retval = NU_EOF;

    /* Do nothing unless SIO is initialized */
    if (NU_SIO_Initialized)
    {
        /* Send string to serial driver */
        retval = NU_SD_Put_String ((CHAR*)s, NU_SIO_Port);
    }

    /* Return EOF on error, non-negative otherwise */
    return (retval);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_SIO_Get_Port
*
*   DESCRIPTION
*
*       This function return a pointer to the common serial port's UART.
*
*   CALLED BY
*
*       Applications
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       NONE
*
*   OUTPUTS
*
*       NU_SERIAL_PORT*                     - NU_SIO_Port: Pointer to the common serial
*                                             port UART.
*                                           - NU_NULL if serial I/O has not
*                                             been initialized
*
***********************************************************************/
NU_SERIAL_PORT* NU_SIO_Get_Port(VOID)
{
    /* Return serial UART if SIO is initialized */
    return (NU_SIO_Initialized ? NU_SIO_Port : NU_NULL);
}


/*************************************************************************
*
* FUNCTION
*
*       Serial_Write
*
* DESCRIPTION
*
*       This function writes a buffer out to the Serial port.
*       If device in polled mode, the function loops till the transmitter is
*       available for transmission.
*       If device in interrupt mode, both blocking and non-blocking behaviors
*       are available. The behavior provided depends on middleware configuration.
*
*       This function writes a byte to the UART
*
*   INPUTS
*
*       SERIAL_SESSION  *port               - Serial port
*       CHAR            *buff               - Buffer to copy data into
*       UINT16          size                - Size of buffer
*       UINT16          *bytes_written      - Return number of bytes written
*
*   OUTPUTS
*
*       STATUS   status                     - Status of write attempt
*
*************************************************************************/
STATUS NU_Serial_Write(SERIAL_SESSION *port, CHAR* buff, UINT16 size, UINT16* bytes_written)
{
    INT         int_level;
    INT         busy;
    STATUS      buf_avail = NU_FALSE;
    STATUS      status = NU_SUCCESS;
    BOOLEAN     initiate_write_flag = NU_FALSE;
    INT         i;


#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

    UNSIGNED  retrieved_events;

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if TX mode is polled */
    if (port->tx_mode == USE_POLLED)
    {
        for(i=0; i < size; i++)
        {
            /* Wait for TX hardware to be ready */
            do
            {
                (VOID)DVC_Dev_Ioctl (port->comp_dev_handle,
                                     (Serial_IOCTL_Base+SERIAL_COMP_IS_TXBSY_CMD),
                                      &busy, sizeof(INT));
            }while (busy == NU_TRUE);

            /* Transmit character */
            status = DVC_Dev_Write (port->comp_dev_handle, &(buff[i]), 1, 0, NU_NULL);
            busy = NU_FALSE;
        }

        *bytes_written = i;
    }
    else
    {
        for(i = 0; i < size; i++)
        {
            /* Disable interrupts for critical section */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Check if buffer is full */
            if(port->tx_buffer_status != NU_BUFFER_FULL)
            {
                buf_avail = NU_TRUE;
            }

#if (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE)

            /* If buffer space not available and blocking mode */
            if((buf_avail != NU_TRUE) && (port->write_mode != NU_NO_SUSPEND))
            {
               do
               {
                    /* Restore interrupts to original level. */
                    (VOID)NU_Local_Control_Interrupts (int_level);

                    /* Block on the event, until the TX buffer status is not full */
                    status = NU_Retrieve_Events(&(port->ser_buffer_event), TX_BUFFER_FULL_TO_DATA_EVT,
                                                NU_AND_CONSUME, &retrieved_events, port->write_mode);

                    /* Disable interrupts for critical section */
                    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                } while ((status == NU_SUCCESS) && (port->tx_buffer_status == NU_BUFFER_FULL));
            }

#endif /*  (CFG_NU_OS_DRVR_SERIAL_BLOCKING_SUPPORT == NU_TRUE) */

            /* If either of these conditions is true, that means data can be written into buffer */
            if ((buf_avail == NU_TRUE) || (status == NU_SUCCESS))
            {
                /* Add character to the buffer. */
                port->tx_buffer[port->tx_buffer_write] = buff[i];

                /* Move the write pointer */
                port->tx_buffer_write++;

                /* Check for wrap of buffer. */
                if (port->tx_buffer_write == port->sd_buffer_size)
                {
                    /* Wrap the TX buffer write pointer */
                    port->tx_buffer_write = 0UL;
                }

                /* Check if the TX buffer is not empty */
                if (port->tx_buffer_status != NU_BUFFER_EMPTY)
                {
                    /* Check for full buffer. */
                    if (port->tx_buffer_write == port->tx_buffer_read)
                    {
                        /* Set the buffer status to show full buffer */
                        port->tx_buffer_status = NU_BUFFER_FULL;
                    }
                }
                else
                {
                    /* Set status to show buffer now has data */
                    port->tx_buffer_status = NU_BUFFER_DATA;

                    /* Transmit character */
                    initiate_write_flag = NU_TRUE;
                }

                /* If this is the first character */
                if(initiate_write_flag == NU_TRUE)
                {
                    /* Transmit character */
                    status = DVC_Dev_Write (port->comp_dev_handle, &(buff[i]), 1, 0, NU_NULL);
                }
            }

            buf_avail = NU_FALSE;
            initiate_write_flag = NU_FALSE;

            /* Restore interrupts to original level. */
            (VOID)NU_Local_Control_Interrupts (int_level);
        }

        *bytes_written = i;
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return status;
}

