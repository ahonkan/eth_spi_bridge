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
*       dir_plgn.c                                                
*                                                                       
* COMPONENT                                                             
*         
*       Nucleus WebServ                                                              
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       File that contains the DIR plugin information.                   
*                                                                       
* DATA STRUCTURES                                                       
*       
*       None                                                                
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       DIR_List_Directory      Plugin to list all files in file system.
*                               Two versions of this function exist, for
*                               with and without an external file system 
*       DIR_Pad                 Converts int to char* and right justifies
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h       
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"

#if INCLUDE_DIR_PLGN

STATIC VOID DIR_Pad(UINT32 number, CHAR* buffer, INT buf_size);

extern WS_FS_FILE * HTTP_Fs_File;

#if !INCLUDE_FILE_SYSTEM
/*  Semaphore to access the in-memory file system. */
extern NU_SEMAPHORE WS_FS_Access;
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DIR_List_Directory                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function gets the structure of the file system and outputs  
*       to the HTTP client the directory structure and the number of     
*       embedded files.  This operation is a plugin and must be          
*       registered within the server before being used.
*       This function behaves differently depending on if an external
*       file system exists, because of this, two functions exist,
*       one for each case.                  
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       WS_REQ_PROCEED          Upon successful completion
*       WS_REQ_ABORTED          When an error occurs
*                                                                       
*************************************************************************/

#if !INCLUDE_FILE_SYSTEM

INT DIR_List_Directory(WS_REQUEST *req)
{
    INT             num_ent = 0;
    CHAR            *file;
    CHAR            outb[400 + WS_URI_LEN];
    CHAR            temp[10];
    WS_FS_FILE      *f;
    INT32           total = 0;
    STATUS          status;

    /*  Get semaphore to access the in-memory file system. */
    status = NU_Obtain_Semaphore(&WS_FS_Access, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {

        /* Build the response header */
        HTTP_Response_Header(req, WS_PROTO_OK );
        HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, WS_TYPE_TXT_HTML);
    
        /* Begin the creation of the HTML page */
        strcpy(outb, "<HTML><HEAD>\n<TITLE>Index of ");
        strcat(outb, "/" );
        strcat(outb, "</TITLE>\n</HEAD><BODY>\n" );
        strcat(outb, "<H1>Directory Index of ");
        strcat(outb, "/");
        strcat(outb, "</H1>\n");
    
        status = WSN_Write_To_Net(req, outb, (UINT32)strlen(outb), WS_PLUGIN_DATA);
    
        strcpy(outb, "<pre>\n");
        f = HTTP_Fs_File;
    
        /* Loop through, getting information from every file stored in memory,
         * and then create a line of that information on the web page.
         */
        while(f && (status == NU_SUCCESS))
        {
            file = f->ws_name; 
            total += f->ws_length;
    
            /* Convert the length of the file to ASCII */
            DIR_Pad((UINT32)f->ws_length, temp, 10);
    
            strcat(outb, temp);
            strcat(outb, " bytes    ");
    
            /* Create an anchor so that user can link straight to file */
            strcat(outb, "<A HREF=\"");
    
            /* Check if this is in the private directory, if so, change link */
            if(strncmp(file, WS_PRIVATE_DIR, sizeof(WS_PRIVATE_DIR) - 1) == 0)
                file += (sizeof(WS_PRIVATE_DIR) - 1);
            strcat(outb, file);
            strcat(outb, "\">");
            strcat(outb, f->ws_name);
            strcat(outb, "</A>\n"); 
    
            /* Write this line out and begin the next line */
            status = WSN_Write_To_Net(req, outb, (UINT32)strlen(outb), WS_PLUGIN_DATA);
            outb[0]=0;
            
            num_ent++;
            
            f = f->ws_next;
        }
        
        if(status == NU_SUCCESS)
        {
            /* Finish the page by printing some statistics */    
            strcpy(outb, "\nTotal Entries ");
            strcat(outb, (CHAR *)NU_ITOA(num_ent, temp, 10));
            strcat(outb, "\nTotal Size    ");
            strcat(outb, (CHAR *)NU_ULTOA((UNSIGNED)total, temp, 10));
            strcat(outb, "\n</pre>\n</body>\n</html>");
    
            /* The page is done, send it on to the user */
            status = WSN_Write_To_Net( req, outb, (UINT32)strlen(outb), WS_PLUGIN_DATA);
            if(status == NU_SUCCESS)
                status = WSN_Write_To_Net( req, NU_NULL, 0, WS_PLUGIN_SEND);
        }
        else
            NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
    
        if (NU_Release_Semaphore(&WS_FS_Access) != NU_SUCCESS)
        {
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
            printf("Failed to release semaphore\n");
#endif
        }
    }

    if(status == NU_SUCCESS)
        status = WS_REQ_PROCEED;
    else
        status = WS_REQ_ABORTED;

    return(status);
}

