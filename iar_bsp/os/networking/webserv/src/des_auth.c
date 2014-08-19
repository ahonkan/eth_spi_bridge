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
*       des_auth.c                                                
*                                                                       
* COMPONENT                                             
*                
*       Nucleus WebServ                                                                       
*                                                                       
* DESCRIPTION          
*
*       This file links the java application with the actual
*       encryption process.
*                                                                       
* DATA STRUCTURES                                                       
*
*       None                                                                       
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       DES_Auth_Initialize         Initializies DES authentication
*       DES_Rand                    Gets a 32 byte random number
*       DES_Decrypt_Mes             Decrypts string found in header
*       DES_Send_Url                Plugin to verify user info
*       DES_New_Key                 Changes the pass key
*       DES_Auth_Add                Plugin to add user and password
*       DES_Auth_Del                Plugin to remove user and password
*       DES_Auth_Add_Entry          Adds a user entry into global list
*       DES_Auth_Delete_Entry       Removes user entry from global list
*       DES_Send_Auth_Salt          Sends random number to applet
*       DES_Make_New_Salt           Creates a 64 bit random number                                             
*       DES_Authenticated_Check     Checks if user has been
*                                   authenticated.                                
*       DES_Check_Timeout           Checks expiration on authenticated
*                                   users.              
*       DES_Auth_Control            Authentication Control routine.  
*       DES_Check_Auth_IP           Check if the ip address is       
*                                   already authenticated.           
*       DES_Add_Authenticated       Adds an authenticated user                                       
*                                   to the auth structure.           
*       DES_Free_Auth               Frees authentication structure   
*                                   for a particular ip address.     
*       DES_Bin_To_Hex              Convert binary to two hex        
*                                   digits.                          
*       DES_Packed_Hex_To_Bin       Convert packed hex to binary.    
*       DES_Hex_to_Binary           Convert hex to binary.           
*       DES_Ascii_To_Nibble         Converts Ascii Hex nibble to     
*                                   binary.                          
*       DES_Ascii_Hex_To_Bin        Converts a hex digit into binary.
*                                                                       
* DEPENDENCIES                                                          
*                          
*       nu_websrv.h
*       auth.h                                             
*                                                                       
************************************************************************/

#include "networking/nu_websr.h"

#if INCLUDE_DES_AUTH
#include "networking/netevent_group.h"
#include "networking/wpw_auth.h"

STATIC WPW_INFO_NODE *DES_Verify_User(CHAR *user_id, CHAR *password);
STATIC INT         DES_Rand(WS_REQUEST * req);
STATIC INT         DES_Send_Url(WS_REQUEST * req);
STATIC INT         DES_New_Key(WS_REQUEST * req);
STATIC INT         DES_Auth_Add(WS_REQUEST * req);
STATIC INT         DES_Auth_Del(WS_REQUEST * req);
STATIC INT16       DES_Auth_Delete_Entry(CHAR *user_id, CHAR *password);
STATIC INT16       DES_Auth_Add_Entry(CHAR *user_id, CHAR *password);
STATIC VOID        DES_Send_Auth_Salt(WS_REQUEST * req);
STATIC VOID        DES_Make_New_Salt(CHAR * salt);
STATIC VOID        DES_Decrypt_Mes(WS_REQUEST *req, UINT8 *cypher_a, UINT8 *cypher_b);

STATIC INT         DES_Auth_Control(WS_SERVER *server, INT auth_cmd, VOID *auth);
STATIC VOID        DES_Free_Auth(WS_AUTH * a);
STATIC INT         DES_Add_Authenticated(WS_SERVER *server, WS_REQUEST * req);
STATIC WS_AUTH     *DES_Check_Auth_IP(WS_SERVER * server , WS_REQUEST *req);

STATIC INT         DES_Ascii_To_Nibble(UINT8 * s); 
INT                DES_Hex_to_Binary(CHAR * s);
STATIC unsigned int DES_Ascii_Hex_To_Bin(UINT8* s);
STATIC VOID        DES_Bin_To_Hex(CHAR c,CHAR * buf);
STATIC VOID        DES_Packed_Hex_To_Bin(CHAR * d, CHAR * s ,INT count);   

