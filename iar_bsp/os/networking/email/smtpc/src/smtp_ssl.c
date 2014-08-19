/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************/

/*************************************************************************
*
*   FILE
*
*       smtp_ssl.c
*
*   COMPONENT
*
*       SMTP Client - SSL API.
*
*   DESCRIPTION
*
*       This file contains the SMTP SSL routines.
*
*   DATA STRUCTURES
*
*       SMTP_Dh512_P
*       SMTP_Dh512_G
*
*   FUNCTIONS
*
*       SMTP_SSLD_Connect
*       SMTP_SSL_Init
*       SMTP_Get_Dh512
*
*   DEPENDENCIES
*
*       nu_storage.h
*       smtp_ssl.h
*
*************************************************************************/
#include "storage/nu_storage.h"
#include "os/networking/email/smtpc/inc/smtp_ssl.h"

static const unsigned char SMTP_Dh512_P[]={
    0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
    0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
    0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
    0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
    0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
    0x47,0x74,0xE8,0x33,
};

static const unsigned char SMTP_Dh512_G[]={
    0x02,
};

/************************************************************************
*
* FUNCTION
*
*      SMTP_SSLD_Connect
*
* DESCRIPTION
*
*      This function runs the client side SSL connection negotiation.
*
* INPUTS
*
*      new_ssl                  Pointer to the area in memory that the
*                               Nucleus SSL layer will use to keep track of
*                               this session.
*      socketd                  Socket descriptor for the current
*                               connection.
*      ssl_ctx                  SSL context to be used.
*
* OUTPUTS
*
*      NU_SUCCESS               On success
*      -1                       Otherwise
*
*************************************************************************/
STATUS SMTP_SSLD_Connect(SSL **new_ssl, INT socketd, SSL_CTX **ssl_ctx)
{
    STATUS     status = NU_SUCCESS;
    SSL        *ssl_struct;

    *new_ssl = NU_NULL;

    /* Create a new context structure for this connection */
    ssl_struct = SSL_new(*ssl_ctx);

    /* If all is well, begin the SSL handshaking with the client */
    if (ssl_struct != NU_NULL)
    {
        /* Attach the socket to a BIO */
        if (!SSL_set_fd(ssl_struct, socketd))
        {
            SSL_free(ssl_struct);
            status = -1;
        }
    }

    if ((status == NU_SUCCESS) && (ssl_struct != NU_NULL))
    {
        if (SSL_connect(ssl_struct) <= 0)
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

    return(status);
} /* SSLD_Connect */

/************************************************************************
*
* FUNCTION
*
*       SMTP_SSL_Init
*
* DESCRIPTION
*
*       Task the completes all initialization of SSL.
*
* INPUTS
*
*       ssl_ctx                 SSL context to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              If initialization is successful.
*       -1                      Otherwise.
*
*************************************************************************/
STATUS SMTP_SSL_Init(SSL_CTX **ssl_ctx)
{
    const SSL_METHOD    *meth = NU_NULL;
    STATUS              status = NU_SUCCESS;

    /* Get the methods that will be used by the connection */
    meth = SSLv23_method();

    /* Initialize the SSL system */
    SSL_library_init();
    *ssl_ctx = SSL_CTX_new(meth);
    if (*ssl_ctx == NU_NULL)
    {
        status = -1;
    }

    if (status == NU_SUCCESS)
    {
        /* Set size of session cache */
        SSL_CTX_sess_set_cache_size(*ssl_ctx, 16);

        /* Load trusted CA for client's certificate verification */
        if (SSL_CTX_load_verify_locations(*ssl_ctx, CFG_NU_OS_NET_EMAIL_SMTPC_SSL_CA_LIST, NU_NULL) != 1)
        {
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Setting the DH parameters to be used by mSSL */
        if (SSL_CTX_set_tmp_dh(*ssl_ctx, SMTP_Get_Dh512()) != 1)
        {
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Load certificates from file.*/
        if (SSL_CTX_use_certificate_file(*ssl_ctx, CFG_NU_OS_NET_EMAIL_SMTPC_SSL_CERT, SSL_FILETYPE_PEM) != 1)
        {
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Load private keys from file.*/
        if (SSL_CTX_use_PrivateKey_file(*ssl_ctx, CFG_NU_OS_NET_EMAIL_SMTPC_SSL_KEY, SSL_FILETYPE_PEM) != 1)
        {
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Load trusted CA List that will be sent to client in certificate request message */
        SSL_CTX_set_client_CA_list(*ssl_ctx, SSL_load_client_CA_file(CFG_NU_OS_NET_EMAIL_SMTPC_SSL_CA_LIST));
    }

    return status;
} /* SMTP_SSL_Init */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Get_Dh512
*
* DESCRIPTION
*
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
DH *SMTP_Get_Dh512(VOID)
{
    DH *dh;

    dh = DH_new();

    if (dh == NU_NULL)
        return(NU_NULL);

    dh->p=BN_bin2bn(SMTP_Dh512_P,sizeof(SMTP_Dh512_P), NU_NULL);
    dh->g=BN_bin2bn(SMTP_Dh512_G,sizeof(SMTP_Dh512_G), NU_NULL);

    if ((dh->p == NU_NULL) || (dh->g == NU_NULL))
        return(NU_NULL);
    return(dh);
} /* SMTP_Get_Dh512 */
