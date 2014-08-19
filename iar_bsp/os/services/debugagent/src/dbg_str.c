/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
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
*       dbg_str.c                                         
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Strings
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C source code for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None       
*                                                                      
*   FUNCTIONS                                                            
*           
*       dbg_str_char_to_uint
*       dbg_str_uint_to_char
*       dbg_str_string_from_byte_binary
*       dbg_str_string_from_byte_decimal
*       dbg_str_string_from_type_hexadecimal
*       dbg_str_string_from_uint_binary
*       dbg_str_string_from_unit_decimal
*       dbg_str_string_from_uint_hexadecimal
*       dbg_str_string_radix
*
*       DBG_STR_Value_String_Get
*       DBG_STR_String_Combine
*       DBG_STR_String_From_UINT
*       DBG_STR_String_To_UINT
*       DBG_STR_String_From_BYTE
*                                                                      
*   DEPENDENCIES
*
*       dbg.h
*                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"

/***** Global variables */

static CHAR                     DBG_STR_Unknown_String[] = "Unknown";

/* Debug Status Value String Array */

static DBG_STR_VALUE_STRING     DBG_STR_Debug_Status_Strings[] = DBG_STR_DBG_STATUS_STRING_INIT;

/* Debug Command type Value String Array */

static DBG_STR_VALUE_STRING     DBG_STR_Debug_Command_Op_Strings[] = DBG_STR_DBUG_CMD_OP_STRING_INIT;

/* Debug Event type Value String Array */

static DBG_STR_VALUE_STRING     DBG_STR_Debug_Event_Id_Strings[] = DBG_STR_DBUG_EVT_ID_STRING_INIT;

/* RSP Packet Type Value String Array */

static DBG_STR_VALUE_STRING     DBG_STR_RSP_Packet_Type_Strings[] = DBG_STR_RSP_PKT_TYPE_STRING_INIT;

/***** Local functions */

/* Local function prototypes */

static DBG_STATUS dbg_str_char_to_uint(CHAR        ch,
                                        UINT *      p_value,
                                        UINT        radix);

static DBG_STATUS dbg_str_uint_to_char(UINT        value,
                                        CHAR *      p_ch,
                                        UINT        radix);

static DBG_STATUS dbg_str_string_from_byte_binary(CHAR *           str,
                                                   UINT8            value,
                                                   BOOLEAN          prefix,
                                                   BOOLEAN          leading_zeros,
                                                   BOOLEAN          null_term);

static DBG_STATUS dbg_str_string_from_byte_decimal(CHAR *          str,
                                                    UINT8           value,
                                                    BOOLEAN         prefix,
                                                    BOOLEAN         leading_zeros,
                                                    BOOLEAN         null_term);

static DBG_STATUS dbg_str_string_from_type_hexadecimal(CHAR *          str,
                                                        UINT8           value,
                                                        BOOLEAN         prefix,
                                                        BOOLEAN         leading_zeros,
                                                        BOOLEAN         null_term);

static DBG_STATUS dbg_str_string_from_uint_binary(CHAR *       str,
                                                   UINT         value,
                                                   BOOLEAN      prefix,
                                                   BOOLEAN      leading_zeros);

static DBG_STATUS dbg_str_string_from_unit_decimal(CHAR *          str,
                                                    UINT            value,
                                                    BOOLEAN         prefix,
                                                    BOOLEAN         leading_zeros);

static DBG_STATUS dbg_str_string_from_uint_hexadecimal(CHAR *      str,
                                                        UINT        value,
                                                        BOOLEAN     prefix,
                                                        BOOLEAN     leading_zeros);

static UINT dbg_str_string_radix(CHAR *        str,
                                  BOOLEAN *     p_has_prefix);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_str_char_to_uint
