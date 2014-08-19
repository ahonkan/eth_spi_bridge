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
*       web_ssl.c                                                 
*                                                                       
* COMPONENT                                                             
*            
*       Nucleus WebServ                                                           
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file contains code to tie Nucleus WebServ into Nucleus SSL.
*       Nucleus SSL must be purchased seperately from Nucleus WebServ.
*       SSL initialization of the library and cryptos make up this file.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       NU_WS_SSL_Init                     Contains SSL initialization
*                                       and task creation.                         
*       NU_SSL_Get_DH512                Seed for Diffie-Hellman encryption
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h
*       err.h
*       ncs_os_file.h        
*                                                                       
************************************************************************/


#include "networking/nu_websr.h"

#if INCLUDE_SSL

STATIC DH       *NU_SSL_Get_DH512(VOID);

NU_TASK                 NU_SSL_Receive_CB;

SSL_CTX  *NU_SSL_CTX;

#ifdef ERR_REPORT
static BIO      *bio_err = NU_NULL;
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*      NU_WS_SSL_Init                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                           
*      Initializes the SSL server for WebServ.
*                                                                       
* INPUTS                                                                
*                                                                       
*      mem_pool             Pointer to memory pool for allocations.
*                                                                       
* OUTPUTS
*                                                                       
*      status               NU_SUCCESS on successful completion
*                                                                       
*************************************************************************/
STATUS NU_WS_SSL_Init(NU_MEMORY_POOL *mem_pool)
{
    STATUS      status;
    VOID        *pointer;

    const SSL_METHOD *meth = NU_NULL;

    /* Create the SSL receive task. */
    status = NU_Allocate_Memory(mem_pool, &pointer, 2000, NU_NO_SUSPEND);
    
    if (status != NU_SUCCESS)
    {
#if NU_WEBSERV_DEBUG
        printf ("Can not create memory for WS_SSL_Receive_Task.\n");
#endif
        return(status);
    }
    
    status = NU_Create_Task(&NU_SSL_Receive_CB, "NUSSLSrv", WS_Receive_Task,
                            WS_SSL_PORT, NU_NULL, pointer, 2000, TM_PRIORITY + 2,
                             1000, NU_PREEMPT, NU_NO_START);
    
    if (status != NU_SUCCESS)
    {
#if NU_WEBSERV_DEBUG
        printf ("Cannot create WS_SSL_Receive_Task\n");
#endif
        return(status);
    }

    NU_Resume_Task(&NU_SSL_Receive_CB);

    /* Begin initialization of the SSL library */

    /* Initialize Library */
    SSL_library_init();

#ifdef ERR_REPORT
    bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
    
    /* Load error strings for debugging */
    SSL_load_error_strings();
#endif

    /* Get the methods that will be used by the connection */

    meth = SSLv23_method();
    if (meth == NU_NULL)
    {
#if NU_WEBSERV_DEBUG
        printf ("Error getting SSL method!\n");
#endif
        return(WS_FAILURE);
    }

    /* Get the SSL context structure that will be used for all connections */
    NU_SSL_CTX = SSL_CTX_new(meth);
    if (NU_SSL_CTX == NU_NULL)
    {
#if NU_WEBSERV_DEBUG
        printf ("Error creating SSL context!\n");
#endif
        return(WS_FAILURE);
    }

    /* Set size of session cache */
    SSL_CTX_sess_set_cache_size(NU_SSL_CTX, 16);

    /* Set the DH parameters to be used by SSL.*/
    status = SSL_CTX_set_tmp_dh(NU_SSL_CTX, NU_SSL_Get_DH512());

    /* Load certificates from file.*/
    if(status == 1)
        status = SSL_CTX_use_certificate_file(NU_SSL_CTX, NU_SSL_CERT, SSL_FILETYPE_PEM); 

    /* Load private keys from file.*/
    if(status == 1)
        status = SSL_CTX_use_PrivateKey_file(NU_SSL_CTX, NU_SSL_KEY, SSL_FILETYPE_PEM);

    /* Load trusted CA List that will be sent to client in certificate request message */
    SSL_CTX_set_client_CA_list(NU_SSL_CTX, SSL_load_client_CA_file(NU_SSL_CA_LIST));

#ifdef ERR_REPORT
    ERR_print_errors(bio_err);
#endif

    if(status == 1)
        return(NU_SUCCESS);
    else
        return(WS_FAILURE);
}

/************************************************************************
*
* FUNCTION
*
*      WS_SSL_Accept
*
* DESCRIPTION
*
*      This function runs the server side SSL connection negotiation.
*
* INPUTS
*
*      new_ssl              Pointer to the area in memory that the
*                           SSL layer will use to keep track of
*                           this session.
*      socketd              Socket descriptor for the current
*                           connection.
*      ssl_context          Pointer to SSL context structure.
*
* OUTPUTS
*
*      NU_SUCCESS       - On success
*      -1               - Otherwise
*
*************************************************************************/

STATUS WS_SSL_Accept(SSL **new_ssl, INT socketd, SSL_CTX *ssl_context)
{
    STATUS     status = NU_SUCCESS;
    SSL        *ssl_struct;

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    *new_ssl = NU_NULL;

    /* Create a new context structure for this connection */
    ssl_struct = SSL_new(ssl_context);

    /* If all is well, begin the SSL handshaking with the client */
    if(ssl_struct != NU_NULL)
    {
        /* Attach the socket to a BIO */
        if (!SSL_set_fd(ssl_struct, socketd))
        {
            SSL_free(ssl_struct);
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        if(SSL_accept(ssl_struct) <= 0)
        {
            /* A problem has occurred, set the return status and
             * free any memory that might have been used
             */
            SSL_shutdown(ssl_struct);
            SSL_free(ssl_struct);
            status = -1;
        }
        else
        {
            /* Set up the pointer to the SSL structure for
             * use throughout connection
             */
            *new_ssl = ssl_struct;
        }
    }

    NU_USER_MODE();

    return(status);
} /* WS_SSL_Accept */

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*      NU_SSL_Get_DH512                                           
*                                                                       
* DESCRIPTION                                                           
*                
*      Seed for Diffie-Hellman encryption                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*      None               
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      dh           Diffie-Hellman seed                                                             
*                                                                       
*************************************************************************/

static const UINT8 dh512_p[]={
    0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
    0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
    0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
    0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
    0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
    0x47,0x74,0xE8,0x33,
};

static const UINT8 dh512_g[]={
    0x02,
};

STATIC DH *NU_SSL_Get_DH512(VOID)
{
    DH *dh;
    
    if ((dh = DH_new()) == NU_NULL) 
        return(NU_NULL);

    dh->p = BN_bin2bn(dh512_p, sizeof(dh512_p), NU_NULL);
    dh->g = BN_bin2bn(dh512_g, sizeof(dh512_g), NU_NULL);

    if ((dh->p == NU_NULL) || (dh->g == NU_NULL))
        return(NU_NULL);

    return(dh);
}

#endif /* INCLUDE_SSL */
