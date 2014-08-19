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
*       sck6_ape.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Add_Prefix_Entry.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Add_Prefix_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*       prefix6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Add_Prefix_Entry                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function adds a prefix entry to the Prefix List for a 
*       specified interface.  If the PRFX6_NO_ADV_AUTO flag is not
*       set for the prefix, a new IPv6 address will be assigned to the
*       interface created from the prefix and the interface identifier 
*       for the specified interface.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       dev_index               The interface index of the interface to 
*                               configure.
*       *prefix_entry           A pointer to the Prefix entry to add:
*
*       *prfx_prefix            A pointer to the Prefix to add.
*       prfx_length             The length of the new Prefix in bits.
*       prfx_adv_valid_lifetime The Valid Lifetime of the new entry.     
*       prfx_adv_pref_lifetime  The Preferred Lifetime of the new
*                               entry.
*       prfx_flags              Flags for the entry:
*
*                               PRFX6_NO_ADV_ON_LINK - do not set the
*                                                      on-link flag
*                                                      in RA's
*                               PRFX6_NO_ADV_AUTO    - do not set the
*                                                      autoconfig flag
*                                                      in RA's
*                               PRFX6_DEC_VAL_LIFE   - Decrement the 
*                                                      Valid Lifetime in
*                                                      real-time in RA's
*                               PRFX6_DEC_PREF_LIFE  - Decrement the
*                                                      Preferred Lifetime
*                                                      in real-time in 
*                                                      RA's
*                               PRFX6_HOME_NETWORK   - Indicates this prefix
*                                                      is a Home Network
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*       NU_NO_MEMORY            Insufficient memory.
*                                                                         
*************************************************************************/
STATUS NU_Add_Prefix_Entry(UINT32 dev_index, DEV6_PRFX_ENTRY *prefix_entry)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Validate the interface and prefix */
    if ( (prefix_entry == NU_NULL) ||
         (prefix_entry->prfx_prefix == NU_NULL) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    status =  NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /* Get a pointer to the device */
    dev_ptr = DEV_Get_Dev_By_Index(dev_index);

    /* Validate the interface */
    if (dev_ptr != NU_NULL)
        status = PREFIX6_Configure_DEV_Prefix_Entry(prefix_entry, dev_ptr);
    else
        status = NU_INVALID_PARM;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Add_Prefix_Entry */
