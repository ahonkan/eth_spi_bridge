/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       ring.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions specific to creating and
*       maintaining a data ring.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RingAlloc
*       RingSetMaxElems
*       RingGetMaxElems
*       RingGetElems
*       RingFree
*       RingPutMem
*       RingPeekMem
*       RingAvailMem
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       link.h
*       mac.h
*       ring.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/link.h"
#include "networking/mac.h"
#include "networking/ring.h"

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

extern rip_t *find_rip(INT32);

extern  NU_MEMORY_POOL  System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       RingAlloc
*
*   DESCRIPTION
*
*       This function sets up a new ring of the specified size with
*       the specified number of elements.
*
*   INPUTS
*
*       size                    The size of the new ring.
*       elems                   The number of elements in the new ring.
*
*   OUTPUTS
*
*       *ring                   A pointer to the new ring.
*       NU_NULL                 The ring could not be created.
*
*************************************************************************/
Ring_t *RingAlloc(INT32 size, INT32 elems)
{
    Ring_t *ring = NU_NULL;

    /* If the number of elements is greater than or equal to 0 and less
     * than the total size of the ring, create the ring.
     */
    if ( (elems >= 0) && (elems <= size) )
    {
        /* Allocate memory for a new ring */
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&ring,
                               (UNSIGNED)(sizeof(Ring_t)),
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            ring = TLS_Normalize_Ptr(ring);

            UTL_Zero(ring, sizeof(Ring_t));

            /* Set the parameters of the ring */
            ring->Size = size;
            ring->Start = 0;
            ring->Stop = 0;
            ring->PosCount = 0;
            ring->PosMax = elems;

            /* Allocate memory for the buffer */
            if (NU_Allocate_Memory(&System_Memory,
                                   (VOID**)&(ring->Buffer),
                                   (UNSIGNED)size, NU_NO_SUSPEND)
                            != NU_SUCCESS)
            {
                RingFree(ring);
                ring = NU_NULL;
            }

            else
            {
                ring->Buffer = TLS_Normalize_Ptr(ring->Buffer);
                UTL_Zero(ring->Buffer, (UINT32)size);
            }
        }
    }

    /* Return a pointer to the new ring */
    return (ring);

} /* RingAlloc */

/************************************************************************
*
*   FUNCTION
*
*       RingSetMaxElems
*
*   DESCRIPTION
*
*       This function sets the number of elements in a ring to the
*       number specified.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*       elems                   The number of elements for the ring.
*
*   OUTPUTS
*
*       NU_TRUE                 The new number of elements was set.
*       NU_FALSE                The number of elements was not set.
*
*************************************************************************/
BOOL RingSetMaxElems(Ring_t *ring, INT32 elems)
{
    BOOL    success = NU_TRUE;

    if ( (ring != NU_NULL) && (elems >= ring->PosCount) &&
         (elems <= ring->Size) )
        ring->PosMax = elems;
    else
        success = NU_FALSE;

    return (success);

} /* RingSetMaxElems */

/************************************************************************
*
*   FUNCTION
*
*       RingGetMaxElems
*
*   DESCRIPTION
*
*       This function returns the maximum number of elements allowed in
*       a given ring.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*
*   OUTPUTS
*
*       retval                  The number of elements allowed in the
*                               ring.
*       NU_NULL                 The ring is NU_NULL.
*
*************************************************************************/
INT32 RingGetMaxElems(Ring_t *ring)
{
    INT32   retval = 0;

    if (ring != NU_NULL)
        retval = ring->PosMax;

    return (retval);

} /* RingGetMaxElems */

/************************************************************************
*
*   FUNCTION
*
*       RingGetElems
*
*   DESCRIPTION
*
*       This function returns the number of elements in a ring.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*
*   OUTPUTS
*
*       retval                  The number of elements in the ring.
*       0                       The ring is NU_NULL.
*
*************************************************************************/
INT32 RingGetElems(Ring_t *ring)
{
    INT32   retval = 0;

    if (ring != NU_NULL)
        retval = ring->PosCount;

    return (retval);

} /* RingGetElems */

