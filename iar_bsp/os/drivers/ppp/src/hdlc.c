/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
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
*       hdlc.c
*
*   COMPONENT
*
*       HDLC - High-level Data Link Control Protocol
*
*   DESCRIPTION
*
*       This file contains routines that organize incoming serial
*       characters into Nucleus Net buffers and outgoing Net buffers
*       to serial characters for transmitting.
*
*   DATA STRUCTURES
*
*       *_ppp_tx_dev_ptr_queue[]
*       _ppp_tx_dev_ptr_queue_read
*       _ppp_tx_dev_ptr_queue_write
*       *_ppp_rx_queue[]
*       _ppp_rx_queue_read
*       _ppp_rx_queue_write
*       fcstab[]
*
*   FUNCTIONS
*
*       HDLC_Initialize
*       HDLC_Init
*       HDLC_RX_Packet
*       HDLC_TX_Packet
*       HDLC_TX_HISR_Entry
*       HDLC_RX_HISR_Entry
*       HDLC_Compute_TX_FCS
*       HDLC_Compute_RX_FCS
*
*   DEPENDENCIES
*
*       ppp.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/ppp.h"

/* Declarations for the interrupt service routines, tasks, and
   functions that are required when interrupts are used. */

/* Array of devices that 'own' a corresponding HDLC transmit buffer. */
DV_DEVICE_ENTRY         *_ppp_tx_dev_ptr_queue[HDLC_MAX_TX_QUEUE_PTRS];
UINT8                   _ppp_tx_dev_ptr_queue_read;
UINT8                   _ppp_tx_dev_ptr_queue_write;

/* Array of HDLC receive buffer descriptors. */
HDLC_TEMP_BUFFER         *_ppp_rx_queue[HDLC_MAX_HOLDING_PACKETS_PTR];
UINT8                   _ppp_rx_queue_read;
UINT8                   _ppp_rx_queue_write;

#if HDLC_DEBUG_PRINT_OK
#define PrintInfo(s)       PPP_Printf(s)
#define PrintErr(s)        PPP_Printf(s)
#else
#define PrintInfo(s)
#define PrintErr(s)
#endif

#ifdef DEBUG_PKT_TRACE
    CHAR debug_rx_buf[PKT_TRACE_SIZE];
    INT  debug_rx_index = 0;
    CHAR debug_tx_buf[PKT_TRACE_SIZE];
    INT  debug_tx_index = 0;
#endif

INT  PPP_Open_Count = 0;

VOID *HDLC_RX_HISR_Mem;
NU_HISR PPP_RX_HISR;
VOID Serial_Rx_Task_Entry(UNSIGNED argc, VOID *argv);

#if (HDLC_POLLED_TX == NU_FALSE)
VOID *HDLC_TX_HISR_Mem;
NU_HISR PPP_TX_HISR;
VOID Serial_Tx_Task_Entry(UNSIGNED argc, VOID *argv);
#endif

/* define the lookup table for the frame check sequence computation
   method, 16 bit version. */
   static UINT16 fcstab[256] = {
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
   };


/*************************************************************************
*
* FUNCTION
*
*       HDLC_Init_Port
*
* DESCRIPTION
*
*       This function initializes the data variables associated with a UART
*
* INPUTS
*
*       DV_DEVICE_ENTRY *                   Device containing UART layer
*                                           to be initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          In case of success.
*
*************************************************************************/
STATUS  HDLC_Init_Port(DV_DEVICE_ENTRY *device)
{
    SERIAL_SESSION  *uart;
    STATUS          ret_status;
    DV_DEV_LABEL    *ser_dev_label = (DV_DEV_LABEL *)device->dev_driver_options;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    LINK_LAYER      *link_layer = (LINK_LAYER *)device->dev_link_layer;
    DV_DEV_ID       ser_device_id;
    INT             dev_count = CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED;
#endif
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Get the PPP device ID */
    ret_status = DVC_Dev_ID_Get(ser_dev_label, 1, &ser_device_id, &dev_count);

    if ((ret_status == NU_SUCCESS) && (dev_count > 0))
    {
        /* Make sure we place a min state request for the Serial
         * driver to be on, or PPP will not function over serial.
         */
        ret_status = NU_PM_Min_Power_State_Request(ser_device_id, POWER_ON_STATE,
                                                   &(link_layer->ppp_pm_handle));

        if (ret_status == NU_SUCCESS)
        {
#endif
            /* Open the port */
            ret_status = NU_Serial_Open(ser_dev_label, &uart);

            /* Ensure the initialization was successful */
            if (ret_status == NU_SUCCESS)
            {
                /* Set the device handle in the device structure */
                device->dev_handle = uart->comp_dev_handle;

                /* Set a pointer to the UART layer of this device. */
                ((PPP_LAYER *) device->ppp_layer)->uart = uart;
            }
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        }
    }
#endif
    /* Return the status of initialization to the caller */
    return (ret_status);
}

/*************************************************************************
* FUNCTION
*
*       Serial_Rx_Task_Entry
*
* DESCRIPTION
*
*       This task waits for a character to be received from the
*       serial driver.When the character has been received, it
*       calls the desired receive function of the PPP serial
*       device according to the communication mode in which the
*       device is operating.     
* INPUTS
*
*       argc                    Argument count. This is an
*                               unused parameter.
*       *argv                   Vector containing pointers
*                               to task arguments. 
* OUTPUTS
*
*       None
*
*************************************************************************/

