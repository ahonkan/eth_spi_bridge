/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2005-2006
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       wireless.h
*
*   COMPONENT
*
*       WL - Wireless Header File
*
*   DESCRIPTION
*
*       This header file containts constants and data structures
*       related to WLAN.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/

/* NOTE: Due to licensing problems, this file has been reverse-engineered
 * and has been written from scratch. Therefore, it uses a proprietary
 * license.
 */

#ifndef NUCLEUS_WIRELESS_H
#define NUCLEUS_WIRELESS_H

#include "nucleus.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Nucleus-specific: Standard GUID for the Wi-Fi class. */
#define WIFI_LABEL {0xe8,0xcd,0x19,0x0d,0x39,0x68,0x42,0x87,0xa4,0x26,0x6d,0xf6,0x14,0x40,0xe8,0xcb}

/* Define the maximum size of the "interface name length". */
#ifndef IFNAMSIZ
#define IFNAMSIZ                        DEV_NAME_LENGTH
#endif

/* Version compatibility with the "Linux Wireless Extensions". */

#ifdef CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE 
#define WIRELESS_EXT                    22   /* for AR6K3 */
#else
#define WIRELESS_EXT                    19   /* for AR6K2 */
#endif


/******************* NUCLEUS-SPECIFIC IOCTL CONSTANTS *******************/

/* Nucleus-Specific: Wireless IOCTL constants. */

#define WIRELESS_CMD_COMMIT             0
#define WIRELESS_CMD_SET_NAME           1           /* Unused. */
#define WIRELESS_CMD_GET_NAME           2
#define WIRELESS_CMD_SET_NWID           3           /* Unused. */
#define WIRELESS_CMD_GET_NWID           4           /* Unused. */
#define WIRELESS_CMD_SET_FREQ           5
#define WIRELESS_CMD_GET_FREQ           6
#define WIRELESS_CMD_SET_MODE           7
#define WIRELESS_CMD_GET_MODE           8
#define WIRELESS_CMD_SET_SENS           9
#define WIRELESS_CMD_GET_SENS           10
#define WIRELESS_CMD_SET_RANGE          11          /* Unused. */
#define WIRELESS_CMD_GET_RANGE          12
#define WIRELESS_CMD_SET_PRIV           13          /* Unused. */
#define WIRELESS_CMD_GET_PRIV           14
#define WIRELESS_CMD_SET_SPY            15
#define WIRELESS_CMD_GET_SPY            16
#define WIRELESS_CMD_SET_WAP            17
#define WIRELESS_CMD_GET_WAP            18
#define WIRELESS_CMD_SET_ESSID          19
#define WIRELESS_CMD_GET_ESSID          20
#define WIRELESS_CMD_SET_NICKN          21
#define WIRELESS_CMD_GET_NICKN          22
#define WIRELESS_CMD_SET_RATE           23
#define WIRELESS_CMD_GET_RATE           24
#define WIRELESS_CMD_SET_WRTS           25
#define WIRELESS_CMD_GET_WRTS           26
#define WIRELESS_CMD_SET_FRAG           27
#define WIRELESS_CMD_GET_FRAG           28
#define WIRELESS_CMD_SET_TXPOW          29
#define WIRELESS_CMD_GET_TXPOW          30
#define WIRELESS_CMD_SET_RETRY          31
#define WIRELESS_CMD_GET_RETRY          32
#define WIRELESS_CMD_SET_ENCODE         33
#define WIRELESS_CMD_GET_ENCODE         34
#define WIRELESS_CMD_SET_POWER          35
#define WIRELESS_CMD_GET_POWER          36
#define WIRELESS_CMD_SET_STATS          37          /* Unused. */
#define WIRELESS_CMD_GET_STATS          38          /* Unused. */
#define WIRELESS_CMD_SET_SCAN           39
#define WIRELESS_CMD_GET_SCAN           40
#define WIRELESS_CMD_SET_GENIE          41
#define WIRELESS_CMD_GET_GENIE          42
#define WIRELESS_CMD_SET_MLME           43
#define WIRELESS_CMD_SET_AUTH           44
#define WIRELESS_CMD_GET_AUTH           45
#define WIRELESS_CMD_SET_ENCODEEXT      46
#define WIRELESS_CMD_GET_ENCODEEXT      47
#define WIRELESS_CMD_SET_PMKSA          48
#define WIRELESS_CMD_SET_THRSPY         49
#define WIRELESS_CMD_GET_THRSPY         50

