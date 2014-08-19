/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       asn1.c                                                   
*
*   DESCRIPTION
*
*       This file contains all ASN1 initialization and syntax
*       verification functions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Asn1Opn
*       Asn1Cls
*       Asn1OctEnc
*       Asn1OctDec
*       Asn1TagEnc
*       Asn1TagDec
*       Asn1IdrEnc
*       Asn1IdrDec
*       Asn1LenEnc
*       Asn1LenDec
*       Asn1HdrEnc
*       Asn1HdrDec
*       Asn1Eoc
*       Asn1EocEnc
*       Asn1EocDec
*       Asn1NulEnc
*       Asn1NulDec
*       Asn1BolDec
*       Asn1IntEnc
*       Asn1IntDec
*       Asn1IntEncLng
*       Asn1IntDecLng
*       Asn1IntEncUns
*       Asn1IntDecUns
*       Asn1IntEncLngUns
*       Asn1IntDecLngUns
*       Asn1EncCounter64
*       Asn1DecCounter64
*       Asn1BtsEnc
*       Asn1BtsDec
*       Asn1OtsEnc
*       Asn1OtsDec
*       Asn1SbiEnc
*       Asn1SbiDec
*       Asn1OjiEnc
*       Asn1OjiDec
*
*   DEPENDENCIES
*
*       asn1.h
*
*************************************************************************/

#include "networking/asn1.h"

/************************************************************************
*
*   FUNCTION
*
*       Asn1Opn
*
*   DESCRIPTION
*
*       Sets up the memory region for the asn1_sck_t data structure
*       passed in that will be used for encoding or decoding purposes.
*
*   INPUTS
*
*       *Asn1                   Pointer to the asn1_sck_t data structure
*                               that will be used for encoding or
*                               decoding purposes.
*       *Buf                    A pointer to the memory that the
*                               asn1_sck_t data structure will use as a
*                               buffer.
*       Len                     The length of the buffer.
*       Mde                     ASN1_ENC if opening the data structure
*                               for encoding, ASN1_DEC if opening the
*                               data structure for decoding.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Asn1Opn(asn1_sck_t *Asn1, UINT8 *Buf, UINT32 Len, UINT32 Mde)
{
    Asn1->Begin = Buf;
    Asn1->End = Buf + Len;

    if (Mde == ASN1_ENC)
        Asn1->Pointer = Buf + Len;
    else
        Asn1->Pointer = Buf;

} /* Asn1Opn */

