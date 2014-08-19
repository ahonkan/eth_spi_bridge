/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        
*
*       ip6_re_sup.c                                 
*
*   DESCRIPTION
*
*       This file contains IPV6 packet fragmentation and reassembly
*       supplemental functions.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP6_Remove_Frag
*       IP6_Insert_Frag
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Remove_Frag                                                   
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
VOID IP6_Remove_Frag(IP6_REASM HUGE *del_frag, IP6_REASM HUGE *prev_frag, 
                    IP6_QUEUE_ELEMENT *fp)
{
    /* Remove the first fragment from the list */
    if (del_frag == fp->ipq_first_frag)
        fp->ipq_first_frag = (IP6_REASM HUGE *)GET32(del_frag, IP6F_NEXT_OFFSET);

    /* Remove the last fragment from the list */
    else if (GET32(del_frag, IP6F_NEXT_OFFSET) == NU_NULL)
        PUT32(prev_frag, IP6F_NEXT_OFFSET, NU_NULL);

    /* Remove a middle fragment from the list */
    else
    {
        /* Set the previous frag's next to point to the deleted frag's
         * next.
         */
        PUT32(prev_frag, IP6F_NEXT_OFFSET, GET32(del_frag, IP6F_NEXT_OFFSET));
    }

} /* IP6_Remove_Frag */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Insert_Frag                                                   
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
VOID IP6_Insert_Frag(IP6_REASM HUGE *new_frag, IP6_QUEUE_ELEMENT *fp)
{
    IP6_REASM  HUGE   *next_frag;
    IP6_REASM  HUGE   *prev_frag;

    prev_frag = fp->ipq_first_frag;

    /* Find a fragment which begins after this one. */
    for (next_frag = fp->ipq_first_frag; next_frag != NU_NULL; 
         next_frag = (IP6_REASM HUGE *)GET32(next_frag, IP6F_NEXT_OFFSET))
    {        
        if (GET16(next_frag, IP6F_FRGOFFSET_OFFSET) > GET16(new_frag, IP6F_FRGOFFSET_OFFSET))
            break;

        /* Save a pointer to the previous fragment */
        prev_frag = next_frag;

    }

    /* Insert the new fragment at the end of the list */
    if (next_frag == NU_NULL) 
    {
        PUT32(prev_frag, IP6F_NEXT_OFFSET, (UINT32)new_frag);
        PUT32(new_frag, IP6F_NEXT_OFFSET, NU_NULL);
    }

    /* Insert the new fragment at the beginning of the list */
    else if (fp->ipq_first_frag == next_frag)
    {
        fp->ipq_first_frag = new_frag;
        PUT32(new_frag, IP6F_NEXT_OFFSET, (UINT32)next_frag);
    }

    /* Insert the new fragment between two existing fragments */
    else
    {
        PUT32(prev_frag, IP6F_NEXT_OFFSET, (UINT32)new_frag);
        PUT32(new_frag, IP6F_NEXT_OFFSET, (UINT32)next_frag);
    }

} /* IP6_Insert_Frag */