#ifdef CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE 
#define WIRELESS_CMD_IEEE80211_SET_PARAM                  61
#define WIRELESS_CMD_IEEE80211_SET_KEY                    62
#define WIRELESS_CMD_IEEE80211_DEL_KEY                    63
#define WIRELESS_CMD_IEEE80211_SET_MLME                   64
#define WIRELESS_CMD_IEEE80211_ADD_PMKID                  65
#define WIRELESS_CMD_IEEE80211_SET_OPTIE                  66
#define WIRELESS_CMD_IEEE80211_WMI_GETREV                 72
#define WIRELESS_CMD_IEEE80211_WMI_SETPWR                 73
#define WIRELESS_CMD_IEEE80211_WMI_SETSCAN                74
#define WIRELESS_CMD_IEEE80211_WMI_SETLISTENINT           75
#define WIRELESS_CMD_IEEE80211_WMI_SETBSSFILTER           76
#define WIRELESS_CMD_IEEE80211_WMI_SET_CHANNELPARAMS      77
#define WIRELESS_CMD_IEEE80211_WMI_SET_PROBEDSSID         78
#define WIRELESS_CMD_IEEE80211_WMI_SET_PMPARAMS           79
#define WIRELESS_CMD_IEEE80211_WMI_SET_BADAP              80
#define WIRELESS_CMD_IEEE80211_WMI_GET_QOS_QUEUE          81
#define WIRELESS_CMD_IEEE80211_WMI_CREATE_QOS             82
#define WIRELESS_CMD_IEEE80211_WMI_DELETE_QOS             83
#define WIRELESS_CMD_IEEE80211_WMI_SET_SNRTHRESHOLD       84
#define WIRELESS_CMD_IEEE80211_WMI_SET_ERR_RRT_BITMSK     85
#define WIRELESS_CMD_IEEE80211_WMI_GET_TARGET_STATS       86
#define WIRELESS_CMD_IEEE80211_WMI_SET_ASSOC_INFO         87
#define WIRELESS_CMD_IEEE80211_WMI_SET_ACCESS_PARAMS      88
#define WIRELESS_CMD_IEEE80211_WMI_SET_BMISS_TIME         89
#define WIRELESS_CMD_IEEE80211_WMI_SET_DISC_TIMEOUT       90
#define WIRELESS_CMD_IEEE80211_WMI_SET_IBSS_PM_CAPS       91
#define WIRELESS_CMD_IEEE80211_WMI_EXTENDED               92
#endif /* CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE */

#ifdef CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE 
#define TOTAL_WIFI_IOCTLS               93                /* Actual count is <100 but the last IOCTL is 92, hence it is set to 93 */
#else
#define TOTAL_WIFI_IOCTLS               51
#endif

/*************************** VARIOUS CONSTANTS **************************/

/* Maximum bit rates in the range struct */
#define IW_MAX_BITRATES		            32

/* Bit-masks for the "auth" flags in the "iw_params" structure. */
#define IW_AUTH_INDEX                   0x0fff
#define IW_AUTH_FLAGS                   0xf000

/* Indices of "auth" parameters used with the IW_AUTH_INDEX mask. */
#define IW_AUTH_WPA_VERSION             0
#define IW_AUTH_CIPHER_PAIRWISE         1
#define IW_AUTH_CIPHER_GROUP            2
#define IW_AUTH_KEY_MGMT                3
#define IW_AUTH_TKIP_COUNTERMEASURES    4
#define IW_AUTH_DROP_UNENCRYPTED        5
#define IW_AUTH_80211_AUTH_ALG          6
#define IW_AUTH_WPA_ENABLED             7
#define IW_AUTH_RX_UNENCRYPTED_EAPOL    8
#define IW_AUTH_ROAMING_CONTROL         9
#define IW_AUTH_PRIVACY_INVOKED         10

