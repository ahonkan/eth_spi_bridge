/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*
 * WPA Supplicant / SSL/TLS interface functions for openssl
 * Copyright (c) 2004-2008, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */
#include "includes.h"

#include "common.h"
#include "tls.h"


#include "networking/nu_net.h"
#include "networking/ssl.h"
#include "networking/os_ssl.h"
#include "networking/err.h"
#include "networking/obj_mac.h"
#include "networking/os_time.h"
#include "networking/ssl_dot1x.h"
#include "crypto/ncs_pem.h"

#ifndef CONFIG_TLS_INTERNAL

#undef BIO_write
#undef BIO_read
#define BIO_write mem_write
#define BIO_read mem_read
#define mSSL_d2i_TYPE unsigned char **

VOID mem_ssl_set_bio(SSL *s, BIO *rbio, BIO *wbio);

static int tls_openssl_ref_count = 0;

struct tls_connection {
	SSL *ssl;
	BIO *ssl_in, *ssl_out;
	char *subject_match, *altsubject_match;
	int read_alerts, write_alerts, failed;

	tls_session_ticket_cb session_ticket_cb;
	void *session_ticket_cb_ctx;

	/* SessionTicket received from OpenSSL hello_extension_cb (server) */
	u8 *session_ticket;
	size_t session_ticket_len;
};


#ifdef CONFIG_NO_STDOUT_DEBUG

static void _tls_show_errors(void)
{
	unsigned long err;

	while ((err = ERR_get_error())) {
		/* Just ignore the errors, since stdout is disabled */
	}
}
#define tls_show_errors(l, f, t) _tls_show_errors()

#else /* CONFIG_NO_STDOUT_DEBUG */

static void tls_show_errors(int level, const char *func, const char *txt)
{
	unsigned long err;

	wpa_printf(level, "NucleusSSL: %s - %s %s",
		   func, txt, ERR_error_string(ERR_get_error(), NULL));

	while ((err = ERR_get_error())) {
		wpa_printf(MSG_INFO, "NucleusSSL: pending error: %s",
			   ERR_error_string(err, NULL));
	}
}

#endif /* CONFIG_NO_STDOUT_DEBUG */


static int tls_cryptoapi_cert(SSL *ssl, const char *name)
{
	wpa_printf(MSG_INFO, "TLS: Cryto API is not supported.");
	return -1;
}

static void ssl_info_cb(SSL *ssl, int where, int ret)
{
	const char *str;
	int w;

	wpa_printf(MSG_DEBUG, "SSL: (where=0x%x ret=0x%x)", where, ret);
	w = where & ~SSL_ST_MASK;
	if (w & SSL_ST_CONNECT)
		str = "SSL_connect";
	else if (w & SSL_ST_ACCEPT)
		str = "SSL_accept";
	else
		str = "undefined";

	if (where & SSL_CB_LOOP) {
		wpa_printf(MSG_DEBUG, "SSL: %s:%s",
			   str, SSL_state_string_long((SSL*)ssl));
	} else if (where & SSL_CB_ALERT) {
		wpa_printf(MSG_INFO, "SSL: SSL3 alert: %s:%s:%s",
			   where & SSL_CB_READ ?
			   "read (remote end reported an error)" :
			   "write (local SSL3 detected an error)",
			   SSL_alert_type_string_long(ret),
			   SSL_alert_desc_string_long(ret));
		if ((ret >> 8) == SSL3_AL_FATAL) {
			struct tls_connection *conn =
				SSL_get_app_data((SSL *) ssl);
			if (where & SSL_CB_READ)
				conn->read_alerts++;
			else
				conn->write_alerts++;
		}
	} else if (where & SSL_CB_EXIT && ret <= 0) {
		wpa_printf(MSG_DEBUG, "SSL: %s:%s in %s",
			   str, ret == 0 ? "failed" : "error",
			   SSL_state_string_long((SSL*)ssl));
	}
}

void * tls_init(const struct tls_config *conf)
{
	SSL_CTX *ssl;
	struct os_time time = {0, 0};
	if (tls_openssl_ref_count == 0) {
		SSL_load_error_strings();

		/* Initialize the system */
		if (SSL_Init(&System_Memory, CFG_NU_OS_NET_WPA_SUPP_SRC_DEFAULT_DRIVE, 
										TLSv1_method(), &ssl) != NU_SUCCESS) {
			wpa_printf(MSG_ERROR, "Unable to initialize Nucleus SSL");
			return NULL;
		}
		os_get_time(&time);
		OS_time_set((UINT32)time.sec);

	} else {
		ssl = SSL_CTX_new(TLSv1_method());
		if (ssl == NULL)
			return NULL;
	}

	tls_openssl_ref_count++;
	ssl->DOT1X_Flag = NU_TRUE;
	SSL_CTX_set_info_callback(ssl, ssl_info_cb);

	return ssl;
}


void tls_deinit(void *ssl_ctx)
{
	SSL_CTX *ssl = ssl_ctx;
	SSL_CTX_free(ssl);

	tls_openssl_ref_count--;
	if (tls_openssl_ref_count == 0) {
		
		ERR_remove_state(0);
		ERR_free_strings();

	}
}


static int tls_engine_init(struct tls_connection *conn, const char *engine_id,
			   const char *pin, const char *key_id,
			   const char *cert_id, const char *ca_cert_id)
{
	wpa_printf(MSG_INFO, "TLS: No support for engine in Nucleus SSL");
	return 0;
}


static void tls_engine_deinit(struct tls_connection *conn)
{

}


int tls_get_errors(void *ssl_ctx)
{
	int count = 0;
	unsigned long err;

	while ((err = ERR_get_error())) {
		wpa_printf(MSG_INFO, "TLS - SSL error: %s",
			   ERR_error_string(err, NULL));
		count++;
	}

	return count;
}

struct tls_connection * tls_connection_init(void *ssl_ctx)
{
	SSL_CTX *ssl = ssl_ctx;
	struct tls_connection *conn;
	long options;

	conn = os_zalloc(sizeof(*conn));
	if (conn == NULL)
		return NULL;
	conn->ssl = SSL_new(ssl);
	if (conn->ssl == NULL) {
		tls_show_errors(MSG_INFO, __func__,
				"Failed to initialize new SSL connection");
		os_free(conn);
		return NULL;
	}

	SSL_set_app_data(conn->ssl, conn);
	options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
		SSL_OP_SINGLE_DH_USE;
	SSL_set_options(conn->ssl, options);

	conn->ssl_in = BIO_new(BIO_s_file());
	if (!conn->ssl_in) {
		tls_show_errors(MSG_INFO, __func__,
				"Failed to create a new BIO for ssl_in");
		SSL_free(conn->ssl);
		os_free(conn);
		return NULL;
	}

