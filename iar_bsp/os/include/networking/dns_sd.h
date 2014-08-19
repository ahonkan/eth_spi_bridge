/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/**************************************************************************
*
* FILENAME
*
*       dns_sd.h
*
* DESCRIPTION
*
*       This include file will handle DNS-SD processing defines.
*
* DATA STRUCTURES
*
*       NU_DNS_SD_INSTANCE
*
* DEPENDENCIES
*
*      No other file dependencies
*
***************************************************************************/

#ifndef DNS_SD_H
#define DNS_SD_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

typedef struct _nu_dns_sd_instance
{
    struct _nu_dns_sd_instance   *dns_sd_next;
    CHAR                        *dns_sd_name;

} NU_DNS_SD_INSTANCE;

typedef struct _nu_dns_sd_service
{
    INT32       dns_key_len;
    UINT16      dns_prio;
    UINT16      dns_weight;
    UINT16      dns_port;
    UINT8       dns_padN[2];
    CHAR        *dns_hostname;      /* Do not reorder the members of this structure. */
    CHAR        *dns_keys;
} NU_DNS_SD_SERVICE;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* DNS_H */
