

/*	This file is used to configure (turn on/off) various 	
 *	PicoServer features.			
 */


/* Select the operating system:
 *
 * PS_NUCLEUS	(Accelerated Technology)
 * PS_VXWORKS	Vxworks
 * PS_LYNX	Lynx OS
 */

#define PS_NUCLEUS



/* To turn all PicoServer extended features on  insert the line
 * #define ALL_F_ON
 * after the comment.
 * (Note: this overrides the feature specific defines below)
 */


/* To turn all PicoServer extended features off  insert the line
 * #define ALL_F_OFF
 * after the comment.
 * (Note: this overrides the feature specific defines below)
 */


/* PicoServer can support a filesystem on mass storage or
 * a memory based filesystem that is compiled into the system
 * or uploaded useing the file upload feature. Picoserver can
 * also support a combination of the two file storage methods
 *
 * To enable an in memory filesystem:
 * #define FS_IN_MEMORY
 */

#define FS_IN_MEMORY

/* Enable the Picoserver directory command
 * This feature is a plugin that lists
 * the users incore filesystem on the
 * users browser.
 */

#define LIST_DIRECTORY



/* PicoServer can support file uploading useing multi-part mime
 * To enable file uploading 
 * #define FILE_UPLOAD
 */

#define	FILE_UPLOAD		/* include file upload processing*/


/* PicoServer can support file file compression of imbedded HTML
 * and other files
 * To support transparent file compression for both mass-storage
 * and in memory filesystems
 *
 * #define FILE_COMPRESSION
 */


#define FILE_COMPRESSION
// #undef FILE_COMPRESSION

 /* PicoServer can support secure user authentication 
  * useing a Java applet and DES encryption support in the server.
  * To support hard encrypted authentication
  */
#define AUTH_40_ENTICATION
 /* to select 40bit DES authentication
  * #define AUTH_56_ENTICATION
  * to select 56bit DES authentication
  * 
  * Note: only one of either (40 or 56 bit) authentication
  * may be enabled at one time.
  */


/* END OF USER CONFIGURATION SECTION */


#ifdef AUTH_56_ENTICATION
#ifndef AUTH_PLUGIN
#define AUTH_PLUGIN
#endif
#endif /* AUTH_56_ENTICATION */

#ifdef AUTH_40_ENTICATION
#ifndef AUTH_PLUGIN
#define AUTH_PLUGIN
#endif
#endif /* AUTH_40_ENTICATION

/* TURN ALL THE FEATURES ON IF USER SELECTED ALL_F_ON */

#ifdef ALL_F_ON

#ifndef FS_IN_MEMORY
#define FS_IN_MEMORY
#endif

#ifndef FILE_UPLOAD
#define FILE_UPLOAD
#endif

#ifndef LIST_DIRECTORY
#define LIST_DIRECTORY
#endif

#ifndef FILE_COMPRESSION
#define FILE_COMPRESSION
#endif

#ifndef AUTHENTICATION
#define AUTH_40_ENTICATION
#endif

#endif

/* TURN ALL THE FEATURES OFF */

#ifdef ALL_F_OFF

#ifdef FS_IN_MEMORY
#undef FS_IN_MEMORY
#endif

#ifdef FILE_UPLOAD
#undef FILE_UPLOAD
#endif

#ifdef LIST_DIRECTORY
#undef LIST_DIRECTORY
#endif

#ifdef FILE_COMPRESSION
#undef FILE_COMPRESSION
#endif

#ifdef AUTHENTICATION
#undef AUTHENTICATION
#endif

#endif

#ifdef	PS_NUCLEUS	
#include "ps_nuc.h"
#endif

#ifdef	PS_VXWORKS
#include "vxworks.h"
#endif

#ifdef	PS_LYNX		
#include "lynx.h"
#endif