	conn->ssl_out = BIO_new(BIO_s_file());
	if (!conn->ssl_out) {
		tls_show_errors(MSG_INFO, __func__,
				"Failed to create a new BIO for ssl_out");
		SSL_free(conn->ssl);
		BIO_free(conn->ssl_in);
		os_free(conn);
		return NULL;
	}

	mem_ssl_set_bio(conn->ssl, conn->ssl_in, conn->ssl_out);

	return conn;
}


void tls_connection_deinit(void *ssl_ctx, struct tls_connection *conn)
{
	if (conn == NULL)
		return;
	SSL_free(conn->ssl);
	tls_engine_deinit(conn);
	os_free(conn->subject_match);
	os_free(conn->altsubject_match);
	os_free(conn->session_ticket);
	os_free(conn);
}


int tls_connection_established(void *ssl_ctx, struct tls_connection *conn)
{
	return conn ? SSL_is_init_finished(conn->ssl) : 0;
}


int tls_connection_shutdown(void *ssl_ctx, struct tls_connection *conn)
{
	if (conn == NULL)
		return -1;

	/* Shutdown previous TLS connection without notifying the peer
	 * because the connection was already terminated in practice
	 * and "close notify" shutdown alert would confuse AS. */
	SSL_set_quiet_shutdown(conn->ssl, 1);
	SSL_shutdown(conn->ssl);
	return 0;
}


static int tls_match_altsubject_component(NCS_X509 *cert, int type,
					  const char *value, size_t len)
{
	GENERAL_NAME *gen;
	void *ext;
	int i, found = 0;

	ext = NCS_X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);

	for (i = 0; ext && i < NCS_sk_GENERAL_NAME_num(ext); i++) {
		gen = NCS_sk_GENERAL_NAME_value(ext, i);
		if (gen->type != type)
			continue;
		if (os_strlen((char *) gen->d.ia5->data) == len &&
		    os_memcmp(value, gen->d.ia5->data, len) == 0)
			found++;
	}

	return found;
}


static int tls_match_altsubject(NCS_X509 *cert, const char *match)
{
	int type;
	const char *pos, *end;
	size_t len;

	pos = match;
	do {
		if (os_strncmp(pos, "EMAIL:", 6) == 0) {
			type = GEN_EMAIL;
			pos += 6;
		} else if (os_strncmp(pos, "DNS:", 4) == 0) {
			type = GEN_DNS;
			pos += 4;
		} else if (os_strncmp(pos, "URI:", 4) == 0) {
			type = GEN_URI;
			pos += 4;
		} else {
			wpa_printf(MSG_INFO, "TLS: Invalid altSubjectName "
				   "match '%s'", pos);
			return 0;
		}
		end = os_strchr(pos, ';');
		while (end) {
			if (os_strncmp(end + 1, "EMAIL:", 6) == 0 ||
			    os_strncmp(end + 1, "DNS:", 4) == 0 ||
			    os_strncmp(end + 1, "URI:", 4) == 0)
				break;
			end = os_strchr(end + 1, ';');
		}
		if (end)
			len = end - pos;
		else
			len = os_strlen(pos);
		if (tls_match_altsubject_component(cert, type, pos, len) > 0)
			return 1;
		pos = end + 1;
	} while (end);

	return 0;
}


static int tls_verify_cb(int preverify_ok, NCS_X509_STORE_CTX *x509_ctx)
{
	char buf[256];
	NCS_X509 *err_cert;
	int err, depth;
	SSL *ssl;
	struct tls_connection *conn;
	char *match, *altmatch;

	err_cert = NCS_X509_STORE_CTX_get_current_cert(x509_ctx);
	err = NCS_X509_STORE_CTX_get_error(x509_ctx);
	depth = NCS_X509_STORE_CTX_get_error_depth(x509_ctx);
	ssl = NCS_X509_STORE_CTX_get_ex_data(x509_ctx,
					 SSL_get_ex_data_X509_STORE_CTX_idx());
	NCS_X509_NAME_oneline(NCS_X509_get_subject_name(err_cert), buf, sizeof(buf));

	conn = SSL_get_app_data(ssl);
	match = conn ? conn->subject_match : NULL;
	altmatch = conn ? conn->altsubject_match : NULL;

	if (!preverify_ok) {
		wpa_printf(MSG_WARNING, "TLS: Certificate verification failed,"
			   " error %d (%s) depth %d for '%s'", err,
			   NCS_X509_verify_cert_error_string(err), depth, buf);
	} else {
		wpa_printf(MSG_DEBUG, "TLS: tls_verify_cb - "
			   "preverify_ok=%d err=%d (%s) depth=%d buf='%s'",
			   preverify_ok, err,
			   NCS_X509_verify_cert_error_string(err), depth, buf);
		if (depth == 0 && match && os_strstr(buf, match) == NULL) {
			wpa_printf(MSG_WARNING, "TLS: Subject '%s' did not "
				   "match with '%s'", buf, match);
			preverify_ok = 0;
		} else if (depth == 0 && altmatch &&
			   !tls_match_altsubject(err_cert, altmatch)) {
			wpa_printf(MSG_WARNING, "TLS: altSubjectName match "
				   "'%s' not found", altmatch);
			preverify_ok = 0;
		}
	}

	return preverify_ok;
}


#ifndef NO_STDIO
static int tls_load_ca_der(void *_ssl_ctx, const char *ca_cert)
{
	SSL_CTX *ssl_ctx = _ssl_ctx;
	NCS_X509_LOOKUP *lookup;
	int ret = 0;

	lookup = NCS_X509_STORE_add_lookup(ssl_ctx->cert_store,
					   NCS_X509_LOOKUP_file());
	if (lookup == NULL) {
		tls_show_errors(MSG_WARNING, __func__,
				"Failed add lookup for X509 store");
		return -1;
	}

	if (!NCS_X509_LOOKUP_load_file(lookup, ca_cert, NCS_X509_FILETYPE_ASN1)) {
		unsigned long err = ERR_peek_error();
		tls_show_errors(MSG_WARNING, __func__,
				"Failed load CA in DER format");
		if (ERR_GET_LIB(err) == ERR_LIB_X509 &&
			ERR_GET_REASON(err) == NCS_X509_R_CERT_ALREADY_IN_HASH_TABLE) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: %s - ignoring "
				   "cert already in hash table error",
				   __func__);
		} else
			ret = -1;
	}

	return ret;
}
#endif /* NO_STDIO */


