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
*       snmp.h                                                   
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in the file SNMP.C.
*
*   DATA STRUCTURES
*
*       snmp_stat_s
*       snmp_object_s
*       snmp_request_s
*       snmp_trap_s
*       snmp_pdu_s
*       SNMP_ENGINE_STRUCT
*       SNMP_BULK_STRUCT
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp_cfg.h
*
************************************************************************/

#ifndef SNMP_H
#define SNMP_H

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Maximum IP address length. */
#define SNMP_MAX_IP_ADDRS           MAX_ADDRESS_SIZE

/* SNMP Message Versions. */
#define SNMP_VERSION_V1            0
#define SNMP_VERSION_V2            1
#define SNMP_VERSION_V3            3

/* SNMP Security Models. */
#define SNMP_ANY                   0
#define SNMP_CBSM_V1               1
#define SNMP_CBSM_V2               2
#define SNMP_USM                   3

/* Cipher PAD. */
#define SNMP_CIPHER_PAD_SIZE       8

/* SNMP PDU Versions. */
#define SNMP_PDU_V1                1
#define SNMP_PDU_V2                2

/* SNMP Transport Protocol and size of field containing port number in bytes. */
#define SNMP_UDP                   1
#define SNMP_PORT_NUM_LEN          2

/* Port Numbers */
#define SNMP_PORT                161
#define SNMP_TRAP_PORT           162

/* Max Settings */
#define MAX_PORTS                  2

/* SNMP Standard Sizes. */
#define SNMP_SIZE_COMM           256     /* 256 */
#define SNMP_SIZE_OBJECTID       128     /* 128 */
#define SNMP_SIZE_BUFCHR         256     /* 256 */
#define SNMP_SIZE_BUFINT         128     /* 128 */
#define SNMP_SIZE_SMALLOBJECTID   32

/* SNMP Standard Message Types */
#define SNMP_PDU_GET               0
#define SNMP_PDU_NEXT              1
#define SNMP_PDU_RESPONSE          2
#define SNMP_PDU_SET               3
#define SNMP_PDU_TRAP_V1           4
#define SNMP_PDU_BULK              5
#define SNMP_PDU_INFORM            6
#define SNMP_PDU_TRAP_V2           7
#define SNMP_PDU_REPORT            8
#define SNMP_PDU_TYPES             9

#define SNMP_PDU_COMMIT            9    /* for internal use only */
#define SNMP_PDU_UNDO              10   /* for internal use only */
#define SNMP_PDU_CREATE            12   /* for internal use only */
#define SNMP_PDU_GET_SET           13   /* for internal use only */

/* SNMP Standard Errors */
#define SNMP_NOERROR                0
#define SNMP_TOOBIG                 1
#define SNMP_NOSUCHNAME             2
#define SNMP_BADVALUE               3
#define SNMP_READONLY               4
#define SNMP_GENERROR               5
#define SNMP_NOACCESS               6
#define SNMP_WRONGTYPE              7
#define SNMP_WRONGLENGTH            8
#define SNMP_WRONGENCODING          9
#define SNMP_WRONGVALUE             10
#define SNMP_NOCREATION             11
#define SNMP_INCONSISTANTVALUE      12
#define SNMP_RESOURCEUNAVAILABLE    13
#define SNMP_COMMITFAILED           14
#define SNMP_UNDOFAILED             15
#define SNMP_AUTHORIZATIONERROR     16
#define SNMP_NOTWRITABLE            17
#define SNMP_INCONSISTANTNAME       18


#define SNMP_NOSUCHOBJECT           128     /*80h*/
#define SNMP_NOSUCHINSTANCE         129     /*81h*/
#define SNMP_ENDOFMIBVIEW           130     /*82h*/


/* NUCLEUS SNMP Error (Used Internally).  */
#define SNMP_ERROR                  100
#define SNMP_SILENT_ERROR           101
#define SNMP_NO_MEMORY              102
#define SNMP_BAD_PARAMETER          103
#define SNMP_WARNING                104
#define SNMP_BAD_POINTER            105
/* RMON Status */
#define SNMP_VALID                   1
#define SNMP_CREATEREQUEST           2
#define SNMP_UNDERCREATION           3
#define SNMP_INVALID                 4


