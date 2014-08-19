/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*
* FILE NAME                                                           
*    
*       wsn.c                                       
*                                                                      
* COMPONENT                                                            
*                                                                      
*       Nucleus WebServ    
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*       This file is used for all accesses to the network layer.
*       This adds the ability to access SSL when necessary.
*                                                                      
* DATA STRUCTURES                                                      
*                                                                      
*                                                                      
* FUNCTIONS                                                            
*                                                                      
*       WSN_Read_Net                 Read data from network once a        
*                                     connection has been made.            
*       WSN_Write_To_Net             Writes data out to the Network.      
*       WSN_Flush_Net                Flushes the output buffer.           
*       WSN_Add_Data                 Adds data to the out buffer
*       WSN_Send                     Sends data across net
*                                                                      
* DEPENDENCIES                                                         
*
*       nu_websr.h
*                                                                      
************************************************************************/

#include "networking/nu_websr.h"

STATIC STATUS  WSN_Add_Data(WS_REQUEST *req, CHAR *buf, INT32 size);

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSN_Write_To_Net                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function to output data to the network.                          
*                                                                      
* INPUTS                                                               
*                                                                      
*       req                            Pointer to Request structure that
*                                      holds all information pertaining 
*                                      to the HTTP request.             
*       buf                            The buffer that holds the        
*                                      information to be written out on 
*                                      the network.                     
*       sz                             The size of the buffer that is to
*                                      be written out.                  
*       mode                           The action for the function to take.   
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     status                           Returns NU_SUCCESS on successful
*                                      completion.
*                                                                      
************************************************************************/
STATUS WSN_Write_To_Net(WS_REQUEST * req, CHAR HUGE * buf, UINT32 sz, STATUS mode)
{
    CHAR HUGE   *tmpptr;
    STATUS      status = NU_SUCCESS;
    
    switch(mode)
    {
    case WS_FILETRNSFR:
    case WS_REDIRECTION:

        /*  Process sending data that either has a content
         *  length or no entity body.
         */
        status = (STATUS)WSN_Send(req, buf, sz);
        break;
        
    case WS_PLUGIN_DATA:
        
        /* Determine if a header is needed */
        if(!req->ws_rdata.ws_out_header[0] && !req->ws_rdata.ws_no_head_flag)
        {
            /* Set the default header for the response */
            HTTP_Response_Header(req, WS_PROTO_OK);
            HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, WS_TYPE_TXT_HTML);
        }
        
        /* Add the data to the outgoing buffer */
        status = WSN_Add_Data(req, (CHAR*)buf, (INT32)sz);

        /* There is not enough memory to finish this request */
        if(status != NU_SUCCESS)
        {
            /* Clear the rdata structure */
            WS_Clear_Rdata(req);
        }
        
        break;
        
    case WS_PLUGIN_SEND:
        /* Get ready to send the data */

        tmpptr = (CHAR HUGE*)(req->ws_rdata.ws_out_data);

        /* If a header is necessary, finish it here with content length */
        if(!req->ws_rdata.ws_no_head_flag)
        {
            /* Insert the Content-Length Field */
            HTTP_Header_Num_Insert(req, WS_CONTENT_LENGTH, (INT32)req->ws_rdata.ws_out_data_sz);

            /* Terminate the header by adding \r\n */
            strcat(req->ws_rdata.ws_out_header, WS_CRLF);

            /* Calculate the Header Size */
            sz = strlen(req->ws_rdata.ws_out_header);
            
            /* Send the header of the response */
            status = (STATUS)WSN_Send(req, req->ws_rdata.ws_out_header, sz);
        }

        /* Send the data */
        if(status == NU_SUCCESS)
            status = (STATUS)WSN_Send(req, tmpptr, (UINT32)req->ws_rdata.ws_out_data_sz);
        
        /* Clear the rdata structure */
        WS_Clear_Rdata(req);

        break;
    }
    
    return status;
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSN_Add_Data                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Adds data to the outgoing buffer.  If the buffer is not large
*     enough, more memory will be allocated to handle the data.   
*                                                                      
* INPUTS                                                               
*                                                                      
*       req                         Pointer to Request structure that
*                                   holds all information pertaining 
*                                   to the HTTP request.             
*       buf                         Data to be added to buffer    
*       size                        Amount of data to be added
*                                                                      
* OUTPUTS                                                              
*                                                                      
*       status                      NU_SUCCESS on successful completion                                    
*                                                                      
************************************************************************/
STATIC STATUS WSN_Add_Data(WS_REQUEST *req, CHAR *buf, INT32 size)
{
    STATUS      status;
    CHAR HUGE   *tmpptr = NU_NULL;
    CHAR        *saveptr;
    INT32       buf_size;

    if(size >= (INT32)req->ws_rdata.ws_out_data_free)
    {
        /*  Get the size of the new buffer.  Set a minimum size
         *  to help prevent too many allocation.
         */
        if(size > WS_OUT_BUFSZ)
            buf_size = size + req->ws_rdata.ws_out_data_sz;
        else
            buf_size = WS_OUT_BUFSZ + req->ws_rdata.ws_out_data_sz;
        
        /*  Allocate a new buffer to store the data in */
        status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID **)&tmpptr,
                                     (UNSIGNED)buf_size, TICKS_PER_SECOND);

        if(status != NU_SUCCESS)
            return (status);

        /*  If there is data already in the buffer, transfer it
         *  into the new buffer
         */
        if(req->ws_rdata.ws_out_data)
        {
            saveptr = (CHAR*)req->ws_rdata.ws_out_data;
            
            /*  Copy the data into the temporary buffer */
            WS_Mem_Cpy((CHAR*)tmpptr, req->ws_rdata.ws_out_data, 
                        (UINT32)req->ws_rdata.ws_out_data_sz);
            
            req->ws_rdata.ws_out_data = (CHAR*)tmpptr;

            if(NU_Deallocate_Memory(saveptr) != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }
        else
            req->ws_rdata.ws_out_data = (CHAR*)tmpptr;
        
        req->ws_rdata.ws_out_data_free = buf_size - req->ws_rdata.ws_out_data_sz;
    }
        
    /*  Increment Past the current data */
    tmpptr = (CHAR HUGE*)TLS_Normalize_Ptr(req->ws_rdata.ws_out_data);
    tmpptr = tmpptr + req->ws_rdata.ws_out_data_sz;
    
    /* Copy the new data into the buffer */
    WS_Mem_Cpy((CHAR*)tmpptr, buf, (UINT32)size);
    
    /* Calculate the new sizes */
    req->ws_rdata.ws_out_data_sz += size;
    req->ws_rdata.ws_out_data_free -= size;

    return (NU_SUCCESS);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSN_Flush_Net                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function that handles buffered output flush.                     