*                                                                      
*   DESCRIPTION   
*                                                       
*       Attempts to translate a character into a numerical value (up to 
*       hexadecimal level).
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       ch - The character.
*                   
*       p_value - The value of the character if the operation is 
*                 successful.
*
*       radix - Indicates the radix of the numerical value.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Success of the operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_char_to_uint(CHAR        ch,
                                        UINT *      p_value,
                                        UINT        radix)
{
    DBG_STATUS      dbg_status;
    UINT            value;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Attempt to translate character into a value using the characters
       radix. */

    switch(ch)
    {
        case '0':
        {
            value = 0;
        
            break;
        
        }

        case '1':
        {
            value = 1;
            
            break;
        
        }

        case '2':
        {
            value = 2;
            
            break;
        
        }

        case '3':
        {
            value = 3;
            
            break;
        
        }

        case '4':
        {
            value = 4;
            
            break;
        
        }

        case '5':
        {
            value = 5;
            
            break;
        
        }

        case '6':
        {
            value = 6;
            
            break;
        
        }

        case '7':
        {
            value = 7;
            
            break;
        
        }

        case '8':
        {
            value = 8;
            
            break;
        
        }

        case '9':
        {
            value = 9;
            
            break;
        
        }

        case 'A':
        case 'a':
        {
            value = 10;
            
            break;
        
        }

        case 'B':
        case 'b':
        {
            value = 11;
            
            break;
        
        }

        case 'C':
        case 'c':
        {
            value = 12;
            
            break;
        
        }

        case 'D':
        case 'd':
        {
            value = 13;
            
            break;
        
        }

        case 'E':
        case 'e':
        {
            value = 14;
            
            break;
        
        }

        case 'F':
        case 'f':
        {
            value = 15;
            
            break;
        
        }
        
        default:
        {
            /* ERROR: Invalid character */
            
            dbg_status = DBG_STATUS_FAILED;

            break;
        
        }
        
    }

    /* Determine if a value was successfully set. */

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Ensure that the value adheres to the radix. */

        if (value < radix)
        {
            /* Update return parameter. */

            *p_value = value;

        }
        else
        {
            /* ERROR: Value has an invalid radix. */

            dbg_status = DBG_STATUS_FAILED;
        
        }
    
    }
    
    return (dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_str_uint_to_char
*                                                                      
*   DESCRIPTION   
*                                                       
*       Attempts to translate any numerical value (up to hexidecimal) into
*       a character.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       value - The numerical value.
*                   
*       p_ch - The  character if the operation is successful.
*
*       radix - Indicates the radix that the character should adhere to.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_uint_to_char(UINT        value,
                                        CHAR *      p_ch,
                                        UINT        radix)
{
    DBG_STATUS      dbg_status;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Ensure that the value adheres to the radix. */

    if (value < radix)
    {
        /* set the character using the value. */

        /* Determine how to proceed based on the UINT value. */
        
        switch(value)
        {
            case 0:
            {
                *p_ch ='0';
                
                break;
            
            }

            case 1:
            {
                *p_ch = '1';
                
                break;
            
            }

            case 2:
            {
                *p_ch = '2';
                
                break;
            
            }

            case 3:
            {
                *p_ch = '3';
                
                break;
            
            }

            case 4:
            {
                *p_ch = '4';
                
                break;
            
            }

            case 5:
            {
                *p_ch = '5';
                
                break;
            
            }

            case 6:
            {
                *p_ch = '6';
                
                break;
            
            }

            case 7:
            {
                *p_ch = '7';
                
                break;
            
            }

            case 8:
            {
                *p_ch = '8';
                
                break;
            
            }

            case 9:
            {
                *p_ch = '9';
                
                break;
            
            }

            case 10:
            {
                *p_ch = 'A';
                
                break;
            
            }

            case 11:
            {
                *p_ch = 'B';
                
                break;
            
            }

            case 12:
            {
                *p_ch = 'C';
                
                break;
            
            }

            case 13:
            {
                *p_ch = 'D';
                
                break;
            
            }

            case 14:
            {
                *p_ch = 'E';
                
                break;
            
            }

            case 15:
            {
                *p_ch = 'F';
                
                break;
            
            }   
            
            default:
            {
                /* ERROR: Invalid value */
                
                dbg_status = DBG_STATUS_FAILED;

                break;
            
            }
            
        }
    
    }
    else
    {
        /* ERROR: Value has an invalid radix. */

        dbg_status = DBG_STATUS_FAILED;
    
    }
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_byte_binary
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a BYTE value to a binary string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The BYTE value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                       string.
*               
*       null_term - Indicates if the NULL terminator should be added to the 
*                   string.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_string_from_byte_binary(CHAR *           str,
                                                   UINT8            value,
                                                   BOOLEAN          prefix,
                                                   BOOLEAN          leading_zeros,
                                                   BOOLEAN          null_term)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_bit;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;

    /* Initialize local variables. */

    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */

    str_idx = 0;

    /* Optionally add a prefix to the string. */

    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'b';
    
    } 
    
    /* Set initial state of leading non-zero tracking. */

    first_non_zero_found = NU_FALSE;
    
    /* Translate each bit into a character in the string. */

    i = 0;
    while ((i < 8) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get bit from value. */
        
        value_bit = ((value << i) >> 7);

        /* Update leading non-zero tracking. */

        if ((value_bit != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        } 

        /* Move to next bit in value. */
        
        i++;
        
        /* Determine if the value should be added to the string. */

        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == 8))
        {
            /* Get character for bit value. */
    
            dbg_status = dbg_str_uint_to_char(value_bit,
                                               &value_ch,
                                               DBG_STR_RADIX_BINARY);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with bit character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            } 

        } 

    } 

    /* Place NULL-terminator at end of string. */
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (null_term == NU_TRUE))
    {
        str[str_idx] = NU_NULL;

    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_byte_decimal
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a BYTE value to a decimal string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The BYTE value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                       string.
*                  
*       null_term - Indicates if the NULL terminator should be added to the 
*                   string.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_string_from_byte_decimal(CHAR *          str,
                                                    UINT8           value,
                                                    BOOLEAN         prefix,
                                                    BOOLEAN         leading_zeros,
                                                    BOOLEAN         null_term)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_remaining;
    UINT            value_digit;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;

    /* Initialize local variables. */

    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */

    str_idx = 0;

    /* Optionally add a prefix to the string. */

    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'd';
    
    } 

    /* Set initial value remaining to the full value. */

    value_remaining = value;
       
    /* Set initial state of leading non-zero tracking. */

    first_non_zero_found = NU_FALSE;
    
    /* Translate each digit into a character in the string. */

    i = 100UL;
    while ((i > 0) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get digit from value. */
        
        value_digit = (value_remaining / i);

        /* Update leading non-zero tracking. */

        if ((value_digit != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        } 

        /* Update value remaining. */

        value_remaining -= (value_digit * i); 
        
        /* Update the divisor. */
        
        i /= 10;
        
        /* Determine if the value should be added to the string. */

        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == 0))
        {
            /* Get character for bit value. */
    
            dbg_status = dbg_str_uint_to_char(value_digit,
                                               &value_ch,
                                               DBG_STR_RADIX_DECIMAL);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with bit character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            } 

        } 

    } 

    /* Place NULL-terminator at end of string. */
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (null_term == NU_TRUE))
    {
        str[str_idx] = NU_NULL;

    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_type_hexadecimal
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a BYTE value to a hexadecimal string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The BYTE value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                       string.
*
*       null_term - Indicates if the NULL terminator should be added to the 
*                   string.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_string_from_type_hexadecimal(CHAR *          str,
                                                        UINT8           value,
                                                        BOOLEAN         prefix,
                                                        BOOLEAN         leading_zeros,
                                                        BOOLEAN         null_term)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_nibble;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;
    
    /* Initialize local variables. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */
    
    str_idx = 0;
    
    /* Optionally add a prefix to the string. */
    
    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'x';
    
    } 
    
    /* Set initial state of leading non-zero tracking. */
    
    first_non_zero_found = NU_FALSE;
    
    /* Translate each nibble into a character in the string. */
    
    i = 0;
    while ((i < 2) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get nibble from value (NOTE: nibble value is in UINT 
           format). */
        
        value_nibble = (((value << (i * 4)) & 0xF0) >> 4);
    
        /* Update leading non-zero tracking. */
    
        if ((value_nibble != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        } 

        /* Move to next nibble in value. */
        
        i++;
        
        /* Determine if the value should be added to the string. */
    
        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == 2))
        {
            /* Get character for nibble value. */
    
            dbg_status = dbg_str_uint_to_char(value_nibble,
                                               &value_ch,
                                               DBG_STR_RADIX_HEXADECIMAL);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with nibble character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            } 
    
        } 
    
    } 
    
    /* Place NULL-terminator at end of string. */
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (null_term == NU_TRUE))
    {
        str[str_idx] = NU_NULL;
    
    } 
    
    return (dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_uint_binary
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a UINT value to a binary string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The UINT value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                      string.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
************************************************************************/
static DBG_STATUS dbg_str_string_from_uint_binary(CHAR *       str,
                                                   UINT         value,
                                                   BOOLEAN      prefix,
                                                   BOOLEAN      leading_zeros)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_bit;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */

    str_idx = 0;

    /* Optionally add a prefix to the string. */

    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'b';
    
    }
    
    /* Set initial state of leading non-zero tracking. */

    first_non_zero_found = NU_FALSE;
    
    /* Translate each bit into a character in the string. */

    i = 0;
    while ((i < (sizeof(UINT) * 8)) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get bit from value. */
        
        value_bit = ((value << i) >> ((sizeof(UINT) * 8) - 1));

        /* Update leading non-zero tracking. */

        if ((value_bit != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        }

        /* Move to next bit in value. */
        
        i++;
        
        /* Determine if the value should be added to the string. */

        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == (sizeof(UINT) * 8)))
        {
            /* Get character for bit value. */
    
            dbg_status = dbg_str_uint_to_char(value_bit,
                                               &value_ch,
                                               DBG_STR_RADIX_BINARY);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with bit character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            }

        }

    }

    /* Place NULL-terminator at end of string. */
    
    if (dbg_status == DBG_STATUS_OK)
    {
        str[str_idx] = NU_NULL;

    }
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_unit_decimal
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a UINT value to a decimal string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The UINT value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                      string.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_string_from_unit_decimal(CHAR *          str,
                                                    UINT            value,
                                                    BOOLEAN         prefix,
                                                    BOOLEAN         leading_zeros)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_remaining;
    UINT            value_digit;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */

    str_idx = 0;

    /* Optionally add a prefix to the string. */

    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'd';
    
    } 

    /* Set initial value remaining to the full value. */

    value_remaining = value;
       
    /* Set initial state of leading non-zero tracking. */

    first_non_zero_found = NU_FALSE;
    
    /* Translate each bit into a character in the string. */

    i = 1000000000UL;
    while ((i > 0) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get digit from value. */
        
        value_digit = (value_remaining / i);

        /* Update leading non-zero tracking. */

        if ((value_digit != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        } 

        /* Update value remaining. */

        value_remaining -= (value_digit * i); 
        
        /* Update the divisor. */
        
        i /= 10;
        
        /* Determine if the value should be added to the string. */

        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == 0))
        {
            /* Get character for bit value. */
    
            dbg_status = dbg_str_uint_to_char(value_digit,
                                               &value_ch,
                                               DBG_STR_RADIX_DECIMAL);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with bit character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            } 

        } 

    } 

    /* Place NULL-terminator at end of string. */
    
    if (dbg_status == DBG_STATUS_OK)
    {
        str[str_idx] = NU_NULL;

    } 
    
    return (dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_from_uint_hexadecimal
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a UINT value to a hexadecimal string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The UINT value.
*
*       prefix - Indicates if a prefix should be placed on the string or
*                not.
*
*       leading_zeros - Indicates if leading zeros should be added to the
*                       string.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_str_string_from_uint_hexadecimal(CHAR *      str,
                                                        UINT        value,
                                                        BOOLEAN     prefix,
                                                        BOOLEAN     leading_zeros)
{
    DBG_STATUS      dbg_status;
    UINT            str_idx;
    UINT            i;
    UINT            value_nibble;
    CHAR            value_ch;
    BOOLEAN         first_non_zero_found;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;
    
    /* Set string index to start of string. */

    str_idx = 0;

    /* Optionally add a prefix to the string. */

    if (prefix == NU_TRUE)
    {
        str[str_idx++] = '0';
        str[str_idx++] = 'x';
    
    } 
    
    /* Set initial state of leading non-zero tracking. */

    first_non_zero_found = NU_FALSE;
    
    /* Translate each bit into a character in the string. */

    i = 0;
    while ((i < (sizeof(UINT) * 2)) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get nibble from value. */
        
        value_nibble = ((value << (i * 4)) >> ((sizeof(UINT) * 8) - 4));

        /* Update leading non-zero tracking. */

        if ((value_nibble != 0) &&
            (first_non_zero_found == NU_FALSE))
        {
            first_non_zero_found = NU_TRUE;
        
        } 

        /* Move to next nibble in value. */
        
        i++;
        
        /* Determine if the value should be added to the string. */

        if ((leading_zeros == NU_TRUE) ||
            (first_non_zero_found == NU_TRUE) ||
            (i == (sizeof(UINT) * 2)))
        {
            /* Get character for bit value. */
    
            dbg_status = dbg_str_uint_to_char(value_nibble,
                                              &value_ch,
                                              DBG_STR_RADIX_HEXADECIMAL);
    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update string with bit character. */
    
                str[str_idx] = value_ch;
    
                /* Move to next string character. */
    
                str_idx++;
                
            } 

        } 

    } 

    /* Conditionally place NULL-terminator at end of string. */
    
    if (dbg_status == DBG_STATUS_OK)
    {
        str[str_idx] = NU_NULL;

    } 
    
    return (dbg_status);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       dbg_str_string_radix
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempt to determine the radix of the string by
*       examining the first few characters of the string.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       str - Pointer to the NULL-terminated string.
*
*       p_has_prefix - Return parameter that indicates if the string has a 
*                    recognized prefix or not.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       Returns a pre-defined radix value or NONE if the string radix 
*       could not be determined.
*                                                                      
*************************************************************************/
static UINT dbg_str_string_radix(CHAR *        str,
                                  BOOLEAN *     p_has_prefix)
{
    UINT    str_radix;

    /* Determine which radix the string is by the prefix (if it has
       one).   Default to assuming the value is decimal. */
    
    switch (str[1])
    {
        case 'b':
        {
            str_radix = DBG_STR_RADIX_BINARY;
            *p_has_prefix = NU_TRUE;
            
            break;
    
        } 

        case 'd':
        {
            str_radix = DBG_STR_RADIX_DECIMAL;
            *p_has_prefix = NU_TRUE;
        
            break;
    
        } 

        case 'x':
        {
            str_radix = DBG_STR_RADIX_HEXADECIMAL;
            *p_has_prefix = NU_TRUE;
        
            break;
    
        } 

        default:
        {
            /* Assume this is a decimal string (with no prefix). */
        
            str_radix = DBG_STR_RADIX_DECIMAL;
            *p_has_prefix = NU_FALSE;

        }
            
    }
    
    return(str_radix);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_STR_Value_String_Get
*                                                                      
*   DESCRIPTION   
*                                                       
*       Retrieves a string for a specified value in a value string set. 
*                                                                                                 
*   INPUTS                                                               
*      
*       value_string_type - The type of value string (specifies the value
*                         string set to be used).
*
*       value - The value to look up.
*
*       p_string - Return parameter that will contain a pointer to an 
*                 appropriate string for the specified value if the
*                 operation is successful.  If the operation is fails a
*                 default string will be returned.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates the results of the operation.
*
*       <other> - Indicates an internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_STR_Value_String_Get(DBG_STR_VALUE_STRING_TYPE       value_string_type,
                                    UINT                            value,
                                    CHAR **                         p_string)
{
    DBG_STATUS                  dbg_status;
    DBG_STR_VALUE_STRING *      p_value_strings;
    UINT                        value_strings_count;
    UINT                        i;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine how to proceed based on the value string type. */
    
    switch (value_string_type)
    {
        case DBG_STR_VALUE_STRING_TYPE_RSP_PKT_TYPE:
        {
            /* Setup the value strings array. */
            
            p_value_strings = &DBG_STR_RSP_Packet_Type_Strings[0];
            value_strings_count = (sizeof(DBG_STR_RSP_Packet_Type_Strings) / sizeof (DBG_STR_VALUE_STRING));
            
            break;
            
        }        
        
        case DBG_STR_VALUE_STRING_TYPE_DBG_STATUS :
        {
            /* Setup the value strings array. */
            
            p_value_strings = &DBG_STR_Debug_Status_Strings[0];
            value_strings_count = (sizeof(DBG_STR_Debug_Status_Strings) / sizeof (DBG_STR_VALUE_STRING));
            
            break;
            
        }        
        
        case DBG_STR_VALUE_STRING_TYPE_DBG_CMD_OP :
        {
            /* Setup the value strings array. */
            
            p_value_strings = &DBG_STR_Debug_Command_Op_Strings[0];
            value_strings_count = (sizeof(DBG_STR_Debug_Command_Op_Strings) / sizeof (DBG_STR_VALUE_STRING));
            
            break;
            
        }        
        
        case DBG_STR_VALUE_STRING_TYPE_DBG_EVT_ID :
        {
            /* Setup the value strings array. */
            
            p_value_strings = &DBG_STR_Debug_Event_Id_Strings[0];
            value_strings_count = (sizeof(DBG_STR_Debug_Event_Id_Strings) / sizeof (DBG_STR_VALUE_STRING));
            
            break;
            
        }         
        
        case DBG_STR_VALUE_STRING_TYPE_NONE:
        default:
        {
            /* ERROR: Invalid value string type. */
            
            dbg_status = DBG_STATUS_INVALID_TYPE;
            
            break;
            
        }
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Search value strings array for a matching value. */
    
        i = 0;
        while((p_value_strings[i].value != value) &&
              (i < value_strings_count))
        {
            i++;
        
        }
    
        /* Determine if we found the target value string and update the
           return parameter accordingly. */
    
        *p_string = (p_value_strings[i].value == value) ? p_value_strings[i].string : &DBG_STR_Unknown_String[0];

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_STR_String_Combine
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function combines several NULL-terminated strings. 
*                                                                                                 
*   INPUTS                                                               
*
*       string_0 - Pointer to the first NULL-terminated string.
*
*       string_1 - Pointer to the second NULL-terminated string.
*
*       string_2 - Pointer to the third NULL-terminated string.
*
*       string_3 - Pointer to the fourth NULL-terminated string.
*
*       string_4 - Pointer to the fifth NULL-terminated string.
*
*       string_max_length - The maximum length (including the NULL
*                           terminator) that the new string may be.  This
*                           should always be lesser than or equal to the 
*                           size of the character array pointed to by the
*                           pNewString parameter.
*
*       string - Pointer to a string that will be updated to be the 
*                 combined (new) string.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       Returns the size of the new string (in characters).
*                                                                      
*************************************************************************/
UINT     DBG_STR_String_Combine(CHAR *          string_0,
                                CHAR *          string_1,
                                CHAR *          string_2,
                                CHAR *          string_3,
                                CHAR *          string_4,
                                UINT            string_max_length,
                                CHAR *          string)
{
    CHAR *          strings[5];
    UINT            string_length;
    UINT            max_string_length;
    UINT            i;
    
    /* Set initial string length. */
    
    string_length = 0;     

    /* Set maximum string length accounting for the NULL terminator. */
    
    max_string_length = (string_max_length - 1);

    /* Setup strings array. */
    
    strings[0] = string_0;
    strings[1] = string_1;
    strings[2] = string_2;
    strings[3] = string_3;
    strings[4] = string_4;    

    /* Combine all strings in the array as long as there is room left. */
    
    i = 0;
    while ((i < 5) &&
           (string_length < max_string_length))
    {
        /* Ensure the component string is not empty. */

        if (strings[i] != NU_NULL)
        {
            /* Add the component string to the new string. */
        
            (VOID)strncpy(&string[string_length],
                          &strings[i][0],
                          (max_string_length - string_length));
        
            /* Update the string length. */
            
            string_length = strlen(&string[0]);

        }
        
        /* Move to the next component string. */
        
        i++;

    }

    /* Ensure a NULL-terminator. */
    
    string[string_length] = NU_NULL;

    return (string_length);
}

/*************************************************************************
* 
*   FUNCTION                                                             
*                                                                      
*       DBG_STR_String_From_UINT
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a UINT value to a string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The UINT value.
*
*       radix - The radix of the output string.
*
*       inc_prefix - Include the prefix?
*
*       inc_lead_zeros - Include the leading zeros?
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Success of the operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_STR_String_From_UINT(CHAR *      str,
                                    UINT        value,
                                    UINT        radix,
                                    BOOLEAN     inc_prefix,
                                    BOOLEAN     inc_lead_zeros)
{
    DBG_STATUS      dbg_status;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Perform setup based on the radix to be used. */

    switch (radix)
    {
        case DBG_STR_RADIX_BINARY:
        {
            dbg_status = dbg_str_string_from_uint_binary(str,
                                                         value,
                                                         inc_prefix,
                                                         inc_lead_zeros);
            
            break;
    
        }
        
        case DBG_STR_RADIX_DECIMAL:
        {
            dbg_status = dbg_str_string_from_unit_decimal(str,
                                                          value,
                                                          inc_prefix,
                                                          inc_lead_zeros);
            
            break;
    
        }

        case DBG_STR_RADIX_HEXADECIMAL:
        {
            dbg_status = dbg_str_string_from_uint_hexadecimal(str,
                                                              value,
                                                              inc_prefix,
                                                              inc_lead_zeros);
            
            break;
    
        }

        default:
        {
            /* ERROR: Unsupported radix. */

            dbg_status = DBG_STATUS_FAILED;
        
            break;
            
        }
        
    }
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_STR_String_To_UINT
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function interprets a string and translates it to a UINT
*       value. It is aware of values in hexadecimal format (i.e. 0xFFFF).
*                                                                                                 
*   INPUTS                                                               
*         
*       str - The string.
*
*       pValue - Return parameter that will contain the converted string
*                value if the operation is successful.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Success of the operation.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_STR_String_To_UINT(CHAR *    str,
                                  UINT *    p_value)
{
    DBG_STATUS      dbg_status;
    UINT            str_radix;
    UINT            str_value;
    UINT            ch_value;
    UINT            i;
    UINT            str_length;
    BOOLEAN         has_prefix;
    UINT            position_value;
    UINT            str_value_length;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Determine the length of the string. */

    str_length = strlen(str);
    
    /* Determine the radix of the string value. */

    str_radix = dbg_str_string_radix(str,
                                     &has_prefix);

    /* Set the string value length value (number of string characters 
       that will be processed) dependent on whether the string has a 
       prefix or not. */
    
    if (has_prefix)
    {
        str_value_length = (str_length - 2);

    }
    else
    {
        str_value_length = str_length;
        
    }
    
    /* Process string from least significant character to most
       significant. */

    str_value = 0;
    position_value = str_radix;
    i = 0;
    while ((i < str_value_length) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Translate character to numerical value. */

        dbg_status = dbg_str_char_to_uint(str[((str_length - 1) - i)],
                                          &ch_value,
                                          str_radix);

        if (dbg_status == DBG_STATUS_OK)
        {
            /* Determine how to proceed based on the current
                power of the character value (tracked by loop 
                iteration counter). */

            if (i == 0)
            {
                /* Add value of character to string value when
                   power is 0 (units place). */

                str_value += ch_value;

            }
            else
            {
                /* Add value of character to string value when
                   power is greater than units... */

                str_value += (ch_value * position_value);

                /* Update the position value. */

                position_value *= str_radix;
                
            }

            /* Move to next character. */

            i++;

        }
         
    }

    /* Determine if the operation succeeded. */

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update return parameter. */

        *p_value = str_value;
    
    }
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_STR_String_From_BYTE
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function translates a BYTE value to a string.
*                                                                                                 
*   INPUTS                                                               
*         
*       str - Return parameter that will contain the converted string if
*             the operation is successful.
*
*       value - The UINT value.
*
*       radix - The radix of the output string.
*
*       inc_prefix - Include the prefix?
*
*       inc_lead_zeros - Include the leading zeros?
*
*       inc_null_term - Include the NULL terminator?
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Success of the operation.
*
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       <other> - Indicates (other) internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_STR_String_From_BYTE(CHAR *      str,
                                    UINT        value,
                                    UINT        radix,
                                    BOOLEAN     inc_prefix,
                                    BOOLEAN     inc_lead_zeros,
                                    BOOLEAN     inc_null_term)
{
    DBG_STATUS      dbg_status;

    /* Perform setup based on the radix to be used. */

    switch (radix)
    {
        case DBG_STR_RADIX_BINARY:
        {
            dbg_status = dbg_str_string_from_byte_binary(str,
                                                         value,
                                                         inc_prefix,
                                                         inc_lead_zeros,
                                                         inc_null_term);
            
            break;
    
        }
        
        case DBG_STR_RADIX_DECIMAL:
        {
            dbg_status = dbg_str_string_from_byte_decimal(str,
                                                          value,
                                                          inc_prefix,
                                                          inc_lead_zeros,
                                                          inc_null_term);
            
            break;
    
        }

        case DBG_STR_RADIX_HEXADECIMAL:
        {
            dbg_status = dbg_str_string_from_type_hexadecimal(str,
                                                              value,
                                                              inc_prefix,
                                                              inc_lead_zeros,
                                                              inc_null_term);
            
            break;
    
        }

        default:
        {
            /* ERROR: Unsupported radix. */

            dbg_status = DBG_STATUS_FAILED;
        
            break;
            
        }
        
    }
        
    return (dbg_status);
}