VOID Serial_Rx_Task_Entry(UNSIGNED argc, VOID *argv)
{
    DV_DEVICE_ENTRY *dev_ptr = argv;
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    SERIAL_SESSION  *uart;
    INT             c;
    /* Get the serial port info. */
    uart = link_layer->uart;

    for(;;)
    {
        /* Attempt to get a character from the serial driver */
        c = NU_Serial_Getchar(uart);
        
        if (link_layer->comm_mode == MDM_NETWORK_COMMUNICATION)
        {
            /* Call the HDLC Receive function */
            HDLC_RX_Packet(dev_ptr, c);
        }

        if (link_layer->comm_mode == MDM_TERMINAL_COMMUNICATION)
        {
            /* Call the modem receive function */
            MDM_Receive(dev_ptr, c);
        }
       
    }
}
#if (HDLC_POLLED_TX == NU_FALSE)
/*************************************************************************
* FUNCTION
*
*       Serial_Tx_Task_Entry
*
* DESCRIPTION
*
*       This task waits for the transmission buffer of the UART
*       to get empty after receiving the event of sending the 
*       PPP packet to the serial driver.When transmission buffer
*       of the UART gets empty, it activates the PPP_TX_HISR to
*       free the NET_BUFFER used by the PPP packet.
* INPUTS
*
*       argc                    Argument count. This is an
*                               unused parameter.
*       *argv                   Vector containing pointers
*                               to task arguments. 
* OUTPUTS
*
*       None             
*
*************************************************************************/
VOID Serial_Tx_Task_Entry(UNSIGNED argc, VOID *argv)
{
    DV_DEVICE_ENTRY *dev_ptr = argv;
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    SERIAL_SESSION  *uart;
    UINT32          ret_events;
    STATUS          status;
    /* Get the serial port info. */
    uart = link_layer->uart;

    for(;;)
    {
        status = NU_Retrieve_Events(&(link_layer->ppp_tx_event), 1, NU_OR_CONSUME, &ret_events, NU_SUSPEND);
        
        if(status == NU_SUCCESS) 
        {
            while(uart->tx_buffer_status != NU_BUFFER_EMPTY);

            if(link_layer->comm_mode == MDM_NETWORK_COMMUNICATION)
            {
                _ppp_tx_dev_ptr_queue [_ppp_tx_dev_ptr_queue_write++] = dev_ptr;

                NU_Activate_HISR (&PPP_TX_HISR);

                if (_ppp_tx_dev_ptr_queue_write == PPP_MAX_TX_QUEUE_PTRS)
                {
                    /* Wrap the tx queue pointer */

                    _ppp_tx_dev_ptr_queue_write = 0;
                }
            }
        }
    }
}
#endif
/*************************************************************************
* FUNCTION
*
*     HDLC_Initialize
*
* DESCRIPTION
*
*     This function initializes the HDLC layer of the protocol stack.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device
*                                        structure for the device that
*                                        generated the RX interrupt.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS HDLC_Initialize(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variable */
    STATUS       status;
    LINK_LAYER   *link_layer;
    
    /* Initialize the HDLC layer */
    status = HDLC_Init(dev_ptr);
    
    /* if HDLC initialization is successful */
    if(status == NU_SUCCESS)
    {
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;

        /* Store MDM layer information. */
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.init = MDM_Init;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.disconnect = MDM_Hangup;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.passive = MDM_Wait_For_Client;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.connect = MDM_Dial;

        /* Initialize the MODEM module. */
        status = MDM_Init(dev_ptr);
    }
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to initialize the modem in HDLC_Initialize",NERR_SEVERE, __FILE__, __LINE__);
    }
    else
    {
        status = NU_Allocate_Memory(PPP_Memory, &(link_layer->ppp_serial_rx_task_mem), PPP_TASK_STACK_SIZE, NU_NO_SUSPEND);
        
        if (status == NU_SUCCESS)
        {
            status = NU_Create_Task(&(link_layer->serial_rx_task), "ser_rx",
                                    Serial_Rx_Task_Entry, 0, dev_ptr, (link_layer->ppp_serial_rx_task_mem),
                                    PPP_TASK_STACK_SIZE, PPP_TASK_PRIORITY, PPP_TASK_TIME_SLICE, PPP_TASK_PREEMPT, NU_NO_START);
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to create the task for the Modem to receive data in HDLC_Initialize",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            else
            {       
                /* Start the Serial Rx task. */
                status = NU_Resume_Task(&(link_layer->serial_rx_task));

                if(status != NU_SUCCESS)
                {
                    return (status);
                }
#if (HDLC_POLLED_TX == NU_FALSE)

                status = NU_Allocate_Memory(PPP_Memory, &(link_layer->ppp_serial_tx_task_mem), PPP_TASK_STACK_SIZE, NU_NO_SUSPEND);
                
                if(status == NU_SUCCESS)
                {
                    status = NU_Create_Task(&(link_layer->serial_tx_task), "ser_tx",
                                            Serial_Tx_Task_Entry, 0, dev_ptr, (link_layer->ppp_serial_tx_task_mem),
                                            PPP_TASK_STACK_SIZE, PPP_TASK_PRIORITY, PPP_TASK_TIME_SLICE, PPP_TASK_PREEMPT, NU_NO_START);
                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to create the task for the Modem to transmit data in HDLC_Initialize",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }
                    else
                    {
                        /* Start the Serial Tx task. */
                        status = NU_Resume_Task(&(link_layer->serial_tx_task));

                        if(status != NU_SUCCESS)
                        {
                            return (status);
                        }
                    }    
                }
                else
                {
                    NLOG_Error_Log("Failed to allocate memory for serial_tx_task in HDLC_Initialize",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }  
#endif
            }
        }
        else
        {
             NLOG_Error_Log("Failed to allocate memory for serial_rx_task in HDLC_Initialize",
                             NERR_SEVERE, __FILE__, __LINE__);
        }  
    }
           
    /* return status */
    return status;

} /* HDLC_Initialize */



/*************************************************************************
* FUNCTION
*
*     HDLC_Init
*
* DESCRIPTION
*
*     This function initializes the HDLC layer of the protocol stack.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr         Pointer to the device
*                                        structure for the device that
*                                        generated the RX interrupt.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS HDLC_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    VOID            *pointer;
    LINK_LAYER      *ppp_layer_ptr;
    STATUS          status;

    /* Set the PPP Memory Pool to the same pool as Net. */
    if (PPP_Memory == NU_NULL)
        PPP_Memory = MEM_Cached;

    /* Only create the HISR once. The same one will be used for all PPP
       links. */
    if (PPP_Open_Count++ == 0)
    {
        /* Allocate a block of memory for the PPP RX HISR stack. */
        if (NU_Allocate_Memory (PPP_Memory,
                            &HDLC_RX_HISR_Mem,
                            PPP_HISR_STACK_SIZE,
                            NU_NO_SUSPEND
                           ) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory for PPP RX HISR.",
                NERR_FATAL, __FILE__, __LINE__);
            return(-1);
        }

        /* Create the HISR. */
        if (NU_Create_HISR (&PPP_RX_HISR, "HDLC_RX",
                        HDLC_RX_HISR_Entry,
                        0, HDLC_RX_HISR_Mem, PPP_HISR_STACK_SIZE) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create PPP RX HISR.",
                NERR_FATAL, __FILE__, __LINE__);
            return(-1);
        }

