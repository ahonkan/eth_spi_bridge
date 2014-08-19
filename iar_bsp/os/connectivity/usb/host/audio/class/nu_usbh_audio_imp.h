/**************************************************************************
*
*               Copyright 2012  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************
***************************************************************************
*
* FILE NAME
*     nu_usbh_audio_imp.h
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains Control Block and definitions for Nucleus USB Host
*     AUDIO Class Driver.
*
* DATA STRUCTURES
*       NU_USBH_AUDIO                       Audio Host Class Driver Control
*                                           Block.
*       NU_USBH_AUD_DEV                     Audio Device control block
*                                           structure.
*       AUDH_FEATURE_CONTROLS               Feature Unit possible Controls.
*       AUDH_STRM_TYPE                      Streaming interfaces MIDI/ASI.
*       AUDH_ENTITY_TYPE                    Audio Entity Type
*                                           (Unit/Terminals).
*       AUDH_ENTITY_SUB_TP                  Audio Entity Sub Types.
*       AUDH_CS_REQ_CODE                    Audio Class Specific request
*                                           codes.
*       AUDH_FORMAT_TAG                     USB Audio Data Formats Code.
*       AUDH_SMPL_FREQ_TYPE                 Sampling frequency type either
*                                           continuous or discrete.
*       NU_USBH_AUD_ENTITY                  Complete info about
*                                           Terminal/Unit.
*       NU_USBH_AUD_FEATURE                 Info related to Feature Unit.
*       AUDH_OP_TERM                        Info related to Output
*                                           Terminal.
*       AUDH_IP_TERM                        Info related to Input Terminal.
*       NU_USBH_AUD_CTRL                    Info related to Audio Control
*                                           interface.
*       AUDH_PLAY_INFORM                    Info related to play/record
*                                           Audio clip.
*       AUDH_CS_ASI_INFO                    Class specific streaming intf
*                                           info.
*       AUDH_ASI_INFO_STRUCT                Streaming Interface info
*                                           structure.
*       NU_USBH_AUD_CTRL_REQ                Audio Control Request
*                                           structure.
*       NU_USBH_AUD_EP_REQ                  Audio Endpoint Request
*                                           structure.
*       AUDH_TYPE1_DSCR_INFO                Class specific Type-1 Format
*                                           Type info.
*       NU_USBH_AUD_FUNC_INFO               Audio Function(Speaker,
*                                           Microphone) info structure.
*       NU_AUDH_TRANSFER                    Structure for holding playing
*                                           and recording transfer info.
*
* FUNCTIONS
*       None.
*
* DEPENDENCIES
*       nu_usbh_audio_dat.h                 Dispatch Table Definitions.
*       nu_usbh_audio_cfg.h                 Audio Host Configurable macros.
*       nu_usbh_ext.h                       USB host external file.
*
**************************************************************************/
/* ===================================================================== */
#ifndef _NU_USBH_AUD_IMP_H_
#define _NU_USBH_AUD_IMP_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error  Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++. */
#endif

#include "nu_usbh_audio_cfg.h"

/* #defines. */

/* Audio class and Subclass code on the basis of which driver is found. */
#define NU_AUDH_CLASS                    0x01/* Audio Class Code. */
#define NU_AUDH_AC_SUBCLASS              0x01/* Audio Control subclass code.
                                              */
#define NU_AUDH_AS_SUBCLASS              0x02/* Audio Streaming subclass. */

/*
 * Number of irps to be submitted simultaneously for a single playback
 * transfer.
 */
#define NU_AUDH_NUM_PLAY_TRANS 			   CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_NUM_PLAY_TRANS

/*
 * Event codes for task events.
 */
#define NU_AUDH_PLAY_TASK_TERMINATE         (1<<0)
#define NU_AUDH_PLAY_TASK_TERMINATED        (1<<1)
#define NU_AUDH_REC_TASK_TERMINATE          (1<<2)
#define NU_AUDH_REC_TASK_TERMINATED         (1<<3)

/* Rollback defines: */
#define NU_AUDH_DEL_EVENTS               0x05
#define NU_AUDH_DEL_CS_ASI               0x04
#define NU_AUDH_DEL_SI_INFO              0x03
#define NU_AUDH_RELEASE_PARSE_MEM        0x02
#define NU_AUDH_DEL_DEV_STRUCT           0x01
#define NU_AUDH_NO_ROLLBACK              0x00

/* Request codes. */
#define NU_AUDH_GET_INF_REQUEST          0xA1
#define NU_AUDH_SET_INF_REQUEST          0x21
#define NU_AUDH_GET_EP_REQUEST           0xA2
#define NU_AUDH_SET_EP_REQUEST           0x22

/* Speaker and Microphone functions. */
#define NU_AUDH_SPKR_FUNCTION            0x01
#define NU_AUDH_MIC_FUNCTION             0x02
#define NU_AUDH_INVALID_ID               0xFF

/* Stack size for the play task and record tasks. */
#define NU_AUDH_PLAY_STACK_SIZE          (4096*4)
#define NU_AUDH_RECORD_STACK_SIZE        (4096*4)

/* Audio control block related. */
/* Following are specific to standard AC header descriptor. */
#define NU_AUDH_HDR_BCDADC_OFSET        (NU_AUDH_DSCR_SUB_TYPE_OFSET + 1)
#define NU_AUDH_HDR_BCDADC_SIZE          0x02
#define NU_AUDH_HDR_TOT_LEN_OFSET       (NU_AUDH_HDR_BCDADC_OFSET + \
                                         NU_AUDH_HDR_BCDADC_SIZE)
