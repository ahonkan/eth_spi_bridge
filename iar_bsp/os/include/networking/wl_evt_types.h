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
*       wl_evt_types.h
*
*   COMPONENT
*
*       WL - WLAN Events Data
*
*   DESCRIPTION
*
*       This file defines the event types and event data for the
*       events of the "wireless" channel.
*
*   DATA STRUCTURES
*
*       WL_EVTDATA_ASSOC_INFO
*       WL_EVTDATA_MICHAEL_MIC_FAILURE
*       WL_EVTDATA_INTERFACE_STATUS
*       WL_EVTDATA_PMKID_CANDIDATE
*       WL_EVTDATA_STKSTART
*       WL_EVTDATA_FT_IES
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef WL_EVT_TYPES_H
#define WL_EVT_TYPES_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

/* Event identifiers for wireless events. */

/* This event is triggered when an 802.11 association or reassociation
 * occurs. The "get BSSID" request should be successful after this
 * event. Optionally, the WL_EVENT_ASSOCINFO can be generated right
 * before this event. Or data of WL_EVENT_ASSOCINFO can also be provided
 * in the WL_EVENT_ASSOC event, if available.
 */
#define WL_EVENT_ASSOC                  0x00000001

/* This event is triggered when an 802.11 disassociation occurs. This
 * may occur as a result of receiving/sending deauthenticate or
 * disassociate frames from/to the AP.
 */
#define WL_EVENT_DISASSOC               0x00000002

/* This event occurs when a Michael MIC (TKIP) error occurs. Additional
 * data related to this event should be passed in the
 * WL_EVTDATA_MICHAEL_MIC_FAILURE structure.
 */
#define WL_EVENT_MICHAEL_MIC_FAILURE    0x00000004

/* This event is triggered after a "scan" request when scan results
 * are available. This event indicates that scan results can now be
 * obtained using the "get scan results" call to the wireless driver.
 */
#define WL_EVENT_SCAN_RESULTS           0x00000008

/* This event is used to provide extra association related data such
 * as IEs, Beacon response frames and Probe response frames. This
 * event should be triggered right before the WL_EVENT_ASSOC event.
 * Optionally, this data can also be provided in the WL_EVENT_ASSOC
 * event. Event data should be provided in the WL_EVTDATA_ASSOC_INFO
 * structure.
 */
#define WL_EVENT_ASSOCINFO              0x00000010

/* This event is used to notify changes in the interface status using
 * the WL_EVTDATA_INTERFACE_STATUS structure. This event occurs, for
 * example, when a wireless card is inserted/removed from the slot.
 */
#define WL_EVENT_INTERFACE_STATUS       0x00000020

/* This event is used to report candidate APs for pre-authentication.
 * This event is optional and depends on whether the listener itself
 * will handle scan requests. If so, this event isn't required. But if
 * scan requests are not handled by the listener, then this event is
 * used to report candidates for WPA2 pre-authentication. This event's
 * data should be passed in the WL_EVTINFO_PMKID_CANDIDATE structure.
 */
#define WL_EVENT_PMKID_CANDIDATE        0x00000040

/* This event is used to request an STK handshake to allow setting up
 * a secure channel between two stations. The peer address should be
 * passed with this event using the WL_EVTINFO_STKSTART structure.
 */
#define WL_EVENT_STKSTART               0x00000080

/* This event is used to report FT IEs for FT authentication
 * sequence from the AP. Event data should be provided in the
 * WL_EVTINFO_FT_IES structure.
 */
#define WL_EVENT_FT_RESPONSE            0x00000100

/* This event is sent on discovery of a new node in AP mode. This
 * event's data should be passed in the WL_EVTDATA_NODE_ADDR structure.
 */
#define WL_EVENT_NODE_REGISTERED        0x00000200

/* This event is sent on expiry of a node in AP mode. This is the
 * opposite of the WL_EVENT_REGISTERED event. This event's data
 * should be passed in the WL_EVTDATA_NODE_ADDR structure.
 */
#define WL_EVENT_NODE_EXPIRED           0x00000400

/* This event is used to send the Generic IE (WPA, RSN, WMM, etc)
 * in scan results. This includes ID and length fields. A single event
 * may contain more than one IE. Scan results may contain one or more
 * WL_EVENT_GENIE events. This event's data should be passed in the
 * WL_EVTDATA_GENIE structure.
 */
