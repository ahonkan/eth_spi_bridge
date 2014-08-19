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
*       ike_cfg.h
*
* COMPONENT
*
*       IKE - Configuration
*
* DESCRIPTION
*
*       This file contains macros used to configure IKE.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_CFG_H
#define IKE_CFG_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */


/**** General IKE Configuration. ****/

/* Resource acquisition timeout. Specifies the number of timer
 * ticks IKE will wait on a resource before timing out. Valid
 * values are from 1 to 4,294,967,293. NU_SUSPEND can also be
 * specified to indicate indefinite suspension.
 */
#define IKE_TIMEOUT                     NU_SUSPEND

/* Blocking mode for IKE exchange. Set this to NU_TRUE to
 * enable IKE exchanges in blocking mode. IPsec always
 * initiates IKE exchanges in non-blocking mode so this
 * should only be enabled if an application directly calls
 * IKE_Initiate with the suspend parameter in the request
 * set to a value other than NU_NO_SUSPEND.
 */
#define IKE_ENABLE_BLOCKING_REQUEST     NU_FALSE

/* Maximum number of blocking IKE exchange requests. Valid
 * values are from 1 to 31. This macro is only used if
 * IKE_ENABLE_BLOCKING_REQUEST is set to NU_TRUE.
 */
#define IKE_MAX_WAIT_EVENTS             5

/**** IKE Debug Configuration. ****/

/* Debugging support. Set this to NU_TRUE to enable
 * extensive error checking and generation of detailed
 * informational messages using Nucleus NET logging.
 */
#define IKE_DEBUG                       NU_TRUE

/* Macro for logging informational messages. This could
 * be set to a target dependent string output function.
 * The default action is to log the message using
 * NLOG_Error_Log.
 */
#if (IKE_DEBUG == NU_TRUE)
#define IKE_DEBUG_LOG(str)              NLOG_Error_Log((str),           \
                                            NERR_INFORMATIONAL,         \
                                            __FILE__, __LINE__)
#else
#define IKE_DEBUG_LOG(str)
#endif

#define IKE_INCLUDE_VERSION_2           NU_TRUE

/**** IKEv1 Exchange Configuration. ****/

/* Support for Main Mode and Aggressive Mode. Both
 * are Phase 1 exchanges and at least one of them
 * MUST be included, to allow a Phase 1 exchange.
 * Aggressive Mode is faster than Main Mode, but
 * also requires more memory.
 */
#define IKE_INCLUDE_MAIN_MODE           NU_TRUE
#define IKE_INCLUDE_AGGR_MODE           NU_FALSE

/* Informational Mode of exchange. Enable this to allow
 * sending and receiving ISAKMP Informational Mode messages.
 */
#define IKE_INCLUDE_INFO_MODE           NU_TRUE

/* Pre-Shared Key Authentication Method. Enable this to
 * allow pre-shared key based authentication in Phase 1.
 */
#define IKE_INCLUDE_PSK_AUTH            NU_TRUE

/* Signature based Authentication Method. Enable this to
 * allow Signature based authentication in Phase 1.
 */
#define IKE_INCLUDE_SIG_AUTH            NU_FALSE

/* Support for INITIAL-CONTACT. This allows synchronization
 * of SAs when a node unexpectedly goes down. All invalid
 * IPsec and IKE SAs resulting from such failure are deleted.
 */
#define IKE_ENABLE_INITIAL_CONTACT      NU_TRUE

/* Domain name support in Identification payload. This data
 * is used during authentication of local and remote nodes
 * and for looking up pre-shared keys. Set this macro to
 * NU_TRUE to enable support for Domain Names (FQDN) and
 * User Domain Names (USER_FQDN) in IKE. If this support is
 * disabled, only single IPv4 and IPv6 addresses can be
 * specified as the local and remote IDs in IKE Policies
 * and pre-shared keys.
 */
#define IKE_ENABLE_DOMAIN_NAME_ID       NU_TRUE

