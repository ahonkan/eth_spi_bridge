/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  setaplha.c                                                   
*
* DESCRIPTION
*
*  Contains the function that sets global alpha value.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Set_Alpha
*
* DEPENDENCIES
*
*  rs_base.h
*  devc.h
*  colorops.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/devc.h"
#include "ui/colorops.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*   RS_Set_Alpha
*
* DESCRIPTION
*
*    Sets the transparency level between 0 being solid
*    to 1 being totally transparent
*
* INPUTS
*
*    trans_level              Percentage of transparency
*    set_text_trans           Flag for text transparency
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RS_Set_Alpha(double trans_level, BOOLEAN set_text_trans)
{

#ifdef  GLOBAL_ALPHA_SUPPORT
        
	NU_SUPERV_USER_VARIABLES

	/* Switch to supervisor mode */
	NU_SUPERVISOR_MODE();

	text_trans = set_text_trans;
	
	alpha_level = trans_level;

	/* Return to user mode */
	NU_USER_MODE();

        
#else

    NU_UNUSED_PARAM(trans_level);
    NU_UNUSED_PARAM(set_text_trans);

#endif  /* GLOBAL_ALPHA_SUPPORT */

}
