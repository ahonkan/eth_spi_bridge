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
*       wsf.c                                                                                                      
*                                                                      
* COMPONENT                                                            
*                                                                      
*       Nucleus WebServ    
*                                                                      
* DESCRIPTION                                                          
*               
*       This file contains all functions that have direct access
*       to the file system.
*                                                                      
* DATA STRUCTURES                                                      
*                                                                      
*                                                                      
* FUNCTIONS                                                            
*                                                                      
*       WSF_File_Request_Status     Checks to see if the file is     
*                                   in memory or the external file   
*                                   system.                          
*       WSF_File_Match              Search for client requested file 
*                                   name.                            
*       WSF_File_Status             Check for external storage of file.                             
*       WSF_Send_File               Write the file to socket.         
*       WSF_Read_File               Copy the file to an array.        
*       WSF_Write_File_System       Write to the mass storage medium.                      
*       WSF_Name_File               Places the filename into correct     
*                                   format.  
*       WSF_Check_Dir_Status        Creates a directory if it does
*                                   not exist
*       WSF_File_Find               Finds the location of the file
*                                                                      
* DEPENDENCIES                                                         
*
*       nu_websr.h
*                                                                      
************************************************************************/

#include "networking/nu_websr.h"
#if !INCLUDE_FILE_SYSTEM
STATIC INT          WSF_File_Match(CHAR * name, CHAR ** start, INT32 * length, INT * type);
#endif

#if INCLUDE_FILE_SYSTEM
STATIC INT          WSF_File_Status(WS_REQUEST * req, CHAR * file_name);
STATIC VOID         WSF_Name_File(CHAR * name, CHAR * buf);
STATIC STATUS       WSF_Check_Dir_Status(CHAR *name_buf);
#endif

extern WS_PLUGIN_STRUCT *HTTP_Plugins;
extern WS_FS_FILE * HTTP_Fs_File;

#if !INCLUDE_FILE_SYSTEM
/* Semaphore to access the in-memory file system. */
extern NU_SEMAPHORE WS_FS_Access;
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WSF_File_Request_Status                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function that searches for a particular uri. The function      
*       first looks to see if the uri is a plugin. If          
*       not then it checks if the file exists within the system.            
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to WS_REQUEST structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The file was found
*       WS_FAILURE              The file was not found
*                                                                       
*************************************************************************/