#if (HDLC_POLLED_TX == NU_FALSE)
        /* Allocate a block of memory for the PPP TX HISR stack. */
        if (NU_Allocate_Memory (PPP_Memory,
                            &HDLC_TX_HISR_Mem,
                            PPP_HISR_STACK_SIZE,
                            NU_NO_SUSPEND
                           ) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory for PPP TX HISR.",
                NERR_FATAL, __FILE__, __LINE__);
            return(-1);
        }

        /* Create the HISR. */
        if (NU_Create_HISR (&PPP_TX_HISR, "HDLC_TX",
                        HDLC_TX_HISR_Entry,
                        0, HDLC_TX_HISR_Mem, PPP_HISR_STACK_SIZE) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create PPP TX HISR.",
                NERR_FATAL, __FILE__, __LINE__);
            return(-1);
        }

        /* Initialize the globals */
        _ppp_tx_dev_ptr_queue_read      = 0;
        _ppp_tx_dev_ptr_queue_write     = 0;
#endif

        _ppp_rx_queue_read   = 0;
        _ppp_rx_queue_write  = 0;
    }

    /* Fill in the device */
    dev_ptr->dev_output     = PPP_Output;
    dev_ptr->dev_input      = PPP_Input;
    dev_ptr->dev_start      = HDLC_TX_Packet;

    /* Set the MTU if it hasn't been set yet, or it is set to
       a higher value. */
    if (dev_ptr->dev_mtu == 0 || dev_ptr->dev_mtu > HDLC_MTU)
        dev_ptr->dev_mtu = HDLC_MTU;

    /* Each link protocol adds a header size to a total for Net to use. */
    dev_ptr->dev_hdrlen     += (UINT8)HDLC_MAX_ADDR_CONTROL_SIZE;

    /* Allocate memory for the PPP layer structure. This structure will
       be pointed to by the device structure for this device. */
    if (NU_Allocate_Memory (PPP_Memory,
                            (VOID **)&ppp_layer_ptr,
                            sizeof (LINK_LAYER),
                            NU_NO_SUSPEND
                           ) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for PPP data structure.",
            NERR_FATAL, __FILE__, __LINE__);
        return(-1);
    }

    /* Zero out the PPP layer information. */
    UTL_Zero ((VOID *)ppp_layer_ptr, sizeof (LINK_LAYER));

    /* Store the address of the ppp layer structure. */
    dev_ptr->dev_link_layer = (VOID *)ppp_layer_ptr;

    /* Assign the type of PPP hardware interface */
    ppp_layer_ptr->hwi.itype = PPP_ITYPE_UART | PPP_ITYPE_MODEM;

    /* Store the address of the device structure for the PPP layer. */
    ppp_layer_ptr->hwi.dev_ptr = dev_ptr;
    ppp_layer_ptr->hwi.hdev_ptr = dev_ptr;

    /* Initialize the connection status. */
    ppp_layer_ptr->connection_status = NU_PPP_DISCONNECTED;

    /* Allocate memory for serial buffers. */
    status = NU_Allocate_Memory(PPP_Memory, &pointer,
        (sizeof(HDLC_TEMP_BUFFER) * HDLC_MAX_HOLDING_PACKETS), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for HDLC buffers.",
            NERR_FATAL, __FILE__, __LINE__);
        return(-1);
    }

    /* Assign the new buffer structure to this PPP device. */
    ppp_layer_ptr->rx_ring = (HDLC_TEMP_BUFFER HUGE *)pointer;

    /* Initialize the UART. */
    status = HDLC_Init_Port(dev_ptr);
    if (status != NU_SUCCESS)
        return (status);

    /* Initialize upper PPP layer for this device. */
    status = PPP_Initialize(dev_ptr);
    if (status == NU_SUCCESS)
    {
        MIB2_ifType_Seti(dev_ptr->dev_index, 23);
        MIB2_ifSpecific_Set(dev_ptr->dev_index, (UINT8*)"ppp");
        MIB2_ifDescr_Seti(dev_ptr->dev_index, "Nucleus PPP interface v3.2");
        MIB2_ifMtu_Set(dev_ptr->dev_index, dev_ptr->dev_mtu);
        MIB2_ifSpeed_Seti(dev_ptr->dev_index, dev_ptr->dev_baud_rate);
    }
    else
    {
        /* Log a fatal error. */
        NLOG_Error_Log("PPP_Initialize failed.",
            NERR_FATAL, __FILE__, __LINE__);
    }

    return status;

} /* HDLC_Init */



