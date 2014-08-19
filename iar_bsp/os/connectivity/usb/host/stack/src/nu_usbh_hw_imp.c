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
*     nu_usbh_hw_imp.c
*
* COMPONENT
*     USB Host
*
* DESCRIPTION
*     This file contains implementations of NU_USBH_HW's supplementary
*     functions.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*     USBH_HW_Register_LISR      Registers USBH_LISR for the given vector.
*     USBH_HW_Deregister_LISR    Deregisters the LISR for the given vector.
*     USBH_LISR                  LISR that handles all USB Host H/W vectors
*                                by activating their corresponding HISR.
* DEPENDENCIES
*     nu_usb.h                   All USB definitions
*
************************************************************************/
#ifndef USBH_HW_IMP_C
#define USBH_HW_IMP_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*     USBH_HW_Register_LISR
*
* DESCRIPTION
*     This function registers a LISR for the given hardware controller
*     and interrupt vector number.
*
* INPUTS
*     controller          pointer to HW control block
*     vector_no           interrupt vector number.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion
*     NU_INVALID_VECTOR   Invalid interrupt vector
*     NU_NO_MORE_LISRS    LISR registration table is full
*     NU_USB_INVLD_ARG    Invalid arguments
*************************************************************************/
STATUS USBH_HW_Register_LISR (NU_USBH_HW * controller,
                              INT vector_no)
{
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 0
    VOID    (*old_lisr) (INT);
#endif
    STATUS  status;
    INT     i;

    if ((nu_usbh->num_irq_entries >= NU_USBH_MAX_HW)
        || (controller == NU_NULL))
    {
        return (NU_USB_INVLD_ARG);
    }

    status = NU_SUCCESS;

    /* check if this vector is already registered */
    for(i = 0; i < nu_usbh->num_irq_entries; i++)
    {
        if(nu_usbh->hash_table[i].vector == vector_no)
            break;
    }

    /* register with Plus only if already not registered */
    if (i == nu_usbh->num_irq_entries)
    {
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 0
        status = NU_Register_LISR (vector_no, USBH_LISR, &old_lisr);
#else        
        status = NU_USB_OTG_Install_ISR(vector_no, USBH_LISR);
#endif

        if(status != NU_SUCCESS)
        {
            return (status);
        }
    }

    /* update the vector/controller map */
    nu_usbh->hash_table[nu_usbh->num_irq_entries].controller = controller;

    nu_usbh->hash_table[nu_usbh->num_irq_entries].vector = vector_no;

    nu_usbh->num_irq_entries++;


    return (status);
}

/*************************************************************************
* FUNCTION
*     USBH_HW_Deregister_LISR
*
* DESCRIPTION
*     This function deregisters a LISR for the given hardware controller
*     and interrupt vector number.
*
* INPUTS
*     controller          pointer to HW control block
*     vector_no           interrupt vector number.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion
*     NU_USB_INVLD_ARG    Indicates specified LISR is not registered.
*     NU_USB_INVLD_ARG    Invalid arguments
*************************************************************************/
STATUS USBH_HW_Deregister_LISR (NU_USBH_HW * controller,
                                INT vector_no)
{
    VOID    (*old_lisr) (INT);
    INT        i;
    STATUS    status = NU_USB_INVLD_ARG;

    if (controller == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }
    /* First, remove the vector / controller map */
    for (i = 0; i < nu_usbh->num_irq_entries; i++)
    {
        if (nu_usbh->hash_table[i].vector == vector_no)
        {
            if (nu_usbh->hash_table[i].controller == controller)
            {
                nu_usbh->num_irq_entries--;

                nu_usbh->hash_table[i] =
                    nu_usbh->hash_table[nu_usbh->num_irq_entries];

                status = NU_SUCCESS;

                break;
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        /* check to see if there are any other controllers associated with
         * this vector */
        for (i = 0; i < nu_usbh->num_irq_entries; i++)
        {
            if (nu_usbh->hash_table[i].vector == vector_no)
            {
                /* another controller with same vector number */
                /* return */
                break;
            }
        }

        if( i == nu_usbh->num_irq_entries)
        {
            /* No controller found with same vector number.
             * De-register USBF_LISR from this vector.
             */
            status = NU_Register_LISR (vector_no, NU_NULL, &old_lisr);
        }
    }

    return (status);
}

/************************************************************************
 *
 * FUNCTION
 *     USBH_LISR
 *
 * DESCRIPTION
 *     This is the LISR for USB Host Stack. It handles ISRs of all host
 *     controllers. It activates a generic HISR that can further handle
 *     these interrupts.
 *
 * INPUTS
 *     INT   vector_no, Vector that has caused this interrupt.
 *
 * OUTPUTS
 *     Void
 *************************************************************************/
VOID USBH_LISR (INT vector_no)
{
    UINT8         i;
    NU_USBH_STACK *stack = NU_NULL;
    STATUS        status;
    NU_USBH_HW     *controller;
    BOOLEAN       activated = NU_FALSE;   /* whether HISR is activated or not */

    /* for each host controller */
    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        /* find ones with matching interrupt vector */
        if (nu_usbh->hash_table[i].vector == vector_no)
        {
            /* get pointer to the controller */
            controller = nu_usbh->hash_table[i].controller;

            /* get pointer to the stack */
            status = NU_USB_HW_Get_Stack((NU_USB_HW *)controller,
                                         (NU_USB_STACK **) &stack);
            if (status == NU_SUCCESS)
            {
                /* disable interrupts until the current ones are handled */
                status = NU_USB_HW_Disable_Interrupts ((NU_USB_HW *)controller);

                if (status == NU_SUCCESS)
                {
                    /* mark pending interrupts for stack task */
                    controller->pending = NU_TRUE;

                    /* avoid activating HISR multiple times for multiple
                     * controllers sharing an interrupt vector */
                    if (activated == NU_FALSE)
                    {
                        status = NU_Activate_HISR ((NU_HISR *) &(stack->usbh_hisr));
                        if (status == NU_SUCCESS)
                        {
                            activated = NU_TRUE;
                        }
                        else
                        {
                            activated = NU_FALSE;
                        }
                    }
                }
            }
        }
    }
}

/************************************************************************/

#endif /* USBH_HW_IMP_C */
/* ======================  End Of File  =============================== */
