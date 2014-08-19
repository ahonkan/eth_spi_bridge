/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*                                                                            
* FILE NAME                                                                         
*                                                                                    
*   dhcpsrv.h                                  1.2
*
* COMPONENT
*
*   Nucleus DHCP Server
*
* DESCRIPTION
*
*   Definitions for creating DHCP resources.
*
* DATA STRUCTURES
*
*   None
*
* DEPENDENCIES
*
*   None
*
**************************************************************************/

#ifndef _DHCPSRV_H_
#define _DHCPSRV_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define END_OF_ENTRY                1       /* end fo entry delimiter has been encountered 
                                                flag */    
#define INFINITY                    0xFFFFFFFFUL  

#define OPEN_BRACKET                0x7B          /* { */
#define CLOSE_BRACKET               0x7D          /* } */
#define COLON                       0x3A          /* entry delimiter */
#define OR_SYMBOL                   0x7C          /* entry delimiter */
#define CARAT                       0x5E          /* entry delimiter */    
#define END_OF_CONFIG               0x3A3A247DUL  /* end of configuration control block marker */
#define END_OF_BINDING              0x3A3A407DUL  /* end of binding marker */    
#define END_OF_OPTIONS              0x3A3A237DUL  /* end of options marker */    
#define END_OF_BINDING_FILE         0x7DC0C1C2UL  /* end of binding file marker */
#define END_OF_OPTIONS_FILE         0x7DC0C1C2UL  /* end of options block file marker */
#define END_OF_CONFIG_FILE          0x7DC0C1C2UL  /* end of config file marker */
#define CONFIG_FILE_BUFF_OVERHEAD   50              /* Memory overhead to account for delimiters. */
#define DEFAULT_RETURN_OPTIONS      0x0133363A3BFF /* Default options that are to be returned to a
                                                    requesting client: subnet mask, lease time, 
                                                    server ID, renewal time, and rebinding time. */
#define END_OF_FILE                 1           /* end of file has been encountered flag */
#define NEW_CONFIG_FOR_BINDINGS     2           /* the bindings that have been read are for a different
                                                    configuration cotrol block */

#define OPTION_COOKIE_INCREMENT     4           /* option cookie length */
#define OVERFLOW_IN_FILE            1           /* if the options field is longer than the */
#define OVERFLOW_IN_SNAME           2           /*  option field lenght, then the overflow */
#define OVERFLOW_IN_BOTH            3           /*  will be placed in the specified location */

/* Option tags for each of the DHCP options */
#define PAD                     0
#define SUBNET_MASK             1
#define TIME_OFFSET             2
#define ROUTER                  3
#define TIME_SERVER             4
#define NAME_SERVER             5
#define DNS_SERVER_ADDR         6
#define LOG_SERVER              7
#define COOKIE_SERVER           8
#define LPR_SERVER              9
#define IMPRESS_SERVER          10
#define RESOURCE_LOC_SERVER     11
#define HOST_NAME               12
#define BOOT_FILE_SIZE          13
#define MERIT_DUMP_FILE         14
#define DNS_DOMAIN_NAME         15
#define SWAP_SERVER             16
#define ROOT_PATH               17
#define EXTENSIONS_PATH         18
#define IP_FORWARD              19
#define NONLOCAL_SRC_ROUTE      20
#define POLICY_FILTER           21
#define MAX_DGRAM_ASSEM_SIZE    22
#define DEFAULT_IP_TTL          23
#define MTU_AGING_TIMEOUT       24
#define MTU_PLATEAU_TABLE       25
#define INTERFACE_MTU           26
#define ALL_SUBNET_LOCAL        27
#define BRDCAST_ADDR            28
#define MASK_DISCOVER           29
#define MASK_SUPPLIER           30
#define ROUTER_DISCOVERY        31
#define ROUTER_SOLICIT          32
#define STATIC_ROUTE            33
#define TRAILER_ENCAP           34
#define ARP_CACHE_TIMEOUT       35
#define ETHER_ENCAP             36
#define DEFAULT_TCP_TTL         37
#define KEEPALIVE_INTERVAL      38
#define KEEPALIVE_GARBAGE       39
#define NIS_DOMAIN_NAME         40
#define NIS_SERVER              41
#define NTP_SERVER              42
#define VENDOR_SPECIFIC         43
#define NETBIOS_NAME_SERVER     44
#define NETBIOS_DIST_SERVER     45
#define NETBIOS_NODE_TYPE       46
#define NETBIOS_SCOPE           47
#define XWINDOW_FONT_SERVER     48
#define XWINDOW_DISPLAY_MANAGER 49
#define ADDRESS_REQUEST         50
#define ADDRESS_TIME            51
#define OPTION_OVERLOAD         52
#define DHCPS_MSG_TYPE          53
#define DHCPS_SERVER_ID         54
#define PARAMETER_LIST          55
#define DHCPS_MESSAGE           56
#define DHCPS_MAX_MSG_SIZE      57
#define DHCPS_RENEWAL_T1        58
#define DHCPS_REBIND_T2         59
#define CLASS_ID                60
#define CLIENT_ID               61
#define NETWARE_DOMAIN_NAME     62
#define NETWARE_OPTIONS         63
#define NISPLUS_DOMAIN_NAME     64
#define NISPLUS_SERVER          65
#define TFTP_SERVER_NAME        66
#define BOOTFILE_NAME           67
#define MOBILE_IP_HOME_AGENT    68
#define SMTP_SERVER             69
#define POP_SERVER              70
#define NNTP_SERVER             71
#define WWW_SERVER              72
#define FINGER_SERVER           73
#define IRC_SERVER              74
#define STREETTALK_SERVER       75
#define STREETTALK_DIR_ASST_SERVER 76
#define USER_CLASS_INFO         77
#define DIRECTORY_AGENT_INFO    78

