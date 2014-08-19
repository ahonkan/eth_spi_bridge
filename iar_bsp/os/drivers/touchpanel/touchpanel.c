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
*       touchpanel.c
*
*   COMPONENT
*
*       Touchpanel driver
*
*   DESCRIPTION
*
*       This file contains the touch panel driver generic functions.
*       In most cases, these functions will not have to be modified.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Touchpanel_Get_Target_Info
*       Touchpanel_Init
*       TP_Device_Mgr
*       TP_Open
*       TP_Pos
*       TP_Encode
*       TP_Calibrate
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_services.h
*       nu_drivers.h
*       stdio.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "ui/nu_ui.h"
#include    "services/nu_services.h"
#include    "drivers/nu_drivers.h"
#include    <stdio.h>

/* Globals */

mouseRcd       *TP_Mouse_Record;
TP_SCREEN_DATA  TP_Screen_Data;

/* Touchpanel controller session handle. */
static DV_DEV_HANDLE  Touchpanel_Ctrl_Session = NU_NULL;

/* Touchpanel callback structure. */
static TOUCHPANEL_CALLBACKS  Touchpanel_Callback;

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       key                                 - Registry path
*       inst_info                           - pointer to touchpanel instance info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - TOUCHPANEL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Touchpanel_Get_Target_Info(const CHAR * key, TOUCHPANEL_INSTANCE_HANDLE *inst_info)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status;

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/x_intercept", (UINT32 *)(&inst_info->x_intercept));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/y_intercept", (UINT32 *)(&inst_info->y_intercept));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/x_slope", (UINT32 *)(&inst_info->x_slope));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/y_slope", (UINT32 *)(&inst_info->y_slope));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/irq_vector_id", (UINT32 *)(&inst_info->irq_vector_id));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/irq_bitmask", (UINT32 *)(&inst_info->irq_bitmask));
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/irq_data", (UINT32 *)(&inst_info->irq_data));
    }
    
    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }

    else
    {
        status = TOUCHPANEL_REGISTRY_ERROR;
    }

    return status;
}