#define WL_EVENT_GENIE                  0x00000800

/* Minimum and maximum range of values for custom wireless event IDs. */
#define WL_EVENT_CUSTOM_MIN             0x00001000
#define WL_EVENT_CUSTOM_MAX             0x00008000

/* This event is used to send the interface removed signal to wireless
 * driver. Based on this event, the driver will allow the hibernate module
 * to proceed and shut down the system. All the other events listed above
 * are usually sent from the wireless driver to the WPA Supplicant whereas 
 * this event is special as it is sent from the WPA Supplicant to the wireless
 * driver to signal removal of the supplicant's interface. This functionality
 * is specifically used to synchronize tasks during hibernation.
 */

#define WL_EVENT_INTERFACE_REM          0x00010000


/* Structure Definitions.
 */

/* Data for WL_EVENT_ASSOC and WL_EVENT_ASSOCINFO events. This data is
 * optional for the former and mandatory for the latter event.
 */
typedef struct wl_evtdata_assoc_info
{
    /* Association and reassociation request IEs. This is required
     * if the driver generates IE and optional is WPA supplicant
     * generated IE are used.
     */
    UINT8           *wl_req_ies;
    UINT32          wl_req_ies_len;         /* Length of "wl_req_ies". */

    /* Association and reassociation response IEs. This data is
     * not used for WPA but should nonetheless be reported if
     * is known to the driver.
     */
    UINT8           *wl_resp_ies;
    UINT32          wl_resp_ies_len;        /* Length of "wl_resp_ies". */

    /* Beacon or Probe Response IEs. */
    UINT8           *wl_beacon_ies;
    UINT32          wl_beacon_ies_len;      /* "wl_beacon_ies" length. */
} WL_EVTDATA_ASSOC_INFO;

/* Data for WL_EVENT_MICHAEL_MIC_FAILURE. */
typedef struct wl_evtdata_michael_mic_failure
{
    INT32           wl_unicast;
} WL_EVTDATA_MICHAEL_MIC_FAILURE;

/* Data for WL_EVENT_INTERFACE_STATUS. */
typedef struct wl_evtdata_interface_status
{
    DV_DEV_ID       wl_dev_id;

    enum {
        WL_EVENT_INTERFACE_ADDED,
        WL_EVENT_INTERFACE_REMOVED,
        WL_EVENT_INTERFACE_UP,
        WL_EVENT_INTERFACE_DORMANT
    } wl_ievent;
} WL_EVTDATA_INTERFACE_STATUS;

/* Data for WL_EVENT_PMKID_CANDIDATE. */
typedef struct wl_evtdata_pmkid_candidate
{
    /* BSSID of candidate. */
    UINT8           wl_bssid[DADDLEN];
    INT32           wl_index;               /* Priority. Smaller value
                                             * is higher priority. */
    INT32           wl_preauth;             /* Whether RSN IE includes
                                             * the pre-auth flag? */
} WL_EVTDATA_PMKID_CANDIDATE;

/* Data for the WL_EVENT_STKSTART event. */
typedef struct wl_evtdata_stkstart
{
    /* Address of the peer. */
    UINT8           wl_peer[DADDLEN];
} WL_EVTDATA_STKSTART;

/* Data for the WL_EVENT_FT_RESPONSE event. */
typedef struct wl_evtdata_ft_ies
{
    UINT8           *wl_ies;
    INT32           wl_ies_len;
    INT32           wl_ft_action;
    UINT8           wl_target_ap[DADDLEN];
} WL_EVTDATA_FT_IES;

/* Data for the WL_EVENT_NODE_REGISTERED and WL_EVENT_NODE_EXPIRED
 * events.
 */
typedef struct wl_evtdata_node_addr
{
    /* Address of the node. */
    UINT8           wl_addr[DADDLEN];
} WL_EVTDATA_NODE_ADDR;

/* Data for the WL_EVENT_GENIE event. */
typedef struct wl_evtdata_genie
{
    INT16           wl_id;
    INT16           wl_data_len;

    /* The data size of this event can be variable, so the data
     * member below is used to access all the following bytes after
     * this structure. */
    UINT8           wl_data[1];
} WL_EVTDATA_GENIE;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* WL_EVT_TYPES_H */
