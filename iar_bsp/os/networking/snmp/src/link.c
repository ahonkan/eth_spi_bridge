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
*       link.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions specific to setting up and
*       maintaining a logical linked list of data.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       LinkAvailSpace
*       LinkAvailData
*       LinkLength
*       LinkSize
*       LinkPush
*       LinkPop
*       LinkAlloc
*       LinkFree
*       LinkCopy
*       LinkSplit
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       link.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/link.h"

extern NU_MEMORY_POOL System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       LinkAvailSpace
*
*   DESCRIPTION
*
*       This function determines how much memory is available in the
*       specified link.
*
*   INPUTS
*
*       *link                       A pointer to the link.
*
*   OUTPUTS
*
*       link->offset                The number of bytes available.
*
*************************************************************************/
UINT16 LinkAvailSpace(const link_t *link)
{
    return (link->offset);

} /* LinkAvailSpace */

/************************************************************************
*
*   FUNCTION
*
*       LinkAvailData
*
*   DESCRIPTION
*
*       This function determines how much data is in the link's buffer.
*
*   INPUTS
*
*       *link                   A pointer to the link.
*
*   OUTPUTS
*
*       link->length            The number of bytes in the buffer.
*
*************************************************************************/
UINT16 LinkAvailData(const link_t *link)
{
    return (link->length);

} /* LinkAvailData */

/************************************************************************
*
*   FUNCTION
*
*       LinkLength
*
*   DESCRIPTION
*
*       This function determines the length of all links from the link
*       specified to the end of the links.
*
*   INPUTS
*
*       *link                   A pointer to the beginning link.
*
*   OUTPUTS
*
*       n                       The length of the links.
*
*************************************************************************/
UINT16 LinkLength(link_t *link)
{
    UINT16  n = 0;
    link_t  *p;

    for (p = link; p != 0; p = p->next)
        n = (UINT16)(n + p->length);

    return (n);

} /* LinkLength */

/************************************************************************
*
*   FUNCTION
*
*       LinkSize
*
*   DESCRIPTION
*
*       This function determines the size of all links from the link
*       specified to the end of all links.
*
*   INPUTS
*
*       *link                   A pointer to the beginning link.
*
*   OUTPUTS
*
*       n                       The size of all links.
*
*************************************************************************/
UINT16 LinkSize(link_t *link)
{
    UINT16  n = 0;
    link_t  *p;

    for (p = link; p != 0; p = p->next)
        n = (UINT16)(n + p->size);

    return (n);

} /* LinkSize */

/************************************************************************
*
*   FUNCTION
*
*       LinkPush
*
*   DESCRIPTION
*
*       This function pushes size bytes of data onto the specified link.
*
*   INPUTS
*
*       **link                  A pointer to a pointer to the data.
*       size                    The number of bytes of data to push
*                               onto the link.
*
*   OUTPUTS
*
*       If successful, this function returns a pointer to the link's
*       buffer.
*       Otherwise, this function returns NU_NULL.
*
*************************************************************************/
UINT8 *LinkPush(link_t **link, UINT16 size)
{
    link_t  *p = NU_NULL;

    /* If the number of bytes is greater than zero */
    if (size)
    {
        /* Get a pointer to the link */
        p = *link;

        /* If a link does not exist or the amount of space available in
         * the link is less than the amount we need to add, allocate more
         * memory for the link.
         */
        if ( (p == 0) || (p->offset < size) )
        {
            /* Allocate memory for the link */
            if (NU_Allocate_Memory(&System_Memory, (VOID**)&p,
                                   (UNSIGNED)(sizeof(link_t)),
                                   NU_NO_SUSPEND) == NU_SUCCESS)
            {
                p = TLS_Normalize_Ptr(p);

                UTL_Zero(p, sizeof(link_t));

                /* Allocate memory for the buffer */
                if (NU_Allocate_Memory(&System_Memory,
                                      (VOID**)&(p->buffer),
                                      (UNSIGNED)size,
                                      NU_NO_SUSPEND) == NU_SUCCESS)
                {
                    p->buffer = TLS_Normalize_Ptr(p->buffer);

                    memset(p->buffer, 0, (unsigned int)size);

                    /* Set the parameters of the link */
                    p->size   = size;
                    p->length = size;
                    p->offset = 0;
                    p->flags  = LINK_FLAG_BUFFER | LINK_FLAG_LINK;
                    p->next   = *link;
                    *link    = p;
                }

                /* Otherwise, there is not enough memory to allocate */
                else
                {
                    if (NU_Deallocate_Memory((VOID*)p) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: Failed to deallocate memory"
                                       ,NERR_SEVERE, __FILE__, __LINE__);
                    }
                    p = NU_NULL;
                }
            }
        }

        /* Otherwise, the link is not NU_NULL and there is available space
         * enough to add the data.
         */
        else
        {
            /* Increment the used data portion of the link */
            p->length = (UINT16)(p->length + size);

            /* Decrement the available data portion of the link */
            p->offset = (UINT16)(p->offset - size);
        }
    }

    /* If the data was pushed, return the new offset into the buffer */
    if (p)
        return (p->buffer + p->offset);

    /* Otherwise, return NU_NULL */
    else
        return (NU_NULL);

} /* LinkPush */