/* Retaining last message of an exchange. In both Phase 1
 * and Phase 2 exchanges, the last message of the
 * exchange can be lost during transmission. Enabling this
 * macro allows retaining the last message of the exchange
 * so that it could be retransmitted if it is lost. The
 * message is retained until the exchange times out. Note
 * that enabling this option consumes more memory and is
 * not recommended if a large number of IKE exchanges are
 * expected to be carried out simultaneously. Note that
 * this applies to the last message of an exchange only.
 * All other messages are always retained until a reply
 * is received.
 */
#define IKE_RETAIN_LAST_MESSAGE         NU_TRUE

/* Default exchange mode of Initiator. If both Main mode
 * and Aggressive mode are allowed in phase 1, then this
 * macro specifies the preferred mode of exchange. Valid
 * values are:
 *    - IKE_XCHG_MAIN
 *    - IKE_XCHG_AGGR
 */
#define IKE_PHASE1_DEFAULT_XCHG         IKE_XCHG_MAIN

/* Support for additional Oakley Groups. Set these to
 * NU_TRUE to enable support for more Oakley Groups
 * used by Diffie-Hellman shared key generation in phase 1
 * and phase 2 (optional). The First Oakley Group (MODP 768)
 * is always supported by IKE but support for these groups
 * is optional.
 */
#define IKE_INCLUDE_MODP_1024           NU_TRUE
#define IKE_INCLUDE_MODP_1536           NU_FALSE
#define IKE_INCLUDE_MODP_2048           NU_FALSE
#define IKE_INCLUDE_MODP_3072           NU_FALSE
#define IKE_INCLUDE_MODP_4096           NU_FALSE
#define IKE_INCLUDE_MODP_6144           NU_FALSE
#define IKE_INCLUDE_MODP_8192           NU_FALSE
#define IKE_INCLUDE_MODP_1024_160_POS   NU_FALSE
#define IKE_INCLUDE_MODP_2048_224_POS   NU_FALSE
#define IKE_INCLUDE_MODP_2048_256_POS   NU_FALSE

/**** IKEv2 Exchange Configuration. ****/

/* Timeout for IKE SA is not negotiated and is configured locally.
 * This value specifies the lifetime of an IKE SA. An event is fired
 * at expiration of this time and appropriate action is taken. */
#define IKE2_SA_TIMEOUT                 28800 /* Seconds */

/* Maximum packet size (in octets) allowed during this exchange. This
 * value should not be set to a value smaller than 1280. RFC4306 mandates
 * support for packets at least 1280 octets in size. 3000 is preferred
 * value. It can be set to a higher value but peer should not be expected
 * to support larger sizes.
 */
#define IKE2_MAX_PACKET_LEN             3000

/**** IKE Timers and Lifetimes. ****/

/* Difference between IPsec SA soft and hard lifetimes, in
 * seconds. The soft lifetime would expire this number of
 * seconds before the hard lifetime. If the hard lifetime
 * is less than or equal to this value, then the soft
 * lifetime is not set. Valid values are from 1 to 28800.
 */
#define IKE_SOFT_LIFETIME_OFFSET        10

/* Interval to wait before retransmitting a message (in seconds).
 * This interval is applied with an exponential back-off. Valid
 * values are from 1 to 28800.
 */
#define IKE_RESEND_INTERVAL             2

/* Time to wait before resending a message. This value is set to the same
 * value as IKEv1.
 */
#define IKE2_MESSAGE_REPLY_TIMEOUT      IKE_RESEND_INTERVAL

/* Time in which an IKEv2 INIT SA or CHILD SA exchange must complete
 * (in seconds). The exchange would fail if it exceeds this duration.
 * Valid values are from 1 to 28800.
 */
#define IKE2_EXCHANGE_TIMEOUT           120

/* Maximum number of message retransmissions before giving up
 * on an exchange. Valid values are from 1 to 10. */
#define IKE_RESEND_COUNT                5

/* Time in which a phase 1 exchange MUST complete (in seconds).
 * The exchange would fail if it exceeds this duration. Valid
 * values are from 1 to 28800.
 */
#define IKE_PHASE1_TIMEOUT              60

/* Time in which a phase 2 exchange MUST complete (in seconds).
 * The exchange would fail if it exceeds this duration. Valid
 * values are from 1 to 28800.
 */
