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
*       usm.c                                                    
*
*   DESCRIPTION
*
*       This file contains functions specific to the User-Based Security
*       Model.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       USM_Init
*       USM_Config
*       USM_Secure
*       USM_Verify
*       USM_Lookup_Users
*       USM_Lookup_Priv_Prot
*       USM_Lookup_Auth_Prot
*       USM_Compare_Index
*       USM_Save_User
*       USM_Add_User_Util
*       USM_Add_User
*       USM_Key_Change
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp.h
*       asn1.h
*       snmp_dis.h
*       usm.h
*       snmp_utl.h
*       nu_sd.h
*       ncl.h
*       snmp_file.h
*       snmp_api.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/snmp.h"
#include "networking/asn1.h"
#include "networking/snmp_dis.h"
#include "networking/usm.h"
#include "networking/snmp_utl.h"

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
#include "drivers/serial.h"
#include "networking/ncl.h"
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif
#include "networking/snmp_api.h"

#if( INCLUDE_SNMPv3 )

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
extern NU_SERIAL_PORT           *Snmp_Serial_Port;
#endif

/* Maximum size of global data. */
#define USM_MAX_GLOBAL_DATA_SIZE        20
#define USM_MAX_SECURITY_PARAM_SIZE     300

/* This is the size of a temporary buffer for Global Data and
 * Security Parameters.
 */
#define USM_TEMP_BUFF_SIZE  \
    (USM_MAX_GLOBAL_DATA_SIZE + USM_MAX_SECURITY_PARAM_SIZE)

/* MIB for SNMP Engine. */
extern SNMP_ENGINE_STRUCT       Snmp_Engine;

/* SNMP Stats. */
extern snmp_stat_t              SnmpStat;

extern USM_AUTH_PROT_STRUCT Usm_Auth_Prot_Table[USM_MAX_AUTH_PROTOCOLS];
extern USM_PRIV_PROT_STRUCT Usm_Priv_Prot_Table[USM_MAX_PRIV_PROTOCOLS];
STATIC USM_CACHED_SECURITY_STRUCT Usm_Cached_Security_Data
                                                    [USM_MAX_CACHED_DATA];

extern NU_MEMORY_POOL System_Memory;

#if (INCLUDE_MIB_MPD == NU_TRUE)
extern SNMP_MPD_MIB_STRUCT             Snmp_Mpd_Mib;
#endif

/* MIB for USM */
USM_MIB_STRUCT                  Usm_Mib;
STATIC UINT8                    Usm_Status = SNMP_MODULE_NOTINITIALIZED;
#if (USM_MAX_USER_USERS)
extern SNMP_USM_USER_STRUCT     Usm_User_Table[];
#endif
/************************************************************************
*
*   FUNCTION
*
*       USM_Init
*
*   DESCRIPTION
*
*       This function initializes the User-based Security Model.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS      Initialization successful.
*       SNMP_ERROR      Initialization unsuccessful.
*
*************************************************************************/
STATUS USM_Init(VOID)
{

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* USM user structure. */
    USM_USERS_STRUCT    usm_user;

    SNMP_READ_FILE      table_information =
        /* File name, Structure pointer, Sizeof structure, Insert
         * function.
         */
        {USM_FILE, NU_NULL,
         (SNMP_ADD_MIB_ENTRY) USM_Add_User, sizeof(USM_USERS_STRUCT)};

#endif

    /* Status of the request. */
    STATUS              status;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Assign a location that will be used to read data from file. */
    table_information.snmp_read_pointer = &usm_user;

    /* Read the table from file. */
    status = SNMP_Read_File(&table_information);

    /* If we successfully read data from file. */
    if(status == NU_SUCCESS)
    {
       /* USM successfully initialized. */
       Usm_Status = SNMP_MODULE_INITIALIZED;
    }
    else
#endif
    {
        Usm_Status = SNMP_MODULE_NOTINITIALIZED;
        status = SNMP_ERROR;
    }

    /* Return status. */
    return (status);

} /* USM_Init */

