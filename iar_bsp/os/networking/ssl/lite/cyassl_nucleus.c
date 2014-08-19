/* Include files */
#include "nucleus.h"
#include "networking/nu_networking.h"

/*Include CyaSSL headers */
#include "cyassl/openssl/ssl.h"
#include "cyassl/ctaocrypt/memory.h"
#include "cyassl/internal.h"
#include "cyassl_nucleus.h"


/*************************************************************************
*
*   FUNCTION
*
*      CYASSL_nucleus_init
*
*   DESCRIPTION
*
*      This function initializes CyaSSL for use with Nucleus
*
**************************************************************************/
SSL_CTX * CYASSL_nucleus_init(void)
{
    static SSL_METHOD * method;
    static SSL_CTX    * ctx;


    /* Check to see if already initialized */
    if (method == 0)
    {
        /* Initialize CyaSSL library */
        CyaSSL_Init();

        /* SSL 3 TLS1.2 */
        method = SSLv3_server_method();

        /* Create SSL context */
        ctx = SSL_CTX_new(method);
    }
    else
    {
        ctx = NU_NULL;
    }

    /* Return created context */
    return (ctx);
}


/*************************************************************************
*
*   FUNCTION
*
*      CYASSL_load_buffer
*
*   DESCRIPTION
*
*      This function loads the certificate in the absence of FILE system support
*
**************************************************************************/
void CYASSL_load_buffer(SSL_CTX* ctx, const char** fname, int type,int size)
{
    /* test buffer load */
    long  sz = 0;
    unsigned char *buff = NU_NULL;
    int i=0;

    /* Allocate buffer */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&buff, size * 65, NU_SUSPEND) == NU_SUCCESS)
    {
       /* convert 2D certificate array of strings into a single character array */
       while(i<size)
       {
           int j=0; /* reset */
           while(fname[i][j]!='\0')
           {
               buff[sz]=fname[i][j];
               j++;
               sz++;
           }
           buff[sz]='\n';
           sz++;
           i++;
       }
       if (type == 1)
       {
          if (CyaSSL_CTX_load_verify_buffer(ctx, buff, sz, SSL_FILETYPE_PEM)
                                             != SSL_SUCCESS)
               printf("can't load buffer ca file");
       }
       else if (type == 2)
       {
           if (CyaSSL_CTX_use_certificate_buffer(ctx, buff, sz, SSL_FILETYPE_PEM) != SSL_SUCCESS)
               printf("can't load buffer cert file");
       }
       else if (type == 3)
       {
           if (CyaSSL_CTX_use_PrivateKey_buffer(ctx, buff, sz, SSL_FILETYPE_PEM) != SSL_SUCCESS)
               printf("can't load buffer key file");
       }

       NU_Deallocate_Memory(buff);
    }
}