static int tls_connection_ca_cert(void *_ssl_ctx, struct tls_connection *conn,
				  const char *ca_cert, const u8 *ca_cert_blob,
				  size_t ca_cert_blob_len, const char *ca_path)
{
	SSL_CTX *ssl_ctx = _ssl_ctx;

	/*
	 * Remove previously configured trusted CA certificates before adding
	 * new ones.
	 */
	NCS_X509_STORE_free(ssl_ctx->cert_store);
	ssl_ctx->cert_store = NCS_X509_STORE_new();
	if (ssl_ctx->cert_store == NULL) {
		wpa_printf(MSG_DEBUG, "NucleusSSL: %s - failed to allocate new "
			   "certificate store", __func__);
		return -1;
	}

	if (ca_cert_blob) {
		NCS_X509 *cert = NCS_d2i_X509(NULL, (mSSL_d2i_TYPE) &ca_cert_blob,
					  ca_cert_blob_len);
		if (cert == NULL) {
			tls_show_errors(MSG_WARNING, __func__,
					"Failed to parse ca_cert_blob");
			return -1;
		}

		if (!NCS_X509_STORE_add_cert(ssl_ctx->cert_store, cert)) {
			unsigned long err = ERR_peek_error();
			tls_show_errors(MSG_WARNING, __func__,
					"Failed to add ca_cert_blob to "
					"certificate store");
			if (ERR_GET_LIB(err) == ERR_LIB_X509 &&
			    ERR_GET_REASON(err) ==
			    NCS_X509_R_CERT_ALREADY_IN_HASH_TABLE) {
				wpa_printf(MSG_DEBUG, "NucleusSSL: %s - ignoring "
					   "cert already in hash table error",
					   __func__);
			} else {
				NCS_X509_free(cert);
				return -1;
			}
		}
		NCS_X509_free(cert);
		wpa_printf(MSG_DEBUG, "NucleusSSL: %s - added ca_cert_blob "
			   "to certificate store", __func__);
		SSL_set_verify(conn->ssl, SSL_VERIFY_PEER, tls_verify_cb);
		return 0;
	}

	if (ca_cert || ca_path) {
#ifndef NO_STDIO
		os_file_setup();
		if (SSL_CTX_load_verify_locations(ssl_ctx, ca_cert, SSL_FILETYPE_ASN1) !=
		    1){
			tls_show_errors(MSG_WARNING, __func__,
					"Failed to load root certificates");
			if (ca_cert &&
			    tls_load_ca_der(ssl_ctx, ca_cert) == 0) {
				wpa_printf(MSG_DEBUG, "NucleusSSL: %s - loaded "
					   "DER format CA certificate",
					   __func__);
			} else
				return -1;
		} else {
			wpa_printf(MSG_DEBUG, "TLS: Trusted root "
				   "certificate(s) loaded");
			tls_get_errors(ssl_ctx);
		}
		SSL_set_verify(conn->ssl, SSL_VERIFY_PEER, tls_verify_cb);
#else /* NO_STDIO */
		wpa_printf(MSG_DEBUG, "NucleusSSL: %s - NO_STDIO",
			   __func__);
		return -1;
#endif /* NO_STDIO */
	} else {
		/* No ca_cert configured - do not try to verify server
		 * certificate */
		SSL_set_verify(conn->ssl, SSL_VERIFY_NONE, NULL);
	}

	return 0;
}


static int tls_global_ca_cert(SSL_CTX *ssl_ctx, const char *ca_cert)
{
	if (ca_cert) {
		if (SSL_CTX_load_verify_locations(ssl_ctx, ca_cert, SSL_FILETYPE_ASN1) != 1)
		{
			tls_show_errors(MSG_WARNING, __func__,
					"Failed to load root certificates");
			return -1;
		}

		wpa_printf(MSG_DEBUG, "TLS: Trusted root "
			   "certificate(s) loaded");

#ifndef NO_STDIO
		/* Add the same CAs to the client certificate requests */
		SSL_CTX_set_client_CA_list(ssl_ctx,
					   SSL_load_client_CA_file(ca_cert, SSL_FILETYPE_PEM));
#endif /* NO_STDIO */
	}
	return 0;
}


int tls_global_set_verify(void *ssl_ctx, int check_crl)
{
	int flags;

	if (check_crl) {
		NCS_X509_STORE *cs = SSL_CTX_get_cert_store(ssl_ctx);
		if (cs == NULL) {
			tls_show_errors(MSG_INFO, __func__, "Failed to get "
					"certificate store when enabling "
					"check_crl");
			return -1;
		}
		flags = NCS_X509_V_FLAG_CRL_CHECK;
		if (check_crl == 2)
			flags |= NCS_X509_V_FLAG_CRL_CHECK_ALL;
		NCS_X509_STORE_CTX_set_flags(ssl_ctx, flags);
	}
	return 0;
}


static int tls_connection_set_subject_match(struct tls_connection *conn,
					    const char *subject_match,
					    const char *altsubject_match)
{
	os_free(conn->subject_match);
	conn->subject_match = NULL;
	if (subject_match) {
		conn->subject_match = os_strdup(subject_match);
		if (conn->subject_match == NULL)
			return -1;
	}

	os_free(conn->altsubject_match);
	conn->altsubject_match = NULL;
	if (altsubject_match) {
		conn->altsubject_match = os_strdup(altsubject_match);
		if (conn->altsubject_match == NULL)
			return -1;
	}

	return 0;
}


int tls_connection_set_verify(void *ssl_ctx, struct tls_connection *conn,
			      int verify_peer)
{
	static int counter = 0;

	if (conn == NULL)
		return -1;

	if (verify_peer) {
		SSL_set_verify(conn->ssl, SSL_VERIFY_PEER |
			       SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
			       SSL_VERIFY_CLIENT_ONCE, tls_verify_cb);
	} else {
		SSL_set_verify(conn->ssl, SSL_VERIFY_NONE, NULL);
	}

	SSL_set_accept_state(conn->ssl);

	/*
	 * Set session id context in order to avoid fatal errors when client
	 * tries to resume a session. However, set the context to a unique
	 * value in order to effectively disable session resumption for now
	 * since not all areas of the server code are ready for it (e.g.,
	 * EAP-TTLS needs special handling for Phase 2 after abbreviated TLS
	 * handshake).
	 */
	counter++;
	SSL_set_session_id_context(conn->ssl,
				   (const unsigned char *) &counter,
				   sizeof(counter));

	return 0;
}


