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
*        nu_usb_subsys_imp.c
*
* COMPONENT
*
*        Nucleus USB software :subsystem component
*
* DESCRIPTION
*       This file provides the default implementation of internal functions
*       of Nucleus USB software's subsystem component.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       usb_get_id              -Allocates a unique 8 bit id.
*       usb_release_id          -Releases unique 8 bit id.
*       usb_is_id_set           -Checks to see if the id is set in the bitmap
*       _NU_USB_SUBSYS_Delete   -deletes a specified subsystem.
*       _NU_USB_SUBSYS_Lock     -Grabs a lock.
*       _NU_USB_SUBSYS_Unlock   -Releases a lock.
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_SUBSYS_IMP_C
#define    USB_SUBSYS_IMP_C

/* ======================  Include Files  ============================= */
#include "nucleus.h"
#include "connectivity/nu_usb.h"

/*************************************************************************
 *
 * FUNCTION
 *      usb_get_id
 *
 * DESCRIPTION
 *    Allocates a unique 8 bit id. Its used for assigning Ids to control blocks
 *    of various classes.
 *
 * INPUTS
 *   UINT8 *bits   ptr to bit map. A 1 in a bit position indicates
 *                 non-availability of the corresponding id and 0 indicates
 *                 availability.
 *   UINT8 size    size of this bit map in bytes.
 *
 * OUTPUTS
 *    UINT8   a unique 8 bit id in the range of 0 to size
 *            or USB_NO_ID, if no unique id is available
 *
 *************************************************************************/
UINT8 usb_get_id (UINT8 *bits,
                  UINT8 size,
                  UINT8 last_id)
{
    UINT8 i, j;
    UINT8 mask;

    ++last_id;
    last_id = last_id % (size * 8);

    /* Look for the first available id */
    for (i = 0; i < size; i++)
    {
        if (bits[i] != 0xff)
        {
            mask = 1;
            for (j = 0; j < 8; j++)
            {
                if ((mask & bits[i]) == 0)
                {
                    if ((i * 8 + j) >= last_id)
                    {
                        bits[i] = bits[i] | mask;
                        return (i * 8 + j);
                    }
                }
                mask = mask << 1;
            }
        }
    }
    return USB_NO_ID;
}

/*************************************************************************
 *
 * FUNCTION
 *      usb_release_id
 *
 * DESCRIPTION
 *    releases unique 8 bit id that was assigned to the object.
 *
 * INPUTS
 *   UINT8 *bits   ptr to bit map. A 1 in a bit position indicates
 *                 non-availability of the corresponding id and 0 indicates
 *                 availability.
 *   UINT8 size    size of this bit map in bytes.
 *
 * OUTPUTS
 *    UINT8   a unique 8 bit id in the range of 0 to size
 *            or USB_NO_ID, if no unique id is available
 *
 *************************************************************************/
VOID usb_release_id (UINT8 *bits,
                     UINT8 size,
                     UINT8 id)
{
    UINT8 i, j;
    UINT8 mask = 1;

    if (id >= size * 8)
        return;

    /* Mark the id as available */
    i = id / 8;
    j = id % 8;
    mask = mask << j;
    bits[i] = bits[i] & (~mask);
}

/*************************************************************************
 *
 * FUNCTION
 *      usb_is_id_set
 *
 * DESCRIPTION
 *    Checks to see if the id is set in the bitmap.
 *
 * INPUTS
 *   UINT8 *bits   ptr to bit map. A 1 in a bit position indicates
 *                 non-availability of the corresponding id and 0 indicates
 *                 availability.
 *   UINT8 size    size of this bit map in bytes.
 *
 * OUTPUTS
 *   1 if the id's bit is set
 *   0 otherwise.
 *
 *************************************************************************/
UINT8 usb_is_id_set (UINT8 *bits,
                     UINT8 size,
                     UINT8 id)
{
    UINT8 i, j;
    UINT8 mask = 1;

    if (id >= size * 8)
        return 0;

    /* Mark the id as available */
    i = id / 8;
    j = id % 8;
    mask = mask << j;
    return (bits[i] & mask);
}

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/*************************************************************************
* FUNCTION
*               _NU_USB_SUBSYS_Delete
*
* DESCRIPTION
*       This function deletes a specified subsystem.
*
* INPUTS
*       cb      pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS          Subsystem deleted successfully
*
*************************************************************************/
STATUS _NU_USB_SUBSYS_Delete (NU_USB_SUBSYS * cb)
{
    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_SUBSYS_Lock
*
* DESCRIPTION
*       This function is used to provide multi-threaded safety to critical
*       data structure. This function grabs the lock.
*
* INPUTS
*       cb      pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USB_SUBSYS_Lock (NU_USB_SUBSYS * cb)
{
    /* This function will be used as-is by the device stack and will be
     * extended by the host stack to implement using a semaphore
     * */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_SUBSYS_Unlock
*
* DESCRIPTION
*       This function is used to provide multi-threaded safety to critical
*       data structure. This function releases the lock.
*
* INPUTS
*       cb      pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USB_SUBSYS_Unlock (NU_USB_SUBSYS * cb)
{
    /* This function will be used as-is by the device stack and will be
     * extended by the host stack to implement using a semaphore
     * */
    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);
    return (NU_SUCCESS);
}

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/*************************************************************************/

#endif /* USB_SUBSYS_IMP_C */
/*************************** end of file ********************************/

