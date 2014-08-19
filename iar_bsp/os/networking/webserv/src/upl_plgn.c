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
*   FILE NAME                                                           
*                                                                       
*       upl_plgn.c                                                
*                                                                       
*   COMPONENT                                                             
*       
*       Nucleus WebServ                                                                    
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This file contains all files related to the upload plugin. To    
*       exclude this feature see INCLUDE_UPLOAD_PLGN defined in ws_cfg.h.      
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       UPL_File_Upload             Plugin to upload a file into the     
*                                   system.                              
*       UPL_Save_Upload_File        Saves an uploaded file to memory.    
*       UPL_Save_File               Save file in memory.  
*       UPL_Compress                Compresses file to be saved in
*                                   file system
*                                                                       
*    DEPENDENCIES                                                          
*                                                                       
*       nu_websr.h
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"

#if INCLUDE_UPLOAD_PLGN

STATUS         UPL_Save_File(WS_REQUEST * req, CHAR *fname, CHAR *filemem, INT32 length);
STATIC VOID    UPL_Save_Upload_File(WS_REQUEST *req, CHAR * fname,CHAR *filemem, INT32 length);

#if INCLUDE_COMPRESSION
STATIC INT32 UPL_Compress(WS_REQUEST *req, CHAR *fname, CHAR *filemem, INT32 length);
#endif /* INCLUDE_COMPRESSION */

extern WS_FS_FILE  *HTTP_Fs_File;

#if !INCLUDE_FILE_SYSTEM
/*  Semaphore to access the in-memory file system. */
extern NU_SEMAPHORE WS_FS_Access;
#endif

