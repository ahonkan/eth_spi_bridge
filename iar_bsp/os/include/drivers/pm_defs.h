/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       pm_defs.h
*
*   COMPONENT
*
*       PM - PPP MIB access
*
*   DESCRIPTION
*
*       Declarations and constants pertaining to the MIB updating
*       side of the PPP SNMP.
*
*   DATA STRUCTURES
*
*       PMSC_ENTRY
*       PMSS_ENTRY
*
*   DEPENDENCIES
*
*       um_defs.h
*
*************************************************************************/
#ifndef PPP_INC_PM_DEFS_H
#define PPP_INC_PM_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "networking/um_defs.h"

#define PM_ENABLED      1
#define PM_DISABLED     2
#define PM_ERROR        -1


#define PML_SilentDiscards_Inc(dev_ptr)                             \
    ((LINK_LAYER*)((DV_DEVICE_ENTRY*)dev_ptr)->dev_link_layer)->    \
                   status.silent_discards++;

#define PML_BadAddresses_Inc(dev_ptr)                               \
    ((LINK_LAYER*)((DV_DEVICE_ENTRY*)dev_ptr)->dev_link_layer)->    \
                   status.address_field_errors++;

#define PML_BadControls_Inc(dev_ptr)                                \
    ((LINK_LAYER*)((DV_DEVICE_ENTRY*)dev_ptr)->dev_link_layer)->    \
                   status.control_field_errors++;

#define PML_Overruns_Inc(dev_ptr)                                   \
    ((LINK_LAYER*)((DV_DEVICE_ENTRY*)dev_ptr)->dev_link_layer)->    \
                   status.overrun_errors++;

#define PML_BadFCSs_Inc(dev_ptr)                                    \
    ((LINK_LAYER*)((DV_DEVICE_ENTRY*)dev_ptr)->dev_link_layer)->    \
                   status.fcs_errors++;


typedef struct pm_sc_entry PMSC_ENTRY;
struct pm_sc_entry
{
    PMSC_ENTRY  *next;
    PMSC_ENTRY  *prev;
    INT32       dev_index;
    INT32       pref;
    INT32       auth;
    INT32       status;
};

typedef struct pm_ss_entry PMSS_ENTRY;
struct pm_ss_entry
{
    PMSS_ENTRY  *next;
    PMSS_ENTRY  *prev;
    INT32       dev_index;
    INT32       um_id;
    INT32       dir;
    INT32       auth;
    INT32       status;
};


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PM_DEFS_H */