#define NU_AUDH_HDR_TOT_LEN_SIZE         0x02
#define NU_AUDH_HDR_NUM_SI_OFSET        (NU_AUDH_HDR_TOT_LEN_OFSET + \
                                         NU_AUDH_HDR_TOT_LEN_SIZE)
#define NU_AUDH_HDR_SI_INF_OFSET        (NU_AUDH_HDR_NUM_SI_OFSET + 1)
#define NU_AUDH_MAX_ENTITIES_TYPE        0x09

/* Class specific header descriptor related. */
#define NU_AUDH_CS_INTERFACE             0x24
#define NU_AUDH_MAX_ENTITY_COUNT         0x09
#define NU_AUDH_MAX_TYPE_FMT_COUNT       0x03

/* Following are valid for all entity types. */
#define NU_AUDH_ENTITY_ID_OFSET          0x03
#define NU_AUDH_LENGTH_OFSET             0x00
#define NU_AUDH_DSCR_TYPE_OFSET          0x01
#define NU_AUDH_DSCR_SUB_TYPE_OFSET      0x02
#define NU_AUDH_CHNL_CLSTR_INFO_SIZE     0x04

/* Specific to selector unit. */
#define NU_AUDH_SEL_NO_IP_PINS_OFSET    (NU_AUDH_ENTITY_ID_OFSET + 1)
#define NU_AUDH_SEL_SRC_ID_OFSET        (NU_AUDH_SEL_NO_IP_PINS_OFSET + 1)
#define NU_AUDH_CONST_SEL_UNIT_SIZE      0x06

/* Specific to mixer unit. */
#define NU_AUDH_MIXER_IP_PINS_CNT_OFSET (NU_AUDH_ENTITY_ID_OFSET + 1)
#define NU_AUDH_MIXER_SRC_ID_OFSET      (NU_AUDH_MIXER_IP_PINS_CNT_OFSET \
                                          +1)
/* Specific to processing unit. */
#define NU_AUDH_PROC_TYPE_OFSET         (NU_AUDH_ENTITY_ID_OFSET + 1)
#define NU_AUDH_PROC_TYPE_SIZE           0x02
#define NU_AUDH_PROC_NO_IP_PINS_OFSET   (NU_AUDH_PROC_TYPE_OFSET + \
                                         NU_AUDH_PROC_TYPE_SIZE)
#define NU_AUDH_PROC_SRC_ID_OFSET       (NU_AUDH_PROC_NO_IP_PINS_OFSET \
                                          +1)
/* Specific to extension unit. */
#define NU_AUDH_EXT_CODE_OFSET          (NU_AUDH_ENTITY_ID_OFSET + 1)
#define NU_AUDH_EXT_CODE_SIZE            0x02
#define NU_AUDH_EXT_NO_IP_PINS_OFSET    (NU_AUDH_EXT_CODE_OFSET + \
                                         NU_AUDH_EXT_CODE_SIZE)
#define NU_AUDH_EXT_SRC_ID_OFSET        (NU_AUDH_EXT_NO_IP_PINS_OFSET + 1)

/* Specific to Terminals. */
#define NU_AUDH_TERM_TYPE_OFSET          0x04
#define NU_AUDH_TERM_TYPE_SIZE           0x02
#define NU_AUDH_ASSOC_TERM_OFSET        (NU_AUDH_TERM_TYPE_OFSET + \
                                         NU_AUDH_TERM_TYPE_SIZE)
#define NU_AUDH_OP_TERM_SRCID_OFSET     (NU_AUDH_ASSOC_TERM_OFSET + 1)
#define NU_AUDH_INPUT_TERM_SIZE          0x0C
#define NU_AUDH_OUTPUT_TERM_SIZE         0x09

/* Specific to Feature Unit. */
#define NU_AUDH_FEATURE_SRC_ID_OFSET    (NU_AUDH_ENTITY_ID_OFSET + 1)
#define NU_AUDH_FU_CTRL_SIZE_OFSET      (NU_AUDH_FEATURE_SRC_ID_OFSET + 1)
#define NU_AUDH_MIN_FEATURE_CTRL_SIZE    0x01
#define NU_AUDH_MAX_FEATURE_CTRL_SIZE    0x04

/* ASI specific. */
#define NU_AUDH_ASI_TERM_LINK_OFSET     (NU_AUDH_DSCR_SUB_TYPE_OFSET + 1)
#define NU_AUDH_ASI_DT_PATH_DEL_OFSET   (NU_AUDH_ASI_TERM_LINK_OFSET  + 1)
#define NU_AUDH_ASI_DSCR_SIZE            0x07
#define NU_AUDH_CS_ASI_EP_DSCR_SIZE      0x07
#define NU_AUDH_MAX_ISO_PKT_IRP          100 /* Do not change this value.
                                              * It will disturb length
                                              * check of NU_USBH_AUD_Play_Sound
                                              * and NU_AUDH_Play_Audio.
                                              * Maximum transactions count
                                              * that can be sent in a
                                              * single iso irp.
                                              */
/* ASI descriptor sub types */
#define NU_AUDH_AS_GENERAL               0x01
#define NU_AUDH_FORMAT_TYPE              0x02
#define NU_AUDH_FORMAT_SPECIFIC          0x03

/* Specific to Endpoint descriptor. */
#define NU_AUDH_EP_GENERAL               0x01
#define NU_AUDH_CS_ENDPOINT              0x25