/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UPL_File_Upload                                                      
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This routine uploads a file to the local file system             
*       This feature is implememted as a plugin. To exclude             
*       this feature see INCLUDE_UPLOAD_PLGN defined in ws_cfg.h .             
*                                                                       
*   INPUTS                                                                
*                                                                       
*       req                 Pointer to request structure
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       WS_REQ_PROCEED      On successful completion of this plugin                                                    
*                                                                       
*************************************************************************/
INT UPL_File_Upload(WS_REQUEST * req)
{
    UINT32      content_length;
    INT32       bytes;
    CHAR        *fstart;
    CHAR HUGE   *fend;
    CHAR        fname[WS_URI_LEN];
    CHAR        bound[64];
    CHAR        *section_start;
    CHAR HUGE   *section_end;
    CHAR HUGE   *ptr;
    CHAR HUGE   *ptr_end;
    INT32       fsize;
    STATUS      status;    
    INT         temp;
    
#if NU_WEBSERV_DEBUG
    printf("UPL_File_Upload()\n");
#endif

    section_start = (CHAR*)req->ws_rdata.ws_in_header;
    section_end = (CHAR HUGE*)&req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz];

    /*  Search for the Content-Length header */
    ptr = WS_Find_Token_by_Case(WS_CONTENT_LENGTH, section_start, 
                                (CHAR*)section_end, WS_TOKEN_CASE_INSENS);
    
    /* Get the value of the content length header */
    ptr += sizeof(WS_CONTENT_LENGTH) - 1;
    content_length = NU_ATOL((CHAR*)ptr);

    /* Determine if there is more data to receive */
    if(content_length > req->ws_rdata.ws_in_data_sz)
    {
        /* Get the rest of the file */
        status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID**)&ptr,
                                    (UNSIGNED)content_length, NU_NO_SUSPEND);
        if(status != NU_SUCCESS)
        {
            content_length -= req->ws_rdata.ws_in_data_sz;

            /* Receive the entire file before sending error message.  
             * If the entire file is not received, the browser and server
             * get out of synch and confusion sets in
             */
            while(content_length > 0)
            {
                bytes = WSN_Read_Net(req, (CHAR*)req->ws_rdata.ws_in_data, 
                                     (UINT16)req->ws_rdata.ws_in_data_sz, NU_NO_SUSPEND);
                if(bytes <= 0)
                    return(WS_REQ_ABORTED);

                content_length -= bytes;
            }
            
            /* Send the error message */
            HTTP_Send_Status_Message(req, WS_PROTO_OK, "File Upload Error-no memory");
            return(WS_REQ_PROCEED);
        }

        /* Copy the current data into new memory block */
        WS_Mem_Cpy(ptr, req->ws_rdata.ws_in_data, req->ws_rdata.ws_in_data_sz);

        /* Deallocate old memory block if necessary */
        if(req->ws_rdata.ws_in_data_flag & WS_MEM_ALLOC)
            if(NU_Deallocate_Memory(req->ws_rdata.ws_in_data) != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

        /* Set the allocation flag*/
        req->ws_rdata.ws_in_data_flag |= WS_MEM_ALLOC;

        /* Set up the pointers to the data */
        req->ws_rdata.ws_in_data = (CHAR*)ptr;
        ptr += req->ws_rdata.ws_in_data_sz;

        /* Calculate the amount of data that is left to get */
        content_length -= req->ws_rdata.ws_in_data_sz;

        /* Set the req variable to the size of the data */
        req->ws_rdata.ws_in_data_sz += content_length;

        /* Receive the rest of the file */
        while(content_length > WS_MAX_RECV_BYTES)
        {
            bytes = WSN_Read_Net(req, (CHAR*)ptr, WS_MAX_RECV_BYTES, NU_NO_SUSPEND);
            if(bytes < 0)
            {
                /* Handle error */
                return(WS_REQ_ABORTED);
            }
            
            ptr += bytes;
            content_length -= bytes;
        }

        while(content_length > 0)
        {
            bytes = WSN_Read_Net(req, (CHAR*)ptr, (UINT16)content_length, NU_NO_SUSPEND);
            if(bytes < 0)
            {
                /* Handle error */
                return(WS_REQ_ABORTED);
            }
            
            content_length -= bytes;
            ptr += bytes;
        }
    }

    /* Parse the MIME data */

    /*  Search for the boundary= tag */
    ptr = WS_Find_Token("boundary=", section_start, (CHAR*)section_end);
    if(ptr)
    {
        /*  Increment Pointer to the Boundary Delimeter */
        ptr += 9;            
        ptr_end = WS_Find_Token(WS_CRLF, ptr, (CHAR*)section_end);
        
        /* Get a copy of the boundry */
        NU_BLOCK_COPY(bound, (CHAR*)ptr, (unsigned int)(ptr_end - (CHAR*)ptr));
        temp = (INT)(ptr_end - ptr);
        bound[temp] = NU_NULL;

        /* Set up some pointers to point to the search area */
        section_start = (CHAR*)req->ws_rdata.ws_in_data;
        section_end = req->ws_rdata.ws_in_data + req->ws_rdata.ws_in_data_sz;

        ptr = WS_Find_Token(WS_FILE_NAME_TAG, section_start, (CHAR*)section_end);
        if(ptr)
        {
            /*  Increment Pointer to Filename  */
            ptr += sizeof(WS_FILE_NAME_TAG) + WS_CRLFCRLF_LEN;
            ptr_end = WS_Find_Token(WS_CRLF, ptr, (CHAR*)section_end);

            /*  Get the Filename */
            NU_BLOCK_COPY(fname, ptr, (unsigned int)(ptr_end - ptr));
            temp = (INT)(ptr_end - ptr);
            fname[temp] = NU_NULL;

            if(strlen(fname) > WS_URI_LEN)
            {
                /* Send the error message */
                HTTP_Send_Status_Message(req, WS_PROTO_OK, "URI too long");
            }
            else
            {            
                /*  Look for the CRLFCRLF delimeter to point to the start
                 *  of actual packet data.
                 */
            
                ptr = WS_Find_Token(WS_CRLFCRLF, ptr_end, (CHAR*)section_end);
            
                /* start of the upload data */
                fstart = (CHAR*)(ptr + WS_CRLFCRLF_LEN);
            
                ptr = WS_Find_Token(bound, fstart, (CHAR*)section_end);
                if(ptr)
                {
                    /* Get a pointer to the end of the file */
                    fend = (CHAR*)(ptr - WS_CRLFCRLF_LEN);
                
                    /*  Set the number of buffer bytes left in the last packet. */    
                    fsize = fend - fstart;
                
                    /*  Save the Upload file */
                    UPL_Save_Upload_File(req, fname, fstart, fsize);
                }
            }
        }
    }
	
	req->ws_fname = NU_NULL;

    /*  Return from plug-in */
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UPL_Save_Upload_File                                                 
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function saves a file to file system.
*       The function is passed in the request structure.  The     
*       file is checked for a previous version and type.  If it did      
*       not exist it saves it to memory.  If it did exist it saved in    
*       in the location it was defined.                                  
*                                                                       
*   INPUTS                                                                
*                                                                       
*       req                         Pointer to Request structure that    
*                                   holds all information pertaining     
*                                   to  the HTTP request.                
*       fname                       File name of the file to be saved.   
*       filemem                     The pointer to the input buffer that 
*                                   contains the file.                   
*       length                      The length of the file in bytes.     
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The function returns WS_FAILURE if it was unable to write the file  
*       to memory or file system.  It returns NU_SUCCESS if    
*       the saving of the file was complete.             
*                                                                        
*************************************************************************/
STATIC VOID UPL_Save_Upload_File(WS_REQUEST * req, CHAR * fname, CHAR *filemem, INT32 length)
{
    CHAR    buf[WS_URI_LEN + WS_PLUGIN_NAME_LEN];
    INT     status;

#if NU_WEBSERV_DEBUG
    printf("UPL_Save_Upload_File %s (%d)\n", fname, length);
#endif

    /* Reset the fname since WSF_File_Request_Status looks for it
     * in the req structure
     */
    req->ws_fname = fname;        
    status = WSF_File_Request_Status(req);

    /* Find out if file exists as a plugin */
    if(status == NU_SUCCESS)
    {
        if( req->ws_stat.ws_flags == WS_PLUGIN )
        {
            /* "' fname 'is the name of a plugin.<BR>Upload failed." */
            strcpy(buf,"\' ");
            strcat(buf, fname);
            strcat(buf, "\'is the name of a plugin.<BR>Upload failed.\n");
            
            HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
            return;
        }
    }

#if INCLUDE_FILE_SYSTEM
    status = WSF_Write_File_System(fname, filemem, (UINT32)length); 
#else
    status = UPL_Save_File(req, fname, filemem, length);
#endif
        
    if (status == NU_SUCCESS)
    {
        /* Not an Error Send Browser Message that it is complete */
        /* "File: 'fname' saved to memory." */
        strcpy(buf, "File: ");
        strcat(buf, fname);
        strcat(buf, " saved to memory.\n");
        HTTP_Send_Status_Message(req, WS_PROTO_OK, buf);
    }
    else
    {
        /* "Error Saving 'fname' to memory. */
        strcpy(buf, "Error Saving ");
        strcat(buf, fname);
        strcat(buf, " to memory.\n");
        HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
    }
}