static int tls_connection_client_cert(struct tls_connection *conn,
				      const char *client_cert,
				      const u8 *client_cert_blob,
				      size_t client_cert_blob_len)
{
	if (client_cert == NULL && client_cert_blob == NULL)
		return 0;

	if (client_cert_blob &&
	    SSL_use_certificate_ASN1(conn->ssl, (u8 *) client_cert_blob,
				     client_cert_blob_len) == 1) {
		wpa_printf(MSG_DEBUG, "NucleusSSL: SSL_use_certificate_ASN1 --> "
			   "OK");
		return 0;
	} else if (client_cert_blob) {
		tls_show_errors(MSG_DEBUG, __func__,
				"SSL_use_certificate_ASN1 failed");
	}

	if (client_cert == NULL)
		return -1;

#ifndef NO_STDIO
	if (SSL_use_certificate_file(conn->ssl, client_cert,
				     SSL_FILETYPE_ASN1) == 1) {
		wpa_printf(MSG_DEBUG, "NucleusSSL: SSL_use_certificate_file (DER)"
			   " --> OK");
		return 0;
	} else {
		tls_show_errors(MSG_DEBUG, __func__,
				"SSL_use_certificate_file (DER) failed");
	}

	if (SSL_use_certificate_file(conn->ssl, client_cert,
				     SSL_FILETYPE_PEM) == 1) {
		wpa_printf(MSG_DEBUG, "NucleusSSL: SSL_use_certificate_file (PEM)"
			   " --> OK");
		return 0;
	} else {
		tls_show_errors(MSG_DEBUG, __func__,
				"SSL_use_certificate_file (PEM) failed");
	}
#else /* NO_STDIO */
	wpa_printf(MSG_DEBUG, "NucleusSSL: %s - NO_STDIO", __func__);
#endif /* NO_STDIO */

	return -1;
}


static int tls_global_client_cert(SSL_CTX *ssl_ctx, const char *client_cert)
{
#ifndef NO_STDIO
	if (client_cert == NULL)
		return 0;

	if (SSL_CTX_use_certificate_file(ssl_ctx, client_cert,
					 SSL_FILETYPE_ASN1) != 1 &&
	    SSL_CTX_use_certificate_file(ssl_ctx, client_cert,
					 SSL_FILETYPE_PEM) != 1) {
		tls_show_errors(MSG_INFO, __func__,
				"Failed to load client certificate");
		return -1;
	}
	return 0;
#else /* NO_STDIO */
	if (client_cert == NULL)
		return 0;
	wpa_printf(MSG_DEBUG, "NucleusSSL: %s - NO_STDIO", __func__);
	return -1;
#endif /* NO_STDIO */
}


static int tls_passwd_cb(char *buf, int size, int rwflag, void *password)
{
	if (password == NULL) {
		return 0;
	}
	os_strlcpy(buf, (char *) password, size);
	return os_strlen(buf);
}


static int tls_read_pkcs12(SSL_CTX *ssl_ctx, SSL *ssl, const char *private_key,
			   const char *passwd)
{
	wpa_printf(MSG_INFO, "TLS: PKCS12 support unavailable - cannot read "
		   "p12/pfx files");
	return -1;

}


static int tls_read_pkcs12_blob(SSL_CTX *ssl_ctx, SSL *ssl,
				const u8 *blob, size_t len, const char *passwd)
{
	wpa_printf(MSG_INFO, "TLS: PKCS12 support unavailable - cannot parse "
		   "p12/pfx blobs");
	return -1;
}





static int tls_connection_engine_client_cert(struct tls_connection *conn,
					     const char *cert_id)
{
	return -1;
}


static int tls_connection_engine_ca_cert(void *_ssl_ctx,
					 struct tls_connection *conn,
					 const char *ca_cert_id)
{
	return -1;
}


static int tls_connection_engine_private_key(struct tls_connection *conn)
{

	wpa_printf(MSG_ERROR, "SSL: Configuration uses engine, but "
		   "engine support is not available");
	return -1;

}


static int tls_connection_private_key(void *_ssl_ctx,
				      struct tls_connection *conn,
				      const char *private_key,
				      const char *private_key_passwd,
				      const u8 *private_key_blob,
				      size_t private_key_blob_len)
{
	SSL_CTX *ssl_ctx = _ssl_ctx;
	char *passwd;
	int ok;

	if (private_key == NULL && private_key_blob == NULL)
		return 0;

	if (private_key_passwd) {
		passwd = os_strdup(private_key_passwd);
		if (passwd == NULL)
			return -1;
	} else
		passwd = NULL;

	SSL_CTX_set_default_passwd_cb(ssl_ctx, tls_passwd_cb);
	SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, passwd);

	ok = 0;
	while (private_key_blob) {
		if (SSL_use_PrivateKey_ASN1(EVP_PKEY_RSA, conn->ssl,
					    (u8 *) private_key_blob,
					    private_key_blob_len) == 1) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: SSL_use_PrivateKey_"
				   "ASN1(EVP_PKEY_RSA) --> OK");
			ok = 1;
			break;
		} else {
			tls_show_errors(MSG_DEBUG, __func__,
					"SSL_use_PrivateKey_ASN1(EVP_PKEY_RSA)"
					" failed");
		}

		if (SSL_use_PrivateKey_ASN1(EVP_PKEY_DSA, conn->ssl,
					    (u8 *) private_key_blob,
					    private_key_blob_len) == 1) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: SSL_use_PrivateKey_"
				   "ASN1(EVP_PKEY_DSA) --> OK");
			ok = 1;
			break;
		} else {
			tls_show_errors(MSG_DEBUG, __func__,
					"SSL_use_PrivateKey_ASN1(EVP_PKEY_DSA)"
					" failed");
		}

		if (SSL_use_RSAPrivateKey_ASN1(conn->ssl,
					       (u8 *) private_key_blob,
					       private_key_blob_len) == 1) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: "
				   "SSL_use_RSAPrivateKey_ASN1 --> OK");
			ok = 1;
			break;
		} else {
			tls_show_errors(MSG_DEBUG, __func__,
					"SSL_use_RSAPrivateKey_ASN1 failed");
		}

		if (tls_read_pkcs12_blob(ssl_ctx, conn->ssl, private_key_blob,
					 private_key_blob_len, passwd) == 0) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: PKCS#12 as blob --> "
				   "OK");
			ok = 1;
			break;
		}

		break;
	}

	while (!ok && private_key) {
#ifndef NO_STDIO
		if (SSL_use_PrivateKey_file(conn->ssl, private_key,
					    SSL_FILETYPE_ASN1) == 1) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: "
				   "SSL_use_PrivateKey_File (DER) --> OK");
			ok = 1;
			break;
		} else {
			tls_show_errors(MSG_DEBUG, __func__,
					"SSL_use_PrivateKey_File (DER) "
					"failed");
		}

		if (SSL_use_PrivateKey_file(conn->ssl, private_key,
					    SSL_FILETYPE_PEM) == 1) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: "
				   "SSL_use_PrivateKey_File (PEM) --> OK");
			ok = 1;
			break;
		} else {
			tls_show_errors(MSG_DEBUG, __func__,
					"SSL_use_PrivateKey_File (PEM) "
					"failed");
		}