/* ASI Class specific data Endpoint related. */
#define NU_AUDH_CS_EP_CTRLS_OFSET       (NU_AUDH_DSCR_SUB_TYPE_OFSET + 1)
#define NU_AUDH_CS_EP_LOCKDEL_UT_OFSET  (NU_AUDH_CS_EP_CTRLS_OFSET + 1)
#define NU_AUDH_CS_EP_LOCK_DEL_OFSET    (NU_AUDH_CS_EP_LOCKDEL_UT_OFSET \
                                          + 1)
#define NU_AUDH_CS_EP_LOCK_DEL_SIZE      0x02
#define NU_AUDH_ASI_FMT_TAG_OFSET       (NU_AUDH_ASI_DT_PATH_DEL_OFSET \
                                          + 1)

/* Specific to the Format Type Descriptors. */
#define NU_AUDH_FMT_TYPE_I_STD_SIZE      0x08
#define NU_AUDH_FMT_TYPE_OFSET          (NU_AUDH_DSCR_SUB_TYPE_OFSET + 1)
#define NU_AUDH_FMT_TYPE_CHNLCNT_OFSET  (NU_AUDH_FMT_TYPE_OFSET + 1)
#define NU_AUDH_FMT_TYPE_SUBFRM_OFSET   (NU_AUDH_FMT_TYPE_CHNLCNT_OFSET \
                                          + 1)
#define NU_AUDH_FMT_TYPE_BITRES_OFSET   (NU_AUDH_FMT_TYPE_SUBFRM_OFSET +1)
#define NU_AUDH_FTYP1_SMPL_TP_OFSET     (NU_AUDH_FMT_TYPE_BITRES_OFSET +1)
#define NU_AUDH_FTM_SMPL_INFO_OFSET     (NU_AUDH_FTYP1_SMPL_TP_OFSET + 1)
#define NU_AUDH_SAMPLING_FREQ_CONTROL            0x01

/* Format types range 0 to NU_AUDH_MAX_FMT_TYPE. */
#define NU_AUDH_MAX_FMT_TYPE             0x02
#define NU_AUDH_GROUP_TYPE_POS           0x08

/* Feature unit controls codes picked up from Audio class spec. */
#define NU_AUDH_MUTE_CONTROL             0x01
#define NU_AUDH_VOLUME_CONTROL           0x02
#define NU_AUDH_BASS_CONTROL             0x03
#define NU_AUDH_MID_CONTROL              0x04
#define NU_AUDH_TREBLE_CONTROL           0x05
#define NU_AUDH_GRAPH_EQ_CONTROL         0x06
#define NU_AUDH_AUTO_GAIN_CONTROL        0x07
#define NU_AUDH_DELAY_CONTROL            0x08
#define NU_AUDH_BASS_BOAST_CONTROL       0x09
#define NU_AUDH_LOUDNESS_CONTROL         0x0A

/* Error Codes. */
#define NU_AUDH_STATUS_BASE              -3900
#define NU_AUDH_INVALID_UNIT_TYPE        NU_AUDH_STATUS_BASE-1
#define NU_AUDH_MEM_FAIL                 NU_AUDH_STATUS_BASE-2
#define NU_AUDH_INVALID_TERM_SIZE        NU_AUDH_STATUS_BASE-3
#define NU_AUDH_IP_CHNL_CLSTR_FAIL       NU_AUDH_STATUS_BASE-4
#define NU_AUDH_MIXER_CHNL_CLSTR_FAIL    NU_AUDH_STATUS_BASE-5
#define NU_AUDH_INVALID_SEL_UNIT_SIZE    NU_AUDH_STATUS_BASE-6
#define NU_AUDH_NO_HEADER                NU_AUDH_STATUS_BASE-7
#define NU_AUDH_INVALID_HEADER_SIZE      NU_AUDH_STATUS_BASE-8
#define NU_AUDH_NO_TERM_UNITS_PRESENT    NU_AUDH_STATUS_BASE-9
#define NU_AUDH_ENT_SIZE_OVERFLOW        NU_AUDH_STATUS_BASE-10
#define NU_AUDH_INVALID_CS_DSCR_TYPE     NU_AUDH_STATUS_BASE-11
#define NU_AUDH_INVALID_CSD_SUB_TYPE     NU_AUDH_STATUS_BASE-12
#define NU_AUDH_INVALID_PROC_TYPE        NU_AUDH_STATUS_BASE-13
#define NU_AUDH_INVALID_PROC_CTRL_SIZE   NU_AUDH_STATUS_BASE-14
#define NU_AUDH_PROC_CHNL_CLSTR_FAIL     NU_AUDH_STATUS_BASE-15
#define NU_AUDH_INVALID_PROC_SIZE        NU_AUDH_STATUS_BASE-16
#define NU_AUDH_INVALID_EXT_SIZE         NU_AUDH_STATUS_BASE-17
#define NU_AUDH_EXT_CHNL_CLSTR_FAIL      NU_AUDH_STATUS_BASE-18
#define NU_AUDH_LESS_NO_CHNLS            NU_AUDH_STATUS_BASE-19
#define NU_AUDH_INVALID_SRC_ID           NU_AUDH_STATUS_BASE-20
#define NU_AUDH_OP_TERM_IS_SRC           NU_AUDH_STATUS_BASE-21
#define NU_AUDH_ASI_DSCR_TYPE_INVALID    NU_AUDH_STATUS_BASE-22
#define NU_AUDH_ASID_SUB_TYPE_INVALID    NU_AUDH_STATUS_BASE-23
#define NU_AUDH_LINK_TERM_NOT_FOUND      NU_AUDH_STATUS_BASE-24
#define NU_AUDH_INVALID_AUD_FMTS_TYPE    NU_AUDH_STATUS_BASE-25
#define NU_AUDH_INVALID_CS_EP_DSCR_SIZE  NU_AUDH_STATUS_BASE-26
#define NU_AUDH_INVALID_TID_TYPE         NU_AUDH_STATUS_BASE-27
#define NU_AUDH_INVALID_TID_SUB_TYPE     NU_AUDH_STATUS_BASE-28
#define NU_AUDH_INVALID_TIF_DSCR_SIZE    NU_AUDH_STATUS_BASE-29
#define NU_AUDH_TYPE_I_FMT_MATCH_FAIL    NU_AUDH_STATUS_BASE-30
#define NU_AUDH_TIFD_INVALID_SUB_TYPE    NU_AUDH_STATUS_BASE-31
#define NU_AUDH_INVALID_CS_EP_DSCR_TYPE  NU_AUDH_STATUS_BASE-32
#define NU_AUDH_INVALID_CS_EPD_SUB_TYPE  NU_AUDH_STATUS_BASE-33
#define NU_AUDH_PARSE_SI_FAILED          NU_AUDH_STATUS_BASE-34
#define NU_AUDH_INVALID_FU_CTRL_SIZE     NU_AUDH_STATUS_BASE-35
#define NU_AUDH_NOT_AVAILBLE             NU_AUDH_STATUS_BASE-37
#define NU_AUDH_NOT_SUPPORTED            NU_AUDH_STATUS_BASE-39
#define NU_AUDH_NOT_FOUND                NU_AUDH_STATUS_BASE-40
#define NU_AUDH_INVALID_CHNL_NO          NU_AUDH_STATUS_BASE-41
#define NU_AUDH_SERVICE_UNAVAILBLE       NU_AUDH_STATUS_BASE-42
#define NU_AUDH_TRANSFER_ABORTED         NU_AUDH_STATUS_BASE-43
#define NU_AUDH_INVALID_SRC_ID_OFSET     NU_AUDH_STATUS_BASE-44
#define NU_AUDH_INVALID_PIN_OFSET        NU_AUDH_STATUS_BASE-45
#define NU_AUDH_FUNC_NOT_PRESENT         NU_AUDH_STATUS_BASE-46
#define NU_AUDH_FU_NOT_PRESENT           NU_AUDH_STATUS_BASE-47
#define NU_AUDH_UNAVAILBLE               NU_AUDH_STATUS_BASE-48