#define IKE_PHASE2_TIMEOUT              45

/**** IKE Buffer Lengths (all lengths are in bytes). ****/

/* Maximum number of SPIs that can be specified for
 * deletion, in a delete payload request sent by the
 * remote node. This can be from 1 to 20.
 */
#define IKE_MAX_DELETE_SPI              2

/* Maximum number of proposal payloads allowed in the SA
 * payload. This is always 1 for a phase 1 exchange and is
 * equal to the security size in the IPsec policy, for
 * phase 2. For example, if AH+ESP are used, then the
 * number of proposals would be 2. This value is applicable
 * to both incoming and outgoing payloads. Valid values are
 * from 1 to 50.
 */
#define IKE_MAX_PROPOSALS               4

/* Maximum number of transforms in a proposal payload.
 * This is the number of different possibilities for
 * a phase 1 exchange. For a phase 2 exchange, it is the
 * product of all possibilities of the phase 2 security
 * protocol. For example, an ESP proposal with encryption
 * algorithm either DES or 3DES and authentication
 * algorithm either MD5 or SHA1 would contain 2 x 2 = 4
 * transform payloads. This value is applicable to both
 * incoming and outgoing payloads. Valid values are from
 * 1 to 50.
 */
#define IKE_MAX_TRANSFORMS              5

/* This macro specifies the maximum length of the domain name
 * within the Identification payload. This is used only if
 * IKE_ENABLE_DOMAIN_NAME_ID is set to NU_TRUE. Valid values
 * must lie within a range of 4 to 256.
 *
 * WARNING: This MUST be a multiple of word size.
 */
#define IKE_MAX_DOMAIN_NAME_LEN         56

/* This macro specifies the maximum length of an IKE group
 * name. It also includes the null terminator at the end of
 * the string. Valid values must lie within a range of 4 to
 * 32.
 *
 * WARNING: This MUST be a multiple of word size.
 */
#define IKE_MAX_GROUP_NAME_LEN          20

/* Outgoing Nonce data length. Valid values are from
 * 4 to 252.
 */
#define IKE_OUTBOUND_NONCE_DATA_LEN     20

/* Maximum length of incoming IKE packets. Valid values are
 * from 200 to the maximum UDP packet size supported by the
 * network interface being used to transmit IKE messages.
 * This must not exceed 65535 in any case. IKEv2 requires handling
 * of packets at least 3000 bytes in length.
 */
#define IKE_MAX_INBOUND_PACKET_LEN      3000

/* Maximum length of outgoing packets. Valid values are from
 * 200 to the maximum UDP packet size supported by the network
 * interface being used to transmit IKE messages. This must
 * not exceed 65535 in any case. IKEv2 requires handling
 * of packets at least 3000 bytes in length.
 */
#define IKE_MAX_OUTBOUND_PACKET_LEN     3000

/* Number of outgoing packet buffers. Each buffer has a size
 * of IKE_MAX_OUTBOUND_PACKET_LEN and is allocated from the
 * IKE memory pool. Each phase 1 and phase 2 exchange uses
 * one buffer for as long as the exchange proceeds. Each
 * exchange may use up to two buffers if the
 * IKE_RETAIN_LAST_MESSAGE macro is set to NU_TRUE. One
 * buffer is also used for sending notification messages.
 */
#define IKE_MAX_BUFFERS                 10

/**** IKE Tasks Configuration. ****/

/* IKE main task configuration. */
/* IKE_TASK_STACK_SIZE can be reduced up to 3500 bytes if RSA is disabled. */
#define IKE_TASK_STACK_SIZE             (1024 * 7)
#define IKE_TASK_PRIORITY               3
#define IKE_TASK_TIME_SLICE             0
#define IKE_TASK_PREEMPT                NU_PREEMPT

/* IKE event task configuration. */
/* IKE_EVENT_TASK_STACK_SIZE can be reduced up to 2500 bytes if RSA is disabled. */
#define IKE_EVENT_TASK_STACK_SIZE       4096
#define IKE_EVENT_TASK_PRIORITY         3
#define IKE_EVENT_TASK_TIME_SLICE       0
#define IKE_EVENT_TASK_PREEMPT          NU_PREEMPT

