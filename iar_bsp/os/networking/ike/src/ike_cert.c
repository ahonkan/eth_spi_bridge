/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_cert.c
*
* COMPONENT
*
*       IKE - Certificates
*
* DESCRIPTION
*
*       This file implements the Certificate handling functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Cert_Match_ASN1DN
*       IKE_Cert_Get_ASN1_SubjectName
*       IKE_Cert_Get_SubjectAltName
*       IKE_Cert_Verify_ID
*       IKE_Cert_Verify
*       IKE_Cert_Get
*       IKE_Cert_Get_PKCS1_Public_Key
*       IKE_Cert_Get_PKCS1_Private_Key
*       IKE_Cert_Get_PKCS1_SPKI
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike_cert.h
*       ike.h
*       [x509.h]
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike_cert.h"
#include "networking/ike.h"

#if(IKE_INCLUDE_SIG_AUTH == NU_TRUE)

#undef SSLEAY_MACROS

#include "openssl/x509.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Match_ASN1DN
*
* DESCRIPTION
*
*       This function compares two Distinguished Names (DN) to see if
*       they match. Used to compare the ID payload received and Subject
*       name.
*
* INPUTS
*
*       *dn_a                   First ID parameter which is to be compared.
*       dn_a_len                Length of the DN in bytes.
*       *dn_b                   Second ID parameter which is to be
*                               compared.
*       dn_b_len                Length of the DN in bytes.
*
* OUTPUTS
*
*       0                       When buffers match.
*       Non-zero                When buffers don't match.
*
*************************************************************************/
INT IKE_Cert_Match_ASN1DN(UINT8 *dn_a, UINT32 dn_a_len, UINT8 *dn_b,
                            UINT32 dn_b_len)
{
    INT ret;

    /* If sizes don't match, return error. The buffers won't match anyway
     * in this case.
     */
    if(dn_a_len != dn_b_len)
    {
        return (-1);
    }

    ret = memcmp(dn_a, dn_b, dn_a_len);

    return (ret);

} /* IKE_Cert_Match_ASN1DN */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get_ASN1_SubjectName
*
* DESCRIPTION
*
*       This functions returns an ASN1 encoding of the SubjectName. The
*       certificate could be either the one sent by the remote host or a
*       local certificate read from a file.
*
* INPUTS
*
*       *cert_data              Raw data of the certificate in PEM
*                               encoding.
*       cert_data_len           Length of the certificate data in bytes.
*       **subject               On return, this contains a dynamically
*                               allocated buffer containing the ASN1
*                               encoding of the subjectName.
*       *subject_len            On return, contains length of the
*                               subjectName in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS              Subject name found.
*       IKE_CERT_ERROR          Certificate could not be decoded or parsed.
*       IKE_NOT_FOUND           The subjectName field is blank.
*       IKE_INVALID_PARAMS      Parameters are not as required.
*       IKE_NO_MEMORY           Memory could not be allocated for buffer.
*
*************************************************************************/
STATUS IKE_Cert_Get_ASN1_SubjectName(UINT8 *cert_data,
                                     UINT32 cert_data_len, UINT8 **subject,
                                     UINT32 *subject_len)
{
    STATUS      status;
    X509        *xcert;
    UINT8       *temp_sub;
    INT         sublen;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (subject == NU_NULL)
        || (subject_len == NU_NULL))
    {
        return IKE_INVALID_PARAMS;
    }
