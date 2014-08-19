/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_com_cfg.h
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains precompile built options for the library.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
**************************************************************************/

/* ===================================================================== */

/* These macros are required to define for including in the support for
 * each Communication device model.
 */
#define INC_ECM_MDL                          /* Ethernet control model */
#define INC_ACM_MDL                          /* Abstract control model */

/* #define INC_DCM_MDL */
/* #define INC_CCM_MDL */
/* #define INC_MCM_MDL */
/* #define INC_TCM_MDL */

/* Internal functionality control.*/
/* Nucleus USB Host Communication driver polls the device on interrupt
 * pipe. There is an internal task for that controlled through these 
 * parameters.
 */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define UH_COM_POLL_STACK_SIZE           (4 * NU_MIN_STACK_SIZE)
#else
#define UH_COM_POLL_STACK_SIZE           2048
#endif
#define UH_COM_POLL_TASK_PRIORITY        50

/* Nucleus USB Host Communication driver polls the device for incoming
 * data. There is an internal task for that controlled through these
 * parameters.
 */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define DATA_POLL_STACK_SIZE             (5 * NU_MIN_STACK_SIZE)
#else
#define DATA_POLL_STACK_SIZE             2048
#endif
#define DATA_POLL_TASK_PRIORITY          50
