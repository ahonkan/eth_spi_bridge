/*************************************************************************
*
*            Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
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
*       display_tgt.c
*
*   COMPONENT
*
*       LTDC - ST LTDC controller driver
*
*   DESCRIPTION
*
*       This file contains the target specific routines for ST LTDC
*       controller driver.
*
*   DATA STRUCTURES
*
*       Display_Tgt_Brightness_Lookup_Table
*       Display_Tgt_Brightness_Current_Level
*
*   FUNCTIONS
*
*       Display_Tgt_Set_State
*       Display_Tgt_Initialize
*       Display_Tgt_Read_Palette
*       Display_Tgt_Write_Palette
*       Display_Tgt_Shutdown
*       Display_Tgt_Pre_Process_Hook
*       Display_Tgt_Post_Process_Hook_Spec
*       Display_Tgt_Backlight_Set_ON
*       Display_Tgt_Backlight_Set_OFF
*       Display_Tgt_Backlight_Status
*       Display_Tgt_Contrast_Set
*       Display_Tgt_Contrast_Get
*       Display_Tgt_Allocate_Framebuffer
*       Display_Tgt_Set_State_ON
*       Display_Tgt_Set_State_OFF
*       Display_Tgt_Write_Register
*       Display_Tgt_Read_Register
*       Display_Tgt_Draw_Rect
*       Display_Tgt_SetCursor
*       Display_Tgt_SetBacklight
*       Display_Tgt_LCD_Configuration
*       Display_Tgt_LCD_Write_Command
*       Display_Tgt_LCD_Write_Data
*       Display_Tgt_LCD_Enable_Power
*       Display_Tgt_LCD_Display_ON
*       Display_Tgt_LCD_Display_OFF
*       Display_Tgt_LCD_SPI_Configuration
*       Display_Tgt_LTDC_Initialization
*       Display_Tgt_LTDC_Enable
*       Display_Tgt_LTDC_Dither_Enable
*       Display_Tgt_LTDC_Layer_Initialization
*       Display_Tgt_LTDC_Enable_Layer
*       Display_Tgt_SPI_I2S_Send_Data
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel
*       nu_services.h
*       nu_drivers.h
*       display_tgt.h
*       nu_ui.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/nu_ui.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/display/ltdc/display_tgt.h"

NU_SPI_HANDLE   Demo_SPI_Handle;

/* LCD target info structure. */
static DISPLAY_INSTANCE_HANDLE *global_instance_ptr = 0;
BOOLEAN backlight = DISABLE; 

extern VOID LCD_Chip_Select(FunctionalState NewState);
extern VOID LCD_Write_Control_Lines(UINT32 GPIOx, UINT16 CtrlPins, BitAction BitVal);
extern VOID chipselect_function(BOOLEAN assert);