/************************************************************************
*
*   FUNCTION
*
*       USM_Config
*
*   DESCRIPTION
*
*       This function configures the User-based Security Model.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Initialization successful.
*       SNMP_ERROR              Initialization unsuccessful.
*
*************************************************************************/
STATUS USM_Config(VOID)
{
    /* Status to return success or error code. */
    STATUS                  status = NU_SUCCESS;

    /* Variable to use in for loop. */
    INT                     i;

    /* Handle to the USM user structure. */
    USM_USERS_STRUCT        user;

    /* Handle to authentication protocol. */
    USM_AUTH_PROT_STRUCT    *auth_prot;


#if (SNMP_CONFIG_SERIAL == NU_TRUE)

    /* config_option determines the initial configuration. The following
     * values are meaningful:
     *      1. Default configuration as specified in snmp_cfg.c
     *      2. No access configuration.     
     */
    UINT8           config_option = 1;

    /* Variable to hold serial input. */
    CHAR            input[2];
#endif


    /* Wait for the USM to be initialized. */
    while(Usm_Status == SNMP_MODULE_NOTSTARTED)
    {
        /* Sleep for a second. */
        NU_Sleep(NU_PLUS_Ticks_Per_Second);
    }

#if (USM_MAX_USER_USERS > 0)

    if (Usm_Status == SNMP_MODULE_NOTINITIALIZED)
    {
#if (SNMP_CONFIG_SERIAL == NU_TRUE)
        /* Null terminating input. */
        input[1] = 0;

        /* Determine the Configuration Option required by the user. */

        /* This is the USM Configuration Wizard. */
        NU_SD_Put_String("*************************************",
                            Snmp_Serial_Port);

        NU_SD_Put_String("*************************************\n\r",
                            Snmp_Serial_Port);

        NU_SD_Put_String("Copyright (c) 1998 - 2007 MGC - Nucleus ",
                            Snmp_Serial_Port);

        NU_SD_Put_String("SNMP - USM Configuration Wizard",
                            Snmp_Serial_Port);

        NU_SD_Put_String("\n\r", Snmp_Serial_Port);

        NU_SD_Put_String("*************************************",
                            Snmp_Serial_Port);

        NU_SD_Put_String("*************************************\n\r",
                            Snmp_Serial_Port);

        NU_SD_Put_String("1. Default configuration as specified in "
                         "snmp_cfg.c", Snmp_Serial_Port);

        NU_SD_Put_String("\n\r", Snmp_Serial_Port);

        NU_SD_Put_String("2. No access configuration.",
                         Snmp_Serial_Port);

        NU_SD_Put_String("\n\r", Snmp_Serial_Port);

        NU_SD_Put_String("\n\n\r", Snmp_Serial_Port);

        NU_SD_Put_String("Enter Configuration Option: ",
                            Snmp_Serial_Port);

        /* Wait for an input. */
        for(;;)
        {
            while (!NU_SD_Data_Ready(Snmp_Serial_Port));

            input[0] = SDC_Get_Char(Snmp_Serial_Port);

            /* Convert from hexadecimal to digit. */
            config_option = (UINT8)NU_AHTOI(input);

            if (config_option <= 2 && config_option >= 1)
            {
                /* Echo back the input. */
                NU_SD_Put_Char(input[0], Snmp_Serial_Port, NU_FALSE);

                NU_SD_Put_String("\n\n\r", Snmp_Serial_Port);

                /* Break out of the loop. */
                break;
            }
        }

        if (config_option == 1)
#endif
        {
            for (i = 0; i < USM_MAX_USER_USERS; i++)
            {
                /* Clear out user structure. */
                UTL_Zero(&user, sizeof(user));

                /* Copy engine ID. */
                NU_BLOCK_COPY(user.usm_user_engine_id,
                            Snmp_Engine.snmp_engine_id,
                            (unsigned int)Snmp_Engine.snmp_engine_id_len);

                /* Update the engine ID length. */
                user.usm_user_engine_id_len =
                                    (UINT8)Snmp_Engine.snmp_engine_id_len;

                /* Update user name. */
                strcpy((CHAR *)(user.usm_user_name),
                       Usm_User_Table[i].usm_user_name);

                /* Update the value of security name. */
                strcpy((CHAR *)(user.usm_security_name),
                       Usm_User_Table[i].usm_user_name);

                /* Set Authentication Protocol. */
                user.usm_auth_index = Usm_User_Table[i].usm_auth_index;

                /* Set Privacy Protocol. */
                user.usm_priv_index = Usm_User_Table[i].usm_priv_index;

#if (INCLUDE_MIB_USM == NU_TRUE)
                /* Set storage type to permanent. */
                user.usm_storage_type = SNMP_STORAGE_PERMANENT;

                /* Set row status as active. */
                user.usm_status = SNMP_ROW_ACTIVE;
#endif

                /* Get the Authentication Protocol. */
                auth_prot = USM_Lookup_Auth_Prot(user.usm_auth_index);

                /* If we did not get the handle to the Authentication
                 * Protocol or we don't have the handle to the hash
                 * function of this Authentication Protocol then return
                 * error code.
                 */
                if(!auth_prot)
                {
                    /* Return error code. */
                    status = SNMP_ERROR;

                    /* Break through the loop so that we can return error
                     * code.
                     */
                    break;
                }

#if (USM_PASSWORD_2_KEY == NU_TRUE)
                else
                {
                        /*If the length of the password provided is
                        less than 8 return error RFC 3414 shorter 
                        if the password to key algorithm is not used 
                        shorter passwords are allowed*/
                        if(strlen(Usm_User_Table[i].usm_auth_password)<8 
                            ||strlen(Usm_User_Table[i].usm_priv_password)<8)
                        {
                            status = SNMP_ERROR;
                            NLOG_Error_Log("SNMP: Password length should be greater than eight characters", 
                                NERR_SEVERE,
                                __FILE__, __LINE__);
                        }
                }
#endif
                /* If no error till now then proceeds. */
                if (status == NU_SUCCESS)
                {
                    /* If we don't have handle to the hash function of
                     * authentication then don't encrypt passwords.
                     */
                    if (auth_prot->usm_password_cb)
                    {
                        /* Get the Authentication Key. */
#if (USM_PASSWORD_2_KEY == NU_TRUE)
                        auth_prot->usm_password_cb((UINT8* )(Usm_User_Table[i].
                                                           usm_auth_password),
                                             (UINT8)(strlen(Usm_User_Table[i].
                                                          usm_auth_password)),
                                               user.usm_auth_key);
#else
                        auth_prot->usm_password_cb((UINT8* )(Usm_User_Table[i].
                                                           usm_auth_password),
                                                    auth_prot->key_length,
                                                    user.usm_auth_key);
#endif
                        /* Update the user own authentication key with
                         * authentication key.
                         */
                        NU_BLOCK_COPY(user.usm_own_auth_key,
                                      user.usm_auth_key,
                                      auth_prot->key_length);

                    /* Get the Privacy Key. */
#if (USM_PASSWORD_2_KEY == NU_TRUE)
                        auth_prot->usm_password_cb((UINT8 *)(Usm_User_Table[i].
                                                           usm_priv_password),
                                             (UINT8)(strlen(Usm_User_Table[i].
                                                          usm_priv_password)),
                                                   user.usm_priv_key);
#else
                        auth_prot->usm_password_cb((UINT8 *)(Usm_User_Table[i].
                                                           usm_priv_password),
                                                   auth_prot->key_length,
                                                   user.usm_priv_key);
#endif

                        /* Ensure that the privacy protocol being used
                         * receives the correct key length.
                         */
                        if(Usm_User_Table[i].usm_priv_index == USM_DES)
                        {
                            /* Truncate any extra bytes computed during
                             * key generation. This is required when SHA
                             * is used with DES and SHA generates 20 byte
                             * key and DES needs only the first 16 bytes.
                             */
                            UTL_Zero(&user.usm_priv_key[USM_DES_PRIV_KEY_SIZE],
                                USM_KEYCHANGE_MAX_SIZE-USM_DES_PRIV_KEY_SIZE);
                        }

                        /* Update the user owned privacy key. */
                        NU_BLOCK_COPY(user.usm_own_priv_key,
                                      user.usm_priv_key, USM_DES_PRIV_KEY_SIZE);
                    }

                    /* Add this entry to the usm_user_table. */
                    status = USM_Add_User((USM_USERS_STRUCT *)&user);

                    /* Was call to USM_Add_User successful? */
                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: USM_Add_User failed in "
                                       "USM_Config", NERR_SEVERE, __FILE__,
                                       __LINE__);

                        break;
                    }
                }
            }
#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
            if (status == NU_SUCCESS)
            {
                /* Create the file. */
                status = SNMP_Create_File(USM_FILE);

                if (status == NU_SUCCESS)
                {
                    status = USM_Save_User(Usm_Mib.usm_user_table.next);
                }
            }
#endif
        }
    }

#else
    status = SNMP_ERROR;
#endif

    /* Return success or error code. */
    return (status);

} /* USM_Config */