/************************************************************************
*
*   FUNCTION
*
*       Asn1Cls
*
*   DESCRIPTION
*
*       This function copies the contents of the asn1_sck_t data
*       structure's buffer (either encoded or decoded data) into the
*       buffer provided and copies the length of the buffer into *Len.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               from which to extract data.
*       **Buf                   A pointer to the pointer of memory store
*                               the data from the asn1_sck_t data
*                               structure.
*       *Len                    A pointer to the place in memory to store
*                               the length of the buffer.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Asn1Cls(const asn1_sck_t *Asn1, UINT8 **Buf, UINT32 *Len)
{
    *Buf = Asn1->Pointer;
    *Len = (UINT32)(Asn1->End - Asn1->Pointer);

} /* Asn1Cls */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OctEnc
*
*   DESCRIPTION
*
*       This function encodes the data in Chr into the Pointer parameter
*       of the asn1_sck_t data structure Asn1.
*
*   INPUTS
*
*       *Asn1                   A pointer to the data structure that will
*                               hold the encoded data.
*       Chr                     The data to encode.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1OctEnc(asn1_sck_t *Asn1, UINT8 Chr)
{
    BOOLEAN    success = NU_TRUE;

    if (Asn1->Pointer <= Asn1->Begin)
    {
        success = NU_FALSE;
    }

    else
        *--(Asn1->Pointer) = Chr;

    return (success);

} /* Asn1OctEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OctDec
*
*   DESCRIPTION
*
*       This function decodes an octet from the asn1_sck_t data structure
*       and places it in the buffer provided.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data
*                               structure that holds the encoded data.
*       *Chr                    A pointer to the buffer that will hold
*                               the decoded octet.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                There was nothing left in the asn1_sck_t
*                               data structure to decode.
*
*************************************************************************/
BOOLEAN Asn1OctDec(asn1_sck_t *Asn1, UINT8 *Chr)
{
    BOOLEAN    success = NU_TRUE;

    if (Asn1->Pointer >= Asn1->End)
    {
        success = NU_FALSE;
    }
    else
        *Chr = *(Asn1->Pointer)++;

    return (success);

} /* Asn1OctDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1TagEnc
*
*   DESCRIPTION
*
*       This function encodes the data in Tag into the Pointer parameter
*       of the asn1_sck_t data structure Asn1.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       Tag                     The data to encode into the asn1_sck_t
*                               data structure.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1TagEnc(asn1_sck_t *Asn1, UINT32 Tag)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    Chr = (UINT8) (Tag & 0x7F);
    Tag >>= 7;

    if (!Asn1OctEnc (Asn1, Chr))
        success = NU_FALSE;

    else
    {
        while (Tag > 0)
        {
            Chr = (UINT8) (Tag | 0x80);
            Tag >>= 7;

            if (!Asn1OctEnc (Asn1, Chr))
            {
                success = NU_FALSE;
                break;
            }
        }
    }

    return (success);

} /* Asn1TagEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1TagDec
*
*   DESCRIPTION
*
*       This function decodes a Tag from the asn1_sck_t data structure
*       and places it in the buffer provided.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Tag                    A pointer to the buffer that will hold
*                               the decoded Tag.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                There was nothing left in the asn1_sck_t
*                               data structure to decode.
*
*************************************************************************/
BOOLEAN Asn1TagDec(asn1_sck_t *Asn1, UINT32 *Tag)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    *Tag = 0;

    do
    {
        if (!Asn1OctDec (Asn1, &Chr))
        {
            success = NU_FALSE;
            break;
        }

        *Tag <<= 7;
        *Tag |= (UINT32)(Chr & 0x7F);

    } while ((Chr & 0x80) == 0x80);

    return (success);

} /* Asn1TagDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IdrEnc
*
*   DESCRIPTION
*
*       This function encodes the data in Tag, Chr and Con and places it
*       in the asn1 data structure.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       Cls                     The data to encode into the asn1_sck_t
*                               data structure.
*       Con                     The data to encode into the asn1_sck_t
*                               data structure.
*       Tag                     The data to encode into the asn1_sck_t
*                               data structure.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1IdrEnc(asn1_sck_t *Asn1, UINT32 Cls, UINT32 Con, UINT32 Tag)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    if (Tag >= 0x1F)
    {
        if (!Asn1TagEnc (Asn1, Tag))
            success = NU_FALSE;

        Tag = 0x1F;
    }

    if (success == NU_TRUE)
    {
        Chr = (UINT8) ((Cls << 6) | (Con << 5) | (Tag));

        if (!Asn1OctEnc (Asn1, Chr))
            success = NU_FALSE;
    }

    return (success);

} /* Asn1IdrEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IdrDec
*
*   DESCRIPTION
*
*       This function decodes the data in the asn1 data structure and
*       places it in Tag, Chr and Con.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Cls                    The data to decode from the asn1_sck_t
*                               data structure.
*       *Con                    The data to decode from the asn1_sck_t
*                               data structure.
*       *Tag                    The data to decode from the asn1_sck_t
*                               data structure.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1IdrDec(asn1_sck_t *Asn1, UINT32 *Cls, UINT32 *Con, UINT32 *Tag)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        *Cls = (UINT32)((Chr & 0xC0) >> 6);
        *Con = (UINT32)((Chr & 0x20) >> 5);
        *Tag = (UINT32)((Chr & 0x1F));

        if (*Tag == 0x1F)
        {
            if (!Asn1TagDec (Asn1, Tag))
                success = NU_FALSE;
        }
    }

    return (success);

} /* Asn1IdrDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1LenEnc
*
*   DESCRIPTION
*
*       This function encodes the data in Def, and places it in the
*       asn1 data structure.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       Def                     A pointer to the data to be encoded.
*       Len                     The length of the data to be encoded.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1LenEnc(asn1_sck_t *Asn1, UINT32 Def, UINT32 Len)
{
    UINT8   Chr = 0, Cnt;
    BOOLEAN    success = NU_TRUE;

    if (!Def)
        /* Chr = 0x80; */
        Chr = 0x00;
    else
    {
        if (Len < 0x80)
            Chr = (UINT8) Len;
        else
        {
            Cnt = 0;

            while (Len > 0)
            {
                Chr = (UINT8) Len;
                Len >>= 8;
                if (!Asn1OctEnc (Asn1, Chr))
                {
                    success = NU_FALSE;
                    break;
                }
                Cnt++;
            }

            if (success == NU_TRUE)
                Chr = (UINT8) (Cnt | 0x80);
        }
    }

    if (success == NU_TRUE)
    {
        if (!Asn1OctEnc (Asn1, Chr))
            success = NU_FALSE;
    }

    return (success);

} /* Asn1LenEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1LenDec
*
*   DESCRIPTION
*
*       This function decodes the data in the asn1_sck_t data structure
*       and places it in Def and Len.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Def                    A pointer into which to place the Def.
*       *Len                    A pointer into which to place the Len.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1LenDec(asn1_sck_t *Asn1, UINT32 *Def, UINT32 *Len)
{
    UINT8   Chr, Cnt;
    BOOLEAN    success = NU_TRUE;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;
    else if (Chr == 0x80)
        *Def = 0;
    else
    {
        *Def = 1;

        if (Chr < 0x80)
            *Len = Chr;
        else
        {
            Cnt = (UINT8) (Chr & 0x7F);
            *Len = 0;
            while (Cnt > 0)
            {
                if (!Asn1OctDec (Asn1, &Chr))
                {
                    success = NU_FALSE;
                    break;
                }

                *Len <<= 8;
                *Len |= Chr;
                Cnt--;
            }
        }
    }

    return (success);

} /* Asn1LenDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1HdrEnc
*
*   DESCRIPTION
*
*       This function encodes a header of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       Cls                     The ASN1 class.
*       Con                     The ASN1 primitive.
*       Tag                     The ASN1 tag.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1HdrEnc(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT32 Cls, UINT32 Con,
                UINT32 Tag)
{
    UINT32  Def, Len;
    BOOLEAN    success = NU_TRUE;

    if (Eoc == 0)
    {
        Def = 0;
        Len = 0;
    }

    else
    {
        Def = 1;
        Len = (UINT32)(Eoc - Asn1->Pointer);
    }

    if (!Asn1LenEnc (Asn1, Def, Len))
        success = NU_FALSE;

    else if (!Asn1IdrEnc (Asn1, Cls, Con, Tag))
        success = NU_FALSE;

    return (success);

} /* Asn1HdrEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1HdrDec
*
*   DESCRIPTION
*
*       This function decodes a header of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       *Cls                    The ASN1 class.
*       *Con                    The ASN1 primitive.
*       *Tag                    The ASN1 tag.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1HdrDec(asn1_sck_t *Asn1, UINT8 **Eoc, UINT32 *Cls, UINT32 *Con,
                UINT32 *Tag)
{
    UINT32  Def, Len;
    BOOLEAN    success = NU_TRUE;

    if (!Asn1IdrDec (Asn1, Cls, Con, Tag))
        success = NU_FALSE;

    else if (!Asn1LenDec (Asn1, &Def, &Len))
        success = NU_FALSE;

    else if ( (Def) &&
              (((UINT32)(Asn1->Pointer + Len)) <= ((UINT32)(Asn1->End))) )
        *Eoc = Asn1->Pointer + Len;

    else
        *Eoc = 0;

    return (success);

} /* Asn1HdrDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1Eoc
*
*   DESCRIPTION
*
*       This function checks that there is data remaining to be encoded.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*
*   OUTPUTS
*
*       NU_TRUE                 There is data remaining to be encoded.
*       NU_FALSE                There is no data remaining to be encoded.
*
*************************************************************************/
BOOLEAN Asn1Eoc(const asn1_sck_t *Asn1, const UINT8 *Eoc)
{
    BOOLEAN    success = NU_TRUE;

    if (Eoc == 0)
    {
        if ( (Asn1->Pointer [0] != 0x00) || (Asn1->Pointer[1] != 0x00) )
            success = NU_FALSE;
    }

    else if (Asn1->Pointer < Eoc)
        success = NU_FALSE;

    return (success);

} /* Asn1Eoc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1EocEnc
*
*   DESCRIPTION
*
*       This function encodes an octet of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1EocEnc(asn1_sck_t *Asn1, UINT8 **Eoc)
{
    BOOLEAN    success = NU_TRUE;

    if (Eoc == 0)
    {
        if (!Asn1OctEnc (Asn1, 0x00))
            success = NU_FALSE;
        else if (!Asn1OctEnc (Asn1, 0x00))
            success = NU_FALSE;
    }
    else
        *Eoc = Asn1->Pointer;

    return (success);

} /* Asn1EocEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1EocDec
*
*   DESCRIPTION
*
*       This function decodes an octet of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1EocDec(asn1_sck_t *Asn1, const UINT8 *Eoc)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    if (Eoc == 0)
    {
        if (!Asn1OctDec (Asn1, &Chr))
            success = NU_FALSE;

        else if (Chr != 0x00)
        {
            success = NU_FALSE;
        }

        else if (!Asn1OctDec (Asn1, &Chr))
            success = NU_FALSE;

        else if (Chr != 0x00)
        {
            success = NU_FALSE;
        }
    }

    else
    {
        if (Asn1->Pointer != Eoc)
        {
            success = NU_FALSE;
        }
    }

    return (success);

} /* Asn1EocDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1NulEnc
*
*   DESCRIPTION
*
*       This function encodes a NU_NULL data type.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1NulEnc(const asn1_sck_t *Asn1, UINT8 **Eoc)
{
    *Eoc = Asn1->Pointer;

    return (NU_TRUE);

} /* Asn1NulEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1NulDec
*
*   DESCRIPTION
*
*       This function decodes a NU_NULL data type.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1NulDec(asn1_sck_t *Asn1, UINT8 *Eoc)
{
    Asn1->Pointer = Eoc;

    return (NU_TRUE);

} /* Asn1NulDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1BolEnc
*
*   DESCRIPTION
*
*       This function encodes a boolean data type.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Bol                     The boolean value.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1BolEnc(asn1_sck_t *Asn1, UINT8 **Eoc, BOOLEAN Bol)
{
    UINT8   Chr = 0;
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;

    if (Bol == 0xFF)
        Chr = (UINT8)0x00;

    if (!Asn1OctEnc (Asn1, Chr))
        success = NU_FALSE;

    return (success);

} /* Asn1BolEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1BolDec
*
*   DESCRIPTION
*
*       This function decodes a boolean of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Bol                    The boolean value.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1BolDec(asn1_sck_t *Asn1, const UINT8 *Eoc, BOOLEAN *Bol)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        if (Chr == 1)
            *Bol = 0;

        if (Asn1->Pointer != Eoc)
        {
            success = NU_FALSE;
        }
    }

    return (success);

} /* Asn1BolDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntEnc
*
*   DESCRIPTION
*
*       This function encodes an integer value.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Int                     The integer value.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1IntEnc(asn1_sck_t *Asn1, UINT8 **Eoc, INT32 Int)
{
    UINT8   Chr,Sgn;
    BOOLEAN    success = NU_TRUE;
    INT32   Lim;

    *Eoc = Asn1->Pointer;

    if (Int < 0)
    {
        Lim = -1;
        Sgn = 0x80;
    }

    else
    {
        Lim = 0;
        Sgn = 0x00;
    }

    do
    {
        Chr = (UINT8) Int;
        Int >>= 8;

        if (!Asn1OctEnc (Asn1, Chr))
        {
            success = NU_FALSE;
            break;
        }
    } while ((Int != Lim) || ((UINT8) (Chr & 0x80) != Sgn));

    return (success);

} /* Asn1IntEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntDec
*
*   DESCRIPTION
*
*       This function decodes an integer value.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Int                    The integer value.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1IntDec(asn1_sck_t *Asn1, const UINT8 *Eoc, INT32 *Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;
    UINT32  Len;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        if (Chr & 0x80)
        {
            *Int = (INT32) 0xFFFFFF00;
            *Int |= Chr;
        }
        else
        {
            *Int = (INT32) Chr;
        }

        Len = 1;

        while (Asn1->Pointer < Eoc)
        {
            if (++Len > sizeof (INT32))
            {

                success = NU_FALSE;
                break;
            }

            if (!Asn1OctDec (Asn1, &Chr))
            {
                success = NU_FALSE;
                break;
            }

            *Int <<= 8;
            *Int |= Chr;
        }
    }

    return (success);

} /* Asn1IntDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntEncLng
*
*   DESCRIPTION
*
*       This function encodes a long integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Int                     The long int of data.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1IntEncLng(asn1_sck_t *Asn1, UINT8 **Eoc, UINT32 Int)
{
    UINT8   Chr, Sgn;
    BOOLEAN    success = NU_TRUE;
    INT32   Lim;

    *Eoc = Asn1->Pointer;

    if (((INT32)Int) < 0)
    {
        Lim = -1;
        Sgn = 0x80;
    }
    else
    {
        Lim = 0;
        Sgn = 0x00;
    }

    do
    {
        Chr = (UINT8) Int;
        Int >>= 8;

        if (!Asn1OctEnc (Asn1, Chr))
        {
            success = NU_FALSE;
            break;
        }
    } while ((Int != (UINT32)Lim) || (UINT8) (Chr & 0x80) != Sgn);

    return (success);

} /* Asn1IntEncLng */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntDecLng
*
*   DESCRIPTION
*
*       This function decodes a long integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Int                    The integer value.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1IntDecLng(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT32 *Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;
    UINT32  Len;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        if (Chr & 0x80)
        {
            *Int = (UINT32) 0xFFFFFF00;
            *Int |= Chr;
        }

        else
        {
            *Int = (UINT32) Chr;
        }
        Len = 1;

        while (Asn1->Pointer < Eoc)
        {
            if (++Len > sizeof (UINT32))
            {
                success = NU_FALSE;
                break;
            }

            if (!Asn1OctDec (Asn1, &Chr))
            {
                success = NU_FALSE;
                break;
            }

            *Int <<= 8;
            *Int |= Chr;
        }
    }

    return (success);

} /* Asn1IntDecLng */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntEncUns
*
*   DESCRIPTION
*
*       This function encodes an unsigned integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Int                     The unsigned integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1IntEncUns(asn1_sck_t *Asn1, UINT8  **Eoc, UINT32 Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;

    do
    {
        Chr = (UINT8) Int;
        Int >>= 8;

        if (!Asn1OctEnc (Asn1, Chr))
        {
            success = NU_FALSE;
            break;
        }
    } while ((Int != 0) || (Chr & 0x80) != 0x00);

    return (success);

} /* Asn1IntEncUns */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntDecUns
*
*   DESCRIPTION
*
*       This function decodes an unsigned integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Int                    The value of the unsigned integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1IntDecUns(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT32 *Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;
    UINT32  Len;

    if (!Asn1OctDec(Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        *Int = Chr;

        if (Chr == 0)
            Len = 0;
        else
            Len = 1;

        while (Asn1->Pointer < Eoc)
        {
            if (++Len > sizeof(INT32))
            {
                success = NU_FALSE;
                break;
            }

            if (!Asn1OctDec(Asn1, &Chr))
            {
                success = NU_FALSE;
                break;
            }

            *Int <<= 8;
            *Int |= Chr;
        }
    }

    return (success);

} /* Asn1IntDecUns */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntEncLngUns
*
*   DESCRIPTION
*
*       This function encodes an unsigned long integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Int                     The unsigned long integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1IntEncLngUns(asn1_sck_t *Asn1, UINT8 **Eoc, UINT32 Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;

    do
    {
        Chr = (UINT8) Int;
        Int >>= 8;

        if (!Asn1OctEnc (Asn1, Chr))
        {
            success = NU_FALSE;
            break;
        }
    } while ((Int != 0) || (Chr & 0x80) != 0x00);

    return (success);

} /* Asn1IntEncLngUns */

/************************************************************************
*
*   FUNCTION
*
*       Asn1IntDecLngUns
*
*   DESCRIPTION
*
*       This function decodes an unsigned integer.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Int                    The unsigned long integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1IntDecLngUns(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT32 *Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;
    UINT32  Len;

    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        *Int = Chr;
        if (Chr == 0)
            Len = 0;
        else
            Len = 1;

        while (Asn1->Pointer < Eoc)
        {
            if (++Len > sizeof (UINT32))
            {
                success = NU_FALSE;
                break;
            }

            if (!Asn1OctDec (Asn1, &Chr))
            {
                success = NU_FALSE;
                break;
            }

            *Int <<= 8;
            *Int |= Chr;
        }
    }

    return (success);

} /* Asn1IntDecLngUns */

/************************************************************************
*
*   FUNCTION
*
*       Asn1EncCounter64
*
*   DESCRIPTION
*
*       This function encodes an unsigned integer maximum of 64 bit and
*       in decimal is (18446744073709551615).
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       Int                     The unsigned long integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1EncCounter64(asn1_sck_t *Asn1, UINT8 **Eoc, COUNTER64 Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;

    do {

        Chr = (UINT8) Int[0];
        Int[0] >>= 8;
        Int[0] |= ((Int[1] & 0xff) << 24);
        Int[1] >>= 8;

        if (!Asn1OctEnc (Asn1, Chr))
        {
            success = NU_FALSE;
            break;
        }

    } while((Int[0] != 0) || (Int[1] != 0) || ((Chr & 0x80) != 0x00));

    return (success);

} /* Asn1EncCounter64 */

/************************************************************************
*
*   FUNCTION
*
*       Asn1DecCounter64
*
*   DESCRIPTION
*
*       This function decodes an unsigned integer maximum of 64 bit and
*       in decimal is (18446744073709551615).
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Int                    The unsigned long integer.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1DecCounter64(asn1_sck_t *Asn1, const UINT8 *Eoc, COUNTER64 *Int)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;
    UINT32  Len = 0;

    UINT32  mostSig32Bits  = 0;
    UINT32  leastSig32Bits = 0;

    /*decoding most significant 32 bits (4 bytes)*/
    if (!Asn1OctDec (Asn1, &Chr))
        success = NU_FALSE;

    else
    {
        leastSig32Bits = Chr;
        if (Chr == 0)
            Len = 0;
        else
            Len = 1;

        while (Asn1->Pointer < Eoc)
        {
            if (++Len > sizeof (UINT32))
            {
                break;
            }

            if (!Asn1OctDec (Asn1, &Chr))
            {
                success = NU_FALSE;
                break;
            }

            leastSig32Bits <<= 8;
            leastSig32Bits |= Chr;
        }
    }

    if ((success == NU_TRUE) && (Len > sizeof(UINT32)))
    {
        /*decoding least significant 32 bits (4 bytes)*/
        if (!Asn1OctDec (Asn1, &Chr))
            success = NU_FALSE;

        else
        {
            mostSig32Bits = (leastSig32Bits>>24);
            leastSig32Bits  <<= 8;
            leastSig32Bits  |= Chr;
            Len = 1;

            while (Asn1->Pointer < Eoc)
            {
                if (++Len > sizeof (UINT32))
                {
                    success = NU_FALSE;
                    break;
                }

                if (!Asn1OctDec (Asn1, &Chr))
                {
                    success = NU_FALSE;
                    break;
                }

                mostSig32Bits <<= 8;
                mostSig32Bits |= (leastSig32Bits >> 24);
                leastSig32Bits <<= 8;
                leastSig32Bits |= Chr;
            }
        }
    }

    if(success == NU_TRUE)
    {
        (*Int)[0] = leastSig32Bits;
        (*Int)[1] = mostSig32Bits;
    }

    return (success);

} /* Asn1DecCounter64 */