/*************************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_display_ltdc_init
*
*   DESCRIPTION
*
*       Initialization function of LCD driver. It handles registration
*       and unregistration of LCD devices.
*
*   INPUTS
*
*       key                                 Path to registry
*       startstop                           Option to register or unregister
*
*   OUTPUTS
*
*       STATUS                              NU_SUCCESS
*                                           DISPLAY_NOT_REGISTERED
*
*************************************************************************/
STATUS nu_bsp_drvr_display_ltdc_init(const CHAR * key, INT startstop)
{
    static DV_DEV_ID        dev_id = -1;
    STATUS                  status = DISPLAY_NOT_REGISTERED;
    DISPLAY_INSTANCE_HANDLE *inst_handle;
    NU_MEMORY_POOL          *sys_pool_ptr;
    VOID                    (*setup_fn)(VOID) = NU_NULL;
    VOID                    (*cleanup_fn)(VOID) = NU_NULL;
    STATUS                  reg_status = NU_SUCCESS;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    LTDC_TGT                *display_tgt;


    if (key != NU_NULL)
    {
        /* If we are starting the device */
        if (startstop == 1)
        {
            /* Create an instance for the device */      
            /* Get system memory pool */
            status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
            
            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the device instance */
                status = NU_Allocate_Memory(sys_pool_ptr, (VOID**)&inst_handle,
                                            sizeof(DISPLAY_INSTANCE_HANDLE), NU_NO_SUSPEND);

                /******************************/
                /* SAVE DEFAULT CFG/TGT INFO  */
                /******************************/
                if (status == NU_SUCCESS)
                {
                    /* Zero out allocated space */
                    memset(inst_handle, 0x00, sizeof(DISPLAY_INSTANCE_HANDLE));
                
                    /******************************/
                    /* SAVE DEFAULT CFG/TGT INFO  */
                    /******************************/

                    /* Allocate memory for the device instance */
                    status = NU_Allocate_Memory(sys_pool_ptr, (VOID**)&(inst_handle->display_tgt_ptr),
                                                sizeof(LTDC_TGT), NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Clear out the allocated memory. */
                        memset(inst_handle->display_tgt_ptr, 0x00, sizeof(LTDC_TGT));

                        status = Display_Tgt_Get_Target_Info(key, (inst_handle->display_tgt_ptr));

                        display_tgt = (LTDC_TGT *)(inst_handle->display_tgt_ptr);

                        if (status == NU_SUCCESS)
                        {
                            /******************************/
                            /* CALL DEVICE SETUP FUNCTION */
                            /******************************/

                            /* If there is a setup function, call it. */
                            strcpy(reg_path, key);
                            strcat(reg_path, "/setup");

                            if (REG_Has_Key(reg_path))
                            {
                                reg_status = REG_Get_UINT32(reg_path, (UINT32 *)&setup_fn);

                                if (reg_status == NU_SUCCESS && setup_fn != NU_NULL)
                                {
                                    display_tgt->setup_func = setup_fn;
                                }
                            }

                            /* If there is a cleanup function, initialize it and call it. */
                            strcpy(reg_path, key);
                            strcat(reg_path, "/cleanup");

                            if (REG_Has_Key(reg_path))
                            {
                                reg_status = REG_Get_UINT32(reg_path, (UINT32 *)&cleanup_fn);
                                if ((reg_status == NU_SUCCESS) && (cleanup_fn))
                                {
                                    display_tgt->cleanup_func = cleanup_fn;
                                    cleanup_fn();
                                }
                            }

                            /************************/
                            /* CONFIGURE THE DEVICE */
                            /************************/
                            strcpy(inst_handle->mw_config_path, key);
                            strcat(inst_handle->mw_config_path, "/mw_settings");
    
                            /* Call the register function */
                            (VOID)Display_Dv_Register (key, inst_handle);
                        }
                    }
                }

            }
            if (status != NU_SUCCESS)
            {
                (VOID)NU_Deallocate_Memory(inst_handle);
            }
        }
        else
        {
            /* If we are stopping an already started device */
            if (dev_id >= 0)
            {
                /* Call the component unregistration function */
                status = Display_Dv_Unregister(key, startstop, dev_id);

                /* Check if unregistration was successful. */
                if (status == NU_SUCCESS)
                {
                    dev_id = -1;
                }
            }
        }
    }

    /* Return completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       CHAR      *key                      - Registry path
*       LTDC_TGT  *tgt_info                 - pointer to target info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - DISPLAY_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Display_Tgt_Get_Target_Info(const CHAR *key, LTDC_TGT *tgt_info)
{
    STATUS     reg_status;
    STATUS     status = NU_SUCCESS;

    reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/base", &tgt_info->lcdc_io_addr);

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_String_Value(key, "/tgt_settings/ref_clock", &(tgt_info->lcdc_ref_clock[0]), NU_DRVR_REF_CLOCK_LEN);
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/mw_settings/screen_width", &tgt_info->lcdc_screen_width);
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/mw_settings/screen_height", &tgt_info->lcdc_screen_height);
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/mw_settings/bits_per_pix", &tgt_info->lcdc_bits_per_pixel);
    }
    
    if (reg_status != NU_SUCCESS)
    {
        status = DISPLAY_REGISTRY_ERROR;
    }
    
    return status;
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Initialize
*
* DESCRIPTION
*
*       This function initializes the display controller.
*
* INPUTS
*
*       instance_ptr                            Instance pointer.
*       display_framebuffer_addr                frame buffer address
*
* OUTPUTS
*
*       status                              NU_SUCCESS or Any error occurred during
*                                           initialization.
*
*************************************************************************/
STATUS Display_Tgt_Initialize(DISPLAY_INSTANCE_HANDLE *instance_ptr, VOID *display_framebuffer_addr) 
{
    LTDC_TGT        *tgt_info = (LTDC_TGT*)(instance_ptr->display_tgt_ptr);
    VOID            (*setup_func)(VOID);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE pmi_dev = (instance_ptr->pmi_dev);
    
    /* Check current state of the device. */
    if (PMI_STATE_GET(pmi_dev) == POWER_OFF_STATE)
    {
        return NU_SUCCESS; 
    }
#endif 

    /* Call the target specific setup function. */
    setup_func = tgt_info->setup_func;
    if(setup_func != NU_NULL)
    {
        setup_func();
    }    

    /* Save global target info for using in post processing operation. */
    global_instance_ptr = instance_ptr;
    
    /* LCD Configuration */
    Display_Tgt_LCD_Configuration();
      
    /* Enable Layer1. Only one layer is used. */
    Display_Tgt_LTDC_Enable_Layer(LTDC_LAYER1_BASE_ADDRESS, ENABLE);
      
    /* Reload configuration of Layer1 */
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + SRCR, LTDC_IMReload);  
    
    /* Enable The LCD */
    Display_Tgt_LTDC_Enable(ENABLE);  

    /* Return completion status. */
    return (NU_SUCCESS);
}
/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Fill_Rect_Hook
*
* DESCRIPTION
*
*       Fills rectangular area on screen with fill color.
*
* INPUTS
*
*       x_min                               Lower x coordinate.
*       x_max                               Higher x coordinate.
*       y_min                               Lower y coordinate.
*       y_max                               Higher y coordinate.
*       fill_colour                         Fill color.
*
* OUTPUTS
*
*        None
*
*************************************************************************/
VOID Display_Tgt_Fill_Rect_Hook(UINT16 x_min, UINT16 x_max, UINT16 y_min, UINT16 y_max, UINT32 fill_colour) 
{
    NU_UNUSED_PARAM(x_min);
    NU_UNUSED_PARAM(x_max);
    NU_UNUSED_PARAM(y_min);
    NU_UNUSED_PARAM(y_max);
    NU_UNUSED_PARAM(fill_colour);

    /* This function will be used in case of SMART LCD */  
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Set_Pixel_Hook
*
* DESCRIPTION
*
*       This function sets a pixel on the screen.
*
* INPUTS
*
*       x_coordinate                        X coordinate of pixel
*       y_coordinate                        Y coordinate of pixel
*       fill_colour                         Pixel value to set
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Display_Tgt_Set_Pixel_Hook(UINT16 x_coordinate, UINT16 y_coordinate, UINT32 fill_colour) 
{
    NU_UNUSED_PARAM(x_coordinate);
    NU_UNUSED_PARAM(y_coordinate);
    NU_UNUSED_PARAM(fill_colour);

    /* This function will be used in case of SMART LCD */      
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Get_Pixel_Hook
*
* DESCRIPTION
*
*       This function gets a pixel from the screen.
*
* INPUTS
*
*       x_coordinate                        X coordinate of pixel
*       y_coordinate                        Y coordinate of pixel
*       fill_colour_ptr                     Pointer to hold pixel value
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Display_Tgt_Get_Pixel_Hook(UINT16 x_coordinate, UINT16 y_coordinate, UINT32* fill_colour_ptr) 
{
    NU_UNUSED_PARAM(x_coordinate);
    NU_UNUSED_PARAM(y_coordinate);
    NU_UNUSED_PARAM(fill_colour_ptr);

    /* This function will be used in case of SMART LCD */  
}
/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Read_Palette
*
* DESCRIPTION
*
*       This function reads the hardware palette (if being used).
*
* INPUTS
*
*       palette_ptr                         Points to the location where
*                                           palette data will be stored.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Display_Tgt_Read_Palette(VOID *palette_ptr)
{
    /* Ensure unused variable don't generate toolset warnings. */
    NU_UNUSED_PARAM(palette_ptr);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Write_Palette
*
* DESCRIPTION
*
*       This function writes the hardware palette (if being used).
*
* INPUTS
*
*       palette_ptr                         Points to the palette to be
*                                           written.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID Display_Tgt_Write_Palette(VOID *palette_ptr)
{
    /* Ensure unused variable don't generate toolset warnings. */
    NU_UNUSED_PARAM(palette_ptr);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Shutdown
*
* DESCRIPTION
*
*       This function turns the LCD controller OFF.
*
* INPUTS
*
*       DISPLAY_INSTANCE_HANDLE    *instance_ptr    - Device instance handle.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID Display_Tgt_Shutdown(DISPLAY_INSTANCE_HANDLE *instance_ptr) 
{
    LTDC_TGT * tgt_inf = (LTDC_TGT *)instance_ptr->display_tgt_ptr; 
    VOID               (*cleanup_fn)(VOID);

    /* Turn the LCD off. */
    (VOID)Display_Tgt_Set_State_OFF((VOID*)tgt_inf);

    /* Call the target specific cleanup_func function. */
    cleanup_fn = tgt_inf->cleanup_func;
    if(cleanup_fn != NU_NULL)
    {
        cleanup_fn();
    }
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Pre_Process_Hook
*
* DESCRIPTION
*
*       This function does any pre-processing required before the LCD
*       buffer is updated.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    Display_Tgt_Pre_Process_Hook(VOID)
{
    /* Add anything required for the pre-processing of the buffer here. */
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Post_Process_Hook
*
* DESCRIPTION
*
*       This function does any post-processing required after the LCD
*       buffer is updated.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    Display_Tgt_Post_Process_Hook(VOID)
{
    
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Backlight_Set_ON
*
* DESCRIPTION
*
*       This function turns the LCD backlight ON.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS  Display_Tgt_Backlight_Set_ON(VOID *tgt_info) 
{
    /* Turn on the LCD controller. */    
    Display_Tgt_LTDC_Enable(ENABLE);

    backlight = ENABLE; 

    /* Return the completion status. */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Backlight_Set_OFF
*
* DESCRIPTION
*
*       This function turns the LCD backlight OFF.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS  Display_Tgt_Backlight_Set_OFF(VOID *tgt_info) 
{
    /* Turn off the LCD controller. */    
    Display_Tgt_LTDC_Enable(DISABLE);

    backlight = DISABLE; 

    /* Return the completion status. */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Backlight_Status
*
* DESCRIPTION
*
*       This function returns the current status of the LCD backlight.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
*       backlight_status_ptr                Points to the location where
*                                           backlight status will be
*                                           returned.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS  Display_Tgt_Backlight_Status(VOID *tgt_info, BOOLEAN *backlight_status_ptr) 
{
    STATUS status = NU_SUCCESS;

    if (backlight_status_ptr != NU_NULL)
    {

        if(backlight == ENABLE)
        {
            /* Backlight is On. */
            *backlight_status_ptr = NU_TRUE;
        }
        else
        {
            /* Backlight is Off. */
            *backlight_status_ptr = NU_FALSE;
        }
    }
    else
    {
        /* Invalid argument passed. */
        status = LCDC_INVALID_VALUE;
    }

    /* Return the completion status. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Contrast_Set
*
* DESCRIPTION
*
*       This function sets the LCD contrast to the specified value.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
*       contrast                            Desired contrast in percent.
*
* OUTPUTS
*
*       status                              NU_SUCCESS
*                                           LCDC_INVALID_CONTRAST_VALUE
*
*************************************************************************/
STATUS  Display_Tgt_Contrast_Set(VOID *tgt_info, INT8 contrast) 
{
    NU_UNUSED_PARAM(tgt_info);
    NU_UNUSED_PARAM(contrast);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Contrast_Get
*
* DESCRIPTION
*
*       This function returns the current value of LCD contrast.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
*       contrast_ptr                        Points to the location where
*                                           contrast information will be
*                                           returned.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS  Display_Tgt_Contrast_Get(VOID *tgt_info, INT8 *contrast_ptr) 
{
    NU_UNUSED_PARAM(tgt_info);

    *contrast_ptr = LCD_CONTRAST_MAX;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       Display_Tgt_Allocate_Framebuffer
*
* DESCRIPTION
*
*       This function allocates the framebuffer.
*
* INPUTS
*
*       tgt_info                            LCDC target information.
*
* OUTPUTS
*
*       VOID *                              Address of the allocated
*                                           buffer. NU_NULL if allocation
*                                           fails.
*
*************************************************************************/
VOID    *Display_Tgt_Allocate_Framebuffer(VOID *tgt_info) 
{
    LTDC_TGT         *tgt_inf = (LTDC_TGT *)tgt_info;
    VOID             *base_address;
    STATUS           memory_status;
    VOID             *buffer_start;
    SIGNED           size;
    NU_MEMORY_POOL   *sys_pool_ptr;

    /* Get the size of the memory that will need to be allocated for the frame buffer. */
    size =  tgt_inf->lcdc_screen_width * tgt_inf->lcdc_screen_height * (tgt_inf->lcdc_bits_per_pixel >> 3);

    /* Get system memory pool */
    memory_status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
            
    if (memory_status == NU_SUCCESS)
    {
        /* Allocate memory for the device instance */
        memory_status = NU_Allocate_Memory(sys_pool_ptr, &base_address, size, NU_NO_SUSPEND);
    }
    
    /* Ensure that there was not an error allocating memory. */
    if (memory_status != NU_SUCCESS)
    {
        /* There was a problem in allocating memory. */
        buffer_start = NU_NULL;
    }

    /* Memory allocation was successful. */
    else
    {
        /* Get the starting address of the frame buffer. */
        buffer_start = base_address;

        /* Also Store in local instance pointer */
        tgt_inf->lcdc_framebuffer_addr = base_address;

        /* Clear the allocated buffer memory. */
        memset((VOID *)base_address, 0, size );
    }

    /* Return the address of the frame buffer. */
    return (buffer_start);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_Set_State_ON
*
*   DESCRIPTION
*
*       This function turns the LCD ON for power management.
*
*   INPUT
*
*       instance_handle                     Instance Handle 
*
*   OUTPUT
*
*                                           NU_SUCCESS
*
*************************************************************************/
STATUS  Display_Tgt_Set_State_ON(VOID *instance_handle)
{
    NU_UNUSED_PARAM(instance_handle);

    /* Display ON Setting */
    Display_Tgt_LTDC_Enable(ENABLE);  

    backlight = ENABLE; 

    /* Return completion status. */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_Set_State_OFF
*
*   DESCRIPTION
*
*       This function turns the LCD OFF for power management.
*
*   INPUT
*
*       tgt_info                            LCDC target information.
*
*   OUTPUT
*
*                                           NU_SUCCESS
*
*************************************************************************/
STATUS    Display_Tgt_Set_State_OFF(VOID *instance_handle)
{
    /* Turn off the LCD controller. */  
    Display_Tgt_LTDC_Enable(DISABLE);

    backlight = DISABLE; 
    
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Configuration
*
*   DESCRIPTION
*
*       This function configures the LCD.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID Display_Tgt_LCD_Configuration()
{
    LTDC_Init                      ltdc;
    LTDC_Layer_Init                ltdc_layer;

    /* Configure the LCD_SPI interface */
    Display_Tgt_LCD_SPI_Configuration(); 

    /* Power on the LCD */
    Display_Tgt_LCD_Enable_Power();

    /* Enable the LTDC Clock */
    RCC_APB2_Periph_Clock_Enable(RCC_APB2Periph_LTDC, ENABLE);

    /* Enable the DMA2D Clock */
    RCC_AHB1_Periph_Clock_Enable(RCC_AHB1Periph_DMA2D, ENABLE); 
    
    /* Enable Pixel Clock by configuring PLLSAI prescalers for LCD */
    RCC_PLL_SAI_Configuration(192, 7, 3);
    RCC_LTDC_Clock_Div_Configuration(RCC_PLLSAIDivR_Div8);

    /* Enable PLLSAI Clock */
    RCC_PLL_SAI_Enable(ENABLE);
    
    /* Wait for PLLSAI activation */
    while(RCC_Get_Flag_Status(RCC_FLAG_PLLSAIRDY) == RESET)
    {
    
    }

    /* LTDC Initialization */

    /* Initialize the horizontal synchronization polarity as active low*/
    ltdc.LTDC_HSPolarity = LTDC_HSPolarity_AL;     
    
    /* Initialize the vertical synchronization polarity as active low */  
    ltdc.LTDC_VSPolarity = LTDC_VSPolarity_AL;     
    
    /* Initialize the data enable polarity as active low */ 
    ltdc.LTDC_DEPolarity = LTDC_DEPolarity_AL;     
    
    /* Initialize the pixel clock polarity as input pixel clock */ 
    ltdc.LTDC_PCPolarity = LTDC_PCPolarity_IPC;

    /* Timing configuration */
    /* Configure horizontal synchronization width */     
    ltdc.LTDC_HorizontalSync = 9;
    
    /* Configure vertical synchronization height */
    ltdc.LTDC_VerticalSync = 1;
    
    /* Configure accumulated horizontal back porch */
    ltdc.LTDC_AccumulatedHBP = 29; 
    
    /* Configure accumulated vertical back porch */
    ltdc.LTDC_AccumulatedVBP = 3;  
    
    /* Configure accumulated active width */  
    ltdc.LTDC_AccumulatedActiveW = 269;
    
    /* Configure accumulated active height */
    ltdc.LTDC_AccumulatedActiveH = 323;
    
    /* Configure total width */
    ltdc.LTDC_TotalWidth = 279; 
    
    /* Configure total height */
    ltdc.LTDC_TotalHeigh = 327;

    Display_Tgt_LTDC_Initialization(&ltdc);

    /* Configure R,G,B component values for LCD background color */                   
    ltdc.LTDC_BackgroundRedValue = 0;            
    ltdc.LTDC_BackgroundGreenValue = 0;          
    ltdc.LTDC_BackgroundBlueValue = 0; 
            
    Display_Tgt_LTDC_Initialization(&ltdc);

    /* Layer1 Configuration */  

    /* If all the active display area is used to display a picture then :
     Horizontal start = horizontal synchronization + Horizontal back porch = 30 
     Horizontal stop = Horizontal start + window width -1 = 30 + 240 -1
     Vertical start   = vertical synchronization + vertical back porch     = 4
     Vertical stop   = Vertical start + window height -1  = 4 + 320 -1      */ 
    ltdc_layer.LTDC_HorizontalStart = 30;
    ltdc_layer.LTDC_HorizontalStop = (240 + 30 - 1); 
    ltdc_layer.LTDC_VerticalStart = 4;
    ltdc_layer.LTDC_VerticalStop = (320 + 4 - 1);

    /* Pixel Format configuration*/           
    ltdc_layer.LTDC_PixelFormat = LTDC_Pixelformat_RGB565;

    /* Alpha constant (255 totally opaque) */
    ltdc_layer.LTDC_ConstantAlpha = 255; 

    /* Configure blending factors */       
    ltdc_layer.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;    
    ltdc_layer.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;  

    /* Default Color configuration (configure A,R,G,B component values) */          
    ltdc_layer.LTDC_DefaultColorBlue = 0;        
    ltdc_layer.LTDC_DefaultColorGreen = 0;       
    ltdc_layer.LTDC_DefaultColorRed = 0;         
    ltdc_layer.LTDC_DefaultColorAlpha = 0;   

    /* Input Address configuration */    
    ltdc_layer.LTDC_CFBStartAdress = (UINT32)global_instance_ptr->display_framebuffer_addr; 

    /* the length of one line of pixels in bytes + 3 then :
     Line Lenth = Active high width x number of bytes per pixel + 3 
     Active high width         = 240 
     number of bytes per pixel = 2    (pixel_format : RGB565) */
    ltdc_layer.LTDC_CFBLineLength = ((240 * 2) + 3);

    /*  the pitch is the increment from the start of one line of pixels to the 
      start of the next line in bytes, then :
      Pitch = Active high width x number of bytes per pixel   */
    ltdc_layer.LTDC_CFBPitch = (240 * 2);  

    /* configure the number of lines */
    ltdc_layer.LTDC_CFBLineNumber = 320;

    Display_Tgt_LTDC_Layer_Initialization(LTDC_LAYER1_BASE_ADDRESS, &ltdc_layer);

    Display_Tgt_LTDC_Dither_Enable(ENABLE);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LTDC_Initialization
*
*   DESCRIPTION
*
*       This function initialize LTDC Controller 
*
*   INPUT
*
*       ltdc_init                      - LTDC Init structure 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID Display_Tgt_LTDC_Initialization(LTDC_Init* ltdc_init)
{
    UINT32 horizontalsync = 0;
    UINT32 accumulatedHBP = 0;
    UINT32 accumulatedactiveW = 0;
    UINT32 totalwidth = 0;
    UINT32 backgreen = 0;
    UINT32 backred = 0;

    /* Sets Synchronization size */
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + SSCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + SSCR) & ~(LTDC_SSCR_VSH | LTDC_SSCR_HSW) );
    horizontalsync = (ltdc_init->LTDC_HorizontalSync << 16);
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + SSCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + SSCR) | (horizontalsync | ltdc_init->LTDC_VerticalSync) );

    /* Sets Accumulated Back porch */
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + BPCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + BPCR) & ~(LTDC_BPCR_AVBP | LTDC_BPCR_AHBP) );
    accumulatedHBP = (ltdc_init->LTDC_AccumulatedHBP << 16);
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + BPCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + BPCR) | (accumulatedHBP | ltdc_init->LTDC_AccumulatedVBP) );

    /* Sets Accumulated Active Width */
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + AWCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + AWCR) & ~(LTDC_AWCR_AAH | LTDC_AWCR_AAW) );
    accumulatedactiveW = (ltdc_init->LTDC_AccumulatedActiveW << 16);
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + AWCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + AWCR) | (accumulatedactiveW | ltdc_init->LTDC_AccumulatedActiveH) );

    /* Sets Total Width */
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + TWCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + TWCR) & ~(LTDC_TWCR_TOTALH | LTDC_TWCR_TOTALW) );
    totalwidth = (ltdc_init->LTDC_TotalWidth << 16);
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + TWCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + TWCR) | (totalwidth | ltdc_init->LTDC_TotalHeigh) );

    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) & (UINT32)GCR_MASK );
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) | (UINT32)(ltdc_init->LTDC_HSPolarity | ltdc_init->LTDC_VSPolarity | ltdc_init->LTDC_DEPolarity | ltdc_init->LTDC_PCPolarity) );

    /* sets the background color value */
    backgreen = (ltdc_init->LTDC_BackgroundGreenValue << 8);
    backred = (ltdc_init->LTDC_BackgroundRedValue << 16);

    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + BCCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + BCCR) & ~(LTDC_BCCR_BCBLUE | LTDC_BCCR_BCGREEN | LTDC_BCCR_BCRED) );
    ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + BCCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + BCCR) | (backred | backgreen | ltdc_init->LTDC_BackgroundBlueValue) );
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LTDC_Enable
*
*   DESCRIPTION
*
*       This function Enable or Disable LTDC Controller  
*
*   INPUT
*
*       state                      - Enable or Disable value  
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID Display_Tgt_LTDC_Enable(FunctionalState state)
{
    if (state != DISABLE)
    {
        /* Enable LTDC by setting LTDCEN bit */
        ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) | (UINT32)LTDC_GCR_LTDCEN );
    }
    else
    {
        /* Disable LTDC by clearing LTDCEN bit */
        ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) & ~(UINT32)LTDC_GCR_LTDCEN );
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LTDC_Dither_Enable
*
*   DESCRIPTION
*
*       This function Enable/Disable Dither feature   
*
*   INPUT
*
*       state                      - Enable or Disable value  
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID Display_Tgt_LTDC_Dither_Enable(FunctionalState state)
{
    if (state != DISABLE)
    {
        /* Enable Dither by setting DTEN bit */
        ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) | (UINT32)LTDC_GCR_DTEN );
    }
    else
    {
        /* Disable Dither by clearing DTEN bit */
        ESAL_GE_MEM_WRITE32(LTDC_BASE_ADDRESS + GCR, ESAL_GE_MEM_READ32(LTDC_BASE_ADDRESS + GCR) & ~(UINT32)LTDC_GCR_DTEN );
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LTDC_Layer_Initialization
*
*   DESCRIPTION
*
*       This function initialize layer of LTDC    
*
*   INPUT
*
*       ltdc_layerx                     - Layer 1/2 base address    
*       ltdc_layer_Init                 - LTDC Layer Init Structure 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID Display_Tgt_LTDC_Layer_Initialization(UINT32 ltdc_layerx, LTDC_Layer_Init* ltdc_layer_Init)
{
    UINT32 whsppos = 0;
    UINT32 wvsppos = 0;
    UINT32 dcgreen = 0;
    UINT32 dcred = 0;
    UINT32 dcalpha = 0;
    UINT32 cfbp = 0;

    /* Configures the horizontal start and stop position */
    whsppos = ltdc_layer_Init->LTDC_HorizontalStop << 16;
    ESAL_GE_MEM_WRITE32(ltdc_layerx + WHPCR, ESAL_GE_MEM_READ32(ltdc_layerx + WHPCR) & ~(LTDC_LxWHPCR_WHSTPOS | LTDC_LxWHPCR_WHSPPOS));
    ESAL_GE_MEM_WRITE32(ltdc_layerx + WHPCR, (ltdc_layer_Init->LTDC_HorizontalStart | whsppos));
        
    /* Configures the vertical start and stop position */
    wvsppos = ltdc_layer_Init->LTDC_VerticalStop << 16;
    ESAL_GE_MEM_WRITE32(ltdc_layerx + WVPCR, ESAL_GE_MEM_READ32(ltdc_layerx + WVPCR) & ~(LTDC_LxWVPCR_WVSTPOS | LTDC_LxWVPCR_WVSPPOS) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + WVPCR, (ltdc_layer_Init->LTDC_VerticalStart | wvsppos) );

    /* Specifies the pixel format */
    ESAL_GE_MEM_WRITE32(ltdc_layerx + PFCR, ESAL_GE_MEM_READ32(ltdc_layerx + PFCR) & ~(LTDC_LxPFCR_PF) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + PFCR, (ltdc_layer_Init->LTDC_PixelFormat) );

    /* Configures the default color values */
    dcgreen = (ltdc_layer_Init->LTDC_DefaultColorGreen << 8);
    dcred = (ltdc_layer_Init->LTDC_DefaultColorRed << 16);
    dcalpha = (ltdc_layer_Init->LTDC_DefaultColorAlpha << 24);
    ESAL_GE_MEM_WRITE32(ltdc_layerx + DCCR, ESAL_GE_MEM_READ32(ltdc_layerx + DCCR) & ~(LTDC_LxDCCR_DCBLUE | LTDC_LxDCCR_DCGREEN | LTDC_LxDCCR_DCRED | LTDC_LxDCCR_DCALPHA) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + DCCR, (ltdc_layer_Init->LTDC_DefaultColorBlue | dcgreen | dcred | dcalpha) );

    /* Specifies the constant alpha value */      
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CACR, ESAL_GE_MEM_READ32(ltdc_layerx + CACR) & ~(LTDC_LxCACR_CONSTA) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CACR, (ltdc_layer_Init->LTDC_ConstantAlpha) );

    /* Specifies the blending factors */
    ESAL_GE_MEM_WRITE32(ltdc_layerx + BFCR, ESAL_GE_MEM_READ32(ltdc_layerx + BFCR) & ~(LTDC_LxBFCR_BF2 | LTDC_LxBFCR_BF1) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + BFCR, (ltdc_layer_Init->LTDC_BlendingFactor_1 | ltdc_layer_Init->LTDC_BlendingFactor_2) );

    /* Configures the color frame buffer start address */
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBAR, ESAL_GE_MEM_READ32(ltdc_layerx + CFBAR) & ~(LTDC_LxCFBAR_CFBADD) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBAR, (ltdc_layer_Init->LTDC_CFBStartAdress) );

    /* Configures the color frame buffer pitch in byte */
    cfbp = (ltdc_layer_Init->LTDC_CFBPitch << 16);
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBLR, ESAL_GE_MEM_READ32(ltdc_layerx + CFBLR) & ~(LTDC_LxCFBLR_CFBLL | LTDC_LxCFBLR_CFBP) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBLR, (ltdc_layer_Init->LTDC_CFBLineLength | cfbp) );

    /* Configures the frame buffer line number */
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBLNR, ESAL_GE_MEM_READ32(ltdc_layerx + CFBLNR) & ~(LTDC_LxCFBLNR_CFBLNBR) );
    ESAL_GE_MEM_WRITE32(ltdc_layerx + CFBLNR,  (ltdc_layer_Init->LTDC_CFBLineNumber) );
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LTDC_Enable_Layer
*
*   DESCRIPTION
*
*       This function initialize layer of LTDC    
*
*   INPUT
*
*       ltdc_layerx                      - Layer 1/2 base address    
*       option                           - Enable/Disable value  
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID Display_Tgt_LTDC_Enable_Layer(UINT32 ltdc_layerx, FunctionalState option)
{
    if (option != DISABLE)
    {
        /* Enable LTDC_Layer by setting LEN bit */
        ESAL_GE_MEM_WRITE32(ltdc_layerx + CR, ESAL_GE_MEM_READ32(ltdc_layerx + CR) | (UINT32)LTDC_LxCR_LEN );
    }
    else
    {
        /* Disable LTDC_Layer by clearing LEN bit */
        ESAL_GE_MEM_WRITE32(ltdc_layerx + CR, ESAL_GE_MEM_READ32(ltdc_layerx + CR) & ~(UINT32)LTDC_LxCR_LEN );
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Write_Command
*
*   DESCRIPTION
*
*       This function sends register address (command) over SPI   
*
*   INPUT
*
*       reg                      - Register (command) Address  
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_Write_Command(UINT8 reg)
{
    /* Reset WRX (LCD_WRX_GPIO_PORT) to send command */
    LCD_Write_Control_Lines(GPIOD_BASE_ADDRESS, LCD_WRX_PIN, Bit_RESET);

    /* Reset LCD control line(/CS) and Send command */
    LCD_Chip_Select(DISABLE);
    
    Display_Tgt_SPI_I2S_Send_Data(SPI5_BASE_ADDRESS, reg);

    ESAL_PR_Delay_USec(10);
    
    LCD_Chip_Select(ENABLE);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Write_Data
*
*   DESCRIPTION
*
*       This function sends data over SPI 
*
*   INPUT
*
*       value                      - Data to be written over SPI   
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_Write_Data(UINT8 value)
{
    /* Set WRX (LCD_WRX_GPIO_PORT) to send data */
    LCD_Write_Control_Lines(GPIOD_BASE_ADDRESS, LCD_WRX_PIN, Bit_SET);
    
    /* Reset LCD control line(/CS) and Send data */  
    LCD_Chip_Select(DISABLE);
    
    Display_Tgt_SPI_I2S_Send_Data(SPI5_BASE_ADDRESS, value);

    ESAL_PR_Delay_USec(10);
    
    LCD_Chip_Select(ENABLE);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Enable_Power
*
*   DESCRIPTION
*
*       This function sends series of commands and data over SPI 
*
*   INPUT
*
*       None 
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_Enable_Power()
{
    Display_Tgt_LCD_Write_Command(0xCA);
    Display_Tgt_LCD_Write_Data(0xC3);
    Display_Tgt_LCD_Write_Data(0x08);
    Display_Tgt_LCD_Write_Data(0x50);
    
    Display_Tgt_LCD_Write_Command(LCD_POWERB);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0xC1);
    Display_Tgt_LCD_Write_Data(0x30);
    
    Display_Tgt_LCD_Write_Command(LCD_POWER_SEQ);
    Display_Tgt_LCD_Write_Data(0x64);
    Display_Tgt_LCD_Write_Data(0x03);
    Display_Tgt_LCD_Write_Data(0x12);
    Display_Tgt_LCD_Write_Data(0x81);
    
    Display_Tgt_LCD_Write_Command(LCD_DTCA);
    Display_Tgt_LCD_Write_Data(0x85);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x78);
    
    Display_Tgt_LCD_Write_Command(LCD_POWERA);
    Display_Tgt_LCD_Write_Data(0x39);
    Display_Tgt_LCD_Write_Data(0x2C);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x34);
    Display_Tgt_LCD_Write_Data(0x02);
    
    Display_Tgt_LCD_Write_Command(LCD_PRC);
    Display_Tgt_LCD_Write_Data(0x20);
    
    Display_Tgt_LCD_Write_Command(LCD_DTCB);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x00);
    
    Display_Tgt_LCD_Write_Command(LCD_FRC);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x1B);
    
    Display_Tgt_LCD_Write_Command(LCD_DFC);
    Display_Tgt_LCD_Write_Data(0x0A);
    Display_Tgt_LCD_Write_Data(0xA2);
    
    Display_Tgt_LCD_Write_Command(LCD_POWER1);
    Display_Tgt_LCD_Write_Data(0x10);
    
    Display_Tgt_LCD_Write_Command(LCD_POWER2);
    Display_Tgt_LCD_Write_Data(0x10);
    
    Display_Tgt_LCD_Write_Command(LCD_VCOM1);
    Display_Tgt_LCD_Write_Data(0x45);
    Display_Tgt_LCD_Write_Data(0x15);
    
    Display_Tgt_LCD_Write_Command(LCD_VCOM2);
    Display_Tgt_LCD_Write_Data(0x90);
    
    Display_Tgt_LCD_Write_Command(LCD_MAC);
    Display_Tgt_LCD_Write_Data(0xC8);
    
    Display_Tgt_LCD_Write_Command(LCD_3GAMMA_EN);
    Display_Tgt_LCD_Write_Data(0x00);
    
    Display_Tgt_LCD_Write_Command(LCD_RGB_INTERFACE);
    Display_Tgt_LCD_Write_Data(0xC2);
    
    Display_Tgt_LCD_Write_Command(LCD_DFC);
    Display_Tgt_LCD_Write_Data(0x0A);
    Display_Tgt_LCD_Write_Data(0xA7);
    Display_Tgt_LCD_Write_Data(0x27);
    Display_Tgt_LCD_Write_Data(0x04);

    /* colomn address set */
    Display_Tgt_LCD_Write_Command(LCD_COLUMN_ADDR);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0xEF);
    
    /* Page Address Set */
    Display_Tgt_LCD_Write_Command(LCD_PAGE_ADDR);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x01);
    Display_Tgt_LCD_Write_Data(0x3F);
    
    Display_Tgt_LCD_Write_Command(LCD_INTERFACE);
    Display_Tgt_LCD_Write_Data(0x01);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x06);

    Display_Tgt_LCD_Write_Command(LCD_GRAM);

    ESAL_PR_Delay_USec(200);

    Display_Tgt_LCD_Write_Command(LCD_GAMMA);
    Display_Tgt_LCD_Write_Data(0x01);

    Display_Tgt_LCD_Write_Command(LCD_PGAMMA);
    Display_Tgt_LCD_Write_Data(0x0F);
    Display_Tgt_LCD_Write_Data(0x29);
    Display_Tgt_LCD_Write_Data(0x24);
    Display_Tgt_LCD_Write_Data(0x0C);
    Display_Tgt_LCD_Write_Data(0x0E);
    Display_Tgt_LCD_Write_Data(0x09);
    Display_Tgt_LCD_Write_Data(0x4E);
    Display_Tgt_LCD_Write_Data(0x78);
    Display_Tgt_LCD_Write_Data(0x3C);
    Display_Tgt_LCD_Write_Data(0x09);
    Display_Tgt_LCD_Write_Data(0x13);
    Display_Tgt_LCD_Write_Data(0x05);
    Display_Tgt_LCD_Write_Data(0x17);
    Display_Tgt_LCD_Write_Data(0x11);
    Display_Tgt_LCD_Write_Data(0x00);
    
    Display_Tgt_LCD_Write_Command(LCD_NGAMMA);
    Display_Tgt_LCD_Write_Data(0x00);
    Display_Tgt_LCD_Write_Data(0x16);
    Display_Tgt_LCD_Write_Data(0x1B);
    Display_Tgt_LCD_Write_Data(0x04);
    Display_Tgt_LCD_Write_Data(0x11);
    Display_Tgt_LCD_Write_Data(0x07);
    Display_Tgt_LCD_Write_Data(0x31);
    Display_Tgt_LCD_Write_Data(0x33);
    Display_Tgt_LCD_Write_Data(0x42);
    Display_Tgt_LCD_Write_Data(0x05);
    Display_Tgt_LCD_Write_Data(0x0C);
    Display_Tgt_LCD_Write_Data(0x0A);
    Display_Tgt_LCD_Write_Data(0x28);
    Display_Tgt_LCD_Write_Data(0x2F);
    Display_Tgt_LCD_Write_Data(0x0F);

    Display_Tgt_LCD_Write_Command(LCD_SLEEP_OUT);
    
    ESAL_PR_Delay_USec(200);
    
    Display_Tgt_LCD_Write_Command(LCD_DISPLAY_ON);
    
    /* GRAM start writing */
    Display_Tgt_LCD_Write_Command(LCD_GRAM);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Display_ON