#if !(defined NET_5_1)
extern WS_SERVER WS_Master_Server;
extern struct WPW_AUTH_NODE WPW_Table[];
WPW_INFO_LIST  DES_Pw_List_Info;
#endif

CHAR * DES_Key_String[9] = {"12345678"}; 
INT     DES_Little_Endian = 0;

UINT8 DES_Reg[WPW_SRG_SIZE]={
    0,1,1,0,1,0,0,1,1,0,0,1,1,1,0,1,1,0,0,1,1,1,0,1,0,0,1,1,0,0,1,1,
    0,1,0,1,0,1,1,0,0,0,1,0,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0,0,1,
    1,0,1,1,0,0,1,1,0,1,0,1,0,1,0,1,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,0,
    1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,1,0,0,1,0,0,1,0,1,0,1,0,1,0,0,0};

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Initialize                                                        
*                                                                       
* DESCRIPTION                                                           
*       
*       Initializes DES authentication within the server
*                                                                
* INPUTS                                                                
*                                                                       
*       None.                                                            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                            
*                                                                       
************************************************************************/
VOID DES_Auth_Initialize(WS_SERVER *server)
{
    INT             i;
    WS_AUTH         *auth;

    /* Endian Test */    
    CHAR            list1[4];
    UNSIGNED        cmp_value = 0x10203040ul;
    VOID            *cmp1;
    VOID            *cmp2;

    /* The following tests if this is a big or little endian
     * machine.  The global is set for use in the encryption
     * process.
     */
    list1[0] = (CHAR)0x10;
    list1[1] = (CHAR)0x20;
    list1[2] = (CHAR)0x40;
    list1[3] = (CHAR)0x80;

    cmp1 = list1;
    cmp2 = &cmp_value;

    if(((char *)cmp1)[0] == ((char *)cmp2)[0])
        DES_Little_Endian = NU_FALSE;
    else
        DES_Little_Endian = NU_TRUE;
    /* End endian test */
    
    auth = &server->ws_master.ws_user_auth[0];
    
    for(i=0; i< WS_AUTH_MAX_USERS - 1; i++)
    {
        auth[i].ws_next = &auth[i + 1];
        auth[i].ws_state = WS_AUTH_FREE ;
    }
    
    auth[i].ws_next = NU_NULL;
    auth[i].ws_state= WS_AUTH_FREE;
    
    /* Set server time out */
    server->ws_master.ws_timeoutval = WS_AUTH_TIMEOUT;

    /*  Register All plug-ins used for authentication */
    HTTP_Register_Plugin(DES_Rand, "ps_rand", NU_NULL);
    HTTP_Register_Plugin(DES_Send_Url, "ps_sendurl", NU_NULL);
    HTTP_Register_Plugin(DES_New_Key, "auth_newkey", NU_NULL);
    HTTP_Register_Plugin(DES_Auth_Del, "auth_del", NU_NULL);
    HTTP_Register_Plugin(DES_Auth_Add, "auth_add", NU_NULL);

#if !(defined NET_5_1)
    /*  Initailize Linked List for password structure */
    for (i = 0; WPW_Table[i].wpw_user_id[0]; i++)
    {
        DES_Auth_Add_Entry(WPW_Table[i].wpw_user_id, WPW_Table[i].wpw_password);
    }
#endif

    /*  Add the crypto key to the server */
    DES_Auth_Control(server, WS_A_CREDS, DES_Key_String);

    /*  Add the authentication Universal Resource Locator to login.htm */
    DES_Auth_Control(server, WS_A_AUTH_URI, WS_AUTH_SCREEN_URI);

    strcpy(server->ws_master.ws_default_uri, (CHAR*)WS_AUTH_DEFAULT_URI);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Rand                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This plugin is called in the authentiction process a 32          
*       byte random number is created the random number is               
*       encrypted with the encryption key and sent to the client         
*       as a hex ascii string.                                           
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
STATIC INT DES_Rand(WS_REQUEST *req)
{
    DES_Send_Auth_Salt(req);
    return(WS_REQ_PROCEED);
}


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Decrypt_Mes                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This gets a string from the HTTP header and then decrypts
*       the message with the attached string.  The decrypted 
*       messages are returned in cypher_a and cypher_b.                                           
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.
*       cypher_a                Pointer to a string where the
*                               first decrypted item will be placed
*       cypher_b                Pointer to a string where the
*                               second decrypted item will be placed
*                               If no second item exists this will
*                               be NULL.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none
*                                                                       
************************************************************************/

STATIC VOID DES_Decrypt_Mes(WS_REQUEST *req, UINT8 *cypher_a, UINT8 *cypher_b)
{
    CHAR            *s;
    UINT8           xor_mask[17];
    INT             i;
    
    UTL_Zero(cypher_a, 17);
    if(cypher_b)
        UTL_Zero(cypher_b, 17);
    
    /* get the VALUE string from the POST request */
    s = HTTP_Token_Value_by_Name("VALUE", req);

#if NU_WEBSERV_DEBUG
    printf("VALUE= %s\n",s);
#endif

    /* convert the user ( cypher_a)
     * and the passwd ( cypher_b), if it exists
     * and the mask ( xor_mask) 
     * to binary
     */ 

    DES_Packed_Hex_To_Bin((CHAR *)cypher_a, (CHAR *)s, 32);

    if(cypher_b)
    {
        DES_Packed_Hex_To_Bin((CHAR *)cypher_b, (CHAR *)s + 32, 32);
        DES_Packed_Hex_To_Bin((CHAR *)xor_mask, (CHAR *)s + 64, 32);
    }
    else
        DES_Packed_Hex_To_Bin((CHAR *)xor_mask, (CHAR *)s + 32, 32);
    

#if NU_WEBSERV_DEBUG
    printf("KEY = %s\n", req->ws_server->ws_master.ws_key);
#endif

    /* decrypt each string */

    ENC_Decrypt((CHAR *)req->ws_server->ws_master.ws_key, (CHAR *)xor_mask, 2);
    ENC_Decrypt((CHAR *)req->ws_server->ws_master.ws_key, (CHAR *)cypher_a, 2);

    if(cypher_b)
        ENC_Decrypt((CHAR *)req->ws_server->ws_master.ws_key, (CHAR *)cypher_b, 2);

    /* XOR our decrypted user and passwd with the 
     * decrypted salt
     */

    for(i = 0; i < 16; i++)
    {
        cypher_a[i] ^= xor_mask[i];
        if(cypher_a[i] == ' ')
            cypher_a[i] = 0;

        if(cypher_b)
        {
            cypher_b[i] ^= xor_mask[i];
            if(cypher_b[i] == ' ')
                cypher_b[i] = 0;
        }            
    }

    /* Make sure there is a null */
    cypher_a[i] = 0; 
    if(cypher_b)
        cypher_b[i] = 0;                                 

#if NU_WEBSERV_DEBUG
    printf("string 1:%s\n",cypher_a);
    if(cypher_b)
        printf("string 2:%s\n",cypher_b);
#endif

}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Verify_User                                                
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
STATIC WPW_INFO_NODE *DES_Verify_User(CHAR *user_id, CHAR *password)
{
#ifdef NET_5_1
    if(UM_Validate_User(user_id, password, UM_WEB) == NU_SUCCESS)
        return((WPW_INFO_NODE *)NU_TRUE);
    return(NU_FALSE);
#else
    STATUS          found = NU_FALSE;
    WPW_INFO_NODE   *pwlist_info;
    
    for(pwlist_info = DES_Pw_List_Info.wpw_list_head;
         pwlist_info; pwlist_info = pwlist_info->wpw_list_next)
    {
        if(strcmp(pwlist_info->wpw_user, user_id) == 0 )
        {
            if (strcmp(pwlist_info->wpw_password, password) == 0)
            {
                found = NU_TRUE;
                break;
            }
        }
    }

    return(pwlist_info);
#endif
}
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Send_Url                                                       
*                                                                       
* DESCRIPTION                                                           
*        
*       This function verifies that the user and password are valid
*       within the system, and then forwards the user to either the
*       secure page, or to a failure to login page.                                                        
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
*       WS_REQ_ABORTED          Upon failure to access network
*                                                                       
************************************************************************/
STATIC INT DES_Send_Url(WS_REQUEST * req)
{
    CHAR            url[WS_URL_LEN];
    UINT8           cypher_a[17];
    UINT8           cypher_b[17];
    
    DES_Decrypt_Mes(req, cypher_a, cypher_b);
    if(DES_Verify_User((CHAR*)cypher_a, (CHAR*)cypher_b))
    {
        /* Send to the Java applet the Redirection Ip address. 
         * http://<server IP address>/<secure-page>,
         */

        /*  Add the authenticated IP address */
        DES_Auth_Control(req->ws_server, WS_A_AUTH_ADD, req);

        HTTP_Uri_To_Url(req, req->ws_server->ws_master.ws_default_uri, url);
    }
    else
    {
        /* Send to the Java applet the Redirection Ip address. 
         * http://<server IP address>/failure.htm,
         */
        HTTP_Uri_To_Url(req, "failure.htm", url);
    }

    if(WS_Write_To_Net(req, url, (UINT32)strlen(url), WS_FILETRNSFR) != NU_SUCCESS)
        return(WS_REQ_ABORTED);
    
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_New_Key                                                      
*                                                                       
* DESCRIPTION                                                           
*              
*       Set the new key for authentication process                                                  
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
*       WS_REQ_ABORTED          Upon failure to access network
*                                                                       
************************************************************************/
STATIC INT DES_New_Key(WS_REQUEST *req)
{
    CHAR            url[WS_URL_LEN];
    UINT8           cypher_a[17];   
    CHAR            key[WS_CRYPT_LEN + 1];

    DES_Decrypt_Mes(req, cypher_a, NU_NULL);
    
    strncpy((CHAR *)key, (CHAR *)cypher_a, WS_CRYPT_LEN);
    key[WS_CRYPT_LEN] = 0;
    strcpy((CHAR *)req->ws_server->ws_master.ws_key, (CHAR *)key);

    /* Send Response to redirect Applet to a specific WebPage 
     * http://<server IP address>/nkey.htm,
     */
    HTTP_Uri_To_Url(req, "nkey.htm", url);

    if(WS_Write_To_Net(req, url, (UINT32)strlen(url), WS_FILETRNSFR) != NU_SUCCESS)
        return(WS_REQ_ABORTED);
    
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Add                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Adds an Authentication User ID and Password to the Doubly Linked 
*       List.  It redirects the browser to an IP address of whether
*       it was successful or not                                                 
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
*       WS_REQ_ABORTED          Upon failure to access network
*                                                                       
************************************************************************/
STATIC INT DES_Auth_Add(WS_REQUEST *req)
{   
    CHAR            url[WS_URL_LEN];
    UINT8           cypher_a[17];
    UINT8           cypher_b[17];
    
    DES_Decrypt_Mes(req, cypher_a, cypher_b);
    
    if(DES_Auth_Add_Entry((CHAR *)cypher_a,(CHAR *)cypher_b) != NU_SUCCESS)
    {
        /* If the add entry fails then send a response. 
         * http://<server IP address>/auserf.htm" 
         */
        HTTP_Uri_To_Url(req, "auserf.htm", url);
    }
    else
    {  
        /* If successful then redirect the URL to 
         * http://<server IP address>/addus.htm"
         */
        HTTP_Uri_To_Url(req, "addus.htm", url);
    }

    if(WS_Write_To_Net(req, url, (UINT32)strlen(url), WS_FILETRNSFR) != NU_SUCCESS)
        return(WS_REQ_ABORTED);
    
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Del                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Routine to Delete an Authentication entry from the Doubly link   
*       list.  It returns to the Java Applet an ip address to redirect   
*       the Browser to.                                                  
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS
*                                                                       
************************************************************************/
STATIC INT DES_Auth_Del(WS_REQUEST *req)
{   
    CHAR            url[WS_URL_LEN];
    UINT8           cypher_a[17];
    UINT8           cypher_b[17];   

    DES_Decrypt_Mes(req, cypher_a, cypher_b);

    if(DES_Auth_Delete_Entry((CHAR *)cypher_a,(CHAR *)cypher_b) != NU_SUCCESS)
    {
        /* If the add entry fails then send a response. 
         * http://<server IP address>/duserf.htm" 
         */
        HTTP_Uri_To_Url(req, "duserf.htm", url);
    }
    else
    {  
        /* If successful then redirect the URL to 
         * http://<server IP address>/delus.htm"
         */
        HTTP_Uri_To_Url(req, "delus.htm", url);
    }

    if(WS_Write_To_Net(req, url, (UINT32)strlen(url), WS_FILETRNSFR) != NU_SUCCESS)
        return(WS_REQ_ABORTED);
    
    return(WS_REQ_PROCEED);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Add_Entry                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function dynamically adds a user id and password combination
*       into the Nucleus WebServ's database.                             
*                                             
* INPUTS
*
*       *user_id            Pointer to the user's id
*       *password           Pointer to the associated password
*
* OUTPUTS
*
*       -1                  The user already exists
*       NU_MEM_ALLOC        Could not allocate memory 
*       NU_SUCCESS          Success
*                          
************************************************************************/
STATIC INT16 DES_Auth_Add_Entry(CHAR *user_id, CHAR *password)
{
#ifdef NET_5_1
    return((INT16)UM_Add_User(user_id, password, UM_WEB, NU_NULL));
#else

    WPW_INFO_NODE   *apwlist_info;

   /*  First Verify that the user does not exist */
    if(DES_Verify_User(user_id, password))
       return(-1);
    
    /*  Allocate Memory for the new database entry   */
    if (NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, (VOID **)&apwlist_info,
                           sizeof(WPW_INFO_NODE), NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
        return (NU_MEM_ALLOC);
    }

    /* Setup the New User id to add */
    strcpy(apwlist_info->wpw_user, user_id);

    /* Setup the password name */
    strcpy(apwlist_info->wpw_password ,password);

    /* Add this host to the list. */
    DLL_Enqueue(&DES_Pw_List_Info, apwlist_info);

    return(NU_SUCCESS);
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Delete_Entry                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function is used to dynamically delete a combination        
*       of user id and password from the Nucleus WebServ's database.     
*                                                                       
* INPUTS
*
*       *user_id            User id to remove from system
*       *password           Associated password
*
* OUTPUTS
*
*       WS_FAILURE          Could not find user
*       NU_SUCCESS          Success
*
************************************************************************/
STATIC INT16 DES_Auth_Delete_Entry(CHAR *user_id, CHAR *password)
{
#ifdef NET_5_1
    UINT16  status;

    status = (UINT16)UM_Validate_User(user_id, password, UM_WEB);
    if(status == NU_SUCCESS)
        status = (UINT16)UM_Del_User(user_id);

    return(status);
#else
    WPW_INFO_NODE   *apwlist_info;

    /*  Searches to see if the name being asked to delete is availble  */
    apwlist_info = DES_Verify_User(user_id, password);

    if (apwlist_info)
    {
        /*  If entry is found remove it from the list and delete it. */
        DLL_Remove(&DES_Pw_List_Info, apwlist_info);
        if(NU_Deallocate_Memory(apwlist_info) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
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
*       DES_Send_Auth_Salt                                                   
*                                                                       
* DESCRIPTION                                                           
*             
*       respond to a client request for authentication
*       by sending a packet of random bits to be
*       combined with the authentication data the user enters
*                                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID DES_Send_Auth_Salt(WS_REQUEST * req)
{
    CHAR    salt[(WPW_SALT_BYTES * 2) + 2];
    CHAR    nacl[WPW_SALT_BYTES];
    INT     j;
    CHAR    *s;

    DES_Make_New_Salt(nacl);                            /* create a new random number */

#if NU_WEBSERV_DEBUG
    s = salt;
    for(j = 0; j < WPW_SALT_BYTES; j++)
    {
        DES_Bin_To_Hex(nacl[j], s);
        s += 2;
    }

    *s = 0;

    printf("random number(before encrypt): %s\n", salt);
#endif
    
    ENC_Encrypt(req->ws_server->ws_master.ws_key, nacl,2);

    s = salt;
    for(j = 0; j < WPW_SALT_BYTES; j++)
    {
        DES_Bin_To_Hex(nacl[j], s);
        s += 2;
    }
    *s = 0;

#if NU_WEBSERV_DEBUG
    printf("random number(after encrypt): %s\n",salt);
#endif

    /* send it to the applet */
    if(WS_Write_To_Net(req, salt, (UINT32)strlen(salt), WS_FILETRNSFR) != NU_SUCCESS)
        NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Make_New_Salt                                                    
*                                                                       
* DESCRIPTION                                                           
*       
*       128 bit pseudo random Shift Register Generator (SRG)
* 
*       This routine stores a new 64 bit random number
*       in the passed array. This routine is
*       endian and word size independent.
*                                                                
*                                                                       
* INPUTS                                                                
*                                                                       
*       *salt                                                             
*                                                                       
* OUTPUTS                                                               
*             
*       None                                                          
*                                                                       
************************************************************************/
STATIC VOID DES_Make_New_Salt(CHAR *salt)
{
    UINT8   r;
    INT     i, j, k;

    /* taps at certain bits are xor'ed
     * with the shift out bit and fed
     * back into the shift in bit.
     */

    r = DES_Reg[WPW_SRG_SIZE - 1];

    for(i = 0; i < (WPW_SRG_SIZE - 1); i++)
    {

        switch(i)
        {
            case 6: case 9: case 18:
            case 31:case 42:case 54:
            case 68:case 90:case 110:

            /* xor shift out bit with random stages */
            r = (r ^ DES_Reg[i]);                       
        };

        DES_Reg[(WPW_SRG_SIZE - 1) - i] = DES_Reg[(WPW_SRG_SIZE - 1) - (i + 1)];
    }

    /* and feed it back into the shift in bit */
    DES_Reg[0] = r;                                 
    
    k = 0;
    for( i = 0; i < WPW_SRG_SIZE - 1; i += 8)
    {
        
        for(j = 0; j < 7; j++)
        {   /* pack the bits back into bytes */
            salt[k] |= DES_Reg[i + j];
            salt[k] <<= 1;
        }
        
        salt[k] |= DES_Reg[i + j];
        k++;
    }   
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Auth_Control                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function used to control authentication.                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       server                  Pointer to the server control    
*                               structure.                       
*       auth_cmd                The Control command being issued.
*       arg                     The arguments being passed in.   
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              On successful completion
*       WS_FAILURE              Failure to authenticate user
*                                                                       
*************************************************************************/
STATIC INT  DES_Auth_Control(WS_SERVER *server, INT auth_cmd, VOID *arg) 
{
    CHAR    **cs;
    WS_AUTH *a;
    INT     r = NU_SUCCESS;

    switch(auth_cmd)
    {
    case WS_A_ENABLE:      
        /* enable authentication */
        server->ws_master.ws_flags |= WS_AUTH_ENABLED;
        break;
        
    case WS_A_DISABLE:
        server->ws_master.ws_flags &= ~WS_AUTH_ENABLED;
        break;
        
    case WS_A_TIMEOUT:
        server->ws_master.ws_timeoutval = (INT)arg;
        break;
        
    case WS_A_LOGOUT:
        /* blow away authentication */
        a = DES_Check_Auth_IP(server, (WS_REQUEST *)arg);

        if(a)
            DES_Free_Auth(a);
        else
            r = WS_FAILURE;
        break;   
        
    case WS_A_CREDS:
        cs = (CHAR**) arg;
        
        if(*cs)
            strcpy(server->ws_master.ws_key, *cs);   
        break;
        
    case WS_A_AUTH_URI:                                
        /* URI to send to unauthenticated user */
        strcpy(server->ws_master.ws_auth_uri, (CHAR*)arg);
        break;
        
    case WS_A_AUTH_ADD:
        /* Add this client to the authenticated list */
        r = DES_Add_Authenticated(server, (WS_REQUEST *)arg); 
        break;   
    }

    return(r);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Authenticated_Check                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function checks to see if this connection is authenticated. 
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Server already authenticated
*       WS_FAILURE              Unable to authenticate server
*                                                                       
*************************************************************************/
INT DES_Authenticated_Check(WS_REQUEST *req)
{
    WS_AUTH * a;

    a = DES_Check_Auth_IP(req->ws_server, req);

    if(a == NU_NULL) 
        return WS_FAILURE;

    if(a->ws_state & WS_AUTH_GIVEN)
    {
        a->ws_countdown = req->ws_server->ws_master.ws_timeoutval;
        return NU_SUCCESS;
    }

    return WS_FAILURE;
}

/***********************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Check_Auth_IP                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to check and see if the ip address is an already        
*       authenticated  address.                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *server                 Pointer to WS_SERVER structure   
*                               that contains all local server   
*                               information.                     
*       *ip                     The ip address to compare with.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       auth_struct             If the ip address was found in the
*                               authentication table
*       NU_NULL                 The ip address was not found
*                                                                       
*************************************************************************/

STATIC WS_AUTH *DES_Check_Auth_IP(WS_SERVER *server, WS_REQUEST *req)
{
    WS_AUTH * auth_struct;
    
    /* look for the ip address in the table */
    auth_struct = server->ws_master.ws_user_auth;
    
    while(auth_struct)
    {
        if (req->ws_family == NU_FAMILY_IP)
        {
        /* Check if this is the client */
            if(auth_struct->ws_ip[0] == (UINT8)req->ws_ip[0] &&
               auth_struct->ws_ip[1] == (UINT8)req->ws_ip[1] &&
               auth_struct->ws_ip[2] == (UINT8)req->ws_ip[2] &&
               auth_struct->ws_ip[3] == (UINT8)req->ws_ip[3])
                return(auth_struct);
        }
#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
        else if (req->ws_family == NU_FAMILY_IP6)
            if(memcmp(auth_struct->ws_ip, req->ws_ip, MAX_ADDRESS_SIZE) == 0)
                return(auth_struct);
#endif
        auth_struct = auth_struct->ws_next;
    }

    return(NU_NULL);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Free_Auth                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to free the authorization for a particular ip           
*       address.                                                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       *a                      Pointer to authentication        
*                               structure.                       
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID DES_Free_Auth(WS_AUTH *a)
{
#if (defined(NET_5_1))
    UTL_Zero(a->ws_ip, MAX_ADDRESS_SIZE);
#else
    UTL_Zero(a->ws_ip, IP_ADDR_LEN);
#endif

    a->ws_state = WS_AUTH_FREE;
    a->ws_countdown = 0;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Add_Authenticated                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to add a client ip address once the user has been       
*       found to be authenticated.                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *server                 Pointer to server structure that 
*                               contains list of authenticated   
*                               clients.                         
*       *ipaddr                 Clients ip address that wishes to
*                               be authenticated.                
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS                 Added the ip address
*       WS_FAILURE                 Unable to add the ip address
*                                                                       
*************************************************************************/
STATIC INT DES_Add_Authenticated(WS_SERVER *server, WS_REQUEST *req)
{
    WS_AUTH *a;

    a = DES_Check_Auth_IP(server, req);

    if(a == NU_NULL)
    {
        a = server->ws_master.ws_user_auth;
        while( a )
        {                                           
            /* find a free auth structure */
            if( a->ws_state & WS_AUTH_FREE)
                break;

            a = a->ws_next;
        }
        
        if( a )
        {
            if (req->ws_family == NU_FAMILY_IP)
            {
                IP_ADDR_COPY(a->ws_ip, req->ws_ip);
            }    
#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
            else if (req->ws_family == NU_FAMILY_IP6)
                NU_BLOCK_COPY(a->ws_ip, req->ws_ip, MAX_ADDRESS_SIZE);
#endif
            else
                return (WS_FAILURE);

            a->ws_state = WS_AUTH_GIVEN;
            a->ws_countdown = server->ws_master.ws_timeoutval;
        }
        else
            return(WS_FAILURE);
    }
    else
    {
        a->ws_state = WS_AUTH_GIVEN;
        a->ws_countdown = server->ws_master.ws_timeoutval;
    }
    
    return(NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Check_Timeout                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to see if it's time to expire one of the authentication 
*       structures.                                                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID DES_Check_Timeout(WS_REQUEST *req)
{
    UINT32              curtime, delta;
    WS_AUTH             *auth_user;

    /* Get the current time in seconds */
    curtime = WS_Get_Seconds;                              

    /* The number of seconds elapsed */
    if(curtime < req->ws_server->ws_master.ws_last_time)
    {
        delta = curtime + (0xffffffff/TICKS_PER_SECOND - 
                            req->ws_server->ws_master.ws_last_time);
    }
    else
        delta = curtime - req->ws_server->ws_master.ws_last_time;

    auth_user = req->ws_server->ws_master.ws_user_auth;
    
    while(auth_user)
    {
        if((auth_user->ws_countdown) && (auth_user->ws_state != WS_AUTH_FREE))
        {
            if(delta >= auth_user->ws_countdown)
            {
                DES_Free_Auth(auth_user);
            }
            else
                auth_user->ws_countdown -= delta;
        }
        
        auth_user = auth_user->ws_next;
    }

    /* remember for next time */
    req->ws_server->ws_master.ws_last_time = curtime;     
}


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Hex_to_Binary                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to convert hex to binary.                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Hex string to be converted to    
*                               binary.                          
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       r                       Binary converted integer.       
*                                                                       
*************************************************************************/
INT DES_Hex_to_Binary(CHAR *s)
{
    register int r=0;

    while( *s )
    {
        r |= DES_Ascii_Hex_To_Bin((UINT8 *)s);
        s+=2;
        if( *(s) )
            r <<= 8;
    }
    
    return(r);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Ascii_Hex_To_Bin                                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to convert an ascii hex digit to binary.                
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Hex character to be converted to 
*                               binary.                          
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       r                       Binary value.                   
*                                                                       
*************************************************************************/
STATIC unsigned int DES_Ascii_Hex_To_Bin(UINT8 *s)
{
    unsigned int r;

    r = DES_Ascii_To_Nibble(s++);
    r = r << 4;
    r = r & 0xf0;
    r |= DES_Ascii_To_Nibble(s);

    return(r);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Ascii_To_Nibble                                                             
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Converts an ascii hex character to a binary nibble.             
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Character to be converted.           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       r                       Converted integer.                   
*                                                                       
*************************************************************************/
STATIC INT DES_Ascii_To_Nibble(UINT8 *s)
{
    unsigned int r;
     
    r = 0;
    
    if((*s >= 'A' ) && (*s <= 'F' ) )
        r = (*s - 'A') + 10;
    else if((*s >= 'a' ) && (*s <= 'f' ) )
        r = (*s - 'a') + 10;
    else if((*s >= '0' ) && (*s <= '9' ) )
        r = (*s - '0');
    return(r);
    
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Bin_To_Hex                                                             
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to convert binary byte to two hex digits.               
*                                                                       
* INPUTS                                                                
*                                                                       
*       c                       Byte to be converted.            
*       *buf                    Buffer to store converted Hex    
*                               digits.                          
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID  DES_Bin_To_Hex(CHAR c,CHAR *buf)
{
    CHAR *hex = "0123456789ABCDEF";

    *buf++ = hex[(c>>4)&0xf];
    *buf   = hex[c&0xf];
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       DES_Packed_Hex_To_Bin                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to convert packed hex to binary.                        
*                                                                       
* INPUTS                                                                
*                                                                       
*       *d                      Destination string address.      
*       *s                      Source Address                   
*       count                   The number of Hex digits.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID  DES_Packed_Hex_To_Bin(CHAR *d, CHAR *s ,INT count)
{
    INT i,j;

    for(i=0,j=0; i<count; i+=2,j++)
    {
        d[j] = (UINT8)DES_Ascii_Hex_To_Bin((UINT8 *)s);
        s += 2;
    }
}

#endif /* INCLUDE_DES_AUTH */