/************************************************************************
*
*   FUNCTION
*
*       Asn1BtsEnc
*
*   DESCRIPTION
*
*       This function encodes a buffer of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       *Bts                    A pointer to the data to encode.
*       BtsLen                  The data to encode.
*       BtsUnu                  The data to encode.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1BtsEnc(asn1_sck_t *Asn1, UINT8 **Eoc, const UINT8 *Bts,
                UINT32 BtsLen, UINT8 BtsUnu)
{
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;
    Bts += BtsLen;

    while (BtsLen-- > 0)
    {
        if (!Asn1OctEnc (Asn1, *--Bts))
        {
            success = NU_FALSE;
            break;
        }
    }

    if (success == NU_TRUE)
    {
        if (!Asn1OctEnc (Asn1, BtsUnu))
            success = NU_FALSE;
    }

    return (success);

} /* Asn1BtsEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1BtsDec
*
*   DESCRIPTION
*
*       This function decodes a buffer of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Bts                    A buffer into which to place the decoded
*                               data.
*       BtsSze                  The length of the data to decode.
*       *BtsLen                 The length of the decoded data.
*       *BtsUnu                 An octet of decoded data.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1BtsDec(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT8 *Bts,
                UINT32 BtsSze, UINT32 *BtsLen, UINT8 *BtsUnu)
{
    BOOLEAN    success = NU_TRUE;

    if (!Asn1OctDec (Asn1, BtsUnu))
        success = NU_FALSE;

    else
    {
        *BtsLen = 0;

        while (Asn1->Pointer < Eoc)
        {
            if (++(*BtsLen) > BtsSze)
            {
                success = NU_FALSE;
                break;
            }

            if (!Asn1OctDec (Asn1, (UINT8 *)Bts++))
            {
                success = NU_FALSE;
                break;
            }
        }
    }

    return (success);

} /* Asn1BtsDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OtsEnc
*
*   DESCRIPTION
*
*       This function encodes a buffer of opaque data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       *Ots                    A pointer to the data to encode.
*       OtsLen                  The length of the data to encode.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1OtsEnc(asn1_sck_t *Asn1, UINT8 **Eoc, const UINT8 *Ots,
                UINT32 OtsLen)
{
    BOOLEAN    success = NU_TRUE;

    *Eoc = Asn1->Pointer;
    Ots += OtsLen;

    while (OtsLen-- > 0)
    {
        if (!Asn1OctEnc (Asn1, *--Ots))
        {
            success = NU_FALSE;
            break;
        }
    }

    return (success);

} /* Asn1OtsEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OtsDec
*
*   DESCRIPTION
*
*       This function decodes a buffer of opaque data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Ots                    A buffer into which to place the decoded
*                               data.
*       OtsSze                  The length to the data to decode.
*       *OtsLen                 The length of the decoded data.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1OtsDec(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT8 *Ots,
                UINT32 OtsSze, UINT32 *OtsLen)
{
    BOOLEAN    success = NU_TRUE;

    *OtsLen = 0;

    while (Asn1->Pointer < Eoc)
    {
        if (++(*OtsLen) > OtsSze)
        {
            success = NU_FALSE;
            break;
        }
        if (!Asn1OctDec (Asn1, (UINT8 *)Ots++))
        {
            success = NU_FALSE;
            break;
        }
    }

    return (success);

} /* Asn1OtsDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1SbiEnc
*
*   DESCRIPTION
*
*       This function encodes a buffer of data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       Sbi                     The data to encode.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1SbiEnc(asn1_sck_t *Asn1, UINT32 Sbi)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    Chr = (UINT8) (Sbi & 0x7F);
    Sbi >>= 7;

    if (!Asn1OctEnc (Asn1, Chr))
        success = NU_FALSE;

    else
    {
        while (Sbi > 0)
        {
            Chr = (UINT8) (Sbi | 0x80);
            Sbi >>= 7;

            if (!Asn1OctEnc (Asn1, Chr))
            {
                success = NU_FALSE;
                break;
            }
        }
    }

    return (success);

} /* Asn1SbiEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1SbiDec
*
*   DESCRIPTION
*
*       This function decodes a buffer of opaque data.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Sbi                    A buffer into which to place the decoded
*                               data.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1SbiDec(asn1_sck_t *Asn1, UINT32 *Sbi)
{
    UINT8   Chr;
    BOOLEAN    success = NU_TRUE;

    *Sbi = 0;

    do
    {
        if (!Asn1OctDec (Asn1, &Chr))
        {
            success = NU_FALSE;
            break;
        }

        *Sbi <<= 7;
        *Sbi |= (UINT32)(Chr & 0x7F);
    } while ((Chr & 0x80) == 0x80);

    return (success);

} /* Asn1SbiDec */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OjiEnc
*
*   DESCRIPTION
*
*       This function encodes an object identifier.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that will hold the encoded data.
*       **Eoc                   A pointer to the end of the buffer.
*       *Oji                    A pointer to the object identifier to
*                               encode.
*       OjiLen                  The length of the data to encode.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully encoded.
*       NU_FALSE                The asn1_sck_t data structure is full.
*
*************************************************************************/
BOOLEAN Asn1OjiEnc(asn1_sck_t *Asn1, UINT8 **Eoc, const UINT32 *Oji,
                UINT32 OjiLen)
{
    BOOLEAN    success = NU_TRUE;
    UINT32  Sbi;

    *Eoc = Asn1->Pointer;

    if (OjiLen < 2)
    {
        success = NU_FALSE;
    }

    else
    {
        Sbi = Oji [1] + Oji [0] * 40;
        Oji += OjiLen;

        while (OjiLen-- > 2)
        {
            if (!Asn1SbiEnc (Asn1, *--Oji))
            {
                success = NU_FALSE;
                break;
            }
        }

        if (!Asn1SbiEnc (Asn1, Sbi))
            success = NU_FALSE;
    }

    return (success);

} /* Asn1OjiEnc */

