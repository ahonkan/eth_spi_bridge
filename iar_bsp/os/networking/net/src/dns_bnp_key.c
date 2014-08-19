/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       dns_bnp_key.c
*
*   DESCRIPTION
*
*       This file contains the API to Build and Parse a Key/Value pairs
*       stored in a TXT record in the DNS host database.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_DNS_Build_Key
*       NU_DNS_Parse_Key
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_DNS_Build_Key
*
*   DESCRIPTION
*
*       This routine is used to build a key/value pair to be stored in
*       the appropriate TXT record of the DNS database. The key/value
*       pair is built from the specified Key/Value data passed into
*       this routine as a string.
*
*   INPUTS
*
*       *buffer                Pointer to the memory where the formatted
*                              key would be stored.
*       *kv_pair               A key/value pairs to be added to the TXT
*                              record. The key/value pairs must be of the
*                              following format:
*                               "<key1>=<value1>\r\n<key2>=<value2>\r\n..."
*       length                 Maximum size of the buffer.
*
*   OUTPUTS
*
*       NU_INVALID_PARM        An input parameter is invalid.
*       NU_NO_MEMORY           The User buffer does not have enough space.
*       NU_INVALID_SIZE        One of the key/value pair is of invalid
*                              length.
*       This routine returns the size of the key that was added to the
*       TXT record.
*
*************************************************************************/
INT32 NU_DNS_Build_Key(CHAR *buffer, const char *kv_pair, INT32 length)
{
#if (INCLUDE_MDNS == NU_TRUE)
    INT32           total_in_kv_len, i, total_out_kv_len = 0;
    UINT8           kv_len;
    CHAR            *temp_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (buffer == NU_NULL) || (kv_pair == NU_NULL) || (length <= 0) )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Calculate the total length of the incoming key/value pair
     * strings.
     */
    total_in_kv_len = strlen(kv_pair);

    /* Ensure that we have enough space in the buffer to store
     * the key/value string. */
    if (total_in_kv_len < length)
    {
        /* Iterate through the entire input key/value string to
         * store every key/value pair in the user provided buffer.
         * The key/value pair data in the user buffer is expected
         * to be in the following format:
         *
         *      "<key1>=<value1>\r\n<key2>=<value2>\r\n..."
         *
         * This key/value pair data then gets stored in the TXT
         * record in the format specified below (as per RFC-1464), each
         * key/value pair preceded by its length:
         *
         *      "<first_length><key1>=<value1><next_length><key2>=<value2>..."
         *
         */
        for( i = 0; ( (i < total_in_kv_len) && (kv_pair) ); )
        {
            /* Locate the end of the current key/value pair. */
            temp_ptr = strstr(kv_pair, "\r\n");

            /* There are possibly more key/value pairs available. */
            if(temp_ptr)
            {
                if((temp_ptr - kv_pair) <= 255)
                {
                    kv_len = temp_ptr - kv_pair;
                }

                else
                {
                    total_out_kv_len = NU_INVALID_SIZE;
                    break;
                }
            }

            /* This is the last key/value pair. */
            else
            {
                if(strlen(kv_pair) <= 255)
                {
                    kv_len = strlen(kv_pair);
                }

                else
                {
                    total_out_kv_len = NU_INVALID_SIZE;
                    break;
                }
            }

            /* Store the size of the key/value pair in the buffer. */
            PUT8(buffer, 0, kv_len);

            /* Increment the total key/value pair size to be
             * returned by the size of the current key/value
             * pair + 1 (for the pair size to be stored in the buffer) */
            total_out_kv_len = total_out_kv_len + kv_len + 1;

            /* Increment the buffer to store the key/value pair */
            buffer++;

            /* Copy the key/value pair into the buffer */
            strncpy(buffer, kv_pair, kv_len);

            /* Increment the buffer to store the next key/value
             * pair.
             */
            buffer += kv_len;

            /* Fetch the next key/value pair. */
            kv_pair += (kv_len + 2);

            /* Increment the counter. */
            i += (kv_len + 2);
        }
    }
    else
    {
        /* There is not enough space in the user provided buffer to store the
         * specified key/value pairs data.
         */
        total_out_kv_len = NU_NO_MEMORY;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (total_out_kv_len);

#else

    return (NU_INVALID_PARM);

#endif

} /* NU_DNS_Build_Key */

