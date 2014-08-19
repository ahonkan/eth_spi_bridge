/*************************************************************************
*
*            Copyright 2002-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       can_cfg.h
*
* COMPONENT
*
*       Nucleus CAN Configuration
*
* DESCRIPTION
*
*       This file contains the configuration options for Nucleus CAN.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
/* Check to avoid multiple file inclusion. */
#ifndef     CAN_CFG_H
#define     CAN_CFG_H

/* Set the identifiers for various operating systems supported by
   Nucleus CAN. */

#define     NU_PLUS_OS                  1   /* Nucleus PLUS RTOS ID for
                                               usage with Nucleus CAN.  */
#define     NU_OSEK_OS                  2   /* Nucleus OSEK RTOS ID for
                                               usage with Nucleus CAN.  */

/* Set the identifiers for various operating modes for Nucleus CAN. */

#define     NU_CAN_LOOPBACK             1   /* Loopback mode utilizing
                                               software loopback device
                                               for Nucleus CAN. No need
                                               for CAN hardware driver. */
#define     NU_CAN_HW_DRIVER_NORMAL     2   /* Normal operating mode using
                                               a hardware drive for CAN.*/
#define     NU_CAN_HW_DRIVER_LOOPBACK   3   /* Loopback at the hardware
                                               level. Will only be
                                               available if the hardware
                                               supports it.             */
#define     NU_CAN_HW_DRIVER_LISTENNOLY 4   /* Listen only mode. Will only
                                               be available if the
                                               hardware supports it.    */

/* DEFINE:      NU_CAN_OPERATING_MODE
   DEFAULT:     NU_CAN_OPERATING_MODE is set to NU_CAN_LOOPBACK.
   DESCRIPTION: This option sets the operating mode for which Nucleus CAN
                is to be configured. Currently supported options are
                listed at the top of this file. This is being set
                using the metadata option of CAN.
   NOTE:        Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                demonstration for the corresponding OS must be built
                after changing this define. */
#define     NU_CAN_OPERATING_MODE           CFG_NU_OS_CONN_CAN_OPERATING_MODE

/* DEFINE:      NU_CAN_MULTIPLE_PORTS_SUPPORT
   DEFAULT:     NU_CAN_MULTIPLE_PORTS_SUPPORT is set to 0.
   DESCRIPTION: This option lets the user integrate multiple hardware
                driver ports for CAN with Nucleus CAN. Upto four CAN
                driver ports can be integrated and used simultaneously
                with Nucleus CAN.
   NOTE:        Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                demonstration for the corresponding OS must be built
                after changing this define. */
#define     NU_CAN_MULTIPLE_PORTS_SUPPORT   0

/* DEFINE:      NU_CAN_MAX_DEV_COUNT
   DEFAULT:     NU_CAN_MAX_DEV_COUNT is set to 1.
   DESCRIPTION: This option lets the user limit the number of multiple
                hardware driver ports for CAN with Nucleus CAN. This
                option depends on the CAN hardware configuration of
                the target. User must make sure that this value is
                greater than or equal to CAN_DEV_COUNT specified in
                specific can_driver.h file.
   NOTE:        Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                demonstration for the corresponding OS must be built
                after changing this define. */
#define     NU_CAN_MAX_DEV_COUNT   1

/* DEFINE:      NU_CAN_DEBUG
   DEFAULT:     NU_CAN_DEBUG is set to 1.
   DESCRIPTION: This option provides some debugging facilities for
                Nucleus CAN services. When set to '1', it will check if
                all the API parameters are valid. It is recommended to
                set this to '0' once an application is fully developed.
   NOTE:        1) Nucleus CAN and Nucleus CAN demonstration for the
                   corresponding OS must be built after changing this
                   options.
                2) This debugging facility is disabled automatically when
                   size/speed optimization option for Nucleus CAN is
                   enabled in this file. */
#define     NU_CAN_DEBUG                    1