/**** IKE Algorithm Configuration. ****/

/* Encryption Algorithms. These are specific to the
 * phase 1 encryption in an IKE exchange. For
 * configuration of encryption used in IPsec SAs, see
 * the Nucleus IPsec configuration file.
 */
#define IKE_INCLUDE_DES                 NU_TRUE  /* DES. */
#define IKE_INCLUDE_3DES                NU_TRUE  /* 3DES. */
#define IKE_INCLUDE_BLOWFISH            NU_TRUE  /* Blowfish. */
#define IKE_INCLUDE_CAST128             NU_TRUE  /* CAST-128. */
#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_AES)
#define IKE_INCLUDE_AES                 NU_TRUE /* AES. */
#else
#define IKE_INCLUDE_AES                 NU_FALSE /* AES. */
#endif /* CFG_NU_OS_CRYPTO_CIPHER_AES_ENABLE */

/* Hash Algorithms. These are specific to the phase 1
 * authentication in an IKE exchange. For configuration
 * of hash algorithms used in IPsec SAs, see the
 * Nucleus IPsec configuration file.
 */
#define IKE_INCLUDE_MD5                 NU_TRUE  /* MD5. */
#define IKE_INCLUDE_SHA1                NU_TRUE  /* SHA1. */

/* Signatures. These can be used as an authentication method
 * in phase 1, as an alternative to pre-shared keys. However,
 * note that certificates are not currently supported by
 * the IKE implementation of Nucleus IPsec, so the
 * public/private key pair must be statically specified in
 * the IKE policy.
 */
#define IKE_INCLUDE_RSA                 NU_FALSE /* RSA signatures. */

/* When using authentication by signatures, digital certificates are
 * used. Support for PEM formatted certificates is the default,
 * as OpenSSL uses PEM certificates by default.
 */
#define IKE_INCLUDE_PEM                 NU_TRUE

/* Support for checking a certificate against CRL. */
#define IKE_INCLUDE_CRL_SUPPORT         NU_FALSE

/* Maximum block size of the symmetric encryption algorithms
 * being used with IKE. Set this to the maximum block size
 * from all the encryption algorithms supported by IKE. This
 * is 8 bytes in most cases and must be 16 bytes if AES is
 * being used for phase 1 encryption.
 *
 * WARNING: This MUST be a multiple of word size.
 */
#if (IKE_INCLUDE_AES == NU_TRUE)
#define IKE_MAX_ENCRYPT_BLOCK_LEN       16
#else
#define IKE_MAX_ENCRYPT_BLOCK_LEN       8
#endif

/* This macro specifies the maximum length of a hash digest.
 * Set this to the maximum digest length from all digest
 * algorithms supported by IKE. If IKE_INCLUDE_SHA1 is set to
 * NU_TRUE, then this must be at least 20 bytes and if
 * IKE_INCLUDE_MD5 is set to NU_TRUE, then this must be at
 * least 16 bytes.
 *
 * WARNING: This MUST be a multiple of word size.
 */
#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#define IKE_MAX_HASH_DATA_LEN           20
#else
#define IKE_MAX_HASH_DATA_LEN           16
#endif

#define IKE2_INCLUDE_RSA                IKE_INCLUDE_RSA

/**** Total Algorithms included in IKE. ****/

/* This macro calculates the total number of encryption
 * algorithms included in the build.
 */
#define IKE_TOTAL_ENCRYPTION_ALGO       (IKE_INCLUDE_DES      + \
                                         IKE_INCLUDE_3DES     + \
                                         IKE_INCLUDE_BLOWFISH + \
                                         IKE_INCLUDE_CAST128  + \
                                         IKE_INCLUDE_AES)

/* This macro calculates the total number of hash algorithms
 * included in the build.
 */
#define IKE_TOTAL_HASH_ALGO             (IKE_INCLUDE_MD5         + \
                                         IKE_INCLUDE_SHA1        + \
                                         (IKE_INCLUDE_DES &&       \
                                         IKE_INCLUDE_VERSION_2)  + \
                                         (IKE_INCLUDE_AES &&       \
                                          IKE_INCLUDE_VERSION_2))

