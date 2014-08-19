/*************************************************************************
*
*               Copyright 2009 Mentor Graphics Corporation
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
*       dbg_rsp_utils.c
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Utilities
*
*   DESCRIPTION
*
*       This file contains the utility functions for the RSP Support
*       Component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RSP_Get_UINT_Data
*       RSP_Get_Int_Data
*       RSP_Get_Data
*       RSP_Ascii_Hex_Array_2_Bin_Array
*       RSP_Remove_Escape_Characters
*       RSP_Add_Escape_Characters
*       RSP_Convert_Bin_Arry_2_Rsp_Pkt
*       RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt
*       RSP_Convert_Str_2_Rsp_Pkt
*       RSP_Convert_Str_And_Bin_Data_2_Rsp_Pkt
*       RSP_Convert_Str_2_Hex_Str
*       RSP_Reverse_Int
*
*   DEPENDENCIES
*
*       dbg.h
*
*
*************************************************************************/

#include    "services/dbg.h"

/* Global variables */

/* Hex to ASCII conversion string */
static CHAR  dbg_rsp_utils_hex_2_ascii_hex[] = "0123456789abcdef";

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Get_UINT_Data
*
*   DESCRIPTION
*
*       This functions gets a data field from a RSP packet. It uses
*       valid delimiters to identify end of data field. It uses NULL
*       to identify end of RSP packet.
*
*   INPUTS
*
*       p_rsp_cmd               - Pointer to RSP byte being processed
*       next_field              - Return pointer to next field
*       data                    - Pointer to data
*       delimiter               - Delimiter to mark end of UINT
*
*   OUTPUTS
*
*       return value            - Number of bytes in the data field
*
*************************************************************************/
RSP_STATUS RSP_Get_UINT_Data(CHAR *     p_rsp_cmd,
                         CHAR **    next_field,
                         UINT *     int_data,
                         CHAR       delimiter)
{
    UINT        num_byts = 0;
    INT         int_value;
    RSP_STATUS  status = RSP_STATUS_OK;
    
    *int_data = 0;

    while(*p_rsp_cmd)
    {
        if(*p_rsp_cmd == delimiter)
        {
            break;
        }
        int_value = Hex_2_Dec(*p_rsp_cmd);

        if(int_value < 0)
        {
            status = RSP_STATUS_FAILED;
            break;
        }

        *int_data = (*int_data<<4) | int_value;

        num_byts++;

        p_rsp_cmd++;
    }
    
    *next_field = p_rsp_cmd+1;

    return(status);

}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Get_Int_Data
*
*   DESCRIPTION
*
*       This functions gets a data field from a RSP packet. It uses
*       valid delimiters to identify end of data field. It uses null
*       to identify end of RSP packet.
*
*   INPUTS
*
*       p_rsp_cmd               - Pointer to RSP byte being processed
*       int_data                - Pointer to output data buffer
*       expected_dlim           - End character to check for
*
*   OUTPUTS
*
*       return value            - Number of bytes in the data field
*
*************************************************************************/
CHAR * RSP_Get_Int_Data(CHAR *  p_rsp_cmd,
                        INT *   int_data,
                        CHAR    expectd_dlim)
{
    UINT        num_byts = 0;
    INT         int_value;
    UINT        is_negative = NU_FALSE;

    *int_data = 0;

    /* Check for a negative number */
    if(*p_rsp_cmd == '-')
    {
        is_negative = NU_TRUE;

        /* Go to the next character */
        p_rsp_cmd++;
    }

    while(*p_rsp_cmd != expectd_dlim)
    {
        int_value = Hex_2_Dec(*p_rsp_cmd);

        num_byts++;

        if (num_byts > (sizeof(INT)*2))
        {
            break;
        }

        *int_data = (*int_data<<4) | int_value;

        p_rsp_cmd++;
    }

    if(is_negative == NU_TRUE)
    {
       *int_data = 0 - *int_data;
    }

    return(p_rsp_cmd+1);

}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Get_TID_Data
*
*   DESCRIPTION
*
*       This functions gets a thread ID field from an RSP packet. It uses
*       valid delimiters to identify end of data field.
*
*   INPUTS
*
*       p_rsp_cmd               - Pointer to RSP byte being processed
*       tid_data                - Pointer to output TID value
*       expected_dlim           - End character to check for
*
*   OUTPUTS
*
*       return value            - Number of bytes in the data field
*
*************************************************************************/
CHAR * RSP_Get_TID_Data(CHAR * p_rsp_cmd, DBG_THREAD_ID * tid_data,
                        CHAR expectd_dlim)
{
    UINT           num_byts = 0;
    UINT           uint_value;
    UINT           local_tid;
    UINT           is_negative = NU_FALSE;

    local_tid = 0;

    /* Handle the a thread ID of -1.  Check for a negative number */
    if(*p_rsp_cmd == '-')
    {
        is_negative = NU_TRUE;

        /* Go to the next character */
        p_rsp_cmd++;
    }

    while(*p_rsp_cmd != expectd_dlim)
    {
        uint_value = (UINT)(Hex_2_Dec(*p_rsp_cmd));

        num_byts++;

        if (num_byts > (sizeof(INT)*2))
        {
            break;
        }

        local_tid = (local_tid<<4) | uint_value;

        p_rsp_cmd++;
    }

    if(is_negative == NU_TRUE)
    {
       local_tid = 0 - local_tid;
    }

    *tid_data = (DBG_THREAD_ID)(local_tid);

    return(p_rsp_cmd+1);

}


