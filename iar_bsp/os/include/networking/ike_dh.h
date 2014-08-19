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
*       ike_dh.h
*
* COMPONENT
*
*       IKE - Diffie-Hellman Groups
*
* DESCRIPTION
*
*       This file contains data used in the Diffie-Hellman
*       Key Exchange.
*
* DATA STRUCTURES
*
*       IKE_OAKLEY_GROUP_INFO
*
* DEPENDENCIES
*
*       dh.h
*
*************************************************************************/
#ifndef IKE_DH_H
#define IKE_DH_H

#include "openssl/dh.h"

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Oakley group constants and macros. ****/

/* Macros which define the supported Diffie-Hellman groups.
 * The IKE_GROUP_NONE value can only be specified in Phase 2.
 */
#define IKE_GROUP_NONE                  0
#define IKE_GROUP_MODP_768              IKE_VAL_MODP_768
#if (IKE_INCLUDE_MODP_1024 == NU_TRUE)
#define IKE_GROUP_MODP_1024             IKE_VAL_MODP_1024
#endif
#if (IKE_INCLUDE_MODP_1536 == NU_TRUE)
#define IKE_GROUP_MODP_1536             IKE_VAL_MODP_1536
#endif
#if (IKE_INCLUDE_MODP_2048 == NU_TRUE)
#define IKE_GROUP_MODP_2048             IKE_VAL_MODP_2048
#endif
#if (IKE_INCLUDE_MODP_3072 == NU_TRUE)
#define IKE_GROUP_MODP_3072             IKE_VAL_MODP_3072
#endif
#if (IKE_INCLUDE_MODP_4096 == NU_TRUE)
#define IKE_GROUP_MODP_4096             IKE_VAL_MODP_4096
#endif
#if (IKE_INCLUDE_MODP_6144 == NU_TRUE)
#define IKE_GROUP_MODP_6144             IKE_VAL_MODP_6144
#endif
#if (IKE_INCLUDE_MODP_8192 == NU_TRUE)
#define IKE_GROUP_MODP_8192             IKE_VAL_MODP_8192
#endif
#if (IKE_INCLUDE_MODP_1024_160_POS == NU_TRUE)
#define IKE_GROUP_MODP_1024_160_POS     IKE_VAL_MODP_1024_160_POS
#endif
#if (IKE_INCLUDE_MODP_2048_224_POS == NU_TRUE)
#define IKE_GROUP_MODP_2048_224_POS     IKE_VAL_MODP_2048_224_POS
#endif
#if (IKE_INCLUDE_MODP_2048_256_POS == NU_TRUE)
#define IKE_GROUP_MODP_2048_256_POS     IKE_VAL_MODP_2048_256_POS
#endif

/* Total number of available Oakley Groups. */
#define IKE_TOTAL_OAKLEY_GROUPS         (sizeof(IKE_Oakley_Groups) /    \
                                         sizeof(IKE_OAKLEY_GROUP_INFO))

/* Default length, in bits, of the private key which is to
 * be generated for the Diffie-Hellman Key Exchange.
 */
#define IKE_DH_PRIVATE_KEY_SIZE         0

/* Oakley Group generator. Currently, is 2 for all groups. */
#define IKE_OAKLEY_GROUP_GEN(type)      DH_GENERATOR_2

/* Identifiers for Diffie Hellman groups. */
#define IKE_DH_GENERATOR_2              DH_GENERATOR_2
#define IKE_DH_GENERATOR_POS            0xffff

/**** Data structures. ****/

/* This structure is used to store data related to an Oakley Group. */
typedef struct ike_oakley_group_info
{
    UINT8           *ike_prime;             /* Prime no. of the group. */
    UINT16          ike_prime_len;          /* Length of prime in bytes. */
    UINT16          ike_group_desc;         /* Group description ID. */
    UINT32          ike_generator;          /* Generator if 32-bit only. */
    UINT8           *ike_generator_ext;     /* Extended generator. */
    UINT16          ike_generator_ext_len;  /* Extended generator size. */
    UINT16          ike_subgroup_len;       /* Subgroup length in bytes. */
} IKE_OAKLEY_GROUP_INFO;

/**** Function prototypes. ****/

UINT8 *IKE_Oakley_Group_Prime(UINT16 type);
UINT16 IKE_Oakley_Group_Length(UINT16 type);
IKE_OAKLEY_GROUP_INFO *IKE_Get_Oakley_Group(UINT16 type);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_DH_H */