/************************************************************************
*
*   FUNCTION
*
*       Asn1OjiDec
*
*   DESCRIPTION
*
*       This function decodes an object identifier.
*
*   INPUTS
*
*       *Asn1                   A pointer to the asn1_sck_t data structure
*                               that holds the encoded data.
*       *Eoc                    A pointer to the end of the buffer.
*       *Oji                    A buffer into which to place the decoded
*                               data.
*       OjiSize                 The size of the data to decode.
*       *OjiLen                 The size of the decoded data.
*
*   OUTPUTS
*
*       NU_TRUE                 Successfully decoded.
*       NU_FALSE                Unsuccessful.
*
*************************************************************************/
BOOLEAN Asn1OjiDec(asn1_sck_t *Asn1, const UINT8 *Eoc, UINT32 *Oji,
                UINT32 OjiSze, UINT32 *OjiLen)
{
    BOOLEAN    success = NU_TRUE;
    UINT32  Sbi;

    if (OjiSze < 2)
    {
        success = NU_FALSE;
    }

    else if (!Asn1SbiDec (Asn1, &Sbi))
        success = NU_FALSE;

    else if (Sbi < 40)
    {
        Oji [0] = 0;
        Oji [1] = Sbi;
    }
    else
    {
        if (Sbi < 80)
        {
            Oji [0] = 1;
            Oji [1] = Sbi - 40;
        }
        else
        {
            Oji [0] = 2;
            Oji [1] = Sbi - 80;
        }
    }

    if (success == NU_TRUE)
    {
        *OjiLen = 2;
        Oji += 2;

        while (Asn1->Pointer < Eoc)
        {
            if (++(*OjiLen) > OjiSze)
            {
                success = NU_FALSE;
                break;
            }
            if (!Asn1SbiDec (Asn1, Oji++))
            {
                success = NU_FALSE;
                break;
            }
        }
    }

    return (success);

} /* Asn1OjiDec */