/* This macro calculates the total number of signature
 * algorithms included in the build.
 */
#define IKE_TOTAL_SIGN_ALGO             (IKE_INCLUDE_RSA       + \
                                         (IKE2_INCLUDE_RSA &&    \
                                          IKE_INCLUDE_VERSION_2))

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)

/* This macro calculates the total number of PRF algorithms
 * included in this build for IKEv2.
 */
#define IKE2_TOTAL_PRF_ALGOS            (IKE_INCLUDE_MD5  +  \
                                         IKE_INCLUDE_SHA1 +  \
                                         IKE_INCLUDE_AES)

#endif

/**** IKE Algorithm IDs and Attributes. ****/

/* DES algorithm constants. */
#if (IKE_INCLUDE_DES == NU_TRUE)
#define IKE_DES                         IKE_VAL_DES_CBC
#else
#define IKE_DES                         -1
#endif

/* 3DES algorithm constants. */
#if (IKE_INCLUDE_3DES == NU_TRUE)
#define IKE_3DES                        IKE_VAL_3DES_CBC
#else
#define IKE_3DES                        -1
#endif

/* Blowfish algorithm constants. */
#if (IKE_INCLUDE_BLOWFISH == NU_TRUE)
#define IKE_BLOWFISH                    IKE_VAL_BF_CBC
#else
#define IKE_BLOWFISH                    -1
#endif

/* CAST128 algorithm constants. */
#if (IKE_INCLUDE_CAST128 == NU_TRUE)
#define IKE_CAST_128                    IKE_VAL_CAST_CBC
#else
#define IKE_CAST_128                    -1
#endif

/* AES algorithm constants. */
#if (IKE_INCLUDE_AES == NU_TRUE)
#define IKE_AES                         IKE_VAL_AES_CBC
#else
#define IKE_AES                         -1
#endif

/* MD5 hash algorithm constants. */
#if (IKE_INCLUDE_MD5 == NU_TRUE)
#define IKE_MD5                         IKE_VAL_MD5
#else
#define IKE_MD5                         -1
#endif

/* SHA1 hash algorithm constants. */
#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#define IKE_SHA1                        IKE_VAL_SHA
#else
#define IKE_SHA1                        -1
#endif

/* RSA signature algorithm constants. */
#if (IKE_INCLUDE_RSA == NU_TRUE)
#define IKE_RSA                         IKE_VAL_RSA_SIG
#else
#define IKE_RSA                         -1
#endif

/* This should not be set to true since the support for NAT traversal is
 * not included in this version.
 */
#define IKE_ENABLE_NAT_TRAVERSAL        NU_FALSE

/**** IKE Configuration Verification. ****/

/* Define macro to specify maximum length of the
 * identification data within the Identification payload.
 */
#if ((IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE) && \
     (IKE_MAX_DOMAIN_NAME_LEN > MAX_ADDRESS_SIZE))
#define IKE_MAX_ID_DATA_LEN             IKE_MAX_DOMAIN_NAME_LEN
#else
#define IKE_MAX_ID_DATA_LEN             MAX_ADDRESS_SIZE
#endif

/* Make sure maximum wait events lie within valid range. */
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
#if ((IKE_MAX_WAIT_EVENTS <= 0) || (IKE_MAX_WAIT_EVENTS > 31))
#error "Maximum wait events must be between 1 and 31, inclusive."
#endif
#endif

/* Make sure at least one phase 1 exchange is enabled. */
#if ((IKE_INCLUDE_MAIN_MODE == NU_FALSE) && \
     (IKE_INCLUDE_AGGR_MODE == NU_FALSE))
#error "At least one of the phase 1 exchanges must be enabled."
#endif

/* Make sure at least one phase 1 authentication
 * method is enabled.
 */
#if ((IKE_INCLUDE_PSK_AUTH == NU_FALSE) && \
     (IKE_INCLUDE_SIG_AUTH == NU_FALSE))
#error "At least one of the phase 1 authentication methods must be enabled."
#endif

