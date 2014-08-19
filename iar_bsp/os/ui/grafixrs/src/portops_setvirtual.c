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
*  portops_setvirtual.c                                                    
*
* DESCRIPTION
*
*  Port operation function - SetVirtual.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  SetVirtual
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
*    SetVirtual
*
* DESCRIPTION
*
*    Function SetVirtual sets the current port for processing virtual
*    port coordinates.  SetVirtual assumes VirtualRect has been called
*    previously to define the ports virtual rectangle.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetVirtual(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set user port flags */
    thePort->portFlags |= pfVirtual;

    /* set shadow port flags */
    theGrafPort.portFlags |= pfVirtual;

    /* update coord xform constants */
    COORDS_rsGblCoord(); 

    /* Return to user mode */
    NU_USER_MODE();
}
