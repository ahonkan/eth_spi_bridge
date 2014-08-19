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
*     nu_usbh_audio_user_imp.c                                         
*
* COMPONENT
*     Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*     This file contains internal routines for user driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     NU_AUDH_USER_Get_FU_Id           Return Feature unit Id. 
*
* DEPENDENCIES
*     nu_usb.h                         All USB definitions.
*
**************************************************************************/

/* ======================  USB Include Files =========================== */
#include "connectivity/nu_usb.h"
#include "nu_usbh_audio_user_imp.h"

/**************************************************************************
* FUNCTION
*     NU_AUDH_USER_Get_FU_Id
*
* DESCRIPTION
*     This function opens the device in sound play mode. if there exist a 
*     streaming input terminal attached with output terminal of speaker type,
*     the device can be opened in play mode.
*
* INPUTS
*     aud_usr_dev         Pointer to audio user device.
*     fu_id               Pointer to the Feature Unit Id.
*     function            Speaker function or Microphone function.
*  
* OUTPUTS
*     None.
*
**************************************************************************/
VOID NU_AUDH_USER_Get_FU_Id(NU_USBH_AUD_USER_DEV  *aud_usr_dev,
                            UINT8                 *fu_id,
                            UINT8                  function )
{

    /* Get feature unit id if function is speaker. */
    if(function == NU_AUDH_SPKR_FUNCTION)
    {
        *fu_id = aud_usr_dev->speaker.feature_unit;
    }

    /* Get feature unit id if function is microphone. */
    if(function == NU_AUDH_MIC_FUNCTION)
    {
        *fu_id = aud_usr_dev->mphone.feature_unit;
    }

}

/* ======================  End Of File  ===============================. */