/************************************************************************
*
*   FUNCTION
*
*       LinkPop
*
*   DESCRIPTION
*
*       This function removes size bytes of data from the link.
*
*   INPUTS
*
*       **link                  A pointer to a pointer to the link.
*       size                    The size of data to remove.
*
*   OUTPUTS
*
*       If successful, this function returns a pointer to the links buffer
*       of data.
*       Otherwise, it returns NU_NULL.
*
*************************************************************************/
UINT8 *LinkPop(link_t **link, UINT16 size)
{
    link_t  *p;

    /* Get a pointer to the link */
    p = *link;

    /* If there is data enough to remove size bytes, do so */
    if ( (p != 0) && (p->length >= size) )
    {
        /* Decrement the available data for the link */
        p->length = (UINT16)(p->length - size);

        /* Decrement the size of the available data for the link */
        p->offset = (UINT16)(p->offset + size);
    }

    /* Otherwise, the link does not have enough data to satisfy the
     * request
     */
    else
        p = NU_NULL;

    /* If there was enough data to satisfy the request, return the new
     * offset into the buffer.
     */
    if (p)
        return (p->buffer + p->offset - size);

    /* Otherwise, return NU_NULL */
    else
        return (NU_NULL);

} /* LinkPop */

/************************************************************************
*
*   FUNCTION
*
*       LinkAlloc
*
*   DESCRIPTION
*
*       This function allocates memory for a new link and initializes the
*       link to the parameters passed in.
*
*   INPUTS
*
*       *link                   A pointer to the new link.
*       *buffer                 A pointer to the link's buffer area.
*       size                    The size of the link.
*       length                  The length of the link.
*       offset                  The offset into the buffer area of the
*                               link.
*       *next                   A pointer to the next link.
*
*   OUTPUTS
*
*       NU_NULL                 An error was encountered in the
*                               initialization.
*       link                    The link was successfully set up.
*
*************************************************************************/
link_t *LinkAlloc(link_t *link, UINT8 *buffer, UINT16 size, UINT16 length,
                  UINT16 offset, link_t *next)
{
    /* If memory has not already been allocated for the link */
    if (link == NU_NULL)
    {
        /* If there is no memory to allocate, return an error */
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&link,
                               (UNSIGNED)(sizeof(link_t)),
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            link = TLS_Normalize_Ptr(link);

            UTL_Zero(link, sizeof(link_t));
            link->flags = LINK_FLAG_LINK;
        }

        else
            link = NU_NULL;
    }

    /* If we are not in an error condition and there has not already been
     * memory allocate for the buffer.
     */
    if ( (link != NU_NULL) && (buffer == NU_NULL) )
    {
        /* If no size was passed in, indicate an error condition */
        if (size)
        {
            /* If there is no memory to allocate, indicate an error
             * condition.
             */
            if (NU_Allocate_Memory(&System_Memory, (VOID**)&buffer,
                                   (UNSIGNED)size, NU_NO_SUSPEND)
                                                        == NU_SUCCESS)
            {
                buffer = TLS_Normalize_Ptr(buffer);
                memset(buffer, 0, (unsigned int)size);
                link->flags |= LINK_FLAG_BUFFER;
            }

            else
            {
                if (NU_Deallocate_Memory((VOID*)link) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }

                link = NU_NULL;
            }
        }

        else
        {
            if (NU_Deallocate_Memory((VOID*)link) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            link = NU_NULL;
        }
    }

    /* If no errors were encountered, finish initializing the link
     * structure.
     */
    if (link != NU_NULL)
    {
        link->buffer   = buffer;
        link->size     = size;
        link->length   = length;
        link->offset   = offset;
        link->next     = next;
    }

    return (link);

} /* LinkAlloc */