INT  WSF_File_Request_Status(WS_REQUEST *req)
{
    CHAR    file_name[WS_URI_LEN + sizeof(WS_PRIVATE_DIR)];
    CHAR    *p;
    WS_PLUGIN_STRUCT *pointer;
    INT     status = NU_SUCCESS;
    CHAR	temp[sizeof(WS_PRIVATE_DIR)];
    
    for(p = req->ws_fname; *p == '/'; p++);
    
    req->ws_stat.ws_flags = WS_NOT_FOUND;

    /* Check if URI is a plugin */
    pointer = HTTP_Plugins;
    while(pointer)
    {
        if(WS_Strcmp(p, pointer->ws_name) == 0 )
        {
            /* URI is a plugin, get data for later use */
            req->ws_stat.plugin = pointer->plugin;
            req->ws_stat.ws_flags = pointer->ws_plg_flag; 
            req->ws_stat.ws_flags |= WS_PLUGIN;
            pointer = NU_NULL;
        }
        else
            pointer = pointer->ws_next;
    }
    
    /* If not a plugin, check if the file exists */
    if(req->ws_stat.ws_flags == WS_NOT_FOUND)
    {
        if(WSF_File_Find(req, req->ws_fname) == WS_FAILURE)
        {
            /* Check if file is in the private directory */
            strcpy(file_name, WS_PRIVATE_DIR);
            strcat(file_name, req->ws_fname);
            if(WSF_File_Find(req, file_name) == WS_FAILURE)
            {
                /* Check if file is in the public directory */
                strcpy(file_name, WS_PUBLIC_DIR);
                strcat(file_name, req->ws_fname);
                if(WSF_File_Find(req, file_name) == WS_FAILURE)
                    status = WS_FAILURE;
                else
                    req->ws_stat.ws_flags |= WS_PUBLIC;
            }
            else
                req->ws_stat.ws_flags |= WS_PRIVATE;
        }
        /* Check if file path includes private directory */
        else
            {
              UTL_Zero(temp, sizeof(WS_PRIVATE_DIR));
              strncpy(temp, req->ws_fname, sizeof(WS_PRIVATE_DIR)-1);
              if(NU_STRICMP(temp, WS_PRIVATE_DIR) == 0)
            	  req->ws_stat.ws_flags |= WS_PRIVATE;
             }
    }
    
    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WSF_File_Find                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function  checks if the file is in memory or on an external
*       file system. If not then it returns a value that states that
*       the uri was not found.            
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to WS_REQUEST structure that
*                               holds all information pertaining 
*                               to the HTTP request.    
*       file_name               Name of file to be searched for.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The file was found
*       WS_FAILURE              The file was not found
*                                                                       
*************************************************************************/
INT WSF_File_Find(WS_REQUEST *req, CHAR *file_name)
{
    INT32   status;
#if !INCLUDE_FILE_SYSTEM
    CHAR    *start;
    INT32   length;
    INT     type;
	
    /* Check if the file exists in the local file system */
    status = WSF_File_Match(file_name, &start, &length, &type);
    if(status == NU_SUCCESS)
    {
        req->ws_stat.ws_address = start;
        req->ws_stat.ws_size = length;
        req->ws_stat.ws_type = (INT16)type;
        req->ws_stat.ws_flags = WS_INCORE | WS_FOUND;
    }  
#else /* #if INCLUDE_FILE_SYSTEM */
    /* Find out if it's on external storage */
    status = WSF_File_Status(req, file_name);
#endif

    return (INT)(status);
}

#if !INCLUDE_FILE_SYSTEM
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WSF_File_Match                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function searches through the in core based file system   
*       for a client requested filename.                                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Filename to be examined.             
*       **start                 Pointer to start location in memory  
*                               of the file.                         
*       *length                 Pointer to the length of the file.   
*       *type                   Flags that concern this file                         
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       SUCCESS                 The filename was found in the
*                               file system structure
*       FAILURE                 The filename was not found             
*                                                                       
*************************************************************************/
STATIC INT WSF_File_Match(CHAR *name, CHAR **start, INT32 *length, INT *type)
{
    WS_FS_FILE  *file_list;
    CHAR        *new_name;
    INT         status = WS_FAILURE;

    /*  Get semaphore to access the in-memory file system. */
    status = NU_Obtain_Semaphore(&WS_FS_Access, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        status = WS_FAILURE;

        /* Get the list of files and search one by one */
        file_list = HTTP_Fs_File;
    
        if(file_list != NU_NULL)
        {
            /* bump past any leading /'s */
            while ( *name == '/' )
                name++;                        
        
            while (file_list && status != NU_SUCCESS)
            {
                /* Check if there is data there */
                if(file_list->ws_length)
                {
                    /* bump past any leading /'s */
                    new_name = file_list->ws_name;
                    while ( *new_name == '/' )
                        new_name++;
    
                    /* Compare file name and requesting file name */
                    if( WS_Strcmp(name, new_name) == 0 )
                    {
                        *start = file_list->ws_addr;
                        *length = file_list->ws_length;
                        *type = file_list->ws_type;
                    
                        status = NU_SUCCESS;
                    }
                }
    
                /* Check the next file */
                file_list = file_list->ws_next;
            }
        }

        if (NU_Release_Semaphore(&WS_FS_Access) != NU_SUCCESS)
        {
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
            printf("Failed to release semaphore\n");
#endif
        }

    }

    if (status != NU_SUCCESS)
    {
        status = WS_FAILURE;
    }
    
    return(status);
}
#endif /* !INCLUDE_FILE_SYSTEM */

#if INCLUDE_FILE_SYSTEM
/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Is_Dir                                        
*                                                                      
* DESCRIPTION                                                          
*
*     Verify if the given directory exists.
*                                                                      
*  INPUTS
*
*      path                        Directory path to verify.
*
*
* OUTPUTS                                                              
*
*     NU_SUCCESS                   If Successful
*     -1                           If Unsuccessful
*
************************************************************************/

INT WSF_Is_Dir(CHAR *path)
{
    CHAR    nu_drive[3];
    CHAR    current_dir[256];
    INT     drive;

    current_dir[0] = 'A';

    drive = NU_Get_Default_Drive();

    nu_drive[0] = (CHAR)('A' + drive);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    NU_Current_Dir ((UINT8*)nu_drive, current_dir);

    if (NU_Set_Current_Dir (path) == NU_SUCCESS)
    {
        NU_Set_Current_Dir(current_dir);
        return (NU_SUCCESS);
    }
    else
        return (-1);
}



/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_File_Status                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      Checks if the file to be saved is on external storage and fills  
*      out the request stat structure on file size.                     
*                                                                      
* INPUTS                                                               
*                                                                      
*      req                         Pointer to Request structure that   
*                                  holds all information pertaining to 
*                                  the HTTP request.
*      file_name                   Name of file to be found                   
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     NU_SUCCESS                   Returned on successful find.                                    
*                                                                      
************************************************************************/

STATIC INT WSF_File_Status(WS_REQUEST * req, CHAR *file_name)
{
    DSTAT    statobj;
    CHAR    buf[WS_URI_LEN + 1];    /* add 1 for WS_FS_PREFIX */
    STATUS  status;

    /* Setup the name so that an external file system will recognize it */
    WSF_Name_File(file_name, buf);

    status = NU_Get_First (&statobj, buf);
    if (status == NU_SUCCESS)
    {
        if (statobj.fattribute & AVOLUME)
        {
            status = NU_Get_Next (&statobj);
        }
    }

    if (status  == NU_SUCCESS)
    {
        req->ws_stat.ws_size = statobj.fsize;
        req->ws_stat.ws_flags = WS_FOUND;
        NU_Done(&statobj);
        return(NU_SUCCESS);
    }
    else
        return(WS_FAILURE);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Name_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Places the file name in the correct format so it can be read or 
*     written with an external file system.                            
*                                                                      
* INPUTS                                                               
*                                                                      
*     name                       Pointer to name of file before correcton.                   
*     buf                        Pointer to corrected name.         
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/
STATIC VOID WSF_Name_File(CHAR * name, CHAR * buf )
{
    CHAR    *path;

    /* base of server root */
    strcpy(buf, WS_FS_PREFIX);       

    while (*name == '/')
        name++;

    for(path = name; *path; path++)
    {
        if(*path == '/')
            *path = '\\';
#if !WS_CASE_SENSITIVE
        else
            *path = (CHAR)NU_TOUPPER(*path);
#endif
    }

    strcat(buf, name);  
    
#if NU_WEBSERV_DEBUG
    printf("NAME = %s\n", buf);
#endif
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Send_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function to write the file to the socket.                     
*                                                                      
* INPUTS                                                               
*                                                                      
*     req                         Pointer to Request structure that    
*                                  holds all information pertaining to  
*                                  the HTTP request.                    
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     Returns NU_SUCCESS if Successful.                                    
*     Returns WS_Failure if unsuccessful.                                 
*                                                                      
************************************************************************/
INT WSF_Send_File(WS_REQUEST * req)
{
    CHAR        buf[WS_URI_LEN + 1];    /* add 1 for WS_FS_PREFIX */
    CHAR        fbuf[WS_FBUF_SZ];
    INT         fd;
    UINT16      mode;
    UINT32      j;
	STATUS      status;


    /* If this file lies within a subdirectory, prepend the 
     * directory name to the file name
     */
    if(req->ws_stat.ws_flags & WS_PUBLIC)
    {
        strcpy(fbuf, WS_PUBLIC_DIR);
        strcat(fbuf, req->ws_fname);
        WSF_Name_File(fbuf, buf);
    }
    else if(req->ws_stat.ws_flags & WS_PRIVATE)
    {
        strcpy(fbuf, WS_PRIVATE_DIR);
        strcat(fbuf, req->ws_fname);
        WSF_Name_File(fbuf, buf);
    }
    else
        WSF_Name_File(req->ws_fname, buf);

    mode = PS_IREAD;

    /* Open the file for reading */
    if((fd = NU_Open((CHAR *)buf, PO_RDONLY, (UINT16)mode)) >= 0)
    {
        while((j = NU_Read(fd, (CHAR *)fbuf, WS_FBUF_SZ)) > 0)
        {
            if(WSN_Write_To_Net(req, fbuf, j, WS_FILETRNSFR) != NU_SUCCESS)
                NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
        }            
    }
    else
    {
#if NU_WEBSERV_DEBUG
        printf("can't open %s \n",buf);
#endif
        return(WS_FAILURE);
    }    

    status = NU_Close(fd);
    if(status != NU_SUCCESS)
        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
    
    return(NU_SUCCESS);
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Read_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Copy the file to an array.                                    
*                                                                      
* INPUTS                                                               
*                                                                      
*     req                         Pointer to Request structure that    
*                                 holds all information pertaining to  
*                                 the HTTP request.                    
*     buffer                      Pointer to the buffer that bytes read
*                                 from the file is to be placed.       
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     Returns NU_SUCCESS if Successful.                                    
*     Returns WS_FAILURE if unsuccessful.                                 
*                                                                      
************************************************************************/
INT WSF_Read_File(WS_REQUEST * req, CHAR * buffer )
{
    CHAR        buf[WS_URI_LEN + sizeof(WS_PRIVATE_DIR)];
    CHAR        temp_buf[WS_URI_LEN + sizeof(WS_PRIVATE_DIR)];
    INT         fd;
    UINT32      j; 
    INT         status = WS_FAILURE;

    /* If this file lies within a subdirectory, prepend the 
     * directory name to the file name
     */
    if(req->ws_stat.ws_flags & WS_PUBLIC)
    {
        strcpy(temp_buf, WS_PUBLIC_DIR);
        strcat(temp_buf, req->ws_fname);
        WSF_Name_File(temp_buf, buf);
    }
    else if(req->ws_stat.ws_flags & WS_PRIVATE)
    {
        strcpy(temp_buf, WS_PRIVATE_DIR);
        strcat(temp_buf, req->ws_fname);
        WSF_Name_File(temp_buf, buf);
    }
    else
        WSF_Name_File(req->ws_fname, buf);
    
    /* Open the file that is to be read */
    if((fd = NU_Open((CHAR *)buf, PO_RDONLY, (UINT16)PS_IREAD)) >= 0)
    {
        j = NU_Read(fd, (CHAR *)buffer, req->ws_stat.ws_size);
        
        status = NU_Close(fd);
        if(status != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        
        if (j == (UINT32)req->ws_stat.ws_size)
            status = NU_SUCCESS;
    }
    else
    {
#if NU_WEBSERV_DEBUG
        printf("can't open %s \n",buf);
#endif
    }   
    
    return(status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Write_File_System                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function that used to write to an external file system.                       
*                                                                      
* INPUTS                                                               
*                                                                      
*     fname                       File name to be written under.       
*     filemem                     The pointer to the file in memory.   
*     length                      The length of the file.              
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     Returns NU_SUCCESS if Successful.                                    
*     Returns WS_FAILURE if unsuccessful.                                 
*                                                                      
************************************************************************/

INT WSF_Write_File_System(CHAR * fname, CHAR HUGE* filemem, UINT32 length)
{
    INT         fd;
    UINT16      mode;
    CHAR        name_buf[WS_URI_LEN];
    STATUS      status;
    
    filemem = (CHAR HUGE*)TLS_Normalize_Ptr(filemem);
    mode = PS_IWRITE;
    WSF_Name_File(fname, name_buf);

    status = WSF_Check_Dir_Status(name_buf);
    
    if(status == NU_SUCCESS)
    {
        if((fd = NU_Open(name_buf, PO_WRONLY|PO_CREAT, mode)) >= 0)
        {
            while(length && status == NU_SUCCESS)
            {
                if(length > USHRT_MAX)
                {
                    if((NU_Write(fd, (CHAR*)filemem, USHRT_MAX)) != USHRT_MAX)
                    {
                        status = NU_Close(fd);
                        if(status != NU_SUCCESS)
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        status = WS_FAILURE;         
                    }

                    length -= USHRT_MAX;
                    filemem += USHRT_MAX;
                }
                else
                {
                    if((NU_Write(fd, (CHAR*)filemem, length)) != length)
                    {
                        status = NU_Close(fd);
                        if(status != NU_SUCCESS)
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        status = WS_FAILURE;         
                    }

                    length = 0;
                }
            }

            status = NU_Close(fd);
            if(status != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        } 
    }
    
    return(status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WSF_Check_Dir_Status                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function checks if a directory structure is in place, if
*     not, it creates it.                       
*                                                                      
* INPUTS                                                               
*                                                                      
*     name_buf                    Full file name with path.       
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     Returns NU_SUCCESS if Successful.                                    
*     Returns WS_FAILURE if unsuccessful.                                 
*                                                                      
************************************************************************/

STATIC STATUS WSF_Check_Dir_Status(CHAR *name_buf)
{
    CHAR    *temp;
    CHAR    dir[64];
    STATUS  status = NU_SUCCESS;

    UTL_Zero(dir, 64);

    /* Check if this file is in a directory */
    temp = name_buf;

    while(*temp == '\\')
        temp++;

    while(*temp && (*temp != '.') && (status == NU_SUCCESS))
    {
        if(*temp == '\\')
        {
            strncpy(dir, name_buf, (unsigned int)(temp - name_buf));

            if(WSF_Is_Dir(dir) != NU_SUCCESS)
                if(NU_Make_Dir(dir) != NU_SUCCESS)
                    status = WS_FAILURE;
        }

        temp++;
    }

    return (status);
}

#endif
