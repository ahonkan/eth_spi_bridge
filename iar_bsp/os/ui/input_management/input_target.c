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
* input_target.c                                                       
*
*
* DESCRIPTION
*
* The input target functions for Nucleus GRAFIX.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
* INPUTT_InitInputDevTable
*
* DEPENDENCIES
*
*  plus/nucleus.h                                        
*  grafixrs/inc/rsconst.h
*  grafixrs/inc/rs_base.h
*  drivers/display/display_config.h                
*  input_management/input_config.h                
*  input_management/input_target.h
*
***************************************************************************/

#include "ui/input_config.h"


/* THIS should be used for any input management for a target */
#include "nucleus.h"

#include "ui/rsconst.h"
#include "ui/rs_base.h"

#include "drivers/display_config.h"

#include "ui/input_target.h"

#include "ui/eventhandler.h"
#include "ui/pointing_device.h"
#include "ui/trackcur.h"

SIGNED inputDeviceTable[16];

extern volatile DEFN q_Size;         

/***************************************************************************
* FUNCTION
*
*    INPUTT_InitInputDevTable
*
* DESCRIPTION
*
*    Initializes the input device table with the input drivers.
*    Initialize bank manager structures.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None. 
*
****************************************************************************/
VOID INPUTT_InitInputDevTable(VOID)
{
    INT32 i = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* For System or USB Mouse */
#if defined(CFG_NU_BSP_DRVR_MOUSE_ENABLE) || defined(CFG_NU_OS_DRVR_USB_HOST_USB_INPUT_ENABLE)
     /* Initialize Input Device Managers - [] Table */
     inputDeviceTable[i] = cMSDRIVER;      
     i++;

#ifdef CFG_NU_BSP_DRVR_MOUSE_ENABLE
     inputDeviceTable[i] = (SIGNED) MOUSET_Device_Mgr;
#else
     inputDeviceTable[i] = (SIGNED) USB_Mouse_Wakeup;
#endif
     i++;

#endif /* CFG_NU_BSP_DRVR_MOUSE_ENABLE || CFG_NU_OS_DRVR_USB_HOST_USB_INPUT_ENABLE */    

     /* For Keypad */
#ifdef  CFG_NU_OS_DRVR_KEYPAD_ENABLE

     /* Initialize Input Device Managers - [] Table */
     inputDeviceTable[i] = cKEYPAD;
     i++;

     inputDeviceTable[i] = (SIGNED) KP_Device_Mgr;
     i++;
     
#endif  /* CFG_NU_OS_DRVR_KEYPAD_ENABLE */

    /* For Touch Panel */
#ifdef  CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE
	     
     inputDeviceTable[i] = cTOUCH;
     i++;
     inputDeviceTable[i] = (SIGNED) TP_Device_Mgr;  
     i++;
	 
#endif  /* CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE */

     if( i == 0)
     {
         inputDeviceTable[i] = cMSDRIVER;      
         i++;

         /* This would have to be filled in if there is an 
            initialization function that is related to the
            the mouse */
         inputDeviceTable[i] = (SIGNED) BankStub;       
         i++;
     }

    /* Last entry of table must be 0xffff */
    /* set end value */
    inputDeviceTable[i] = -1;  

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    nu_os_drvr_input_init
*
* DESCRIPTION
*
*    Initializes Input Management.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None. 
*
****************************************************************************/
STATUS nu_os_ui_input_mgmt_init(const CHAR * key, INT compctrl)
{
    STATUS status = NU_SUCCESS;

    if (compctrl == RUNLEVEL_START)
	{
	    q_Size = 50;        /* event queue size in elements */
	    
        /* Set default event mask */
        EVENTH_MaskEvent(mPRESS + mREL + mKEYDN + mPOS);
	    
		INPUTT_InitInputDevTable();
		
		EVENTH_EventQueue(NU_TRUE);

        /* If there is an input device included, initialize it here */

#ifndef CYCLE
#ifdef  INCLUDE_INPUT_DEVICE

#if     defined(CFG_NU_BSP_DRVR_MOUSE_ENABLE) || defined(CFG_NU_OS_DRVR_USB_HOST_USB_INPUT_ENABLE)

        /* Initialize the mouse driver */
        status = PD_InitInputDevice(cMSDRIVER);

#endif  /* CFG_NU_BSP_DRVR_MOUSE_ENABLE || CFG_NU_OS_DRVR_USB_HOST_USB_INPUT_ENABLE */

#ifdef  CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE

#ifndef NU_SIMULATION

	    /* Initialize the touch panel driver */
		if(status == NU_SUCCESS)
		{
    		status = PD_InitInputDevice(cTOUCH);
		}
		
#if 	(CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE)

		if(status == NU_SUCCESS)
		{
	    	/* If the touch panel driver is present, calibrate it here */
		    TP_Calibrate();
		}

#endif  /* CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE */
		
		
#endif  /* NU_SIMULATION */

#endif  /* CFG_NU_OS_DRVR_TOUCHPANEL_ENABLE */

#ifdef  CFG_NU_OS_DRVR_KEYPAD_ENABLE

	    /* Initialize the keypad driver */
    	if(status == NU_SUCCESS)
		{
    		status = PD_InitInputDevice(cKEYPAD);
		}
		
#endif  /* CFG_NU_OS_DRVR_KEYPAD_ENABLE */

#endif	/* INCLUDE_INPUT_DEVICE */

#endif  /* CYCLE */

#ifdef  USE_CURSOR

		/* Turn on cursor tracking */
	   	TC_TrackCursor(NU_TRUE);
	   	
#endif  /* USE_CURSOR */

	}
	
	return (status);
	
}

#ifndef CFG_NU_OS_DRVR_DISPLAY_ENABLE

INT32 BankStub(VOID)
{
    return(0);
}

VOID   SCREENS_InitRowTable( grafMap *argBitMap, INT32 argInrLve, INT32 argInrSeg, INT32 argInrSize)
{
    NU_UNUSED_PARAM(argInrLve);
    NU_UNUSED_PARAM(argInrSeg);
    NU_UNUSED_PARAM(argInrSize);
    NU_UNUSED_PARAM(argBitMap);
}

INT32  SCREENI_InitBitmap(INT32 argDEVICE, grafMap *argGRAFMAP)
{
    NU_UNUSED_PARAM(argDEVICE);
    NU_UNUSED_PARAM(argGRAFMAP);

    return 0;
}

INT32  SCREENS_CloseGrafDriver(grafMap *argGRAFMAP)
{
    NU_UNUSED_PARAM(argGRAFMAP);

    return 0;
}

VOID   SCREENS_InitBankManager(grafMap *argGRAFMAP)
{
    NU_UNUSED_PARAM(argGRAFMAP);
}

VOID   BLITS_RestoreGlobals(VOID)
{
    return;
}

VOID   BLITS_SaveGlobals(VOID)
{
    return;
}

STATUS CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR )
{
    NU_UNUSED_PARAM(srcPORT);
    NU_UNUSED_PARAM(dstPORT);
    NU_UNUSED_PARAM(argSrcR);
    NU_UNUSED_PARAM(argDstR);

    return 0;
}

#endif

