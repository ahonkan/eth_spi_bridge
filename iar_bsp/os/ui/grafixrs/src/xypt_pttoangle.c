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
*  xypt_pttoangle.c                                                       
*
* DESCRIPTION
*
*  This file contains Point operation function - PtToAngle.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PtToAngle
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  xypt.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/xypt.h"

extern double MATH_sqrt(double v);

/***************************************************************************
* FUNCTION
*
*    PtToAngle
*
* DESCRIPTION
*
*    Function PtToAngle returns the angle, in integer tenth of degrees, for the 
*    line between the center of rectangle R and the specified point, PT.
*
* INPUTS
*
*    rect *R   - Pointer to the center of rectangle.
*
*    point *PT - Pointer to the specified point.
*
* OUTPUTS
*
*    SIGNED    - Returns the angle, in integer tenth of degrees.
*
***************************************************************************/
#ifdef FIXPOINT
SIGNED PtToAngle( rect *R, point *PT)
{
    rect tempRect;
    point tempPt;
    INT32 DltX, DltY;
    INT32 Ctr_X , Ctr_Y;
    SIGNED hypot;
    SIGNED lclAngle;
    SIGNED sin_rad;
    SIGNED ResX;
    SIGNED ResY;
    SIGNED qDltXX;
    SIGNED qDltYY;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {   
        /* convert from user to global */
        U2GR(*R, &tempRect , 1);
        U2GP(PT->X , PT->Y, &tempPt.X, &tempPt.Y, 1);
    }
    else
    {
        /* get rectangle */
        tempRect = * R; 

        /* get point */
        tempPt = *PT;   
    }

    /* find center point of rect */
    Ctr_X = (tempRect.Xmax + tempRect.Xmin) >> 1;
    Ctr_Y = (tempRect.Ymax + tempRect.Ymin) >> 1;

    /* compute delta from center to point */
    DltX = tempPt.X - Ctr_X;
    DltY = tempPt.Y - Ctr_Y;

    ResX = theGrafPort.portMap->pixResX;
    ResY = theGrafPort.portMap->pixResY;

    qDltXX = Fix_div((DltX << 16),(ResX << 16));
    qDltXX = Fix_mul(qDltXX,qDltXX);

    qDltYY = Fix_div((DltY << 16),(ResY << 16));

    /* take hypothesis of two X and Y coordination  */
    hypot = Fix_sqrt((UNSIGNED)(qDltXX + (Fix_mul(qDltYY,qDltYY))));
    
    /* if angle of Zero there is no graph */
    if( hypot == 0 )
    {
        lclAngle = 0;
    }

    else
    {
        /* Test to find out angle's position */
        if( DltY >= 0 )
        {
            /* negative angle */
            sin_rad = Fix_div(qDltYY,hypot);
            if( sin_rad >= 0x00010000 )
            {
                sin_rad = 0xFFFF;
            }
            lclAngle = - Nu_asin(sin_rad);
        }
        else
        {
            /* positive angle */
            sin_rad = Fix_div(-qDltYY,hypot);

            if( sin_rad >= 0x00010000 )
            {
                sin_rad = 0xFFFF;
            }

            lclAngle = Nu_asin(sin_rad);
        }

        if( DltX < 0 )
        {
            lclAngle = 1800 - lclAngle;
        }
        if( lclAngle < 0 )
        {
            lclAngle = 3600 + lclAngle ;
        }
    }
 
    /* Return to user mode */
    NU_USER_MODE();

    return(lclAngle);
}

/* floating-point math */
#else   
SIGNED PtToAngle( rect *R, point *PT)
{
    rect tempRect;
    point tempPt;
    INT32 DltX, DltY;
    INT32 Ctr_X , Ctr_Y;
    double hypot;
    SIGNED lclAngle;
    double sin_rad;
    double  ResX;
    double  ResY;
    double  qDltXX;
    double  qDltYY;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GR(*R, &tempRect, 1);
        U2GP(PT->X , PT->Y, &tempPt.X, &tempPt.Y, 1);
    }
    else
    {
        /* get rectangle */
        tempRect = * R; 

        /* get point */
        tempPt = *PT;   
    }

    /* find center point of rect */
    Ctr_X = (tempRect.Xmax + tempRect.Xmin) >> 1;
    Ctr_Y = (tempRect.Ymax + tempRect.Ymin) >> 1;

    /* compute delta from center to point */
    DltX = tempPt.X - Ctr_X;
    DltY = tempPt.Y - Ctr_Y;

    ResX = theGrafPort.portMap->pixResX;
    ResY = theGrafPort.portMap->pixResY;

    qDltXX = (double) (DltX / ResX);
    qDltXX = qDltXX * qDltXX;

    qDltYY = (double) (DltY / ResY);

    /* take hypotenuse of two X and Y coordinates */
    hypot = MATH_sqrt( (double)(qDltXX + (qDltYY*qDltYY)) );

    /* if angle of Zero there is no graph */
    if( hypot == 0 )
    {
        lclAngle = 0;
    }

    else
    {
        /* Test to find out angle's position */
        sin_rad = qDltYY / hypot;
        if( sin_rad >= 1.0 )
        {
            sin_rad = .99999999;
        }
        if( sin_rad <= -1.0 )
        {
            sin_rad = -.99999999;
        }

        lclAngle = (SIGNED) (-1800 * asin(sin_rad) / 3.1415926535);

        if( DltX < 0 )
        {
            lclAngle = 1800 - lclAngle;
        }
        if( lclAngle < 0 )
        {
            lclAngle = 3600 + lclAngle;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(lclAngle);
}

#endif