/************************************************************************
*
*   FUNCTION
*
*       RingFree
*
*   DESCRIPTION
*
*       This function deletes a given ring.
*
*   INPUTS
*
*       *ring                   A pointer to the ring to delete.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID RingFree(Ring_t *ring)
{
    if (ring != NU_NULL)
    {
        if(ring->Buffer != NU_NULL)
            NU_Deallocate_Memory((VOID*)(ring->Buffer));

        NU_Deallocate_Memory((VOID*)ring);
    }

} /* RingFree */

/************************************************************************
*
*   FUNCTION
*
*       RingPutMem
*
*   DESCRIPTION
*
*       This function increments the number of elements in a given ring
*       by 1.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*
*   OUTPUTS
*
*       NU_TRUE                 The number of elements was incremented.
*       NU_FALSE                Either the ring is NU_NULL or the number
*                               of elements in the ring is already at its
*                               maximum.
*
*************************************************************************/
BOOL RingPutMem(Ring_t *ring)
{
    BOOL    success = NU_TRUE;

    if ( (ring != NU_NULL) && (ring->PosCount < ring->PosMax) )
        ring->PosCount++;
    else
        success = NU_FALSE;

    return (success);

} /* RingPutMem */

/************************************************************************
*
*   FUNCTION
*
*       RingPeekMem
*
*   DESCRIPTION
*
*       This function retrieves the receive frame information for an
*       element in a link.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*       elem                    The element in the link.
*       offset                  The offset into the user's buffer into
*                               which to start copying.
*       **p                     A pointer to a pointer to the receive
*                               frame information.
*       *mem                    The buffer into which to copy the data.
*       *len                    The length of data to copy.
*       flag                    If this flag is set, copy all the data
*                               from the rip_t data structure.
*
*   OUTPUTS
*
*       NU_TRUE                 The data was copied.
*       NU_FALSE                The data was not copied.
*
*************************************************************************/
BOOL RingPeekMem(Ring_t *ring, INT32 elem, INT32 offset, UINT8 **p,
                 UINT8 *mem, INT32 *len, INT32 flag)
{
    BOOL    success = NU_TRUE;
    INT32   n = 0;
    rip_t   *rip;

    if ( (ring != NU_NULL) && (elem < ring->PosCount) )
    {
        if ((rip = (rip_t *)find_rip(elem)) != NU_NULL)
        {
            if (rip->rip_used )
                n = rip->rip_size;

            if ( (mem != NU_NULL) && (len != NU_NULL) && (*len > 0) )
                n = *len;

            if (mem != NU_NULL)
            {
                if (flag)
                {
                    NU_BLOCK_COPY(mem, rip, sizeof(rip_t));
                    *p = (UINT8 *)rip;
                }
                else
                {
                    NU_BLOCK_COPY(mem,
                                (UINT8 *)((UINT32)(rip->rip_pktp)+offset),
                                (unsigned int)n);

                    *p = rip->rip_pktp;
                }
            }

            if (len != NU_NULL)
                *len = n;
        }
        else
            success = NU_FALSE;
    }
    else
        success = NU_FALSE;

    return (success);

} /* RingPeekMem */

/************************************************************************
*
*   FUNCTION
*
*       RingAvailMem
*
*   DESCRIPTION
*
*       This function returns the amount of memory available in the given
*       ring.
*
*   INPUTS
*
*       *ring                   A pointer to the ring.
*
*   OUTPUTS
*
*       NU_FALSE                There is no memory available.
*       NU_TRUE                 There is memory available.
*
*************************************************************************/
INT32 RingAvailMem(Ring_t *ring)
{
    INT32   retval = NU_FALSE;

    if (ring != NU_NULL)
        retval = NU_TRUE;

    return (retval);

} /* RingAvailMem */

#endif