/* DEFINE:      NU_CAN_MMU_SUPPORT
   DEFAULT:     NU_CAN_MMU_SUPPORT is set to 0.
   DESCRIPTION: This option adds the MMU support in Nucleus CAN provided
                the underlying OS provides the MMU support.
   NOTE:        1) Nucleus CAN and Nucleus CAN demonstration for the
                   corresponding OS must be built after changing this
                   options.
                2) Currently only Nucleus PLUS provides MMU support so,
                   this option would be disabled if selected for
                   Nucleus OSEK. */
#define     NU_CAN_MMU_SUPPORT              0

/* DEFINE:      NU_CAN_OPTIMIZE_FOR_SIZE
   DEFAULT:     NU_CAN_OPTIMIZE_FOR_SIZE is set to 0.
   DESCRIPTION: This option allows the user to tailor Nucleus CAN to
                small memory. This is being set using the metadata
                option of CAN.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this define
                2) This option disables the debugging support and
                   decreases the default queue sizes for Nucleus CAN.
                3) When speed optimization is also enabled along with
                   size optimization in this file, the speed optimization
                   would be favored more than size optimization. */
#define     NU_CAN_OPTIMIZE_FOR_SIZE        CFG_NU_OS_CONN_CAN_OPTIMIZE_FOR_SIZE

/* DEFINE:      NU_CAN_OPTIMIZE_FOR_SPEED
   DEFAULT:     NU_CAN_OPTIMIZE_FOR_SPEED is set to 0.
   DESCRIPTION: This option allows the user to tailor Nucleus CAN for
                high speed message processing. This is being set using
                the metadata option of CAN.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) Enabling this option would result in a larger
                   code size. */
#define     NU_CAN_OPTIMIZE_FOR_SPEED       CFG_NU_OS_CONN_CAN_OPTIMIZE_FOR_SPEED

/* DEFINE:      NU_CAN_SUPPORTS_RTR
   DEFAULT:     NU_CAN_SUPPORTS_RTR is set to 1.
   DESCRIPTION: This option provides the facility to turn off/on the
                support for RTR message transmission from the node
                running Nucleus CAN and thus help reduce the code size
                for application which doesn't need RTR messages.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) Disabling the support for RTR only disables sending
                   RTR from node. The node may still receive the RTR
                   messages.
                3) If the nodes supports RTR, then additionally
                   Nucleus CAN provides the facility to set responses
                   for RTR which will be transmitted automatically on
                   the reception of an a matching RTR reception. */
#define     NU_CAN_SUPPORTS_RTR             1

/* DEFINE:      NU_CAN_AUTOMATIC_RTR_RESPONSE
   DEFAULT:     NU_CAN_AUTOMATIC_RTR_RESPONSE is set to 1.
   DESCRIPTION: This option provides the facility for setting responses
                for RTR messages in advance. The message response will
                be transmitted when the node detects a matching RTR
                request from a remote node.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) For Automatic RTR response, the RTR support must also
                   be enabled. */
#define     NU_CAN_AUTOMATIC_RTR_RESPONSE   1

/* DEFINE:      CAN_REJECT_SELF_TX_PACKET
   DEFAULT:     CAN_REJECT_SELF_TX_PACKET is set to 1.
   DESCRIPTION: A CAN node receives back the frame transmitted itself
                which is undesirable in most of the applications. This
                option avoids the overhead on the application by rejecting
                the self transmitted CAN frame at the lowest level.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) When set this option will not allow the reception of a
                   frame with any ID by itself except a message with ID 0.
                   0 ID message is the highest priority message in a CAN
                   network and most of the protocol make use of this for
                   network monitoring so, its reception is not rejected by
                   setting this option. */
#define     NU_CAN_REJECT_SELF_TX_PACKET        1

/* DEFINE:      NU_CAN_ENABLE_TIME_STAMP
   DEFAULT:     NU_CAN_ENABLE_TIME_STAMP is set to 0.
   DESCRIPTION: Some CAN controllers provide the facility of a time stamp,
                the moment a CAN frame is transmitted on the bus or
                received by the node successfully. This option will allow
                to forward that time stamp through a field in CAN_PACKET
                structure.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) This option may not be supported by all CAN drivers/
                   controllers. Check with your controller's documentation
                   for more detail. */