#else /* NO_STDIO */
		wpa_printf(MSG_DEBUG, "NucleusSSL: %s - NO_STDIO",
			   __func__);
#endif /* NO_STDIO */

		if (tls_read_pkcs12(ssl_ctx, conn->ssl, private_key, passwd)
		    == 0) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: Reading PKCS#12 file "
				   "--> OK");
			ok = 1;
			break;
		}

		if (tls_cryptoapi_cert(conn->ssl, private_key) == 0) {
			wpa_printf(MSG_DEBUG, "NucleusSSL: Using CryptoAPI to "
				   "access certificate store --> OK");
			ok = 1;
			break;
		}

		break;
	}

	if (!ok) {
		wpa_printf(MSG_INFO, "NucleusSSL: Failed to load private key");
		os_free(passwd);
		ERR_clear_error();
		return -1;
	}
	ERR_clear_error();
	SSL_CTX_set_default_passwd_cb(ssl_ctx, NULL);
	os_free(passwd);
	
	if (!SSL_check_private_key(conn->ssl)) {
		tls_show_errors(MSG_INFO, __func__, "Private key failed "
				"verification");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "SSL: Private key loaded successfully");
	return 0;
}


static int tls_global_private_key(SSL_CTX *ssl_ctx, const char *private_key,
				  const char *private_key_passwd)
{
	char *passwd;

	if (private_key == NULL)
		return 0;

	if (private_key_passwd) {
		passwd = os_strdup(private_key_passwd);
		if (passwd == NULL)
			return -1;
	} else
		passwd = NULL;

	SSL_CTX_set_default_passwd_cb(ssl_ctx, tls_passwd_cb);
	SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, passwd);
	if (
#ifndef NO_STDIO
	    SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key,
					SSL_FILETYPE_ASN1) != 1 &&
	    SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key,
					SSL_FILETYPE_PEM) != 1 &&
#endif /* NO_STDIO */
	    tls_read_pkcs12(ssl_ctx, NULL, private_key, passwd)) {
		tls_show_errors(MSG_INFO, __func__,
				"Failed to load private key");
		os_free(passwd);
		ERR_clear_error();
		return -1;
	}
	os_free(passwd);
	ERR_clear_error();
	SSL_CTX_set_default_passwd_cb(ssl_ctx, NULL);
	
	if (!SSL_CTX_check_private_key(ssl_ctx)) {
		tls_show_errors(MSG_INFO, __func__,
				"Private key failed verification");
		return -1;
	}

	return 0;
}


