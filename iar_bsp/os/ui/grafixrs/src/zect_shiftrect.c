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
*  zect_shiftrect.c                                                       
*
* DESCRIPTION
*
*  This file contains rectangle operation function - ShiftRect.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ShiftRect
*
* DEPENDENCIES
*
*  rs_base.h
*  zect.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/zect.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    ShiftRect
*
* DESCRIPTION
*
*    Function ShiftRect moves the specified rectangle by adding the delta-X (DX) 
*    value to the rectangle 'R' X coordinates, and delta-Y (DY) values to the 
*    rectangle 'R' Y coordinates.  The rectangle retains its shade and size, it is 
*    simply offset to a different position on the coordinate plane.
*
*    The area(s) voided by the rectangle move are returned are returned in 
*    rectangles Rect1 and Rect2.  The function return value, 0,1 or 2,  indicates if 
*    no (DX = DY = 0), one (Rect1) or two (Rect1 & Rect2) voided rectangles are returned.
*
*    Note: this functions assumes a proper rectangle where Xmin, Ymin are less
*    than Xmax, Ymax, respectively.
*                                 _________________
*                                |Dst              |
*        ...................     |                 |
*        :Src              : ==> |                 |
*        :                 :     |                 |
*        :    Rect1        :     |_________________|
*        :                 :
*        :.................:
*
*      
*         DX<0                      DX=0                       DX>0
*   _________________         __________________         _________________
*  |Dst              |       |Dst               |       |Dst              |
*  |    ...          |.....  | . . . . . . . .  |  .....|          ...    |
*  |    :            | Src:  |                  |  :Src |            :    |  DY>0
*  |              | Rect2 :  |                  |  : Rect2 |              |
*  |_________________|....:  |__________________|  :....|_________________|
*       :    Rect1        :  :        Rect1  Src:  :       Rect1     :
*       :.................:  :..................:  :.................:
*
*   _________________ .....   __________________   ..... _________________
*  |Dst .            | Src:  |Src/Dst           |  :Src |Dst         .    |
*  |    :            |    :  |                  |  :    |            :    |
*  |    :         | Rect1 :  |                  |  : Rect1 |         :    |  DY=0
*  |    :            |    :  |                  |  :    |            :    |
*  |_________________|....:  |__________________|  :....|_________________|
*
*       ...................  ....................  ...................
*       :Src  Rect1       :  :Src Rect1         :  :Src  Rect1       :
*   _________________ ....:  :__________________:  :.... _________________
*  |Dst              |    :  |Dst               |  :    |Dst              |  DY<0
*  |    .         | Rect2 :  |                  |  : Rect2 |         .    |
*  |    :..          |....:  |..................|  :....|          ..:    |
*  |                 |       |                  |       |                 |
*  |_________________|       |__________________|       |_________________|
*
*
* INPUTS
*
*    rect *R     - Pointer to the rectangle.
*
*    INT32 dltX  - Delta x.
*
*    INT32 dltY  - Delta y.
*
*    rect *Rect1 - Pointer to the second rectangle.
*
*    rect *Rect2 - Pointer to the second rectangle.
*
* OUTPUTS
*
*    INT32       - Return value, 0,1 or 2 (see above).
*
***************************************************************************/
INT32 ShiftRect( rect *R, INT32 dltX, INT32 dltY, rect *Rect1, rect *Rect2)
{
    INT32 value;

    INT32 width;
    INT32 height;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Load passing parameters for calculation purpose */
    *Rect1    = *R;
    width  = R->Xmax - R->Xmin;
    height = R->Ymax - R->Ymin;

    /* move the rectangle */
    R->Xmin = R->Xmin + dltX;
    R->Xmax = R->Xmax + dltX;
    R->Ymin = R->Ymin + dltY;
    R->Ymax = R->Ymax + dltY;

    while(1)
    {
        if( dltX == 0)
        {
            /* 0 or 1 rectangle returned; Rect1->X values = R->X values */
            if( dltY == 0)
            {
                /* nothing moved */
                value = 0;

                /* while(1) */
                break; 
            }

            if( dltY < 0)
            {
                /* moving down */
                if( -dltY >= height )
                {
                    /* moved out of source */
                    value = 1; 

                    /* while(1) */
                    break; 
                }

                /* adjust Ymin */
                Rect1->Ymin = R->Ymax; 

                value = 1;

                /* while(1) */
                break; 
            }
            else
            {
                /* moving up */
                if( dltY >= height)
                {
                    /* moved out of source */
                    value = 1;

                    /* while(1) */
                    break; 
                }

                /* adjust Ymax */
                Rect1->Ymax = R->Ymin; 

                value = 1;

                /* while(1) */
                break; 
            }
        }

        /* Case of negative delta X with all value of delta Y */
        if( dltX < 0 )                             
        {
            if( -dltX >= width )
            {
                /* moved out of source */
                value = 1;

                /* while(1) */
                break; 
            }

            Rect2->Xmax = Rect1->Xmax; 
            Rect2->Xmin = R->Xmax;

            if( dltY == 0 )
            {
                /* moved left */
                Rect1->Xmin = Rect2->Xmin;
                value =1;

                /* while(1) */
                break; 
            }

            if( dltY < 0 )
            {
                /* moving down */
                if( -dltY >= height )
                {
                    /* moved out of source */
                    value = 1;

                    /* while(1) */
                    break; 
                }
                Rect2->Ymin = Rect1->Ymin;
                Rect2->Ymax = R->Ymax; 
                Rect1->Ymin = R->Ymax;
                value = 2;

                /* while(1) */
                break; 
            }
            else
            {
                /* moving up */
                if( dltY >= height )
                {
                    /* moved out of source */
                    value = 1;

                    /* while(1) */
                    break; 
                }
                Rect2->Ymax = Rect1->Ymax; 
                Rect2->Ymin = R->Ymin;
                Rect1->Ymax = R->Ymin;
                value = 2;

                /* while(1) */
                break; 
            }
        }

        /* Case of positive delta X with all value of delta Y  */
        if( dltX >= width )
        {
            /* moved out of source */
            value = 1;

            /* while(1) */
            break; 
        }

        Rect2->Xmin = Rect1->Xmin; 
        Rect2->Xmax = R->Xmin;

        if( dltY == 0 )
        {
            /* moved right */
            Rect1->Xmax = Rect2->Xmax;
            value = 1;

            /* while(1) */
            break; 
        }

        if( dltY < 0 )
        {
            /* moving down */
            if( -dltY >= height )
            {
                /* moved out of source */
                value = 1;

                /* while(1) */
                break; 
            }
            Rect2->Ymin = Rect1->Ymin;
            Rect2->Ymax = R->Ymax; 
            Rect1->Ymin = R->Ymax;
            value = 2;

            /* while(1) */
            break;
        }
        else
        {
            /* moving up */
            if( dltY >= height )
            {
                /* moved out of source */
                value = 1;

                /* while(1) */
                break; 
            }
            Rect2->Ymax = Rect1->Ymax; 
            Rect2->Ymin = R->Ymin;
            Rect1->Ymax = R->Ymin;
            value = 2;

            /* while(1) */
            break; 
        }

    } /* while(1) */

    /* Return to user mode */
    NU_USER_MODE();

    return( value);
}
