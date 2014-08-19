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

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_hw_imp.c 
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal function implementations for the
*       Nucleus USB Function H/W supplementary services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       USBF_LISR                           Interrupt Servicing Function.
*       usbf_hisr                           Handles ISRs of all function
*                                           controllers.
*       USBF_HW_Deregister_LISR             Hardware LISR de-registration.
*       USBF_HW_Register_LISR               Hardware LISR registration.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef USBF_HW_IMP_C
#define USBF_HW_IMP_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       USBF_LISR
*
* DESCRIPTION
*
*       This is the LISR for USBF Stack. It handles ISRs of all function
*       controllers by activating the USB Function HISR.
*
* INPUTS
*
*       vector                              Vector that has caused this
*                                           interrupt.
*
* OUTPUTS
*
*       NONE.
*
**************************************************************************/
VOID USBF_LISR (INT vector)
{
    UINT8         i;
    NU_USBF_STACK *stack;
    STATUS        status = NU_SUCCESS;

    for (i = 0; i < nu_usbf->num_irq_entries; i++)
    {
        if (nu_usbf->hash_table[i].vector == vector)
        {
            status = NU_USB_HW_Get_Stack (
                    (NU_USB_HW *) nu_usbf->hash_table[i].fc,
                    (NU_USB_STACK **) & stack);

            if (status == NU_SUCCESS)
            {
                status |= NU_USB_HW_Disable_Interrupts (
                                  (NU_USB_HW *) nu_usbf->hash_table[i].fc);
                stack->usbf_hisr.vector = vector; 
                status |= NU_Activate_HISR (
                                           (NU_HISR *)&(stack->usbf_hisr));
            }
        }
    }
    NU_UNUSED_PARAM ( status );
}

/**************************************************************************
*
* FUNCTION
*       usbf_hisr
*
* DESCRIPTION
*       This is the HISR that handles all the IRQs of USB function 
*       controllers.
*
* INPUTS
*      None.
*
* OUTPUTS
*      None.
*
***************************************************************************/
VOID usbf_hisr (VOID)
{
    NU_USBF_HISR *stack_hisr;
    INT vector;
    STATUS status = NU_SUCCESS;
    INT i = 0;

    stack_hisr = (NU_USBF_HISR *) NU_Current_HISR_Pointer ();
    if (stack_hisr == NU_NULL)
    {
        return;
    }

    vector = stack_hisr->vector;

    /* Find which FC owns this interrupt vector.
     * Once found, invoke the interrupt servicing function
     */
    for (i = 0; i < nu_usbf->num_irq_entries; i++)
    {
        if (nu_usbf->hash_table[i].vector == vector)
        {
            status = NU_USB_HW_ISR (
                                (NU_USB_HW *) (nu_usbf->hash_table[i].fc));
            status |= NU_USB_HW_Enable_Interrupts (
                                (NU_USB_HW *) nu_usbf->hash_table[i].fc);
        }
    }
    NU_UNUSED_PARAM ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_HW_Register_LISR
*
* DESCRIPTION
*
*       Hardware LISR registration with the singleton.
*       This function adds the given vector number and the controller
*       to the map maintained and registers the Stack Global LISR
*       with the Plus for this vector.
*
* INPUTS
*
*       fc                                  Function Controller which owns
*                                           the given vector.
*       vector_num                          Interrupt vector number owned
*                                           by the Controller.
*
* OUTPUTS
*
*       NU_STATUS                           LISR has been registered
*                                           successfully.
*       NU_INVALID_VECTOR                   indicates that the specified
*                                           vector is invalid.
*       NU_NOT_REGISTERED                   Indicates the vector is not
*                                           currently registered.
*       NU_NO_MORE_LISRS                    Indicates the maximum number of
*                                           registered LISRs has been
*                                           exceeded.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  USBF_HW_Register_LISR (NU_USBF_HW *fc,
                               INT         vector_num)
{
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 0
    VOID    (*old_lisr) (INT);
#endif
    STATUS  status = NU_SUCCESS;
    INT     i;
    INT     index = nu_usbf->num_irq_entries;

    /* Error checking.   */
    if ((fc == NU_NULL) || (index >= (NU_USB_MAX_IRQ * NU_USBF_MAX_HW)))
    {
        return  NU_USB_INVLD_ARG;
    }

    /* Check if this vector is already registered. */ 
    for(i = 0; i < nu_usbf->num_irq_entries; i++)
    {
        if(nu_usbf->hash_table[i].vector == vector_num)
        {
            break;
        }
    }

    /* Register with Plus only if already not registered. */
    if (i == nu_usbf->num_irq_entries)
    {
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 0
        status = NU_Register_LISR (vector_num, USBF_LISR, &old_lisr);
#else
        status = NU_USB_OTG_Install_ISR(vector_num, USBF_LISR);
#endif
        if(status != NU_SUCCESS)
        {
            return (status);
        }
    }

    /* Place into the map. */
    nu_usbf->hash_table[index].fc = fc;

    nu_usbf->hash_table[index].vector = vector_num;

    nu_usbf->num_irq_entries++;

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_HW_Deregister_LISR
*
* DESCRIPTION
*
*       Hardware LISR de-registration with the singleton.
*       This function deletes the given vector number and the controller
*       from the map maintained and de-registers the Stack Global LISR
*       with the Plus for this vector.
*
* INPUTS
*
*       fc                                  Function Controller which owns
*                                           the given vector.
*       vector_num                          Interrupt vector number owned
*                                           by the Controller.
*
* OUTPUTS
*
*       NU_STATUS                           LISR has been de-registered
*                                           successfully.
*       NU_NOT_REGISTERED                   Indicates the vector is not
*                                           currently registered.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  USBF_HW_Deregister_LISR (NU_USBF_HW *fc,
                                 INT         vector_num)
{
    VOID    (*old_lisr) (INT);
    INT     i;
    STATUS  status = NU_NOT_REGISTERED;

    /* Error checking.   */
    if (fc == NU_NULL)
    {
        return  NU_USB_INVLD_ARG;
    }
    /* Find the vector in the map. */
    for (i = 0; i < nu_usbf->num_irq_entries; i++)
    {
        if (nu_usbf->hash_table[i].vector == vector_num)
        {
            if (nu_usbf->hash_table[i].fc == fc)
            {
                nu_usbf->num_irq_entries--;

                nu_usbf->hash_table[i] =
                    nu_usbf->hash_table[nu_usbf->num_irq_entries];

                status = NU_SUCCESS;
                break;
            }
        }
    }

    /* Found the vector. De-register this with Plus. */
    if (status == NU_SUCCESS)
    {
        /* Check to see if there are any other controllers associated with
         * this vector.
         */
        for (i = 0; i < nu_usbf->num_irq_entries; i++)
        {
            if (nu_usbf->hash_table[i].vector == vector_num)
            {
                /* Another controller with same vector number
                 * Return.
                 */
                break;
            }
        }

        if ( i == nu_usbf->num_irq_entries)
        {
            /* No controller found with same vector number.
             * De-register USBF_LISR from this vector.
             */
            status = NU_Register_LISR (vector_num, NU_NULL, &old_lisr);
        }
    }

    return (status);
}

/*************************************************************************/
#endif /* USBF_HW_IMP_C */
/* ======================  End Of File  ================================ */