static int tls_connection_dh(struct tls_connection *conn, const char *dh_file)
{
#ifdef NO_DH
	if (dh_file == NULL)
		return 0;
	wpa_printf(MSG_ERROR, "TLS: NucleusSSL does not include DH support, but "
		   "dh_file specified");
	return -1;
#else /* NO_DH */
	DH *dh;
	BIO *bio;

	/* TODO: add support for dh_blob */
	if (dh_file == NULL)
		return 0;
	if (conn == NULL)
		return -1;

	bio = BIO_new_file(dh_file, "r");
	if (bio == NULL) {
		wpa_printf(MSG_INFO, "TLS: Failed to open DH file '%s': %s",
			   dh_file, ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
	BIO_free(bio);
#ifndef NO_DSA
	while (dh == NULL) {
		DSA *dsa;
		wpa_printf(MSG_DEBUG, "TLS: Failed to parse DH file '%s': %s -"
			   " trying to parse as DSA params", dh_file,
			   ERR_error_string(ERR_get_error(), NULL));
		bio = BIO_new_file(dh_file, "r");
		if (bio == NULL)
			break;
		dsa = PEM_read_bio_DSAparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		if (!dsa) {
			wpa_printf(MSG_DEBUG, "TLS: Failed to parse DSA file "
				   "'%s': %s", dh_file,
				   ERR_error_string(ERR_get_error(), NULL));
			break;
		}

		wpa_printf(MSG_DEBUG, "TLS: DH file in DSA param format");
		dh = DSA_dup_DH(dsa);
		DSA_free(dsa);
		if (dh == NULL) {
			wpa_printf(MSG_INFO, "TLS: Failed to convert DSA "
				   "params into DH params");
			break;
		}
		break;
	}
#endif /* !NO_DSA */
	if (dh == NULL) {
		wpa_printf(MSG_INFO, "TLS: Failed to read/parse DH/DSA file "
			   "'%s'", dh_file);
		return -1;
	}

	if (SSL_set_tmp_dh(conn->ssl, dh) != 1) {
		wpa_printf(MSG_INFO, "TLS: Failed to set DH params from '%s': "
			   "%s", dh_file,
			   ERR_error_string(ERR_get_error(), NULL));
		DH_free(dh);
		return -1;
	}
	DH_free(dh);
	return 0;
#endif /* NO_DH */
}


static int tls_global_dh(SSL_CTX *ssl_ctx, const char *dh_file)
{
#ifdef NO_DH
	if (dh_file == NULL)
		return 0;
	wpa_printf(MSG_ERROR, "TLS: NucleusSSL does not include DH support, but "
		   "dh_file specified");
	return -1;
#else /* NO_DH */
	DH *dh;
	BIO *bio;

	/* TODO: add support for dh_blob */
	if (dh_file == NULL)
		return 0;
	if (ssl_ctx == NULL)
		return -1;

	bio = BIO_new_file(dh_file, "r");
	if (bio == NULL) {
		wpa_printf(MSG_INFO, "TLS: Failed to open DH file '%s': %s",
			   dh_file, ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
	BIO_free(bio);
#ifndef NO_DSA
	while (dh == NULL) {
		DSA *dsa;
		wpa_printf(MSG_DEBUG, "TLS: Failed to parse DH file '%s': %s -"
			   " trying to parse as DSA params", dh_file,
			   ERR_error_string(ERR_get_error(), NULL));
		bio = BIO_new_file(dh_file, "r");
		if (bio == NULL)
			break;
		dsa = PEM_read_bio_DSAparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		if (!dsa) {
			wpa_printf(MSG_DEBUG, "TLS: Failed to parse DSA file "
				   "'%s': %s", dh_file,
				   ERR_error_string(ERR_get_error(), NULL));
			break;
		}

		wpa_printf(MSG_DEBUG, "TLS: DH file in DSA param format");
		dh = DSA_dup_DH(dsa);
		DSA_free(dsa);
		if (dh == NULL) {
			wpa_printf(MSG_INFO, "TLS: Failed to convert DSA "
				   "params into DH params");
			break;
		}
		break;
	}
#endif /* !NO_DSA */
	if (dh == NULL) {
		wpa_printf(MSG_INFO, "TLS: Failed to read/parse DH/DSA file "
			   "'%s'", dh_file);
		return -1;
	}

	if (SSL_CTX_set_tmp_dh(ssl_ctx, dh) != 1) {
		wpa_printf(MSG_INFO, "TLS: Failed to set DH params from '%s': "
			   "%s", dh_file,
			   ERR_error_string(ERR_get_error(), NULL));
		DH_free(dh);
		return -1;
	}
	DH_free(dh);
	return 0;
#endif /* NO_DH */
}


int tls_connection_get_keys(void *ssl_ctx, struct tls_connection *conn,
			    struct tls_keys *keys)
{
	SSL *ssl;

	if (conn == NULL || keys == NULL)
		return -1;
	ssl = conn->ssl;
	if (ssl == NULL || ssl->s3 == NULL || ssl->session == NULL)
		return -1;

	os_memset(keys, 0, sizeof(*keys));
	keys->master_key = ssl->session->master_key;
	keys->master_key_len = ssl->session->master_key_length;
	keys->client_random = ssl->s3->client_random;
	keys->client_random_len = SSL3_RANDOM_SIZE;
	keys->server_random = ssl->s3->server_random;
	keys->server_random_len = SSL3_RANDOM_SIZE;

	return 0;
}


int tls_connection_prf(void *tls_ctx, struct tls_connection *conn,
		       const char *label, int server_random_first,
		       u8 *out, size_t out_len)
{
	return -1;
}


u8 * tls_connection_handshake(void *ssl_ctx, struct tls_connection *conn,
			      const u8 *in_data, size_t in_len,
			      size_t *out_len, u8 **appl_data,
			      size_t *appl_data_len)
{
	int res;
	u8 *out_data;

	if (appl_data)
		*appl_data = NULL;

	/*
	 * Give TLS handshake data from the server (if available) to NucleusSSL
	 * for processing.
	 */
	if (in_data &&
	    BIO_write(conn->ssl_in, (CHAR *)in_data, in_len) < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Handshake failed - BIO_write");
		return NULL;
	}

	/* Initiate TLS handshake or continue the existing handshake */
	res = SSL_connect(conn->ssl);
	if (res != 1) {
		int err = SSL_get_error(conn->ssl, res);
		if (err == SSL_ERROR_WANT_READ)
			wpa_printf(MSG_DEBUG, "SSL: SSL_connect - want "
				   "more data");
		else if (err == SSL_ERROR_WANT_WRITE)
			wpa_printf(MSG_DEBUG, "SSL: SSL_connect - want to "
				   "write");
		else {
			tls_show_errors(MSG_INFO, __func__, "SSL_connect");
			conn->failed++;
		}
	}

	/* Get the TLS handshake data to be sent to the server */
	res = BIO_ctrl_pending(conn->ssl_out);
	wpa_printf(MSG_DEBUG, "SSL: %d bytes pending from ssl_out", res);
	out_data = os_malloc(res == 0 ? 1 : res);
	if (out_data == NULL) {
		wpa_printf(MSG_DEBUG, "SSL: Failed to allocate memory for "
			   "handshake output (%d bytes)", res);
		if (BIO_reset(conn->ssl_out) < 0) {
			tls_show_errors(MSG_INFO, __func__,
					"BIO_reset failed");
		}
		*out_len = 0;
		return NULL;
	}
	res = res == 0 ? 0 : BIO_read(conn->ssl_out, out_data, res);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Handshake failed - BIO_read");
		if (BIO_reset(conn->ssl_out) < 0) {
			tls_show_errors(MSG_INFO, __func__,
					"BIO_reset failed");
		}
		*out_len = 0;
		return NULL;
	}
	*out_len = res;

	if (SSL_is_init_finished(conn->ssl) && appl_data) {
		*appl_data = os_malloc(in_len);
		if (*appl_data) {
			res = SSL_read(conn->ssl, (CHAR *)*appl_data, in_len);
			if (res < 0) {
				tls_show_errors(MSG_INFO, __func__,
						"Failed to read possible "
						"Application Data");
				os_free(*appl_data);
				*appl_data = NULL;
			} else if (res == 0) {
				os_free(*appl_data);
				*appl_data = 0;
			} else {
				*appl_data_len = res;
				wpa_hexdump_key(MSG_MSGDUMP, "SSL: Application"
						" Data in Finish message",
						*appl_data, *appl_data_len);
			}
		}
	}

	return out_data;
}


u8 * tls_connection_server_handshake(void *ssl_ctx,
				     struct tls_connection *conn,
				     const u8 *in_data, size_t in_len,
				     size_t *out_len)
{
	int res;
	u8 *out_data;

	/*
	 * Give TLS handshake data from the client (if available) to NucleusSSL
	 * for processing.
	 */
	if (in_data &&
	    BIO_write(conn->ssl_in, (CHAR *)in_data, in_len) < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Handshake failed - BIO_write");
		return NULL;
	}

	/* Initiate TLS handshake or continue the existing handshake */
	res = SSL_accept(conn->ssl);
	if (res != 1) {
		int err = SSL_get_error(conn->ssl, res);
		if (err == SSL_ERROR_WANT_READ)
			wpa_printf(MSG_DEBUG, "SSL: SSL_accept - want "
				   "more data");
		else if (err == SSL_ERROR_WANT_WRITE)
			wpa_printf(MSG_DEBUG, "SSL: SSL_accept - want to "
				   "write");
		else {
			tls_show_errors(MSG_INFO, __func__, "SSL_accept");
			return NULL;
		}
	}

	/* Get the TLS handshake data to be sent to the client */
	res = BIO_ctrl_pending(conn->ssl_out);
	wpa_printf(MSG_DEBUG, "SSL: %d bytes pending from ssl_out", res);
	out_data = os_malloc(res == 0 ? 1 : res);
	if (out_data == NULL) {
		wpa_printf(MSG_DEBUG, "SSL: Failed to allocate memory for "
			   "handshake output (%d bytes)", res);
		if (BIO_reset(conn->ssl_out) < 0) {
			tls_show_errors(MSG_INFO, __func__,
					"BIO_reset failed");
		}
		*out_len = 0;
		return NULL;
	}
	res = res == 0 ? 0 : BIO_read(conn->ssl_out, out_data, res);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Handshake failed - BIO_read");
		if (BIO_reset(conn->ssl_out) < 0) {
			tls_show_errors(MSG_INFO, __func__,
					"BIO_reset failed");
		}
		*out_len = 0;
		return NULL;
	}
	*out_len = res;
	return out_data;
}


int tls_connection_encrypt(void *ssl_ctx, struct tls_connection *conn,
			   const u8 *in_data, size_t in_len,
			   u8 *out_data, size_t out_len)
{
	int res;

	if (conn == NULL)
		return -1;

	/* Give plaintext data for NucleusSSL to encrypt into the TLS tunnel. */
	if ((res = BIO_reset(conn->ssl_in)) < 0 ||
	    (res = BIO_reset(conn->ssl_out)) < 0) {
		tls_show_errors(MSG_INFO, __func__, "BIO_reset failed");
		return res;
	}
	res = SSL_write(conn->ssl, (CHAR *)in_data, in_len);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Encryption failed - SSL_write");
		return res;
	}

	/* Read encrypted data to be sent to the server */
	res = BIO_read(conn->ssl_out, out_data, out_len);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Encryption failed - BIO_read");
		return res;
	}

	return res;
}


