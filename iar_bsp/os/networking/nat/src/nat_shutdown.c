/***************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2011
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat_shutdown.c                                              
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains the functions necessary to shutdown Nucleus NAT
*       on the node.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.                             
*                                               
*   FUNCTIONS                                                                  
*                                             
*       NAT_Shutdown
*       NAT_Cleanup
*       NAT_Remove_Interface
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       externs.h
*       nat_defs.h            
*       nat_extr.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"

extern NAT_DEV_LIST             NAT_Internal_Devices;
extern DV_DEVICE_ENTRY          *NAT_External_Device;
extern NAT_TRANSLATION_TABLE    NAT_Translation_Table;
extern INT                      IP_NAT_Initialize;
extern NAT_PORTMAP_TABLE        NAT_Portmap_Table;

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Shutdown
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function uninitializes the Translation Table and Portmap
*       Table.  This routine should be called by the application layer.
*                                                                       
*   INPUTS                                                                
*          
*       None.
*                                                                       
*   OUTPUTS                                                               
*                 
*       NU_SUCCESS
*
****************************************************************************/
STATUS NAT_Shutdown(VOID)
{
    STATUS      status;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the TCP semaphore since the IP layer could be in the NAT
     * module right now.
     */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Shutdown Nucleus NAT. */
        NAT_Cleanup();

        status = NU_Release_Semaphore(&TCP_Resource);
    }

    NU_USER_MODE();

    return (status);

} /* NAT_Shutdown */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Cleanup
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function uninitializes the Translation Table and Portmap
*       Table.
*                                                                       
*   INPUTS                                                                
*          
*       None.
*                                                                       
*   OUTPUTS                                                               
*                 
*       None.
*
****************************************************************************/
VOID NAT_Cleanup(VOID)
{
    NAT_DEV_ENTRY       *current_device;
    NAT_PORTMAP_ENTRY   *current_entry, *next_entry;

    /* Deallocate all memory from the Portmap Table. */
    current_entry = NAT_Portmap_Table.nat_head;

    while (current_entry)
    {
        /* Save a pointer to the next portmap entry. */
        next_entry = current_entry->nat_next;

        /* Deallocate the memory for this entry. */
        NU_Deallocate_Memory(current_entry);

        /* Restore the pointer to the next entry to be removed. */
        current_entry = next_entry;
    }

    NAT_Portmap_Table.nat_head = NU_NULL;

#if (INCLUDE_TCP == NU_TRUE)

    /* Deallocate memory for the TCP translation table. */
    NU_Deallocate_Memory(NAT_Translation_Table.nat_tcp_table);

    NAT_Translation_Table.nat_tcp_table = NU_NULL;

#endif

#if (INCLUDE_UDP == NU_TRUE)

    /* Deallocate memory for the UDP translation table. */
    NU_Deallocate_Memory(NAT_Translation_Table.nat_udp_table);

    NAT_Translation_Table.nat_udp_table = NU_NULL;

#endif

    /* Deallocate memory for the ICMP translation table. */
    NU_Deallocate_Memory(NAT_Translation_Table.nat_icmp_table);

    NAT_Translation_Table.nat_icmp_table = NU_NULL;

    /* Deallocate memory for the Port list. */
    NU_Deallocate_Memory(NAT_Translation_Table.nat_port_list);

    NAT_Translation_Table.nat_port_list = NU_NULL;

    /* Get the first internal device off the list. */
    current_device = DLL_Dequeue(&NAT_Internal_Devices);

    /* Deallocate memory for each of the internal devices on the list. */
    while (current_device)
    {
        NU_Deallocate_Memory(current_device);

        /* Get the next device on the list. */
        current_device = DLL_Dequeue(&NAT_Internal_Devices);
    }

    /* Set the global variable to NULL just for safety. */
    NAT_External_Device = NU_NULL;

    /* Unset the timer tasks to cleanup the TCP and UDP entries in the Address
     * Translation Table.
     */
#if (INCLUDE_TCP == NU_TRUE)
    TQ_Timerunset(NAT_CLEANUP_TCP, TQ_CLEAR_EXACT, 0, 0);
#endif
#if (INCLUDE_UDP == NU_TRUE)
    TQ_Timerunset(NAT_CLEANUP_UDP, TQ_CLEAR_EXACT, 0, 0);
#endif

    /* Unset the timer task to cleanup the ICMP entries in the ICMP Table */
    TQ_Timerunset(NAT_CLEANUP_ICMP, TQ_CLEAR_EXACT, 0, 0);

    /* Indicate that Nucleus NAT is no longer initialized. */
    IP_NAT_Initialize = 0;
    
} /* NAT_Cleanup */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Remove_Interface
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function checks if the specified interface is being used by
*       NAT and uninitializes the Translation Table and Portmap
*       Table if so.  This routine should be called by Nucleus NET when
*       an interface is removed from the system.
*                                                                       
*   INPUTS                                                                
*          
*       None.
*                                                                       
*   OUTPUTS                                                               
*                 
*       None.
*
****************************************************************************/
VOID NAT_Remove_Interface(const DV_DEVICE_ENTRY *dv_entry)
{
    NAT_DEV_ENTRY   *dv_next;

    /* Get a pointer to the first device in the list of internal devices. */
    dv_next = NAT_Internal_Devices.nat_head;

    /* Check if the device being removed is an internal device. */
    while (dv_next)
    {
        if (dv_next->nat_device == dv_entry)
            break;

        dv_next = dv_next->nat_dev_next;
    }

    /* If the device being removed is an internal device or the external
     * device, stop Nucleus NAT now.
     */
    if ( (dv_next) || (dv_entry == NAT_External_Device) )
        NAT_Cleanup();

} /* NAT_Remove_Interface */
