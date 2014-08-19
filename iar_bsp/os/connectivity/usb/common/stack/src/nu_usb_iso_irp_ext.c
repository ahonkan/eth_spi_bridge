/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_iso_irp_ext.c
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*        This file contains implementation of NU_USB_ISO_IRP services. ISO IRP is
*        Input/Output Request Packet, which is used by stack to communicate
*        information to hardware.
*
*
* DATA STRUCTURES
*       None.
*
* FUNCTIONS
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_ISO_IRP_EXT_C
#define	USB_ISO_IRP_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Create
*
* DESCRIPTION
*       This function Creates an instance of ISO IRP with initial values.
*
* INPUTS
*       cb                      pointer to ISO IRP control block.
*       num_transactions        Number of ISO transactions specified with this ISO IRP.
*       data                    pointer to a location which contains
*                               transmitted or received data.
*       lengths                 An array containing 'num_transactions' elements.
*                               Element i indicates the length of iso data in
*                               the buffer 'data' for the iso transaction i.
*       callback                pointer to a function invoked when an ISO IRP
*                               is completed.
*       context                 pointer to a location to pass context
*                               information back and forth between ISO IRP
*                               submitter and ISO IRP server.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Create(NU_USB_ISO_IRP *cb,
                             UINT16 num_transactions,
                             UINT8 **buffer_array,
                             UINT16 *lengths,
                             UINT16 *actual_lengths,
                             NU_USB_IRP_CALLBACK callback,
                             VOID *context)
{
    STATUS status;

    NU_USB_PTRCHK(cb);

    /* Clear the control block */
    memset(cb, 0, sizeof(NU_USB_ISO_IRP));

    /* Populate the control block. */
    cb->actual_num_transactions = 0;
    cb->num_transactions = num_transactions;
    cb->lengths = lengths;
    cb->actual_lengths = actual_lengths;
    cb->buffer_array = buffer_array;

    /* Call the base function. */
    /* returning status of function. */
    status = NU_USB_IRP_Create((NU_USB_IRP *) cb,
                                      0,
                                      NU_NULL,
                                      NU_FALSE,
                                      NU_FALSE,
                                      callback,
                                      context,
                                      0);

    return (status);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Delete
*
* DESCRIPTION
*       This function deletes the instance of an ISO IRP. This doesn't free
*       the memory associated with the ISO IRP control block.
*
* INPUTS
*       cb          pointer to ISO IRP control block
*
* OUTPUTS
*       NU_SUCCESS  indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Delete(NU_USB_ISO_IRP *cb)
{
    STATUS status;

    NU_USB_PTRCHK(cb);

    status = NU_USB_IRP_Delete((NU_USB_IRP *) cb);

    return (status);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Get_Num_Transactions
*
* DESCRIPTION
*       This function returns the number of ISO transactions set for this
*       ISO IRP in num_transactions_out
*
* INPUTS
*       cb                      pointer to ISO IRP control block
*       num_transactions_out    pointer to a variable to hold the number of
*                               ISO transactions associated with this ISO IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Get_Num_Transactions(NU_USB_ISO_IRP *cb,
                                           UINT16 *num_transactions_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(num_transactions_out);

    *num_transactions_out = cb->num_transactions;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_ISO_IRP_Set_Num_Transactions
*
* DESCRIPTION
*
*       This function sets the number of transactions associated with an
*       ISO IRP.
*
* INPUTS
*       cb                  pointer to ISO IRP control block.
*       num_transactions    Number of ISO transaction for this IRP.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Set_Num_Transactions(NU_USB_ISO_IRP *cb,
                                           UINT16 num_transactions)
{
    NU_USB_PTRCHK(cb);

    cb->num_transactions = num_transactions;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Get_Buffer_Array
*
* DESCRIPTION
*       This function returns the Data pointer associated
*       with ISO IRP in data_out
*
* INPUTS
*       cb               pointer to ISO IRP control block
*       buffer_array_out Pointer to a memory location to hold the pointer
*                        to data of associated ISO IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Get_Buffer_Array(NU_USB_ISO_IRP *cb,
                                       UINT8 ***buffer_array_out)
{
    NU_USB_PTRCHK(cb);

    *buffer_array_out = cb->buffer_array;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Set_Buffer_Array
*
* DESCRIPTION
*       This function sets the data pointer of data transfer associated with
*       an  ISO IRP.
*
* INPUTS
*       cb           pointer to ISO IRP control block.
*       buffer_array pointer to a location to store data.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Set_Buffer_Array(NU_USB_ISO_IRP *cb,
                                       UINT8 **buffer_array)
{
    STATUS status;

    NU_USB_PTRCHK(cb);

    cb->buffer_array = buffer_array;
    status = NU_USB_IRP_Set_Data((NU_USB_IRP *) cb, NU_NULL);

    return (status);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Get_Actual_Num_Transactions
*
* DESCRIPTION
*       This function returns the actual number of transactions transferred
*       by ISO IRP in actual_num_transactions_out
*
* INPUTS
*       cb          pointer to ISO IRP control block
*       actual_num_transactions_out
*                   pointer to a variable to hold the returned number
*                   of transactions
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Get_Actual_Num_Transactions(NU_USB_ISO_IRP *cb,
                                                  UINT16 *actual_num_transactions_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(actual_num_transactions_out);

    *actual_num_transactions_out = cb->actual_num_transactions;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Set_Actual_Num_Transactions
*
* DESCRIPTION
*       This function sets the actual length of data transfer associated with an
*       ISO IRP.
*
* INPUTS
*       cb          pointer to ISO IRP control block.
*       length      actual length to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Set_Actual_Num_Transactions(NU_USB_ISO_IRP *cb,
                                                  UINT16 actual_num_transactions)
{
    NU_USB_PTRCHK(cb);

    cb->actual_num_transactions = actual_num_transactions;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Get_Lengths
*
* DESCRIPTION
*       This function returns the lengths pointer associated
*       with ISO IRP in lengths_out.
*       'Lengths' is an array containing 'num_transactions' elements.
*       Element i indicates the length of iso data in the buffer 'data' for
*       the iso transaction i.
*
* INPUTS
*       cb          pointer to ISO IRP control block
*       lengths_out Pointer to a memory location to hold the pointer
*                   to lengths of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Get_Lengths(NU_USB_ISO_IRP *cb, UINT16 **lengths_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(lengths_out);

    *lengths_out = cb->lengths;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Set_Lengths
*
* DESCRIPTION
*       This function sets the lengths pointer of data transfer associated with
*       an  ISO IRP.
*       'Lengths' is an array containing 'num_transactions' elements.
*       Element i indicates the length of iso data in the buffer 'data' for
*       the iso transaction i.

*
* INPUTS
*       cb          pointer to ISO IRP control block.
*       lengths     pointer to a location containing data lengths for individual
*                   ISO transactions.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Set_Lengths(NU_USB_ISO_IRP *cb, UINT16 *lengths)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(lengths);

    cb->lengths = lengths;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Get_Actual_Lengths
*
* DESCRIPTION
*       This function returns the lengths pointer associated
*       with ISO IRP in actual lengths.
*       'Actual_Lengths' is an array containing 'actual_num_transactions'
*       elements. Element i indicates the length of iso data in the buffer
*       'data' for the iso transaction i.
*
* INPUTS
*       cb          pointer to ISO IRP control block
*       lengths_in  Pointer to a memory location to hold the pointer
*                   to actual lengths of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Get_Actual_Lengths(NU_USB_ISO_IRP *cb,
                                         UINT16 **lengths_in)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(lengths_in);

    *lengths_in = cb->actual_lengths;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_ISO_IRP_Set_Actual_Lengths
*
* DESCRIPTION
*       This function sets the actual lengths pointer of data transfer
*       associated with an ISO IRP.
*       'Actual_Lengths' is an array containing 'actual_num_transactions'
*       elements. Element i indicates the length of iso data in the buffer
*       'data' for the iso transaction i.

*
* INPUTS
*       cb               pointer to ISO IRP control block.
*       actual_lengths   pointer to a location containing data lengths
*                        for individual ISO transactions.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_ISO_IRP_Set_Actual_Lengths(NU_USB_ISO_IRP *cb,
                                         UINT16 *actual_lengths)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(actual_lengths);

    cb->actual_lengths = actual_lengths;

    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_ISO_IRP_EXT_C */
/*************************** end of file ********************************/