int tls_connection_decrypt(void *ssl_ctx, struct tls_connection *conn,
			   const u8 *in_data, size_t in_len,
			   u8 *out_data, size_t out_len)
{
	int res;

	/* Give encrypted data from TLS tunnel for NucleusSSL to decrypt. */
	res = BIO_write(conn->ssl_in, (CHAR *)in_data, in_len);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Decryption failed - BIO_write");
		return res;
	}
	if (BIO_reset(conn->ssl_out) < 0) {
		tls_show_errors(MSG_INFO, __func__, "BIO_reset failed");
		return res;
	}

	/* Read decrypted data for further processing */
	res = SSL_read(conn->ssl, (CHAR *)out_data, out_len);
	if (res < 0) {
		tls_show_errors(MSG_INFO, __func__,
				"Decryption failed - SSL_read");
		return res;
	}

	return res;
}


int tls_connection_resumed(void *ssl_ctx, struct tls_connection *conn)
{
	return conn ? conn->ssl->hit : 0;
}


int tls_connection_set_cipher_list(void *tls_ctx, struct tls_connection *conn,
				   u8 *ciphers)
{
	char buf[100], *pos, *end;
	u8 *c;
	int ret;

	if (conn == NULL || conn->ssl == NULL || ciphers == NULL)
		return -1;

	buf[0] = '\0';
	pos = buf;
	end = pos + sizeof(buf);

	c = ciphers;
	while (*c != TLS_CIPHER_NONE) {
		const char *suite;

		switch (*c) {
		case TLS_CIPHER_RC4_SHA:
			suite = "RC4-SHA";
			break;
		case TLS_CIPHER_AES128_SHA:
			suite = "AES128-SHA";
			break;
		case TLS_CIPHER_RSA_DHE_AES128_SHA:
			suite = "DHE-RSA-AES128-SHA";
			break;
		case TLS_CIPHER_ANON_DH_AES128_SHA:
			suite = "ADH-AES128-SHA";
			break;
		default:
			wpa_printf(MSG_DEBUG, "TLS: Unsupported "
				   "cipher selection: %d", *c);
			return -1;
		}
		ret = os_snprintf(pos, end - pos, ":%s", suite);
		if (ret < 0 || ret >= end - pos)
			break;
		pos += ret;

		c++;
	}

	wpa_printf(MSG_DEBUG, "NucleusSSL: cipher suites: %s", buf + 1);

	if (SSL_set_cipher_list(conn->ssl, buf + 1) != 1) {
		tls_show_errors(MSG_INFO, __func__,
				"Cipher suite configuration failed");
		return -1;
	}

	return 0;
}


int tls_get_cipher(void *ssl_ctx, struct tls_connection *conn,
		   char *buf, size_t buflen)
{
	const char *name;
	if (conn == NULL || conn->ssl == NULL)
		return -1;

	name = SSL_get_cipher(conn->ssl);
	if (name == NULL)
		return -1;

	os_strlcpy(buf, name, buflen);
	return 0;
}

#if defined(EAP_FAST) || defined(EAP_FAST_DYNAMIC)
/* ClientHello TLS extensions require a patch to openssl, so this function is
 * commented out unless explicitly needed for EAP-FAST in order to be able to
 * build this file with unmodified openssl. */
int tls_connection_client_hello_ext(void *ssl_ctx, struct tls_connection *conn,
				    int ext_type, const u8 *data,
				    size_t data_len)
{
	if (conn == NULL || conn->ssl == NULL || ext_type != 35)
		return -1;

#ifdef CONFIG_OPENSSL_TICKET_OVERRIDE
	if (SSL_set_session_ticket_ext(conn->ssl, (void *) data,
				       data_len) != 1)
		return -1;
#else /* CONFIG_OPENSSL_TICKET_OVERRIDE */
	if (SSL_set_hello_extension(conn->ssl, ext_type, (void *) data,
				    data_len) != 1)
		return -1;
#endif /* CONFIG_OPENSSL_TICKET_OVERRIDE */

	return 0;
}
#endif /* EAP_FAST || EAP_FAST_DYNAMIC */


int tls_connection_get_failed(void *ssl_ctx, struct tls_connection *conn)
{
	if (conn == NULL)
		return -1;
	return conn->failed;
}


int tls_connection_get_read_alerts(void *ssl_ctx, struct tls_connection *conn)
{
	if (conn == NULL)
		return -1;
	return conn->read_alerts;
}


int tls_connection_get_write_alerts(void *ssl_ctx, struct tls_connection *conn)
{
	if (conn == NULL)
		return -1;
	return conn->write_alerts;
}


