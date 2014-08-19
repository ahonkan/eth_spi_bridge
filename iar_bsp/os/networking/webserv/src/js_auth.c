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
*       js_auth.c                                                 
*                                                                       
* COMPONENT                                                             
*      
*       Nucleus WebServ                                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Javascript authentication places a script on the client
*       side that is used to alter a cookie id value.  This value
*       is altered in a way that is predictable to the server.
*       When the server receives the id, if it matches it's 
*       predictions, the client is authenticated.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*                                                                
* FUNCTIONS                                                      
*                                                                
*       JS_Init_Auth        Initializes javascript authentication
*       JS_Login            Plugin to log user into server
*       JS_Logout           Plugin to log user out of server
*       JS_Auth_Check       Check if user is currently authencticated
*       JS_Get_User_Auth    Gets the auth structure associated with
*                           the current user
*       JS_Free_Auth        Removes user from currently authenticated
*                           list
*       JS_Check_Timeout    Checks authenticated users against their
*                           timeout value
*       JS_Add_User         Adds a user to user/password list
*       JS_Delete_User      Deletes a user from user/password list
*       JS_Send_Script      Plugin to send the encrypted javascript
*                           to the client.
*       JS_Alter_ID_Script  Sends the javascript to alter the id on
*                           the client side
*       JS_Encrypt_Script   Encrypts javascript and attaches the
*                           javascript that decodes
*       JS_Replace_Chars    Replaces control characters in buffer
*       JS_Index_Of         Returns the array index of the
*                           character queried
*       JS_Insert_String    Inserts a string at the begining of
*                           another string
*       JS_Add_Delete_Auth  Plugin to add and delete users
*       JS_Show_Users       Creates a table of users and passwords
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h 
*       wpw_auth.h
*       netevent_group.h
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"

#if INCLUDE_JS_AUTH
#include "networking/wpw_auth.h"
#include "networking/netevent_group.h"


STATIC INT JS_Login(WS_REQUEST *req);
STATIC INT JS_Logout(WS_REQUEST *req);
STATIC VOID JS_Free_Auth(WS_AUTH *auth_struct);
STATIC WS_AUTH *JS_Get_User_Auth(WS_REQUEST *req);
STATIC INT32 JS_Index_Of(CHAR target, CHAR *string_buf);
STATIC STATUS JS_Replace_Chars(CHAR HUGE *string_buf, UINT16 arr_size);
STATIC VOID JS_Insert_String(CHAR *insert_str, CHAR *string_buf);
STATIC INT JS_Add_Delete_Auth(WS_REQUEST *req);
STATIC INT JS_Show_Users(WS_REQUEST *req);

#if !(defined NET_5_1)
extern struct WPW_AUTH_NODE WPW_Table[];
extern WS_SERVER WS_Master_Server;
WPW_INFO_LIST   JS_Pw_List_Info;
static NU_PROTECT JS_Pw_List_Protect;
#endif

/* Login failure */
static const CHAR JS_Failure_Text[]="\
<HEAD><TITLE>Login Failed</TITLE></HEAD>\r\n\
<BODY><H1>Login Failed</H1><BODY>";

/* Maximum users */
static const CHAR JS_Max_Users_Text[]="\
<HEAD><TITLE>Max Users</TITLE></HEAD>\r\n\
<BODY><H1>Max Users</H1>\r\n\
<P>The maximum amount of users are logged into the server at this time. \
Please try again later.<BODY>";

static const CHAR JS_Auth_Screen[]="\
<html>\n\
<head>\n\
<title>Authenticated</title>\n\
</head>\n\n\
<body bgcolor=\"#FFFFFF\" text=\"#3366BB\" leftmargin=\"0\" topmargin=\"0\" onUnload=\"logout()\">\n\
<table width=\"200\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\">\n\
<tr>\n<td align=\"center\" valign=\"center\" height=\"150\">\n\
<img src=\"a.gif\">\n</td>\n</tr>\n\
<tr>\n<td align=\"center\">\n\
<font size=\"3\">Closing this window will log you out of the server.</font>\n\
</td>\n</tr>\n</table>\n\
<form name=\"logout\" method=\"post\" action=\"logout\"></form>\n\
</body>\n</html>";