/* Possible values for the "IW_AUTH_WPA_VERSION" parameter. */
#define IW_AUTH_WPA_VERSION_DISABLED    0x0001
#define IW_AUTH_WPA_VERSION_WPA         0x0002
#define IW_AUTH_WPA_VERSION_WPA2        0x0004

/* Possible values for the "IW_AUTH_CIPHER_PAIRWISE" and
 * "IW_AUTH_CIPHER_GROUP" parameters. */
#define IW_AUTH_CIPHER_NONE             0x0001
#define IW_AUTH_CIPHER_WEP40            0x0002
#define IW_AUTH_CIPHER_TKIP             0x0004
#define IW_AUTH_CIPHER_CCMP             0x0008
#define IW_AUTH_CIPHER_WEP104           0x0010

/* Possible values for the "IW_AUTH_KEY_MGMT" parameter. */
#define IW_AUTH_KEY_MGMT_802_1X         0x0001
#define IW_AUTH_KEY_MGMT_PSK            0x0002

/* Possible values for the "IW_AUTH_80211_AUTH_ALG" parameter. */
#define IW_AUTH_ALG_OPEN_SYSTEM         0x0001
#define IW_AUTH_ALG_SHARED_KEY          0x0002
#define IW_AUTH_ALG_LEAP                0x0004

/* Encoding algorithm identifiers. */
#define IW_ENCODE_ALG_NONE              1
#define IW_ENCODE_ALG_WEP               2
#define IW_ENCODE_ALG_TKIP              3
#define IW_ENCODE_ALG_CCMP              4
#define IW_ENCODE_ALG_PMK               5
#define IW_ENCODE_ALG_SM4               6
#define IW_ENCODE_ALG_AES_CMAC          7

/* Encoding flags (extended). */
#define IW_ENCODE_EXT_GROUP_KEY         0x0001
#define IW_ENCODE_EXT_SET_TX_KEY        0x0002
#define IW_ENCODE_EXT_SET_RX_KEY        0x0004
#define IW_ENCODE_EXT_RX_SEQ_VALID      0x0008

/* Flags for encoding capabilities. */
#define IW_ENC_CAPA_WPA                 0x0001
#define IW_ENC_CAPA_WPA2                0x0002
#define IW_ENC_CAPA_4WAY_HANDSHAKE      0x0004
#define IW_ENC_CAPA_CIPHER_CCMP         0x0008
#define IW_ENC_CAPA_CIPHER_TKIP         0x0010

/* Flags for scan capability */
#define IW_SCAN_CAPA_NONE               0x00
#define IW_SCAN_CAPA_ESSID              0x01
#define IW_SCAN_CAPA_BSSID              0x02
#define IW_SCAN_CAPA_CHANNEL            0x04
#define IW_SCAN_CAPA_MODE               0x08
#define IW_SCAN_CAPA_RATE               0x10
#define IW_SCAN_CAPA_TYPE               0x20
#define IW_SCAN_CAPA_TIME               0x40

/* Macro for event capability */
#define IW_EVENT_CAPA_BASE(cmd)         ((cmd >= SIOCIWFIRSTPRIV) ?       \
                                         (cmd - SIOCIWFIRSTPRIV + 0x60) : \
                                         (cmd - SIOCIWFIRST))

#define IW_EVENT_CAPA_MASK(cmd)         (1 << (IW_EVENT_CAPA_BASE(cmd) & 0x1F))

 /* This list is valid for most 802.11 devices, customise as needed... */
#define IW_EVENT_CAPA_K_0               (IW_EVENT_CAPA_MASK(SIOCSIWFREQ) | \
                                         IW_EVENT_CAPA_MASK(SIOCSIWMODE) | \
                                         IW_EVENT_CAPA_MASK(SIOCSIWESSID))
                 
#define IW_EVENT_CAPA_K_1               (IW_EVENT_CAPA_MASK(SIOCSIWENCODE))              

/* Size of the TX/RX sequences, used if the IW_ENCODE_EXT_SET_TX_KEY
 * or the IW_ENCODE_EXT_SET_RX_KEY flags are set. */