#define     NU_CAN_ENABLE_TIME_STAMP            0

/* DEFINE:      CAN_INQUEUE_SIZE and CAN_OUTQUEUE_SIZE
   DEFAULT:     Set to 64 and 32 respectively.
   DESCRIPTION: These defines allow the user to configure the length of
                the queue that handles incoming or outgoing CAN messages.
   NOTE:        1) Nucleus CAN, Nucleus CAN Driver and Nucleus CAN
                   demonstration for the corresponding OS must be
                   rebuilt after changing this option.
                2) Never set the values of these two defines to '0' */
#define     CAN_INQUEUE_SIZE                128
#define     CAN_OUTQUEUE_SIZE               64

/* Default baud rate for Nucleus CAN.  This is being set using the metadata
   option of CAN.*/
#define     CAN_DEFAULT_BAUDRATE            CFG_NU_OS_CONN_CAN_DEFAULT_BAUD_RATE

/* *********************************************************************

  Changes in the sections from this point onwards are not recommended for
  the user. They are intended to make sure that the configuration
  resulting from the options selected above are valid. If invalid it
  either displays a compile time error or adjusts the options
  automatically, where necessary.

************************************************************************/

/* Port ID to refer to the CAN driver ports integrated with Nucleus
   CAN. Values of these must not be changed. Maximum four CAN driver
   ports may be integrated with Nucleus CAN. */

#define     CAN_PORT1                   0
#define     CAN_PORT2                   (CAN_PORT1 + 1)
#define     CAN_PORT3                   (CAN_PORT2 + 1)
#define     CAN_PORT4                   (CAN_PORT3 + 1)

/* Driver ID for each driver in a port. Maximum eight drivers per port
   are supported. */

#define     CAN1_DRIVER                 0
#define     CAN2_DRIVER                 (CAN1_DRIVER + 1)
#define     CAN3_DRIVER                 (CAN2_DRIVER + 1)
#define     CAN4_DRIVER                 (CAN3_DRIVER + 1)
#define     CAN5_DRIVER                 (CAN4_DRIVER + 1)
#define     CAN6_DRIVER                 (CAN5_DRIVER + 1)
#define     CAN7_DRIVER                 (CAN6_DRIVER + 1)
#define     CAN8_DRIVER                 (CAN7_DRIVER + 1)

/* Enable or disable various options to match speed/size optimization. */
#if         (NU_CAN_OPTIMIZE_FOR_SIZE)

#undef      NU_CAN_DEBUG
#define     NU_CAN_DEBUG                    0

#undef      CAN_INQUEUE_SIZE
#define     CAN_INQUEUE_SIZE                16

#undef      CAN_OUTQUEUE_SIZE
#define     CAN_OUTQUEUE_SIZE               14

#endif      /* (NU_CAN_OPTIMIZE_FOR_SIZE) */

/* Make sure RTR services are supported for automatic RTR response. */
#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && !NU_CAN_SUPPORTS_RTR)

#undef      NU_CAN_SUPPORTS_RTR
#define     NU_CAN_SUPPORTS_RTR            1

#endif      /* (NU_CAN_AUTOMATIC_RTR_RESPONSE && !NU_CAN_SUPPORTS_RTR) */

#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)

#undef      NU_CAN_MULTIPLE_PORTS_SUPPORT
#define     NU_CAN_MULTIPLE_PORTS_SUPPORT   0

#undef      CAN_MAX_PORTS
#define     CAN_MAX_PORTS                   1

#undef      CAN_DEV_COUNT
#define     CAN_DEV_COUNT                   1

#undef      CAN_REJECT_SELF_TX_PACKET
#define     CAN_REJECT_SELF_TX_PACKET       0

#undef      CAN_INQUEUE_SIZE
#define     CAN_INQUEUE_SIZE                16

#undef      CAN_OUTQUEUE_SIZE
#define     CAN_OUTQUEUE_SIZE               2

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

#endif      /* !CAN_CFG_H */