/* Event Flags. */
#define NU_AUDH_CTRL_SENT                0x01
#define NU_AUDH_INTR_SENT                0x02
#define NU_AUDH_OUT_SENT                 0x08
#define NU_AUDH_AVAILBLE                 0x01

/* Validation Macros. */
#define NU_AUDH_IS_TERM_SIZE_VALID(a,b)  \
    ((((a) == AUDH_INPUT_TERMINAL) && ((b) == NU_AUDH_INPUT_TERM_SIZE)) ||\
    (((a) == AUDH_OUTPUT_TERMINAL) &&((b) == NU_AUDH_OUTPUT_TERM_SIZE)) )

#define NU_AUDH_IS_VALID_ACI_SUB_TYPE(a) \
            (a > (UINT8)AUDH_HEADER) && (a <= (UINT8)AUDH_EXTENSION_UNIT)

/* Is the entity of terminal type? */
#define NU_AUDH_IS_TERMINAL(a)           \
        ((a->ent_tp == AUDH_INPUT_TERMINAL) || \
         (a->ent_tp == AUDH_OUTPUT_TERMINAL))

/* Is it one of the TYPE-I format code? */
#define NU_AUDH_IS_TYPE_I_FORMAT(a) ((a == AUDH_PCM)  || \
                                    (a == AUDH_PCM8) || \
                                    (a == AUDH_IEEE_FLOAT) || \
                                    (a == AUDH_ALAW) || \
                                    (a == AUDH_MULAW))

extern NU_EVENT_GROUP USBH_AUD_USER_INIT_Events;

/* Streaming interfaces types. */
typedef enum
{
    AUDH_ASI = 0x02, AUDH_MSI = 0x03

}AUDH_STRM_TYPE;

/* Entity types. */
typedef enum
{
    AUDH_AC_DSCR_UNDEF, AUDH_HEADER, AUDH_INPUT_TERMINAL,
    AUDH_OUTPUT_TERMINAL, AUDH_MIXER_UNIT, AUDH_SELECTOR_UNIT,
    AUDH_FEATURE_UNIT, AUDH_PROCESSING_UNIT, AUDH_EXTENSION_UNIT

}AUDH_ENTITY_TYPE;