/*************************************************************************
* FUNCTION
*
*     HDLC_RX_Packet
*
* DESCRIPTION
*
*     This function is called from the LISR whenever a
*     character receive interrupt occurs.  It reads the received character
*     from the UART and adds it to the buffer.  When a complete packet has
*     been received the HISR will be activated to notify the upper layer
*     software that a packet has arrived.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *device         Pointer to the device
*                                        structure for the device that
*                                        generated the RX interrupt.
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS is always returned
*
*************************************************************************/
STATUS HDLC_RX_Packet(DV_DEVICE_ENTRY *device, INT c)
{
    LINK_LAYER      *link_layer;

    /* Get a pointer to the PPP link layer structure. */
    link_layer = (LINK_LAYER *)device->dev_link_layer;

    /* Was the previous character received an escape character. */
    /* The character following the escape needs to be replaced with
       its XOR value. */
    if (link_layer->esc_received)
    {
        switch(c)
        {
            case (PPP_HDLC_FRAME ^ PPP_HDLC_TRANSPARENCY):

                if (link_layer->received < PPP_MAX_RX_SIZE)
                {
                    link_layer->rx_ring[link_layer->rx_ring_write].buffer[link_layer->received++]
                                                = PPP_HDLC_FRAME;

#ifdef DEBUG_PKT_TRACE
                    if (debug_rx_index == PKT_TRACE_SIZE)
                        debug_rx_index = 0;

                    debug_rx_buf[debug_rx_index++] = PPP_HDLC_FRAME;
#endif
                }

                break;

            case (PPP_HDLC_CONTROL_ESCAPE ^ PPP_HDLC_TRANSPARENCY):

                if (link_layer->received < PPP_MAX_RX_SIZE)
                {
                    link_layer->rx_ring[link_layer->rx_ring_write].buffer[link_layer->received++]
                                                = PPP_HDLC_CONTROL_ESCAPE;


#ifdef DEBUG_PKT_TRACE
                    if (debug_rx_index == PKT_TRACE_SIZE)
                        debug_rx_index = 0;

                    debug_rx_buf[debug_rx_index++] = PPP_HDLC_CONTROL_ESCAPE;
#endif
                }

                break;

            default:

                if (link_layer->received < PPP_MAX_RX_SIZE)
                {
                    link_layer->rx_ring[link_layer->rx_ring_write].buffer[link_layer->received++]
                                                = ( c ^PPP_HDLC_TRANSPARENCY);


#ifdef DEBUG_PKT_TRACE
                    if (debug_rx_index == PKT_TRACE_SIZE)
                        debug_rx_index = 0;

                    debug_rx_buf[debug_rx_index++] = (c ^ PPP_HDLC_TRANSPARENCY);
#endif
                }

                break;
        }

        /* Reset the escape received flag. */
        link_layer->esc_received = NU_FALSE;
    }

    /* This is either just another character or its the end of the
       frame. */

    else
    {
        switch (c)
        {
            /* If it's an END character, then we're done with the packet. */

            case PPP_HDLC_FRAME:

                /* A minor optimization:  If there is no data in the packet,
                   ignore it.  This is meant to avoid bothering IP with all the
                   empty packets generated by the duplicate END characters which
                   are in Serial Line IP.
                */

                if(link_layer->received)
                {
                    /* Store the length of this packet and a pointer to it for
                       the HISR */

                    /*  Store the device for this interrupt. */
                    link_layer->rx_ring[link_layer->rx_ring_write].device = device;

                    /* Update the length of current buffer. */
                    link_layer->rx_ring[link_layer->rx_ring_write].size = link_layer->received;

                    /* Add this packet to the HISR's packet pointer list. Make
                       sure there is room first. We must also check for looping
                       of the write pointer. */
                    if ((_ppp_rx_queue_write + 1) >= HDLC_MAX_HOLDING_PACKETS_PTR)
                    {
                        /* This is the case if it does wrap. */
                        if (_ppp_rx_queue_read != 0)
                        {
                            _ppp_rx_queue[_ppp_rx_queue_write] = (HDLC_TEMP_BUFFER *)
                                            &link_layer->rx_ring[link_layer->rx_ring_write++];

                            /* Wrap the HISR write pointer. */
                            _ppp_rx_queue_write = 0;

                            /* If using SNMP, this will update the MIB */
                            MIB2_ifInOctets_Addi(device->dev_index, link_layer->received);

                            /* Activate the HISR.  The HISR will inform the upper layer
                               protocols that a packet has arrived. */
                            NU_Activate_HISR (&PPP_RX_HISR);
                        }
                        else
                            NLOG_Error_Log("Misaligned pointers to HDLC buffer data.", NERR_SEVERE, __FILE__, __LINE__);

                    }
                    else
                    {
                        /* This is the case if it doesn't wrap */
                        if ((_ppp_rx_queue_write + 1) != _ppp_rx_queue_read)
                        {
                            _ppp_rx_queue[_ppp_rx_queue_write++] = (HDLC_TEMP_BUFFER *)
                                        &link_layer->rx_ring[link_layer->rx_ring_write++];

                            /* If using SNMP, this will update the MIB */
                            MIB2_ifInOctets_Addi(device->dev_index, link_layer->received);

                            /* Update the total data received. */
                            link_layer->data_received = link_layer->received;

                            /* Activate the HISR.  The HISR will inform the upper layer
                               protocols that a packet has arrived. */
                            NU_Activate_HISR (&PPP_RX_HISR);
                        }
                        else
                            NLOG_Error_Log("Misaligned pointers to HDLC buffer data.", NERR_SEVERE, __FILE__, __LINE__);

                    }

                    /* Make sure the LISR index wraps around if needed. */
                    link_layer->rx_ring_write %= (UINT8) HDLC_MAX_HOLDING_PACKETS;

                    /* Reset the number of bytes received. */
                    link_layer->received = 0;

                }

                break;

            case PPP_HDLC_CONTROL_ESCAPE:

                /* Set the escape received flag. */
                link_layer->esc_received = NU_TRUE;

                break;

            default:

                if (link_layer->received < PPP_MAX_RX_SIZE)
                {
                    /* Store the char if we are under our MTU and if the char
                       is not one that is supposed to be control escaped. */
                    if (c < PPP_MAX_ACCM)
                    {
                        if (!(PPP_Two_To_Power(c) & (UINT32)(((LINK_LAYER *)device->dev_link_layer)
                            ->lcp.options.local.accm)))
                        {
#ifdef DEBUG_PKT_TRACE
                            if (debug_rx_index == PKT_TRACE_SIZE)
                                debug_rx_index = 0;

                            debug_rx_buf [debug_rx_index++] = c;
#endif

                        link_layer->rx_ring[link_layer->rx_ring_write].buffer[link_layer->received++] = c;
                        }
                    }
                    else
                    {
                        link_layer->rx_ring[link_layer->rx_ring_write].buffer[link_layer->received++] = c;
                    }
                }
                break;
        }

    }

    return (NU_SUCCESS);

} /* HDLC_RX_Packet */