*
*   DESCRIPTION
*
*       This function ON the LCD  
*
*   INPUT
*
*       None    
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_Display_ON()
{
    /* Display ON */
    Display_Tgt_LCD_Write_Command(LCD_DISPLAY_ON);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_Display_OFF
*
*   DESCRIPTION
*
*       This function OFF the LCD  
*
*   INPUT
*
*       None    
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_Display_OFF()
{
    /* Display Off */
    Display_Tgt_LCD_Write_Command(LCD_DISPLAY_OFF);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_LCD_SPI_Configuration
*
*   DESCRIPTION
*
*       This function initialize LCD SPI Configuration     
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_LCD_SPI_Configuration()
{
    /* Register SPI Slave */
    NU_SPI_Register_Slave(SPI_DEVICE_NAME, SPI_BAUD_RATE, SPI_TRANSFER_SIZE, SPI_TRANSFER_MODE|SPI_TRANSFER_BIT_ORDER|SPI_SS_POL_DONT_CARE,
                            chipselect_function, &Demo_SPI_Handle);
}

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_SPI_I2S_Send_Data
*
*   DESCRIPTION
*
*       This function sends data over SPI using I2S mode
*
*   INPUT
*
*       SPIx                 - SPI device number  
*       data                 - data to be written 
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Display_Tgt_SPI_I2S_Send_Data(UINT32 SPIx, UINT8 data)
{
    /* Send Data */
    NU_SPI_Write(Demo_SPI_Handle, SPI_POLLED_IO, &data, 1);
}



