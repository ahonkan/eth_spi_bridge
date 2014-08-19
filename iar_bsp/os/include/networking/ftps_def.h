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
*       ftps_def.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server-Level API
*       Function Definitions
*
*   DESCRIPTION
*
*       This file contains the definitions for the Nucleus FTP Server API.
*       It includes the API constant defines as well as definitions of all
*       API specific types.
*
*   DATA STRUCTURES
*
*       FSP_CB
*       CMD_FLAGS
*       FTP_SERVER
*       FTPSACCT
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       ftp_cfg.h
*
*************************************************************************/

#ifndef FTPS_DEF
#define FTPS_DEF

#include "networking/target.h"
#include "networking/ftp_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/*************************************************************/

/* FSP_Command_Read() state constants */
enum { FCR_START,
   FCR_LETTER2,
   FCR_LETTER3,
   FCR_LETTER4,
   FCR_LETTER5,
   FCR_SPACE,
   FCR_AFTER_SPACE,
   FCR_FIND_LF,
   FCR_BAD_FIND_LF,
   FCR_BAD_CMD,
   FCR_END };

/* File related constants */
#define CDRIVE                   "C:"

#define MSG150    "150 File status okay; about to open data connection.\r\n"
#define SIZE150      54
#define MSG200    "200 Command okay.\r\n"
#define SIZE200      19
#define MSG202    "202 Command not implemented, superfluous at this site.\r\n"
#define SIZE202      55
#define MSG211    "211 Type: ASCII, Structure: FILE, Mode: Stream.\r\n"
#define SIZE211      49
#define MSG211a      "211 Type: IMAGE, Structure: FILE, Mode: Stream.\r\n"
#define SIZE211a     49
#define MSG211b   "211- Extensions supported in Nucleus FTP Server\r\n\
 SIZE\r\n\
 REST STREAM\r\n\
211 End\r\n"
#define MSG214    "214- The following commands are recognized (* =>'s unimplemented).\r\n"
#define SIZE214      68
#define MSG214a     "  USER    REIN*   TYPE    STOR    RNFR    CWD     SYST    CDUP    STOU*\r\n"
#define MSG214b     "  PASS    QUIT    STRU    APPE    RNTO    LIST    STAT    RMD     XRMD \r\n"
#define MSG214c     "  ACCT*   PORT    MODE    ALLO    ABOR    NLST    HELP    PWD     XMKD \r\n"
#define MSG214d     "  SMNT*   PASV    RETR    REST    DELE    SIZE    NOOP    MKD     XPWD \r\n"
#define SIZE214a    73
#define MSG214e     "214 ;-)\r\n"
#define SIZE214e    9
#define MSG215    "215 UNIX Type: L8 Version: Nucleus-ftpd\r\n"
#define SIZE215      41
#define MSG220    "220 Nucleus FTP Server (Version 1.7) ready.\r\n"
#define SIZE220      45
#define MSG221    "221 Service closing control connection.  Logged out if appropriate.\r\n"
#define SIZE221      69
#define MSG226    "226 Closing data connection.  Requested file action successful.\r\n"
#define SIZE226      65
#define MSG227    "227 Entering Passive Mode ("
#define SIZE227      27
#define MSG229    "229 Entering Extended Passive Mode (|||"
#define SIZE229      39
#define MSG230    "230 User logged in, proceed.\r\n"
#define SIZE230      30
#define MSG250    "250 Requested file action okay, completed.\r\n"
#define SIZE250      44
#define MSG257    "\" is the current working directory.\r\n"
#define SIZE257      42
#define MSG257a   "\" directory has been created.\r\n"
#define SIZE257a     36
#define MSG331    "331 User name okay, need password.\r\n"
#define SIZE331      36
#define MSG350    "350 Requested file action pending further information.\r\n"
#define SIZE350      56
#define MSG421    "421 Service not available, closing control connection.\r\n"
#define SIZE421      56
#define MSG425    "425 Cannot open data connection.\r\n"
#define SIZE425      34
#define MSG426    "426 Transfer aborted, closing control connection.\r\n"
#define SIZE426      51
#define MSG450    "450 Requested file action not taken.\r\n"
#define SIZE450      38
#define MSG451    "451 Requested action aborted: local error in processing.\r\n"
#define SIZE451      58
#define MSG452    "452 Requested action not taken: Insufficient storage space in system.\r\n"
#define SIZE452      71
#define MSG500    "500 Syntax error, command unrecognized.\r\n"
#define SIZE500      41
#define MSG501    "501 Syntax error in parameters or arguments.\r\n"
#define SIZE501      46
#define MSG503    "503 Bad sequence of commands.\r\n"
#define SIZE503      31
#define MSG504    "504 Command not implemented for that parameter.\r\n"
#define SIZE504      49

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
#define MSG522    "522 Network protocol not supported, use (1,2).\r\n"
#define SIZE522      48
#else
#define MSG522    "522 Network protocol not supported, use (1).\r\n"
#define SIZE522      46
#endif