int tls_connection_set_params(void *tls_ctx, struct tls_connection *conn,
			      const struct tls_connection_params *params)
{
	int ret;
	unsigned long err;

	if (conn == NULL)
		return -1;

	while ((err = ERR_get_error())) {
		wpa_printf(MSG_INFO, "%s: Clearing pending SSL error: %s",
			   __func__, ERR_error_string(err, NULL));
	}

	if (params->engine) {
		wpa_printf(MSG_DEBUG, "SSL: Initializing TLS engine");
		ret = tls_engine_init(conn, params->engine_id, params->pin,
				      params->key_id, params->cert_id,
				      params->ca_cert_id);
		if (ret)
			return ret;
	}
	if (tls_connection_set_subject_match(conn,
					     params->subject_match,
					     params->altsubject_match))
		return -1;

	if (params->engine && params->ca_cert_id) {
		if (tls_connection_engine_ca_cert(tls_ctx, conn,
						  params->ca_cert_id))
			return TLS_SET_PARAMS_ENGINE_PRV_VERIFY_FAILED;
	} else if (tls_connection_ca_cert(tls_ctx, conn, params->ca_cert,
					  params->ca_cert_blob,
					  params->ca_cert_blob_len,
					  params->ca_path))
		return -1;

	if (params->engine && params->cert_id) {
		if (tls_connection_engine_client_cert(conn, params->cert_id))
			return TLS_SET_PARAMS_ENGINE_PRV_VERIFY_FAILED;
	} else if (tls_connection_client_cert(conn, params->client_cert,
					      params->client_cert_blob,
					      params->client_cert_blob_len))
		return -1;

	if (params->engine && params->key_id) {
		wpa_printf(MSG_DEBUG, "TLS: Using private key from engine");
		if (tls_connection_engine_private_key(conn))
			return TLS_SET_PARAMS_ENGINE_PRV_VERIFY_FAILED;
	} else if (tls_connection_private_key(tls_ctx, conn,
					      params->private_key,
					      params->private_key_passwd,
					      params->private_key_blob,
					      params->private_key_blob_len)) {
		wpa_printf(MSG_INFO, "TLS: Failed to load private key '%s'",
			   params->private_key);
		return -1;
	}

	if (tls_connection_dh(conn, params->dh_file)) {
		wpa_printf(MSG_INFO, "TLS: Failed to load DH file '%s'",
			   params->dh_file);
		return -1;
	}

	tls_get_errors(tls_ctx);

	return 0;
}


int tls_global_set_params(void *tls_ctx,
			  const struct tls_connection_params *params)
{
	SSL_CTX *ssl_ctx = tls_ctx;
	unsigned long err;

	while ((err = ERR_get_error())) {
		wpa_printf(MSG_INFO, "%s: Clearing pending SSL error: %s",
			   __func__, ERR_error_string(err, NULL));
	}

	if (tls_global_ca_cert(ssl_ctx, params->ca_cert))
		return -1;

	if (tls_global_client_cert(ssl_ctx, params->client_cert))
		return -1;

	if (tls_global_private_key(ssl_ctx, params->private_key,
				   params->private_key_passwd))
		return -1;

	if (tls_global_dh(ssl_ctx, params->dh_file)) {
		wpa_printf(MSG_INFO, "TLS: Failed to load DH file '%s'",
			   params->dh_file);
		return -1;
	}

	return 0;
}


int tls_connection_get_keyblock_size(void *tls_ctx,
				     struct tls_connection *conn)
{
	const EVP_CIPHER *c;
	const EVP_MD *h;

	if (conn == NULL || conn->ssl == NULL ||
	    conn->ssl->enc_read_ctx == NULL ||
	    conn->ssl->enc_read_ctx->cipher == NULL ||
	    conn->ssl->read_hash == NULL)
		return -1;

	c = conn->ssl->enc_read_ctx->cipher;

	h = conn->ssl->read_hash;


	return 2 * (EVP_CIPHER_key_length(c) +
		    EVP_MD_size(h) +
		    EVP_CIPHER_iv_length(c));
}


unsigned int tls_capabilities(void *tls_ctx)
{
	return 0;
}


int tls_connection_set_ia(void *tls_ctx, struct tls_connection *conn,
			  int tls_ia)
{
	return -1;
}


int tls_connection_ia_send_phase_finished(void *tls_ctx,
					  struct tls_connection *conn,
					  int final,
					  u8 *out_data, size_t out_len)
{
	return -1;
}


int tls_connection_ia_final_phase_finished(void *tls_ctx,
					   struct tls_connection *conn)
{
	return -1;
}


int tls_connection_ia_permute_inner_secret(void *tls_ctx,
					   struct tls_connection *conn,
					   const u8 *key, size_t key_len)
{
	return -1;
}


#if defined(EAP_FAST) || defined(EAP_FAST_DYNAMIC)
/* Pre-shared secred requires a patch to openssl, so this function is
 * commented out unless explicitly needed for EAP-FAST in order to be able to
 * build this file with unmodified openssl. */

static int tls_sess_sec_cb(SSL *s, void *secret, int *secret_len,
			   STACK_OF(SSL_CIPHER) *peer_ciphers,
			   SSL_CIPHER **cipher, void *arg)
{
	struct tls_connection *conn = arg;
	int ret;

	if (conn == NULL || conn->session_ticket_cb == NULL)
		return 0;

	ret = conn->session_ticket_cb(conn->session_ticket_cb_ctx,
				      conn->session_ticket,
				      conn->session_ticket_len,
				      s->s3->client_random,
				      s->s3->server_random, secret);
	os_free(conn->session_ticket);
	conn->session_ticket = NULL;

	if (ret <= 0)
		return 0;

	*secret_len = SSL_MAX_MASTER_KEY_LENGTH;
	return 1;
}




static int tls_hello_ext_cb(SSL *s, TLS_EXTENSION *ext, void *arg)
{
	struct tls_connection *conn = arg;

	if (conn == NULL || conn->session_ticket_cb == NULL)
		return 0;

	wpa_printf(MSG_DEBUG, "OpenSSL: %s: type=%d length=%d", __func__,
		   ext->type, ext->length);

	os_free(conn->session_ticket);
	conn->session_ticket = NULL;

	if (ext->type == 35) {
		wpa_hexdump(MSG_DEBUG, "OpenSSL: ClientHello SessionTicket "
			    "extension", ext->data, ext->length);
		conn->session_ticket = os_malloc(ext->length);
		if (conn->session_ticket == NULL)
			return SSL_AD_INTERNAL_ERROR;

		os_memcpy(conn->session_ticket, ext->data, ext->length);
		conn->session_ticket_len = ext->length;
	}

	return 0;
}

#endif /* EAP_FAST || EAP_FAST_DYNAMIC */


int tls_connection_set_session_ticket_cb(void *tls_ctx,
					 struct tls_connection *conn,
					 tls_session_ticket_cb cb,
					 void *ctx)
{
#if defined(EAP_FAST) || defined(EAP_FAST_DYNAMIC)
	conn->session_ticket_cb = cb;
	conn->session_ticket_cb_ctx = ctx;

	if (cb) {
		if (SSL_set_session_secret_cb(conn->ssl, tls_sess_sec_cb,
					      conn) != 1)
			return -1;

		if (SSL_set_hello_extension_cb(conn->ssl, tls_hello_ext_cb,
					       conn) != 1)
			return -1;

	} else {
		if (SSL_set_session_secret_cb(conn->ssl, NULL, NULL) != 1)
			return -1;

		if (SSL_set_hello_extension_cb(conn->ssl, NULL, NULL) != 1)
			return -1;


	}

	return 0;
#else /* EAP_FAST || EAP_FAST_DYNAMIC */
	return -1;
#endif /* EAP_FAST || EAP_FAST_DYNAMIC */
}
#endif /* CONFIG_TLS_INTERNAL */