#else

/* Define the bit positions within File's DATESTR */
#define YEARMASK    0xFE00
#define MONTHMASK   0x01E0
#define DAYMASK     0x001F
#define HOURMASK    0xF800
#define MINMASK     0x07E0
#define SECMASK     0x001F
INT DIR_List_Directory(WS_REQUEST *req )
{
    INT             num_ent = 0;
    CHAR            outb[400 + WS_URI_LEN];
    CHAR            temp[40];
    CHAR            *dir;
    CHAR            *temp_ptr;
    CHAR            dir_temp[3] = {0,0,0};
    STATUS          status;
    DSTAT           statobj;
    INT32           total = 0;
    INT             tm_hour;
    INT             tm_min;
    INT             tm_sec;
    INT             tm_mon;
    INT             tm_mday;
    INT             tm_year;


    /* Build the response header */
    HTTP_Response_Header(req, WS_PROTO_OK);
    HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, WS_TYPE_TXT_HTML);

    /* Get the directory that the user would like a listing of */
    dir = HTTP_Token_Value_by_Name("dir", req);
    if(!dir || !dir[1])
    {
        dir_temp[0] = '\\';
        dir = dir_temp;
    }

    /* Begin the creation of the HTML page */
    strcpy(outb, "<HTML><HEAD>\n<TITLE>Index of ");
    strcat(outb, dir );
    strcat(outb, "</TITLE>\n</HEAD><BODY>\n" );
    strcat(outb, "<H1>Directory Index of ");
    strcat(outb, dir);
    strcat(outb, "</H1>\n");

    status = WSN_Write_To_Net(req, outb, (UINT32)strlen(outb), WS_PLUGIN_DATA);

    if(status == NU_SUCCESS)
    {
        /* Set up the search pattern and call search function */
        strcpy(outb, dir);
        for(temp_ptr = outb;*temp_ptr; temp_ptr++)
            if(*temp_ptr == '/')
                *temp_ptr = '\\';
        strcat(outb, "\\*.*");

        status = NU_Get_First (&statobj, &outb[1]);
        if (status == NU_SUCCESS)
        {
            if (statobj.fattribute & AVOLUME)
            {
                status = NU_Get_Next (&statobj);
            }
        }
    }
    
    /* If the directory exists, continue */
    if(status == NU_SUCCESS)
    {
        outb[strlen(dir)] = 0;
        status = NU_Set_Current_Dir (outb);

        strcpy(outb, "<pre>");
        
        /* Loop through, getting information from every file stored in memory,
         * and then create a line of that information on the web page.
         */
        while( status == NU_SUCCESS )
        {
            /* Extract time information. */
            tm_year = ((statobj.fupdate & YEARMASK ) >> 9) + 1980;
            tm_mon  = ((statobj.fupdate & MONTHMASK) >> 5);
            tm_mday = ( statobj.fupdate & DAYMASK  );
            tm_hour = ((statobj.fuptime & HOURMASK ) >> 11);
            tm_min  = ((statobj.fuptime & MINMASK  ) >> 5);
            tm_sec  = ((statobj.fuptime & SECMASK  ) << 1);

            /* Print the date */
            DIR_Pad((UINT32)tm_year, temp, 8);
            strcat(outb, temp);
            strcat(outb, "-");
            DIR_Pad((UINT32)tm_mon, temp, 3);
            strcat(outb, temp);
            strcat(outb, "-");
            DIR_Pad((UINT32)tm_mday, temp, 3);
            strcat(outb, temp);
            
            /* Print the time */
            DIR_Pad((UINT32)tm_hour, temp, 6);
            strcat(outb, temp);
            strcat(outb, ":");
            DIR_Pad((UINT32)tm_min, temp, 3);
            strcat(outb, temp);
            strcat(outb, ":");
            DIR_Pad((UINT32)tm_sec, temp, 3);
            strcat(outb, temp);
            strcat(outb, "   ");

            /* If this is a directory, label it so */
            if(statobj.fattribute & ADIRENT) 
                strcat(outb, "      &lt;DIR&gt; ");
            else
            {
                /* If it is not a directory, print the size of the file */
                total += statobj.fsize;
                DIR_Pad(statobj.fsize, temp, 10);
                strcat(outb, temp);
            }
            
            /* Begin the anchor for the user to link through */
            strcat(outb,"   <A HREF=\"");
            
            /* If this is a directory, set the link to come back to this plugin */
            if(statobj.fattribute & ADIRENT) 
            {   
                strcat(outb, "dir");
                strcat(outb, "?dir=");
                strncpy(temp, statobj.lfname, sizeof(temp)-1);
                temp[sizeof(temp)-1] = 0;
                if(!dir_temp[0] && *temp != '.')
                    strcat(outb, dir);

                strcat(outb, "/");

                if(*temp != '.')
                    strcat(outb, temp);
            }
            else
            {
                /* This is a file */
                strncpy(temp, statobj.lfname, sizeof(temp)-1);
                temp[sizeof(temp)-1] = 0;
                if(!dir_temp[0])
                {
                    strcat(outb, dir);
                    strcat(outb, "/");
                }
                strcat(outb, temp);
            }            
            
            strcat(outb, "\">");
            
            /* Print the name */
            strcat(outb, temp);
            strcat(outb, "</A>\n"); 

            /* Write this line out and begin the next line */
            status = WSN_Write_To_Net(req, outb, (UINT32)strlen(outb), WS_PLUGIN_DATA);
            outb[0] = 0;
            
            /* Get the next file in the directory */
            if(status == NU_SUCCESS)
                status = NU_Get_Next (&statobj);

            num_ent++;
        }
        
        /* Finish the page by printing some statistics */    
        strcpy(outb, "\nTotal Entries ");
        strcat(outb, (CHAR *)NU_ITOA(num_ent, temp, 10));
        strcat(outb, "\nTotal Size    ");
        strcat(outb, (CHAR *)NU_ULTOA((unsigned long)total, temp, 10));
        strcat(outb, "\n</pre>\n</body>\n</html>");

        /* The page is done, send it on to the user */
        status = WSN_Write_To_Net( req, outb, (UINT32)strlen( outb ), WS_PLUGIN_DATA);
        if(status == NU_SUCCESS)
            status = WSN_Write_To_Net( req, NU_NULL, 0, WS_PLUGIN_SEND);
    }
    else
        NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);

    NU_Done(&statobj);


    NU_Set_Current_Dir (WS_FS_PREFIX);

    if(status == NU_SUCCESS)
        status = WS_REQ_PROCEED;
    else
        status = WS_REQ_ABORTED;

    return(status);
}
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DIR_Pad                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function converts an integer to an ASCII string.  It
*       right justifies the string using spaces for padding.
*                                                                       
* INPUTS                                                                
*                                                                       
*       number                  Integer to be converted 
*       *buffer                 Pointer to the buffer where the
*                               new string will reside.
*       buf_size                Size of the buffer.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/

STATIC VOID DIR_Pad(UINT32 number, CHAR* buffer, INT buf_size)
{
    INT     x;
    INT     i;

    /* Move to the last position in the array */
    buf_size--;

    /* Convert the number and place it into the buffer */
    NU_ULTOA(number, buffer, 10);

    /* Make the last character in the string a null terminator */
    buffer[buf_size--] = '\0';
    x = strlen(buffer);

    /* Shift the string to the end of the array, therefore making it
     * right justified.
     */
    for(i = buf_size; x; i--)
        buffer[i] = buffer[--x];

    /* Pad the begining of the string with spaces */
    while(i > -1)
        buffer[i--] = ' ';
}

#endif /* INCLUDE_DIR_PLGN */