static const CHAR JS_Code_Start[]="\n\
<script language=\"JavaScript\">\n\
<!--\n\
var key = 1;\n\
function GetCookie(name) {\n\
  var allcookies = document.cookie;\n\
  name = name + \"=\";\n\
  var pos = allcookies.indexOf(name);\n\
  if(pos != -1) {\n\
    var start = pos + name.length;\n\
    var end = allcookies.indexOf(\";\", start);\n\
    if(end == -1)\n\
      end = allcookies.length;\n\
    var value = allcookies.substring(start, end);\n\
    value = unescape(value);\n\
    return value;\n\
  }\n\
  return null;\n\
}\n\n\
function AlterID(){\n\
b=\"";

static const CHAR JS_Code_Middle[]="\
\";\n\
a=\"";

static const CHAR JS_Code_End[]="\
\";\n\
  key = key.toString(10);\n\
  if (key) {\n\
    fin = \"\";\n\
    pos = 0;\n\
    for (i=0;i<b.length;i++) {\n\
      first = b.charAt(i);\n\
      second = key.charAt(pos);\n\
      fin+=a.charAt((a.indexOf(first)\n\
        -a.indexOf(second)+a.length)%a.length);\n\
      pos = (pos+1)%key.length;\n\
    }\n\
  eval(fin);\n\
  fin=a=b=\"\";\n\
  }\n\
}\n\n\
function logout(){\n\
  document.logout.submit();\n\
  AlterID();\n\
}\n\
// -->\n\
</script>";

static const CHAR JS_Code_Script[]="\
function SetCookie (name, value) {\r\n\
  var expires = new Date();\r\n\
  expires.setTime(expires.getTime() + (60 * 1000) * 2 );\r\n\
  document.cookie = name + \"=\" + escape (value) +\r\n\
      \"; expires=\" + expires.toGMTString();\r\n\
}\r\n\
\r\n\
  var id = GetCookie(\'id\');\r\n\
  var user = GetCookie(\'user\');\r\n\
  var newid;\r\n\
  if(key != null && id != null){\r\n\
    newid = (id * key) % "WS_AUTH_MAX_KEY_STR";\r\n\
    SetCookie(\'user\', user);\r\n\
    SetCookie(\'id\', newid.toString(10));\r\n\
  }\r\n";

const CHAR JS_Alter_ID[]="\
<script>\n\
<!--\n\
  var win=window.open(\"\", \"authwin\", \"width=250,height=200\");\n\
  win.AlterID();\n\
// -->\n\
</script>\n";

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Init_Auth                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Initializes the javascript authentication process by setting
*       the timeout value, and initializing the user list.                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       none   
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       status          Either NU_SUCCESS or WS_FAILURE
*                                                                       
*************************************************************************/