/************************************************************************
*
*   FUNCTION
*
*       USM_Secure
*
*   DESCRIPTION
*
*       This function secures the outgoing message
*
*   INPUTS
*
*       snmp_mp                 Message processing model.
*       **whole_message         The whole message to be sent.
*       *msg_len                Message length
*       max_msg_size            Maximum message size for the Manager.
*       snmp_sm                 Security model
*       *security_engine_id     Authoritative Engine Id
*       security_engine_id_len  Authoritative Engine Id length
*       *security_name          Security name of the principal
*       security_level          Security level
*       *scoped_pdu             Pointer to the scoped PDU
*       *security_state_ref     Security information for the session
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Secure(UINT32 snmp_mp, UINT8 **whole_message, UINT32 *msg_len,
        UINT32 max_msg_size, UINT32 snmp_sm, UINT8 *security_engine_id,
        UINT32 security_engine_id_len, UINT8 *security_name,
        UINT8 security_level, UINT8 *scoped_pdu, VOID *security_state_ref)
{
    /* Handle to the USM user structure. */
    USM_USERS_STRUCT        *user;

    /* Handle to the USM Authentication protocol. */
    USM_AUTH_PROT_STRUCT    *auth_prot;

    /* Handle to the USM Privacy protocol. */
    USM_PRIV_PROT_STRUCT    *priv_prot;

    /* Privacy key change. */
    UINT8                   priv_key_change[USM_KEYCHANGE_MAX_SIZE];

    /* Authentication key change. */
    UINT8                   auth_key_change[USM_KEYCHANGE_MAX_SIZE];

    /* Key length. */
    UINT8                   key_length = 0;

    /* Asn1 structure. */
    asn1_sck_t              asn1;

    /* Pointer to end of ASN1 class. */
    UINT8                   *end = NU_NULL;

    UINT8                   temp_octet[SNMP_SIZE_BUFCHR];
    UINT32                  temp_len;
    static UINT8            temp_snmp_buff[USM_TEMP_BUFF_SIZE];
    UINT32                  buff_len;
    UINT32                  time;
    STATUS                  status = NU_SUCCESS;
    UINT8                   *security_param;
    UINT32                  security_param_len;
    UINT8                   *auth_param = NU_NULL;

    /* Unused Parameters. */
    UNUSED_PARAMETER(max_msg_size);
    UNUSED_PARAMETER(snmp_sm);
    UNUSED_PARAMETER(security_engine_id_len);

    /* Get the user from the usm_user_table if a securityReference is not
     * passed.
     */
    if (security_state_ref == NU_NULL)
    {
        /* Get the handle to the USM user structure. */
        user = USM_Lookup_Users(security_name);

        /* Was a user found? */
        if((user == NU_NULL) ||
           (user->usm_user_engine_id_len !=
                            ((UINT8)(Snmp_Engine.snmp_engine_id_len)) ) ||
           (memcmp(user->usm_user_engine_id, Snmp_Engine.snmp_engine_id,
                   user->usm_user_engine_id_len) != 0))
        {
            /* Return error code. */
            return (USM_UNKOWNSECURITYNAME_ERROR);
        }

        /* Get the Authentication Protocol. */
        auth_prot = USM_Lookup_Auth_Prot(user->usm_auth_index);

        /* Get the Privacy Protocol. */
        priv_prot = USM_Lookup_Priv_Prot(user->usm_priv_index);

        /* Get the keys. */
        if(auth_prot != NU_NULL)
        {
            key_length = auth_prot->key_length;
            NU_BLOCK_COPY(auth_key_change, user->usm_auth_key,
                         key_length);
            NU_BLOCK_COPY(priv_key_change, user->usm_priv_key,
                          key_length);
        }

    }
    else
    {
        strcpy((CHAR *)security_name,
            (CHAR *)(((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                                        usm_msg_user_name));

        /* Get the Privacy Protocol. */
        priv_prot =
            ((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                            usm_priv_protocol;

        /* Get the Authentication Protocol. */
        auth_prot = ((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                                    usm_auth_protocol;

        /* Get the keys. */
        if(auth_prot != NU_NULL)
        {
            /* Get key length. */
            key_length = auth_prot->key_length;

            /* Get Privacy key change. */
            NU_BLOCK_COPY(priv_key_change,
                    (((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                    usm_priv_key), key_length);

            /* Get Authentication key change. */
            NU_BLOCK_COPY(auth_key_change,
                    (((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                    usm_auth_key), key_length);

        }

        /* Set engine ID to the local Engine ID. */
        NU_BLOCK_COPY(security_engine_id, Snmp_Engine.snmp_engine_id,
                      (unsigned int)Snmp_Engine.snmp_engine_id_len);

        /* Clear the cache. */
        ((USM_CACHED_SECURITY_STRUCT *)security_state_ref)->
                                            usm_is_occupied = NU_FALSE;
    }

    /* Copy data from whole_message up to where the scoped_pdu starts in
     * to temp_snmp_buff.
     */
    buff_len = (UINT32)(scoped_pdu - (*whole_message));
    NU_BLOCK_COPY(temp_snmp_buff, *whole_message,
                  (unsigned int)buff_len);

    /* Update the whole message. */
    *msg_len -= buff_len;
    *whole_message += buff_len;

    /* Open ASN for encoding securityParam. This is placed at the end of
     * the temp_snmp_buff.
     */
    Asn1Opn(&asn1, temp_snmp_buff + buff_len,
            USM_TEMP_BUFF_SIZE - buff_len, ASN1_ENC);

    /* Check whether the user supports the security level being requested.
     */
    switch (security_level)
    {
    case SNMP_SECURITY_AUTHPRIV:

        /* Check that a privacy protocol is supported. */
        if(priv_prot == NU_NULL || priv_prot->usm_encrypt_cb == NU_NULL)
        {
            Usm_Mib.usm_stats_tab.usm_unsupported_sec_level++;
            status = USM_UNSUPPORTED_SECURITY_LEVEL;
            break;
        }

    case SNMP_SECURITY_AUTHNOPRIV:

        /* Check that an authentication protocol is supported. */
        if(auth_prot == NU_NULL || auth_prot->usm_secure_cb == NU_NULL)
        {
            Usm_Mib.usm_stats_tab.usm_unsupported_sec_level++;
            status = USM_UNSUPPORTED_SECURITY_LEVEL;
            break;
        }

        if(security_level == SNMP_SECURITY_AUTHPRIV)
        {
            /* If privacy is required, do the encryption. */
            if(priv_prot->usm_encrypt_cb(priv_key_change, key_length,
                            whole_message, msg_len, temp_octet,
                            &temp_len, (UINT16)(*msg_len + 8)) != NU_SUCCESS)
            {
                status = USM_ENCRYPTION_ERROR;
                break;
            }

            /* Encode the msgPrivacyParameters. */
            else if (!Asn1OtsEnc (&asn1, &end, temp_octet, temp_len))
            {
                status = SNMP_ERROR;
                break;
            }

            /* This is the header for the msgPrivacyParameters. */
            else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI,
                                    ASN1_OTS))
            {
                status = SNMP_ERROR;
                break;
            }

        }


    case SNMP_SECURITY_NOAUTHNOPRIV:

        if(security_level != SNMP_SECURITY_AUTHPRIV)
        {
            /* Encode 0 length msgPrivacyParameters, since privacy is not
             * supported.
             */
            if (!Asn1HdrEnc (&asn1, NU_NULL, ASN1_UNI, ASN1_PRI,
                                ASN1_OTS))
            {
                status = SNMP_ERROR;
                break;
            }

        }

        /* If Authentication is required then encode the auth parameters.
         */
        if(security_level > SNMP_SECURITY_NOAUTHNOPRIV)
        {
            /* Encode 0 octets in the msgAuthenticationParam. */
            UTL_Zero(temp_octet, (UNSIGNED)auth_prot->usm_param_length);

            if (!Asn1OtsEnc (&asn1, &auth_param, temp_octet,
                             auth_prot->usm_param_length))
            {
                status = SNMP_ERROR;
                break;
            }

            else if (!Asn1HdrEnc (&asn1, auth_param, ASN1_UNI, ASN1_PRI,
                                  ASN1_OTS))
            {
                status = SNMP_ERROR;
                break;
            }
        }
        else
        {
            /* Encode the msgAuthenticationParameters (0 Length), because
             * authentication is not supported.
             */
            if (!Asn1HdrEnc (&asn1, NU_NULL, ASN1_UNI, ASN1_PRI,
                             ASN1_OTS))
            {
                status = SNMP_ERROR;
                break;
            }
        }

        /* Encode the usm_msg_user_name. */
        if (!Asn1OtsEnc (&asn1, &end, security_name,
                        (UINT32)(strlen((CHAR *)security_name))))
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI, ASN1_OTS))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the msgAuthoritativeEngineTime. */

        /* Retrieve the time. */
        else if(SNMP_Engine_Time(&time) != NU_SUCCESS)
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1IntEncUns (&asn1, &end, time))
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI, ASN1_INT))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the msgAuthoritativeEngineBoots. */
        else if (!Asn1IntEncUns (&asn1, &end,
                                    Snmp_Engine.snmp_engine_boots))
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI, ASN1_INT))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the msgAuthoritativeEngineID. */
        else if (!Asn1OtsEnc (&asn1, &end, Snmp_Engine.snmp_engine_id,
                                Snmp_Engine.snmp_engine_id_len))
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI, ASN1_OTS))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the Sequence Header. */
        else if (!Asn1HdrEnc (&asn1,
                              (temp_snmp_buff + USM_TEMP_BUFF_SIZE),
                              ASN1_UNI, ASN1_CON, ASN1_SEQ))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the Octet String Header for the security Parameter. */
        else if (!Asn1HdrEnc (&asn1,
                                (temp_snmp_buff + USM_TEMP_BUFF_SIZE),
                                ASN1_UNI, ASN1_PRI, ASN1_OTS))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Close the ASN connection, update securityParam and security
         * ParamLen.
         */
        Asn1Cls(&asn1, &security_param, &security_param_len);

        /* Copy the Global Data, securityParam in to whole_message. */
        if((*msg_len += buff_len + security_param_len) <=
                                Snmp_Engine.snmp_max_message_size)
        {
            /* Copy the securityParam to the whole_message Location. */
            (*whole_message) -= security_param_len;
            NU_BLOCK_COPY((*whole_message), security_param,
                          (unsigned int)security_param_len);

            /* Calculate the position for the authentication parameter
             * with respect to the whole message.
             */
            auth_param = (*whole_message) + (auth_param - security_param);

            /* Copy back the contents of temp_snmp_buff to whole_message.
             */
            (*whole_message) -= buff_len;
            NU_BLOCK_COPY((*whole_message), temp_snmp_buff,
                          (unsigned int)buff_len);
        }

        /* The message is too big.*/
        else
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the SNMP Message header. */

        /* Setup the asn1 pointers. */
        asn1.Pointer = *whole_message;
        asn1.Begin = (*whole_message) - (Snmp_Engine.snmp_max_message_size
                                            - (*msg_len));
        asn1.End = (*whole_message) + (*msg_len);

        /* Encode the SNMP message version. */
        if (!Asn1IntEncUns (&asn1, &end, snmp_mp))
        {
            status = SNMP_ERROR;
            break;
        }

        else if (!Asn1HdrEnc (&asn1, end, ASN1_UNI, ASN1_PRI, ASN1_INT))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Encode the SNMP Sequence Header. */
        else if (!Asn1HdrEnc (&asn1, asn1.End, ASN1_UNI, ASN1_CON,
                                ASN1_SEQ))
        {
            status = SNMP_ERROR;
            break;
        }

        /* Close the ASN connection, update whole_message and msg_len. */
        Asn1Cls(&asn1, whole_message, msg_len);

        /* If Authentication is required then, do the authentication. */
        if(security_level > SNMP_SECURITY_NOAUTHNOPRIV)
        {

            /* Make a call to the Authentication Protocol. The last
             * argument is the location where msgAuthenticationParameters
             * is to be placed.
             */
            if(auth_prot->usm_secure_cb(auth_key_change, key_length,
                        *whole_message, *msg_len,
                        (auth_param - auth_prot->usm_param_length))
                                    != NU_SUCCESS)
            {
                status = USM_AUTHENTICATION_ERROR;
                break;
            }
        }

        break;

    default:
        Usm_Mib.usm_stats_tab.usm_unsupported_sec_level++;
        status = (USM_UNSUPPORTED_SECURITY_LEVEL);
        break;
    }

    return (status);

} /* USM_Secure */