/*************************************************************************
* FUNCTION
*
*     HDLC_TX_Packet
*
* DESCRIPTION
*
*     This function will send the PPP packet. This includes adding the
*     correct PPP header and computing the Frame Check Sequence over the
*     packet.
*
* INPUTS
*
*     DEV_DEVICE_ENTRY  *device         Pointer to device structure for
*                                        the device to send the packet
*                                        over.
*     NET_BUFFER        *buf_ptr        Pointer to the buffer that
*                                        holds the packet to be sent.
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS is always returned
*
*************************************************************************/
STATUS HDLC_TX_Packet(DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr)
{
    NET_BUFFER          *tmp_buf_ptr = buf_ptr;
    SERIAL_SESSION      *uart;
    LCP_LAYER           *lcp;
    UINT16              frame_check, pkt_type;
    UINT8       HUGE    *ppp_header_ptr;
    INT16               length;
    UINT8               more_to_send = NU_TRUE;

    /* Get the UART structure for this device. */
    uart = ((LINK_LAYER *)device->dev_link_layer)->uart;

    /* Get the LCP structure for this device. */
    lcp = &((LINK_LAYER *)device->dev_link_layer)->lcp;

    /* See if we need to compress the PPP header. This is only
       valid for IP packets. */
#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE))
    if (buf_ptr->mem_flags & (NET_IP | NET_IP6))
#elif (INCLUDE_IPV6 == NU_TRUE)
    if (buf_ptr->mem_flags & NET_IP6)
#else
    if (buf_ptr->mem_flags & NET_IP)
#endif
    {
        /* See if we need to compress the protocol header. */
        if (lcp->options.remote.flags & PPP_FLAG_PFC)
        {
            /* The protocol is already written to the packet in full size,
               so we just need to adjust the data pointer by one byte. */
            buf_ptr->data_ptr           += PPP_PROTOCOL_HEADER_1BYTE;
            buf_ptr->data_len           -= PPP_PROTOCOL_HEADER_1BYTE;
            buf_ptr->mem_total_data_len -= PPP_PROTOCOL_HEADER_1BYTE;
        }

        /* See if we need to write the address/control header. */
        if (!(lcp->options.remote.flags & PPP_FLAG_ACC))
        {
            /* Make room for the address/control header and adjust the lengths */
            buf_ptr->data_ptr           -= HDLC_MAX_ADDR_CONTROL_SIZE;
            buf_ptr->data_len           += HDLC_MAX_ADDR_CONTROL_SIZE;
            buf_ptr->mem_total_data_len += HDLC_MAX_ADDR_CONTROL_SIZE;

            /* Add the HDLC header information. */
            PUT16(buf_ptr->data_ptr, 0, PPP_ADDR_CONTROL);
        }
    }
    else
    {
        /* Always add the address/control field for non-IP packets. */
        PPP_Add_Protocol(buf_ptr, PPP_ADDR_CONTROL);
    }

    /* Now compute the FCS. */
    frame_check = HDLC_Compute_TX_FCS(PPP_INIT_FCS16, buf_ptr);
    frame_check ^= 0xffff;                  /* get the complement */

#if (HDLC_POLLED_TX == NU_TRUE)

    /* Send a FLAG character to flush out any characters the remote host might
       have received because of line noise and to mark the beginning of this
       PPP HDLC frame. */

    NU_Serial_Putchar(uart, PPP_HDLC_FRAME);

#endif

    /* Loop through all the buffers in the chain and send out each byte. */
    while (more_to_send)
    {
        /* Get the length so that we can loop through the packet and send out
           each byte. */
        length = (UINT16) tmp_buf_ptr->data_len;

        /* Get a pointer to the data. */
        ppp_header_ptr = tmp_buf_ptr->data_ptr;

        /* Now loop through the encapsulated packet and send it out */
        while(length-- > 0)
        {
            switch((CHAR)*ppp_header_ptr)
            {
                /* If it's the same code as a FRAME character, then send the special
                   two character code so that the receiver does not think we sent
                   a FRAME.
                */
                case PPP_HDLC_FRAME:
#ifdef DEBUG_PKT_TRACE
                    if (debug_tx_index == PKT_TRACE_SIZE)
                        debug_tx_index = 0;

                    debug_tx_buf [debug_tx_index++] = (PPP_HDLC_FRAME);

#endif

#if (HDLC_POLLED_TX == NU_TRUE)
                    NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                    NU_Serial_Putchar(uart, PPP_HDLC_FRAME ^ PPP_HDLC_TRANSPARENCY);
#else
                    /* Store the byte. */
                    uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                    /* Check for wrapping of the write pointer. */
                    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                    /* Store the byte. */
                    uart->tx_buffer[uart->tx_buffer_write++] = (PPP_HDLC_FRAME ^ PPP_HDLC_TRANSPARENCY);

                    /* Check for wrapping of the write pointer. */
                    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif

                    break;

                /* If it's the same code as an ESC character, then send the special
                   two character code so that the receiver does not think we sent
                   an ESC.
                */
            case PPP_HDLC_CONTROL_ESCAPE:
#ifdef DEBUG_PKT_TRACE
                    if (debug_tx_index == PKT_TRACE_SIZE)
                        debug_tx_index = 0;

                    debug_tx_buf [debug_tx_index++] = PPP_HDLC_CONTROL_ESCAPE;

#endif

#if (HDLC_POLLED_TX == NU_TRUE)
                    NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                    NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE ^ PPP_HDLC_TRANSPARENCY);
#else
                    /* Store the byte. */
                    uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                    /* Check for wrapping of the write pointer. */
                    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                    /* Store the byte. */
                    uart->tx_buffer[uart->tx_buffer_write++] = (PPP_HDLC_CONTROL_ESCAPE ^ PPP_HDLC_TRANSPARENCY);

                    /* Check for wrapping of the write pointer. */
                    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif

                    break;

                /* Otherwise it is just a regular character. We will either send
                   it or encode it depending on the ACCM. */
                default:

#ifdef DEBUG_PKT_TRACE
                        if (debug_tx_index == PKT_TRACE_SIZE)
                            debug_tx_index = 0;

                        debug_tx_buf [debug_tx_index++] = *ppp_header_ptr;
#endif

                    /* Since the ACCM is only for the first 32 chars. see if
                       we even need to check it for this one. */
                    if (*ppp_header_ptr >= PPP_MAX_ACCM)
                    {
                       /* guess not so send it */
#if (HDLC_POLLED_TX == NU_TRUE)
                        NU_Serial_Putchar(uart, *ppp_header_ptr);
#else
                        /* Store the byte. */
                        uart->tx_buffer[uart->tx_buffer_write++] = *ppp_header_ptr;

                        /* Check for wrapping of the write pointer. */
                        uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                    }
                    else

                        /* else check the character against the map */

                        if ((PPP_Two_To_Power(*ppp_header_ptr) & (UINT32)lcp->options.remote.accm)
                            || lcp->state != OPENED)
                        {

                            /* It is in the map so send the two char code. */

#if (HDLC_POLLED_TX == NU_TRUE)
                            NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                            NU_Serial_Putchar(uart, (CHAR) (*ppp_header_ptr ^ PPP_HDLC_TRANSPARENCY));
#else
                            /* Store the byte. */
                            uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                            /* Check for wrapping of the write pointer. */
                            uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                            /* Store the byte. */
                            uart->tx_buffer[uart->tx_buffer_write++] = (*ppp_header_ptr ^ PPP_HDLC_TRANSPARENCY);

                            /* Check for wrapping of the write pointer. */
                            uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                        }
                        else
                        {
                            /* It is not in the map so send it in the clear. */
#if (HDLC_POLLED_TX == NU_TRUE)
                            NU_Serial_Putchar(uart, *ppp_header_ptr);
#else
                            /* Store the byte. */
                            uart->tx_buffer[uart->tx_buffer_write++] = *ppp_header_ptr;

                            /* Check for wrapping of the write pointer. */
                            uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif

                        }
            }

            /* Move to the next byte. */
            ppp_header_ptr++;
        }

        /* Move the buffer pointer to the next buffer. */
        tmp_buf_ptr = tmp_buf_ptr->next_buffer;

        /* If there are no more buffers then there is no more to send. */
        if (!tmp_buf_ptr)
        {
            more_to_send = NU_FALSE;
        }

    }


    /* Now set the length to two for the two bytes of the FCS that are appended
       to the end of each packet. */
    length = HDLC_FCS_SIZE;

    /* Swap the FCS, remember that the FCS is put on LSB first. */

    /* Reuse the pkt_type variable. */
    pkt_type = frame_check;

    /* Get a pointer to the FCS. */
    ppp_header_ptr = (UINT8 *) &frame_check;

    /* Swap it. */
    ppp_header_ptr[0] = (UINT8) pkt_type;
    ppp_header_ptr[1] = (UINT8) (pkt_type >> 8);

    /* Put the FCS on the end of the packet. */
    while (length--)
    {
        switch((CHAR)*ppp_header_ptr)
        {
            /* If it's the same code as a FRAME character, then send the special
               two character code so that the receiver does not think we sent
               a FRAME.
            */
            case PPP_HDLC_FRAME:
#ifdef DEBUG_PKT_TRACE
                if (debug_tx_index == PKT_TRACE_SIZE)
                    debug_tx_index = 0;

                debug_tx_buf [debug_tx_index++] = (PPP_HDLC_FRAME);

#endif

#if (HDLC_POLLED_TX == NU_TRUE)
                NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                NU_Serial_Putchar(uart, PPP_HDLC_FRAME ^ PPP_HDLC_TRANSPARENCY);
#else
                /* Store the byte. */
                uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                /* Check for wrapping of the write pointer. */
                uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                /* Store the byte. */
                uart->tx_buffer[uart->tx_buffer_write++] = (PPP_HDLC_FRAME ^ PPP_HDLC_TRANSPARENCY);

                /* Check for wrapping of the write pointer. */
                uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                break;

            /* If it's the same code as an ESC character, then send the special
               two character code so that the receiver does not think we sent
               an ESC.
            */
            case PPP_HDLC_CONTROL_ESCAPE:
#ifdef DEBUG_PKT_TRACE
                if (debug_tx_index == PKT_TRACE_SIZE)
                    debug_tx_index = 0;

                debug_tx_buf [debug_tx_index++] = PPP_HDLC_CONTROL_ESCAPE;

#endif

#if (HDLC_POLLED_TX == NU_TRUE)
                NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE ^ PPP_HDLC_TRANSPARENCY);
#else
                /* Store the byte. */
                uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                /* Check for wrapping of the write pointer. */
                uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                /* Store the byte. */
                uart->tx_buffer[uart->tx_buffer_write++] = (PPP_HDLC_CONTROL_ESCAPE ^ PPP_HDLC_TRANSPARENCY);

                /* Check for wrapping of the write pointer. */
                uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                break;

            /* Otherwise it is just a regular character. We will either send
               it or encode it depending on the ACCM. */
            default:

#ifdef DEBUG_PKT_TRACE
                    if (debug_tx_index == PKT_TRACE_SIZE)
                        debug_tx_index = 0;

                    debug_tx_buf [debug_tx_index++] = *ppp_header_ptr;
#endif

                /* Since the ACCM is only for the first 32 chars. see if
                   we even need to check it for this one. */
                if (*ppp_header_ptr >= PPP_MAX_ACCM)
                {
                    /* guess not so send it */
#if (HDLC_POLLED_TX == NU_TRUE)
                    NU_Serial_Putchar(uart, *ppp_header_ptr);
#else
                    /* Store the byte. */
                    uart->tx_buffer[uart->tx_buffer_write++] = *ppp_header_ptr;

                    /* Check for wrapping of the write pointer. */
                    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                }
                else

                    /* else check the character against the map */

                    if ((PPP_Two_To_Power(*ppp_header_ptr) & (UINT32)lcp->options.remote.accm)
                            || lcp->state != OPENED)
                    {

                        /* It is in the map so send the two char code. */
#if (HDLC_POLLED_TX == NU_TRUE)
                        NU_Serial_Putchar(uart, PPP_HDLC_CONTROL_ESCAPE);
                        NU_Serial_Putchar(uart, (CHAR) (*ppp_header_ptr ^ PPP_HDLC_TRANSPARENCY));
#else
                        /* Store the byte. */
                        uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_CONTROL_ESCAPE;

                        /* Check for wrapping of the write pointer. */
                        uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

                        /* Store the byte. */
                        uart->tx_buffer[uart->tx_buffer_write++] = (*ppp_header_ptr ^ PPP_HDLC_TRANSPARENCY);

                        /* Check for wrapping of the write pointer. */
                        uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif
                    }
                    else
                    {
                        /* It is not in the map so send it in the clear. */
#if (HDLC_POLLED_TX == NU_TRUE)
                        NU_Serial_Putchar(uart, *ppp_header_ptr);
#else
                        /* Store the byte. */
                        uart->tx_buffer[uart->tx_buffer_write++] = *ppp_header_ptr;

                        /* Check for wrapping of the write pointer. */
                        uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;
#endif

                    }
        }

        ppp_header_ptr++;
    }

    /* Terminate the packet. */