#if !INCLUDE_FILE_SYSTEM
/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UPL_Save_File                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Function to save an uploaded file into the incore memory file    
*       system.  It checks if file compression is enabled.  If it is     
*       then it checks if it is necessary to compress the file.  A slot to  
*       place the file in memory is looked for.  Once it has found its   
*       slot in memory it write the file to that memory location.        
*                                                                       
*   INPUTS                                                                
*                                                                       
*       fname                       File name of the file to be saved.   
*       filemem                     The pointer to the input buffer that 
*                                   contains the file.                   
*       length                      The length of the file in bytes.     
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The function returns WS_FAILURE if it was unable to write the file  
*       to memory. It returns NU_SUCCESS if the saving of the file to       
*       memory was complete.                                             
*                                                                       
*************************************************************************/
STATUS UPL_Save_File(WS_REQUEST * req, CHAR *fname, CHAR *filemem, INT32 length)
{
    INT32       comp_length;
    WS_FS_FILE  *curr; 
    WS_FS_FILE  *file_ptr = NU_NULL;
    WS_FS_FILE  *prev;
    CHAR        *mem_fname;
    STATUS      status;
    CHAR HUGE   *file_buf;

    /*  Get semaphore to access the in-memory file system. */
    status = NU_Obtain_Semaphore(&WS_FS_Access, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {


#if NU_WEBSERV_DEBUG
        printf("memory: save:%s (%x) %d\n", fname, *filemem, length);
#endif 
        
#if INCLUDE_COMPRESSION
        comp_length = UPL_Compress(req, fname, filemem, length);    
#else
        comp_length = length;
#endif 
        
        /* see if the file exists already */
        curr = prev = HTTP_Fs_File;
        
        while(curr)
        {
            /* TRY:
             * 
             * 1. see if it fits in the old slot
             *    (if we are overwriting an existing file)
             *
             * 2. If we have no choice we allocate new space 
             *    for the uploaded file
             */
            
            mem_fname = curr->ws_name;
            while (*mem_fname == '/')
                mem_fname++;

            if(strcmp(fname, mem_fname) == 0)
            {
                /* File exists in incore file system  
                 * Make sure file is no bigger than the existing file length 
                 */
                
                /* will it fit in old slot? */
                if((curr->ws_type & WS_COMPILED) || (comp_length > curr->ws_length))
                {
                    if(curr == prev)
                        HTTP_Fs_File = curr->ws_next;
                    else
                        prev->ws_next = curr->ws_next;

                    /* If the WS_COMPILED flag is set, the slot in in ROM
                     * and cannot be used or deallocated
                     */
                    if(!(curr->ws_type & WS_COMPILED))
                        if(NU_Deallocate_Memory(curr) != NU_SUCCESS)
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                }
                else
                {
                    /* The file will fit into the current slot */
                    file_ptr = curr;            
                    
                    /*  Set the memory to all 0's */
                    UTL_Zero(file_ptr->ws_addr, (UINT32)comp_length);
                }

                /* set the stop loop flag */
                curr = NU_NULL;
            }
            else
            {    
                prev = curr; /* previous link */
                curr = curr->ws_next;
            }
        }
        
        if(!file_ptr)
        {
            status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID*)&file_buf, 
                                        (UNSIGNED)(comp_length + sizeof(WS_FS_FILE)),
                                        NU_NO_SUSPEND);
            if(status != NU_SUCCESS)
            {
                if (NU_Release_Semaphore(&WS_FS_Access) != NU_SUCCESS)
                {
                    NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
                    printf("Failed to release semaphore\n");
#endif
                }
                return(WS_FAILURE);
            }

            file_ptr = (WS_FS_FILE *)file_buf;
            file_ptr->ws_addr = (CHAR*)(file_buf + (sizeof(WS_FS_FILE)));
            
            strcpy(file_ptr->ws_name, "/");
            strcat(file_ptr->ws_name, fname);
            
            file_ptr->ws_type = 0;

            file_ptr->ws_next = prev->ws_next;
            prev->ws_next = file_ptr;
        }


        if(comp_length != length)
        {
            /*  Add Compressed File tag */
            file_ptr->ws_type = WS_COMPRESSED;
        }
        else
        {
            /*  Don't add compressed File tag */
            file_ptr->ws_type &= ~WS_COMPRESSED;
        }
        
        WS_Mem_Cpy(file_ptr->ws_addr, filemem, (UINT32)comp_length);
        file_ptr->ws_clength = comp_length;
        file_ptr->ws_length = length;

        if (NU_Release_Semaphore(&WS_FS_Access) != NU_SUCCESS)
        {
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
            printf("Failed to release semaphore\n");
#endif
        }

    }

    return(NU_SUCCESS);
}
#endif