*                                                                      
* INPUTS                                                               
*                                                                      
*     req                        Pointer to Request structure that    
*                                 holds all information pertaining to  
*                                 the HTTP request.                    
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/

VOID WSN_Flush_Net(WS_REQUEST * req)
{
    if(req->ws_rdata.ws_out_data)
        if(WSN_Write_To_Net(req, 0, 0, WS_PLUGIN_SEND) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        
    /* Clear the rdata structure */
    WS_Clear_Rdata(req);

    if(req->ws_rdata.ws_in_data_flag & WS_MEM_ALLOC)
    {
        if(NU_Deallocate_Memory(req->ws_rdata.ws_in_data) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    }

    req->ws_rdata.ws_in_data = NU_NULL;
    req->ws_rdata.ws_in_data_sz = 0;
    req->ws_rdata.ws_in_header_sz = 0;
    memset( req->ws_rdata.ws_in_header, 0x00, sizeof(req->ws_rdata.ws_in_header) );
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSN_Read_Net                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function to read data after a connection has been made from the  
*     network.                                                                                                                              
*                                                                      
* INPUTS                                                               
*                                                                      
*       req                        Pointer to Request structure that    
*                                  holds all information pertaining to  
*                                  the HTTP request.                    
*       buf                        The buffer to place received data.    
*       sz                         The size of the buffer.    
*       timer                      Length of time to wait for data to
*                                  come through this socket.                     
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     bytes_received              The total bytes received from an     
*                                  NU_recv call.                        
*                                                                      
************************************************************************/

INT32 WSN_Read_Net(WS_REQUEST * req, CHAR * buf, UINT16 sz, UNSIGNED timer)
{
    INT32       bytes_recieved = 0;
    STATUS      status = NU_SUCCESS;
    FD_SET      readfs;

#if INCLUDE_SSL
    /* If using SSL, must check if any bytes pending in SSL buffers
     * before checking on socket for pending data. 
     */
    if (req->ws_ssl && timer)
    {
        if (SSL_pending((SSL*)req->ws_ssl))
            /* Don't wait on socket for data */
            timer = 0;
    }
#endif
 
    if(timer)
    {
        /* Initialize the bitmap */
        NU_FD_Init(&readfs);
        NU_FD_Set(req->ws_sd, &readfs);

        /* Is there any data on the socket? */
        status = NU_Select((req->ws_sd + 1), &readfs, NU_NULL, NU_NULL, timer);
    }

    if(status == NU_SUCCESS)
    {
#if INCLUDE_SSL
        if(req->ws_ssl)
            bytes_recieved = SSL_read((SSL*)req->ws_ssl, buf, sz);
        else
#endif
            bytes_recieved = NU_Recv(req->ws_sd, buf, sz, NU_NO_SUSPEND);
    }
    
    return(bytes_recieved);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSN_Send                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function to write data to the network.                                                                                                                              
*                                                                      
* INPUTS                                                               
*                                                                      
*       req                        Pointer to Request structure that    
*                                  holds all information pertaining to  
*                                  the HTTP request.                    
*       buffer                     The buffer holding outgoing data.  
*       size                       The size of the buffer that is to be 
*                                  written out.                         
*                                                                      
* OUTPUTS                                                              
*                                                                      
*       NU_SUCCESS                 On successful completion     
*                                                                      
************************************************************************/

INT32 WSN_Send(WS_REQUEST *req, CHAR HUGE *buffer, UINT32 size)
{
    INT32       bytes_sent = 0;
    CHAR HUGE   *tmpptr;

    tmpptr = buffer;
    
    /* If there is more data than the MAX_SEND_SIZE will allow, send
     * data in chunks.
     */
    while((WS_MAX_SEND_SIZE < size) && (bytes_sent >= 0))
    {
#if INCLUDE_SSL
        if(req->ws_ssl)
            bytes_sent = SSL_write((SSL*)req->ws_ssl, (CHAR*)tmpptr, WS_MAX_SEND_SIZE);
        else
#endif
            bytes_sent = NU_Send(req->ws_sd, (CHAR*)tmpptr, WS_MAX_SEND_SIZE, NU_NULL);

        size -= bytes_sent;
        tmpptr += bytes_sent;
    }
    
    /* If there is any data left, send it */
    if(size && (bytes_sent >= 0))
    {
#if INCLUDE_SSL
        if(req->ws_ssl)
            bytes_sent = SSL_write((SSL*)req->ws_ssl, (CHAR*)tmpptr, (int)size);
        else
#endif
            bytes_sent = NU_Send(req->ws_sd, (CHAR*)tmpptr, (UINT16)size, NU_NULL);
    }

    if(bytes_sent < 0)
        return(bytes_sent);
    return(NU_SUCCESS);
}