#if (HDLC_POLLED_TX == NU_TRUE)

    NU_Serial_Putchar(uart, PPP_HDLC_FRAME);

#else

    /* Store the byte. */
    uart->tx_buffer[uart->tx_buffer_write++] = PPP_HDLC_FRAME;

    /* Check for wrapping of the write pointer. */
    uart->tx_buffer_write %= (UINT16) uart->sd_buffer_size;

#endif

#if (HDLC_POLLED_TX == NU_FALSE)

    /* Send a FLAG character to flush out any characters the remote host might
       have received because of line noise and to mark the beginning of this
       PPP HDLC frame. This will also start the TX interrupts which will finish
       sending the TX buffer. */
    NU_Serial_Putchar(uart, PPP_HDLC_FRAME);

#endif

    /* If using SNMP, this will update the MIB */
    MIB2_ifOutOctets_Add(device, buf_ptr->mem_total_data_len + HDLC_FCS_SIZE);

    return (NU_SUCCESS);

} /* HDLC_TX_Packet */

/*************************************************************************
* FUNCTION
*
*     HDLC_RX_HISR_Entry
*
* DESCRIPTION
*
*    This is the entry function for the PPP HISR.  The HISR will check the
*    FCS of the incoming packet. If it passes, an event will be set to
*    notify the upper layer that a packet needs to be processed.
*
* INPUTS
*
*     None
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID HDLC_RX_HISR_Entry(VOID)
{
    NET_BUFFER      *buf_ptr, *work_buf;
    UINT16          bytes_left;
    UINT8           *buffer;
    UINT8           acclen = 0;
    UINT8           pfclen = 0;
    UINT16          hdrlen, paylen;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    NU_SUPERVISOR_MODE();

    /* Get a pointer to the device. */
    dev_ptr = _ppp_rx_queue[_ppp_rx_queue_read]->device;
    link_layer = dev_ptr->dev_link_layer;

    /* Get a pointer to the data. */
    buffer = _ppp_rx_queue[_ppp_rx_queue_read]->buffer;

    /* Store the total size, removing the two bytes of the FCS. */
    bytes_left = (UINT16)(_ppp_rx_queue[_ppp_rx_queue_read]->size - HDLC_FCS_SIZE);

    /* Determine offset if Address & Control compression is used. */
    if (buffer[0] == 0xff)
    {
        acclen += (UINT8)HDLC_MAX_ADDR_CONTROL_SIZE;
        if (buffer[1] != 0x03)
        {
            /* Control field is invalid. Update the PPP MIB. */
            PML_BadControls_Inc(dev_ptr);
        }
    }

    /* Determine offset if Protocol Field compression is used. */
    if (buffer[acclen] & 1)
        pfclen += PPP_PROTOCOL_HEADER_1BYTE;
    else
        pfclen += PPP_PROTOCOL_HEADER_2BYTES;

    hdrlen = (UINT16)(sizeof(DLAYER) - (acclen + pfclen));
    paylen = (UINT16)(NET_PARENT_BUFFER_SIZE - hdrlen);

    /* Check the RX packet length against the requested MRU. */
    if ((link_layer->data_received - hdrlen - HDLC_FCS_SIZE) > link_layer->lcp.options.local.mru)
    {
        PrintErr("Discard - invalid MRU ");

        /* Update statistics. */
        PML_Overruns_Inc(dev_ptr);
        MIB2_ifInErrors_Inc(dev_ptr);

        /* Remove the temporary buffer entry. */
        _ppp_rx_queue[_ppp_rx_queue_read] = NU_NULL;
    }

    /* Check the FCS to make sure this is a valid packet before we
      bother the upper layers with it. */
    else if (HDLC_Compute_RX_FCS (PPP_INIT_FCS16,
        _ppp_rx_queue[_ppp_rx_queue_read]) != PPP_GOOD_FCS16)
    {
        PrintErr("Discard - invalid FCS ");

        /* Update statistics. */
        PML_BadFCSs_Inc(dev_ptr);
        MIB2_ifInErrors_Inc(dev_ptr);

        /* Remove the temporary buffer entry. */
        _ppp_rx_queue[_ppp_rx_queue_read] = NU_NULL;
    }
    else
    {
        /* Allocate a buffer chain to copy this packet into. */
        buf_ptr = MEM_Buffer_Chain_Dequeue (&MEM_Buffer_Freelist,
                ((INT32)(bytes_left + hdrlen)));

        /* Make sure we got some buffers. */
        if (buf_ptr)
        {
            /* Set the total length in the parent buffer. */
            buf_ptr->mem_total_data_len = bytes_left;

            /* Set the data pointer. */
            buf_ptr->data_ptr = (buf_ptr->mem_parent_packet + hdrlen);

            /* Now break this packet into a buffer chain. */

            /* Will it all fit into one buffer? */
            if (bytes_left <= paylen)
            {
                /* Store the number of bytes held by this buffer, this includes the
                   protocol headers. */
                buf_ptr->data_len = buf_ptr->mem_total_data_len;

                /* Copy the data. */
                memcpy  (buf_ptr->data_ptr, buffer, (unsigned int)bytes_left);
            }
            else
            {
                /* Fill the parent buffer in the chain. This one is slightly smaller than
                   the rest in the chain. */
                memcpy (buf_ptr->data_ptr, buffer, (unsigned int)paylen);

                /* Take off the bytes just copied from the total bytes left. */
                bytes_left = bytes_left - paylen;

                /* Store the number of bytes in this buffer. */
                buf_ptr->data_len = paylen;

                /* Bump it the number of bytes just copied. */
                buffer += paylen;

                /* Get a work buffer pointer to the buffer chain */
                work_buf = buf_ptr;

                /* Break the rest up into the multiple buffers in the chain. */
                do
                {
                    /* Move to the next buffer in the chain */
                    work_buf = work_buf->next_buffer;

                    /* If the bytes left will fit into one buffer then copy them over */
                    if (bytes_left <= NET_MAX_BUFFER_SIZE)
                    {
                        /* Copy the rest of the data. */
                        memcpy (work_buf->mem_packet, buffer, (unsigned int)bytes_left);

                        /* Set the data pointer */
                        work_buf->data_ptr = work_buf->mem_packet;

                        /* Store the number of bytes in this buffer. */
                        work_buf->data_len = bytes_left;

                        /* Update the data bytes left to copy. */
                        bytes_left = 0;

                    }
                    else
                    {
                        /* Copy all that will fit into a single buffer */
                        memcpy (work_buf->mem_packet, buffer, NET_MAX_BUFFER_SIZE);

                        /* Update the buffer pointer */
                        buffer += NET_MAX_BUFFER_SIZE;

                        /* Set the data pointer */
                        work_buf->data_ptr = work_buf->mem_packet;

                        /* Store the number of bytes in this buffer. */
                        work_buf->data_len = NET_MAX_BUFFER_SIZE;

                        /* Update the data bytes left to copy. */
                        bytes_left -= (UINT16)NET_MAX_BUFFER_SIZE;

                    }

                } while ( (bytes_left > 0) &&
                          (bytes_left <=
                          (_ppp_rx_queue[_ppp_rx_queue_read]->size - HDLC_FCS_SIZE)) );

            } /* end if it will fit into one buffer */

            /* Remove Address and Control header if it exists. */
            if (acclen)
            {
                buf_ptr->data_ptr += acclen;
                buf_ptr->data_len -= acclen;
                buf_ptr->mem_total_data_len -= acclen;
            }

            /* Set the device that this packet was RX on. */
            buf_ptr->mem_buf_device = _ppp_rx_queue[_ppp_rx_queue_read]->device;

            /* Remove the temporary buffer entry. */
            _ppp_rx_queue[_ppp_rx_queue_read] = NU_NULL;

            /* Move the packet onto the buffer list, where the upper
            layer protocols can find it. */
            MEM_Buffer_Enqueue (&MEM_Buffer_List, buf_ptr);

            /* If using SNMP, this will update the MIB */
            MIB2_ifInNUcastPkts_Inci(buf_ptr->mem_buf_device->dev_index);

            /* Let the upper layer know a good packet is here. */
            NU_Set_Events(&Buffers_Available, (UNSIGNED)2, NU_OR);
        }
        else
        {
            NLOG_Error_Log("Failed to allocate Net buffer.", NERR_SEVERE, __FILE__, __LINE__);

            /* Bump the number of packets discarded. */
            PML_SilentDiscards_Inc(_ppp_rx_queue[_ppp_rx_queue_read]->device);

            /* If using SNMP, this will update the MIB */
            MIB2_ifInDiscards_Inci(_ppp_rx_queue[_ppp_rx_queue_read]->device->dev_index);
        }
    }

    /* Bump the index and make sure the ring buffer loops. */
    _ppp_rx_queue_read++;
    _ppp_rx_queue_read %= (UINT8) HDLC_MAX_HOLDING_PACKETS_PTR;


    NU_USER_MODE();

} /* HDLC_RX_HISR_Entry */