/* SNMP DATA Types */
#define SNMP_NULL                    0
#define SNMP_INTEGER                 1    /* LngInt */
#define SNMP_OCTETSTR                2    /* BufChr */
#define SNMP_OBJECTID                3    /* BufInt */
#define SNMP_IPADDR                  4    /* LngUns */
#define SNMP_COUNTER                 5    /* LngUns */
#define SNMP_GAUGE                   6    /* LngUns */
#define SNMP_TIMETICKS               7    /* LngUns */
#define SNMP_OPAQUE                  8    /* BufChr */
#define SNMP_DISPLAYSTR              2    /* BufChr */
#define SNMP_COUNTER64               9    /* BigInt (for 64-bit number)*/

#define SNMP_IPA                     0    /* IpAddress, APPLICATION (0) */
#define SNMP_CNT                     1    /* Counter,   APPLICATION (1) */
#define SNMP_GGE                     2    /* Gauge,     APPLICATION (2) */
#define SNMP_TIT                     3    /* TimeTicks  APPLICATION (3) */
#define SNMP_OPQ                     4    /* Opaque,    APPLICATION (4) */
#define SNMP_C64                     6    /* Counter64  APPLICATION (6) */

/* SNMP storage types. */
#define SNMP_STORAGE_OTHER           1
#define SNMP_STORAGE_VOLATILE        2
#define SNMP_STORAGE_NONVOLATILE     3
#define SNMP_STORAGE_PERMANENT       4
#define SNMP_STORAGE_READONLY        5

/* SNMP row status. */
#define SNMP_ROW_ACTIVE              1
#define SNMP_ROW_NOTINSERVICE        2
#define SNMP_ROW_NOTREADY            3
#define SNMP_ROW_CREATEANDGO         4
#define SNMP_ROW_CREATEANDWAIT       5
#define SNMP_ROW_DESTROY             6

/* SNMP Security Level. */
#define SNMP_SECURITY_NOAUTHNOPRIV   1
#define SNMP_SECURITY_AUTHNOPRIV     2
#define SNMP_SECURITY_AUTHPRIV       3

/* The following defines give the status of a particular Subsystem/Model
   at a particular time. */
#define SNMP_MODULE_NOTSTARTED       0
#define SNMP_MODULE_INITIALIZED      1
#define SNMP_MODULE_NOTINITIALIZED   2

/* SNMP Access Rights. */
#define SNMP_READ_ONLY          1
#define SNMP_WRITE_ONLY         2
#define SNMP_READ_WRITE         3

/* Events being used through out SNMP. */
#define SNMP_REQUEST_ARRIVED        0x00000001
#define SNMP_NOTIFICATION_ARRIVED   0x00000002

/* Defines used by SNMP Queues. */
#define SNMP_TAKEN              1           /* Indicates that the element
                                               is being used. */
#define SNMP_READY              2           /* Indicates that the element
                                               is ready for being
                                               processed. */

/* MIB for SNMP Engine. */
typedef struct snmp_engine_struct
{
    UINT32          snmp_engine_id_len;
    UINT32          snmp_engine_boots;
    UINT32          snmp_max_message_size;
    NU_TIMER        snmp_engine_timer;
    UINT8           snmp_timer_resets;
    UINT8           snmp_engine_id[SNMP_SIZE_SMALLOBJECTID];

    UINT8           snmp_pad[3];
}SNMP_ENGINE_STRUCT;


/* Typedefs */
typedef struct  snmp_stat_s                 snmp_stat_t;
typedef struct  snmp_request_s              snmp_request_t;
typedef struct  snmp_trap_s                 snmp_trap_t;
typedef union   snmp_pdu_s                  snmp_pdu_t;
typedef union   snmp_syntax_s               snmp_syntax_t;
typedef struct  snmp_object_s               snmp_object_t;

typedef UINT32 COUNTER64[2];

