/******************************************************************************
*                                                                             
*              Copyright Mentor Graphics Corporation 2006                     
*                        All Rights Reserved.                                 
*                                                                             
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS       
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS        
*  SUBJECT TO LICENSE TERMS.                                                  
*                                                                             
*                                                                             
*******************************************************************************
*******************************************************************************
*                                                                       
* FILE NAME                                                             
*                                                                       
*      bagif.h                                                          
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*       Contains defines, prototypes and all of the data structures necessary     
*       for using and displaying animated Gifs.                                        
*
*  DATA STRUCTURES                                                      
*       
*       LCT_STRUCT
*       Animated_GIF
*       Animated_Entry                                                   
*                                                                       
*  FUNCTIONS                                                            
*                                                                       
*       None                                                            
*                                                                       
*  DEPENDENCIES                                                         
*                                                                       
*       image/inc/imgincludes.h
*
*************************************************************************/
#ifndef BAGIF_H
#define BAGIF_H

#include "ui/imgincludes.h"
#ifdef IMG_INCLUDED

#define BRW_INCLUDE_GIF_SUPPORT

#define COLOR_PALETTE_ARRAY_SIZE    512

#ifdef BRW_INCLUDE_GIF_SUPPORT
#define START_ANIM    0x0001   /*  Used to start the Logo */
#define STOP_ANIM     0x0002   /*  Used to Stop Logo */
#define ANIM_FREE_DONE 0x0004  /*  Used for the Animate Gif Task to tell routine that he is through */

#define GETIT           1
#define NOTFOUND        2
#define COORD_LEN       5
#define STOP_TASK       1
#define SUSPEND_TASK    2
#define START_ANIM_TASK 4
#define TASK_SUSPENDED  8
#define TASK_START_AGAIN 16
#define GIF_STACK_SIZE  40000

#define BRW_MONOCHROME_TRANSPARENT_COLOR   Black 



void AnimateGif(UNSIGNED argc, VOID *argv);
STATUS init_animated_Gif_Entry(void);   /*  Initialize the Animated Gif Structure   */
VOID init_animated_Gif(UINT32);
STATUS Create_Animated_Gif_Task(CHAR *name);
STATUS NU_Create_Animated_Gif_Tasks(CHAR *name);  /*  Animated Gif Creation routine */

typedef struct _LCT_STRUCT                /*  The local Color table Palette information */
{
    palData Locol_Ct[COLOR_PALETTE_ARRAY_SIZE];
} LCT_STRUCT;

typedef enum _Animated_Disposal
{
	NO_DISPOSAL = 1,
	RESTORE_BACKGROUND,
	RESTORE_PREVIOUS,
}Animated_Disposal;

typedef struct _Animated_GIF
{
    unsigned char *imgframe[NUM_ANIMATED_FRAMES];    /*  The Animated Frames Image Data                                */
    short x_origin[NUM_ANIMATED_FRAMES];             /*  The x origin offset for each frame                            */
    short y_origin[NUM_ANIMATED_FRAMES];             /*  The y organ offset for each frame                             */

    short imwidth[NUM_ANIMATED_FRAMES];              /*  The image width for each frame                                */
    short imheight[NUM_ANIMATED_FRAMES];             /*  The image height for each frame                               */
    short row_bytes[NUM_ANIMATED_FRAMES];            /*  The row_bytes for each frame                                  */
    short TransParent;                               /*  Transparent animated Gifs                                     */

    short GCT;                                       /*  Does this image have a Global color Table                     */
    unsigned char LCT[NUM_ANIMATED_FRAMES];          /*  The local Color Table for each frame                          */
    palData Global_Ct[COLOR_PALETTE_ARRAY_SIZE];     /*  The global Color Palette for this image                       */
    LCT_STRUCT *Lct[NUM_ANIMATED_FRAMES];            /*  The local color table for each frame if present              */
    int transparent_Index[NUM_ANIMATED_FRAMES];      /*  The transparent color index for each frame                    */
    short NumFrames;                                 /*  The number of frames for each image.                          */
    short Size_difference;                           /*  Is their a size difference for each frame in this image       */
    short BackGround;                                /*  Is this a background image                                    */
    unsigned int delay[NUM_ANIMATED_FRAMES];         /*  The delay(Hundredth of Milliseconds) of each animated frame.  */
    short CurrentFrame;                              /*  The current frame                                             */
    rect aRect;                                      /*  The initial image rectangle                                   */

    int transparent[NUM_ANIMATED_FRAMES];            /*  Frame transparency                                            */
    Animated_Disposal disposal[NUM_ANIMATED_FRAMES]; /*  Disposal of last image                                        */
}Animated_GIF;                                       /*  The animated Gif data Structure                               */



typedef struct _Animated_Entry
{
    Animated_GIF *agif[NUM_ANIMATED_GIFS];          /*  The animated Gif structure              */
    int NumAGifs;                                   /*  counter for the Number of Animated Gifs */
    int Maxdelay;                                   /*  The max delay for each image            */
    VOID *pointer[NUM_ANIMATED_GIFS];               /*  Task Pointer each animated GIf          */
    int Task_Stopped[NUM_ANIMATED_GIFS];            /*  Tasks stopped for each animated Gif     */
    int Task_Started[NUM_ANIMATED_GIFS];            /*  Tasks started for each animated Gif     */
    int Suspended;                                  /*  Is the animated Gif suspended           */
}Animated_Entry;                                    /*  The entire animated Gif structure.      */

typedef struct _Anim_Gif_Task_List
{
    CHAR        task_name[NU_MAX_NAME];
    UINT16      task_number;
    VOID        *task_pointer;
}Gif_Task_List;

#define BRW_HUNDREDTH_SECOND        1

#endif

extern NU_MEMORY_POOL  System_Memory;

#endif /* IMG_INCLUDED */
#endif 



