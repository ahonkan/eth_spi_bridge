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
*       ppec.c                                    
*
* COMPONENT
*
*       PPEC - PPPoE Driver Configuration
*
* DESCRIPTION
*
*       Functions and data structures that can be configured to
*       the needs of the application. Any other user-defined data
*       or functions specific to this driver should also be placed here.
*
* DATA STRUCTURES
*
*       PPEC_AC_Services
*       PPEC_Host_Services
*       PPEC_CookieMask
*
* FUNCTIONS
*
*       PPEC_Decode_Cookie
*       PPEC_Encode_Cookie
*       PPEC_VerifyCookie
*
* DEPENDENCIES
*
*       None
*
***************************************************************************/

#include "drivers/ppe_defs.h"
#include "drivers/ppe_extr.h"



/* This list of AC services should be modified, according to the
   definition of PPE_SERVICE, to define the contents of service
   information. Only the name field is used by default, but any
   unused fields should be given a null value. */
PPE_SERVICE PPEC_AC_Services[PPE_NUM_SERVICES] =
{
    {(UINT8*)"Service #1"},
    {(UINT8*)"Service #2"}
};



/* Define the host services using the same guidelines as the 
   AC services above. */
PPE_SERVICE PPEC_Host_Services[PPE_NUM_SERVICES] =
{
    {(UINT8*)"Service #1"},
    {(UINT8*)"Service #2"}
};



/* Just a random jumble of characters, used as a mask for encryption
   of our private session information. */
CHAR PPEC_CookieMask[PPE_COOKIE_SIZE + 1] = "KJE%RG@9";




/************************************************************************
* FUNCTION
*
*       PPEC_Encode_Cookie
*
* DESCRIPTION
*
*       Convert the peer's ethernet address and identifier into an
*       8 character HOSTUNIQ string and encrypt it using a simple
*       XOR operation using the PPEC_CookieMask. This can be modified
*       to improve the encryption or change the type of string to
*       produce.
*
* INPUTS
*
*       *macaddr        - Ethernet address of the peer.
*       sid             - An identifier for finding the correct vdevice.
*       *string         - A preallocated memory buffer for the results.
*
* OUTPUTS
*
*       None.
*
************************************************************************/
VOID PPEC_Encode_Cookie(UINT8 *macaddr, UINT16 sid, UINT8 *string)
{
    UINT8 i;

    /* Copy the ethernet address and "session id" to the given string
       memory. */
    memcpy(string, macaddr, 6);
    memcpy(&string[6], &sid, 2);

    /* Now encode it by simply XORing the cookie mask. */
    for (i = 0; i < PPE_COOKIE_SIZE; i++)
        string[i] ^= PPEC_CookieMask[i];
}




/************************************************************************
* FUNCTION
*
*       PPEC_Decode_Cookie
*
* DESCRIPTION
*
*       Uses a simple XOR algorithm against the PPEC_CookieMask to
*       decrypt a HOSTUNIQ string into its mac address and identifier
*       elements. It should essentially undo what the PPEC_Encode_Cookie
*       does. On return, the mac address can be found at the first 6
*       bytes of the string, and the identifier will be the last 2 bytes.
*       The identifier is also returned to the caller.
*
* INPUTS
*
*       *tag                - Pointer to the cookie tag.
*
* OUTPUTS
*
*       UINT16              - The vdevice identifier.
*
************************************************************************/
UINT16 PPEC_Decode_Cookie(PPE_TAG *tag)
{
    UINT8   i;
    UINT16  sid;

    /* Decode the cookie by XORing the cookie mask. What we should
       have left is the source ethernet address, and the "session id"
       we assigned it during the last discovery stage. */
    for (i = 0; i < PPE_COOKIE_SIZE; i++)
        tag->ppe_string[i] ^= PPEC_CookieMask[i];

    /* The sid is always in native order. */
    memcpy(&sid, &tag->ppe_string[6], 2);

    return sid;
}