#define IW_ENCODE_SEQ_MAX_SIZE          8

/* Bit-masks for different encoding indices and flags. */
#define IW_ENCODE_INDEX                 0x00ff
#define IW_ENCODE_FLAGS                 0xff00      /* Unused. */
#define IW_ENCODE_MODE                  0xf000      /* Unused. */

/* Encoding flags. */
#define IW_ENCODE_ENABLED               0x0000
#define IW_ENCODE_DISABLED              0x8000
#define IW_ENCODE_OPEN                  0x4000
#define IW_ENCODE_RESTRICTED            0x2000
#define IW_ENCODE_NOKEY                 0x0800
#define IW_ENCODE_TEMP                  0x0400

/* Maximum number of frequencies. */
#define IW_MAX_FREQUENCIES              32

/* Commands used with SIOCSIWMLME. */
#define IW_MLME_DEAUTH                  1
#define IW_MLME_DISASSOC                2
#define IW_MLME_AUTH                    3
#define IW_MLME_ASSOC                   4

/* Different wireless modes. */
#define IW_MODE_ADHOC                   1
#define IW_MODE_INFRA                   2
#define IW_MODE_MASTER                  3

/* Commands used with SIOCSIWPMKSA. */
#define IW_PMKSA_ADD                    1
#define IW_PMKSA_FLUSH                  2
#define IW_PMKSA_REMOVE                 3

/* Flags for power management. */
#define IW_POWER_PERIOD                 0x0001
#define IW_POWER_TIMEOUT                0x0002

/* Bit-mask for SIOCGIWRETRY/SIOCSIWRETRY. */
#define IW_RETRY_TYPE                   0xff00
#define IW_RETRY_MODIFIER               0x00ff

/* Options for SIOCGIWRETRY/SIOCSIWRETRY. */
#define IW_RETRY_LIFETIME               0x0100
#define IW_RETRY_LIMIT                  0x0200
#define IW_RETRY_MAX                    0x0001
#define IW_RETRY_MIN                    0x0002

/* Misc. commands and options. */
#define IW_SCAN_THIS_ESSID              0x0001
#define IW_TXPOW_DBM                    1

/* Maximum number of encoding sizes. */
#define IW_MAX_ENCODING_SIZES           8

/* Maximum ESSID length, in bytes. */
#define IW_ESSID_MAX_SIZE               32

/* Maximum length of PMKID. */
#define IW_PMKID_LEN                    16

/* Maximum size of scan data which can be returned. */
#define IW_SCAN_MAX_DATA                4096

/******************************** EVENTS ********************************/

/* Event IDs. */
#define IWEVCUSTOM                  9000
#define IWEVGENIE                   9001
#define IWEVQUAL                    9002
#define IWEVREGISTERED              9003
#define IWEVEXPIRED                 9004
#define IWEVPMKIDCAND               9005
#define IWEVASSOCRESPIE             9006
#define IWEVASSOCREQIE              9007

/* Sizes of event data specified in the "iw_event" structure. */
#define IW_EV_LCP_LEN               (sizeof(struct iw_event) - \
                                     sizeof(union iwreq_data))
#define IW_EV_ADDR_LEN              (IW_EV_LCP_LEN + \
                                     sizeof(struct iw_int_sockaddr))
#define IW_EV_CHAR_LEN              (IW_EV_LCP_LEN + IFNAMSIZ)
#define IW_EV_FREQ_LEN              (IW_EV_LCP_LEN + sizeof(struct iw_freq))
#define IW_EV_PARAM_LEN             (IW_EV_LCP_LEN + sizeof(struct iw_param))
#define IW_EV_QUAL_LEN              (IW_EV_LCP_LEN + sizeof(struct iw_quality))
#define IW_EV_UINT_LEN              (IW_EV_LCP_LEN + sizeof(UINT32))
#define IW_EV_POINT_OFF             (((char *)(&((struct iw_point *)NULL) \
                                     ->length)) - ((char *)NULL))
#define IW_EV_POINT_LEN             (IW_EV_LCP_LEN + \
                                     sizeof(struct iw_point) - \
                                     IW_EV_POINT_OFF)

/**************************** DATA STRUCTURES ***************************/

