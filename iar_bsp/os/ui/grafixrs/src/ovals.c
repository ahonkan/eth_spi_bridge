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
*  ovals.c                                                      
*
* DESCRIPTION
*
*  This file contains the OvalPt function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  OvalPt
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  ovals.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/ovals.h"

/***************************************************************************
* FUNCTION
*
*    OvalPt
*
* DESCRIPTION
*
*    Function OvalPt calculates the point gblPt of the elliptical oval inscribed
*    in rectangle argR which intersects a vector at angle degrees.  The center of
*    the rectangle is the center of the oval and the base of the angular measurement.
*    Angles are measured in integer increments of 0.1 degrees from the 3 o'clock
*    position (0 degrees), with positive angles proceeding counter-clockwise,
*    negative angles clockwise.
*
* INPUTS
*
*    rect *argR   - Pointer to arc rectangle.
*
*    INT32 rsangle  - Angle.
*
*    point *gblPt - Pointer to calculated point gblPt.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
#ifdef FIXPOINT
VOID OvalPt(rect *argR, INT32 rsangle, point *gblPt)
{
    /* radY^2 */
    dblFix qA;          
    
    /* radX^2 */
    dblFix qB;          
    
    /* work variable */
    dblFix qTmp1;  

    /* A + (B*M*M) */
    dblFix qA_BMM;      
    
    /* A / (A + B*M*M) */
    dblFix qAdA_BMM;    
    dblFix dblOne = {0, 0x00010000};
    
    /* gblCenterR */
    rect   ctrR;       
    
    /* gblDx, gblDy */
    INT32  dltX;        
    INT32  dltY;
    
    /* pointer for "switch" function */
    INT16  quadrant;   
    
    /* x,y radii */
    INT32  radX;        
    INT32  radY;
    INT32  tmpRad;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get rectangle */
    ctrR = *argR;

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GR(ctrR, &ctrR, 1);
    }

    radX = ((ctrR.Xmax - ctrR.Xmin) >> 1);
    radY = ((ctrR.Ymax - ctrR.Ymin) >> 1);
    InsetRect(&ctrR, radX, radY);

    /* insure ( 0 <= rsangle < 3600 ) */
    rsangle = ChkAngle(rsangle);    

    /* Dispatch according to the angle */
    /* one of four 90 degree quadrants */
    quadrant = rsangle / 900; 

    /* angle is between 0 and 90 degrees */
    rsangle = rsangle % 900;    

    /* check four trivial cases */
    if( rsangle == 0 )
    {
        switch (quadrant)
        {
        case 0: /* 0 */
            gblPt->X = ctrR.Xmax + radX;
            gblPt->Y = ctrR.Ymin;
            break;

        case 1: /* 90 */
            gblPt->X = ctrR.Xmin;
            gblPt->Y = ctrR.Ymin - radY;
            break;

        case 2: /* 180 */
            gblPt->X = ctrR.Xmin - radX;
            gblPt->Y = ctrR.Ymax;
            break;

        default:    /* 270 */
            gblPt->X = ctrR.Xmax;
            gblPt->Y = ctrR.Ymax + radY;
            break;
        }
    }
    else
    {
        if( (quadrant == 1) || (quadrant == 3) )
        {
            /* swap radX & radY */
            tmpRad = radX;
            radX = radY;
            radY = tmpRad;
        }

        /* Ready to compute inter-quadrant point */
        qA.fUpper = 0;
        qA.fLower = (radY << 16);
        dFix_mul(&qA, &qA, &qA);
        qB.fUpper = 0;
        qB.fLower = (radX << 16);
        dFix_mul(&qB, &qB, &qB);
        qTmp1.fLower = Fix_div(iSin(rsangle), iCos(rsangle));
        qTmp1.fLower = Fix_mul(qTmp1.fLower, qTmp1.fLower);
        qTmp1.fUpper = 0;

        dFix_mul(&qB, &qTmp1, &qA_BMM);
        dFix_add(&qA, &qA_BMM, &qA_BMM);
        dFix_div(&qA, &qA_BMM, &qAdA_BMM);

        dFix_mul(&qB, &qAdA_BMM, &qB);
        dltX = ((Fix_sqrt(qB.fLower) + 0x00008000) >> 16);
        dFix_sub(&dblOne, &qAdA_BMM, &qAdA_BMM);
        dFix_mul(&qA, &qAdA_BMM, &qA);
        dltY = ((Fix_sqrt(qA.fLower) + 0x00008000) >> 16);

        /* Ready to calculate point */
        switch (quadrant)
        {
        case 0: /* Quadrant 0 */
            gblPt->X = ctrR.Xmax + dltX;
            gblPt->Y = ctrR.Ymin - dltY;
            break;

        case 1:  /* Quadrant 1 */
            gblPt->X = ctrR.Xmin - dltY;
            gblPt->Y = ctrR.Ymin - dltX;
            break;

        case 2:  /* Quadrant 2 */
            gblPt->X = ctrR.Xmin - dltX;
            gblPt->Y = ctrR.Ymax + dltY;
            break;

        default:  /* Quadrant 3 */
            gblPt->X = ctrR.Xmax + dltY;
            gblPt->Y = ctrR.Ymax + dltX;
        }
    }

    if( globalLevel > 0 )
    {
        /* convert from global to user */
        G2UP(gblPt->X, gblPt->Y, &gblPt->X, &gblPt->Y);
    }

    /* Return to user mode */
    NU_USER_MODE();

}
#else
VOID OvalPt(rect *argR, INT32 rsangle, point *gblPt)
{ 
    /* radY^2 */
    double qA;         
    
    /* radX^2 */
    double qB;          
    
    /* work variable */
    double qTmp1;       
    double qTmp2;
    double qTmp3;
    
    /* A + (B*M*M) */
    double qA_BMM;      
    
    /* A / (A + B*M*M) */
    double qAdA_BMM;    
    
    /* gblCenterR */
    rect   ctrR;        
    
    /* gblDx, gblDy */
    INT32  dltX;        
    INT32  dltY;
    
    /* pointer for "switch" function */
    INT16  quadrant;    
    
    /* x,y radii */
    INT32  radX;        
    INT32  radY;
    INT16  tmpRad;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get rectangle */
    ctrR = *argR;

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GR(ctrR, &ctrR, 1);
    }

    radX = ((ctrR.Xmax - ctrR.Xmin) >> 1);
    radY = ((ctrR.Ymax - ctrR.Ymin) >> 1);
    InsetRect(&ctrR, radX, radY);

    /* insure ( 0 <= rsangle < 3600 ) */
    rsangle = ChkAngle(rsangle);    

    /* Dispatch according to the angle */
    /* one of four 90 degree quadrants */
    quadrant = rsangle / 900; 

    /* angle is between 0 and 90 degrees */
    rsangle = rsangle % 900;    

    /* check four trivial cases */
    if( rsangle == 0 )
    {
        switch (quadrant)
        {
        case 0: /* 0 */
            gblPt->X = ctrR.Xmax + radX;
            gblPt->Y = ctrR.Ymin;
            break;

        case 1: /* 90 */
            gblPt->X = ctrR.Xmin;
            gblPt->Y = ctrR.Ymin - radY;
            break;

        case 2: /* 180 */
            gblPt->X = ctrR.Xmin - radX;
            gblPt->Y = ctrR.Ymax;
            break;

        default:    /* 270 */
            gblPt->X = ctrR.Xmax;
            gblPt->Y = ctrR.Ymax + radY;
            break;
        }
    }
    else
    {
        if( (quadrant == 1) || (quadrant == 3) )
        {
            /* swap radX & radY */
            tmpRad = radX;
            radX = radY;
            radY = tmpRad;
        }

        /* Ready to compute inter-quadrant point */
        qA = radY * radY;
        qB = radX * radX;
        qTmp1 = rsangle;
        qTmp1 = 3.1415926535 * qTmp1 / 1800.0;
        qTmp2 = MATH_sin(qTmp1,0,0);
        qTmp3 = MATH_cos(qTmp1,0);        
        qTmp1 = qTmp2/qTmp3;
        qTmp1 *= qTmp1;
        qA_BMM = qA + qB * qTmp1;
        qAdA_BMM = qA /  qA_BMM;
        qB *= qAdA_BMM;
		dltX = (INT32)(MATH_sqrt(qB) + .5);
        qAdA_BMM = 1 - qAdA_BMM;
        qA *= qAdA_BMM;
        dltY = (INT32)(MATH_sqrt(qA) + .5);

        /* Ready to calculate point */
        switch (quadrant)
        {
        case 0: /* Quadrant 0 */
            gblPt->X = ctrR.Xmax + dltX;
            gblPt->Y = ctrR.Ymin - dltY;
            break;

        case 1:  /* Quadrant 1 */
            gblPt->X = ctrR.Xmin - dltY;
            gblPt->Y = ctrR.Ymin - dltX;
            break;

        case 2:  /* Quadrant 2 */
            gblPt->X = ctrR.Xmin - dltX;
            gblPt->Y = ctrR.Ymax + dltY;
            break;

        default:  /* Quadrant 3 */
            gblPt->X = ctrR.Xmax + dltY;
            gblPt->Y = ctrR.Ymax + dltX;
        }
    }

    if( globalLevel > 0 )
    {
        /* convert from global to user */
        G2UP(gblPt->X, gblPt->Y, &gblPt->X, &gblPt->Y);
    }

    /* Return to user mode */
    NU_USER_MODE();

}

#endif
