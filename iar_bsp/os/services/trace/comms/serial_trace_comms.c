/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       serial_trace_comms.c
*
*   COMPONENT
*
*       Trace Communication
*
*   DESCRIPTION
*
*       Implement the serial components for trace data transmit
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "os/services/trace/comms/trace_comms.h"

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == SERIAL_INTERFACE)
#include    "drivers/nu_drivers.h"
#include    "services/nu_trace.h"

/* Local global variables */
static SERIAL_SESSION *Serial_IO_Handle;
BOOLEAN Serial_Ready = NU_FALSE;

/***********************************************************************
*
*   FUNCTION
*
*       Serial_Trace_Comms_Open
*
*   DESCRIPTION
*
*       Open serial port for trace data transmission
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status                          return status
*
***********************************************************************/
STATUS  Serial_Trace_Comms_Open(VOID)
{
    STATUS          status;

    /* Try to open serial device */
    status = NU_Serial_Open(NU_NULL, &Serial_IO_Handle);

    if (status == NU_SUCCESS)
    {
        /* To indicate serial port is ready */
        Serial_Ready = NU_TRUE;
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Serial_Trace_Comms_Transmit
*
*   DESCRIPTION
*
*       Transmit trace data pay-load
*
*   INPUTS
*
*       buff                            - Pointer to tx buffer
*       size                            - The size of the TX buffer
*
*   OUTPUTS
*
*       status                          - Success or error code
*
***********************************************************************/
STATUS  Serial_Trace_Comms_Transmit(CHAR* buff, UINT16 size)
{
    STATUS  status = NU_TRACE_COMMS_TX_ERROR;
    UINT16  bytes_written = 0;
    UINT16  pkt_size;
    CHAR*   p_char;
    UINT32  cksum = 0;
    CHAR*   p=(CHAR*)&cksum;


    if (Serial_Ready == NU_TRUE)
    {
        p_char = buff;
        pkt_size = size;

        /* Compute checksum */
        while(pkt_size--)
        {
            cksum += *p_char;
            p_char++;
        }

        /* Append checksum */
        p_char[0]=p[0];
        p_char[1]=p[1];
        p_char[2]=p[2];
        p_char[3]=p[3];

        /* Transmit pay-load */
        status = NU_Serial_Write(Serial_IO_Handle, buff, (size + NU_TRACE_COMMS_CKSUM_SIZE), &bytes_written);

        if(status != NU_SUCCESS)
        {
            status = NU_TRACE_COMMS_TX_ERROR;
        }
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Serial_Trace_Comms_Close
*
*   DESCRIPTION
*
*       CLose serial port.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Return status
*
***********************************************************************/
STATUS  Serial_Trace_Comms_Close(VOID)
{
    /* Close serial port */
    return(NU_Serial_Close(Serial_IO_Handle));
}

/***********************************************************************
*
*   FUNCTION
*
*       Serial_Trace_Comms_Is_Ready
*
*   DESCRIPTION
*
*       Is Serial interface ready for trace communications ?
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_TRUE or NU_FALSE
*
***********************************************************************/
BOOLEAN   Serial_Trace_Comms_Is_Ready(VOID)
{
    return (Serial_Ready);
}

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == 1) */