/* Terminal type codes picked up form the terminal type spec. */
typedef enum
{
    /* Invalid sub type. */
    AUDH_SUB_TYPE_UNDEFINED,
    /* USB terminal types. */
    AUDH_UNDEFINED = 0x0100,
    AUDH_STREAMING, AUDH_VENDOR_SPECIFIC = 0x01FF,
    /* Input terminal types. */
    AUDH_INP_UNDEF = 0x0200,
    AUDH_MICROPHONE, AUDH_DESKTOP_MICROPHONE, AUDH_PSNL_MICROPHONE,
    AUDH_OMIN_DIR_MICROPHONE, AUDH_MICROPHONE_ARR,
    AUDH_PROC_MICROPHONE_ARR,
    /* Output terminal types. */
    AUDH_OUTP_UNDEF = 0x0300,
    AUDH_SPEAKER, AUDH_HEADPHONES,  AUDH_HEAD_MNT_DSPL_AUD,
    AUDH_DESKTOP_SPEAKER, AUDH_ROOM_SPEAKER, AUDH_COMM_SPEAKER,
    AUDH_LFE_SPEAKER,
    /* Bi-directional terminal types. */
    AUDH_BI_DIR_UNDEF = 0x0400,
    AUDH_HANDSET, AUDH_HEADSET, AUDH_SPKPHONE_NO_ECHO_RED,
    AUDH_ECHO_SUP_SPK_PHONE, AUDH_ECHO_CNCL_SPKPHONE,
    /* Telephony terminal types. */
    AUDH_TELE_UNDEF = 0x0500,
    AUDH_PHONE_LINE, AUDH_TELEPHONE, AUDH_DOWN_LINE_PHONE,
    /* External terminal types. */
    AUDH_EXT_UNDEF = 0x0600,
    AUDH_ANLOG_CONN, AUDH_DGT_INF, AUDH_LINE_CONN, AUDH_LEGENCY_CONN,
    AUDH_S_PDIF_INF, AUDH_1394_DA_STRM, AUDH_1394_DV_STRM_SND_TRACK,
    /* Embedded function terminal types. */
    AUDH_EMBD_UNDEF = 0x0700, AUDH_EMBD_LVL_CLBR_SRC, AUDH_EQ_NOICE,
    AUDH_CD_PLAYER, AUDH_DAT, AUDH_DCC, AUDH_MINI_DISK,
    AUDH_ANLOG_TAPE, AUDH_PHONEGRAPH, AUDH_VCR, AUDH_VID_DISC,
    AUDH_DVD, AUDH_TV_TUNER, AUDH_STLT_REC, AUDH_RADIO_RCV,
    AUDH_CABLE_TUNER, AUDH_DSS, AUDH_RADIO_TRANSM,
    AUDH_MULTI_TRACK_REC, AUDH_SYNTH,
    /* Processing unit process types. */
    AUDH_UNDEF_PROC = 0x0000,
    AUDH_UD_MIX_PROC, AUDH_DB_PROC, AUDH_3D_STEREO_PROC,
    AUDH_REVERB_PROC, AUDH_CHORUS_PROC, AUDH_DYN_RNG_CMPR_PROC

} AUDH_ENTITY_SUB_TP;

/* Class specific request codes. */
typedef enum
{
    AUDH_REQ_CODE_UNDEF = 0x00,
    AUDH_SET_CUR        = 0x01, AUDH_SET_MIN, AUDH_SET_MAX, AUDH_SET_RES,
    AUDH_GET_CUR        = 0x81, AUDH_GET_MIN, AUDH_GET_MAX, AUDH_GET_RES

}AUDH_CS_REQ_CODE;

/* USB Audio Data Formats Code. */
typedef enum
{

    /* Type1 Formats. */
    AUDH_TYPE_1_UNDEFINED = 0x0000, AUDH_PCM, AUDH_PCM8, AUDH_IEEE_FLOAT,
    AUDH_ALAW, AUDH_MULAW,

    /* Type2 Formats. */
    AUDH_TYPE_2_UNDEFINED = 0x1000, AUDH_MPEG, AUDH_AC_3,

    /* Type3 Formats. */
    AUDH_TYPE_3_UNDEFINED = 0x2000, AUDH_IEC1937_AC_3,
    AUDH_IEC1937_MPEG_1_LAYER_1,
    AUDH_IEC1937_MPEG_1_LATER_2 = 0x2003,
    AUDH_IEC1937_MPEG_1_LAYER_3 = 0x2003,
    AUDH_IEC1937_MPEG_2_NOEXT   = 0x2003,
    AUDH_IEC1937_MPEG_2_EXT,
    AUDH_IEC1937_MPEG_2_LAYER_1_LS,
    AUDH_IEC1937_MPEG_2_LAYER_2_LS = 0x2006,
    AUDH_IEC1937_MPEG_2_LAYER_3_LS = 0x2006,

    /* Zero band-width alt setting. */
    AUDH_ZERO_BANDWIDTH = 0x3000

} AUDH_FORMAT_TAG;

/* Sampling Frequency types. */
typedef enum
{
    AUDH_CONTINEOUS,                         /* Continuous sampling freq.
                                              */
    AUDH_DISCRETE                            /* Discrete sampling freq.
                                              */

} AUDH_SMPL_FREQ_TYPE;

/* Complete information about an entity. (Terminal / Unit). */
typedef struct nu_usbh_aud_entity
{
    VOID                 *ent_tp_spec_info;  /* Pointer to entity specific
                                              * information.
                                              */
    UINT8                *pred_list;         /* Predecessors entity list.
                                              */
    UINT8                *succ_list;         /* Successor entities list.
                                              */
    AUDH_ENTITY_TYPE      ent_tp;            /* Type of the entity. */
    AUDH_ENTITY_SUB_TP    ent_sub_tp;        /* Terminal type, processing
                                              * type etc. Valid in case of
                                              * terminals null otherwise.
                                              */
    UINT8                 ent_id;            /* Entity id. */
    UINT8                 ent_dscr_len;      /* Entity descriptor length.
                                              */
    UINT8                 ip_pins_count;     /* Input pin count will be
                                              * zero in case of input
                                              * terminal, one in case of
                                              * output terminal and vary
                                              * from unit to unit.
                                              */
    UINT8                 succ_count;        /* Number of successors. */

}NU_USBH_AUD_ENTITY;

