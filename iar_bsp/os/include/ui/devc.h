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
*  devc.h                                                       
*
* DESCRIPTION
*
*  This table relates the combined devTechs to a specific blit type.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _DEVC_H_
#define _DEVC_H_

/* device manager function codes */

/* enable for use */
#define DMWAKEUP    0   

/* display graphics */
#define DMGRAFIX    1   

/* display text */
#define DMTEXT      2   

/* write palette */
#define DMWPAL      3   

/* read palette */
#define DMRPAL      4   

/* query presence */
#define DMQRY       5   

/* set overscan color */
#define DMOVERSCAN  6   

/* flip page */
#define DMFLIP      7   

/* wait for vertical retrace */
#define DMRETRACE   8   

/* disable for use */
#define DMSHUTDOWN  9   

/* structure of data at devParam for palette functions */
typedef struct _argsPalInfo
{
    /* palette number */
    INT16 palNum;       

    /* begin color index */
    INT16 palBgn;       

    /* end color index */
    INT16 palEnd;       

    /* Padding for the structure */
    INT16 pad;          

    /* pointer to palette data */
    palData *palDataPtr;

} argsPalInfo;

/* VESA Function 01 - Mode Information Block */
typedef struct _VESAInfo
{   
    /* mode attributes */
    INT16 ModeAtt;      

    /* window A attributes */
    UINT8 WinAAtt;      

    /* window B attributes */
    UINT8 WinBAtt;      

    /* window granularity */
    INT16 WinGran;      

    /* window size */
    INT16 WinSize;      
    
    /* window A start segment */
    INT16 WinASeg;      

    /* window B start segment */
    INT16 WinBSeg;      
    
    /* pointer to window function */
    SIGNED WinPtr;      
    
    /* bytes per scan line */
    INT16 BPSL;         

    /* horizontal resolution */
    INT16 XRes;         
    
    /* vertical resolution */
    INT16 YRes;         

    /* character cell width */
    UINT8 XChar;        

    /* character cell height */
    UINT8 YChar;        
    
    /* number of memory planes */
    UINT8 Planes;       

    /* bits per pixel */
    UINT8 Bits;         

    /* number of memory banks */    
    UINT8 Banks;        

    /* memory model type */
    UINT8 Model;        

    /* bank size in kb */
    UINT8 BSize;        

    /* number of image pages */
    UINT8 Pages;        

    /* reserved for page function */
    UINT8 VESARsvd;     
    
    /* pad to 256 bytes */
    UINT8 VESAPad[225]; 

} VESAInfo;

/* function codes for DMRETRACE */
#define statusRETRACE   0
#define enterRETRACE    1
#define enterVIDEO      2
#define inRETRACE       3
#define inVIDEO         4

#endif /* _DEVC_H_ */






