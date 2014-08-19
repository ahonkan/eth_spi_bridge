/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE                                             VERSION
*
*       fc_defs.h                                    1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - FTP Client & Server Common
*       Definitions
*
*   DESCRIPTION
*
*       This file contains common definitions for the Nucleus FTP Server
*       and Client API. It includes the API constant defines as well as
*       definitions of all API specific types.
*
*   DATA STRUCTURES
*
*       IP_ADDR_STRUCT      Stores an ip address and port combination
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       net_cfg.h
*
*************************************************************************/

#ifndef FC_DEFS
#define FC_DEFS

#include "networking/target.h"
#include "networking/net_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Return codes */
#define FTP_PASSIVE_MODE_ON             1
#define FTP_EPASSIVE_MODE_ON            2
#define FTP_PASSIVE_MODE_OFF            0
#define FTP_CMD_RECEIVED                1

#define FTP_EOF     0

/*
 * The range for Nucleus FTP error codes is -1501 to -1750
 */
#define FTP_INVALID_CLIENT              -1501
#define FTP_INVALID_TASK                -1502
#define FTP_STACK_ERROR                 -1503
#define FTP_INVALID_USER                -1504
#define FTP_INVALID_PASSWORD            -1505
#define FTP_INVALID_TYPE_CODE           -1506
#define FTP_BUFFER_OVERRUN              -1507
#define FTP_BAD_HOST                    -1508
#define FTP_BAD_RESPONSE                -1509
#define FTP_TIMEOUT                     -1510
#define FTP_NEED_PASSWORD               -1511
#define FTP_NEED_ACCOUNT                -1512
#define FTP_INVALID_ACCOUNT             -1513
#define FTP_INVALID_STRU_CODE           -1514
#define FTP_INVALID_MODE_CODE           -1515
#define FTP_INVALID_BUFFER              -1516
#define FTP_TRANSFER_ABORT              -1517
#define FTP_FILE_ERROR                  -1518
#define FTP_BAD_MSG_FORMAT              -1519
#define FTP_REPLY_BUFFER_OVERRUN        -1520
#define FTP_INVALID_IP_ADDR             -1521
#define FTP_BAD_CMD_FORMAT              -1522
#define FTP_BAD_FILE_DESCRIPTOR         -1523
#define FTP_WRITE_FAILED                -1524
#define FTP_FILE_NOT_FOUND              -1525
#define FTP_NO_FILE_DESCRIPTOR_AVAIL    -1526
#define FTP_FILE_ALREADY_EXISTS         -1527
#define FTP_SPECIAL_ACCESS_ATTEMPTED    -1528
#define FTP_INVALID_FILE_POINTER        -1529
#define FTP_UNKNOWN_FILE_ERROR          -1530
#define FTP_SYNTAX_ERROR                -1531
#define FTP_REGISTRATION_FAILURE        -1532
#define FTP_OPEN_DRIVE_FAILURE          -1533
#define FTP_CURRENT_DIR_FAILURE         -1534
#define FTP_MEMORY                      -1535
#define FTP_SERVICE_UNAVAILABLE         -1536
#define FTP_CMD_NOT_IMPLEMENTED         -1537
#define FTP_BAD_CMD_SEQUENCE            -1538
#define FTP_FILE_UNAVAILABLE            -1539
#define FTP_INVALID_FILE_NAME           -1540
#define FTP_CMD_UNRECOGNIZED            -1541
#define FTP_UNKNOWN_NETWORK_PROTOCOL    -1542
#define FTP_INVALID_PARM                -1543
#define FTPS_CTASK_STRU_MALLOC_FAILURE  -1544
#define FTPS_CTASK_MALLOC_FAILURE       -1545
#define FTPS_CTASK_CREATE_FAILURE       -1546
#define FTPS_CTASK_KERNEL_BIND_FAILURE  -1547
#define FTPS_SERVER_TERMINATED          -1548

/* Internal constants */
#define FTP_WELLKNOWN                   21
#define FTP_VALID_PATTERN               0x10229700L
#define FILENAME_SIZE                   12
#define FTP_SERVER_MAX_CONNECTIONS      TCP_MAX_PORTS

/* General Purpose Flags */
#define FTP_SERVER_STOP_ALL             0x1

/* FTP_Group events. */
#define RETR_Event                      1
#define STOR_Event                      2
#define APPE_Event                      4
#define LIST_Event                      8
#define NLST_Event                      16
#define FTP_COMMAND_READY               31

/* Parameter values */
#define FTP_TYPE_IMAGE                  0
#define FTP_TYPE_ASCII                  1
#define FTP_TYPE_LOCAL8                 0

/* Zero Copy buffer allocation. Set FTP_MAX_ZC_BUFFERS to a reasonable
 * value. Larger values of this variable limits the number of successful
 * simultaneous file transfers at any given time.
 * For example: 10 Buffers = 5120 bytes = 5K mem allocated per ZC call.
 */
#define FTP_MAX_ZC_BUFFERS              10
#define FTP_MAX_ZC_BUFFER_SIZE (FTP_MAX_ZC_BUFFERS * NET_PARENT_BUFFER_SIZE)

/* typedefs */
typedef struct IP_ADDR_STRUCT
{
#ifdef NET_5_1
   UINT8    ip_num[MAX_ADDRESS_SIZE];
#else
   UINT8    ip_num[IP_ADDR_LEN];
#endif
   INT16    port_num;
   UINT8    padN[2];
} IP_ADDR;


/* FTP_Unused_Parameter is defined in xprot/ftp/src/fst.c */
extern UINT32 FTP_Unused_Parameter;

/* this macro is used to remove compilation warnings. */
#define FTP_UNUSED_PARAMETER(x)  FTP_Unused_Parameter = ((UINT32)(x))


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