/* Holds the information related to the feature unit. */
typedef struct  feature_info
{
    VOID                 *ctrls_list;        /* Pointer to the list of
                                              * supported controls for each
                                              * of the channels.
                                              */
    NU_USBH_AUD_ENTITY   *ent_info;          /* Parent data structure
                                              * pointer.
                                              */
    UINT8                 ctrl_size;         /* Size of supported controls
                                              * info for each of the
                                              * channels.
                                              */

    DATA_ELEMENT          cs_padding[3];

} NU_USBH_AUD_FEATURE;

typedef struct AUDH_ASI_INFO_STRUCT AUDH_ASI_INFO;

/* Holds the information about the Audio Control Interface. */
typedef struct  nu_usbh_aud_ctrl
{
    /* Complete information of all the entities. */
    NU_USBH_AUD_ENTITY   *ent_id_info[NU_AUDH_MAX_POSSIBLE_ENTITIES];
    UINT8                *si_list;           /* Streaming Interfaces List.
                                              */
    UINT16                tot_len;           /* Total length including all
                                              * units and terminals.
                                              */
    UINT8                 max_ent_id;        /* Maximum entity ID present
                                              * in this ACI. This will
                                              * reduce effort in those
                                              * operations that goes
                                              * through all the entities.
                                              */

    /*Store ACI Header Descriptor related info. */
    UINT8                 hdr_len;           /* Header Descriptor length.
                                              */
    UINT8                 si_count;          /* Number of Streaming
                                              * Interfaces for this ACI.
                                              */

    DATA_ELEMENT        cs_padding[3];


}NU_USBH_AUD_CTRL;

typedef VOID (*NU_USBH_AUD_Data_Callback) (
                       VOID*      device,
                       VOID*      buffer,
                       UINT32     status,
                       UINT32     transfer_length);

/* Class specific ASI information. */
typedef struct audh_cs_asi_info
{

    VOID                 *fmt_tp_dscr;       /* Format type information. */

    NU_USB_ALT_SETTG     *op_alt_sttg;       /* Alternate setting having
                                              * iso pipe.
                                              */
    NU_USB_PIPE          *pcb_iso_pipe;      /* Either iso in or iso out.*/
    AUDH_ASI_INFO        *asi_info;
    AUDH_ENTITY_TYPE      term_link_tp;      /* Type of terminal link. */

    AUDH_FORMAT_TAG       fmt_tag;           /* Format type code. */

    UINT8                 term_link;         /* Terminal associated with
                                              * this ASI alt setting.
                                              */


    DATA_ELEMENT          cs_padding[3];


}AUDH_CS_ASI_INFO;

/* This is the information holder for a playing or recording session. This
 * block is created per session per audio device by audio class driver.
 */
typedef struct splay_inform
{

    NU_QUEUE           double_buff_queue;    /* Queue for double buffering.
                                              */
    NU_USBH_AUD_Data_Callback
                          call_back;         /* Application call back
                                              * function pointer to be
                                              * called on session
                                              * completion.
                                              */
    NU_TASK              *pcb_task;          /* Task control block to
                                              * schedule transactions.
                                              */
    VOID                 *p_stack;           /* Pointer to scheduling task
                                              * stack.
                                              */
    VOID                 *p_buffer;          /* Pointer to application
                                              * buffer to hold or send
                                              * data.
                                              */
    AUDH_CS_ASI_INFO     *cs_asi_info;       /* Class specific Audio
                                              * streaming interface info.
                                              */
    VOID                    *queue_pointer;

    UINT32                buffer_length;     /* Length for application
                                              * buffer in bytes.
                                              */
    UINT32                data_rate;         /* Data rate to be maintain on
                                              * USB in bytes per second.
                                              */
    INT                   service_status;    /* Status for application to
                                              * report session status.
                                              */
    UINT32                compensator;       /* Compensator used in
                                              * packetizing algorithm.
                                              */
    UINT32                pkt_per_frame;     /* Packets pet frame.*/

    UINT8                 poll_interval;     /* Endpoint polling interval.
                                              */

    UINT8                 channels;

    UINT8                   sample_size;


    DATA_ELEMENT          cs_padding[1];


}AUDH_PLAY_INFORM;

typedef struct _usbh_aud_play_session_context
{
    NU_USB_ISO_IRP irp;

    UINT8               *buffer_ptrs_array[NU_AUDH_MAX_ISO_PKT_IRP];

    UINT16               pkt_len_array[NU_AUDH_MAX_ISO_PKT_IRP];

} USBH_AUD_SESSION_CONTEXT;

typedef struct nu_audh_transfer
{
    USBH_AUD_SESSION_CONTEXT iso_irp[NU_AUDH_NUM_PLAY_TRANS];

    NU_USBH_AUD_Data_Callback call_back;

    VOID * p_buffer;

    UINT8 * p_curr_tx_buffer;

    UINT32 buffer_length;

    UINT32 curr_tx_length;

    UINT32 irp_count;
} NU_AUDH_TRANSFER;