/* Data structure used for value of Object Identifier whose data
structure is defined after it's definition*/
union snmp_syntax_s
{
    INT32        LngInt;                       /* to store INTEGER */
    UINT32       LngUns;                       /* to store IPAddress or
                                                * COUNTER or GAUGE or
                                                * TIMETICKS
                                                */
    COUNTER64    BigInt;                       /* to store very big number
                                                * (64 bit)
                                                */
    UINT8        BufChr [SNMP_SIZE_BUFCHR];    /* to store Octet String or
                                                * Opaque data
                                                */
    UINT32       BufInt [SNMP_SIZE_BUFINT];    /* to store Object
                                                * Identifier
                                                */
    VOID         *Ptr;
};


/*data structure used for Object Identifier*/
struct snmp_object_s
{
    /* For all protocols Operations defined in RFC 1905
     * (get,getnext,getbulk,inform etc.
     */
    UINT32          Request;

    /*The buffer to store object identifier*/
    UINT32          Id[SNMP_SIZE_OBJECTID];

    /*Stores the length of Object identifier*/
    UINT32          IdLen;

    /*stored in Obj->Type(e.g. INTEGER,OCTETSTR,OPAQUE,IPADD,COUNTER etc*/
    UINT32          Type;

    /*stores the length of the value for a particular Object Identifier*/
    UINT32          SyntaxLen;

    /*stores the value for the Object Identifier*/
    snmp_syntax_t   Syntax;
};


struct snmp_stat_s
{
    UINT32    OutPkts;
    UINT32    OutTooBigs;
    UINT32    OutNoSuchNames;
    UINT32    OutBadValues;
    UINT32    OutReadOnlys;
    UINT32    OutGenErrs;
    UINT32    OutGetRequests;
    UINT32    OutGetNexts;
    UINT32    OutSetRequests;
    UINT32    OutGetResponses;
    UINT32    OutTraps;

    UINT32    OutGetBulks;
    UINT32    OutInforms;
    UINT32    OutReports;

    UINT32    InPkts;
    UINT32    InTooBigs;
    UINT32    InNoSuchNames;
    UINT32    InBadValues;
    UINT32    InReadOnlys;
    UINT32    InGenErrs;
    UINT32    InGetRequests;
    UINT32    InGetNexts;
    UINT32    InSetRequests;
    UINT32    InGetResponses;
    UINT32    InTraps;
    UINT32    InBadVersions;
    UINT32    InASNParseErrs;
    UINT32    InBadTypes;

    UINT32    InGetBulks;
    UINT32    InInforms;
    UINT32    InReports;
};


struct snmp_request_s
{
    UINT32    Type;
    UINT32    Id;
    UINT32    ErrorStatus;
    UINT32    ErrorIndex;
};


struct snmp_trap_s
{
    UINT32    Type;
    UINT32    Id [SNMP_SIZE_BUFINT];
    UINT32    IdLen;
    UINT8     IpAddress[SNMP_MAX_IP_ADDRS];
    UINT32    IpAddressLen;
    UINT32    General;
    UINT32    Specific;
    UINT32    Time;
};


typedef struct snmp_bulk_struct
{
    UINT32    Type;
    UINT32    snmp_id;
    INT32     snmp_non_repeaters;
    INT32     snmp_max_repetitions;
}  SNMP_BULK_STRUCT;

union snmp_pdu_s
{
    UINT32              Type;
    struct snmp_trap_s  Trap;
    snmp_request_t      Request;
#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )
    SNMP_BULK_STRUCT    Bulk;
#endif
};


/* Initialization and Configuration functions common to all models. */
typedef STATUS (*SNMP_INIT)(VOID);
typedef STATUS (*SNMP_CONFIG)(VOID);

STATUS SNMP_Setup_Socket (INT *socket);
STATUS SNMP_Engine_Init(VOID);
STATUS SNMP_Engine_Config(VOID);
VOID SNMP_Configuration(VOID);
VOID SNMP_Timer_Expired(UNSIGNED id);
STATUS SNMP_Engine_Time(UINT32 *snmp_engine_time);
STATUS SNMP_Get_Engine_ID(UINT8 *engine_id, UINT32 *engine_id_len);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif  /* SNMP_H */


