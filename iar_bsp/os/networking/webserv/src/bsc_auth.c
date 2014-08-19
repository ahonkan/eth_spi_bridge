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

/***************************************************************************
*                                                                          
* FILENAME                                                                
*                                                                          
*       bsc_auth.c                                             
*
* COMPONENT
*
*       Nucleus WebServ
*                                                                          
* DESCRIPTION                                                              
*                                                                          
*       This File Contains several routines used for Initializing and       
*       implementing the HTTP 1.0 Basic Authentication Algorithm.  
*       Also contains two user plug-ins that are used to add, delete 
*       and show users on the system. To include this feature ws_cfg.h 
*       must have INCLUDE_BASIC_AUTH defined as NU_TRUE.                                            
*                                                                          
* DATA STRUCTURES                                                          
*                                                                          
*       None
*                                                                          
* FUNCTIONS                                                                
*                                                                          
*       BSC_Auth_Init                   Initializes authentication
*       BSC_Auth_Add_Entry              Adds user id to auth list
*       BSC_Auth_Delete_Entry           Removes user id from auth list
*       BSC_Parse_Auth                  Authenticates user
*       BSC_Base64_Decode               Decodes encoded string
*       BSC_Add_Delete_Auth             Plugin to add or remove user
*                                       from auth list
*       BSC_Show_Users                  Plugin to display all users
*                                       in the auth list
*       BSC_Verify_User                 Verifies user is in auth list
*                                                                           
*                                                                          
* DEPENDENCIES                                                             
*                                                                          
*       bsc_auth.h                                                            
*       nu_websrv.h                                                     
*                                                                          
***************************************************************************/

#include "networking/nu_websr.h"

#if  INCLUDE_BASIC_AUTH
#include "networking/netevent_group.h"
#include "networking/wpw_auth.h"

/*  401 Unauthorized Response Field */
static CHAR BSC_Text_401[]="\
HTTP/1.1 401 Unauthorized\r\n\
WWW-Authenticate: Basic realm=\" ";

static CHAR BSC_Text_401c[] = "\"\r\n\
Content-type: text/html\r\n\
Content-length: 0\r\n\r\n";

STATIC WPW_INFO_NODE *BSC_Verify_User(CHAR *user_id, CHAR *password);

/*  Define User PLug-in for adding and deleting a user. */
STATIC INT BSC_Add_Delete_Auth(WS_REQUEST *req);

/*  Define Server Side Include Function that will show all users on
 *  the system.
 */
STATIC INT BSC_Show_Users(WS_REQUEST *req);

#if !(defined NET_5_1)
extern struct WPW_AUTH_NODE WPW_Table[];
extern WS_SERVER WS_Master_Server;
/*  Setup Pointers to the linked List structure Basic Auth Link List */
WPW_INFO_LIST  BSC_Pw_List_info;
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Auth_Init                                                     
*                                                                       
* DESCRIPTION
*            
*       Function to initialize the Basic Authentication utilities.
*
* INPUTS
*
*       None
*
* OUTPUTS                                                           
*           
*       NU_MEM_ALLOC        Could not allocate memory
*       NU_SUCCESS          Success                                                            
*                                                                       
************************************************************************/