#if (HDLC_POLLED_TX == NU_FALSE)
/*************************************************************************
* FUNCTION
*
*     HDLC_TX_HISR_Entry
*
* DESCRIPTION
*
*     Entry function for transmit HISR. This function is responsible for
*     freeing the buffers used by the transmitted packet. It will also
*     begin transmission of the next packet if there is one ready on the
*     transmit queue.
*
* INPUTS
*
*     None
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID HDLC_TX_HISR_Entry(VOID)
{
    DV_DEVICE_ENTRY *dev_ptr;
    INT old_val;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Get a pointer to the device that activated this HISR. */
    dev_ptr = _ppp_tx_dev_ptr_queue[_ppp_tx_dev_ptr_queue_read];

    /* Make sure there was a device structure there. */
    if (dev_ptr)
    {
        /* Disable interrupts. */
        old_val = NU_Control_Interrupts (NU_DISABLE_INTERRUPTS);

        /* Clear this entry. */
        _ppp_tx_dev_ptr_queue[_ppp_tx_dev_ptr_queue_read] = NU_NULL;

        /* If there is a buffer on the TX queue then remove it. It was just
           transmitted. */
        if (dev_ptr->dev_transq.head != NU_NULL)
        {
            /* Give the buffers held by this packet back to the stack. */
            if (dev_ptr->dev_transq.head->mem_dlist)
                DEV_Recover_TX_Buffers (dev_ptr);
            else
            {
                NLOG_Error_Log("Net buffer transmitted without deallocation destination.", NERR_SEVERE, __FILE__, __LINE__);

                /* Error. Give it to the freelist */
                MEM_Multiple_Buffer_Chain_Free(dev_ptr->dev_transq.head);

                /* Remove buffer from transq list. */
                MEM_Buffer_Dequeue(&dev_ptr->dev_transq);

                /* Decrement the number of buffers in the transq list. */
                dev_ptr->dev_transq_length--;
            }

            /* If using SNMP, this will update the MIB */
            MIB2_ifOutQLen_Dec(dev_ptr->dev_index);

            /* Check to see if there is another packet that needs to be
               sent. */
            if (dev_ptr->dev_transq.head)
            {
                /* Restore the interrupts */
                NU_Control_Interrupts (old_val);

                /* Send the next packet in the tx queue. */
                dev_ptr->dev_start(dev_ptr, dev_ptr->dev_transq.head);
            }
        }

        /* Restore the interrupts */
        NU_Control_Interrupts (old_val);
    }

    /* Bump the read index. */
    _ppp_tx_dev_ptr_queue_read++;

    /* Check for wrap of the ring buffer. */
    _ppp_tx_dev_ptr_queue_read %= (UINT8) HDLC_MAX_TX_QUEUE_PTRS;

    NU_USER_MODE();

} /* HDLC_TX_HISR_Entry */
#endif


