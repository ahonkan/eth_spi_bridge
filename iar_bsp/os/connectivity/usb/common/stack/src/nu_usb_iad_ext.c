/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_iad_ext.c
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*        This file contains implementation of NU_USB_IAD services.
*
*
* DATA STRUCTURES
*       None.
*
* FUNCTIONS
*    NU_USB_IAD_Check_Interface       - Check to see if the interface
*                                       belongs to the IAD.
*    NU_USB_IAD_Get_First_Interface   - Get the first interface belongs
*                                       to the IAD.
*                                       endpoint.
*    NU_USB_IAD_Get_Last_Interface    - Get the last interface belongs
*                                       to the IAD.
*    NU_USB_IAD_Get_Desc              - Get the IAD descriptor.
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_IAD_EXT_C
#define	USB_IAD_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*       NU_USB_IAD_Check_Interface
*
* DESCRIPTION
*       This function checks if the interface ID belong to this IAD.
*
* INPUTS
*       cb                      pointer to ISO IRP control block.
*       intf_num                Interface number to be checked
*
* OUTPUTS
*       NU_SUCCESS              Indicates interface belong to the IAD.
*       NU_USB_INVLD_ARG        Indicates interface not belong to the IAD.
*
*************************************************************************/
STATUS NU_USB_IAD_Check_Interface(NU_USB_IAD *cb,
                                  UINT8 intf_num)
{
    NU_USB_PTRCHK(cb);

    if ((intf_num >= cb->first_intf) &&
        (intf_num >= cb->last_intf))
        return (NU_SUCCESS);

    return NU_USB_INVLD_ARG;

}

/*************************************************************************
* FUNCTION
*       NU_USB_IAD_Get_First_Interface
*
* DESCRIPTION
*       Get the first interface ID belong to this group.
*
* INPUTS
*       cb                      pointer to IAD control block.
*       intf_num_out         Pointer to variable to hold the
*                               returned value of the First Interface
*                               number of this IAD
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IAD_Get_First_Interface(NU_USB_IAD *cb,
                             UINT8 *intf_num_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf_num_out);

    *intf_num_out = cb->first_intf;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*       NU_USB_IAD_Get_Last_Interface
*
* DESCRIPTION
*       Get the last interface ID belong to this group.
*
* INPUTS
*       cb                      pointer to IAD control block.
*       intf_num_out         Pointer to variable to hold the
*                               returned value of the last Interface
*                               number of this IAD
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IAD_Get_Last_Interface(NU_USB_IAD *cb,
                             UINT8 *intf_num_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf_num_out);

    *intf_num_out = cb->last_intf;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*       NU_USB_IAD_Get_Desc
*
* DESCRIPTION
*       Get the IAD descriptor.
*
* INPUTS
*       cb                      pointer to IAD control block.
*       desc_out                Pointer to variable to hold the
*                               returned pointer of the this IAD
*                               descriptor
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IAD_Get_Desc(NU_USB_IAD *cb,
                           NU_USB_IAD_DESC **desc_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(desc_out);

    *desc_out = cb->iad_desc;
    return (NU_SUCCESS);

}

/*************************************************************************/

#endif /* USB_IAD_EXT_C */
/*************************** end of file ********************************/
