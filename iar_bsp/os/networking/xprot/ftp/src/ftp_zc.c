/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       ftp_zc.c                                       
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package
*
*   DESCRIPTION
*
*       This file contains those functions necessary to read data from
*       and write data to Zero Copy buffers.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FTP_ZC_Write_Data
*       FTP_ZC_Read_Data
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/fc_defs.h"
#include "storage/nu_storage.h"

#ifdef NET_5_1

/*************************************************************************
*
*   FUNCTION
*
*       FTP_ZC_Write_Data
*
*   DESCRIPTION
*
*       This routine writes a buffer chain of data to the specified
*       file.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the head of the chain of
*                               buffers from which to get the data.
*       fd                      The file descriptor to which to write
*                               the data.
*
*   OUTPUTS
*
*       Total number of bytes written to the file.
*
*************************************************************************/
INT FTP_ZC_Write_Data(NET_BUFFER *buf_ptr, INT fd)
{
    NET_BUFFER  *seg_ptr;
    CHAR        *data_ptr;
    UINT16      bytes_avail;
    UINT32      temp32;
    INT         status = -1;
    SIGNED      totalBytes = 0;

    /* Get a pointer to the buffer chain of data */
    seg_ptr = buf_ptr;

    /* Traverse the chain of buffers, writing the data to the file. */
    while (seg_ptr)
    {
        /* Get the first segment in the buffer chain */
        data_ptr = NU_ZC_SEGMENT_DATA(seg_ptr);

        /* Determine how many bytes are in this segment. */
        temp32 = NU_ZC_SEGMENT_BYTES_AVAIL(seg_ptr);

        /* temp32 is used to remove compiler warnings. */
        bytes_avail = (UINT16)temp32;

        /* Write the data to the file */
        if (bytes_avail > 0)
        {
            status = NU_Write(fd, data_ptr, bytes_avail);

            /* If the data could not be written to the file, send an
             * error to the client and stop trying to write to the file.
             */
            if (status <= 0)
                break;

            totalBytes += bytes_avail;
        }

        /* Get a pointer to the next segment */
        seg_ptr = NU_ZC_SEGMENT_NEXT(seg_ptr);
    }

    /* If an error did not occur, return the total number of bytes
     * copied from the buffer.
     */
    if (status > 0)
        status = (INT)totalBytes;

    return (status);

} /* FTP_ZC_Write_Data */

/*************************************************************************
*
*   FUNCTION
*
*       FTP_ZC_Read_Data
*
*   DESCRIPTION
*
*       This routine allocates a NET buffer chain and reads the specified
*       number of bytes (or as much as will fit in the chain) from a file
*       into the chain of buffers.
*
*   INPUTS
*
*       **buf_ptr               A pointer to a pointer to the head of
*                               the NET buffer chain.
*       bytes_to_copy           The maximum number of bytes to be copied
*                               into the buffer chain.
*       socketd                 The socket associated with the
*                               connection.
*       fd                      The file descriptor from which to copy
*                               the data.
*
*   OUTPUTS
*
*       Total number of bytes written to the buffer chain.
*
*************************************************************************/
INT FTP_ZC_Read_Data(NET_BUFFER **buf_ptr, UINT32 bytes_to_copy,
                     INT socketd, INT fd)
{
    NET_BUFFER  *seg_ptr;
    CHAR        *data_ptr;
    INT32       bytes_left, bytes_avail;
    INT         bytesReceived = 0;

    /* Get a buffer for the data and the total number
     * of bytes that will fit in this buffer.
     */
    if (bytes_to_copy > FTP_MAX_ZC_BUFFER_SIZE)
        bytes_to_copy = FTP_MAX_ZC_BUFFER_SIZE;

    bytes_avail = NU_ZC_Allocate_Buffer(buf_ptr, (UINT16)bytes_to_copy,
                                        socketd);

    if (bytes_avail > 0)
    {
        /* Get a pointer to the head of the buffer chain */
        seg_ptr = *buf_ptr;

        /* Fill the buffer chain with the data, one segment
         * at a time.
         */
        while (seg_ptr)
        {
            /* Get a pointer to the data */
            data_ptr = NU_ZC_SEGMENT_DATA(seg_ptr);

            /* Determine the number of bytes that will fit in
             * this segment.
             */
            bytes_left = (INT32)NU_ZC_SEGMENT_BYTES_LEFT(*buf_ptr, seg_ptr,
                                                         socketd);

            /* Only write as many bytes as indicated available by
             * the buffer allocation call.  Be careful not to exceed
             * the MSS.
             */
            if (bytes_left > bytes_avail)
                bytes_left = bytes_avail;

            /* Copy as much data from the file as will fit in
             * the buffer.
             */
            bytes_left = NU_Read(fd, data_ptr, bytes_left);

            /* If a file error occurred or there is not more data to
             * copy from the file.  Break out of the loop.
             */
            if (bytes_left <= 0)
            {
                /* If an error occurred, set bytesReceived to the error
                 * so the data will not be transmitted and an error code
                 * will be transmitted.
                 */
                if (bytes_left < 0)
                {
                    bytesReceived = (INT)bytes_left;
                    NU_ZC_Deallocate_Buffer(*buf_ptr);
                }

                /* If no data was copied into the buffer, deallocate the
                 * buffers.
                 */
                else if (bytesReceived == 0)
                    NU_ZC_Deallocate_Buffer(*buf_ptr);

                break;
            }

            /* Increment the number of bytes written to the buffer
             * chain.
             */
            bytesReceived += (INT)bytes_left;

            /* Decrement the number of bytes available in the buffer
             * chain.
             */
            bytes_avail -= bytes_left;

            /* Get a pointer to the next segment */
            seg_ptr = NU_ZC_SEGMENT_NEXT(seg_ptr);
        }
    }

    /* Return the error */
    else
        bytesReceived = (INT)bytes_avail;

    return (bytesReceived);

} /* FTP_ZC_Read_Data */

#endif
