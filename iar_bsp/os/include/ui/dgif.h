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
* FILE NAME                                                             
*                                                                      
*       dgif.h                                                           
*                                                                      
* COMPONENT                                                            
*                                                                      
*       GIF - part of GIF workspace                                     
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*       Holds defines, prototypes and data structure to handle the GIF module.                             
*                                                                      
* DATA STRUCTURES                                                      
*                                                                      
*        GIF_FILE_DATA    - holds the GIF file data
*        Gif_Info         - holds the GIF information
*        Gif89A           - Animated GIF information          
*                                                                                                                                           
* DEPENDENCIES
*
*       image/inc/bagif.h                                                        
*                                                                      
******************************************************************************/
#ifndef DGIF_H
#define DGIF_H

#include "ui/bagif.h"

#ifdef IMG_INCLUDED

/* Browser Monochrome Foreground Color */
#define BRW_MONOCHROME_FCOLOR              Black  


typedef struct GIF_FILE_DATA_STRUCT
{
    char    *file_buffer;       /*  Holds the Gif file buffer */    
    INT32   total_bytes;        /*  Total size of the Gif file to be decoded */
    INT32   bytes_remaining;    /*  The number of bytes remaining after a part of the buffer has been read */
    INT32   current_byte;       /*  Current byte of the buffer pointer */
    char    *offset_ptr;        /*  Temporary pointer to offset the Gif to certain location within the buffer */
} GIF_FILE_DATA;


/*  Macros for rounding a number to nearest 4 byte boundary */
#define IsBitSet(x, b)      (((x) & (b)) == (b))


/*  Miscellaneous defines */
#define     MAX_LZW_BITS        12

#define IS_INTERLACE            0x40
#define IS_LOCALCOLORMAP        0x80
#define IS_GLOBALCOLORMAP       0x80

#define GET_UNSIGNI(q)  ((((q)[1])<<8)|(q)[0])

/*  Gif Data Structure that holds information about the Gif images          */
struct Gif_Info_Struct{
       unsigned int    Background;        /*  Background Color    */
       int             gif_Width;         /*  Gif Width           */
       int             gif_Height;        /*  Gif Height          */
       unsigned int    Bits_Per_Pixel;    /*  Bits Per Pixel      */
       unsigned int    Color_Resol;       /*  Color Resolution    */
       unsigned int    Aspect_Ratio;      /*  Aspect Ratio        */
       int             Gray_scale;        /*  Is it Gray Scale?   */
       int             Num_Images;        /*  Number of Images    */
        
};


/*  Data Structure Specific to 89A gifs */
struct Gif89A_Struct
{
       int     transparent_gif_color_index;     /*  Is it a Transparent Gif                  */
       int     delayTime;                       /*  What is the Delay Time to the next image */
       int     inputFlag;                       /*  Input Flag                               */
       int     disposal;                        /*  Disposal of last image                   */
       int     transparent;                     /*  The Image is TransParent */
};

/*  The main entry function into the gif decoder */
int decompress_gif(ioSTRUCT *thisPage);

/* Internal Function Prototypes */
int GIF_Read_Info(ioSTRUCT *thisPage);
int GIF_Get_Color_Map ( int number_entries);
void GIF_Process_Extension_Character( int extension_label );
int GIF_Get_Block_of_Data( unsigned char  *buf );
int GIF_Get_Code_From_Data ( int code_size, int initialize );
int GIF_Read_Input_Buffer(unsigned char *buffer,int data_amount);
int GIF_LZW_Byte_Read( int initialize, int input_code_size );
int GIF_Image_Decode( int width_of_image, int height_of_image, int gray_scale, int image_interlaced, int ignore_image );
int GIF_GetColorCount( unsigned char flags );


/* Defines for Errors received regarding GIFS */
#define GIF_ERROR_BAD_FILE           -1
#define GIF_ERROR_BAD_TYPE           -2
#define GIF_ERROR_INPUT_NOT_EXPECTED -3
#define GIF_COLOR_MAP_ERROR          -4
#define GIF_OVERFLOW                 -5
#define GIF_DONE_LZW                 -2
#define GIF_BAD_CODE                 -6
#define GIF_LZW_FAIL                 -7
#define GIF_OUT_OF_MEMORY            -8
#define GIF_IMAGE_IGNORED            -9

/* the GIF header */ 
#define GIF_HEAD                    "GIF"

/* Const macros for GIF ease of use */
#define GIF_START                   ','
#define GIF_TERMINATE               ';'
#define GIF_EXTENSION               '!'

/* GIF Header Extension MACROS */
#define GIF_PLAIN_TEXT_EXT          0x01
#define GIF_APP_EXT                 0xff
#define GIF_COMMENT_EXT             0xfe
#define GIF_GRAPHIC_CTRL_EXT        0xf9

#endif /* IMG_INCLUDED */
#endif /* DGIF_H */

