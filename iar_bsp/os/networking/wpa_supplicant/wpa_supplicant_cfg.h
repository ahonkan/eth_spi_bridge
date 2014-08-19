/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*	FILENAME
*
*		wpa_supplicant_cfg.h
*
*	DESCRIPTION
*
*		This file contains all the build-time configuration macros
*		that control the various configuration settings of the
*		WPA Supplicant.
*
*	DATA STRUCTURES
*
*		None
*
*	FUNCTIONS
*
*		None
*
*	DEPENDENCIES
*
*		None
*
*************************************************************************/
#ifndef WPA_SUPPLICANT_CFG_H
#define WPA_SUPPLICANT_CFG_H

#ifdef __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* Supplicant version. These should not be changed. */
#define WPA_SUPPLICANT_VERSION					"1.0"

/* Size of the incoming packets buffer in bytes. All incoming packets of
 * the wpa_supplicant are first copied into a buffer of this size and are
 * then processed. */
#define WPA_SUPPLICANT_INPUT_BUFFER_SIZE		4000


/*
 * WPA_SUPPLICANT CONFIGURATION PARAMETERS
 *   
 * The options below are equivalent to the command-line arguments passed
 * to wpa_supplicant daemon present in other operating systems. In
 * Nucleus, the supplicant is started up as a task. It's initial arguments
 * are specified using the below listed configuation macros.
 */

/* Specify a unique path to the control interface. Set this macro to NULL
 * if the control interface is disabled. This option is specified by the
 * switch "-g" in the command-line client. Its default value is NULL.
 */
#define WPA_SUPPLICANT_PARAM_CONTROL_INTERFACE		NULL

/* If a control interface is specified, then set this macro to a
 * non-zero value to make the wpa_supplicant wait for a monitor program
 * to connect to the control interface. This option is specified by "-W"
 * in the command-line client. Its default value is 0.
 */
#define WPA_SUPPLICANT_PARAM_WAIT_FOR_MONITOR		0

/* This option allows sending debug output to a file instead of serial
 * output. It should be set to path/filename to the debug output file.
 * This option is specified by the switch "-f" in the command-line
 * client. The default value of this macro is "wpa_supplicant_log.txt".
 */
#define WPA_SUPPLICANT_PARAM_DEBUG_FILE_PATH		"wpa_supplicant_log.txt"

/* Debugging verbosity level. Possible values of this macro are listed
 * below in increasing order of debug output:
 *
 * - MSG_MSGDUMP: Log packet dumps.
 * - MSG_DEBUG: Log debug level messages.
 * - MSG_INFO: Log informational messages.
 * - MSG_WARNING: Log warning messages.
 * - MSG_ERROR: Log error messages.
 *
 * This option is specified by the switch "-d" and "-q" in the command
 * line client. Its default value is MSG_DEBUG.
 */
#define WPA_SUPPLICANT_PARAM_DEBUG_LEVEL			MSG_DEBUG

/* The following macro controls whether keys and passwords should be
 * displayed in debug output. Enabling this option facilitates debugging
 * but it is a potential security thread if this option is enabled in
 * production grade systems. This option is specified by the switch "-K"
 * in the command-line client. Set this macro to a non-zero value to
 * enable this option. The default value of this option is 1.
 */
#define WPA_SUPPLICANT_PARAM_DEBUG_SHOW_KEYS		1

/* Include timestamp in debug messages? To enable this option, set this
 * macro to a non-zero. This option is specified by the switch "-t" in
 * the command-line client. Its default value is 0.
 */
#define WPA_SUPPLICANT_PARAM_DEBUG_SHOW_TIMESTAMP	0

/*
 * TASK CONFIGURATIONS
 *
 * The options below allow configuring OS task related settings of
 * the wpa_supplicant. These are usually not modified.
 */

/* Configure Nucleus main task that starts the wpa_supplicant. This is
 * the task which runs the wpa_supplicant daemon. Most of the processing
 * of the wpa_supplicant takes place in the context of this task. */
#define WPA_SUPPLICANT_MAIN_TASK_STACK_SIZE		10000
#define WPA_SUPPLICANT_MAIN_TASK_PRIORITY		4
#define WPA_SUPPLICANT_MAIN_TASK_TIME_SLICE		0
#define WPA_SUPPLICANT_MAIN_TASK_PREEMPT   		NU_PREEMPT

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef WPA_SUPPLICANT_CFG_H */
