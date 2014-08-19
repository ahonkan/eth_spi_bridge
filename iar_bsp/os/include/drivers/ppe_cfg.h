/*************************************************************************
*
*               Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
****************************************************************************
* FILE NAME                                          
*
*       ppe_cfg.h                                  
*
* COMPONENT
*
*       PPE - PPPoE Driver application configuration
*
* DESCRIPTION
*
*       These data structures are provided to allow the application
*       developer to configure the PPPoE service information and
*       any other miscellaneous information.
*
* DATA STRUCTURES
*
*       PPE_SERVICE
*
* DEPENDENCIES
*
*       None
*
***************************************************************************/
#ifndef PPE_CFG_H
#define PPE_CFG_H

/* This option controls how PPPoE will handle the MSS option in
   TCP SYN packets. The problem is that most hosts assume a MTU
   for the path to be 1500 bytes, a MSS of 1460. But when PPPoE
   is involved the MTU is 1492, MSS of 1452. This can cause 
   problems with host sending packets that are the size of the 
   MTU. Setting this option to NU_TRUE enables PPPoE to scan
   all packets. TCP SYN packets with the MSS option will be modified
   to contain a MSS of 1452 if the stated MSS is greater than 1452.
   NU_TRUE is the default setting of this option.
*/
#define INCLUDE_PPPOE_TCP_MSS_FIX   NU_TRUE

/* Define the name of the system or client/server application. */
#define PPE_AC_NAME         "PPPoE Server"
#define PPE_HOST_NAME       "PPPoE Client"


/* Comment out either of these that will NOT be used for the application.
   e.g. if the system is being used only as an access concentrator/server,
   comment out INCLUDE_PPE_HOST. */
#define INCLUDE_PPE_AC      NU_TRUE
#define INCLUDE_PPE_HOST    NU_TRUE

/* If using AC services, define the total number of services that are
   provided for peer host connections. */
#define PPE_NUM_SERVICES    2

/* Define the structure of the peer services. Only the name field is
   mandatory and it is the only field that is used within the PPPoE
   driver itself. However, other fields can be added, so that if a
   particular service is used, extra proprietary information for that
   service can be obtained and used. */
typedef struct PPE_Services
{
    UINT8 *name;
/*    UINT16 dummy1;     <-- Example -- not used by PPPoE! */
/*    UINT16 dummy2;     <-- Example -- not used by PPPoE! */
} PPE_SERVICE;


/* Define the printf function that can be used for debugging. By
   default this is defined to nothing in order to disable this 
   feature. To enable printing of PPPoE information define this
   macro to your print output function. 
*/
#define PPE_Printf(x) 

/* Define this alias to view debug messages using the above print function.
   By default, debugging is disabled and should not be enabled until
   the PPE_Printf(x) macro is aliased to a valid display function
   for your target. */
#define NU_DEBUG_PPE NU_FALSE

#endif