/*************************************************************************
*
*   FUNCTION
*
*       NU_DNS_Parse_Key
*
*   DESCRIPTION
*
*       This routine is used to parse out a key/value pair from the
*       incoming data and return them to the caller. It takes in a buffer
*       containing key/value pairs stored in the format specified in
*       RFC-1464; an offset into the buffer to a particular key/value
*       pair and returns the key/value pair along with their total length.
*       When looking for a particular key/value pair, a user can use this
*       routine within a loop to start from the beginning of the buffer
*       and use the return length as an offset for a subsequent call to
*       this routine.
*
*   INPUTS
*
*       *buffer                A user buffer that gets returned from
*                              NU_DNS_SD_Look_Up().
*       offset                 Offset into buffer to the beginning of the
*                              next key.
*       **out_key              On return, would contain the pointer to
*                              the memory in buffer that contains the
*                              Key.
*       **out_value            On return, would contain the pointer to
*                              the memory in buffer that contains the
*                              Value of the respective Key.
*
*   OUTPUTS
*
*       NU_INVALID_PARM        An input parameter is invalid.
*       This routine returns the total size of the key/value data parsed
*       so far. This return value can be used as an offset to fetch the
*       next Key/Value pair from the same buffer.
*
*************************************************************************/
INT32 NU_DNS_Parse_Key(CHAR *buffer, INT32 offset, CHAR **out_key,
                       CHAR **out_value)
{
#if (INCLUDE_MDNS == NU_TRUE)
    UINT8           i, j, kv_len = NU_NULL;
    CHAR            *kv_string;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input.  Do not check the total length of buffer for > 0
     * since the length of the current key could be zero.
     */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (buffer == NU_NULL) || (offset < 0) ||
         (out_key == NU_NULL) || (out_value == NU_NULL)
       )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If the length is zero, there is no key/value pair to return. */
    if (*(buffer + offset) != 0)
    {
        /* Set the beginning of the Key/Value pair to be retrieved. */
        kv_string = buffer + offset;

        /* Retrieve the length of the key/value pair stored in the byte
         * preceding the key/value pair itself.
         */
        kv_len = (UINT8)*kv_string;

        /* Move the entire key/value pair by one byte to make room
         * for NULL termination.
         */
        memmove(kv_string, (kv_string + 1), kv_len);

        /* Set the offset for the next Key/Value pair. */
        offset += (kv_len + 1);

        /* NULL terminate the entire Key/Value pair. */
        kv_string[kv_len] = '\0';

        /* Save the Key in the return pointer */
        *out_key = kv_string;

        /* Iterate through the key/value pair to extract the correct
         * key and value.
         */
        for (i = 0; i < kv_len; i++)
        {
            /* Clear all the escape characters contained in the Key. */
            if ( (*kv_string == '`') && ((*(kv_string + 1) == ' ') ||
                                         (*(kv_string + 1) == '`') ||
                                         (*(kv_string + 1) == '=')) )
            {
                /* Move the key data by 1 byte to account for the clearing
                 * of the escape character.
                 */
                memmove(kv_string, (kv_string + 1), (kv_len - i));

                /* Move to the next byte. */
                kv_string++;
            }
            else if ((*kv_string == '='))
            {
                /* Replace the '=' sign with NULL terminator to indicate the
                 * end of the Key. */
                *kv_string = '\0';

                /* Save the Value in the return pointer */
                *out_value = ++kv_string;

                /* Ensure that the Key/value pair does not begin with a
                 * '=' sign.
                 */
                if (i > 0)
                {
                    /* Iterate through the value data to clear all the escape
                     * characters and return the correct value information.
                     */
                    for (j = i; (j < kv_len) && (*kv_string != '\0'); j++)
                    {
                        /* Check to see if its an escape character. */
                        if ( (*kv_string == '`') && ((*(kv_string + 1) == ' ') ||
                                                     (*(kv_string + 1) == '`') ||
                                                     (*(kv_string + 1) == '=')) )
                        {
                            /* Move the value data by 1 to account for the clearing of
                             * the escape character.
                             */
                            memmove(kv_string, (kv_string + 1), (kv_len - j));
                        }

                        /* Move to the next byte. */
                        kv_string++;
                    }
                }

                break;
            }
            else
            {
                kv_string++;
            }
        }

        /* Check for unusual key/value pairs. */
        if (i == 0)
        {
            /* Key/Value pair beginning with a '=' sign in which case
             * both Key and Value would be NULL.
             */
            *out_key = NU_NULL;
            *out_value = NU_NULL;
        }
        else if ( (i == j) || (i == kv_len) )
        {
            /*  - Key value pair ends with a '=' in which case the Value
             *     would be NULL.
             *  - Key value pair without a '=' sign as a separator in which
             *     case the entire string would be the Key and the Value
             *     would be NULL.
             */
            *out_value = NU_NULL;
        }
    }

    /* Return 1 to indicate that there is only a data length in the
     * key/value pair.
     */
    else
    {
        offset = 1;
        *out_key = NU_NULL;
        *out_value = NU_NULL;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (offset);

#else

    return (NU_INVALID_PARM);

#endif

} /* NU_DNS_Parse_Key */
