/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  nu_math.c                                                    
*
* DESCRIPTION
*
*  Fixed-point math functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  Fix_mul
*  Fix_sqrt
*  Fix_div
*  iCos
*  iSin
*  Nu_asin
*  dFix_add
*  dFix_sub
*  dFix_mul
*  dFix_div
*  MATH_sin
*  MATH_cos
*  MATH_sqr
*  MATH_sqrt
*
* DEPENDENCIES
*
*  rs_base.h
*  nu_math.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/nu_math.h"

/* Define if we are using floating point variables or not */
/* These floating point variables and functions are not used */
/* when FIXPOINT is defined.  FIXPOINT is defined by default. */

#ifndef FIXPOINT

/* Union defined to be used in the following macros */
typedef union 
{
  double value;
  struct 
  {
    UINT32 msw;
    UINT32 lsw;
  } parts;
} double_shape_type;

/* macros to be used with the math functions in this file */

#define GET_HIGH_WORD(w,x) w = ((INT32 *)&(x))[1]

#define INSERT_WORDS(d,ix0,ix1)                                 \
do                                                              \
{                                                               \
    double_shape_type iw_u;                                     \
    iw_u.parts.msw = (ix0);                                     \
    iw_u.parts.lsw = (ix1);                                     \
    (d) = iw_u.value;                                           \
                                                                \
} while (0)

#define EXTRACT_WORDS(ix0,ix1,d)                                \
do                                                              \
{                                                               \
    double_shape_type ew_u;                                     \
    ew_u.value = (d);                                           \
    (ix0) = ew_u.parts.msw;                                     \
    (ix1) = ew_u.parts.lsw;                                     \
                                                                \
} while (0)

#endif /* #ifndef FIXPOINT */

#ifndef CPU_FLOAT
/***************************************************************************
* FUNCTION
*
*    Fix_mul
*
* DESCRIPTION
*
*    Multiply function.
*
* INPUTS
*
*    SIGNED first  - First number.
*
*    SIGNED second - Second number.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED Fix_mul( SIGNED first, SIGNED second)
{
    SIGNED   result;
    UNSIGNED ai, bi, af, bf, item, tem;
    INT16    sign = 1;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (first < 0)
    {
        first = -first;
        sign *= -1;
    }
    if(second < 0)  
    {
        second = -second;
        sign *= -1;
    }

    af = (UNSIGNED) (first & 0xFFFF);
    bf = (UNSIGNED) (second & 0xFFFF);
    ai = (UNSIGNED) (first >> 16);
    bi = (UNSIGNED) (second >> 16);

    item = ai * bi;
    if(item > 32767)
    {
        /* Number is too big */
        result = 0x7FFFFFFF * sign; 
    }
    else
    {
        tem = af * bf;
        item = (item << 16) + (af * bi) + (ai * bf) + (tem >> 16);
        if(item >= 0x80000000)
        {
            /* Number is too big */
            result = 0x7FFFFFFF * sign; 
        }
        else
        {
            result = sign * ((SIGNED) item);
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(result);
}

/***************************************************************************
* FUNCTION
*
*    Fix_sqrt
*
* DESCRIPTION
*
*    Square Root function.
*
* INPUTS
*
*    SIGNED root - Number.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED Fix_sqrt(UNSIGNED root)
{
    INT32    i;
    UNSIGNED init;
    UNSIGNED tem = 0;
    dblFix   first;
    dblFix   second;
    dblFix   result;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( root > 0)
    {
        init = (1 << 16);
        
        for( i = 0; i < 10; i++)
        {
            if (root >= 0x80000000)
            {
                first.fUpper = 0;
                first.fLower = root;
                second.fUpper = 0;
                second.fLower = init;
                dFix_div(&first,&second,&result);

                tem = ((init + (result.fLower)) >> 1);

            }
            else
            {
                tem = ((init + (Fix_div(root,init))) >> 1);
            }
            
            if( init > tem)
            {
                if( (init - tem) < 0x0080)
                {
                    break;
                }
            }
            else
            {
                if( (tem - init) < 0x0080)
                {
                    break;
                }
            }
            init = tem;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(tem);        
}

/***************************************************************************
* FUNCTION
*
*    Fix_div
*
* DESCRIPTION
*
*    Divide function.
*
* INPUTS
*
*    SIGNED a - First number.
*
*    SIGNED b - Second number.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED Fix_div(SIGNED a, SIGNED b)
{
    INT16    sign = 1;
    UNSIGNED aa, bb;
    SIGNED   result = 0;
    INT32    i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (a != 0) && (b != 0) )
    {
        if(a < 0)
        {
            a = -a;
            sign *= -1;
        }
        if(b < 0)
        {
            b = -b;
            sign *= -1;
        }

        aa = (UNSIGNED) a;
        bb = (UNSIGNED) b;

        if(aa >= bb)
        {
            /* get integer part */
            result = (aa / bb);
            aa -= (result * bb);
        }

        /* now get fractional part by comparison method */
        for (i = 0; i < 16; i++)
        {
            result = (result << 1);
            aa = (aa << 1);
            if (bb > aa)
            {
                continue;
            }

            result += 1;
            aa -= bb;
            if (!(aa))
            {
                /* done early */
                result = (result << (15 - i));
                break;
            }
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(result * sign);
}