/************************************************************************
* FUNCTION
*
*       Touchpanel_Init
*
* DESCRIPTION
*
*       This function prepares the Touchpanel control block structure by
*       extracting the content from the specific info structure and
*       calls the device specific setup function if it exists.
*
* INPUTS
*
*       TOUCHPANEL_INSTANCE_HANDLE    *inst_handle          - Instance handle
*       CHAR                                     *key
*
* OUTPUTS
*
*       STATUS        status               - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Touchpanel_Init(TOUCHPANEL_INSTANCE_HANDLE *inst_handle, const CHAR *key)
{
    STATUS         status = NU_SUCCESS;
    CHAR           reg_path[REG_MAX_KEY_LENGTH];
    STATUS         (*setup_fn)(VOID);

    /******************************/
    /* CALL DEVICE SETUP FUNCTION */
    /******************************/

    /* Setup touchpanel */
    /* If there is a setup function, call it */
    strcpy(reg_path, key);
    strcat(reg_path, "/setup");

    if (REG_Has_Key(reg_path))
    {
        status = REG_Get_UINT32_Value(key, "/setup", (UINT32 *)&setup_fn);

        if (status == NU_SUCCESS)
        {
            setup_fn();
        }
        else
        {
            status = NU_NOT_REGISTERED;
        }
    }
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       TP_Device_Mgr
*
*   DESCRIPTION
*
*       This function is touchpanel's device driver.
*
*   INPUTS
*
*       rcd                                 Touchpanel record
*       md                                  Command: IMOPEN, IMCLOSE, IMPOSN
*
*   OUTPUTS
*
*       status                              Return value of the open,
*                                           close, and position functions
*
**************************************************************************/
INT32 TP_Device_Mgr(VOID *rcd, INT32 md)
{
    INT32   ret_val = 0;

    if (md == IMOPEN)
    {
        /* Call the touch panel initialization function. */
        ret_val = TP_Open(rcd);
    }
    else if (md == IMCLOSE)
    {
        /* Call the touch panel close function. */
        TP_Close();
    }
    else
    {
        /* Call the function to get the touch position. */
        ret_val = TP_Pos(rcd);
    }

    /* Return the status. */
    return (ret_val);
}
/*************************************************************************
*
*   FUNCTION
*
*       TP_Open
*
*   DESCRIPTION
*
*       This function is touchpanel's open routine that  initializes
*       touchpanel controller.
*
*   INPUTS
*
*       rcd                                 Touchpanel record
*
*   OUTPUTS
*
*       NU_SUCCESS                          Device opened successfully
*       [error code]                        Error code, otherwise
*
**************************************************************************/
INT32 TP_Open(VOID *rcd)
{
    STATUS         status;
    DV_DEV_LABEL   labels[] = {{TOUCHPANEL_LABEL}};
    INT            label_cnt = 1;
    DV_DEV_ID      touchpanel_dev_id_list[CFG_NU_OS_DRVR_TOUCHPANEL_MAX_DEVS_SUPPORTED];
    INT            dev_id_cnt = CFG_NU_OS_DRVR_TOUCHPANEL_MAX_DEVS_SUPPORTED;

    /* Set the driver's input record pointer. */
    TP_Mouse_Record = (mouseRcd*)rcd;

    /* Get list of all labels for touchpanel. */
    status = DVC_Dev_ID_Get (labels, label_cnt, touchpanel_dev_id_list, &dev_id_cnt);

    if ((status == NU_SUCCESS) && (dev_id_cnt > 0))
    {
        /* Hardware initialization. Open Touchpanel controller. */
        status = DVC_Dev_ID_Open(touchpanel_dev_id_list[0], labels, 1, &Touchpanel_Ctrl_Session);
    }

    if (status == NU_SUCCESS)
    {
        /* Read the touchpanel data. */
        TP_Pos(rcd);

        /* Initialize the event state to released. */
        TP_Screen_Data.tp_press_state = mREL;
    }

    /* Return the completion status of the service. */
    return (INT32)(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       TP_Close
*
*   DESCRIPTION
*
*       This function is touchpanel's close routine.
*
*   INPUTS
*
*       rcd                                 Toucpanel record
*
*   OUTPUTS
*
*      VOID
*
**************************************************************************/
VOID TP_Close(VOID)
{
    /* Call device close function if a session exists. */
    if (Touchpanel_Ctrl_Session)
    {
        (VOID)DVC_Dev_Close(Touchpanel_Ctrl_Session);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       TP_Pos
*
*   DESCRIPTION
*
*       This function updates the position of screen coordinates.
*
*   INPUTS
*
*       rcd                                 Touchpanel record
*
*   OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully
*
*************************************************************************/
INT32 TP_Pos(VOID *rcd)
{
    INT16  xpos, ypos;
    mouseRcd *tp_rcd = (mouseRcd*)rcd;

    /* Limit the coordinates. */

    xpos = tp_rcd->mrEvent.eventX;
    ypos = tp_rcd->mrEvent.eventY;

    /* Is the x coordinate < minimum? */
    if (xpos < tp_rcd->mrLimits.Xmin)
    {
        /* It is, so set the x coordinate = minimum. */
        xpos = tp_rcd->mrLimits.Xmin;
    }

    /* Is the x coordinate > max x size of the screen? */
    if (xpos > tp_rcd->mrLimits.Xmax)
    {
        /* It is, so set the x coordinate = max x size of the screen. */
        xpos = tp_rcd->mrLimits.Xmax;
    }

    /* Is the y coordinate < minimum? */
    if (ypos < tp_rcd->mrLimits.Ymin)
    {
        /* It is, so set the y coordinate = minimum. */
        ypos = tp_rcd->mrLimits.Ymin;
    }

    /* Is the y coordinate > max y size of the screen? */
    if (ypos > tp_rcd->mrLimits.Ymax)
    {
        /* It is, so set the y coordinate = max y size of the screen. */
        ypos = tp_rcd->mrLimits.Ymax;
    }

    /* X coordinate exhibits nonlinear intercept on this touchpanel
       so we need to do following adjustment. */
    if (Touchpanel_Callback.tp_y_coordinate_adjustment)
    {
        Touchpanel_Callback.tp_x_coordinate_adjustment(&xpos);
    }

    /* Y coordinate exhibits nonlinear intercept on this touchpanel
       so we need to do following adjustment. */
    if (Touchpanel_Callback.tp_y_coordinate_adjustment)
    {
        Touchpanel_Callback.tp_y_coordinate_adjustment(&ypos);
    }

    /* Scale back to device coordinates. */

    if (TP_Screen_Data.tp_x_slope != 0)
    {
        TP_Screen_Data.tp_x_raw = ((xpos - TP_Screen_Data.tp_x_intercept) << 7) /
                                    TP_Screen_Data.tp_x_slope;
    }

    if (TP_Screen_Data.tp_y_slope != 0)
    {
        TP_Screen_Data.tp_y_raw = ((ypos - TP_Screen_Data.tp_y_intercept) << 7) /
                                    TP_Screen_Data.tp_y_slope;
    }

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       TP_Encode
*
*   DESCRIPTION
*
*       This function takes raw touchpanel data and converts it to
*       Grafix mouse data and event record data.
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
VOID TP_Encode(VOID)
{
    INT16   xpos, ypos;

    /* Our internal mice drivers have the extra added feature of returning
       the current keyboard state word. */
    TP_Mouse_Record->mrEvent.eventState = defKBrd.mrEvent.eventState;

    /* Scale the x, y. */

    xpos = ((TP_Screen_Data.tp_x_raw * TP_Screen_Data.tp_x_slope) >> 7) +
            TP_Screen_Data.tp_x_intercept;
    ypos = ((TP_Screen_Data.tp_y_raw * TP_Screen_Data.tp_y_slope) >> 7) +
            TP_Screen_Data.tp_y_intercept;

    /* X coordinate exhibits nonlinear intercept on this touchpanel
       so we need to do following adjustment. */
    if (Touchpanel_Callback.tp_x_coordinate_adjustment)
    {
        Touchpanel_Callback.tp_x_coordinate_adjustment(&xpos);
    }

    /* Y coordinate exhibits nonlinear intercept on this touchpanel
       so we need to do following adjustment. */
    if (Touchpanel_Callback.tp_y_coordinate_adjustment)
    {
        Touchpanel_Callback.tp_y_coordinate_adjustment(&ypos);
    }

    /* Limit the x, y. */
    if (xpos < TP_Mouse_Record->mrLimits.Xmin)
    {
        xpos = TP_Mouse_Record->mrLimits.Xmin;
    }
    else if (xpos > TP_Mouse_Record->mrLimits.Xmax)
    {
        xpos = TP_Mouse_Record->mrLimits.Xmax;
    }

    if (ypos < TP_Mouse_Record->mrLimits.Ymin)
    {
        ypos = TP_Mouse_Record->mrLimits.Ymin;
    }
    else if (ypos > TP_Mouse_Record->mrLimits.Ymax)
    {
        ypos = TP_Mouse_Record->mrLimits.Ymax;
    }

    /* Have we moved? */
    if (!TP_Mouse_Record->mrEvent.eventType &&
       ((TP_Mouse_Record->mrEvent.eventX != xpos) ||
        (TP_Mouse_Record->mrEvent.eventY != ypos)))
    {
        TP_Mouse_Record->mrEvent.eventType = mPOS;
    }

    /* Check if there is an event present. */
    if (TP_Mouse_Record->mrEvent.eventType)
    {
        /* Save coordinates in the event area. */

        TP_Mouse_Record->mrEvent.eventX = xpos;
        TP_Mouse_Record->mrEvent.eventY = ypos;

        /* Call the call back routine. */
        curInput->mrCallBack(TP_Mouse_Record);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       TP_Register_Callbacks
*
*   DESCRIPTION
*
*       This function registers callback functions which are implemented 
*       by the user.
*
*   INPUTS
*
*       tp_cb                               Pointer to Touchpanel callback
*                                           structure
*
*   OUTPUTS
*
*       None
*
**************************************************************************/
VOID TP_Register_Callbacks(TOUCHPANEL_CALLBACKS *tp_cb)
{
    /* Copy callback function pointers to global variable. */
    Touchpanel_Callback.tp_x_coordinate_adjustment = tp_cb->tp_x_coordinate_adjustment;
    Touchpanel_Callback.tp_y_coordinate_adjustment = tp_cb->tp_y_coordinate_adjustment;
}