/*************************************************************************
* FUNCTION
*
*     HDLC_Compute_TX_FCS
*
* DESCRIPTION
*
*    This function will compute the frame check sequence for the frame
*    contained at the address pointed to by frame_ptr. The computation of
*    the FCS spans over multiple buffers in the chain containing the
*    packet.
*
* INPUTS
*
*    UINT16             fcs             The initial FCS to start with
*    NET_BUFFER         *buf_ptr        Pointer to the buffer that
*                                        holds the packet to be sent.
*
* OUTPUTS
*
*    UINT16                             The computed FCS
*
*************************************************************************/
UINT16 HDLC_Compute_TX_FCS(UINT16 fcs, NET_BUFFER *buf_ptr)
{
    NET_BUFFER          *tmp_buf_ptr = buf_ptr;
    INT32               len;
    UINT8       HUGE    *frame_ptr;

    /* Get the length of the buffer and a pointer to the data. */
    len         = tmp_buf_ptr->data_len;
    frame_ptr   = tmp_buf_ptr->data_ptr;

    while (tmp_buf_ptr)
    {
        /* Compute the FCS over the bytes in this buffer. */
        while (len-- > 0)
            fcs = (UINT16) ((fcs >> 8) ^ fcstab[(fcs ^ *frame_ptr++) & 0xff]);

        tmp_buf_ptr = tmp_buf_ptr->next_buffer;

        if (tmp_buf_ptr)
        {
            /* Get the length of the buffer and a pointer to the data. */
            len         = tmp_buf_ptr->data_len;
            frame_ptr   = tmp_buf_ptr->data_ptr;
        }

    }

    return (fcs);
} /* HDLC_Compute_TX_FCS */



/*************************************************************************
* FUNCTION
*
*     HDLC_Compute_RX_FCS
*
* DESCRIPTION
*
*     This function will compute the frame check sequence for the frame
*     contained at the address pointed to by frame_ptr. The computation
*     does not span buffers. It is done over a contiguous area of memory
*     only.
*
* INPUTS
*
*     UINT16            fcs             The initial FCS to start with
*     PPP_TEMP_BUFFER   *buf_ptr        Pointer to the packet to
*                                        compute the FCS over.
*
* OUTPUTS
*
*     UINT16                            The computed FCS
*
*************************************************************************/
UINT16 HDLC_Compute_RX_FCS(UINT16 fcs, HDLC_TEMP_BUFFER *buf_ptr)
{
    UINT32 len;
    UINT8  HUGE *frame_ptr;

    /* Get the length of the buffer and a pointer to the data. */
    len         = buf_ptr->size;
    frame_ptr   = buf_ptr->buffer;

    /* Compute the FCS. */
    while (len--)
        fcs = (UINT16) ((fcs >> 8) ^ fcstab[(fcs ^ *frame_ptr++) & 0xff]);

    return (fcs);

} /* HDLC_Compute_RX_FCS */
