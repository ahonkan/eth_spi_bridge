/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
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
*       keypad_ge_defs.h
*
*   COMPONENT
*
*       Keypad driver
*
*   DESCRIPTION
*
*       This file contains defines and constants for Nucleus Keypad Driver
*       generic component.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     KEYPAD_GE_DEFS
#define     KEYPAD_GE_DEFS

#include    "nucleus.h"

/* Define maximum timer count. */
#define     KP_MAX_TIMER_COUNT   \
            ESAL_GE_TMR_COUNT_CALC(ESAL_GE_TMR_OS_CLOCK_RATE, \
                                   ESAL_GE_TMR_OS_CLOCK_PRESCALE, \
                                   NU_PLUS_TICKS_PER_SEC)

/* Macro for determining timer maximum count. */

#ifdef      NU_COUNT_DOWN

/* The timer used is count down timer. */
#define     KP_CALC_TIME(start, end)          \
            ((start < end) ? ((KP_MAX_TIMER_COUNT - end) + start): \
            (start - end))

#else

/* The timer used is count up timer. */
#define     KP_CALC_TIME(start, end) \
            ((start < end) ? (end - start): \
            ((KP_MAX_TIMER_COUNT - end) + start))

#endif      /* NU_COUNT_DOWN */

/* Code to indicate no key pressed condition. */
#define     KP_NO_KEY                   0xFFFF

/* Define the event code for key down event. */
#define     KP_KEY_DOWN_EVENT           0x1

/* Define the event code for key up event. */
#define     KP_KEY_UP_EVENT             0x2

/* Define the event code for transfer complete event. Used if I2C or SPI
   is used as the interface.*/
#define     KP_TRANSFER_DONE_EVENT      0x4

/* Keypad driver HISR priority. */
#define     KP_HISR_PRIORITY            2

/* Keypad driver HISR stack size. */
#define     KP_HISR_STACK_SIZE          ((NU_MIN_STACK_SIZE * 3) / 2)

/* Keypad driver task priority. */
#define     KP_TASK_PRIORITY            0

/* Keypad driver task stack size. */
#define     KP_TASK_STACK_SIZE          ((NU_MIN_STACK_SIZE * 3) / 2)

#endif      /* KEYPAD_GE_DEFS */
