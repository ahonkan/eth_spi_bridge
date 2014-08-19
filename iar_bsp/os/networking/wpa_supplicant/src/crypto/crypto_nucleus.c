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
 * WPA Supplicant / Crypto wrapper for internal crypto implementation
 * Copyright (c) 2006-2007, Jouni Malinen <j@w1.fi>
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
#include "crypto/ncs_api.h"       /* The NCS interface. */
#include "crypto/aes.h"             /* The NCS AES interface. */

#include "common.h"
#include "crypto.h"
#include "md5.h"
#include "sha1.h"
#include "rc4.h"

struct crypto_hash {
	enum crypto_hash_alg alg;
	NCS_HASH_CTX         u;
	UINT8                key[64];
	size_t               key_len;
};


#ifndef CONFIG_CRYPTO_INTERNAL 
/**
 * md4_vector - MD4 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 */
void md4_vector(size_t num_elem, const UINT8 *addr[],
				const size_t *len, UINT8 *mac)
{
	NCS_HASH_CTX context;
	UINT8 mac_len;
	size_t i;

	/* Initialize the hash context. */
	if (NCS_Hash_Init(NCS_MD4, &context) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in initialization of MD4.");

	for (i = 0; i < num_elem; i++) {
		if (NCS_Hash_Update(&context,(UINT8*)addr[i], len[i])
							!= NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash update of MD4.");
	}

	mac_len = 0;

	/* Finalize the hashing to compute the digest. */
	if (NCS_Hash_Final(&context, mac, &mac_len) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash final of MD4.");
}


/**
 * md5_vector - MD5 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 */
void md5_vector(size_t num_elem, const UINT8 *addr[],
				const size_t *len, UINT8 *mac)
{
	NCS_HASH_CTX context;
	UINT8 mac_len;
	size_t i;

	/* Initialize the hash context. */
	if (NCS_Hash_Init(NCS_MD5, &context) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in initialization of MD5");

	for (i = 0; i < num_elem; i++) {
		if (NCS_Hash_Update(&context, (UINT8*)addr[i], len[i])
							!= NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash update of MD5.");
	}

	mac_len = 0;

	/* Finalize the hashing to compute the digest. */
	if (NCS_Hash_Final(&context, mac, &mac_len) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash final of MD5.");       
}


/**
 * sha1_vector - SHA-1 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 */
void sha1_vector(size_t num_elem, const UINT8 *addr[],
				const size_t *len, UINT8 *mac)
{
	NCS_HASH_CTX context;
	UINT8 mac_len;
	size_t i;

	/* Initialize the hash context. */
	if (NCS_Hash_Init(NCS_SHA1, &context) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in initialization of SHA1.");

	for (i = 0; i < num_elem; i++) {
		if (NCS_Hash_Update(&context, (UINT8*)addr[i], len[i])
							!= NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WPA-NCS: Error in update of SHA1.");
	}

	mac_len = 0;

	/* Finalize the hashing to compute the digest. */
	if (NCS_Hash_Final(&context, mac, &mac_len) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash final of SHA1.");
}


/**
 * sha256_vector - SHA256 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 */
void sha256_vector(size_t num_elem, const UINT8 *addr[],
					const size_t *len, UINT8 *mac)
{
	NCS_HASH_CTX context;
	UINT8 mac_len;
	size_t i;

	/* Initialize the hash context. */
	if (NCS_Hash_Init(NCS_SHA256, &context) != NU_SUCCESS)
		wpa_printf(MSG_ERROR,
				"WPA-NCS: Error in hash initialization of SHA256.");

	for (i = 0; i < num_elem; i++) {
		if (NCS_Hash_Update(&context, (UINT8*)addr[i], len[i])
							!= NU_SUCCESS)
			wpa_printf(MSG_ERROR, "WPA-NCS: Error in update of SHA256.");
	}

	mac_len = 0;

	/* Finalize the hashing to compute the digest. */
	if (NCS_Hash_Final(&context, mac, &mac_len) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in hash final of SHA256.");
}


void * aes_encrypt_init(const UINT8 *key, size_t len)
{
	AES_KEY *ak;
	ak = os_malloc(sizeof(*ak));
	if (ak == NULL)
		return NULL;
	if (AES_Set_Encrypt_Key(key, 8 * len, ak) < 0) {
		os_free(ak);
		return NULL;
	}
	return ak;
}


void aes_encrypt(void *ctx, const UINT8 *plain, UINT8 *crypt)
{
	AES_Encrypt(plain, crypt, ctx);
}


void aes_encrypt_deinit(void *ctx)
{
	os_free(ctx);
}


void * aes_decrypt_init(const UINT8 *key, size_t len)
{
	AES_KEY *ak;
	ak = os_malloc(sizeof(*ak));
	if (ak == NULL)
		return NULL;
	if (AES_Set_Decrypt_Key(key, 8 * len, ak) < 0) {
		os_free(ak);
		return NULL;
	}
	return ak;
}


void aes_decrypt(void *ctx, const UINT8 *crypt, UINT8 *plain)
{
	AES_Decrypt(crypt, plain, ctx);
}


void aes_decrypt_deinit(void *ctx)
{
	os_free(ctx);
}


/**
 * des_encrypt - Encrypt one block with DES
 * @clear: 8 octets (in)
 * @key: 7 octets (in) (no parity bits included)
 * @cypher: 8 octets (out)
 */
void des_encrypt(const UINT8 *clear, const UINT8 *key, UINT8 *cypher)
{
	int i;
	NCS_ENCRYPT_REQ req;
	UINT8 pkey[8], next, tmp;

	/* Add parity bits to the key */
	next = 0;
	for (i = 0; i < 7; i++) {
		tmp = key[i];
		pkey[i] = (tmp >> i) | next | 1;
		next = tmp << (7 - i);
	}
	pkey[i] = next | 1;

	/* Specify encryption mode of operation. */
	req.ncs_operation = NCS_ENCRYPT;

	/* Set encryption algorithm to des. */
	req.ncs_algo = NCS_DES;

	/* Encrypt in ECB Mode. */
	req.ncs_mode = NCS_ECB_MODE;

	/* Specify the key and set key schedule to NULL. */
	req.ncs_key = pkey;
	req.ncs_key_schedule = NU_NULL;

	/* Request default key length. */
	req.ncs_key_len = 0;

	/* Specify the initialization vector. */
	req.ncs_iv = NU_NULL;

	/* Copy the plaintext from the global variable because
	 * it would be overwritten. */
	os_memcpy(cypher, clear, 8);

	/* Specify the plain text. */
	req.ncs_buffer = cypher;
	req.ncs_text_len = 8;
	req.ncs_buffer_len = 8;

	/* Make the request for encryption. On return, req.ncs_buffer
	 * will contain the cipher text and req.ncs_text_len will
	 * contain the length of cipher text. */
	if (NCS_Encrypt(&req) != NU_SUCCESS)
		wpa_printf(MSG_ERROR, "WPA-NCS: Error in encryption.");
}


#if defined(EAP_FAST) || defined(CONFIG_WPS)
int crypto_mod_exp(const UINT8 *base, size_t base_len,
		   const UINT8 *power, size_t power_len,
		   const UINT8 *modulus, size_t modulus_len,
		   UINT8 *result, size_t *result_len)
{
	NCS_BIGNUM *bn_base, *bn_exp, *bn_modulus, *bn_result;
	int ret = -1;
	NCS_BN_CTX *ctx;

	ctx = NCS_BN_CTX_new();
	if (ctx == NULL)
		return -1;

	bn_base = NCS_BN_bin2bn(base, base_len, NULL);
	bn_exp = NCS_BN_bin2bn(power, power_len, NULL);
	bn_modulus = NCS_BN_bin2bn(modulus, modulus_len, NULL);
	bn_result = NCS_BN_new();

	if (bn_base == NULL || bn_exp == NULL || bn_modulus == NULL ||
		bn_result == NULL)
		goto error;

	if (NCS_BN_mod_exp(bn_result, bn_base, bn_exp, bn_modulus, ctx) != 1)
		goto error;

	*result_len = NCS_BN_bn2bin(bn_result, result);
	ret = 0;

error:
	NCS_BN_free(bn_base);
	NCS_BN_free(bn_exp);
	NCS_BN_free(bn_modulus);
	NCS_BN_free(bn_result);
	NCS_BN_CTX_free(ctx);
	return ret;
}

#endif /* EAP_FAST || CONFIG_WPS */
#endif /* !CONFIG_CRYPTO_INTERNAL */