#endif

    /* Get X509 structure for certificate from raw data buffer. */
    xcert = d2i_X509(NU_NULL, (const UINT8**)&cert_data, (INT32)cert_data_len);

    if(xcert != NU_NULL)
    {
        /* Read the length of subjectName. With NULL as buffer pointer,
         * only the length is returned.
         */
        sublen = i2d_X509_NAME(xcert->cert_info->subject, NU_NULL);

        if(sublen > 0)
        {
            /* SubjectName exists and is not empty. Allocate memory and
             * read its contents.
             */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)subject, sublen, NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                /* Buffer pointer is placed at the end of buffer after
                 * reading in the data by the following call. Use a
                 * temporary pointer to preserve the original pointer.
                 */
                temp_sub = *subject;
                *subject_len = i2d_X509_NAME(xcert->cert_info->subject,
                                             &temp_sub);
            }

            else
            {
                NLOG_Error_Log("Memory could not be allocated for buffer\
                               to read subjectName", NERR_SEVERE, __FILE__,
                               __LINE__);
            }
        }

        else
        {
            status = IKE_NOT_FOUND;
        }
    }

    else
    {
        status = IKE_CERT_ERROR;
    }

    return (status);

} /* IKE_Cert_Get_ASN1_SubjectName */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get_SubjectAltName
*
* DESCRIPTION
*
*       This functions returns an ASN1 encoding of the subjectAltName.
*       The certificate could be either the one sent by the remote host or
*       a local certificate read from a file.
*
* INPUTS
*
*       *cert_data              Raw data of the certificate in PEM encoding
*       cert_data_len           Length of the certificate data in bytes.
*       index                   If multiple items are present in
*                               subjectAltName, this is a one-based index
*                               of the item being requested.
*       *type                   On return, this contains the type of the
*                               subjectAltName (IP, FQDN, USER_FQDN).
*       **value                 On return, this contains a dynamically
*                               allocated buffer containing the
*                               null-terminated subjectAltName.
*
* OUTPUTS
*
*       NU_SUCCESS              Subject Alt Name found.
*       IKE_CERT_ERROR          Certificate could not be decoded or parsed.
*       IKE_NOT_FOUND           The optional subjectAltName field is
*                               missing or is blank.
*       IKE_INVALID_PARAMS      Parameters are not as required.
*
*************************************************************************/
STATUS IKE_Cert_Get_SubjectAltName(UINT8 *cert_data, UINT32 cert_data_len,
                                   UINT8 index, UINT8 *type,
                                   UINT8 **value)
{
    STATUS              status = IKE_CERT_ERROR;
    X509                *xcert;
    X509_EXTENSION      *ext = NU_NULL;
    INT                 loc = 0;
    INT                 ext_count;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (index < 1) || (type == NU_NULL)
       || (value == NU_NULL))
    {
        return IKE_INVALID_PARAMS;
    }
#endif

    /* Get the X509 structure from raw certificate data. */
    xcert = d2i_X509(NU_NULL, (const UINT8**)&cert_data, (INT32)cert_data_len);

    if(xcert != NU_NULL)
    {
        /* Get the count of extensions present in the certificate. */
        ext_count = X509_get_ext_count(xcert);

        if(ext_count <= 0)
        {
            /* If no extension is present, return error. */
            return IKE_NOT_FOUND;
        }

        for( ; index > 0 ; index--)
        {
            loc = X509_get_ext_by_NID(xcert, NID_subject_alt_name, loc);
            if(loc == -1)
            {
                break;
            }
        }

        if(loc != -1)
        {
            ext = X509_get_ext(xcert, loc);
        }

        /* If specified extension was found. */
        if(ext != NU_NULL)
        {
            *type = (UINT8)(ext->value->type);
            /* Extension was found, now allocate memory to read the value
             * into.
             */
            status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)value,
                                        ext->value->length, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                NU_BLOCK_COPY(*value, ext->value->data, ext->value->length);
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory.",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            /* "ext" was NULL. Required extension not found. */
            status = IKE_NOT_FOUND;
        }

        X509_free(xcert);
    }

    return (status);

} /* IKE_Cert_Get_SubjectAltName */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Verify_ID
*
* DESCRIPTION
*
*       This function compares the peer ID in ID payload and the
*       SubjectAltName in the certificate received.
*
* INPUTS
*
*       *cert_data              Certificate received from peer.
*       cert_len                Length of the certificate received.
*       *id                     IKE Identifier (calculated from the ID
*                               payload).
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CERT_ERROR          Certificate could not be decoded or parsed,
*                               or required field was not found.
*       IKE_ID_MISMATCH         ID matching failed.
*
*************************************************************************/
STATUS IKE_Cert_Verify_ID(UINT8 *cert_data, UINT32 cert_len,
                          IKE_IDENTIFIER *id)
{
    STATUS      status;
    INT8        count;
    UINT8       *sub_name = NU_NULL;
    UINT8       *id_data = NU_NULL;
    UINT32      sub_len;
    UINT32      id_data_len = 0;
    UINT8       altname_type;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (id == NU_NULL))
    {
        return IKE_INVALID_PARAMS;
    }
