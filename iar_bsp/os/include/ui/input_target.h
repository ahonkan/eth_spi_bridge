/***************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.                       
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*                                                                       
****************************************************************************

****************************************************************************
*
* FILE NAME                                                               
*
*  input_target.h                                                      
*
*
* DESCRIPTION
*
*  The input target externs and defines for Nucleus GRAFIX.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _INPUT_TARGET_H_
#define _INPUT_TARGET_H_

#include "nucleus_gen_cfg.h"
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* Prototypes needed for Input Management */

#ifdef  CFG_NU_OS_DRVR_KEYPAD_ENABLE

extern 	INT32 KP_Device_Mgr(mouseRcd *rcd, short md);

#endif  /* CFG_NU_OS_DRVR_KEYPAD_ENABLE */

#ifdef  CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE

extern 	INT32 TP_Device_Mgr(VOID *rcd, INT32 md);

#if     (CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE)

extern  VOID  TP_Calibrate(VOID);

#endif  /* CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE */

#endif  /* CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE */

#ifndef  CFG_NU_OS_DRVR_DISPLAY_ENABLE

INT32   BankStub(VOID);
VOID    SCREENS_InitRowTable( grafMap *argBitMap, INT32 argInrLve, INT32 argInrSeg, INT32 argInrSize);
INT32   SCREENI_InitBitmap(INT32 argDEVICE, grafMap *argGRAFMAP);
INT32   SCREENS_CloseGrafDriver(grafMap *argGRAFMAP);
VOID    SCREENS_InitBankManager(grafMap *argGRAFMAP);
VOID    BLITS_RestoreGlobals(VOID);
VOID    BLITS_SaveGlobals(VOID);
STATUS  CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR );

#else

extern  INT32 BankStub(VOID);

#endif

#ifdef CFG_NU_BSP_DRVR_MOUSE_ENABLE
extern INT32 MOUSET_Device_Mgr(mouseRcd *rcd, INT16 md);
#elif defined(CFG_NU_OS_DRVR_USB_HOST_USB_INPUT_ENABLE)
extern INT32 USB_Mouse_Wakeup(mouseRcd *rcd, INT32 md);
#endif /* CFG_NU_BSP_DRVR_MOUSE_PL050_KMI1_ENABLE */

#endif /* _INPUT_TARGET_H_ */

