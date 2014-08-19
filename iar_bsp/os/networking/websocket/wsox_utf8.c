/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       wsox_utf8.c
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the Nucleus WebSocket routines for decoding a
*       stream of data to ensure it is proper UTF8 format.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       WSOX_Decode_UTF8
*       WSOX_Validate_UTF8
*
*   DEPENDENCIES
*
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"

STATIC UINT32 WSOX_Decode_UTF8(UINT32* state, UINT32* codep, UINT32 byte);

#define WSOX_UTF8_ACCEPT    0
#define WSOX_UTF8_REJECT    12

static const UINT8 WSOX_Utf8d[] = {
  /* The first part of the table maps bytes to character classes that
   * to reduce the size of the transition table and create bitmasks.
   */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  /* The second part is a transition table that maps a combination
   * of a state of the automaton and a character class to a state.
   */
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Decode_UTF8
*
*   DESCRIPTION
*
*       This routine checks that a byte is proper UTF8 format.
*
*   INPUTS
*
*       *state                  The state of the comparison.
*       *codep                  The current code point for the stream
*                               of data to which the byte being
*                               validated belongs.
*       byte                    The byte being checked.
*
*   OUTPUTS
*
*       The state of the comparison, which is the value at the offset
*       into the UTF8 array.
*
*************************************************************************/
STATIC UINT32 WSOX_Decode_UTF8(UINT32 *state, UINT32 *codep, UINT32 byte)
{
  UINT32 type = WSOX_Utf8d[byte];

  *codep = (*state != WSOX_UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = WSOX_Utf8d[256 + *state + type];
  return (*state);

} /* WSOX_Decode_UTF8 */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Validate_UTF8
*
*   DESCRIPTION
*
*       This routine checks that a data stream is proper UTF8 format.
*
*   INPUTS
*
*       *s                      The data stream to validate.
*       length                  The length of the stream.
*       *byte                   If an error occurred, the byte number
*                               at which the error was found.
*
*   OUTPUTS
*
*       NU_TRUE                 The data is valid.
*       NU_FALSE                The data is invalid.
*
*************************************************************************/
BOOLEAN WSOX_Validate_UTF8(CHAR *s, UINT64 length, UINT64 *byte)
{
  UINT32    codepoint, state = WSOX_UTF8_ACCEPT;
  UINT64    i;
  BOOLEAN   save_byte = NU_TRUE;

  for (i = 0; i < length; i++)
  {
      WSOX_Decode_UTF8(&state, &codepoint, *s++);

      /* This byte could be the start/middle of an encoded value. */
      if (state != WSOX_UTF8_ACCEPT)
      {
          /* If another suspicious byte has not already been found. */
          if (save_byte == NU_TRUE)
          {
              /* Save this byte. */
              *byte = i;

              /* Don't save any additional bytes until a good byte
               * is found.
               */
              save_byte = NU_FALSE;
          }
      }

      /* This byte is good.  Reset the suspicious byte indicator. */
      else
      {
          save_byte = NU_TRUE;
      }
  }

  return (state == WSOX_UTF8_ACCEPT);

} /* WSOX_Validate_UTF8 */