/* Soft lifetime offset must lie within valid range. */
#if ((IKE_SOFT_LIFETIME_OFFSET <= 0) || (IKE_SOFT_LIFETIME_OFFSET > 28800))
#error "Soft lifetime offset must be from 1 to 28800 seconds."
#endif

/* Message re-send interval must lie within valid range. */
#if ((IKE_RESEND_INTERVAL <= 0) || (IKE_RESEND_INTERVAL > 28800))
#error "Message resend interval must be from 1 to 28800 seconds."
#endif

/* Message re-send count must lie within valid range. */
#if ((IKE_RESEND_COUNT <= 0) || (IKE_RESEND_COUNT > 10))
#error "Message resend count must be from 1 to 10."
#endif

/* Phase 1 timeout must lie within valid range. */
#if ((IKE_PHASE1_TIMEOUT <= 0) || (IKE_PHASE1_TIMEOUT > 28800))
#error "Phase 1 timeout must be from 1 to 28800 seconds."
#endif

/* Phase 2 timeout must lie within valid range. */
#if ((IKE_PHASE2_TIMEOUT <= 0) || (IKE_PHASE2_TIMEOUT > 28800))
#error "Phase 2 timeout must be from 1 to 28800 seconds."
#endif

/* Maximum number of delete payload SPIs must lie within valid range. */
#if ((IKE_MAX_DELETE_SPI <= 0) || (IKE_MAX_DELETE_SPI > 20))
#error "Maximum number of delete payload SPIs must be greater than zero."
#endif

/* Maximum number of proposals must lie within valid range. */
#if ((IKE_MAX_PROPOSALS <= 0) || (IKE_MAX_PROPOSALS > 50))
#error "Maximum number of proposals must be from 1 to 50."
#endif

/* Maximum number of transforms must lie within valid range. */
#if ((IKE_MAX_TRANSFORMS <= 0) || (IKE_MAX_TRANSFORMS > 50))
#error "Maximum number of transforms must be from 1 to 50."
#endif

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
/* Maximum domain name must lie within valid range. */
#if ((IKE_MAX_DOMAIN_NAME_LEN < 4) || (IKE_MAX_DOMAIN_NAME_LEN > 256))
#error "Maximum domain name length must be from 4 to 256."
#endif
/* Maximum domain name length must be a multiple of word size. */
#if (IKE_MAX_DOMAIN_NAME_LEN % 4 != 0)
#error "Maximum domain name length must be a multiple of word size."
#endif
#endif

/* Maximum group name length must lie within valid range. */
#if ((IKE_MAX_GROUP_NAME_LEN < 4) || (IKE_MAX_GROUP_NAME_LEN > 32))
#error "Maximum group name length must be from 4 to 32."
#endif

/* Maximum group name length must be a multiple of word size. */
#if (IKE_MAX_GROUP_NAME_LEN % 4 != 0)
#error "Maximum group name length must be a multiple of word size."
#endif

/* Maximum Nonce Data length must lie within valid range. */
#if ((IKE_OUTBOUND_NONCE_DATA_LEN < 4) || \
     (IKE_OUTBOUND_NONCE_DATA_LEN > 252))
#error "Maximum Nonce Data length must be from 4 to 252."
#endif

/* Maximum Nonce Data length must be a multiple of word size. */
#if (IKE_OUTBOUND_NONCE_DATA_LEN % 4 != 0)
#error "Maximum Nonce Data length must be a multiple of word size."
#endif

/* Maximum inbound packet length must be at least 200 bytes. */
#if (IKE_MAX_INBOUND_PACKET_LEN < 200)
#error "Maximum inbound packet length must be at least 200 bytes."
#endif

#if ((IKE_INCLUDE_VERSION_2 == NU_TRUE) && \
     (IKE2_MAX_PACKET_LEN < 1280))
#error "Packet size cannot be set to value smaller than 1280"
#endif

#if ((IKE_INCLUDE_VERSION_2 == NU_TRUE) && \
    (IKE_MAX_INBOUND_PACKET_LEN < IKE2_MAX_PACKET_LEN))
#error "Maximum inbound packet length must be at least 3000 bytes for IKEv2."
#endif

