/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       calibration.c
*
*   COMPONENT
*
*       Touchpanel driver
*
*   DESCRIPTION
*
*       This file contains the touch panel driver calibration function.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TP_Calibrate
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE))

#include    "ui/nu_ui.h"

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION */

#include    "services/nu_services.h"
#include    "drivers/nu_drivers.h"

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE))

/* Globals */
extern TP_SCREEN_DATA   TP_Screen_Data;

/* External functions. */

extern      INT32   EVENTH_StoreEvent(rsEvent *argEV);
extern      VOID    PD_ReadMouse(SIGNED *argX, SIGNED *argY, INT32* argButtons);
extern      VOID    ScreenRect(rect *SCRNRECT);
extern      VOID    SetRect(rect *R, INT32 X1, INT32 Y1, INT32 X2, INT32 Y2);
extern      VOID    MoveTo(INT32 argX, INT32 argY);
extern      VOID    LineTo(INT32 valX, INT32 valY);
extern      INT32   PtInRect(point *fpTESTPT, rect *argRect, INT32 sizX, INT32 sizY);
extern      INT32   EVENTH_KeyEvent(INT32 rsWAIT, rsEvent *rsEVNT);
extern      VOID    BackColor (INT32 argCOLOR);
extern      VOID    ShowCursor(VOID);

#endif /* CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION */

/*************************************************************************
*
*   FUNCTION
*
*       TP_Calibrate
*
*   DESCRIPTION
*
*       This function calibrates the touchpanel.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID TP_Calibrate(VOID)
{

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION == NU_TRUE))

    INT32       tchPnlCalPt = 0;
    point       tchPnlPt[5];
    rect        tpRect, scrnRect;
    INT16       boxSize;
    point       tpPoint;
    rsEvent     waitEvent;
    UINT8       done = 0;
    TextSetUp   newTextSetUp;
    PenSetUp    newPenSetUp;

    RS_Reset_Text();
    RS_Reset_Pen();

    ScreenRect(&scrnRect);
    SetRect(&tpRect, 0, 0, scrnRect.Xmax, scrnRect.Ymax);

    scrnRect.Xmax++;
    scrnRect.Ymax++;
    boxSize = scrnRect.Xmax >> 4;

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    /* Clear screen */
    HideCursor();
#endif

    RS_Get_Text_Setup(&newTextSetUp);
    newTextSetUp.face                = cNormal;
    newTextSetUp.charHorizontalAlign = alignLeft;
    newTextSetUp.charVerticalAlign   = alignTop;

    RS_Text_Setup(&newTextSetUp);

    RS_Get_Pen_Setup(&newPenSetUp);
    RS_Pen_Setup(&newPenSetUp, White);
    BackColor(Black);

    RS_Rectangle_Draw( ERASE, &tpRect, -1, 0, 0);

    newPenSetUp.penWidth = 2;
    newPenSetUp.penHeight = 2;
    RS_Pen_Setup(&newPenSetUp, White);

    MoveTo (10, boxSize);

    RS_Text_Draw((VOID *) "Touch panel calibration", STR, 0, 127);

    /* Draw touch box in top left corner. */
    SetRect(&tpRect, boxSize, (40 + boxSize), (boxSize << 1), (40 + (boxSize << 1)));
    RS_Rectangle_Draw( FRAME, &tpRect, -1, 0, 0);

    newPenSetUp.penWidth = 0;
    newPenSetUp.penHeight = 0;
    RS_Pen_Setup(&newPenSetUp, White);

    MoveTo(tpRect.Xmin, tpRect.Ymin);
    LineTo(tpRect.Xmax, tpRect.Ymax);
    MoveTo(tpRect.Xmax, tpRect.Ymin);
    LineTo(tpRect.Xmin, tpRect.Ymax);

    MoveTo(10, ((scrnRect.Ymax >> 1) + (boxSize << 1)));
    RS_Text_Draw((VOID *) "Touch the center of the box", STR, 0, 127);

    tchPnlCalPt = 0;
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
    ShowCursor();
