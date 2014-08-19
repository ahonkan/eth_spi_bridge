/***************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME
*
*  blit_specific.h
*
* DESCRIPTION
*
*  Source and destination blit codes.
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
#ifndef _BLIT_SPECIFIC_H_
#define _BLIT_SPECIFIC_H_

#define self2self   0 * 4 /* 0  */
#define mono2self   1 * 4 /* 4  */
#define mem2self    2 * 4 /* 8  */
#define self2mem    3 * 4 /* 12 */
#define bliterr     0xFF  /* Type Dependent */

UINT8 blitxfertbl[208] = {

/* Dest = 00h - User Supplied    Source = */
        self2self,  /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        mem2self,   /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        mem2self,   /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

 /* Dest = 01h - Monochrome  Source = */
        bliterr,    /* 00h - User supplied Primitives */
        self2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 02h - Multi-bit         Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        self2self,  /* 02h - Multi-bit */
        mem2self,   /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        mem2self,   /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 03h - MultiPlane   Source = */
        self2mem,   /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        self2mem,   /* 02h - Multi-bit */
        self2self,  /* 03h - MultiPlane */
        self2mem,   /* 04h - EGA */
        self2mem,   /* 05h - VGA */
        self2mem,   /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 04h - EGA          Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        mem2self,   /* 03h - MultiPlane */
        self2self,  /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 05h - VGA          Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        mem2self,   /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        self2self,  /* 05h - VGA */
        self2self,  /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest =  06h - VGA segmented   Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        mem2self,   /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        self2self,  /* 05h - VGA */
        self2self,  /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 07h - 8514/A       Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        self2self,  /* 07h - 8514/A */
        mem2self,   /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 08h - 8-Bit        Source = */
        self2mem,   /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        self2mem,   /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        self2mem,   /* 07h - 8514/A */
        self2self,  /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 09h - 16-Bit       Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        self2self,  /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        self2mem,   /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 0ah - 24-Bit       Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        self2self,  /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        self2mem,   /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 0bh - 16-Bit       Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        mem2self,   /* 09h - 16-bit, 64K-color */
        bliterr,    /* 0ah - 24-bit, 16M-color */
        self2self,  /* 0bh - 16-bit, 64K-color */
        bliterr,    /* 0ch - 24-bit, 16M-color */
        0,0,0,

/* Dest = 0ch - 24-Bit       Source = */
        bliterr,    /* 00h - User supplied Primitives */
        mono2self,  /* 01h - Monochrome */
        bliterr,    /* 02h - Multi-bit */
        bliterr,    /* 03h - MultiPlane */
        bliterr,    /* 04h - EGA */
        bliterr,    /* 05h - VGA */
        bliterr,    /* 06h - VGA segmented */
        bliterr,    /* 07h - 8514/A */
        bliterr,    /* 08h - 8-Bit */
        bliterr,    /* 09h - 16-bit, 64K-color */
        mem2self,   /* 0ah - 24-bit, 16M-color */
        bliterr,    /* 0bh - 16-bit, 64K-color */
        self2self,  /* 0ch - 24-bit, 16M-color */
        0,0,0};

#endif /* _BLIT_SPECIFIC_H_ */