/* Maximum inbound packet length must not exceed UDP packet limit. */
#if (IKE_MAX_INBOUND_PACKET_LEN > 65535)
#error "Maximum inbound packet length must be less than 65535 bytes."
#endif

/* Maximum outbound packet length must be at least 200 bytes. */
#if (IKE_MAX_OUTBOUND_PACKET_LEN < 200)
#error "Maximum outbound packet length must be at least 200 bytes."
#endif

#if ((IKE_INCLUDE_VERSION_2 == NU_TRUE) && \
    (IKE_MAX_OUTBOUND_PACKET_LEN < 3000))
#error "Maximum outbound packet length must be at least 3000 bytes for IKEv2."
#endif

/* Maximum outbound packet length must not exceed UDP packet limit. */
#if (IKE_MAX_OUTBOUND_PACKET_LEN > 65535)
#error "Maximum outbound packet length must be less than 65535 bytes."
#endif

/* Make sure packet length is enough for the optional MODP groups. */
#if ((IKE_INCLUDE_MODP_8192 == NU_TRUE) && \
     (IKE_MAX_OUTBOUND_PACKET_LEN < 1350))
#error "Max. outbound packet length must be >=1350 bytes for 8192-bit MODP."
#elif ((IKE_INCLUDE_MODP_6144 == NU_TRUE) && \
       (IKE_MAX_OUTBOUND_PACKET_LEN < 1100))
#error "Max. outbound packet length must be >=1100 bytes for 6144-bit MODP."
#elif ((IKE_INCLUDE_MODP_4096 == NU_TRUE) && \
       (IKE_MAX_OUTBOUND_PACKET_LEN < 750))
#error "Max. outbound packet length must be >=750 bytes for 4096-bit MODP."
#elif ((IKE_INCLUDE_MODP_3072 == NU_TRUE) && \
       (IKE_MAX_OUTBOUND_PACKET_LEN < 620))
#error "Max. outbound packet length must be >=620 bytes for 3072-bit MODP."
#endif

/* Verify maximum number of send buffers. */
#if (IKE_RETAIN_LAST_MESSAGE == NU_TRUE)
#if (IKE_MAX_BUFFERS < 2)
#error "If message retain is enabled, maximum buffers must be at least 2."
#endif
#else
#if (IKE_MAX_BUFFERS <= 0)
#error "Maximum buffers must be greater than zero."
#endif
#endif

/* Make sure encryption block length is at least 16 if using AES. */
#if ((IKE_INCLUDE_AES == NU_TRUE) && \
     (IKE_MAX_ENCRYPT_BLOCK_LEN < 16))
#error "Maximum encryption block length must be at least 16 if using AES."
#endif

/* Make sure hash digest length is valid. */
#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#if (IKE_MAX_HASH_DATA_LEN < 20)
#error "Maximum hash digest length must be at least 20 when using SHA1."
#endif
#elif (IKE_INCLUDE_MD5 == NU_TRUE)
#if (IKE_MAX_HASH_DATA_LEN < 16)
#error "Maximum hash digest length must be at least 16 when using MD5."
#endif
#endif

/* Make sure at least one encryption algorithm is enabled. */
#if (IKE_TOTAL_ENCRYPTION_ALGO <= 0)
#error "At least one encryption algorithm must be enabled."
#endif

/* Make sure at least one hash algorithm is enabled. */
#if (IKE_TOTAL_HASH_ALGO <= 0)
#error "At least one hash algorithm must be enabled."
#endif

/* Make sure at least one signature algorithm is enabled, if
 * signature based authentication is enabled.
 */
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
#if (IKE_TOTAL_SIGN_ALGO <= 0)
#error "At least one signature algorithm must be enabled."
#endif
#endif

#if ((IKE_INCLUDE_VERSION_2 == NU_TRUE) && (IKE_INCLUDE_RSA == NU_TRUE))
#ifdef OPENSSL_NO_RSA
#error "RSA authentication with IKEv2 requires inclusion of RSA in OpenSSL"
#endif
#endif

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
#error "NAT Traversal not supported in this version."
#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_CFG_H */
