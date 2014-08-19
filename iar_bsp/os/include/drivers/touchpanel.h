/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       touchpanel.h
*
*   COMPONENT
*
*       Touchpanel driver
*
*   DESCRIPTION
*
*       This file contains prototypes and externs for touchpanel.c
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus_gen_cfg.h
*       nucleus.h
*
**************************************************************************/
#ifndef     _TOUCHPANEL_H_
#define     _TOUCHPANEL_H_

#include "nucleus_gen_cfg.h"
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"

/* Maximum number of Touchpanel sessions/instances */
#define TOUCHPANEL_MAX_INSTANCES          1
#define TOUCHPANEL_MAX_SESSIONS          (1 * TOUCHPANEL_MAX_INSTANCES)


/* Touch panel screen data for device to screen conversion. */
typedef struct _touchpanel_screen_data_struct_
{

    unsigned char   tp_press_state;         /* pressed state */
    short           tp_x_raw;               /* raw x position */
    short           tp_y_raw;               /* raw Y position */
    short           tp_x_slope;             /* X Slope */
    short           tp_y_slope;             /* Y Slope */
    short           tp_x_intercept;         /* X Intercept */
    short           tp_y_intercept;         /* Y Intercept */

} TP_SCREEN_DATA;

/* Structure for Touch panel callback routines to be implemented by the user. */
typedef struct _touchpanel_callback_struct_
{
    VOID        (*tp_x_coordinate_adjustment)(INT16 *xpos);
    VOID        (*tp_y_coordinate_adjustment)(INT16 *ypos);

} TOUCHPANEL_CALLBACKS;

/* Touch panel driver functions */

STATUS      Touchpanel_Get_Target_Info(const CHAR * key, TOUCHPANEL_INSTANCE_HANDLE *inst_info);
STATUS      Touchpanel_Init(TOUCHPANEL_INSTANCE_HANDLE *inst_handle, const CHAR *key);
INT32       TP_Device_Mgr(VOID *rcd, INT32 md);
INT32       TP_Open(VOID *rcd);
VOID        TP_Close(VOID);
INT32       TP_Pos(VOID *rcd);
VOID        TP_Encode(VOID);
VOID        TP_Error_Handler(STATUS error);
VOID        TP_Calibrate(VOID);
VOID        TP_Register_Callbacks(TOUCHPANEL_CALLBACKS *tp_cb);

/* Macro to suppress unused parameter/variable warnings. */
#define     TP_UNUSED_PARAMETER(x)      (VOID)x

#define     TOUCHPANEL_LABEL  {0x60,0x6B,0xC5,0x06,0x8A,0xD0,0x4f,0x50,0xB4,0x98,0xA0,0xEF,0x2B,0x6C,0xCC,0x96}

#endif      /* _TOUCHPANEL_H_ */