/*************************************************************************
*
*   FUNCTION
*
*       RSP_Get_Data
*
*   DESCRIPTION
*
*       This functions gets a data field from a RSP packet. It uses
*       valid delimiters to identify end of data field. It uses NULL
*       to identify end of RSP packet.
*
*   INPUTS
*
*       p_rsp_cmd               - Pointer to RSP byte being processed
*       data                    - Pointer to data
*       num_byts                - Number of bytes to compare
*
*   OUTPUTS
*
*       return value            - Number of bytes in the data field
*
*************************************************************************/
RSP_STATUS  RSP_Get_Data(CHAR **    p_rsp_cmd,
                         CHAR *     data,
                         CHAR       expectd_dlim)
{
    UINT            num_byts = 0;
    UINT8           tmp_byt = 0;
    CHAR *          p_src = *p_rsp_cmd;
    CHAR *          p_dst = data;
    RSP_STATUS      ret_val = RSP_STATUS_OK;

    while(*p_src != expectd_dlim)
    {
        /* Obtain first hex byte as decimal CHAR*/
        tmp_byt = (UINT8)Hex_2_Dec(*p_src);

        /* Invalid delimiter detected report packet error */
        if(tmp_byt == ((UINT8)-1))
        {
            ret_val = RSP_STATUS_PACKET_ERROR;
            break;
        }

        /* Shift decimal CHAR to most significant nibble position */
        tmp_byt = (tmp_byt << 4);

        /* increment source pointer to hex byte */
        p_src++;

        /* Obtain second hex byte as decimal CHAR in least significant
        nibble position */
        tmp_byt |= (UINT8)Hex_2_Dec(*p_src);

        /* Copy full decimal byte obtained to pointer to destination */
        *p_dst = tmp_byt;

        /* Increment decimal bytes processed count */
        num_byts ++;

        /* Increment source and destination pointers */
        p_src++;
        p_dst++;

        /* Buffer over flow, return packet error */
        if(num_byts > RSP_MAX_PACKET_SIZE_SUPPORTED)
        {
            ret_val = RSP_STATUS_PACKET_ERROR;
            break;
        }
    }

    /* Return pointer to next byte to be processed */
    *p_rsp_cmd = p_src + 1;

    return(ret_val);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Ascii_Hex_Array_2_Bin_Array
*
*   DESCRIPTION
*
*      This function converts an ASCII encoded hex array to a binary
*      decimal array and loads it to memory.
*
*   INPUTS
*
*      p_hex_arry         Pointer to start of source hex array
*      p_bin_arry         Pointer to start of destination binary array
*      expectd_dlim       Expected end of field character.
*      max_size           Used to prevent endless while loops.
*                         max_size < sizeof(p_bin_arry).
*
*   OUTPUTS
*
*      num_byts           Number of bytes written to memory
*
*************************************************************************/
UINT RSP_Ascii_Hex_Array_2_Bin_Array(CHAR *     p_hex_arry,
                                     CHAR *     p_bin_arry,
                                     CHAR       expectd_dlim,
                                     INT        max_size)
{
    CHAR        tmp_byt = 0;
    CHAR        tmp_byt2 = 0;
    CHAR *      p_hex = p_hex_arry;
    CHAR *      p_byt = p_bin_arry;
    UINT        num_byts = 0;

    while((*p_hex != expectd_dlim) && (num_byts < max_size))
    {
        /* Construct a byte out of two ASCII Hex bytes */
        tmp_byt = Hex_2_Dec(*p_hex);
        p_hex++;
        tmp_byt2 = Hex_2_Dec(*p_hex);
        tmp_byt = ((tmp_byt << 4) | tmp_byt2);
        p_hex++;

        /* Copy the constructed byte to memory */
        *p_byt++ = tmp_byt;

        /* Increment bytes written */
        num_byts++;
    }

    return (num_byts);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Remove_Escape_Characters
*
*   DESCRIPTION
*
*      This function removes RSP escape characters '}' from the incoming.
*      client data.  It then does an exclusive or with the next byte to
*      restore the original byte.  The RSP client will escape
*      '$', '#' and '}' all other characters are allowed by the client.
*
*   INPUTS
*
*      p_src_arry         Pointer to start of the source binary array
*      p_des_arry         Pointer to start of destination binary array
*      expectd_dlim       End of data character.
*      max_size           Used to prevent endless while loops.
*                         max_size < sizeof(p_des_arry).
*
*   OUTPUTS
*
*      num_byts           Number of bytes written to the destination array.
*
*************************************************************************/
UINT RSP_Remove_Escape_Characters(CHAR *    p_src_arry,
                                  CHAR *    p_des_arry,
                                  CHAR      expectd_dlim,
                                  INT       max_size)
{
    CHAR        tmp_byt = 0;
    CHAR *      p_src = p_src_arry;
    CHAR *      p_des = p_des_arry;
    UINT        num_byts = 0;

    while((*p_src != expectd_dlim) && (num_byts < max_size))
    {
        if (*p_src != RSP_ESCAPE_CHAR)
        {
            /* Copy the byte. */
            *p_des++ = *p_src++;
        }
        else
        {
            /* An escape character was found. */

            /* Increment past the escape character. */
             p_src++;

            /* Copy the byte to a local variable. */
            tmp_byt = *p_src;

            /* Restore the original value. XOR with 0x20. */
            tmp_byt = tmp_byt ^ RSP_ESCAPE_VALUE;

            /* Copy the byte. */
            *p_des = tmp_byt;

            /* Increment the Destination and Source pointers. */
             p_src++;
             p_des++;
        }

        /* Increment bytes written */
        num_byts++;
    }

    return (num_byts);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Add_Escape_Characters
*
*   DESCRIPTION
*
*      This function adds RSP escape characters '}' to the outgoing.
*      client data.  It then does an XOR with the character that needs to
*      be changed from the original value.  The characters that need to be escaped
*      are '$', '#', '}' and '*' all other characters are allowed by RSP.
*
*   INPUTS
*
*      p_src_arry         Pointer to start of the source binary array
*      p_des_arry         Pointer to start of destination binary array
*      p_cnt              Integer pointer to the number of bytes in the
*                         source data. It can be reduced if there are too
*                         many escape bytes.
*
*   OUTPUTS
*
*      total_byts         Number of bytes written to the destination array.
*                         This value includes the escape characters.  This value
*                         is used to determine the number of escape characters
*                         added.  It should not be returned to the client.
*
*************************************************************************/
UINT RSP_Add_Escape_Characters(CHAR *   p_src_arry,
                               CHAR *   p_des_arry,
                               INT *    p_cnt)
{
    CHAR        tmp_byt = 0;
    CHAR *      p_src = p_src_arry;
    CHAR *      p_des = p_des_arry;
    UINT        num_byts = 0;
    UINT        escape_byts = 0;
    UINT        total_byts = 0;
    INT         i;
    INT         cnt = *p_cnt;

    for (i = 0; i < cnt; i++)
    {
        if ((*p_src == RSP_ESCAPE_CHAR) || (*p_src == RSP_RUN_LEN_ENCODE_CHAR) || \
            (*p_src == RSP_PKT_START_CHAR) || (*p_src == RSP_PKT_END_CHAR))
        {
            /* Copy the escape character to the destination pointer. */
            *p_des++ = RSP_ESCAPE_CHAR;

            /* Increment escape bytes written */
            escape_byts++;

            /* Copy the src byte to a local variable. */
            tmp_byt = *p_src;

            /* XOR with 0x20 to change the value. */
            tmp_byt = tmp_byt ^ RSP_ESCAPE_VALUE;

            /* Copy the byte. */
            *p_des = tmp_byt;

            /* Increment bytes written */
            num_byts++;

            /* Make sure the max packet size is not exceeded. */
            if ((num_byts + escape_byts) > (RSP_MAX_PACKET_SIZE_SUPPORTED - RSP_F_PKT_OVHD))
            {
                /* Decrement bytes written. */
                num_byts--;

                /* Decrement escape bytes written */
                escape_byts--;

                /* Exit the loop. */
                i = cnt;
            }
            else
            {
                /* Increment the Destination and Source pointers. */
                 p_src++;
                 p_des++;
            }
        }
        else
        {
            /* Copy the byte. */
            *p_des++ = *p_src++;

            /* Increment bytes written */
            num_byts++;

            /* Make sure the max packet size is not exceeded. */
            if ((num_byts + escape_byts) > (RSP_MAX_PACKET_SIZE_SUPPORTED - RSP_F_PKT_OVHD))
            {
                /* Decrement bytes written. */
                num_byts--;

                /* Exit the loop. */
                i = cnt;
            }
        }
    }

    /* Total bytes written excluding '}' escape characters. */
    *p_cnt = num_byts;

    /* Return the all bytes written including the escape bytes. */
    total_byts = num_byts  + escape_byts;

    return (total_byts);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Convert_Bin_Arry_2_Rsp_Pkt
*
*   DESCRIPTION
*
*      This function is used to convert a binary array of data present in
*      memory to an RSP packet
*
*   INPUTS
*
*      p_bin_arry         Pointer to Binary Array
*      size_bin_arry      Size of Binary Array
*      p_rsp_pkt          Pointer to RSP Packet
*      size_rsp_pkt       Size of RSP Packet
*
*   OUTPUTS
*
*      rsp_pkt_size       Size of RSP packet
*
*************************************************************************/
UINT RSP_Convert_Bin_Arry_2_Rsp_Pkt(CHAR *  p_bin_arry,
                                    UINT    size_bin_arry,
                                    CHAR *  p_rsp_pkt,
                                    UINT *  size_rsp_pkt)
{
    CHAR    tmp_byt = 0;
    UINT    rsp_pkt_size = 0;
    CHAR    check_sum = 0;

    /* Copy in the '$' to indicate start of packet */
    *p_rsp_pkt++ = '$';

    /* Convert all requested bytes in memory to Hex chars */
    while(size_bin_arry-- > 0)
    {
        tmp_byt = *p_bin_arry;

        /* Convert most significant nibble of source byte into hex char */
        *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[(tmp_byt>>4) & 0x0F];

        /* Update checksum */
        check_sum += *p_rsp_pkt;

        /* Increment destination pointer */
        p_rsp_pkt++;

        /* Convert least significant nibble of source byte into hex char */
        *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[tmp_byt & 0x0F];

        /* Update checksum */
        check_sum += *p_rsp_pkt;

        /* Increment destination pointer */
        p_rsp_pkt++;

        /* Increment source pointer and destination pointer */
        p_bin_arry++;

        /* Increment number of Hex chars generated */
        rsp_pkt_size += 2;
    }

    /* Copy in the '#' to indicate end of packet */
    *p_rsp_pkt++ = '#';

    /* Copy in checksum LSB */
    *p_rsp_pkt++ = dbg_rsp_utils_hex_2_ascii_hex[(check_sum >> 4)& 0x0F];

    /* Format char to transmit */
    *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[check_sum & 0x0F];

    /* Account for packet handling specific chars $,#,checksum */
    rsp_pkt_size += 4;

    *size_rsp_pkt = rsp_pkt_size;

    /* Return RSP packet size */
    return (rsp_pkt_size);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt
*
*   DESCRIPTION
*
*      This function is used to build an RSP packet using the expedited register
*      set defined by GDB.  This can be a stop reply or asynchronous notification
*      packet (%).  The start of the packet is sent by the function caller as a
*      string.  This allows it to be used by either packet type.  The expedited
*      registers are a binary array of data.  The caller also sends the thread ID
*      of the thread that had the stop event.
*
*
*   INPUTS
*
*      p_start_str      Pointer to the packet start string.  This should
*                       contain all needed characters up to the first expedited
*                       register number.
*      thread_id        The ID of the thread that had the stop event
*      p_bin_arry       Pointer to Binary Array of expedited register numbers and
*                       register values.  The format will be 32 bit Register Number
*                       followed by the 32 bit register value.
*      size_bin_arry    Size of Binary Array in bytes
*      p_rsp_pkt        Pointer to RSP Packet
*      size_rsp_pkt     Size of RSP Packet
*
*   OUTPUTS
*
*      rsp_pkt_size     Size of RSP packet
*
*************************************************************************/
UINT RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt(CHAR *             p_start_str,
                                             DBG_THREAD_ID      thread_id,
                                             CHAR *             p_bin_arry,
                                             UINT               size_bin_arry,
                                             CHAR *             p_rsp_pkt,
                                             UINT *             size_rsp_pkt)
{
    UINT8   tmp_byt = 0;
    UINT    rsp_pkt_size = 0;
    UINT8   check_sum = 0;
    INT *   int_data_ptr;
    UINT    size = 0;
    CHAR    reg_num;
    CHAR *  thd_str = "thread:";
    CHAR    str_array[10];
    UINT    str_array_size = sizeof(str_array);

    /* Copy the start string.*/

    while (*p_start_str != '\0')
    {
        /* Update check_sum.  Skip '$' and '%'. */
        if ((*p_start_str != '$') && (*p_start_str != '%'))
        {
            check_sum += *p_start_str;
        }

        /* copy byte */
        *p_rsp_pkt++ = *p_start_str++;

        /* Increment bytes copied */
        rsp_pkt_size++;

    }

    /* Get an integer pointer to the binary array. */
    int_data_ptr = (INT *)p_bin_arry;


    size = 0;
    /* Convert all requested bytes in memory to Ascii Hex chars */
    while(size < size_bin_arry)
    {
        /* Get the register number. The register numbers start at byte 0, 8, 16 etc. */
        if ((size % 8) == 0)
        {
            /* Get the Least Significant Byte. This accounts for endianess.
             * The register number is in a 32 bit field.   */
            reg_num = (CHAR)int_data_ptr[size/4];

            /* Convert most significant nibble of register number into ascii hex char */
            *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[(reg_num>>4) & 0x0F];

            /* Update checksum */
            check_sum += *p_rsp_pkt;

            /* Increment destination pointer */
            p_rsp_pkt++;

            /* Convert least significant nibble of source byte into ascii hex char */
            *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[reg_num & 0x0F];

            /* Update checksum */
            check_sum += *p_rsp_pkt;

            /* Increment destination pointer */
            p_rsp_pkt++;

            /* The register number needs to be followed by a colon ':' */
            *p_rsp_pkt = ':';

            /* Update checksum */
            check_sum += *p_rsp_pkt;

            /* Increment destination pointer */
            p_rsp_pkt++;

            /* Increment number of ascii chars generated by 3. */
            rsp_pkt_size += 3;

            /* Move to the register value and increment the source pointer. */
            size += 4;
            p_bin_arry += 4;

        }
        else  /* Get the register value. */
        {
            tmp_byt = *p_bin_arry;

            /* Convert most significant nibble of source byte into hex char */
            *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[(tmp_byt>>4) & 0x0F];

            /* Update checksum */
            check_sum += *p_rsp_pkt;

            /* Increment destination pointer */
            p_rsp_pkt++;

            /* Convert least significant nibble of source byte into hex char */
            *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[tmp_byt & 0x0F];

            /* Update checksum */
            check_sum += *p_rsp_pkt;

            /* Increment destination pointer */
            p_rsp_pkt++;

            /* Increment source pointer to the next byte. */
            p_bin_arry++;

            /* Increment number of ascii Hex chars generated */
            rsp_pkt_size += 2;

            /* Increment the size. */
            size++;

            /*  The register value needs to be followed by a semi-colon ';' */
            if ((size % 8) == 0)
            {
                /* Add the semi-colon to the RSP packet. */
                *p_rsp_pkt = ';';

                /* Update checksum */
                check_sum += *p_rsp_pkt;

                /* Increment destination pointer */
                p_rsp_pkt++;

                /* Increment number of ascii Hex chars generated by 1 */
                rsp_pkt_size++;
            }
        }
    }

    /* Add the string "thread:" to the packet. */
    while (*thd_str != '\0')
    {
        /* Update the check_sum. */
        check_sum += *thd_str;

        /* copy byte */
        *p_rsp_pkt++ = *thd_str++;

        /* Increment bytes copied */
        rsp_pkt_size++;
    }


    /* Add the thread ID to the packet. */

    /* Convert the thread_id to ascii. */
    DBG_STR_String_From_UINT(&str_array[0],(UINT)thread_id,
                            DBG_STR_RADIX_HEXADECIMAL,
                            DBG_STR_INC_PREFIX_DISABLE,
                            DBG_STR_INC_LEADING_ZEROS_DISABLE);

    /* Copy the thread ID into the packet. */
    tmp_byt = 0;
    while((str_array[tmp_byt] != '\0') && (tmp_byt < str_array_size))
    {
        /* Update the check_sum. */
        check_sum += str_array[tmp_byt];

        /* copy byte */
        *p_rsp_pkt++ = str_array[tmp_byt++];

        /* Increment bytes copied */
        rsp_pkt_size++;
    }


    /* Add a semi-colon after the thread ID to the RSP packet. */
    *p_rsp_pkt = ';';

    /* Update checksum */
    check_sum += *p_rsp_pkt;

    /* Increment destination pointer */
    p_rsp_pkt++;

    /* Increment number of ascii Hex chars generated by 1 */
    rsp_pkt_size++;


    /* Copy in the '#' to indicate end of packet */
    *p_rsp_pkt++ = '#';

    /* Copy in checksum LSB */
    *p_rsp_pkt++ = dbg_rsp_utils_hex_2_ascii_hex[(check_sum >> 4)& 0x0F];

    /* Format char to transmit */
    *p_rsp_pkt = dbg_rsp_utils_hex_2_ascii_hex[check_sum & 0x0F];

    /* Account for packet handling specific chars #,checksum */
    rsp_pkt_size += 3;

    *size_rsp_pkt = rsp_pkt_size;

    /* Return RSP packet size */
    return (rsp_pkt_size);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Convert_Str_2_Rsp_Pkt
*
*   DESCRIPTION
*
*      This function converts the string passed in to an ASCII RSP Packet.
*
*
*   INPUTS
*
*       char *      Pointer to source data
*       char *      Pointer to destination
*
*   OUTPUTS
*
*       int        Number of bytes copied to destination
*
*************************************************************************/
INT RSP_Convert_Str_2_Rsp_Pkt(CHAR *    p_src_str,
                              CHAR *    p_rsp_pkt)
{
    CHAR *      src = p_src_str;
    CHAR *      des = p_rsp_pkt;
    INT         num_bytes = 0;
    UINT        check_sum = 0;

    /* Copy in the '$' to indicate start of packet */
    *des++ = '$';

    while (*src != '\0')
    {
        /* Update check_sum */
        check_sum += *src;

        /* copy byte */
        *des++ = *src++;

        /* Increment bytes copied */
        num_bytes++;

    }

    /* Copy in '#' to indicate end of packet */
    *des++ = '#';

    /* Copy in check_sum LSB */
    *des++ = dbg_rsp_utils_hex_2_ascii_hex[(check_sum >> 4)& 0x0F];

    /* Format char to transmit */
    *des = dbg_rsp_utils_hex_2_ascii_hex[check_sum & 0x0F];

    /* Account count for packet handling specific chars $,#,check_sum */
    num_bytes += 4;

    return (num_bytes);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Convert_Str_And_Bin_Data_2_Rsp_Pkt
*
*   DESCRIPTION
*
*      This function converts the string passed in plus the
*      binary data to an RSP Packet.
*
*
*   INPUTS
*
*       p_start_str     Pointer to Starting ascii string
*       p_src_data      Pointer to source binary data
*       srcDataLen      Binary data length
*       p_rsp_pkt       Pointer to destination
*
*   OUTPUTS
*
*       num_bytes        Number of bytes copied to destination
*
*************************************************************************/
INT RSP_Convert_Str_And_Bin_Data_2_Rsp_Pkt(CHAR *   p_start_str,
                                           CHAR *   p_src_data,
                                           INT      srcDataLen,
                                           CHAR *   p_rsp_pkt)
{
    CHAR *  src = p_src_data;
    CHAR *  des = p_rsp_pkt;
    INT     num_bytes = 0;
    UINT    check_sum = 0;
    UINT    i = 0;

    /* Copy in the '$' to indicate start of packet */
    *des++ = '$';

    /* Copy the start string.*/

    while (*p_start_str != '\0')
    {
        check_sum += *p_start_str;

        /* copy byte */
        *des++ = *p_start_str++;

        /* Increment bytes copied */
        num_bytes++;

    }

    for(i = 0; i < srcDataLen; i++)
    {
        /* Update check_sum */
        check_sum += *src;

        /* copy byte */
        *des++ = *src++;

        /* Increment bytes copied */
        num_bytes++;

    }

    /* Copy in '#' to indicate end of packet */
    *des++ = '#';

    /* Copy in check_sum LSB */
    *des++ = dbg_rsp_utils_hex_2_ascii_hex[(check_sum >> 4)& 0x0F];

    /* Format CHAR to transmit */
    *des = dbg_rsp_utils_hex_2_ascii_hex[check_sum & 0x0F];

    /* Account count for packet handling specific CHARs $,#,check_sum */
    num_bytes += 4;

    return (num_bytes);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Convert_Str_2_Hex_Str
*
*   DESCRIPTION
*
*      This function converts the string passed in to an ASCII Hex string.
*
*
*   INPUTS
*
*       p_src_str - The source string.
*
*       p_hex_str - The hex string.
*
*   OUTPUTS
*
*       The size (in characters) of the hex string.
*
*************************************************************************/
INT RSP_Convert_Str_2_Hex_Str(CHAR * p_src_str,
                              CHAR * p_hex_str)
{
    INT         hex_str_size;
    DBG_STATUS  dbg_status;
    UINT        src_char;
    UINT        i;

    /* Process all characters in source string into the hex string. */

    dbg_status = DBG_STATUS_OK;
    i = 0;
    hex_str_size = 0;
    while ((dbg_status == DBG_STATUS_OK) &&
           (p_src_str[i] != NU_NULL))
    {
        src_char = (UINT)p_src_str[i];

        dbg_status = DBG_STR_String_From_BYTE(&p_hex_str[hex_str_size],
                                              src_char,
                                              DBG_STR_RADIX_HEXADECIMAL,
                                              DBG_STR_INC_PREFIX_DISABLE,
                                              DBG_STR_INC_LEADING_ZEROS_ENABLE,
                                              DBG_STR_INC_NULL_TERM_DISABLE);

        if (dbg_status == DBG_STATUS_OK)
        {
            i += 1;
            hex_str_size += 2;
        }

    }

    /* Ensure hex string is NULL-termianted. */

    p_hex_str[hex_str_size] = NU_NULL;

    return (hex_str_size);
}

/*************************************************************************
*
*   FUNCTION
*
*      RSP_Reverse_Int
*
*   DESCRIPTION
*
*      This function reverses the byte order of an integer.
*
*
*   INPUTS
*
*       i  -  integer value to be reversed
*
*   OUTPUTS
*
*       The reversed integer.
*
*************************************************************************/
INT RSP_Reverse_Int (INT i)
{
    CHAR c1, c2, c3, c4;

    c1 = i & 255;
    c2 = (i >> 8) & 255;
    c3 = (i >> 16) & 255;
    c4 = (i >> 24) & 255;

    return ((INT)c1 << 24) + ((INT)c2 << 16) + ((INT)c3 << 8) + c4;
}