/************************************************************************
*
*   FUNCTION
*
*       USM_Verify
*
*   DESCRIPTION
*
*       This function verifies the incoming message.
*
*   INPUTS
*
*       snmp_mp                 Message processing model.
*       max_msg_size            Maximum message size for the manager.
*       *security_param         Security Parameters.
*       snmp_sm                 Security Model
*       *security_level         Security Level
*       **whole_message         The whole message as received on the wire.
*       *msg_len                Message length
*       *security_engine_id     Authoritative engine id
*       *security_engine_id_len Authoritative engine id length
*       *security_name          Security name of the principal
*       *max_response_pdu       Maximum size for the response scoped PDU.
*       **security_state_ref    Security information for the session
*       *error_indication       Status information if any error was
*                               encountered.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Verify(UINT32 snmp_mp, UINT32 max_msg_size,
                  UINT8 *security_param, UINT32 snmp_sm,
                  UINT8 *security_level, UINT8 **whole_message,
                  UINT32 *msg_len, UINT8 *security_engine_id,
                  UINT32 *security_engine_id_len, UINT8 *security_name,
                  UINT32 *max_response_pdu, VOID **security_state_ref,
                  SNMP_ERROR_STRUCT *error_indication)
{
    STATUS      status = NU_SUCCESS;
    UINT8       *Eoc, *End = NU_NULL;
    UINT32      Cls, Con, Tag;
    UINT32      temp;
    UINT32      engine_boots = 0;
    UINT32      engine_time = 0;
    UINT32      i;
    asn1_sck_t  Asn1;
    UINT8       *auth_param = NU_NULL;
    UINT32      auth_param_len = 0;
    UINT8       priv_param[SNMP_SIZE_BUFCHR];
    UINT8       temp_engine_id[SNMP_SIZE_BUFCHR];
    UINT8       temp_security_name[SNMP_SIZE_BUFCHR];
    INT32       temp_no = 0;
    UINT32      priv_param_len = 0;
    UINT32      time;

    USM_AUTH_PROT_STRUCT    *auth_prot = NU_NULL;
    USM_PRIV_PROT_STRUCT    *priv_prot = NU_NULL;
    USM_USERS_STRUCT        *user;

    /* USM Stats OID. */
    UINT32     usm_stats_oid[] = {1, 3, 6, 1, 6, 3, 15, 1, 1};
    UINT8      oid_len = 9;

    static UINT32           authentication_trap_oid[] = SNMP_AUTH_FAILURE_TRAP;    
    SNMP_NOTIFY_REQ_STRUCT  *snmp_notification;

    /* Unused Parameters. */
    UNUSED_PARAMETER(snmp_mp);
    UNUSED_PARAMETER(snmp_sm);

    /* Decode the security parameters. */

    /* Open ASN connection for decoding. */
    Asn1Opn(&Asn1, security_param,
        (UINT32)(*msg_len - (security_param - *whole_message)), ASN1_DEC);

    /* Decode the Octet String which encapsulates the security_param. */
    if (!Asn1HdrDec (&Asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OTS))
        status = SNMP_ERROR;

    else
    {
        /* Calculate the max_response_pdu (step 1). This is the maximum
         * message size, minus the length of the global data and the
         * security parameters. Here the length of the global data is
         * being subtracted.
         */
        (*max_response_pdu) =
                max_msg_size - (UINT32)(security_param - *whole_message);

        /* Decode the Sequence Header. */

        if (!Asn1HdrDec (&Asn1, &Eoc, &Cls, &Con, &Tag))
            status = SNMP_ERROR;

        else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) ||
                 (Con != ASN1_CON) || (Tag != ASN1_SEQ))
            status = SNMP_ERROR;

        /* Decode the msgAuthoritativeEngineID. */
        else if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
            status = SNMP_ERROR;

        else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                 (Con != ASN1_PRI) || (Tag != ASN1_OTS))
            status = SNMP_ERROR;


        if (!Asn1OtsDec (&Asn1, End, &temp_engine_id[0],
                         SNMP_SIZE_BUFCHR,
                         (UINT32 *)security_engine_id_len))
            status = SNMP_ERROR;

        /* If a security engine id received if greater than 32 bytes, then
         * decode take the first 32 bytes as engine id. */
        if (*security_engine_id_len > SNMP_SIZE_SMALLOBJECTID)
        {
            NU_BLOCK_COPY(security_engine_id, &temp_engine_id, 
                SNMP_SIZE_SMALLOBJECTID);

            *security_engine_id_len = SNMP_SIZE_SMALLOBJECTID;
        }
        else
        {
            NU_BLOCK_COPY(security_engine_id, &temp_engine_id, 
                *security_engine_id_len);
        }

        /* Decode the msgAuthoritativeEngineBoots. */
        if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
            status = SNMP_ERROR;

        else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                 (Con != ASN1_PRI) || (Tag != ASN1_INT))
            status = SNMP_ERROR;

        else if (!Asn1IntDec (&Asn1, End, &temp_no))
            status = SNMP_ERROR;

        else if (temp_no < 0)
        {
            SnmpStat.InASNParseErrs++;
            return (ASN1_ERR_DEC_BADVALUE);
        }

        else 
        {
            engine_boots = (UINT32) temp_no;
        }

        /* Decode the msgAuthoritativeEngineTime. */
        if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
            status = SNMP_ERROR;

        else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                 (Con != ASN1_PRI) || (Tag != ASN1_INT))
            status = SNMP_ERROR;

        else if (!Asn1IntDecUns (&Asn1, End, &engine_time))
            status = SNMP_ERROR;

        /* Decode the usm_msg_user_name. */
        else if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
            status = SNMP_ERROR;

        else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                 (Con != ASN1_PRI) || (Tag != ASN1_OTS))
            status = SNMP_ERROR;

        else if (!Asn1OtsDec (&Asn1, End, &temp_security_name[0],
                              SNMP_SIZE_BUFCHR, &temp))
            status = SNMP_ERROR;

        else
        {
            if (temp > SNMP_SIZE_SMALLOBJECTID)
            {
                status = SNMP_ERROR;
            }
            else
            {
                NU_BLOCK_COPY(security_name, &temp_security_name, temp);
                security_name[temp] = '\0';
            }

            /* Decode the authParameter. */

            /* Set auth_param to the start of msgAuthenticationParameters
             * field (pointing to after the header.
             */
            auth_param = &Asn1.Pointer[2];

            if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
                status = SNMP_ERROR;

            else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                     (Con != ASN1_PRI) || (Tag != ASN1_OTS))
                status = SNMP_ERROR;

            else if (!Asn1OtsDec (&Asn1, End, priv_param,
                                  SNMP_SIZE_BUFCHR - 1,
                                  (UINT32 *)&auth_param_len))
                status = SNMP_ERROR;

            /* Decode the Privacy Parameters. */
            else if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
                status = SNMP_ERROR;

            else if ((End == NU_NULL) || (Cls != ASN1_UNI) ||
                     (Con != ASN1_PRI) || (Tag != ASN1_OTS))
                status = SNMP_ERROR;

            else if (!Asn1OtsDec (&Asn1, End, priv_param,
                                  SNMP_SIZE_BUFCHR - 1,
                                  (UINT32 *)&priv_param_len))
                status = SNMP_ERROR;
        }
    }

    /* Check whether the decoding was successful. */
    if(status != NU_SUCCESS)
    {
        SnmpStat.InASNParseErrs++;
        return (USM_PARSE_ERROR);
    }

    /* The parsing has been successful. Prepare the cache. */
    for(i = 0; i < USM_MAX_CACHED_DATA; i++)
    {
        if(Usm_Cached_Security_Data[i].usm_is_occupied == NU_FALSE)
        {
            /* We have found an empty location. */

            /* Clear out the Cache. */
            UTL_Zero(&Usm_Cached_Security_Data[i],
                     sizeof(USM_CACHED_SECURITY_STRUCT));

            /* Set up the user information. */
            Usm_Cached_Security_Data[i].usm_is_occupied = NU_TRUE;
            strcpy((CHAR *)Usm_Cached_Security_Data[i].usm_msg_user_name,
                   (CHAR *)security_name);

            security_state_ref[0] = (&Usm_Cached_Security_Data[i]);

            /* Break out of the loop. */
            break;
        }
    }

    /* Did we find an empty location? */
    if(i == USM_MAX_CACHED_DATA)
    {
        /* No we did not! */
        return (SNMP_ERROR);
    }

    /* Does the security_engine_id match? */
    else if(*security_engine_id_len != Snmp_Engine.snmp_engine_id_len ||
        (memcmp(security_engine_id, Snmp_Engine.snmp_engine_id,
                (unsigned int)Snmp_Engine.snmp_engine_id_len) !=0))
    {
        /* Increment the usm_unknown_engine_id counter. */
        Usm_Mib.usm_stats_tab.usm_unknown_engine_id++;
        *security_level = SNMP_SECURITY_NOAUTHNOPRIV;

        /* Fill up the status Information. */
        NU_BLOCK_COPY(error_indication->snmp_oid, usm_stats_oid,
                      sizeof(UINT32) * oid_len);
        error_indication->snmp_oid[oid_len] = 4;
        error_indication->snmp_oid[oid_len + 1] = 0;
        error_indication->snmp_oid_len = (UINT32)(oid_len + 2);

        error_indication->snmp_value =
                            Usm_Mib.usm_stats_tab.usm_unknown_engine_id;

        Usm_Cached_Security_Data[i].usm_is_occupied = NU_FALSE;

        return (USM_UNKNOWNENGINEID_ERROR);
    }

    /* Find the user. */
    user = USM_Lookup_Users(security_name);

    /* If the user does not exist, or the security_engine_id does not
     * match, then we have an error condition.
     */
    if((user == NU_NULL) ||
       (user->usm_user_engine_id_len != (UINT8) (*security_engine_id_len))
       || (memcmp(security_engine_id, user->usm_user_engine_id,
                  (unsigned int)*security_engine_id_len) != 0))
    {
        Usm_Mib.usm_stats_tab.usm_unkown_user_name++;
        *security_level = SNMP_SECURITY_NOAUTHNOPRIV;

        /* Fill up the status Information. */
        NU_BLOCK_COPY(error_indication->snmp_oid, usm_stats_oid,
                      sizeof(UINT32) * oid_len);
        error_indication->snmp_oid[oid_len] = 3;
        error_indication->snmp_oid[oid_len + 1] = 0;
        error_indication->snmp_oid_len = (UINT32)(oid_len + 2);

        error_indication->snmp_value =
                            Usm_Mib.usm_stats_tab.usm_unkown_user_name;

        Usm_Cached_Security_Data[i].usm_is_occupied = NU_FALSE;

        return (USM_UNKNOWNUSERNAME_ERROR);
    }

    /* Does the user support the required security level? If the user
     * does, then do the required processing.
     */
    switch (*security_level)
    {
    case SNMP_SECURITY_AUTHPRIV:

        /* Get the Privacy Protocol. */
        priv_prot = USM_Lookup_Priv_Prot(user->usm_priv_index);

        /* Check that a privacy protocol is supported. */
        if((!priv_prot) || (!(priv_prot->usm_encrypt_cb)))
        {
            status = USM_UNSUPPORTED_SECURITY_LEVEL;
            break;
        }

    case SNMP_SECURITY_AUTHNOPRIV:

        /* Get the Authentication Protocol. */
        auth_prot = USM_Lookup_Auth_Prot(user->usm_auth_index);

        /* Check that an authentication protocol is supported. */
        if((!auth_prot) || (!(auth_prot->usm_secure_cb)))
        {
            status = USM_UNSUPPORTED_SECURITY_LEVEL;
            break;
        }

        /* Make a call to the Authentication Protocol. */
        else if(auth_prot->usm_verify_cb(user->usm_auth_key,
                        auth_prot->key_length, auth_param, auth_param_len,
                        *whole_message, *msg_len) != NU_SUCCESS)
        {
            status = USM_AUTHENTICATION_ERROR;
            break;
        }

        /* Retrieve the SNMP Engine Time. */
        else if(SNMP_Engine_Time(&time) != NU_SUCCESS)
        {
            status = SNMP_ERROR;
            break;
        }

        /* Is the message outside the Time Window? */
        else if((Snmp_Engine.snmp_engine_boots == 0xFFFFFFFF) ||
                (Snmp_Engine.snmp_engine_boots != engine_boots) ||
                ((INT32)(time - engine_time) > 150) ||
                ((INT32)(time - engine_time) < -150))
        {
            status = USM_NOTINTIMEWINDOW_ERROR;
            break;
        }

        /* Authentication has been successful, the whole_message should
         * point to the scoped PDU now.
         */
        (*whole_message) = Asn1.Pointer;
        (*msg_len) = (UINT32)(Asn1.End - Asn1.Pointer);

        if(*security_level == SNMP_SECURITY_AUTHPRIV)
        {
            /* If privacy is required, do the decryption. */

            /* Make a call to the Privacy Protocol. */
            status = priv_prot->usm_decrypt_cb(user->usm_priv_key,
               USM_DES_PRIV_KEY_SIZE, priv_param, priv_param_len,
                whole_message, msg_len);

            if(status != NU_SUCCESS)
            {
                if (status == ASN1_ERR_DEC_EOC_MISMATCH) 
                {
                    status = SNMP_ERROR;
                }

                else if (status != ASN1_ERR_DEC_BADVALUE)
                {
                    status = USM_DECRYPTION_ERROR;
                }

                break;
            }
        }

    case SNMP_SECURITY_NOAUTHNOPRIV:

        /* If neither decryption, nor authentication was required. */
        if(*security_level == SNMP_SECURITY_NOAUTHNOPRIV)
        {
            /* Point to the scopedPDU. */
            *whole_message = Asn1.Pointer;
            *msg_len = (UINT32)(Asn1.End - Asn1.Pointer);
        }

        break;

    default:
        status = USM_UNSUPPORTED_SECURITY_LEVEL;
        break;
    }

    /* Calculate the max_response_pdu (step 2). This is the maximum
     * message size, minus the length of the global data and the security
     * parameters. Here the length of the security parameters is being
     * subtracted.
     */
    (*max_response_pdu) -= (20 + Snmp_Engine.snmp_engine_id_len +
                            strlen((CHAR *)security_name));

    /* If authentication is required, then subtract the authentication
     * parameters length as well.
     */
    if(auth_prot != NU_NULL)
        (*max_response_pdu) -= auth_prot->usm_param_length;

    /* If privacy is required, then subtract the privacy parameters length
     * as well.
     */
    if(priv_prot != NU_NULL)
        *max_response_pdu -= priv_prot->usm_param_length;

    /* Cache the security data. */
    ((USM_CACHED_SECURITY_STRUCT *)security_state_ref[0])->
            usm_auth_protocol = auth_prot;

    if(auth_prot != NU_NULL)
        NU_BLOCK_COPY(
            ((USM_CACHED_SECURITY_STRUCT *)security_state_ref[0])->
                usm_auth_key, user->usm_auth_key, auth_prot->key_length);

    ((USM_CACHED_SECURITY_STRUCT *)security_state_ref[0])->
                usm_priv_protocol = priv_prot;

    if((priv_prot) && (auth_prot))
        NU_BLOCK_COPY(
            ((USM_CACHED_SECURITY_STRUCT *)security_state_ref[0])->
            usm_priv_key, user->usm_priv_key, auth_prot->key_length);

    /* If the verification for the message failed... */
    if(status != NU_SUCCESS)
    {
        if (status != ASN1_ERR_DEC_BADVALUE)
        {
            /* Fill up the status Information. */
            NU_BLOCK_COPY(error_indication->snmp_oid, usm_stats_oid,
                sizeof(UINT32) * oid_len);

            error_indication->snmp_oid_len = (UINT32)(oid_len + 2);
            (*security_level) = SNMP_SECURITY_NOAUTHNOPRIV;
        }

        Usm_Cached_Security_Data[i].usm_is_occupied = NU_FALSE;

        /* If the given security_level is not supported. */
        if(status ==  USM_UNSUPPORTED_SECURITY_LEVEL)
        {
            Usm_Mib.usm_stats_tab.usm_unsupported_sec_level++;
#if (INCLUDE_MIB_MPD == NU_TRUE)
            Snmp_Mpd_Mib.snmp_invalid_msg++;
#endif
            error_indication->snmp_value =
                    Usm_Mib.usm_stats_tab.usm_unsupported_sec_level;
            error_indication->snmp_oid[oid_len] = 1;
            error_indication->snmp_oid[oid_len + 1] = 0;
        }

        /* If the message is not in the time window. */
        else if(status == USM_NOTINTIMEWINDOW_ERROR)
        {
            Usm_Mib.usm_stats_tab.usm_not_in_time_win++;
            error_indication->snmp_value =
                        Usm_Mib.usm_stats_tab.usm_not_in_time_win;
            error_indication->snmp_oid[oid_len] = 2;
            error_indication->snmp_oid[oid_len + 1] = 0;
            *security_level = SNMP_SECURITY_AUTHNOPRIV;
        }

        /* If the authentication did not succeed. */
        else if(status == USM_AUTHENTICATION_ERROR)
        {
            Usm_Mib.usm_stats_tab.usm_wrong_digests++;
            error_indication->snmp_value =
                    Usm_Mib.usm_stats_tab.usm_wrong_digests;
            error_indication->snmp_oid[oid_len] = 5;
            error_indication->snmp_oid[oid_len + 1] = 0;

            /* Send the authentication failure trap. */
            if (snmp_cfg.authentrap_enable == NU_TRUE )
            {
                /* Get an empty location to place the notification. */
                if(SNMP_Get_Notification_Ptr(&snmp_notification) ==
                                                               NU_SUCCESS)
                {           
                    /* Clear the structure. */
                    UTL_Zero(snmp_notification,
                             sizeof(SNMP_NOTIFY_REQ_STRUCT));

                    /* Copy the Authentication Failure OID. */
                    memcpy(snmp_notification->OID.notification_oid,
                            authentication_trap_oid,
                            sizeof(UINT32) * SNMP_TRAP_OID_LEN);

                    snmp_notification->OID.oid_len = SNMP_TRAP_OID_LEN;

                    /* The notification is ready to be sent. */
                    SNMP_Notification_Ready(snmp_notification);
                }
            }
        }

        /* If the decryption did not succeed. */
        else if(status == USM_DECRYPTION_ERROR)
        {
            Usm_Mib.usm_stats_tab.usm_decryption_err++;
            error_indication->snmp_value =
                    Usm_Mib.usm_stats_tab.usm_decryption_err;
            error_indication->snmp_oid[oid_len] = 6;
            error_indication->snmp_oid[oid_len + 1] = 0;
        }

        return (status);
    }

    /* The message passed security. */

    /* The operation is successful. */
    return (NU_SUCCESS);

} /* USM_Verify */