/***************************************************************************
* FUNCTION
*
*    iCos
*
* DESCRIPTION
*
*    Function iCos returns a double-word fixed point fraction of the sine of ANGLE 
*    which is specified in integer tenth of degrees (0-3599 = 0 to 359.9 degrees). 
*    The returned word value in ax is an unsigned integer fraction of the sine of 
*    ANGLE with an implied decimal point to the left of the high order bit. 
*    Register bx is set to 0000h for positive sine values, or to FF00h for 
*    negative sine values.  AX is returned with an unsigned positive fraction 
*    value for the sine/cosine function.  BX indicates whether the sine/cosine is 
*    positive or negative.  For example:
*
*         bx . ax
*            ---- ----
*        0000.FFFF   = +0.99999 (+1.0)
*        0000.8000   = +0.5
*        0000.4000   = +0.25
*        0000.2000   = +0.125
*        0000.0000   =  0
*        FF00.2000   = -0.125
*        FF00.8000   = -0.5
*        FF00.4000   = -0.25
*        FF00.FFFF   = -0.99999 (-1.0)
*
*    Sine values are extracted from the sin lookup table based on the following 
*    identities:
*
*    sin(angle) =  000.0-089.9    sin(angle)
*        090.0-179.9  sin(180-angle)
*        180.0-269.9 -sin(angle-180)
*        270.0-359.9 -sin(360-angle)
*    cos(angle) =  sin(angle+90)
*
*    The sinTbl array defines the value for each integer angle to an accuracy of 
*    0.0000152 (1/64636).  Linear interpolation is used to compute tenth of degree 
*    angle increments.
*
* INPUTS
*
*    SIGNED Angle - Angle.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED iCos(SIGNED Angle)
{
    SIGNED value = 0x0000FFFF;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( Angle != 0)
    {
        value = iSin(Angle + 900);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    iSin
*
* DESCRIPTION
*
*    Sine function.
*
* INPUTS
*
*    SIGNED Angle  - Angle.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED iSin(SIGNED Angle)
{
    INT16 temAngle, temAngleFract, sign = 1;
    SIGNED result = 0;

    UINT16 sinTbl[]= {
        0x0000,  0x0478,  0x08EF,  0x0D66,  0x11DC, /*sin  0- 4 */
        0x1650,  0x1AC2,  0x1F33,  0x23A1,  0x280C, /*sin  5- 9 */
        0x2C74,  0x30D9,  0x353A,  0x3996,  0x3DEF, /*sin 10-14 */
        0x4242,  0x4690,  0x4AD9,  0x4F1C,  0x5358, /*sin 15-19 */
        0x578F,  0x5BBE,  0x5FE6,  0x6407,  0x6820, /*sin 20-24 */
        0x6C31,  0x7039,  0x7439,  0x782F,  0x7C1C, /*sin 25-29 */
        0x8000,  0x83DA,  0x87A9,  0x8B6D,  0x8F27, /*sin 30-34 */
        0x92D6,  0x9679,  0x9A11,  0x9D9C,  0xA11B, /*sin 35-39 */
        0xA48E,  0xA7F3,  0xAB4C,  0xAE97,  0xB1D5, /*sin 40-44 */
        0xB505,  0xB827,  0xBB3A,  0xBE3F,  0xC135, /*sin 45-49 */
        0xC41B,  0xC6F3,  0xC9BB,  0xCC73,  0xCF1C, /*sin 50-54 */
        0xD1B4,  0xD43C,  0xD6B3,  0xD91A,  0xDB6F, /*sin 55-59 */
        0xDDB4,  0xDFE7,  0xE209,  0xE419,  0xE617, /*sin 60-64 */
        0xE804,  0xE9DE,  0xEBA6,  0xED5C,  0xEEFF, /*sin 65-69 */
        0xF090,  0xF20E,  0xF378,  0xF4D0,  0xF615, /*sin 70-74 */
        0xF747,  0xF865,  0xF970,  0xFA68,  0xFB4C, /*sin 75-79 */
        0xFC1C,  0xFCD9,  0xFD82,  0xFE18,  0xFE99, /*sin 80-84 */
        0xFF07,  0xFF60,  0xFFA6,  0xFFD8,  0xFFF6, /*sin 85-89 */
        0xFFFF};                                    /*sin 90 */


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( Angle != 0)
    {
        while( Angle < 0)
        {
            Angle += 3600;
        }

        while( Angle >= 3600)
        {
            Angle -= 3600;
        }
    
        if( Angle > 2700) 
        {
            Angle = 3600 - Angle;
            sign = -1;
        }
        else if( Angle > 1800)
        {
            Angle = Angle - 1800;
            sign = -1;
        }
        else if( Angle > 900)
        {
            Angle = 1800 - Angle;
        }

        temAngle = Angle / 10;
        temAngleFract = Angle - (10 * temAngle);
    
        result = (SIGNED) sinTbl[temAngle];
        if( temAngleFract)
        {
            if((temAngle + 1) <= 90)
            {
                result += ((temAngleFract * ((SIGNED) sinTbl[temAngle + 1]
                           - result)) / 10);
            }
        }
        result *= sign;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (result);
}

