/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       tgr_mib.h                                                
*
*   DESCRIPTION
*
*       This file contains the functions for the Target Address
*       Table.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       snmp_cfg.h
*
************************************************************************/

#ifndef TGR_MIB_H
#define TGR_MIB_H

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/snmp_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define MAX_TARG_PARAM_SZE      33
#define MAX_TARG_NAME_SZE       33
#define MAX_TIMEOUT             2147483648UL

#define TARGET_FOUND            1
#define TARGET_NOT_FOUND        0

#define PARAMS_FOUND            TARGET_FOUND
#define PARAMS_NOT_FOUND        TARGET_NOT_FOUND

#define TDOMAIN_INITIALIZED     0x01    /* 0000_0001 */
#define TADDRESS_INITIALIZED    0x02    /* 0000_0010 */
#define PARAMS_INITIALZED       0x04    /* 0000_0100 */

#define MP_INITIALIZED          0x01    /* 0000_0001 */
#define SM_INITIALIZED          0x02
#define SN_INITIALIZED          0x04
#define SL_INITIALIZED          0x08

#define TARGET_INITIALIZED      0x07    /* 0000_0111 */
#define PARAMS_INITIALIZED      0x0F    /* 0000_1111 */

#define SNMP_TARGET_ADDR_SUB_LEN        11
#define SNMP_TARGET_ADDR_ATTR_OFFSET    10
#define SNMP_TARGET_PARAM_SUB_LEN       11
#define SNMP_TARGET_PARAM_ATTR_OFFSET   10

typedef struct snmp_target_address_table
{
    /* End of snmpTargetAddrExtTable. */
    struct snmp_target_address_table *next;
    struct snmp_target_address_table *previous;

    UINT32    snmp_target_addr_tDomain;
    UINT32    snmp_target_addr_time_out;    /* number of timer ticks.
                                             * 1 tick = 10 milli seconds
                                             */
    UINT32    tag_list_len;
    UINT32    params_len;
    INT32     snmp_target_addr_retry_count;

    /* Used in Target extension table. */
    UINT32    snmp_target_addr_mms;

    INT16     snmp_target_addr_tfamily;

    /* null terminated */
    CHAR      snmp_target_addr_name[MAX_TARG_NAME_SZE];
    UINT8     snmp_target_addr_tAddress[SNMP_MAX_IP_ADDRS];
    UINT8     snmp_target_addr_tag_list[SNMP_SIZE_BUFCHR];
    UINT8     snmp_target_addr_params[MAX_TARG_NAME_SZE];

    /* The following two elements are used in the target extension table. */
    UINT8     snmp_target_addr_tmask[SNMP_MAX_IP_ADDRS];
    UINT8     snmp_ext_enabled;       /* 0 if the extended table is not
                                       * being used 1 if it is being used.
                                       */
    UINT8     snmp_pad1;

    /* The below field is added to cater SNMP over UDP domain format of
     * IP address followed by port number.
     */
    UINT16    snmp_target_addr_portnumber;

#if (INCLUDE_MIB_TARGET == NU_TRUE)
    UINT8     snmp_target_addr_storage_type;
    UINT8     snmp_target_addr_row_status;
    UINT8     snmp_target_addr_row_flag;
    UINT8     snmp_pad2;
#endif

}SNMP_TARGET_ADDRESS_TABLE;



typedef struct snmp_target_addr_root
{
    struct snmp_target_address_table *next;
    struct snmp_target_address_table *previous;

}SNMP_TARGET_ADDR_ROOT;

typedef struct snmp_target_params_table
{
    struct snmp_target_params_table *next;
    struct snmp_target_params_table *previous;

    UINT32    snmp_target_params_mp_model;
    UINT32    snmp_target_params_security_model;
    INT32     snmp_target_params_security_level;

    /* null terminated */
    CHAR      snmp_target_params_security_name[SNMP_SIZE_BUFCHR];
    /* null terminated */
    CHAR      snmp_target_params_name[MAX_TARG_NAME_SZE];

#if (INCLUDE_MIB_TARGET == NU_TRUE)
    UINT8     snmp_target_params_storage_type;
    UINT8     snmp_target_params_row_status;
    UINT8     snmp_target_params_row_flag;
#else
    UINT8     snmp_pad[3];
#endif

}SNMP_TARGET_PARAMS_TABLE;

typedef struct snmp_target_params_root
{
    struct snmp_target_params_table *next;
    struct snmp_target_params_table *previous;

}SNMP_TARGET_PARAMS_ROOT;


typedef struct snmp_target_mib
{
#if (INCLUDE_MIB_TARGET == NU_TRUE)
    UINT32                       snmp_target_spin_lock;
    UINT32                       snmp_unavailable_contexts;
    UINT32                       snmp_unknown_contexts;
#endif
    SNMP_TARGET_ADDR_ROOT        target_addr_table;
    SNMP_TARGET_PARAMS_ROOT      target_params_table;

} SNMP_TARGET_MIB;

UINT16 snmpTargetSpinLock(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 snmpTargetAddrEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 snmpTargetParamsEntry(snmp_object_t *obj, UINT16 idlen,
                             VOID *param);
UINT16 snmpUnavailableContexts(snmp_object_t *obj, UINT16 idlen,
                               VOID *param);
UINT16 snmpUnknownContexts(snmp_object_t *obj, UINT16 idlen, VOID *param);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
STATUS Save_Target_Addr(SNMP_TARGET_ADDRESS_TABLE *target_addr);
STATUS Save_Target_Params(SNMP_TARGET_PARAMS_TABLE *target_param);
#endif
#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