STATUS JS_Init_Auth(WS_SERVER *server_struct)
{
    STATUS          status = NU_SUCCESS;
    WS_AUTH         *auth_struct;
    UINT16          index;
    
    /* Get the array of currently authenticated users */
    auth_struct = &server_struct->ws_master.ws_user_auth[0];

    /* Clear out list of currently authenticated users */
    for(index = 0; index < WS_AUTH_MAX_USERS; index++)
        auth_struct[index].ws_user[0] = NU_NULL;

    /* Set non-active user time out value */
    server_struct->ws_master.ws_timeoutval = WS_AUTH_TIMEOUT;

    /* Set auth callback function pointer */
    server_struct->ws_master.ws_callback = NU_NULL;
    
    /* Register the jsauth plugins with the server */
    WS_Register_Plugin(JS_Login, "login", NU_NULL);
    WS_Register_Plugin(JS_Logout, "logout", NU_NULL);
    WS_Register_Plugin(JS_Show_Users, "userp", NU_NULL);
    WS_Register_Plugin(JS_Add_Delete_Auth, "addelu", NU_NULL);
    
#if !(defined NET_5_1)
    /* Initialize the password list */
    JS_Pw_List_Info.wpw_list_head = NU_NULL;
    JS_Pw_List_Info.wpw_list_tail = NU_NULL;
    UTL_Zero(&JS_Pw_List_Protect, sizeof(NU_PROTECT));

    /*  Initailize Linked List for password structure */
    for (index = 0; status == NU_SUCCESS && 
         WPW_Table[index].wpw_user_id[0]; index++)
    {
        status = JS_Add_User(WPW_Table[index].wpw_user_id,
                                    WPW_Table[index].wpw_password);
    }
#endif

    /* Set the default uri's used during authentication */
    strcpy(server_struct->ws_master.ws_auth_uri, (CHAR*)WS_AUTH_SCREEN_URI);
    strcpy(server_struct->ws_master.ws_default_uri, (CHAR*)WS_AUTH_DEFAULT_URI);
    
    return (status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Login                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This plugin is called during the login proccess.  
*       It first determines if the user is valid.  It then places
*       the user in the currently authenticated list, along with
*       password and id value.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       n\a             Always returns WS_REQ_PROCEED
*                                                                       
*************************************************************************/

STATIC INT JS_Login(WS_REQUEST *req)
{
    CHAR            *user;
    CHAR            *id;
    CHAR HUGE       *key = NU_NULL;
    WS_AUTH         *auth_struct;
    UINT16          index;
    INT             status = WS_REQ_ABORTED;
#ifdef NET_5_1
    UM_USER         user_struct;
#else
    WPW_INFO_NODE   *pwlist;
#endif

    /* Get pointers to the cookies provided by client */
    user = HTTP_Cookie_Value_by_Name(WS_AUTH_COOKIE_USER, req);
    id = HTTP_Cookie_Value_by_Name(WS_AUTH_COOKIE_ID, req);

    /* If the cookie exists, try and authenticate */
    if(user)
    {
#ifdef NET_5_1
        if(UM_Find_User(user, &user_struct) == NU_SUCCESS)
            key = user_struct.um_pw;
#else
        /* Traverse through Pw list structure to see if user is verifiable. */
        for(pwlist = JS_Pw_List_Info.wpw_list_head; pwlist && !key;
                    pwlist = pwlist->wpw_list_next)
        {
            if(strcmp(pwlist->wpw_user, user) == 0 )
            {
                key = pwlist->wpw_password;
            }
        }
#endif

        /* If the user was not found, send a failure message */          
        if(!key)
            HTTP_Send_Status_Message(req, WS_PROTO_OK, (CHAR*)JS_Failure_Text);
        else
        {
            /* Find out if user is already authenticated */
            auth_struct = req->ws_server->ws_master.ws_user_auth;
            
            for(index = 0; strlen(auth_struct[index].ws_user) != 0 &&
                index < WS_AUTH_MAX_USERS; index++);
            
            /* Was there an empty slot? */
            if(index == WS_AUTH_MAX_USERS)
                /* If all slots are filled, send max users message */
                HTTP_Send_Status_Message(req, WS_PROTO_OK, (CHAR*)JS_Max_Users_Text);
            else
            {
                /* Otherwise add user to authenticated list */
                strcpy(auth_struct[index].ws_user, user);
                auth_struct[index].ws_id = NU_ATOL(id);
                auth_struct[index].ws_countdown = WS_AUTH_TIMEOUT;
                
                /* Add ascii values of password together to get key */
                while(key[0])
                {
                    auth_struct[index].ws_key += (UINT16)key[0];
                    key++;
                }
                
                if(req->ws_server->ws_master.ws_callback)
                    (req->ws_server->ws_master.ws_callback)(auth_struct);

                JS_Send_Script(req);
            }
        }
        
        status = WS_REQ_PROCEED;
    }

    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Logout                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Plugin to logout the user.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       WS_REQ_PROCEED
*                                                                       
*************************************************************************/

STATIC INT JS_Logout(WS_REQUEST *req)
{
    WS_AUTH     *auth_struct;
    
    /* Get the user's auth information */
    auth_struct = JS_Get_User_Auth(req);

    /* If the user exists, free the slot used */
    if(auth_struct)
        JS_Free_Auth(auth_struct);

    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Auth_Check                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       It is checked if the user is in the list of currently
*       authenticated users.  If so, the timeout value is reset.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS      If the user is currently authenticated
*       WS_FAILURE      If the user is not authenticated
*                                                                       
*************************************************************************/

STATUS JS_Auth_Check(WS_REQUEST *req)
{
    WS_AUTH     *auth_struct;

    /* Get the user's auth information */
    auth_struct = JS_Get_User_Auth(req);

    /* Is user currently authenticated? */
    if(auth_struct)
    {
        /* Reset timeout and return success */
        auth_struct->ws_countdown = WS_AUTH_TIMEOUT;
        return NU_SUCCESS;
    }

    /* Return failure if not found */
    return WS_FAILURE;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Get_User_Auth                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function gets the user and id cookies and then compares
*       them to the list of authenticated users.  If a match is found,
*       a pointer to the structure holding the user's info is returned.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       *auth_struct    Pointer to user's authentication information
*       NU_NULL         If user was not found
*                                                                       
*************************************************************************/

STATIC WS_AUTH *JS_Get_User_Auth(WS_REQUEST *req)
{
    CHAR        *user;
    CHAR        *id;
    WS_AUTH     *auth_struct;
    INT16       index;

    /* Search for user and id cookie */
    user = HTTP_Cookie_Value_by_Name(WS_AUTH_COOKIE_USER, req);
    id = HTTP_Cookie_Value_by_Name(WS_AUTH_COOKIE_ID, req);
    
    /* If cookie exists, find user in authenticated list */
    if(user)
    {
        /* Get a pointer to the currently authenticated array */
        auth_struct = req->ws_server->ws_master.ws_user_auth;

        /* Loop through array looking for a match */
        for(index = 0; index < WS_AUTH_MAX_USERS; index++)
        {
            /* Check user name */
            if(strcmp(auth_struct[index].ws_user, user) == 0)
            {
                /* Check id */
                if(auth_struct[index].ws_id == (UINT32)NU_ATOL(id))
                    return(&(auth_struct[index]));
            }
        }
    }

    return(NU_NULL);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Free_Auth                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       To free an entry in the auth list, each value is set to 0.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *auth_struct    Pointer to user's authentication information
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

STATIC VOID JS_Free_Auth(WS_AUTH *auth_struct)
{
    auth_struct->ws_user[0] = NU_NULL;
    auth_struct->ws_id = 0;
    auth_struct->ws_key = 0;
    auth_struct->ws_client_data = NU_NULL;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Check_Timeout                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When called, this function loops through all authenticated
*       users and updates their timeout clock.  If it has been
*       expired, the user is removed from the list.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

VOID JS_Check_Timeout(WS_REQUEST *req)
{
    UINT32              curtime, delta;
    WS_AUTH             *auth_user;
    UINT16              index;

    /* Get the current time in seconds */
    curtime = WS_Get_Seconds;                              

    /* The number of seconds elapsed */
    if(curtime < req->ws_server->ws_master.ws_last_time)
    {
        delta = curtime + (0xfffffffful/TICKS_PER_SECOND - 
                            req->ws_server->ws_master.ws_last_time);
    }
    else
        delta = curtime - req->ws_server->ws_master.ws_last_time;

    /* Get a pointer to the list of currently authenticated users */
    auth_user = req->ws_server->ws_master.ws_user_auth;

    /* Check each slot in the list for timeout */
    for(index = 0; index < WS_AUTH_MAX_USERS; index++)
    {
        if((auth_user->ws_countdown) && auth_user->ws_user[0])
        {
            /* If the user is timed out, free structure.
             * If not, decrement timeout value by time elapsed.
             */
            if(delta >= auth_user->ws_countdown)
                JS_Free_Auth(auth_user);
            else
                auth_user->ws_countdown -= delta;
        }

        auth_user++;
    }

    /* Remember for next time */
    req->ws_server->ws_master.ws_last_time = curtime;     
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Add_User                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function adds the user and password to the global list
*       of possible users to authenticate.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *user           User name to add to global list of accepted
*                       users.            
*       *password       Password that accompanies the user name.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS      If user was added to the list.
*       NU_MEM_ALLOC    If there is not enough memory to add user.
*       WS_FAILURE      If user is already in the list.      
*                                                                       
*************************************************************************/

STATUS JS_Add_User(CHAR *user, CHAR *password)
{
#ifdef NET_5_1
    return (UM_Add_User(user, password, UM_WEB, NU_NULL));
#else
    INT16           found = 0;
    WPW_INFO_NODE   *pwlist;
    WPW_INFO_NODE   *pwnode;

    /*  Allocate Memory for the new database entry   */
    if (NU_Allocate_Memory (WS_Master_Server.ws_memory_pool, (VOID **)&pwnode,
                            sizeof(WPW_INFO_NODE), NU_NO_SUSPEND) != NU_SUCCESS)
        return(NU_MEM_ALLOC);
    
    /* Protect against multiple threads accessing list */
    NU_Protect(&JS_Pw_List_Protect);

    /* First Verify that the user does not exist */
    for(pwlist = JS_Pw_List_Info.wpw_list_head; pwlist && !found;
                                    pwlist = pwlist->wpw_list_next)
    {
        if(strcmp(pwlist->wpw_user, user) == 0 )
            if(strcmp(pwlist->wpw_password, password) == 0) 
                found++;
    }

    if(found || strlen(user) > WS_AUTH_NAME_LEN || strlen(password) > WS_AUTH_NAME_LEN)
    {
        /* Relinquish protection */
        NU_Unprotect();

        if(NU_Deallocate_Memory(pwnode) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

        return(WS_FAILURE);
    }
    
    /* Setup the New User id to add */
    strcpy(pwnode->wpw_user, user);

    /* Setup the password name */
    strcpy(pwnode->wpw_password, password);

    /* Add this host to the list. */
    DLL_Enqueue(&JS_Pw_List_Info, pwnode);

    /* Relinquish protection */
    NU_Unprotect();

    return(NU_SUCCESS);
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Delete_User                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function removes the user from the global list
*       of possible authenticated users.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *user           User name to delete from global list of accepted
*                       users.            
*       *password       Password that accompanies the user name.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS      If user was removed from the list.
*       WS_FAILURE      If user is was not in the list.      
*                                                                       
*************************************************************************/

STATUS JS_Delete_User(CHAR *user, CHAR *password)
{
    STATUS          status = NU_SUCCESS;

#ifdef NET_5_1
    status = UM_Validate_User(user, password, UM_WEB);
    if(status == NU_SUCCESS)
        status = UM_Del_User(user);
#else
    WPW_INFO_NODE   *pwlist;
    INT             found = 0;

    /* Protect against multiple threads accessing list */
    NU_Protect(&JS_Pw_List_Protect);
    
    pwlist = JS_Pw_List_Info.wpw_list_head;

    /*  Searches to see if the name being asked to delete is availble  */
    while(pwlist && !found)
    {
        if((strcmp(pwlist->wpw_user, user) == 0) &&
           (strcmp(pwlist->wpw_password, password) == 0))
                found++;
        else
            pwlist = pwlist->wpw_list_next;
    }

    if(found)
    {
        /*  If entry is found remove it from the list and delete it. */
        DLL_Remove(&JS_Pw_List_Info, pwlist);

        /* Relinquish protection */
        NU_Unprotect();
        
        if(NU_Deallocate_Memory(pwlist) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
    }
    else
    {
        /* Relinquish protection */
        NU_Unprotect();
        
        status = WS_FAILURE;
    }
#endif

    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Send_Script                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This plugin sends the encoded javascript to the client.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

INT JS_Send_Script(WS_REQUEST *req)
{
    WS_AUTH     *auth_struct;
    CHAR        *buffer;
    STATUS      status = WS_REQ_ABORTED;

    /* Get the user's auth structure */
    auth_struct = JS_Get_User_Auth(req);

    if(auth_struct)
    {
        status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID **)&buffer,
                                    WS_AUTH_SCRIPT_SZ, NU_NO_SUSPEND);
        
        if(status == NU_SUCCESS)
        {
            /* Invoke the javascript encryption process using the user's
             * key for the encoding process.
             */
            JS_Encrypt_Script(auth_struct->ws_key, JS_Code_Script, buffer);

            /* Send the encrypted script to the network */
            status = WSN_Write_To_Net(req, (CHAR*)JS_Auth_Screen, 
                                      (UINT32)strlen(JS_Auth_Screen), WS_PLUGIN_DATA);
            if(status == NU_SUCCESS)
                status = WSN_Write_To_Net(req, buffer, (UINT32)strlen(buffer), WS_PLUGIN_DATA);

            if(status == NU_SUCCESS)
            {
                /* Prevent caching of this file */
                HTTP_Header_Name_Insert(req, WS_PRAGMA, "no-cache");
            
                HTTP_Set_Cookie(req, "url", req->ws_server->ws_master.ws_default_uri);
                status = WSN_Write_To_Net(req, NU_NULL, 0, WS_PLUGIN_SEND);
            }
            else
                NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
       
            /* Update the user's id to reflect the value the server will
             * expect next.
             */
            auth_struct->ws_id = (auth_struct->ws_id * auth_struct->ws_key)
                                    % WS_AUTH_MAX_KEY;

            NU_Deallocate_Memory(buffer);
        }

        if(status == NU_SUCCESS)
            status = WS_REQ_PROCEED;
        else
            status = WS_REQ_ABORTED;

    }

    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Alter_ID_Script                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When mode is present, this function sends the call to
*       alter the ID cookie.  If mode is 0, then this function
*       returns the size of the script.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.
*       mode            The mode that will be used to send the data.
*                       Either WS_FILETRNSFR or WS_PLUGIN_DATA.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       size            This is the size of the ecoded script.
*       0               If script was attempted to be sent, but
*                       client is not authorized.
*                                                                       
*************************************************************************/

UINT16 JS_Alter_ID_Script(WS_REQUEST *req, UINT8 mode)
{
    WS_AUTH     *auth_struct;
    static UINT16 size = 0;

    /* Set the size of the script only once to keep from
     * making unnecessary redundant calls
     */
    if(!size)
        size = (UINT16)strlen(JS_Alter_ID);
    
    if(req)
    {
        /* Get the user's auth structure */
        auth_struct = JS_Get_User_Auth(req);
        
        if(auth_struct)
        {
            /* Change the ID for the next request */
            auth_struct->ws_id = 
                (auth_struct->ws_id * auth_struct->ws_key)
                % WS_AUTH_MAX_KEY;
            
            if(WSN_Write_To_Net(req, (CHAR*)JS_Alter_ID, (UINT32)size, (INT)mode) != NU_SUCCESS)
                NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
        else
            return(0);
    }

    return(size);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Encrypt_Script                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Takes a javascript and encodes using the key 
*       specified.  The output is a javascript that, through the use
*       of the cookie 'key', can decrypt the original javascript on
*       the browser that has the correct key present.
*                                                                       
* INPUTS                                                                
*                                                                       
*       key             Key value used to encrypt, and later decrypt
*                       the encoded javascript.
*       *script         The actual javascript that will be encoded.
*       *buffer         Where the outgoing data will be placed.
*                       The array should be WS_AUTH_SCRIPT_SZ in size.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

VOID JS_Encrypt_Script(const UINT32 key, const CHAR HUGE *script, CHAR HUGE *buffer)
{
    CHAR        temp[12];
    CHAR HUGE   *key_ptr = NU_ULTOA(key, temp, 10);
    INT         pos = 0;
    INT16       index;
    UINT8       alpha_length;
    UINT8       key_length;
    INT32       script_length;
    UINT16      start_length;
    UINT32      first;
    UINT32      second;
    CHAR HUGE   *fin;

    /* Set up the base alpha array */
    CHAR alpha[128] = "@abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~!#$%^&*():;/.";

    /* Be sure the buffer is cleared of any data */
    UTL_Zero(buffer, WS_AUTH_SCRIPT_SZ);

    /* Get the sizes to the strings */
    alpha_length = strlen(alpha);
    key_length = (UINT8)strlen((CHAR*)key_ptr);
    script_length = strlen((CHAR*)script);
    start_length = (UINT16)strlen(JS_Code_Start);

    /* Check if the script is too large */
    if(script_length > WS_AUTH_SCRIPT_SZ)
        return;
    
    /* Add the begining of the decoding javascript */
    strcpy((CHAR*)buffer, JS_Code_Start);

    /* Set a pointer to the end of the begining 
     *(or is that the begining of the end?) 
     */
    fin = buffer + start_length;

    /* Step through key to verify that all characters used
     * exist within the array alpha
     */
    for(index = 0; index < key_length; index++)
    {
        /* If it doesn't exist, add this character to the array */
        if((JS_Index_Of(key_ptr[index], alpha)) == WS_FAILURE)
        {
            alpha[alpha_length] = key_ptr[index];
            alpha[++alpha_length] = NU_NULL;
            if(alpha_length >= 118)
                break;
        }
    }

    /* Step through script to verify that all characters used
     * exist within the array alpha
     */
    for(index = 0; index < script_length; index++)
    {
        /* If it doesn't exist, add this character to the array */
        if((JS_Index_Of(script[index], alpha)) == WS_FAILURE)
        {
            alpha[alpha_length] = script[index];
            alpha[++alpha_length] = NU_NULL;
            if(alpha_length >= 119)
                break;
        }
    }

    /* Do the actual encoding process */
    for(index = 0; index < script_length; index++)
    {
        first = JS_Index_Of(script[index], alpha);
        second = JS_Index_Of(key_ptr[pos], alpha);
        fin[index] = alpha[(UINT8)((first + second) % alpha_length)];
        pos = (pos + 1) % key_length;
    }

    /* Go through the alpha list and encoded buffer and replace
     * any control characters with their ascii representations
     */
    if(JS_Replace_Chars(alpha, 128) == NU_SUCCESS)
        if(JS_Replace_Chars(fin, WS_AUTH_SCRIPT_SZ) == NU_SUCCESS)
        {
            /* Word wrap the encoded script */  
            for(index = (INT16)strlen((CHAR*)fin) - 80; ((index > 0) && 
                    ((index + start_length) < WS_AUTH_SCRIPT_SZ)); index -= 80)
            {
                while(fin[index - 1] == '\\')
                    index++;
                JS_Insert_String("\"\n+\"", (CHAR*)(&fin[index]));
            }
        }

    if((index + start_length + strlen(JS_Code_Middle) + 
            alpha_length + strlen(JS_Code_End)) < WS_AUTH_SCRIPT_SZ)
    {
        /* Assemble the rest of the decode script around the
         * encoded script
         */
        strcat((CHAR*)fin, JS_Code_Middle);
        strcat((CHAR*)fin, alpha);
        strcat((CHAR*)fin, JS_Code_End);
    }
    else
        UTL_Zero(buffer, WS_AUTH_SCRIPT_SZ);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Replace_Chars                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Steps through the character array and replaces all control
*       characters with the ASCII representation.
*                                                                       
* INPUTS                                                                
*                                                                       
*       string_buf      Array that will be modified.
*       arr_size        Size of the array
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

STATIC STATUS JS_Replace_Chars(CHAR HUGE *string_buf, UINT16 arr_size)
{
    UINT32      index;
    UINT16      count;
    CHAR        new_string[WS_AUTH_SCRIPT_SZ];
    CHAR        temp;

    string_buf = TLS_Normalize_Ptr(string_buf);

    /* Initialize empty string */
    new_string[0] = 0;

    /* Loop through string_buf and copy the equivalent characters
     * to the new_string
     */
    for(index = 0, count = 0; string_buf[index]; index++, count++)
    {
        switch(string_buf[index])
        {
        case '\n':
            /* Replace line feed */
            strcat(new_string, "\\n");
            count++;
            break;
        case '\"':
            /* Replace quotation */
            strcat(new_string, "\\\"");
            count++;
            break;
        case '\r':
            /* Replace carrige return */
            strcat(new_string, "\\r");
            count++;
            break;
        case '\t':
            /* Replace tab */
            strcat(new_string, "\\t");
            count++;
            break;
        case '\\':
            /* Replace control character */
            strcat(new_string, "\\\\");
            count++;
            break;
        default:
            /* Character is ok, copy straight over and null terminate */
            temp =  string_buf[index];
            new_string[count] = temp;
            new_string[count + 1] = NU_NULL;
        }

        if(count >= arr_size)
            return(WS_FAILURE);
    }

    /* Copy the new string into the original */
    strcpy((CHAR*)string_buf, new_string);

    return(NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Index_Of                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Finds the occurance of target within string_buf.  The
*       return value is the offset from the begining of the buffer.
*                                                                       
* INPUTS                                                                
*       
*       target          Character to search for in string_buf.
*       string_buf      Character array to search for target.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       index           Index into the array string_buf at which
*                       target first occurs.
*       WS_FAILURE      If target does not occur within string_buf.
*                                                                       
*************************************************************************/

STATIC INT32 JS_Index_Of(CHAR target, CHAR *string_buf)
{
    CHAR HUGE   *index;
    CHAR        temp_string[2];

    /* Change character target into a string */
    temp_string[0] = target;
    temp_string[1] = NU_NULL;

    /* Search string_buf for the occurance of temp_string */
    index = WS_In_String(temp_string, string_buf);

    /* If it exists, return its offset from the beginin of string_buf */
    if(index) 
        return(index - string_buf);
    else
        return WS_FAILURE;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Insert_String                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Places a string at the begining of the other.
*                                                                       
* INPUTS                                                                
*       
*       *insert_str     String that will be inserted into the existing
*                       string.
*       *string_buf     Existing string that will have new string
*                       inserted at begining.  This string size is
*                       no greater than WS_AUTH_SCRIPT_SZ.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
*************************************************************************/

STATIC VOID JS_Insert_String(CHAR *insert_str, CHAR *string_buf)
{
    CHAR    new_string[WS_AUTH_SCRIPT_SZ];

    /* Initialize empty string */
    new_string[0] = 0;

    /* Copy existing string into temp string */
    strcpy(new_string, string_buf);

    /* Copy the inserting string into the existing string */
    strcpy(string_buf, insert_str);

    /* Copy the temp string after the new string */
    strcat(string_buf, new_string);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       JS_Add_Delete_Auth                                               
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
*       *req
*
* OUTPUTS
*
*       WS_REQ_PROCEED
*                                                                       
************************************************************************/
STATIC INT JS_Add_Delete_Auth(WS_REQUEST *req)
{
    CHAR        buf[200];
    CHAR        *user;
    CHAR        *password;
    INT         add_func = 0;
    INT         del_func = 0;
    CHAR        *pg_string;
    INT16       status = WS_FAILURE;
    
    /*  Check if arguement name is equal to user_id */
    user = HTTP_Token_Value_by_Name("user_id", req);

    /*  Check if the name is eual to password */
    password = HTTP_Token_Value_by_Name("password", req);

    /*  Verify it is an add or delete function */
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
        status = JS_Add_User(user, password);
        /* Check for failure */
        if(status != NU_SUCCESS)            
            strcat(buf,"User id and password already exists.<BR></FONT>");
    }
    else if(del_func)
    {
        /*  If the user selected Delete then try add delete the user and the password */
        status = JS_Delete_User(user,password);
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
*       JS_Show_Users                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This plug-in is a server side include function that shows all the
*       user id's and passwords that are in the Nucleus WebServ's         
*       database.                                                        
*        
* INPUTS
*
*       *req            Pointer to Request structure that holds
*                       all information pertaining to the HTTP request.
*
* OUTPUTS
*
*       WS_REQ_PROCEED                                                    
*                                                                       
************************************************************************/
STATIC INT JS_Show_Users(WS_REQUEST *req)
{
    UINT16           len;
    INT             status = WS_REQ_PROCEED;
#ifdef NET_5_1
    CHAR            ubuf[320 + UM_MAX_NAME_SIZE + UM_MAX_PW_SIZE];
    CHAR            password[UM_MAX_PW_SIZE];
    UM_USER         user;
    UM_Find_User_First(&user, UM_WEB);
#else
    CHAR            ubuf[320 + WS_AUTH_NAME_LEN + WS_AUTH_NAME_LEN];
    CHAR            password[WS_AUTH_NAME_LEN];
    WPW_INFO_NODE   *pwlist;
#endif

    /*  Set up table for HTML Markup  */
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
        do
        {
            len = (UINT16)strlen(user.um_pw);
#else
        for(pwlist = JS_Pw_List_Info.wpw_list_head;
            pwlist && status == WS_REQ_PROCEED;
            pwlist = pwlist->wpw_list_next)
        {
            len = (UINT16)strlen((CHAR *)pwlist->wpw_password);
#endif
            UTL_Zero(password, WS_AUTH_NAME_LEN);
            memset(password, 0x2a, (unsigned int)len);
        
            /*  Print to ubuf  the user id and password name  */
            strcpy(ubuf,"<tr><td align=\"center\" width=\"20%%\"> \
                    <FONT COLOR=\"#980040\">");
#ifdef NET_5_1
            strcat(ubuf, user.um_name);
#else
            strcat(ubuf, pwlist->wpw_user);
#endif
            strcat(ubuf, "</font> </td> \
                      <td align=\"center\" width=\"20%%\"> \
                      <FONT COLOR=\"#980040\">");
            strcat(ubuf, password);
            strcat(ubuf, "</font></td></tr>");
        
            /*  Output the user id and statistics   */
            if(WSN_Write_To_Net(req, ubuf, (UINT32)strlen(ubuf), WS_PLUGIN_DATA) != NU_SUCCESS)
                status = WS_REQ_ABORTED;
        }
#ifdef NET_5_1
            while(UM_Find_User_Next(user.um_name, &user, UM_WEB) == NU_SUCCESS);
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
#endif
