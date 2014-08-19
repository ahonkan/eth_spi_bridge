/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*  DESCRIPTION
*
*       This file contains services constants common to both the
*       application and the actual Nucleus Services.
*
***********************************************************************/

#ifndef NU_UI_H
#define NU_UI_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Include auto-generated configuration file */
#include "nucleus_gen_cfg.h"

/* Include required display driver file */
#include "drivers/display_config.h"

#ifdef CFG_NU_OS_UI_GRAFIXRS_ENABLE
#include "ui/rs_app.h"
#include "ui/coords.h"
#include "ui/rserrors.h"
#include "ui/globalrsv.h"
#include "ui/devc.h"
#endif /* CFG_NU_OS_UI_GRAFIXRS_ENABLE */

#ifdef CFG_NU_OS_UI_IMAGE_ENABLE
#include  "ui/jpeglib.h"
#endif /* CFG_NU_OS_UI_IMAGE_ENABLE */

#ifdef CFG_NU_OS_UI_IFX_ENABLE
#include "ui/ifxui_defs.h"
#include "ui/ifxui_rtl.h"
#include "ui/ifxui_uriparser.h"
#include "ui/ifxui_integration.h"
#include "ui/ifxui_engine.h"
#include "ui/ifxui_module_integration.h"
#endif /* CFG_NU_OS_UI_IFX_ENABLE */

#ifdef CFG_NU_OS_UI_INPUT_MGMT_ENABLE
#include "ui/trackcur.h"
#endif /* CFG_NU_OS_UI_INPUT_MGMT_ENABLE */

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* NU_UI_H */
