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
*   FILE NAME                                               
*
*       nu_usb_bos_ext.c
*
*   COMPONENT
*
*       Nucleus USB Base Stack
*
*   DESCRIPTION
*
*       This file contains implementation of NU_USB_BOS services.
*
*   DATA STRUCTURES
*
*
*   FUNCTIONS
*
*       NU_USB_BOS_Get_Num_DevCap
*                       - User can call this API to get the number of
*                         device capability descriptors in a Binary Device
*                         Object Store (BOS).
*
*       NU_USB_BOS_Get_Total_Length
*                       - User can call this API to get total length of
*                         Binary Device Object Store (BOS) descriptor. This
*                         includes length of BOS descriptor and all of it’s
*                         sub descriptors (device capability descriptors).
*
*       NU_USB_DEVCAP_USB2Ext_Get_LPM
*                       - User can call this API to get value of LPM bit in
*                         bmAttribute field of USB 2.0 extension device
*                         capability descriptor.
*
*       NU_USB_DEVCAP_SuprSpd_Get_LTM
*                       - User can call this API to get value of LTM bit in
*                         bmAttribute field of Super Speed device
*                         capability descriptor.
*
*       NU_USB_DEVCAP_SuprSpd_Get_LS
*                       - User can call this API to check if Low Speed is
*                         supported by a USB 3.0 device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_FS
*                       - User can call this API to check if Full Speed is
*                         supported by a USB 3.0 device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_HS
*                       - User can call this API to check if High Speed is
*                         supported by a USB 3.0 device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_SS
*                       - User can call this API to check if Super Speed is
*                         supported by a USB 3.0 device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_Functionality
*                       - User can call this API to get the lowest speed at
*                         which functionality is supported by this USB 3.0
*                         device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat
*                       - User can call this API to get U1 exit latency
*                         field supported by this USB 3.0 device.
*
*       NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat
*                       - User can call this API to get U2 exit latency
*                         field supported by this USB 3.0 device.
*
*       NU_USB_DEVCAP_CntnrID_Get_CID
*                       - User can call this API to get Container ID field
*                         of container ID device capability descriptor
*                         supported by this USB 3.0 device.
*
*   DEPENDENCIES
*
*       nu_usb.h        All USB definitions.
*
************************************************************************/

#include "connectivity/nu_usb.h"

/*Only include this file if stack is configured for Super Speed USB 
  (USB 3.0). */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_BOS_Get_Num_DevCap