/************************************************************************
*
*   FUNCTION
*
*       USM_Lookup_Users
*
*   DESCRIPTION
*
*       This function gets a user entry from the usm_user_table based on
*       the usm_security_name.
*
*   INPUTS
*
*       msg_user_name   The usm_security_name for the user entry that is
*                       required.
*
*   OUTPUTS
*
*       Pointer to the user entry if the operation is successful.
*       NU_NULL if the user entry could not be retrieved.
*
*************************************************************************/
USM_USERS_STRUCT* USM_Lookup_Users(UINT8 *msg_user_name)
{
    USM_USERS_STRUCT        *user;

    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (msg_user_name != NU_NULL)
    {
        /* Traverse through the list until we find the msg_user_name, or
         * we reach the end of the list.
         */
        user = Usm_Mib.usm_user_table.next;

        while(user)
        {
            if((strcmp((CHAR *)user->usm_security_name,
                                (CHAR *)msg_user_name)) == 0)
            {
                /* User Name matched, this is the entry we are looking
                 * for.
                 */
                break;
            }

            user = user->next;
        }
    }

    else
    {
        user = NU_NULL;
    }
    
    NU_USER_MODE();

    /* Return handle to the user if found. Otherwise, return NU_NULL. */
    return (user);

} /* USM_Lookup_Users */

