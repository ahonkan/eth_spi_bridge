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
*  portops_portpattern.c                                                    
*
* DESCRIPTION
*
*  Port operation function - PortPattern.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PortPattern
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  portops.h
*  fonti.h
*  rs_api.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/portops.h"
#include "ui/fonti.h"
#include "ui/rs_api.h"

/***************************************************************************
* FUNCTION
*
*    PortPattern
*
* DESCRIPTION
*
*    Function PortPattern sets the "portPat" field of the current grafPort
*    to the users pattern list structure.
*
* INPUTS
*
*    patList *argPLIST - Pointer to the pattern list structure.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PortPattern(patList *argPLIST)
{
    
#ifdef  FILL_PATTERNS_SUPPORT
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set shadow port */
    theGrafPort.portPat  = argPLIST;

    /* set default blitRcd */
    grafBlit.blitPatl = argPLIST;

    /* set user port */
    thePort->portPat  = argPLIST;

    /* Return to user mode */
    NU_USER_MODE();

#else

    NU_UNUSED_PARAM(argPLIST);
    
#endif /* FILL_PATTERNS_SUPPORT */

}