*
*   DESCRIPTION
*
*       User can call this API to get the number of device capability
*       descriptors in a Binary Device Object Store (BOS).
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block
*
*       num_devcap_out  - Pointer to UINT8. This contains bNumDeviceCaps
*                         field of BOS descriptor when function returns.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘num_devcap_out’ contains the total count of
*                         device capability descriptors in BOS pointed to
*                         by ‘cb’.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates BOS descriptor is not present.
*
*************************************************************************/
STATUS NU_USB_BOS_Get_Num_DevCap (NU_USB_BOS *cb, UINT8 *num_devcap_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(num_devcap_out);
    
    /* If BOS Descriptor is valid. */
    if ( cb->bos_desc != NU_NULL )
    {
        *num_devcap_out = cb->bos_desc->bNumDeviceCaps;
        status          = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_BOS_Get_Total_Length
*
*   DESCRIPTION
*
*       User can call this API to get total length of Binary Device Object
*       Store (BOS) descriptor. This includes length of BOS descriptor and
*       all of its sub descriptors (device capability descriptors).
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       totallength_out - Pointer to UINT16. This contains wTotalLength
*                         field of BOS descriptor when function returns.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                        ‘totallength_out’ contains the total length of BOS
*                         pointed to by ‘cb’.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates BOS descriptor is not present.
*
*************************************************************************/
STATUS NU_USB_BOS_Get_Total_Length (NU_USB_BOS *cb,
                                    UINT16     *totallength_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(totallength_out);
    
    /* If BOS Descriptor is valid. */
    if ( cb->bos_desc != NU_NULL )
    {
        *totallength_out    = HOST_2_LE16(cb->bos_desc->wTotalLength);
        status              = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_USB2Ext_Get_LPM
*
*   DESCRIPTION
*
*       User can call this API to get value of LPM bit in bmAttribute field
*       of USB 2.0 extension device capability descriptor.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       lpm_out         - Pointer to BOOLEAN. This contains value of LPM
*                         bit in bmAttribute field of USB 2.0 extension
*                         descriptor when function returns.
*                         NU_TRUE:  if LPM bit is set.
*                         NU_FALSE: if LPM bit is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘lpm_out’ contains value of LPM bit of
*                         bmAttribute field of USB 2.0 extension device
*                         capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates USB 2.0 Extension device capability
*                         descriptor is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_USB2Ext_Get_LPM (NU_USB_BOS   *cb,
                                      BOOLEAN      *lpm_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(lpm_out);

    /* Initialize status variable. */
    status = NU_NOT_PRESENT;

    /* If USB 2.0 extension device capability descriptor is valid. */
    if ( cb->devcap_usb2ext_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit1 of bmAttributes fields of USB 2.0 Extension device
           capability descriptor set? */
        if ( (cb->devcap_usb2ext_desc->bmAttributes & LPM_MASK) ==
                                                             LPM_MASK )
        {
            *lpm_out = NU_TRUE;
        }
        else
        {
            *lpm_out = NU_FALSE;
        }
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_LTM
*
*   DESCRIPTION
*
*       User can call this API to get value of LTM bit in bmAttribute field
*       of Super Speed device capability descriptor.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       ltm_out         - Pointer to BOOLEAN. This contains value of LTM
*                         bit in bmAttribute field of Super Speed
*                         descriptor when function returns.
*                         NU_TRUE:  if LTM bit is set.
*                         NU_FALSE: if LTM bit is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘ltm_out’ contains value of LTM bit of
*                         bmAttribute field of Super Speed device
*                         capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_LTM (NU_USB_BOS   *cb,
                                    BOOLEAN    *ltm_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(ltm_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit1 of bmAttributes fields of Super Speed device
           capability descriptor set? */
        if ((cb->devcap_ss_desc->bmAttributes & LTM_MASK) == LTM_MASK)
        {
            *ltm_out = NU_TRUE;
        }
        else
        {
            *ltm_out = NU_FALSE;
        }
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_LS
*
*   DESCRIPTION
*
*       User can call this API to check if Low Speed is supported by a
*       USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       ls_out          - Pointer to BOOLEAN. This contains value of Bit 0
*                         (Low Speed Supported) in wSpeedsSupported field
*                         of Super Speed device capability descriptor when
*                         function returns.
*                         NU_TRUE:  if Bit 0 is set.
*                         NU_FALSE: if Bit 0 is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘ls_out’ contains value of Bit 0 of
*                         wSpeedsSupported field of Super device capability
*                         descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_LS (NU_USB_BOS *cb,
                                     BOOLEAN    *ls_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(ls_out);

    /* Initialize status variable. */
    status = NU_NOT_PRESENT;

    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit0 of wSpeedsSupported  fields of Super Speed device
           capability descriptor set? */
        if ( (HOST_2_LE16(cb->devcap_ss_desc->wSpeedsSupported) & LS_MASK) ==
                                                             LS_MASK )
        {
            *ls_out = NU_TRUE;
        }
        else
        {
            *ls_out = NU_FALSE;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_FS
*
*   DESCRIPTION
*
*       User can call this API to check if Full Speed is supported by a
*       USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       fs_out          - Pointer to BOOLEAN. This contains value of Bit 1
*                         in wSpeedsSupported field of Super Speed device
*                         capability descriptor when function returns.
*                         NU_TRUE:  if Bit 1 is set.
*                         NU_FALSE: if Bit 1 is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘fs_out’ contains value of Bit 1 (Full Speed
*                         supported) of wSpeedsSupported field of Super
*                         Speed device capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_FS (NU_USB_BOS *cb,
                                     BOOLEAN    *fs_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fs_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit1 of wSpeedsSupported  fields of Super Speed device
           capability descriptor set? */
        if ( (HOST_2_LE16(cb->devcap_ss_desc->wSpeedsSupported) & FS_MASK) ==
                                                             FS_MASK )
        {
            *fs_out = NU_TRUE;
        }
        else
        {
            *fs_out = NU_FALSE;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_HS
*
*   DESCRIPTION
*
*       User can call this API to check if High Speed is supported by a
*       USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       hs_out          - Pointer to BOOLEAN. This contains value of Bit 2
*                         (High Speed supported) in wSpeedsSupported field
*                         of Super Speed device capability descriptor when
*                         the function returns.
*                         NU_TRUE:  if Bit 2 is set.
*                         NU_FALSE: if Bit 2 is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘hs_out’ contains value of Bit 2 of
*                         wSpeedsSupported field of Super Speed device
*                         capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_HS (NU_USB_BOS *cb,
                                     BOOLEAN    *hs_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hs_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit2 of wSpeedsSupported  fields of Super Speed device
         * capability descriptor set.
		 */
        if ( (HOST_2_LE16(cb->devcap_ss_desc->wSpeedsSupported) & HS_MASK) ==
                                                             HS_MASK )
        {
            *hs_out = NU_TRUE;
        }
        else
        {
            *hs_out = NU_FALSE;
        }
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_SS
*
*   DESCRIPTION
*
*       User can call this API to check if Super Speed is supported by a
*       USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       ss_out          - Pointer to BOOLEAN. This contains value of Bit 3
*                         (Super Speed supported) in wSpeedsSupported field
*                         of Super Speed device capability descriptor when
*                         function returns.
*                         NU_TRUE:  if Bit 3 is set.
*                         NU_FALSE: if Bit 3 is not set.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘ss_out’ contains value of Bit 3 of
*                         wSpeedsSupported field of Super Speed device
*                         capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_SS (NU_USB_BOS *cb,
                                     BOOLEAN    *ss_out)
{
    STATUS 	status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(ss_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        status = NU_SUCCESS;

        /* Is bit3 of wSpeedsSupported  fields of Super Speed device
         *  capability descriptor set. 
		 */
        if ( (HOST_2_LE16(cb->devcap_ss_desc->wSpeedsSupported) & SS_MASK) == 
														SS_MASK )
        {
            *ss_out = NU_TRUE;
        }
        else
        {
            *ss_out = NU_FALSE;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_Functionality
*
*   DESCRIPTION
*
*       User can call this API to get the lowest speed at which
*       functionality is supported by this USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       func_out        - Pointer to UINT8. This contains value of
*                         bFunctionalitySupport field of Super Speed device
*                         capability.
*                         0:    Low Speed.
*                         1:    Full Speed.
*                         2:    High Speed.
*                         3:    Super Speed.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘func_out’ contains value of
*                         bFunctionalitySupport field of Super Speed device
*                         capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_Functionality (NU_USB_BOS *cb,
                                                UINT8      *func_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(func_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;

    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        *func_out   = cb->devcap_ss_desc->bFunctionalitySupported;
        status      = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat
*
*   DESCRIPTION
*
*       User can call this API to get U1 exit latency field supported by
*       this USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       u1exitlat_out   - Pointer to UINT8. This contains value of
*                         bU1ExitLat filed of Super Speed device capability
*                         descriptor.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘u1exitlat_out’ contains value of bU1ExitLat
*                         field of SuperSpeed device capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_U1ExitLat (NU_USB_BOS *cb,
                                            UINT8      *u1exitlat_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(u1exitlat_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        *u1exitlat_out  = cb->devcap_ss_desc->bU1DevExitLat;
        status          = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}
/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat
*
*   DESCRIPTION
*
*       User can call this API to get U2 exit latency field supported by
*       this USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       u2exitlat_out   - Pointer to UINT16. This contains value of
*                         wU2ExitLat field of Super Speed device capability
*                         descriptor.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘u2exitlat_out’ contains value of wU2ExitLat
*                         field of SuperSpeed device capability descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates SuperSpeed device capability descriptor
*                         is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_SuprSpd_Get_U2ExitLat (NU_USB_BOS *cb,
                                            UINT16     *u2exitlat_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(u2exitlat_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Super Speed device capability descriptor is valid. */
    if ( cb->devcap_ss_desc != NU_NULL )
    {
        *u2exitlat_out  = HOST_2_LE16(cb->devcap_ss_desc->wU2DevExitLat);
        status          = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVCAP_CntnrID_Get_CID
*
*   DESCRIPTION
*
*       User can call this API to get Container ID field of container ID
*       device capability descriptor supported by this USB 3.0 device.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_BOS control block.
*
*       continerid_out  - Double pointer to UINT8 array. This contains 16
*                         byte value of ContainerID filed of Container ID
*                         device capability descriptor.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘containerid_out’ contains value of ContainerID
*                         field of Container ID device capability
*                         descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates ContainerID device capability
*                         descriptor is not present.
*
*************************************************************************/
STATUS NU_USB_DEVCAP_CntnrID_Get_CID (NU_USB_BOS    *cb,
                                      UINT8         **containerid_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(containerid_out);
    
    /* Initialize status variable. */
    status = NU_NOT_PRESENT;
    
    /* If Container ID device capability descriptor is valid. */
    if ( cb->devcap_cid_desc != NU_NULL )
    {
        /* Assign the Container ID field to *continerid_out */
        *containerid_out    = cb->devcap_cid_desc->ContainerID;
        status              = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/*************************** end of file ********************************/