/************************************************************************
*
*   FUNCTION
*
*       USM_Lookup_Priv_Prot
*
*   DESCRIPTION
*
*       This function gets the Privacy Protocol corresponding to the given
*       usm_index.
*
*   INPUTS
*
*       usm_index       The usm_index of the Privacy Protocol.
*
*   OUTPUTS
*
*       Pointer to the protocol if the operation is successful.
*       NU_NULL if the protocol could not be retrieved.
*
*************************************************************************/
USM_PRIV_PROT_STRUCT* USM_Lookup_Priv_Prot(UINT32 usm_index)
{
    UINT8                   i;
    USM_PRIV_PROT_STRUCT*   priv = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Look through the array to find the Privacy Protocol required. */
    for(i = 0; i < USM_MAX_PRIV_PROTOCOLS; i++)
    {
        /* If we found the required handle to required Privacy Protocol.
         */
        if(Usm_Priv_Prot_Table[i].usm_index == usm_index)
        {
            /* Return handle to the privacy protocol. */
            priv = &Usm_Priv_Prot_Table[i];

            /* Break through the loop. */
            break;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return handle to the privacy protocol if we found one. Otherwise,
     * return NU_NULL.
     */
    return (priv);

} /* USM_Lookup_Priv_Prot */

/************************************************************************
*
*   FUNCTION
*
*       USM_Lookup_Auth_Prot
*
*   DESCRIPTION
*
*       This function retrieves the Authentication Protocol corresponding
*       to the usm_index passed.
*
*   INPUTS
*
*       usm_index       The usm_index of the Authentication Protocol.
*
*   OUTPUTS
*
*       Pointer to the protocol if the operation is successful.
*       NU_NULL if the protocol could not be retrieved.
*
*************************************************************************/
USM_AUTH_PROT_STRUCT* USM_Lookup_Auth_Prot(UINT32 usm_index)
{
    UINT8                   i;
    USM_AUTH_PROT_STRUCT    *auth = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Look through the array to find the Authentication Protocol
     * required.
     */
    for(i = 0; i < USM_MAX_AUTH_PROTOCOLS; i++)
    {
        /* If found the required Authentication protocol. */
        if(Usm_Auth_Prot_Table[i].usm_index == usm_index)
        {
            /* Get the handle to the Authentication Protocol to be
             * returned.
             */
            auth = &(Usm_Auth_Prot_Table[i]);

            /* Break through the loop. */
            break;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the handle to the Authentication Protocol, if we found one.
       Otherwise, return NU_NULL. */
    return (auth);

} /* USM_Lookup_Auth_Prot */

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       USM_Compare_Index
*
*   DESCRIPTION
*
*       This function is used to compare the indices of the USM
*       table entries. This function is being used by the file
*       component.
*
*   INPUTS
*
*       *left_side                      Pointer to a USM entry.
*       *right_side                     Pointer to a USM entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 USM_Compare_Index(VOID *left_side, VOID *right_side)
{
    USM_USERS_STRUCT        *left_ptr = left_side;
    USM_USERS_STRUCT        *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the security engine ID and the user name. This forms the
     * index for the USM table.
     */
    if((left_ptr->usm_user_engine_id_len ==
        right_ptr->usm_user_engine_id_len) &&
       (memcmp(left_ptr->usm_user_engine_id,
               right_ptr->usm_user_engine_id,
               left_ptr->usm_user_engine_id_len) == 0) &&
       (strcmp((CHAR *)left_ptr->usm_user_name,
               (CHAR *)right_ptr->usm_user_name) == 0))
    {
        cmp = 0;
    }

    return (cmp);

} /* USM_Compare_Index */

/************************************************************************
*
*   FUNCTION
*
*       USM_Save_User
*
*   DESCRIPTION
*
*       This function saves the new state of user to the file.
*
*   INPUTS
*
*       *user               The user whose changed state is to be saved.
*
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation successful.
*
*************************************************************************/
STATUS USM_Save_User (USM_USERS_STRUCT *user)
{
    /* This variable will be used by the file component to temporarily
     * store information read from file.
     */
    USM_USERS_STRUCT        read_usm;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Usm_Mib.usm_user_table.next,
        user,
        &read_usm,
        SNMP_MEMBER_OFFSET(USM_USERS_STRUCT, usm_storage_type),
        SNMP_MEMBER_OFFSET(USM_USERS_STRUCT, usm_status),
        sizeof(USM_USERS_STRUCT),
        USM_FILE,
        USM_Compare_Index,
        INCLUDE_MIB_USM);

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* USM_Save_User */

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

/************************************************************************
*
*   FUNCTION
*
*       USM_Add_User_Util
*
*   DESCRIPTION
*
*       This function adds a new user to the USM User Table.
*
*   INPUTS
*
*       *node                   Information about the new user to be added
*                               to the USM User Table.
*       *root                   Pointer to the root of USM user table.
*
*   OUTPUTS
*
*       NU_SUCCESS              When successful.
*       SNMP_WARNING            Already exists.
*       SNMP_NO_MEMORY          Memory allocation failed.
*       SNMP_INVALID_PARAMETER  Invalid arguments.
*
*************************************************************************/
STATUS USM_Add_User_Util(USM_USERS_STRUCT * node,
                         USM_USERS_TABLE_ROOT *root)
{
    USM_USERS_STRUCT            *temp = NU_NULL;
    USM_USERS_STRUCT            *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    /* If we have valid argument. */
    if (node)
    {
        /* Get a pointer to the root of the list. */
        temp = root->next;

        /* Find the location where the node is to be inserted. */
        while (temp)
        {
            /* Comparing engine ID Lengths. */
            cmp = temp->usm_user_engine_id_len -
                                node->usm_user_engine_id_len;

            /* If engine ID lengths are equal then compare engine ID
             * value.
             */
            if (cmp == 0)
            {
                /* Comparing engine ID. */
                cmp = memcmp(temp->usm_user_engine_id,
                             node->usm_user_engine_id,
                             temp->usm_user_engine_id_len);

                /* If engine ID length and engine ID of current entry and
                 * node passed in are equal then compare user name.
                 */
                if (cmp == 0)
                {
                    /* Comparing user name. */
                    cmp = UTL_Admin_String_Cmp(
                                        (CHAR *)(temp->usm_user_name),
                                        (CHAR *)(node->usm_user_name));

                }
            }

            /* If current entry has the same index as that of node passed
             * in then set the error code.
             */
            if (cmp == 0)
            {
                /* Setting error code. */
                status = SNMP_WARNING;
            }

            /* If we have reached at proper location then break through
             * the loop.
             */
            if (cmp >= 0)
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            temp = temp->next;
        }
    }

    /* If we did not get the valid argument then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_INVALID_PARAMETER;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if ((status = NU_Allocate_Memory(&System_Memory,
                            (VOID**)(&new_node), sizeof(USM_USERS_STRUCT),
                            NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(USM_USERS_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at the end
             * of the list.
             */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(root, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(root, new_node, temp);
            }
        }

        /* If memory allocation failed. */
        else
        {
            NLOG_Error_Log("SNMP: Failed to allocate memory.",
                            NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    return (status);

} /* USM_Add_User_Util */

/************************************************************************
*
*   FUNCTION
*
*       USM_Add_User
*
*   DESCRIPTION
*
*       This function adds a new user to the usm_user_table.
*
*   INPUTS
*
*       *node                   Information about the new user to be added
*                               to the usm_user_table.
*
*   OUTPUTS
*
*       NU_SUCCESS              When successful.
*       SNMP_WARNING            Already exists.
*       SNMP_NO_MEMORY          Memory allocation failed.
*       SNMP_BAD_PARAMETER      Invalid arguments.
*
*************************************************************************/
STATUS USM_Add_User(USM_USERS_STRUCT *node)
{
    STATUS                      status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */
    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_USM == NU_TRUE)
         (node->usm_status >= SNMP_ROW_ACTIVE) &&
         (node->usm_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->usm_status != SNMP_ROW_ACTIVE) &&
             (node->usm_status != SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->usm_storage_type >= SNMP_STORAGE_OTHER) &&
             (node->usm_storage_type <= SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           ((strlen((CHAR *)(node->usm_user_name)) != 0)) ) ) )
    {
        status = USM_Add_User_Util(node, &(Usm_Mib.usm_user_table));
    }
    
    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();

    /* Return success or error code. */
    return (status);

} /* USM_Add_User */

/************************************************************************
*
*   FUNCTION
*
*       USM_Key_Change
*
*   DESCRIPTION
*
*       This function changes the users key.
*
*   INPUTS
*
*       usm_auth_index          The authentication protocol for the user.
*       *current_key            User's current key.
*       *new_key                User's new key.
*       new_key_len             Length of the user's key.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Key_Change(UINT32 usm_auth_index, UINT8 *current_key,
                      UINT8 *new_key, UINT32 new_key_len)
{
    UINT8           i;
    STATUS          status = SNMP_ERROR;

    /* Loop through the Authentication Protocols. */
    for(i = 0; i < USM_MAX_AUTH_PROTOCOLS; i++)
    {
        /* If we reached at the required Authentication Protocol. */
        if (Usm_Auth_Prot_Table[i].usm_index == usm_auth_index)
        {
            if(Usm_Auth_Prot_Table[i].usm_key_change_cb != NU_NULL)
            {
                status = Usm_Auth_Prot_Table[i].usm_key_change_cb(
                                    current_key, new_key, new_key_len);
            }
            else
            {
                status = NU_SUCCESS;
            }

            /* Break through the loop to return success or error code. */
            break;
        }
    }

    /* Return success or error code. */
    return (status);

} /* USM_Key_Change */


#endif



