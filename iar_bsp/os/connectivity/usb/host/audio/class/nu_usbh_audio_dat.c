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
* FILE NAME
*     nu_usbh_audio_dat.c
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file initializes the dispatch table for host AUDIO class driver.
*     Some of those uses functionality of NU_USB_DRVR basic implementation,
*     others are either extended or ignored. It also initializes other data
*     structures needed for host AUDIO class driver internal implementation.
*
* DATA STRUCTURES
*     usbh_audio_dispatch        AUDIO Class Driver dispatch table.
*     NU_AUDH_Unit_Parse_Fptr    Array of function pointers for parsing
*                                unit specific information.
*     NU_AUDH_Fmt_Parse_Fptr     Array of function pointers for parsing
*                                format type descriptors.
*     NU_Input_Pins_Count_Offset Array of input pins indexed by unit type.
*     NU_Src_Id_Offset           Array of SourceId field offset. This field
*                                is present in Units and Output terminals.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     nu_usb.h                   All USB definitions.
*
**************************************************************************/
/* USB Include Files */
#include "connectivity/nu_usb.h"
#include "nu_usbh_audio_ext.h"

/* Global pointer for the class driver. */
NU_USBH_AUDIO *NU_USBH_Audio_Cb_Pt;

/* Creating a global instance for audio driver's dispatch table which is
 * just of the type of USB driver dispatch table.
 */
const NU_USB_DRVR_DISPATCH usbh_audio_dispatch =
{
    {
        _NU_USBH_AUD_Delete,
        _NU_USB_Get_Name,
        _NU_USB_Get_Object_Id
    },
    _NU_USB_DRVR_Examine_Intf,
    NU_NULL,
    _NU_USB_DRVR_Get_Score,
    NU_NULL,
    _NU_USBH_AUD_Initialize_Intf,
    _NU_USBH_AUD_Disconnect
};

/* Array of function pointers for parsing the Units specific info. Array is
 * indexed by the Unit type.
 */
NU_AUDH_UNIT_PARSE_FUNC NU_AUDH_Unit_Parse_Fptr[] =
{
    NU_AUDH_Parse_Invalid_Unit,
    NU_AUDH_Parse_Invalid_Unit,
    NU_AUDH_Parse_Invalid_Unit,
    NU_AUDH_Parse_Invalid_Unit,
    NU_AUDH_Parse_Mixer_Unit,
    NU_AUDH_Parse_Sel_Unit,
    NU_AUDH_Parse_Feature_Unit,
    NU_AUDH_Parse_Proc_Unit,
    NU_AUDH_Parse_Ext_Unit
};

/* Array of function pointers for parsing the format type descriptors.
 * This array is indexed by the format type group code.
 */
NU_AUDH_FMT_PARSE_FUNC NU_AUDH_Fmt_Parse_Fptr[] =
{
    NU_AUDH_Parse_Type_I_Dscr,
    NU_AUDH_Parse_Type_II_Dscr,
    NU_AUDH_Parse_Type_III_Dscr,
};

/* This array contains the offset for the input pins count field in the the
 * Unit type descriptors. This array is indexed by the unit type.
 */
INT NU_Input_Pins_Count_Offset[] =
{
    NU_AUDH_INVALID_PIN_OFSET,
    NU_AUDH_INVALID_PIN_OFSET,
    NU_AUDH_INVALID_PIN_OFSET,
    NU_AUDH_INVALID_PIN_OFSET,
    NU_AUDH_MIXER_IP_PINS_CNT_OFSET,
    NU_AUDH_SEL_NO_IP_PINS_OFSET,
    NU_AUDH_INVALID_PIN_OFSET,
    NU_AUDH_PROC_NO_IP_PINS_OFSET,
    NU_AUDH_EXT_NO_IP_PINS_OFSET
};

/* This array contains the offsets of sourceId field defined in different
 * unit and output terminals descriptors. This array is indexed by entity
 * type.
 */
INT NU_Src_Id_Offset[] =
{
    NU_AUDH_INVALID_SRC_ID_OFSET,
    NU_AUDH_INVALID_SRC_ID_OFSET,
    NU_AUDH_INVALID_SRC_ID_OFSET,
    NU_AUDH_OP_TERM_SRCID_OFSET,
    NU_AUDH_MIXER_SRC_ID_OFSET,
    NU_AUDH_SEL_SRC_ID_OFSET,
    NU_AUDH_FEATURE_SRC_ID_OFSET,
    NU_AUDH_PROC_SRC_ID_OFSET,
    NU_AUDH_EXT_SRC_ID_OFSET
};

/************************* end of file ***********************************/