/* Audio Device Control Block. */
typedef struct nu_usbh_aud_dev
{
    /* USB Resources required by the driver. */
    CS_NODE               node;              /* Linked list node. */
    NU_USB_DRVR          *pcb_aud_drvr;      /* Pointer to class driver
                                              * control block.
                                              */
    NU_USB_USER          *pcb_user_drvr;     /* Pointer to User control
                                              * block.
                                              */
    NU_USB_STACK         *pcb_stack;         /* Pointer to stack control
                                              * block.
                                              */
    NU_USB_DEVICE        *pcb_device;        /* Pointer to device control
                                              * block.
                                              */
    NU_USB_CFG           *pcb_cfg;           /* Pointer to active
                                              * configuration.
                                              */
    NU_USB_INTF          *pcb_ac_intf;       /* Audio Control Interface. */
    NU_USB_ALT_SETTG     *pcb_ac_alt_settg;  /* Active alternate setting
                                              * for Audio Control
                                              * Interface (ACI).
                                              */
    NU_USB_PIPE          *pcb_iso_out_pipe;  /* Iso OUT pipe for active
                                              * alternate setting.
                                              */
    NU_USB_PIPE          *pcb_iso_in_pipe;   /* Iso IN pipe for active
                                              * alternate setting.
                                              */

    NU_USB_PIPE          *pcb_ctrl_pipe;     /* Control pipe pointer. */
    NU_USBH_CTRL_IRP	 *ctrl_irp;          /* Control transfer IRP.       */

    /* Resources required by the driver. */
    NU_EVENT_GROUP        trans_events;
    NU_EVENT_GROUP        task_events;
    NU_SEMAPHORE          sm_ctrl_trans;

    NU_SEMAPHORE          rec_sem;
    NU_SEMAPHORE          play_sem;

    AUDH_PLAY_INFORM      cb_play_inform;
    AUDH_PLAY_INFORM      cb_record_inform;

    /* Descriptor Parsing related information. */
    VOID                 *cs_descr;          /* Pointer to AC class
                                              * specific descriptors
                                              * present in device.
                                              */

    UINT32                len_cs_descr;      /* Length of class specific
                                              * descriptor present in
                                              * device.
                                              */

    NU_USBH_AUD_CTRL     *ac_cs_info;        /* Pointer to Audio Control
                                              * class specific information.
                                              */
    /* List of all audio streaming interfaces. */
    AUDH_ASI_INFO        *si_info[NU_AUDH_MAX_STREAMING_INFS];

    UINT32                bus_speed;         /* High speed or Full speed
                                              * device.
                                              */
}NU_USBH_AUD_DEV;

/* Audio Host Class  Driver Control Block. */
typedef struct nu_usbh_audio
{
    /* The base class control block. */
    NU_USB_DRVR           cb_drvr;
    NU_USBH_USER         *pcb_user_drvr;
    NU_USBH_AUD_DEV      *pcb_first_device;
    NU_MEMORY_POOL       *memory_pool;
    NU_SEMAPHORE          sm_aud_lock;

} NU_USBH_AUDIO;

/* Holds the class specific ASI information for one alternate
 * setting of ASI.
 */
struct AUDH_ASI_INFO_STRUCT
{

    NU_USB_INTF          *strm_inf;          /* Standard interface. */
    NU_USB_ALT_SETTG     *zbw_alt_sttg;      /* Pointer to zero band width
                                              * alternate setting.
                                              */
    /* Class specific Audio Streaming Interfaces info. */
    AUDH_CS_ASI_INFO     *cs_asi_info[NU_USB_MAX_ALT_SETTINGS];

    NU_USBH_AUD_DEV      *aud_dev_info;      /* Parent data structure
                                              * pointer.
                                              */
};

/* Audio control request structure. */
typedef struct nu_usbh_aud_ctrl_req
{
    UINT8                *trans_buff;        /* Parameter block for all
                                              * requests.
                                              */

    AUDH_CS_REQ_CODE      req;               /* Class specific request
                                              * codes.
                                              */

    UINT16                ctrl_slctr;        /* Represents the Control
                                              * Selector for Terminal,
                                              * Feature, processing and
                                              * extension unit requests.
                                              */
    UINT16                buf_len;           /* Length of parameter block
                                              * buffer.
                                              */
    UINT8                 id;                /* Id of the unit or terminal.
                                              */
    UINT8                 channel;           /* Represents CN in feature
                                              * units req or ICN in mixer
                                              * unit requests.
                                              */

    DATA_ELEMENT          cs_padding[2];

}NU_USBH_AUD_CTRL_REQ;

/* Endpoint request structure. */
typedef struct nu_usbh_aud_ep_req
{

    AUDH_CS_REQ_CODE      req;               /* Class specific request
                                              * codes.
                                              */
    UINT8                *trans_buff;        /* Parameter block for all
                                              * requests.
                                              */
    UINT16                ctrl_slctr;        /* Represents the CS for
                                              * Terminal, Feature,
                                              * processing and extension
                                              * unit requests.
                                              */

    DATA_ELEMENT          cs_padding[2];


}NU_USBH_AUD_EP_REQ;

/* Holds the information about the Type-1 format type descriptor. */
typedef struct aud_type1_descr_info
{

    UINT32               *smpl_freq_list;    /* Supported sampling
                                              * frequencies list.
                                              */
    UINT32                min_smpl_freq;     /* Minimum supported sampling
                                              * frequency.
                                              */
    UINT32                max_smpl_freq;     /* Maximum supported sampling
                                              * frequency.
                                              */
    UINT16                loc_del;           /* Lock delay. */
    UINT8                 smpl_tp;           /* 0:Continuous frequencies
                                              * else:Discrete sampling
                                              * frequencies.
                                              */
    UINT8                 chnls_count;       /* Number of channels in an
                                              * Audio sample.
                                              */
    UINT8                 sub_frm_size;      /* Sample size to be sent for
                                              * each of the channel, in
                                              * bytes.
                                              */
    UINT8                 bit_res;           /* Actual bits used in the sub
                                              * frame.
                                              */
    UINT8                 smpl_freq_count;   /* Discrete sampling
                                              * frequencies count.
                                              */

    DATA_ELEMENT          cs_padding[1];


} AUDH_TYPE1_DSCR_INFO;