#endif

    switch(id->ike_type)
    {
    case IKE_DER_ASN1_DN:
        /* Distinguished Name being used as ID data. This is to be matched
         * with the SubjectName in the certificate, so get the SubjectName
         * from certificate.
         */
        status = IKE_Cert_Get_ASN1_SubjectName(cert_data, cert_len,
                                               &sub_name, &sub_len);

        if(status == NU_SUCCESS)
        {
            /* Get the ID data and length from ID payload. */
            id_data = (UINT8*)(id->ike_addr.ike_domain);
            id_data_len = (UINT16)strlen(id->ike_addr.ike_domain);

            /* Match the two buffers. */
            if(IKE_Cert_Match_ASN1DN(sub_name, sub_len, (UINT8*)id_data,
                                      id_data_len) == 0)
            {
                status = NU_SUCCESS;
            }

            else
            {
                status = IKE_ID_MISMATCH;
            }
        }

        else
        {
            NLOG_Error_Log("Failed to read SubjectName from certificate",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        break;

    default:

        /* Whatever the case other than IKE_DER_ASN1_DN, subjectAltName is
         * to be read from certificate and the value compared with the ID
         * payload. Loop for all the matching fields in certificate*/
        for (count = 1; ; count++)
        {
            status = IKE_Cert_Get_SubjectAltName(cert_data, cert_len,
                                                 count, &altname_type,
                                                 &sub_name);
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to read SubjectAltName from \
                               certificate", NERR_SEVERE, __FILE__,
                               __LINE__);
                break;
            }

            /* subjectAltName is a null terminated value, find its length.
             */
            sub_len = strlen((const char *)sub_name);

            switch(id->ike_type)
            {
            case IKE_IPV4:
                id_data = id->ike_addr.ike_ip.ike_addr1;
                /* In case of IPv4, address length is 4 bytes. */
                id_data_len = 4;
                break;

            case IKE_IPV6:
                id_data = id->ike_addr.ike_ip.ike_addr1;
                /* In case of IPv6, address length is 16 bytes. */
                id_data_len = 16;
                break;

            case IKE_DOMAIN_NAME:
            case IKE_USER_DOMAIN_NAME:
                id_data = (UINT8*)(id->ike_addr.ike_domain);
                id_data_len = strlen(id->ike_addr.ike_domain);
                break;

            default:
                NLOG_Error_Log("ID type not supported.", NERR_SEVERE,
                                __FILE__, __LINE__);
                break;
            }

            if(IKE_Cert_Match_ASN1DN(sub_name, sub_len, id_data,
                                      id_data_len) != 0)
            {
                status = IKE_ID_MISMATCH;
                NLOG_Error_Log("ID mismatch in certificate.", NERR_SEVERE,
                    __FILE__, __LINE__);
            }
        }

        break;
    }

    if(sub_name != NU_NULL)
    {
        if(NU_Deallocate_Memory(sub_name) != NU_SUCCESS)
        {
            NLOG_Error_Log("Memory could not be deallocated.",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE_Cert_Verify_ID */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Verify
*
* DESCRIPTION
*
*       This function is used to verify a certificate received from a
*       remote host. It checks the certificate against the CA.
*
* INPUTS
*
*       *cert_data              Raw data of the certificate in PEM encoding
*       cert_data_len           Length of the certificate data in bytes.
*       *ca_cert_file           Path to the local CA certificate file.
*       encoding                Encoding of the certificate files.
*       crl_file                CRL file to check the certificate against.
*       check_crl               Flag to indicate whether the certificate
*                               should be checked against CRL or not.
*
* OUTPUTS
*
*       NU_SUCCESS              Certificate verification succeeded.
*       IKE_AUTH_FAILED         Certificate verification failed.
*       IKE_CERT_ERROR          Certificate parsing error.
*       IKE_GEN_ERROR           Error in creating certificate store.
*       IKE_INVALID_PARAMS      Parameters to function were not valid.
*
*************************************************************************/
STATUS IKE_Cert_Verify(UINT8 *cert_data, UINT32 cert_len,
                       UINT8 *ca_cert_file, UINT8 encoding,
                       UINT8 *crl_file, UINT8 check_crl)
{
    STATUS          status = NU_SUCCESS;
    X509_STORE_CTX  ctx;
    X509_LOOKUP     *lookup;
    X509_STORE      *store;
    X509            *xcert;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (ca_cert_file == NU_NULL) ||
       ((crl_file == NU_NULL) && (check_crl == 1)))
    {
        return IKE_INVALID_PARAMS;
    }
#endif

    /* Create a new X509 certificate store. */
    store = X509_STORE_new();

    if(store != NU_NULL)
    {
        /* Store created successfully! Add lookup methods to it. */
        lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());

        if(lookup != NU_NULL)
        {
            /* Load the certificate(s) from the CA certificate file.
             * This function returns the number of certificates loaded
             * from the file specified, however we do not need to be
             * concerned with that here.
             */
            X509_LOOKUP_load_file(lookup, (const char *)ca_cert_file,
                                  encoding);

            /* Read the certificate to be verified into an X509 structure.
             */
            xcert = d2i_X509(NU_NULL, (const UINT8**)&cert_data, (INT32)cert_len);

            if(xcert != NU_NULL)
            {
                /* Certificate read successfully.*/
#ifdef X509_CRL_ENABLED
                if(check_crl != 0)
                {
                    /* Certificate checking against CRL is requested. Set
                     * the appropriate flag in store context and load the
                     * CRL file. */
                    if(crl_file != NU_NULL)
                    {
                        X509_STORE_CTX_set_flags(&ctx,
                                                 X509_V_FLAG_CRL_CHECK);
                        X509_load_crl_file(lookup,
                                           (const char *)crl_file,
                                           encoding);
                    }
                }
#endif
                /* Initialize the store context. */
                X509_STORE_CTX_init(&ctx, store, xcert, NU_NULL);
                // TODO: Add_Algos();

                if(X509_verify_cert(&ctx) == 0)
                {
                    status = IKE_AUTH_FAILED;
                    NLOG_Error_Log("Certificate verification failed. ",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                status = IKE_CERT_ERROR;
            }
        }

        else
        {
            status = IKE_GEN_ERROR;
        }
    }

    else
    {
        status = IKE_GEN_ERROR;
    }

    return (status);

} /* IKE_Cert_Verify */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get
*
* DESCRIPTION
*
*       This function reads data from a local certificate file and returns
*       it in a dynamically allocated buffer.
*
* INPUTS
*
*       *cert_file              Local file containing the certificate.
*       **cert_data             On return, this contains raw data of the
*                               certificate in PEM encoding stored in a
*                               dynamically allocated buffer.
*       cert_data_len           On return, this contains length of the
*                               certificate data in bytes.
*       cert_enc                Encoding of the certificate.
*
* OUTPUTS
*
*       NU_SUCCESS              Certificate file read successfully.
*       IKE_INVALID_PARAMS      Parameters are not correct.
*       IKE_NOT_FOUND           File was not found.
*       IKE_GEN_ERROR           Error in cipher suite.
*       IKE_CERT_FILE_ERROR     Certificate file not read successfully.
*       IKE_CERT_ERROR          Certificate could not be parsed.
*
*************************************************************************/
STATUS IKE_Cert_Get(UINT8 *cert_file, UINT8 **cert_data,
                    UINT16 *cert_data_len, UINT8 cert_enc)
{
    STATUS      status = NU_SUCCESS;
    BIO         *bio;
    X509        *xcert = NULL;
    INT         len;
    UINT8       *temp_cert;

    bio = BIO_new(BIO_s_file());

    if(bio != NU_NULL)
    {
        if(BIO_read_filename(bio,cert_file) > 0)
        {
            switch(cert_enc)
            {
            case IKE_X509_FILETYPE_ASN1:
                xcert = d2i_X509_bio(bio, NULL);
                break;

#if(IKE_INCLUDE_PEM == NU_TRUE)
            case IKE_X509_FILETYPE_PEM:
                xcert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
                break;
#endif

            default:
                status = IKE_INVALID_PARAMS;
                break;
            }

            if((xcert == NU_NULL) && (status != IKE_INVALID_PARAMS))
            {
                status = IKE_CERT_ERROR;
            }
        }

        else
        {
            status = IKE_CERT_FILE_ERROR;
        }

        if(status == NU_SUCCESS)
        {
            /* Get the length of the certificate data. */
            len = i2d_X509(xcert, NU_NULL);
            /* Allocate buffer to hold the certificate data. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)cert_data, len,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* The call to i2d_X509 places the pointer at the end
                 * of the buffer it has just written. A temporary pointer
                 * is used to preserve the original buffer pointer. */
                temp_cert = *cert_data;
                *cert_data_len = (UINT16)i2d_X509(xcert, &temp_cert);
                X509_free(xcert);
            }

            else
            {
                NLOG_Error_Log("Memory could not be allocated for buffer \
                               to hold the certificate data. ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                status = IKE_NO_MEMORY;
            }
        }

        BIO_free(bio);
    }

    else
    {
        status = IKE_GEN_ERROR;
    }

    return (status);

} /* IKE_Cert_Get */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get_PKCS1_Public_Key
*
* DESCRIPTION
*
*       This function reads the public key from a local certificate file
*       and returns it to the caller.
*
* INPUTS
*
*       *cert_data              Certificate to read key from.
*       cert_len                Length of the certificate data.
*       **key_data              On return, this contains raw data of the
*                               key in ASN1 encoding, stored in a
*                               dynamically allocated buffer.
*       key_data_len            On return, this contains length of the key
*                               data in bytes.
*
* OUTPUTS
*
*       IKE_INVALID_PARAMS  File contains invalid encoding.
*       IKE_CERT_ERROR      Certificate could not be parsed.
*       IKE_NOT_FOUND       Public key not found.
*       IKE_NO_MEMORY       Memory could not be allocated for buffer.
*       NU_SUCCESS          Key read successfully.
*
*************************************************************************/
STATUS IKE_Cert_Get_PKCS1_Public_Key(UINT8 *cert_data, UINT32 cert_len,
                                     UINT8 **key_data,
                                     UINT32 *key_data_len)
{
    STATUS          status;
    EVP_PKEY        *evp_key;
    X509            *x509;
    UINT8           *temp_key;
    INT             len;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (key_data == NU_NULL) ||
       (key_data_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get X509 structure from certificate data. */
    x509 = d2i_X509(NU_NULL, (const UINT8**)&cert_data, (INT32)cert_len);

    if(x509 != NU_NULL)
    {
        /* Certificate parsed successfully, now read the key. */
        evp_key = X509_get_pubkey(x509);

        if(evp_key != NU_NULL)
        {
            /* Find the key length. This is done with providing NULL in
             * place of buffer for key data.
             */
            len = i2d_PublicKey(evp_key, NU_NULL);

            /* Allocate memory for buffer to hold key data. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)key_data, len,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* The following call places the pointer to buffer at the
                 * end of the buffer after writing it. Use a temporary
                 * pointer to preserve the original pointer.
                 */
                temp_key = *key_data;
                *key_data_len = i2d_PublicKey(evp_key, &temp_key);
            }

            else
            {
                NLOG_Error_Log("Memory could not be allocated for buffer \
                               to hold the key data.", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        else
        {
            status = IKE_NOT_FOUND;
        }
    }

    else
    {
        status = IKE_CERT_ERROR;
    }

    return (status);

} /* IKE_Cert_Get_PKCS1_Public_Key */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get_PKCS1_Private_Key
*
* DESCRIPTION
*
*       This function reads the private key from a local file and returns
*       it to the caller.
*
* INPUTS
*
*       *file                   Path to file containing the private key.
*       **key_data              On return, this contains raw data of the
*                               key in ASN1 encoding, stored in a
*                               dynamically allocated buffer.
*       key_data_len            On return, this contains length of the key
*                               data in bytes.
*       enc                     Encoding of the key file.
*       callback                Callback function (provided by user) to
*                               read in the password for encrypted PEM
*                               encoded private key file.(Only used when
*                               support for PEM format is enabled.)
*
* OUTPUTS
*
*       IKE_INVALID_PARAMS      File contains invalid encoding.
*       IKE_NOT_FOUND           File was not found.
*
*************************************************************************/
#if(IKE_INCLUDE_PEM == NU_TRUE)
STATUS IKE_Cert_Get_PKCS1_Private_Key(UINT8 *file, UINT8 **key_data,
                                      UINT32 *key_data_len, UINT8 enc,
                                      pem_password_cb *callback)
#else
STATUS IKE_Cert_Get_PKCS1_Private_Key(UINT8 *file, UINT8 **key_data,
                                      UINT32 *key_data_len, UINT8 enc)
#endif
{
    STATUS          status = NU_SUCCESS;
    BIO             *bio;
    EVP_PKEY        *key = NULL;
    UINT32          length;
    UINT8           *temp_key;

    bio = BIO_new(BIO_s_file());

    if(bio != NU_NULL)
    {
        if(BIO_read_filename(bio, file) > 0)
        {
            switch(enc)
            {
            case IKE_X509_FILETYPE_ASN1:
#if(IKE_INCLUDE_PEM == NU_TRUE)
                UNUSED_PARAMETER(callback);
#endif
                key = d2i_PrivateKey_bio(bio, NULL);
                break;

#if(IKE_INCLUDE_PEM == NU_TRUE)
            case IKE_X509_FILETYPE_PEM:
                /* TODO: Add_Algos(); */
                key = PEM_read_bio_PrivateKey(bio, NU_NULL, callback,
                                              NU_NULL);
                break;
#endif

            default:
                status = IKE_INVALID_PARAMS;
                break;
            }

            if((key == NU_NULL) && (status != IKE_INVALID_PARAMS))
            {
                status = IKE_CERT_ERROR;
            }
        }

        else
        {
            status = IKE_CERT_FILE_ERROR;
        }

        if(status == NU_SUCCESS)
        {
            /* Read the length of the key. */
            length = i2d_PrivateKey(key, NU_NULL);
            /* Allocate memory for buffer to hold the key data. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)key_data, length,
                                        NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                /* The following call places the buffer pointer at the end.
                 * Therefore, a temporary pointer is used to preserve the
                 * original pointer.
                 */
                temp_key = *key_data;
                *key_data_len = i2d_PrivateKey(key, &temp_key);
            }
            EVP_PKEY_free(key);
        }

        BIO_free(bio);
    }

    else
    {
        status = IKE_GEN_ERROR;
    }

    return (status);

} /* IKE_Cert_Get_PKCS1_Private_Key */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Cert_Get_PKCS1_SPKI
*
* DESCRIPTION
*
*       This function reads the Subject Public Key Information (algorithm
*       identifier which specifies which public key crypto + public key)
*       from a local certificate file and returns it to the caller.
*
* INPUTS
*
*       *cert_data              Certificate to read key from.
*       cert_len                Length of the certificate data.
*       **spki                  On return, this contains raw data of the
*                               key in ASN1 encoding, stored in a
*                               dynamically allocated buffer.
*       *spki_len               On return, this contains length of the key
*                               data in bytes.
*
* OUTPUTS
*
*       IKE_INVALID_PARAMS      File contains invalid encoding.
*       IKE_CERT_ERROR          Certificate could not be parsed.
*       IKE_NOT_FOUND           Public key not found.
*       IKE_NO_MEMORY           Memory could not be allocated for buffer.
*       NU_SUCCESS              Key read successfully.
*
*************************************************************************/
STATUS IKE_Cert_Get_PKCS1_SPKI(UINT8 *cert_data, UINT32 cert_len,
                                     UINT8 **spki, UINT32 *spki_len)
{
    STATUS          status;
    EVP_PKEY        *evp_key;
    X509            *x509;
    UINT8           *temp_key;
    INT             len;

#if(IKE_DEBUG == NU_TRUE)
    if((cert_data == NU_NULL) || (spki == NU_NULL) ||
       (spki_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get X509 structure from certificate data. */
    x509 = d2i_X509(NU_NULL, (const unsigned char **)&cert_data, (INT32)cert_len);

    if(x509 != NU_NULL)
    {
        /* Certificate parsed successfully, now read the key. */
        evp_key = X509_get_pubkey(x509);

        if(evp_key != NU_NULL)
        {
            /* Find the SPKI length. This is done with providing NULL in
             * place of return buffer.
             */
            len = i2d_PUBKEY(evp_key, NU_NULL);


            /* Allocate memory for buffer to hold SPKI data. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)spki, len,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* The following call places the pointer to buffer at the
                 * end of the buffer after writing it. Use a temporary
                 * pointer to preserve the original pointer.
                 */
                temp_key = *spki;
                *spki_len = i2d_PUBKEY(evp_key, &temp_key);
            }

            else
            {
                NLOG_Error_Log("Memory could not be allocated for buffer \
                        to hold the Subject Public Key Information data.",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            status = IKE_NOT_FOUND;
        }
    }

    else
    {
        status = IKE_CERT_ERROR;
    }

    return (status);

} /* IKE_Cert_Get_PKCS1_SPKI */

#endif /* #if(IKE_INCLUDE_SIG_AUTH == NU_TRUE) */