#define MSG530    "530 Not logged in.\r\n"
#define SIZE530      20
#define MSG550    "550 Requested file action not taken.\r\n"
#define SIZE550      38
#define MSG553    "553 Requested action not taken.  File name not allowed.\r\n"
#define SIZE553      57
#define ERRORMSG    "ERROR: This ftp server was built using the In-Memory File System.\r\n\
This file system does not provide the functionality required by FTP.\r\n\
Please see the FTP Server section of the Extended Protocol Manual for more information \
regarding FTP file systems.\r\n"
#define ERRORMSGSZ  252

#define FTPS_RN_SEQ                 1
#define FTPS_RN_DIR                 2
#define FTPS_RN_FILE                4
#define FTPS_RN_MOVE                8
#define FTPS_TRANSFER_BLOCK_SIZE    256
#define FTPS_FILE_ERROR             -1

/*** Fixes for defect 10296. ***/
/* Abstract test for directory delimiter when parsing RX pathnames. */
#define FTPS_IS_PATH_DELIMITER(c)  ((c == '\\') || (c == '/'))

/* Character to use as our local directory delimiter. This should be defined
   by the local filesystem. */
#define FTPS_NATIVE_PATH_DELIMITER          '\\'
#define FTPS_NATIVE_PATH_DELIMITER_STR      "\\"

#define FTPS_TX_PATH_DELIMITER              '/'
#define FTPS_TX_PATH_DELIMITER_STR          "/"
/*** End of fix. ***/

typedef struct _FSP_CB
{
    INT     fsp_socketd;
    INT     fsp_drive;
    INT16   fsp_family;
    UINT8   padN[2];
    NU_TASK *fsp_currentID;
} FSP_CB;

typedef struct FLAGS
{
   INT16    userFlag;
   INT16    passFlag;
   INT16    typeFlag;
   INT16    modeFlag;
   INT16    struFlag;
   INT16    rnfrFlag;
   INT16    taskFlag;
   INT16    pasvFlag;
}CMD_FLAGS;

typedef struct FTP_SERVER_STRUCT
{
   UNSIGNED           validPattern;
   NU_TASK            *parentID;
   NU_TASK            *taskID;
   NU_TASK            *datatask;
   NU_SEMAPHORE       fileWriteLock;
   CHAR               taskName[8];
   DATA_ELEMENT       taskStatus;
   UNSIGNED           scheduleCount;
   OPTION             priority;
   OPTION             preempt;
   UNSIGNED           timeSlice;
   VOID               *stackBase;
   UNSIGNED           stackSize;
   UNSIGNED           minStack;
   struct addr_struct serverAddr;
   struct addr_struct serverDataAddr;
   struct addr_struct clientAddr;
   struct addr_struct clientDataAddr;
   INT                socketd;
   INT                dataSocketd;
   INT                transferType;
   INT                lastError;
   INT                stackError;
   INT                replyIndex, replyTail;
   INT                defaultDrive;
   
  
#ifdef NET_5_1
   NET_BUFFER         *ftpsDataBuf;
#endif
   
   CHAR               *replyBuff;
   CHAR               *fileSpec;
   CHAR               *path;
   CHAR               *renamePath;
   CHAR               *currentWorkingDir;

   
   CHAR               *filename;
   CHAR               *renameFile;

   
   struct FLAGS       cmdFTP;
   CHAR               user[FTP_MAX_ID_LENGTH];
   NU_EVENT_GROUP     FTP_Events;
   STATUS             transferStatus;
   INT32              restart;
} FTP_SERVER;


/* this is the structure that will hold the password list */
typedef struct FTPSacct
{
    CHAR id[FTP_MAX_ID_LENGTH];
    CHAR pw[FTP_MAX_PW_LENGTH];
} FTPSACCT;

/* ASCII total of the commands */
#define USER                319
#define PASS                311
#define QUIT                323
#define ACCT                283
#define FEAT                288
#define CWD                 222
#define CDUP                300
#define XCWD                310
#define PORT                325
#define PASV                314
#define TYPE                322
#define MODE                293
#define STRU                334
#define RETR                317
#define EPSV_REST           318
#define STOR                328
#define APPE                294
#define RNFR                312
#define RNTO_XPWD           323
#define ABOR                292
#define DELE                282
#define EPRT_XRMD_SIZE      315
#define RMD                 227
#define MKD                 220
#define XMKD                308
#define PWD                 235
#define NLST                321
#define LIST_STAT_NOOP      316
#define SYST                339
#define FST_HELP            297

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