/* Internally-used definition of the "sockaddr" structure. */
struct iw_int_sockaddr
{
    UINT32          sa_family;
    UINT8           sa_data[32];
};

/* Data structure for SIOCGIWENCODEEXT/SIOCSIWENCODEEXT. */
struct iw_encode_ext
{
    UINT32          ext_flags;
    UINT16          alg;
    UINT16          key_len;

    struct iw_int_sockaddr addr;

    UINT8           rx_seq[IW_ENCODE_SEQ_MAX_SIZE];
    UINT8           tx_seq[IW_ENCODE_SEQ_MAX_SIZE];
    UINT8           key[1];
};

/* Data used with various options. */
struct iw_param
{
    INT32           value;
    UINT16          flags;
    UINT8           fixed;
    UINT8           disabled;
};

/* Data used with various options.
 *
 * NOTE: Make sure the "length" item in this structure is always the
 *       first data member. An optional "pointer" data member can be
 *       added as the first data member, if required.
 */
struct iw_point
{
    UINT16          length;
    UINT16          flags;
};

/* Data used with SIOCGIWFREQ/SIOCSIWFREQ. */
struct iw_freq
{
    INT32           m;
    INT16           e;
    UINT8           i;
    UINT8           padding;
};

/* Data used to specify link quality. Usually used with the
 * SIOCGIWTHRSPY/SIOCSIWTHRSPY IOCTL. */
struct iw_quality
{
    UINT8           qual;
    UINT8           noise;
    UINT8           level;
    UINT8           updated;
};

/* Data used with SIOCSIWMLME. */
struct iw_mlme
{
    UINT16          cmd;
    UINT16          reason_code;

    struct iw_int_sockaddr      addr;
};

/* Data used with SIOCSIWPMKSA.*/
struct iw_pmksa
{
    UINT32          cmd;
    UINT8           pmkid[16];

    struct iw_int_sockaddr      bssid;
};

struct iw_range
{
    UINT32          enc_capa;
#ifdef CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE
    UINT32          event_capa[6];
    UINT8           scan_capa;
    INT32           bitrate[IW_MAX_BITRATES];   /* list, in bps */
#endif
    UINT32          txpower_capa;
    UINT32          throughput;
    INT32           sensitivity;
    INT32           min_pmp;
    INT32           max_pmp;
    INT32           min_pmt;
    INT32           max_pmt;
    INT32           min_retry;
    INT32           max_retry;
    INT32           min_rts;
    INT32           max_rts;
    INT32           min_frag;
    INT32           max_frag;
    UINT16          pmp_flags;
    UINT16          pmt_flags;
    UINT16          pm_capa;
    UINT16          retry_capa;
    UINT16          retry_flags;
    UINT16          num_channels;
    UINT16          encoding_size[IW_MAX_ENCODING_SIZES];
    UINT8           max_encoding_tokens;
    UINT8           num_encoding_sizes;
    UINT8           num_bitrates;
    UINT8           num_frequency;
    UINT8           we_version_compiled;
    UINT8           we_version_source;

    struct iw_freq              freq[IW_MAX_FREQUENCIES];
    struct iw_quality           max_qual;
};

/* Data used with SIOCSIWSCAN. */
struct iw_scan_req
{
    UINT8           essid[IW_ESSID_MAX_SIZE];
    UINT8           essid_len;
#ifdef CFG_NU_OS_DRVR_WLAN_AR6K3_ENABLE 
    UINT8           num_channels;
    struct iw_freq  channel_list[IW_MAX_FREQUENCIES];
#endif

    struct iw_int_sockaddr      bssid;
};

union iwreq_data {
    struct iw_freq          freq;
    struct iw_param         bitrate;
    struct iw_quality       qual;
    struct iw_point         essid;
    struct iw_point         data;
    struct iw_int_sockaddr  ap_addr;

    UINT8                   name[IFNAMSIZ + 1];
    UINT32                  mode;
};

/* Data structure for storing event data. */
struct iw_event
{
    UINT16          len;
    UINT16          cmd;

    union iwreq_data    u;
};

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif  /* NUCLEUS_WIRELESS_H */
