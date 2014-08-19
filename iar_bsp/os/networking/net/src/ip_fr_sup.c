/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ip_fr_sup.c
*
* DESCRIPTION
*
*       This file contains IP packet fragmentation and reassembly
*   supplemental functions.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Remove_Frag
*       IP_Insert_Frag
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       IP_Remove_Frag
*
*   DESCRIPTION
*
*       Removes a fragment from the list.
*
*   INPUTS
*
*       *del_frag               Pointer to the fragment to delete from
*                               the list.
*       *prev_frag              Pointer to the previous fragment in the
*                               list.
*       *fp                     Pointer to the ip element queue structure.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Remove_Frag(IP_FRAG *del_frag, IP_FRAG *prev_frag,
                    IP_QUEUE_ELEMENT *fp)
{
    /* Remove the first fragment from the list */
    if (del_frag == fp->ipq_first_frag)
        fp->ipq_first_frag = (IP_FRAG *)GET32(del_frag, IPF_NEXT_OFFSET);

    /* Remove the last fragment from the list */
    else if (GET32(del_frag, IPF_NEXT_OFFSET) == 0)
        PUT32(prev_frag, IPF_NEXT_OFFSET, 0);

    /* Remove a middle fragment from the list */
    else
    {
        /* Set the previous frag's next to point to the deleted frag's
         * next.
         */
        PUT32(prev_frag, IPF_NEXT_OFFSET, GET32(del_frag, IPF_NEXT_OFFSET));
    }

} /* IP_Remove_Frag */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Insert_Frag
*
*   DESCRIPTION
*
*       Inserts a fragment into the fragment linked list.
*
*   INPUTS
*
*       *new_frag               Pointer to the IP Fragmentation structure
*       *fp                     Pointer to the IP queue element structure
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Insert_Frag(IP_FRAG *new_frag, IP_QUEUE_ELEMENT *fp)
{
    IP_FRAG     *next_frag;
    IP_FRAG     *prev_frag = NU_NULL;

    /* Find a fragment which begins after this one. */
    for (next_frag = fp->ipq_first_frag; next_frag != NU_NULL;
         next_frag = (IP_FRAG*)GET32(next_frag, IPF_NEXT_OFFSET))
    {
        if (GET16(next_frag, IPF_OFF_OFFSET) > GET16(new_frag, IPF_OFF_OFFSET))
            break;

        /* Save a pointer to the previous fragment */
        prev_frag = next_frag;
    }

    /* Insert the new fragment at the beginning of the list */
    if (fp->ipq_first_frag == next_frag)
    {
        fp->ipq_first_frag = new_frag;
        PUT32(new_frag, IPF_NEXT_OFFSET, (UINT32)next_frag);
    }

    /* Insert the new fragment at the end of the list */
    else if (next_frag == NU_NULL)
    {
        PUT32(prev_frag, IPF_NEXT_OFFSET, (UINT32)new_frag);
        PUT32(new_frag, IPF_NEXT_OFFSET, 0);
    }

    /* Insert the new fragment between two existing fragments */
    else
    {
        PUT32(prev_frag, IPF_NEXT_OFFSET, (UINT32)new_frag);
        PUT32(new_frag, IPF_NEXT_OFFSET, (UINT32)next_frag);
    }

} /* IP_Insert_Frag */

#endif