/************************************************************************
*
*   FUNCTION
*
*       LinkFree
*
*   DESCRIPTION
*
*       This function frees the memory of the buffer of a link.
*
*   INPUTS
*
*       *link                   A pointer to the link.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LinkFree(link_t *link)
{
   if (link->flags & LINK_FLAG_BUFFER)
   {
       if (NU_Deallocate_Memory((VOID*)(link->buffer)) != NU_SUCCESS)
       {
           NLOG_Error_Log("SNMP: Failed to deallocate memory",
                            NERR_SEVERE, __FILE__, __LINE__);
       }
   }

   if (link->flags & LINK_FLAG_LINK)
   {
       if (NU_Deallocate_Memory((VOID*)link) != NU_SUCCESS)
       {
           NLOG_Error_Log("SNMP: Failed to deallocate memory",
                            NERR_SEVERE, __FILE__, __LINE__);
       }
   }

} /* LinkFree */

/************************************************************************
*
*   FUNCTION
*
*       LinkCopy
*
*   DESCRIPTION
*
*       This function copies the contents of the buffer of a link into
*       the user's buffer.
*
*   INPUTS
*
*       *link                   A pointer to the link.
*       *buffer                 A pointer to the buffer into which to
*                               copy the data.
*       size                    The amount of data to copy.
*
*   OUTPUTS
*
*       NU_TRUE                 The data was copied.
*       NU_FALSE                The data was not copied.
*
*************************************************************************/
BOOLEAN LinkCopy(link_t *link, UINT8 *buffer, UINT16 size)
{
    BOOLEAN    success = NU_TRUE;

    /* If the amount of data to be copied is less than or equal to the
     * amount of data we want to copy, copy the data
     */
    if (LinkLength(link) <= size)
    {
        /* While there is data to be copied */
        while (link != 0)
        {
            NU_BLOCK_COPY(buffer, link->buffer + link->offset,
                          (unsigned int)(link->length));

            buffer += link->length;
            link = link->next;
        }
    }
    else
        success = NU_FALSE;

    return (success);

} /* LinkCopy */

/************************************************************************
*
*   FUNCTION
*
*       LinkSplit
*
*   DESCRIPTION
*
*       This function reduces the length of each valid link in the list
*       by length bytes and sets the tail of the list to the last valid
*       link in the list.
*
*   INPUTS
*
*       *head                   A pointer to the head of the list.
*       *tail                   A pointer to the tail of the list.
*       length                  The length of the list.
*
*   OUTPUTS
*
*       NU_TRUE                 The link was successfully split.
*       NU_FALSE                The link was not split.
*
*************************************************************************/
BOOLEAN LinkSplit(link_t *head, link_t *tail, UINT16 length)
{
    BOOLEAN    success = NU_TRUE;
    link_t  *p;

    /* If the head and tail are not NU_NULL */
    if ( (head != NU_NULL) && (tail != NU_NULL) )
    {
        /* Get a pointer to the head of the list */
        p = head;

        /* While there is a link to process and the length of the link is
         * less than the length passed in
         */
        while ( (p != 0) && (length > p->length) )
        {
            /* Decrement the length of the link */
            length = (UINT16)(length - p->length);

            /* Get the next link */
            p = p->next;
        }

        /* If we reached the end of the links set the tail to zeros */
        if (p == NU_NULL)
        {
            tail->buffer = 0;
            tail->size   = 0;
            tail->offset = 0;
            tail->length = 0;
            success = NU_FALSE;
        }

        /* Otherwise, set the tail to the link */
        else
        {
            tail->next   = p->next;
            tail->buffer = p->buffer;
            tail->size   = p->size;
            tail->offset = (UINT16)(p->offset + length);
            tail->length = (UINT16)(p->length - length);
            head->length = length;
        }
    }
    else
        success = NU_FALSE;

    return (success);

} /* LinkSplit */