/* The delay in waiting for disk (in seconds). */
#define DHCPS_DISK_TIMEOUT_DELAY 30

/* These are macros that are used by the server for
    internal options that do not have a specific option
    number associated with them. */
#define STRUCTURE_STATUS_FLAGS      240
#define CONFIG_CONTROL_BLOCK_NAME   241
#define NETWORK_INTERFACE_NAME      242
#define APPEND_ENTRY                243
#define SIADDR                      244
#define ALLOW_BOOTP                 245
#define IP_ADDRESS                  246
#define MAX_LEASE                   247
#define DEFAULT_LEASE               248
#define IP_ADDRESS_RANGE            249
#define STATIC_IP_ALLOCATION        250
#define DHCP_OFFERED_WAIT           251
#define SUBNET_ADDRESS              252
#define NUMBER_CONFIG_STRUCTS       253
#define NUMBER_IP_ADDRS             254


/* functions prototype definition */
INT    DHCPSDB_Isspace(register INT);

/* This macro is to exclude unsupported options from the build */
#define DHCPS_UNSUPPPORTED_OPTION   NU_FALSE                

/* This macro is used to remove warnings. */
#define DHCPS_UNUSED_PARAMETER(x)  DHCPS_Unused_Parameter = ((UINT32)(x))

/* Function prototype */
VOID   DHCPSDB_Adjust_Buffer(CHAR **);
STATUS DHCPSDB_Get_String(CHAR **, CHAR *);
VOID   DHCPSDB_Eat_Whitespace(CHAR **);
VOID   DHCPS_Process_Options_Block_Entry(CHAR *, const CHAR *);
STATUS DHCPSDB_IP_Compare(const UINT8 *, const UINT8 *);
STATUS DHCPSDB_Supported_Options_List(const CHAR *, struct OPTIONMAP *);
STATUS DHCPSDB_Calculate_Subnet_Number (const struct id_struct *, const struct id_struct *, struct id_struct *);
DHCPS_BINDING *DHCPS_Add_Binding_To_Offer(DHCPS_CONFIG_CB *, DHCPS_BINDING *);
DHCPS_BINDING *DHCPS_Get_Binding_For_Lease(const DHCPS_CONFIG_CB *, const DHCPS_CLIENT_ID *, 
                                            const struct id_struct *);

UINT32 DHCPS_Choose_Lease_Length (const DHCPS_CONFIG_CB *, const DHCPS_BINDING *);

STATUS DHCPS_Evaluate_Symbol(CHAR **, DHCPS_CONFIG_CB *);

VOID   DHCPS_Get_Client_id(DHCPLAYER *, DHCPS_CLIENT_ID *);
STATUS DHCPS_Get_File_Entry(CHAR *, UINT16 *, const CHAR *, INT);
UINT8 *DHCPS_Get_Option(DHCPLAYER *, CHAR); 

VOID   DHCPS_Remove_Offer(DHCPS_OFFERED *);
VOID   DHCPS_Process_Binding_Entry(CHAR *, const CHAR *);
STATUS DHCPS_Process_Configuration_Entry(DHCPS_CONFIG_CB *, CHAR *);

STATUS DHCPS_Send(const DHCPLAYER *, UINT8, DHCPS_BINDING *, INT, struct id_struct *);

STATUS DHCPS_Process_Discover(DHCPS_CONFIG_CB *, DHCPLAYER *, INT, struct id_struct *);
STATUS DHCPS_Process_Request(const DHCPS_CONFIG_CB *, DHCPLAYER *,  INT, struct id_struct *);
STATUS DHCPS_Process_Release(const DHCPS_CONFIG_CB *, DHCPLAYER *);
VOID   DHCPS_Process_Decline(const DHCPS_CONFIG_CB *, DHCPLAYER *);
STATUS DHCPS_Process_Inform (DHCPS_CONFIG_CB *, const DHCPLAYER *,  INT, struct id_struct *);

VOID   DHCPS_Update_Lease_Times(DHCPS_BINDING *);

UINT8 *DHCPS_Add_Option_To_Memory_Block (DHCPS_CONFIG_CB *, const struct id_struct *, UINT8, 
                                            DHCPS_OPTIONS **);
STATUS DHCPS_Remove_Option_From_Memory_Block (const DHCPS_CONFIG_CB *, const struct id_struct *, 
                                              UINT8, const VOID *);
UINT8 *DHCPS_Get_Option_From_Memory_Block (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8);

DHCPS_CONFIG_CB *DHCPS_Find_Configuration_Control_Block (const CHAR *, const struct id_struct *);
UINT8 DHCPS_Get_Option_From_Configuration(DHCPS_BINDING *, UINT8, UINT8, UINT8 *);

STATUS DHCPS_Add_Entry_To_Array (struct id_struct *, const struct id_struct *, UINT8);
STATUS DHCPS_Remove_Entry_From_Array (struct id_struct *, const  struct id_struct *, UINT8);

STATUS DHCPS_Save_Config_Struct_To_File(VOID);
STATUS DHCPS_Save_Binding_To_File(VOID);
STATUS DHCPS_Save_Options_Block_To_File(VOID);

UINT32 DHCPS_Check_Duetime(UINT32);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _DHCPSRV_H_ */

