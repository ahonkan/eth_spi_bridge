/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       rs_exp.c
*
*   DESCRIPTION
*
*       Export symbols for Grafix RS.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_UI_GRAFIXRS_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/proc_extern.h"
#include "ui/nu_ui.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_UI_GRAFIXRS);

/* Export task control functions. */
NU_EXPORT_SYMBOL (BackColor);
NU_EXPORT_SYMBOL (RS_Polygon_Draw);
NU_EXPORT_SYMBOL (SetPixel);
NU_EXPORT_SYMBOL (RS_Oval_Draw);
NU_EXPORT_SYMBOL (MoveTo);
NU_EXPORT_SYMBOL (SetFont);
NU_EXPORT_SYMBOL (RS_Arc_Draw);
NU_EXPORT_SYMBOL (ScreenRect);
NU_EXPORT_SYMBOL (STR_str_cpy);
NU_EXPORT_SYMBOL (LineTo);
NU_EXPORT_SYMBOL (RS_Reset_Pen);
NU_EXPORT_SYMBOL (RS_Text_Setup);
NU_EXPORT_SYMBOL (SetRect);
NU_EXPORT_SYMBOL (RS_Get_Pen_Setup);
NU_EXPORT_SYMBOL (RS_Rectangle_Draw);
NU_EXPORT_SYMBOL (RS_Reset_Text);
NU_EXPORT_SYMBOL (RS_Text_Draw);
NU_EXPORT_SYMBOL (GetPort);
NU_EXPORT_SYMBOL (RS_Get_Text_Setup);
NU_EXPORT_SYMBOL (RS_Pen_Setup);
NU_EXPORT_SYMBOL (STD_l_toa);

#ifdef CFG_NU_OS_UI_INPUT_MGMT_ENABLE
NU_EXPORT_SYMBOL (EVENTH_KeyEvent);
#endif

#endif /* CFG_NU_OS_UI_GRAFIXRS_EXPORT_SYMBOLS == NU_TRUE */