#if INCLUDE_COMPRESSION
/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UPL_Compress                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*        This function checks if the file should be compressed, and
*        does so when necessary.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       fname                       File name of the file to be saved.   
*       filemem                     The pointer to the input buffer that 
*                                   contains the file.                   
*       length                      The length of the file in bytes.     
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       size                        Size of file
*                                                                       
*************************************************************************/
STATIC INT32 UPL_Compress(WS_REQUEST *req, CHAR *fname, CHAR *filemem, INT32 length)
{
    CHAR        *buf;
    INT32       size;
    STATUS      status;

#if NU_WEBSERV_DEBUG
    printf("COMPRESSION enabled\n");
#endif
    
    /*  Get the Filename Extension */
    buf = fname;
    while (*(buf + 1) && (*buf != '.'))
        buf++;
    buf++;

    /* find out what the compressed size would be */
    size = CFS_Compress(WS_DONT_OUTPUT, filemem, NU_NULL, length);

#if  NU_WEBSERV_DEBUG
    printf("compressed length = %d\n", size);
#endif
 
    /*  Dont bother compressing unless there is some size benefit.
     *  Gifs and Jpegs are already compressed as far as they can be.     
     *  SSI's should not be compressed because of the dynamic 
     *  calls during serving of the files.                                                          
     */
    if(strcmp(buf, "gif") && strcmp(buf, "jpg") && strcmp(buf, "jpeg") &&
        strcmp(buf, "ssi") && ((size + WS_CHD_SZ ) < (length - 16)))
    {
        status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID*)&buf, 
                                    (UNSIGNED)size, NU_NO_SUSPEND);
        if(status != NU_SUCCESS)
            return(WS_FAILURE);

        /* do the real compression */
        if(size != CFS_Compress(WS_DO_OUTPUT, filemem, buf, length))
        {
#if NU_WEBSERV_DEBUG
            printf("Compression Phase error\n");
#endif
            return(WS_FAILURE);
        }
        
        NU_BLOCK_COPY(filemem, WS_CHD, WS_CHD_SZ);
        WS_Mem_Cpy(filemem + WS_CHD_SZ, buf, (UINT32)size);
        if(NU_Deallocate_Memory(buf) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
        
        size += WS_CHD_SZ;
    }
    else
    {
        size = length;
    }

    return(size);
}
#endif /* INCLUDE_COMPRESSION */
#endif /* INCLUDE_UPLOAD_PLGN */