INT16 BSC_Auth_Init(VOID)
{
    STATUS          status = NU_SUCCESS;
#if !(defined NET_5_1)
    INT             i;
#endif

    /*  Register the Plug-in to show all users */
    WS_Register_Plugin(BSC_Show_Users, "userp", NU_NULL);

    /*  Register the plug-in that adds or deletes a user. */
    WS_Register_Plugin(BSC_Add_Delete_Auth, "addelu", NU_NULL);
    
#if !(defined NET_5_1)
    /* Build the Basic Authentication Password List list. */
    for (i = 0; status == NU_SUCCESS && 
         WPW_Table[i].wpw_user_id[0]; i++)
    {
        status = BSC_Auth_Add_Entry(WPW_Table[i].wpw_user_id,
                                    WPW_Table[i].wpw_password);
    }
#endif

    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Verify_User                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      This function searches the list of users to find a match
*      with the id given.                             
*                             
* INPUTS
*
*       *user_id
*       *password
*
* OUTPUTS
*
*       bpwlist_info            A match was found
*       NU_FALSE                A match was not found
*                                                                       
************************************************************************/

STATIC WPW_INFO_NODE *BSC_Verify_User(CHAR *user_id, CHAR *password)
{
#ifdef NET_5_1
    if(UM_Validate_User(user_id, password, UM_WEB) == NU_SUCCESS)
        return((WPW_INFO_NODE *)NU_TRUE);
    return(NU_FALSE);
#else
    STATUS          found = NU_FALSE;
    WPW_INFO_NODE   *bpwlist_info;
    
    for(bpwlist_info = BSC_Pw_List_info.wpw_list_head;
         bpwlist_info; bpwlist_info = bpwlist_info->wpw_list_next)
    {
        if(strcmp(bpwlist_info->wpw_user, user_id) == 0 )
        {
            if (strcmp(bpwlist_info->wpw_password, password) == 0)
            {
                found = NU_TRUE;
                break;
            }
        }
    }

    return(bpwlist_info);
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Auth_Add_Entry                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      This function dynamically adds a user id and password combination
*      into the Nucleus WebServ's database.                             
*                             
* INPUTS
*
*       *user_id
*       *password
*
* OUTPUTS
*
*       WPW_BASIC_AUTH_FAILED   The user id and password already exist
*       NU_MEM_ALLOC            Could not allocate memory
*       NU_SUCCESS              Success
*                                                                       
************************************************************************/
INT16 BSC_Auth_Add_Entry(CHAR *user_id, CHAR *password)
{
    INT16           status;
    
#ifdef NET_5_1
    status = (INT16)UM_Add_User(user_id, password, UM_WEB, NU_NULL);
#else
    WPW_INFO_NODE   *bpwlist_info;
    status = NU_SUCCESS;

    if(BSC_Verify_User(user_id, password))
       status = WPW_BASIC_AUTH_FAILED;
    
    if(status == NU_SUCCESS && 
       strlen(user_id) < WS_AUTH_NAME_LEN && strlen(password) < WS_AUTH_NAME_LEN)
    {
        /*  Allocate Memory for the new database entry   */
        status = NU_Allocate_Memory (WS_Master_Server.ws_memory_pool,
                                    (VOID **)&bpwlist_info,
                                     sizeof (WPW_INFO_NODE), NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Setup the New User id to add */
            strcpy(bpwlist_info->wpw_user, user_id);

            /* Setup the password */
            strcpy(bpwlist_info->wpw_password, password);

            /* Add this host to the list. */
            DLL_Enqueue(&BSC_Pw_List_info, bpwlist_info);
        }
    }
    else
        status = WS_FAILURE;
#endif
    
    return(status);
}
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Auth_Delete_Entry                                             
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      This function is used to dynamically delete a combination        
*      of user id and password from the Nucleus WebServ's database.     
*                                  
* INPUTS
*
*       *user_id
*       *password
*
* OUTPUTS
*
*       NU_SUCCESS              Success      
*                                                                       
************************************************************************/
INT16 BSC_Auth_Delete_Entry(CHAR *user_id, CHAR *password)
{
#ifdef NET_5_1
    UINT16  status;

    status = (UINT16)UM_Validate_User(user_id, password, UM_WEB);
    if(status == NU_SUCCESS)
        status = (UINT16)UM_Del_User(user_id);

    return(status);
#else
    WPW_INFO_NODE   *bpwlist_info;

    bpwlist_info = BSC_Verify_User(user_id, password);
    if (bpwlist_info)
    {
        /*  If entry is found remove it from the list and delete it. */
        DLL_Remove(&BSC_Pw_List_info, bpwlist_info);
        if(NU_Deallocate_Memory(bpwlist_info) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    }
    else
        return(WS_FAILURE);

    return(NU_SUCCESS);
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Parse_Auth                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function is used to process the Basic Authentication        
*       method.  This inlcudes parsing the HTTP packet and getting the   
*       bas64 encoded user id and password.  It then calls a function    
*       that decodes the string into a userid:password format. Then the  
*       database checks if the user exists on the system.  If the   
*       Authorization token is not found a 401 message is sent to pop-up 
*       the browser's network password method. If the user exists the    
*       server acts as normal.                                           
*                          
* INPUTS
*
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*
* OUTPUTS
*
*       FAILURE                 Could not allocate memory
*       WPW_BASIC_AUTH_FAILED   Authentication failed
*       NU_SUCCESS              Success                    
*                                                                       
************************************************************************/
STATUS BSC_Parse_Auth(WS_REQUEST *req)
{
    CHAR            total_char[WPW_BASIC_MAX_SIZE];
    CHAR            final_decode[WPW_BASIC_MAX_SIZE];
    CHAR            *user_id;
    CHAR            *password;
    CHAR HUGE       *temp_ptr;
    CHAR HUGE       *line_ptr;
    CHAR            *req_buf_end;
    CHAR            buf[sizeof(BSC_Text_401) + WS_URI_LEN + sizeof(BSC_Text_401c)];
    UINT32          len;
    INT             count = 0;

    /*  Prepare to check first Packet of Incoming HTTP data */
    line_ptr = (CHAR HUGE*)req->ws_rdata.ws_in_header;

    /*  Set the End of the header  */
    req_buf_end = (CHAR*)&req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz];

    /* Get to the first Sequence delimeter CRLF*/
    while((*line_ptr != 0x0d) && (*(line_ptr + 1) != 0x0a))
        line_ptr++;
    line_ptr++;

    /*  Search for  HTTP Authorization Header  */
    temp_ptr = WS_Find_Token("Authorization:", (CHAR *)line_ptr, (CHAR *)req_buf_end);
    if(temp_ptr)
    {
        /*  Search for the Basic Authetication Marker  */
        temp_ptr = (CHAR HUGE*)WS_Find_Token("Basic", (CHAR *)temp_ptr, req_buf_end);
        if(temp_ptr)
        {
           line_ptr = total_char;
           temp_ptr += 6;
           while((*temp_ptr != '\n') && (*temp_ptr !='\r') && (*temp_ptr) &&
                 (count < (WPW_BASIC_MAX_SIZE - 1)))
           {
               *line_ptr++ = *temp_ptr++;
               count++;
           }
           count = 0;
           *line_ptr = '\0';
#if NU_WEBSERV_DEBUG
       printf("total_char = %s\r\n", total_char);
#endif
        }
    }

    if(temp_ptr)
    {
        /* Decode user id and password */
        BSC_Base64_Decode((UINT8*)total_char, (UINT8*)final_decode);

#if NU_WEBSERV_DEBUG
        printf(" final decode = %s \r\n", final_decode);
#endif

        user_id = final_decode;
        for(; (final_decode[count] != ':') && (count < (WPW_BASIC_MAX_SIZE - 1)); count++);

        /*  Increment the count past the : marker */
        if(count < WPW_BASIC_MAX_SIZE)
        {
            final_decode[count++] = 0;
            password = &final_decode[count];

#if NU_WEBSERV_DEBUG
            printf("user_id = %s password = %s \r\n",user_id, password);
#endif
            count = (INT)BSC_Verify_User(user_id, password);
        }
        else
            count = 0;
    }

    /*  If not found then send try again */
    if(!temp_ptr || !count)
    {
        /*  Send the 401 Response message */
        strcpy(buf, BSC_Text_401);
        strcat(buf, req->ws_fname);
        strcat(buf, BSC_Text_401c);
        len = strlen(buf);
        if(WSN_Write_To_Net(req, buf, len, WS_FILETRNSFR) != NU_SUCCESS)
            NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
        
        return(WPW_BASIC_AUTH_FAILED);
    }
    else
    {
        /*  If authenticated continue processing */
        return(NU_SUCCESS);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Base64_Decode                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function is used to decode a base64 encoded string.  It     
*       returns the decoded string to the calling routine. 
*                     
* INPUTS
*
*       *total_char             Encoded string
*       *final_decode           Location of decoded string.
*
* OUTPUTS
*          
*       none
*                                                             
************************************************************************/
VOID BSC_Base64_Decode(UINT8 *total_char, UINT8 final_decode[])
{
    INT16           index = 0;
    INT16           len;
    INT16           end_text = 0;
    INT16           decode_counter;
    INT16           count;
    UINT8           decode_value[WPW_BASIC_MAX_SIZE];
    UINT8           tempbits;
    UINT8           ch;

    len = (INT16)strlen((CHAR *)total_char);

    while((index < len))
    {
        ch= total_char[index];
        if ((ch >= 'A') && (ch <= 'Z'))
             ch= ch- 'A';

        else if((ch >='a') && (ch <= 'z'))
                 ch= ch -'a' +26;

        else if ((ch >='0') && (ch <= '9'))
                  ch = ch - '0' +52;

        else if (ch == '+')
                 ch = 62;

        else if (ch == '=')
        {
            end_text++;
            ch='\0';
        }

        else if (ch == '/')
             ch = 63;

        decode_value[index] = ch;
        index++;
    }

    index = index - end_text;
    count = 0;
    decode_counter = 0;

    while(count < index)
    {
        final_decode[decode_counter] = (UINT8)(decode_value[count] << 2);
        tempbits = (UINT8)((decode_value[count + 1] & 0x30) >> 4);
        final_decode[decode_counter] = final_decode[decode_counter] | tempbits;
        decode_counter++;
        final_decode[decode_counter] = (UINT8)((decode_value[count + 1] & 0x0f) << 4);
        tempbits = (UINT8)((decode_value[count + 2] & 0x3c) >> 2);
        final_decode[decode_counter] = final_decode[decode_counter] | tempbits;
        decode_counter++;
        final_decode[decode_counter] = (UINT8)((decode_value[count + 2] & 0x03) << 6);
        final_decode[decode_counter] = final_decode[decode_counter] | (decode_value[count +3]);
        decode_counter++;
        count = count + 4;
    }

    final_decode[decode_counter] = 0;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Add_Delete_Auth                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       A plug-in used to add and delete users and passwords to and from 
*       the Nucleus WebServ's database.  This plug-in will redirect the  
*       URL to user.ssi if it is successful. If the plug-in is           
*       unsuccessful it will give an error message with a tag to return  
*       it to the addel.htm web page.                                                  
*
* INPUTS                                                 
*
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*
* OUTPUTS
*
*       WS_REQ_PROCEED
*                                                                       
************************************************************************/
STATIC INT BSC_Add_Delete_Auth(WS_REQUEST *req)
{
    CHAR        buf[200];
    CHAR        *user;
    CHAR        *password;
    INT         add_func = 0;
    INT         del_func = 0;
    CHAR        *pg_string;
    INT16       status = WS_FAILURE;

    /*  Get the user_id */
    user = HTTP_Token_Value_by_Name("user_id", req);

    /*  Get the password */
    password = HTTP_Token_Value_by_Name("password", req);

    /*  Get the function to be performed  */
    pg_string = HTTP_Token_Value_by_Name("AddDel", req);

    if(!user || !password || !pg_string)
        return(WS_REQ_ABORTED);

    /*  Verify it is an add or delete function */
    if(pg_string)
    {
        /*  Check if it is an add */
        if(strncmp(pg_string, "Add", 3) == 0)
        {
            /* Then set the add_func to true */
            add_func = 1;
        }
        /*  Check if it is a delete function */
        else if(strncmp(pg_string, "Delete", 6) == 0)
        {
            /*  Then set the delete to a true */
            del_func = 1;
        }
    }

    /*  This is setting up a page for the response */
    strcpy(buf,"<BR><FONT COLOR=\"#980040\" SIZE=4>\n");

    if(add_func)
    {  
        /*  If add function  then add the entry */
        status = BSC_Auth_Add_Entry(user, password);
        /* Check for failure */
        if(status != NU_SUCCESS)            
            strcat(buf,"User id and password already exists.<BR></FONT>");
    }
    else if(del_func)
    {
        /*  If the user selected Delete then try add delete the user and the password */
        status = BSC_Auth_Delete_Entry(user,password);
        if(status != NU_SUCCESS)            
            strcat(buf,"Unable to find user, password combination.<BR></FONT>");
    }
    else
        strcat(buf,"Illegal POST parameter.<BR></FONT>");
    

    if(status == NU_SUCCESS)
        /* The operation was successful */
        HTTP_Redirect_Client(req,"/user.ssi");
    else
    {
        strcat(buf,"<p align=\"left\"><a href=\"addel.htm\"><fontsize=\"3\">Back to Add and Deleting</font></a></p>");
        HTTP_Send_Status_Message(req, WS_PROTO_OK, buf);
    }
    
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       BSC_Show_Users                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This plug-in is a server side include function that shows all the
*       user id's and password that are in the Nucleus WebServ's         
*       database.                                                        
*        
* INPUTS
*
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*
* OUTPUTS
*
*       WS_REQ_PROCEED                                                    
*                                                                       
************************************************************************/
STATIC INT BSC_Show_Users(WS_REQUEST *req)
{
    UINT16          len;
    INT             status = WS_REQ_PROCEED;

#ifdef NET_5_1
    CHAR            ubuf[320 + UM_MAX_NAME_SIZE + UM_MAX_PW_SIZE];
    CHAR            password[UM_MAX_PW_SIZE];
    UM_USER         user;
#else
    CHAR            ubuf[320 + WS_AUTH_NAME_LEN + WS_AUTH_NAME_LEN];
    CHAR            password[WS_AUTH_NAME_LEN];
    WPW_INFO_NODE   *bpwlist;
#endif

    /*  Set up table for HTMLMarkup  */
    strcpy(ubuf,"<div align=\"center\"><center>");
    strcat(ubuf,"<table border=\"4\" cellpadding=\"2\" width=\"40%%\">");
    
    /*  Set up the table columns and headers for the SSI User ID/ Password */
    strcat(ubuf,"<tr><td align=\"center\" width=\"20%%\"> \
               <FONT COLOR=\"black\"> USER ID </font></td> \
               <td align=\"center\" width=\"20%%\">\
               <FONT COLOR=\"black\">PASSWORD</font></td></tr>");
    
    if(WSN_Write_To_Net(req, ubuf, (UINT32)strlen(ubuf), WS_PLUGIN_DATA) != NU_SUCCESS)
        status = WS_REQ_ABORTED;
    
    /*  Traverse through Password/user list structure to see if user is verified. */
    if(status == WS_REQ_PROCEED)
    {
#ifdef NET_5_1

        /* If there is at least one user in the list, send it */
        if (UM_Find_User_First(&user, UM_WEB) == NU_SUCCESS)
        {
            do
            {
                len = (UINT16)strlen(user.um_pw);
#else
        for(bpwlist = BSC_Pw_List_info.wpw_list_head;
            bpwlist && status == WS_REQ_PROCEED;
            bpwlist = bpwlist->wpw_list_next)
        {
            len = (UINT16)strlen((CHAR *)bpwlist->wpw_password);
#endif
                UTL_Zero(password, WS_AUTH_NAME_LEN);
                memset(password, 0x2a, (unsigned int)len);
        
                /*  Print to ubuf  the user id and password name  */
                strcpy(ubuf,"<tr><td align=\"center\" width=\"20%%\"><FONT COLOR=\"#980040\">");
#ifdef NET_5_1
                strcat(ubuf, user.um_name);
#else
            strcat(ubuf, bpwlist->wpw_user);
#endif
                strcat(ubuf, "</font> </td>");
                strcat(ubuf, "<td align=\"center\" width=\"20%%\"><FONT COLOR=\"#980040\">");
                strcat(ubuf, password);
                strcat(ubuf, "</font></td></tr>");
        
                /*  Output the user id and statistics   */
                if(WSN_Write_To_Net(req, ubuf, (UINT32)strlen(ubuf), WS_PLUGIN_DATA) != NU_SUCCESS)
                    status = WS_REQ_ABORTED;
            }
#ifdef NET_5_1
            while(UM_Find_User_Next(user.um_name, &user, UM_WEB) == NU_SUCCESS);
        }
#endif
    }

    /*  Make end of table HTML and output */
    if(status == WS_REQ_PROCEED)
    {
        strcpy(ubuf,"</table></center></div>");
        if(WSN_Write_To_Net(req, ubuf, (UINT32)strlen(ubuf), WS_PLUGIN_DATA) != NU_SUCCESS)
            status = WS_REQ_ABORTED;
    }

    /* return to Request Proceed */
    return(status);
}
#endif /* INCLUDE_BASIC_AUTH */
