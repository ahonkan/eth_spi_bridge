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
*       ike_cert.h
*
* COMPONENT
*
*       IKE - General
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes which are used in the Certificates component of IKE.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       x509.h
*       ike_db.h
*       pem.h
*
*************************************************************************/

#ifndef IKE_CERT_H
#define IKE_CERT_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

#include "openssl/x509.h"
#include "networking/ike_db.h"

#if (IKE_INCLUDE_PEM == NU_TRUE)
#include "openssl/pem.h"
#endif

#define IKE_X509_FILETYPE_ASN1   X509_FILETYPE_ASN1
#define IKE_X509_FILETYPE_PEM    X509_FILETYPE_PEM

/**** Function Prototypes ****/

INT IKE_Cert_Match_ASN1DN(UINT8 *dn_a, UINT32 dn_a_len, UINT8 *dn_b,
                             UINT32 dn_b_len);

STATUS IKE_Cert_Get_ASN1_SubjectName(UINT8 *cert_data,
                                     UINT32 cert_data_len, UINT8 **subject,
                                     UINT32 *subject_len);

STATUS IKE_Cert_Get_SubjectAltName(UINT8 *cert_data, UINT32 cert_data_len,
                                   UINT8 index, UINT8 *type,
                                   UINT8 **value);

STATUS IKE_Cert_Verify_ID(UINT8 *cert_data, UINT32 cert_len,
                          IKE_IDENTIFIER *id);

STATUS IKE_Cert_Verify(UINT8 *cert_data, UINT32 cert_len,
                       UINT8 *ca_cert_file, UINT8 encoding,
                       UINT8 *crl_file, UINT8 check_crl);

STATUS IKE_Cert_Get(UINT8 *cert_file, UINT8 **cert_data,
                    UINT16 *cert_data_len, UINT8 cert_enc);

STATUS IKE_Cert_Get_PKCS1_Public_Key(UINT8 *cert_data, UINT32 cert_len,
                                     UINT8 **key_data,UINT32 *key_data_len);

STATUS IKE_Cert_Get_PKCS1_SPKI(UINT8 *cert_data, UINT32 cert_len,
                                     UINT8 **spki, UINT32 *spki_len);

#if (IKE_INCLUDE_PEM == NU_TRUE)
STATUS IKE_Cert_Get_PKCS1_Private_Key(UINT8 *file, UINT8 **key_data,
                                      UINT32 *key_data_len, UINT8 enc,
                                      pem_password_cb *callback);
#else
STATUS IKE_Cert_Get_PKCS1_Private_Key(UINT8 *file, UINT8 **key_data,
                                      UINT32 *key_data_len, UINT8 enc);
#endif

#endif /* #if (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef IKE_CERT_H */