/***************************************************************************
* FUNCTION
*
*    Nu_asin
*
* DESCRIPTION
*
*    Arcsine function.
*
* INPUTS
*
*    SIGNED radian  - Angle.
*
* OUTPUTS
*
*    SIGNED - Returns the result.
*
***************************************************************************/
SIGNED Nu_asin(SIGNED radian)
{
    UINT16 sinTbl[]= {
        0x0000,  0x0478,  0x08EF,  0x0D66,  0x11DC, /*sin  0- 4 */
        0x1650,  0x1AC2,  0x1F33,  0x23A1,  0x280C, /*sin  5- 9 */
        0x2C74,  0x30D9,  0x353A,  0x3996,  0x3DEF, /*sin 10-14 */
        0x4242,  0x4690,  0x4AD9,  0x4F1C,  0x5358, /*sin 15-19 */
        0x578F,  0x5BBE,  0x5FE6,  0x6407,  0x6820, /*sin 20-24 */
        0x6C31,  0x7039,  0x7439,  0x782F,  0x7C1C, /*sin 25-29 */
        0x8000,  0x83DA,  0x87A9,  0x8B6D,  0x8F27, /*sin 30-34 */
        0x92D6,  0x9679,  0x9A11,  0x9D9C,  0xA11B, /*sin 35-39 */
        0xA48E,  0xA7F3,  0xAB4C,  0xAE97,  0xB1D5, /*sin 40-44 */
        0xB505,  0xB827,  0xBB3A,  0xBE3F,  0xC135, /*sin 45-49 */
        0xC41B,  0xC6F3,  0xC9BB,  0xCC73,  0xCF1C, /*sin 50-54 */
        0xD1B4,  0xD43C,  0xD6B3,  0xD91A,  0xDB6F, /*sin 55-59 */
        0xDDB4,  0xDFE7,  0xE209,  0xE419,  0xE617, /*sin 60-64 */
        0xE804,  0xE9DE,  0xEBA6,  0xED5C,  0xEEFF, /*sin 65-69 */
        0xF090,  0xF20E,  0xF378,  0xF4D0,  0xF615, /*sin 70-74 */
        0xF747,  0xF865,  0xF970,  0xFA68,  0xFB4C, /*sin 75-79 */
        0xFC1C,  0xFCD9,  0xFD82,  0xFE18,  0xFE99, /*sin 80-84 */
        0xFF07,  0xFF60,  0xFFA6,  0xFFD8,  0xFFF6, /*sin 85-89 */
        0xFFFF};                                    /*sin 90 */
    UINT16 compare, i = 1;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    compare = (UINT16) (radian & 0x0000FFFF);
    
    while(sinTbl[i] < compare)
    {
        i++;
    }

    compare = 10 * (sinTbl[i] - compare);
    if( compare)
    {
        /* interpolate between table points */
        compare /= (sinTbl[i] - sinTbl[i-1]);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return((i*10) - compare);
}

/***************************************************************************
* FUNCTION
*
*    dFix_add
*
* DESCRIPTION
*
*    Add function.
*
* INPUTS
*
*    dblFix *first  - Pointer to the first number.
*
*    dblFix *second - Pointer to the second.
*
*    dblFix *result - Pointer to the result
*
* OUTPUTS
*
*    INT32 - Returns 0
*
***************************************************************************/
INT32 dFix_add(dblFix *first, dblFix *second, dblFix *result)
{
    INT32    lCarry = 0;
    UNSIGNED dTempL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get partial sum of lower */
    dTempL = first->fLower + second->fLower;

    /* test for overflow */
    if (first->fLower & 0x80000000)
    {
        /* first_msb & (second_msb | !result_msb) is a carry */
        if ((second->fLower & 0x80000000) || (!(dTempL & 0x80000000)))
        {
            lCarry = 1;
        }
    }
    else
    {
        /* (second_msb & !result_msb) is a carry */
        if ((second->fLower & 0x80000000) && (!(dTempL & 0x80000000)))
        {
            lCarry = 1;
        }
    }

    /* now get sum of upper and carry */
    result->fUpper = first->fUpper + second->fUpper + lCarry;
    result->fLower = dTempL;

    /* Return to user mode */
    NU_USER_MODE();

    return(0);
}

/***************************************************************************
* FUNCTION
*
*    dFix_sub
*
* DESCRIPTION
*
*    Subtract function.
*
* INPUTS
*
*    dblFix *first  - Pointer to the first number.
*
*    dblFix *second - Pointer to the second.
*
*    dblFix *result - Pointer to the result
*
* OUTPUTS
*
*    INT32 - Returns 0
*
***************************************************************************/
INT32 dFix_sub(dblFix *first, dblFix *second, dblFix *result)
{
    INT32    lBorrow = 0;
    UNSIGNED dTempL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get partial sum of lower */
    dTempL = first->fLower - second->fLower;

    /* test for overflow */
    if (first->fLower & 0x80000000)
    {
        /* first_msb & second_msb & result_msb) is a borrow */
        if( (second->fLower & 0x80000000) && (dTempL & 0x80000000) )
        {
            lBorrow = 1;
        }
    }
    else
    {
        /* (result_msb | second_msb) is a borrow */
        if( (dTempL & 0x80000000) || (second->fLower & 0x80000000) )
        {
            lBorrow = 1;
        }
    }

    /* now subtract upper and borrow */
    result->fUpper = first->fUpper - second->fUpper - lBorrow;
    result->fLower = dTempL;

    /* Return to user mode */
    NU_USER_MODE();

    return(0);
}


/***************************************************************************
* FUNCTION
*
*    dFix_mul
*
* DESCRIPTION
*
*    Multiply function.
*
* INPUTS
*
*    dblFix *first  - Pointer to the first number.
*
*    dblFix *second - Pointer to the second.
*
*    dblFix *result - Pointer to the result
*
* OUTPUTS
*
*    INT32 - Returns overflow.
*
***************************************************************************/
INT32 dFix_mul(dblFix *first, dblFix *second, dblFix *result)
{
    INT16    Done = NU_FALSE;
    INT32    value;
    UNSIGNED fA, fB, fC, fD;
    UNSIGNED sA, sB, sC, sD;
    UNSIGNED rA, rB, rC, rD;
    INT32    sign = 1;
    dblFix   dFtemp, dblSum, dblZero = {0,0};


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    do
    {
        /* test for trivial case */
        if( ( (first->fUpper == 0) && (first->fLower == 0)) ||
            ((second->fUpper == 0) && (second->fLower == 0)) )
        {
            /* result is 0 */
            result->fUpper = 0;
            result->fLower = 0;
            value = 0;
            Done = NU_TRUE;
            break; 
        }

        /* split the input into 4 14-bit short positive values */
        if(first->fUpper < 0)
        {
            /* negative data - invert and set sign */
            sign = -1;
            dFix_sub(&dblZero, first, &dFtemp);
            fD = (dFtemp.fLower & 0x00003fff); 
            fC = ((dFtemp.fLower >> 14) & 0x00003fff);
            fB = (((dFtemp.fLower >> 28) | (dFtemp.fUpper << 4)) & 0x00003fff);
            fA = ((dFtemp.fUpper >> 10) & 0x00003fff);
        }
        else
        {   /* split it */
            fD = (first->fLower & 0x00003fff); 
            fC = ((first->fLower >> 14) & 0x00003fff);
            fB = (((first->fLower >> 28) | (first->fUpper << 4)) & 0x00003fff);
            fA = ((first->fUpper >> 10) & 0x00003fff);
        }

        if( second->fUpper < 0)
        {
            /* negative data - invert and set sign */
            sign *= -1;
            dFix_sub(&dblZero, second, &dFtemp);
            sD = (dFtemp.fLower & 0x00003fff); 
            sC = ((dFtemp.fLower >> 14) & 0x00003fff);
            sB = (((dFtemp.fLower >> 28) | (dFtemp.fUpper << 4)) & 0x00003fff);
            sA = ((dFtemp.fUpper >> 10) & 0x00003fff);
        }
        else
        {
            /* split it */
            sD = (second->fLower & 0x00003fff); 
            sC = ((second->fLower >> 14) & 0x00003fff);
            sB = (((second->fLower >> 28) | (second->fUpper << 4)) & 0x00003fff);
            sA = ((second->fUpper >> 10) & 0x00003fff);
        }

        /* calculate partial products */
        /* first check for trivial overflow */
        rA = fA * sA;
        if( rA )
        {   
            /* overflow */
            value = -1; 
            Done = NU_TRUE;
            break; 
        }

        rA = fA * sB;
        if( rA )
        {
            /* overflow */
            value = -1; 
            Done = NU_TRUE;
            break; 
        }

        rA = fB * sA;
        if( rA )
        {
            /* overflow */
            value = -1; 
            Done = NU_TRUE;
            break; 
        }

        /* now go to bottom for rounding */
        rD = (((fD * sD) + 0x00002000) >> 14);
        rD += ((fC  * sD) + (fD * sC));
        rC = ((fB * sD) + (fC * sC) + (fD * sB));
        rB = ((fA * sD) + (fB * sC) + (fC * sB) + (fD * sA));
        rA = ((fA * sC) + (fB * sB) + (fC * sA));

        /* now combine back to result */
        if( sign < 0 )
        {
            /* negative result */
            dFtemp.fLower = (rD << 2);
            dFtemp.fUpper = 0;
            dblSum.fLower = (rC << 16);
            dblSum.fUpper = (rC >> 16);
            dFix_add(&dblSum, &dFtemp, &dblSum);
            dblSum.fLower = (((dblSum.fLower + 0x00000008) >> 4)
                            + (dblSum.fUpper << 28));
            dblSum.fUpper = (dblSum.fUpper >> 4);
            dFtemp.fLower = (rB << 26);
            dFtemp.fUpper = (rB >> 6);
            dFix_add(&dblSum, &dFtemp, &dblSum);
            dFtemp.fLower = 0;
            dFtemp.fUpper = (rA << 8);
            dFix_add(&dblSum, &dFtemp, &dblSum);
            dFix_sub(&dblZero, &dblSum, result);
        }
        else
        {
            dFtemp.fLower = (rD << 2);
            dFtemp.fUpper = 0;
            dblSum.fLower = (rC << 16);
            dblSum.fUpper = (rC >> 16);
            dFix_add(&dblSum, &dFtemp, &dblSum);
            dblSum.fLower = (((dblSum.fLower + 0x00000008) >> 4)
                        + (dblSum.fUpper << 28));
            dblSum.fUpper = (dblSum.fUpper >> 4);
            dFtemp.fLower = (rB << 26);
            dFtemp.fUpper = (rB >> 6);
            dFix_add(&dblSum, &dFtemp, &dblSum);
            dFtemp.fLower = 0;
            dFtemp.fUpper = (rA << 8);
            dFix_add(&dblSum, &dFtemp, result);
        }

        /* No overflow since otherwise we can't reach this point. */
        value = 0;
        
        Done = NU_TRUE;
        
        break;
        
    } while (!Done);

    /* to remove paradigm warning */
    (VOID)Done;

    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}

/***************************************************************************
* FUNCTION
*
*    dFix_div
*
* DESCRIPTION
*
*    Divide function.
*
* INPUTS
*
*    dblFix *first  - Pointer to the first number.
*
*    dblFix *second - Pointer to the second.
*
*    dblFix *result - Pointer to the result
*
* OUTPUTS
*
*    INT32 - Returns overflow.
*
***************************************************************************/
INT32 dFix_div(dblFix *first, dblFix *second, dblFix *result)
{
    INT32  value;
    dblFix dFtemp1, dFtemp2, dFtemp3 = {0,0};
    dblFix dblZero = {0,0}, dblOne = {0,0x00010000};
    INT16  sign = 1;
    INT32  i;
    UINT16 qFract = 0;
    NU_SUPERV_USER_VARIABLES

    result->fUpper = 0;
    result->fLower = 0;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* test for trivial case */
    if( (first->fUpper == 0) && (first->fLower == 0) )
    {
        /* result is 0 */
        value = 0;
    }
    else if( (second->fUpper == 0) && (second->fLower == 0) )
    {
        /* divide by 0 */
        value = -1;
    }
    else
    {
        /* set up temp data */
        if( first->fUpper < 0 )
        {
            /* negative data - invert and set sign */
            sign = -1;
            dFix_sub(&dblZero, first, &dFtemp1);
        }
        else
        {
            /* split it */
            dFtemp1.fUpper = first->fUpper;
            dFtemp1.fLower = first->fLower;
        }
        if( second->fUpper < 0 )
        {
            /* negative data - invert and set sign */
            sign *= -1;
            dFix_sub(&dblZero, second, &dFtemp2);
        }
        else
        {
            /* split it */
            dFtemp2.fUpper = second->fUpper;
            dFtemp2.fLower = second->fLower;
        }

        /* get integer part if any */
        for( i = 0; i < 48; i++)
        {
            /* check if divisor > dividend */
            dFix_sub(&dFtemp1, &dFtemp2, &dFtemp3);

            if( dFtemp3.fUpper & 0x80000000)
            {
                break;
            }

            /* no so shift divisor left */
            dFtemp2.fUpper = (dFtemp2.fUpper << 1);

            if( dFtemp2.fLower & 0x80000000)
            {
                dFtemp2.fUpper++;
            }
            dFtemp2.fLower = (dFtemp2.fLower << 1);
        }

        /* now subtract and shift divisor back right */
        while( i-- )
        {
            /* shift divisor right */
            dFtemp2.fLower = (dFtemp2.fLower >> 1);
            if( dFtemp2.fUpper & 0x00000001)
            {
                dFtemp2.fLower += 0x80000000;
            }
            dFtemp2.fUpper = (dFtemp2.fUpper >> 1);

            /* shift result integer left */
            result->fUpper = (result->fUpper << 1);
            if( result->fLower & 0x80000000 )
            {
                result->fUpper++;
            }
            result->fLower = (result->fLower << 1);

            /* check if divisor > dividend */
            dFix_sub(&dFtemp1, &dFtemp2, &dFtemp3);
            if( dFtemp3.fUpper & 0x80000000)
            {
                continue;
            }

            /* got a bit */
            dFix_add(result, &dblOne, result);

            /* go to next cycle */
            dFtemp1.fUpper = dFtemp3.fUpper;
            dFtemp1.fLower = dFtemp3.fLower;
        }

        /* now get fractional part by comparison method */
        for( i = 0; i < 16; i++)
        {
            qFract = (qFract << 1);
            dFtemp1.fUpper = (dFtemp1.fUpper << 1);
            if( dFtemp1.fLower & 0x80000000)
            {
                dFtemp1.fUpper++;
            }
            dFtemp1.fLower = (dFtemp1.fLower << 1);

            dFix_sub(&dFtemp1, &dFtemp2, &dFtemp3);
            if( dFtemp3.fUpper & 0x80000000)
            {
                continue;
            }

            /* got a bit */
            qFract += 1;
            if( (dFtemp3.fLower == 0) && (dFtemp3.fUpper == 0) )
            {
                /* done early */
                qFract = (qFract << (15 - i));
                break;
            }

            /* go to next cycle */
            dFtemp1.fUpper = dFtemp3.fUpper;
            dFtemp1.fLower = dFtemp3.fLower;
        }

        /* finally, combine integer and fraction */
        result->fLower = result->fLower | ((UNSIGNED) qFract);
        if( sign < 0 )
        {
            /* negative result */
            dFix_sub(&dblZero, result, result);
        }

        value = 0;
    } /* else */

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}
#else
/***************************************************************************
* FUNCTION
*
*    MATH_sin
*
* DESCRIPTION
*
*    Function MATH_sin calculates the sine value.
*
* INPUTS
*
*    double x  - The value that we are trying to get the sine of.
*
*    double y  - the tail of x
*
*    INT32  iy - indicates whether y is 0. (if iy = 0, y assume to be 0). .
*
* OUTPUTS
*
*    double ret_val - sine value.
*
***************************************************************************/
double MATH_sin(double x, double y, INT32 iy)
{
    double z,r,v;
    double ret_val;

    /* static variables to be used in this function. */
    static const double 
    half =  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
    S1   = -1.66666666666666324348e-01, /* 0xBFC55555, 0x55555549 */
    S2   =  8.33333333332248946124e-03, /* 0x3F811111, 0x1110F8A6 */
    S3   = -1.98412698298579493134e-04, /* 0xBF2A01A0, 0x19C161D5 */
    S4   =  2.75573137070700676789e-06, /* 0x3EC71DE3, 0x57B1FE7D */
    S5   = -2.50507602534068634195e-08, /* 0xBE5AE5E6, 0x8A2B9CEB */
    S6   =  1.58969099521155010221e-10; /* 0x3DE5D93A, 0x5ACFD57C */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Get the squared value of x */
    z =  x * x;

    /* Get the cubed value of x */
    v =  z * x;

    /* sin(x) is approximated by a polynomial of degree 13 on [0,pi/4] */
    r =  S2 + z * (S3 + z * (S4 + z * ( S5 + z * S6)));

    /* Check if we are starting on the y = 0 axis */
    if(iy == 0) 
    {
        ret_val = x + v * (S1 + z * r);
    }
    else
    {
        /* We are not starting on the y = 0 axis */
        ret_val = x - ((z * (half * y - v * r) - y) - v * S1);
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* return */
    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*    MATH_cos
*
* DESCRIPTION
*
*    Function MATH_cos calculates the cosine value.
*
* INPUTS
*
*    double x  - The value that we are trying to get the cosine of.
*
*    double y  - the tail of x
*
* OUTPUTS
*
*    double ret_val - the cosine of x.
*
***************************************************************************/
double MATH_cos(double x, double y)
{
    double a,hz,z,r,qx,ret_val;
    INT32 ix;
    UINT8 done = NU_FALSE;

    /* static variables to be used in this function. */
    static const double 
    one  =  1.00000000000000000000e+00, /* 0x3FF00000, 0x00000000 */
    C1   =  4.16666666666666019037e-02, /* 0x3FA55555, 0x5555554C */
    C2   = -1.38888888888741095749e-03, /* 0xBF56C16C, 0x16C15177 */
    C3   =  2.48015872894767294178e-05, /* 0x3EFA01A0, 0x19CB1590 */
    C4   = -2.75573143513906633035e-07, /* 0xBE927E4F, 0x809C52AD */
    C5   =  2.08757232129817482790e-09, /* 0x3E21EE9E, 0xBDB4B1C4 */
    C6   = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    GET_HIGH_WORD(ix,x);

    /* ix = |x|'s high word*/
    ix &= 0x7fffffff;
    
    /* if x < 2**27 */
    if(ix < 0x3e400000) 
    {
        if((INT32)x == 0) 
        {
            /* generate inexact */
            ret_val = one;

            /* set done flag to exit the function */
            done = NU_TRUE;
        }
    }

    if (!done)
    {
        /* get the square of x */
        z  = x * x;

        /* cos(x) is approximated by a polynomial of degree 14 on [0,pi/4] */
        r  = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));

        /* if |x| < 0.3 */
        if(ix < 0x3FD33333) 
        {
            /* a correction term is necessary in cos(x) */
            ret_val = one - (0.5 * z - (z * r - x * y));
        }
        else 
        {
            /* let qx = |x|/4 with the last 32 bits mask off, and */
            /* if x > 0.78125, let qx = 0.28125. */
            if(ix > 0x3fe90000) 
            {                
                qx = 0.28125;
            } 
            else 
            {
                /* x/4 */
                INSERT_WORDS(qx,ix - 0x00200000,0);
            }

            /* Note that 1-qx and (x*x/2-qx) is EXACT here, and the */
            /* magnitude of the latter is at least a quarter of x*x/2, */
            /* thus, reducing the rounding error in the subtraction. */
            hz = 0.5 * z - qx;
            a  = one - qx;
        
            ret_val = a - (hz - (z * r - x * y));
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* return */
    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*    MATH_sqr
*
* DESCRIPTION
*
*    Function MATH_sqr helps MATH_sqrt calculate the square root.
*
* INPUTS
*
*    double b - initial starting point.
*
*    double a - initial end point.
*
*    double v - The value that we need to find the square root of.
*
* OUTPUTS
*
*    double ret_val - The square root of v.
*
***************************************************************************/
double MATH_sqr(double b,double a,double v)
{
    double mid;
    double ret_val;

    /* make sure we have a positive number. */
    if((abs(a - b)) <= 1) 
    {
        /* range too small? */
        ret_val = a;
    }
    else
    {
        /* compute the mid-point */
        mid = (b + a) / 2;

        /* check for high or low recurse */
        if(mid * mid >= v) 
        {
            /* high recurse */
            ret_val = MATH_sqr(b,mid,v);
        }
        else
        {
            /* low recurse */
            ret_val = MATH_sqr(mid,a,v);
        }
    }

    /* return */
    return ret_val;
}

/***************************************************************************
* FUNCTION
*
*    MATH_sqrt
*
* DESCRIPTION
*
*    Function MATH_sqrt computes the square root of v. Uses recursive 
*    MATH_sqr function..
*
* INPUTS
*
*    double v - The value that we need to find the square root of.
*
* OUTPUTS
*
*    The square root of v.
*
***************************************************************************/
double MATH_sqrt(double v)
{
    double ret_val;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    ret_val = MATH_sqr(0,v,v);
    
    /* Return to user mode */
    NU_USER_MODE();
    
    return ret_val;
}

#endif /* #ifndef CPU_FLOAT */