#endif

    while (1)
    {
        while (!EVENTH_KeyEvent(1, &waitEvent));

        if (waitEvent.eventType != evntRELEASE)
        {
            continue;
        }

        ScreenRect(&scrnRect);
        scrnRect.Xmax++;
        scrnRect.Ymax++;
        boxSize = scrnRect.Xmax >> 4;

#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
        /* Clear screen */
        HideCursor();
#endif

        RS_Pen_Setup(&newPenSetUp, White);

        BackColor(Black);
        SetRect(&tpRect, 0, 40, scrnRect.Xmax, scrnRect.Ymax);
        RS_Rectangle_Draw( ERASE, &tpRect, -1, 0, 0);

        newPenSetUp.penWidth = 2;
        newPenSetUp.penHeight = 2;
        RS_Pen_Setup(&newPenSetUp, White);

        /* Save raw touch point */
        tchPnlPt[tchPnlCalPt].X = TP_Screen_Data.tp_x_raw;
        tchPnlPt[tchPnlCalPt].Y = TP_Screen_Data.tp_y_raw;

        /* Draw next box */
        switch(tchPnlCalPt)
        {
        case 0:
            tchPnlCalPt = 1;
            SetRect(&tpRect, (scrnRect.Xmax - (boxSize << 1)), (40 + boxSize),
                (scrnRect.Xmax - boxSize), (40 + (boxSize << 1)));
            break;

        case 1:
            tchPnlCalPt = 2;
            SetRect(&tpRect, (scrnRect.Xmax - (boxSize << 1)),
                (scrnRect.Ymax - (boxSize << 1)), (scrnRect.Xmax - boxSize),
                (scrnRect.Ymax - boxSize));
            break;

        case 2:
            tchPnlCalPt = 3;
            SetRect(&tpRect, boxSize, (scrnRect.Ymax - (boxSize << 1)),
                (boxSize << 1), (scrnRect.Ymax - boxSize));
            break;

        case 3:
            tchPnlCalPt = 4;

            /* Initialize scaling factors */
            TP_Screen_Data.tp_x_slope = ((scrnRect.Xmax - (3 * boxSize)) << 7) /
                ((tchPnlPt[1].X + tchPnlPt[2].X - tchPnlPt[0].X - tchPnlPt[3].X) >> 1);
            TP_Screen_Data.tp_x_intercept = (scrnRect.Xmax - boxSize - (boxSize >> 1)) -
                ((TP_Screen_Data.tp_x_slope * ((tchPnlPt[1].X + tchPnlPt[2].X) >> 1)) >> 7);
            TP_Screen_Data.tp_y_slope = ((scrnRect.Ymax - (3 * boxSize) - 40) << 7) /
                ((tchPnlPt[3].Y + tchPnlPt[2].Y - tchPnlPt[0].Y - tchPnlPt[1].Y) >> 1);
            TP_Screen_Data.tp_y_intercept = (scrnRect.Ymax - boxSize - (boxSize >> 1)) -
                ((TP_Screen_Data.tp_y_slope * ((tchPnlPt[2].Y + tchPnlPt[3].Y) >> 1)) >> 7);

            /* Draw center box */
            SetRect(&tpRect, ((scrnRect.Xmax >> 1) - (boxSize >> 1)),
                ((scrnRect.Ymax >> 1) - (boxSize >> 1)), ((scrnRect.Xmax >> 1) +
                (boxSize >> 1)), ((scrnRect.Ymax >> 1) + (boxSize >> 1)));
            break;

        case 4:
            SetRect(&tpRect, ((scrnRect.Xmax >> 1) - (boxSize >> 1)),
                ((scrnRect.Ymax >> 1) - (boxSize >> 1)), ((scrnRect.Xmax >> 1) +
                (boxSize >> 1)), ((scrnRect.Ymax >> 1) + (boxSize >> 1)));

            tpPoint.X = waitEvent.eventX;
            tpPoint.Y = waitEvent.eventY;

            if (PtInRect(&tpPoint, &tpRect, 2, 2))
            {
                /* Calibration OK */
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
                ShowCursor();
#endif
                newPenSetUp.penWidth = 0;
                newPenSetUp.penHeight = 0;
                RS_Pen_Setup(&newPenSetUp, White);

                done = 1;
            }
            else
            {
                /* Must recalibrate */
                MoveTo(10, (40 + (boxSize << 2)));
                RS_Text_Draw((VOID *)"Recalibration required", STR, 0, 127);

                tchPnlCalPt = 0;
                SetRect(&tpRect, boxSize, (40 + boxSize),
                    (boxSize << 1), (40 + (boxSize << 1)));
                break;
            }
        }

        if (!(done))
        {
            newPenSetUp.penWidth = 2;
            newPenSetUp.penHeight = 2;
            RS_Pen_Setup(&newPenSetUp, White);

            RS_Rectangle_Draw( FRAME, &tpRect, -1, 0, 0);

            newPenSetUp.penWidth = 0;
            newPenSetUp.penHeight = 0;
            RS_Pen_Setup(&newPenSetUp, White);

            MoveTo(tpRect.Xmin, tpRect.Ymin);
            LineTo(tpRect.Xmax, tpRect.Ymax);
            MoveTo(tpRect.Xmax, tpRect.Ymin);
            LineTo(tpRect.Xmin, tpRect.Ymax);

            MoveTo(10, ((scrnRect.Ymax >> 1) + (boxSize << 1)));
            RS_Text_Draw((VOID *)"Touch the center of the box", STR, 0, 127);
#if         (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)
            ShowCursor();
#endif
        }
        else
        {
            break; /* while(1) */
        }
    }

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_CALIBRATION */

}