/* Audio function information structure. */
typedef struct nu_usb_aud_fnc_info
{
    UINT8                 input_term;        /* Input terminal id. */
    UINT8                 feature_unit;      /* Feature Unit id. */
    UINT8                 output_term;       /* Output terminal id. */
    DATA_ELEMENT          cs_padding[1];

} NU_USBH_AUD_FUNC_INFO;

/* Functions type defines. */
typedef STATUS (*NU_AUDH_UNIT_PARSE_FUNC)(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *ent_info);

typedef STATUS (*NU_AUDH_FMT_PARSE_FUNC)(
           UINT8               *scan,
           AUDH_CS_ASI_INFO    *asi_info,
           NU_USBH_AUD_DEV     *pcb_audio_drvr);

/* Function declarations. */
VOID   NU_AUDH_CTRL_IRP_Complete(
           NU_USB_PIPE         *pcb_pipe,
           NU_USB_IRP          *pcb_irp);

VOID   NU_AUDH_ISO_IN_IRP_Complete (
          NU_USB_PIPE          *iso_pipe,
          NU_USB_IRP           *iso_irp);

VOID NU_AUDH_ISO_OUT_IRP_Complete (
          NU_USB_PIPE          *iso_pipe,
          NU_USB_IRP           *iso_irp);

STATUS NU_AUDH_Parse_AC_Dscr(
           NU_USBH_AUD_DEV     *audio_dev);

STATUS NU_AUDH_Parse_Entities(
           NU_USBH_AUD_DEV     *audio_dev,
           NU_USBH_AUD_CTRL    *ac_info);

STATUS NU_AUDH_Parse_Terminal(
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *ent_info,
           NU_USBH_AUD_DEV     *audio_dev );

STATUS NU_AUDH_Parse_Mixer_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *unit_info);

STATUS NU_AUDH_Parse_Feature_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *unit_info);

STATUS NU_AUDH_Parse_Proc_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *unit_info);

STATUS NU_AUDH_Parse_Ext_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *ent_info);

STATUS NU_AUDH_Parse_Sel_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *ent_info);

STATUS NU_AUDH_Parse_Invalid_Unit(
           NU_USBH_AUD_DEV     *audio_dev,
           UINT8               *cs_descr,
           NU_USBH_AUD_ENTITY  *ent_info);

STATUS NU_AUDH_Parse_ACI_Strm_Infs(
           AUDH_CS_ASI_INFO    *as_info,
           NU_USBH_AUD_DEV     *pcb_audio_dev);

STATUS NU_AUDH_Parse_Type_I_Dscr(
           UINT8               *scan,
           AUDH_CS_ASI_INFO    *asi_info,
           NU_USBH_AUD_DEV     *pcb_audio_dev);

STATUS NU_AUDH_Parse_Type_II_Dscr(
           UINT8               *scan,
           AUDH_CS_ASI_INFO    *asi_info,
           NU_USBH_AUD_DEV     *pcb_audio_drvr);

STATUS NU_AUDH_Parse_Type_III_Dscr(
           UINT8               *scan,
           AUDH_CS_ASI_INFO    *asi_info,
           NU_USBH_AUD_DEV     *pcb_audio_drvr);

STATUS  NU_AUDH_Updt_Vldt_Conn(
          NU_USBH_AUD_DEV      *aud_dev,
          NU_USBH_AUD_CTRL     *ac_info);

STATUS  NU_AUDH_Calc_Succ_Cnt(
          NU_USBH_AUD_CTRL     *ac_info);

STATUS NU_AUDH_Update_Succ(
          NU_USBH_AUD_CTRL     *ac_info,
          UINT8                *curr_id);

STATUS NU_AUDH_Free_Structures(
          NU_USBH_AUD_DEV      *p_curr_device);

STATUS NU_AUDH_Initialize_Device(
          NU_USBH_AUD_DEV      *pcb_aud_device);

STATUS NU_AUDH_Parse_Audio_Strm_Infs(
          AUDH_CS_ASI_INFO     *cs_asi_info,
          NU_USBH_AUD_DEV      *pcb_curr_device);

VOID   NU_AUDH_Play_Audio(
          UNSIGNED              pcb_aud_drvr,
          VOID                 *pcb_aud_device);

VOID   NU_AUDH_Record_Audio(
          UNSIGNED              pcb_aud_drvr,
          VOID                 *pcb_aud_device);
VOID NU_AUDH_Fill_ISO_Buffer(
          NU_USB_ISO_IRP       *irp,
          UINT8               **tx_data,
          UINT32               *tx_len ,
          AUDH_PLAY_INFORM     *play_info,
          UINT16               *trans_len,
          UINT8               **transaction);
VOID   NU_AUDH_Packetize (
          NU_USB_ISO_IRP       *irp,
          UINT8               **tx_data,
          UINT32               *tx_len,
          UINT16                maxp,
          UINT16               *trans_len_array,
          UINT8               **trans_ptr_array);

STATUS NU_AUDH_Set_Events(NU_USBH_AUD_DEV * pcb_audio_device,
                          UINT32            mask);

STATUS NU_AUDH_Get_Events(NU_USBH_AUD_DEV * pcb_audio_device,
                          UINT32            mask,
                          UINT32            suspend);

VOID   NU_AUDH_Record_Single_Buffer(UNSIGNED  pcb_aud_drvr,
                                          VOID     *pcb_device);

/* ===================================================================== */

#include "nu_usbh_audio_dat.h"

/* ===================================================================== */

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif
