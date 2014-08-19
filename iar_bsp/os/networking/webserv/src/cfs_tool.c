/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*                                                                       
* FILE NAME                                                             
*                                                                       
*       cfs_tool.c                                                
*                                                                       
* COMPONENT                                                             
*         
*       Nucleus WebServ                                                             
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file holds functions that are used in the process of        
*       compressing and decompressing files.                             
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       Totals                          A global array to hold the       
*                                       cumulative totals.               
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       CFS_Compress                    Compresses a file in memory or   
*                                       magnetic media.
*       CFS_Decompress                  Decompresses a file in memory or 
*                                       magnetic media.                  
*       CFS_Fatal_Error                 Prints an Error message to a     
*                                       file.                            
*       CFS_Close_Output_Bit_File       Closes Output Bit File.          
*       CFS_Compress_File               Compresses Input Bit File.       
*       CFS_Open_Input_Bit_File         Open Input Bit file.             
*       CFS_Open_Output_Bit_File        Open the Output Bit file.        
*       CFS_Expand_File                 Expand compressed file.          
*       CFS_Build_Model                 Builds the compressed file model.
*       CFS_Init_Arithmetic_Encoder     Initializes the state of the     
*                                       Arithmetic Encoder.              
*       CFS_Convert_Int_To_Symbol       Converts an integer count into a 
*                                       symbol structure.                
*       CFS_Encode_Symbol               Encodes a symbol for file        
*                                       compression.                     
*       CFS_Flush_Arithmetic_Encoder    Flushes Arithmetic encoder.      
*       CFS_Output_Bits                 Outputs a bit based on a count   
*                                       and a specific code.             
*       CFS_Input_Counts                Reads the number of counts from  
*                                       the input file builds the table  
*                                       of cumulative counts.            
*       CFS_Init_Arithmetic_Decoder     Initializes the state of the     
*                                       Arithmetic Decoder.              
*       CFS_Get_Symbol_Scale            Retrieves symbol scale.          
*       CFS_Get_Current_Count           Retrieves current Count.         
*       CFS_Convert_Symbol_To_Int       Converts symbol to integer.      
*       CFS_Remove_Symbol_From_Stream   Removes a charcter from the input
*                                       stream.                          
*       CFS_Buf_Putc                    Puts a character in a buffer.    
*       CFS_Count_Bytes                 Counts number of bytes.          
*       CFS_Scale_Counts                Scales the number of byte counts.
*       CFS_Output_Counts               Outputs the byte counts.         
*       CFS_Build_Totals                Builds the table of cumulative   
*                                       totals.                          
*       CFS_Buf_Getc                    Gets a character from the input  
*                                       bit_file.                        
*       CFS_Input_Bit                   Reads a bit from an input file   
*                                       stream.                          
*       CFS_Input_Bits                  Inputs a bit based on a count.   
*       CFS_Output_Bit                  Outputs a Specific bit in the    
*                                       file stream.                     
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h           
*                                                                       
************************************************************************/

#include "networking/nu_websr.h"

#if INCLUDE_COMPRESSION



/* The variables in this structure define the current state of the arithmetic
 * coder/decoder.  
 */
typedef struct _CFS_STATE {
    INT32  CFS_Underflow_Bits;           /* Number of underflow bits pending */
    UINT16 CFS_Code;                     /* The present input code value     */
    UINT16 CFS_Low;                      /* Start of the current code range  */
    UINT16 CFS_High;                     /* End of the current code range    */
    UINT8  padding[2];                   /* Maintain quad-word alignment     */
    INT16  CFS_Totals[258];              /* The cumulative totals            */
} CFS_STATE;



/*
 * Internal function prototypes.
 */
STATIC VOID            CFS_Compress_File( CFS_STATE *cfsp, WS_BIT_FILE * buf_input, WS_BIT_FILE * buf_output );
STATIC VOID            CFS_Expand_File(CFS_STATE *cfsp, WS_BIT_FILE *input, WS_BIT_FILE *output);
STATIC VOID            CFS_Open_Input_Bit_File( WS_BIT_FILE *name,CHAR * buf, INT32 length );
STATIC VOID            CFS_Open_Output_Bit_File( WS_BIT_FILE *name, CHAR * buf );
STATIC VOID            CFS_Output_Bit( WS_BIT_FILE *bit_file, INT bit );
STATIC VOID            CFS_Output_Bits( WS_BIT_FILE *bit_file, UINT32 code, INT count );
STATIC INT             CFS_Input_Bit( WS_BIT_FILE *bit_file );
STATIC VOID            CFS_Close_Output_Bit_File( WS_BIT_FILE *bit_file );
STATIC INT             CFS_Buf_Getc( WS_BIT_FILE *bitfile );
STATIC INT             CFS_Buf_Putc( unsigned int out, WS_BIT_FILE *bitfile );
STATIC VOID            CFS_Build_Model(CFS_STATE *cfsp, WS_BIT_FILE *input, WS_BIT_FILE *output);
STATIC VOID            CFS_Scale_Counts( UINT32 counts[], UINT8 scaled_counts[] );
STATIC VOID            CFS_Build_Totals( CFS_STATE *cfsp,  UINT8 scaled_counts[] );
STATIC VOID            CFS_Count_Bytes( WS_BIT_FILE *input, UINT32 counts[] );
STATIC VOID            CFS_Output_Counts( WS_BIT_FILE *output, UINT8 scaled_counts[] );
STATIC VOID            CFS_Input_Counts(CFS_STATE *cfsp, WS_BIT_FILE *input);
STATIC VOID            CFS_Convert_Int_To_Symbol(CFS_STATE *cfsp, INT c, WS_SYMBOL *s);
STATIC VOID            CFS_Get_Symbol_Scale(CFS_STATE *cfsp, WS_SYMBOL *s);
STATIC INT             CFS_Convert_Symbol_To_Int(CFS_STATE *cfsp, INT count, WS_SYMBOL *s);
STATIC VOID            CFS_Init_Arithmetic_Encoder(CFS_STATE *cfsp);
STATIC VOID            CFS_Encode_Symbol(CFS_STATE *cfsp, WS_BIT_FILE *stream, WS_SYMBOL *s);
STATIC VOID            CFS_Flush_Arithmetic_Encoder(CFS_STATE *cfsp, WS_BIT_FILE *stream);
STATIC INT16           CFS_Get_Current_Count(CFS_STATE *cfsp, WS_SYMBOL *s);
STATIC VOID            CFS_Init_Arithmetic_Decoder(CFS_STATE *cfsp, WS_BIT_FILE *stream);
STATIC VOID            CFS_Remove_Symbol_From_Stream(CFS_STATE *cfsp, WS_BIT_FILE *stream, WS_SYMBOL *s);
STATIC VOID            CFS_Fatal_Error( CHAR *s);

#define CFS_END_OF_STREAM 256
#define CFS_PACIFIER_COUNT 2047

#ifndef SEEK_SET
#define SEEK_SET 0
#endif



/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*      CFS_Compress                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      Function used to compress an html, gif, jpeg, or text files. This
*      function compresses the files for the in memory file system and  
*      os dependent file system.                                        
*                                                                       
* INPUTS                                                                
*                                                                       
*      *inbuf                   Pointer to the input buffer where the       
*                               file is coming from. Usually     
*                               memory.                          
*      length                   The length of the data to be     
*                               compressed.                      
*      mode                     Flag that idicates whether       
*                               Not Output or Output             
*      *outbuf                  Pointer to the output buffer where the      
*                               the memory is to be stored.      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      out.length+1             Pointer to next position to be   
*                               compressed.                      
*
************************************************************************/
INT32 CFS_Compress( INT mode, CHAR *inbuf, CHAR *outbuf, INT32 length )
{
    CFS_STATE   cfs_state;
    WS_BIT_FILE out;
    WS_BIT_FILE in;

    CFS_Open_Output_Bit_File( &out, outbuf );
    out.mode = mode;
    CFS_Open_Input_Bit_File( &in, inbuf, length );

    CFS_Compress_File( &cfs_state, &in, &out );
    CFS_Close_Output_Bit_File( &out );

    return( out.length + 1 );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Decompress                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to decompress files resident in the file system.                                        
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *inbuf                  Pointer to the input buffer to be 
*                               decompressed. 
*       *outbuf                 Pointer to the output buffer of 
*                               decompressed file.                            
*       ilen                    Length of the input buffer.      
*       olen                    Length of the output buffer.     
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS
*                                                                       
***********************************************************************/
INT CFS_Decompress(WS_REQUEST *req, CHAR *inbuf, CHAR *outbuf, INT32 inlen)
{
    CFS_STATE   cfs_state;
    WS_BIT_FILE in;
    WS_BIT_FILE out;
    CHAR        buffer[WS_CFS_DECOMP_SZ];

    /* This parameter is saved for possible future use */
    UNUSED_PARAMETER(outbuf);

#if NU_WEBSERV_DEBUG
    printf( "CFS_Decompress(%x %x %d) \n", inbuf, outbuf, inlen );
#endif

    CFS_Open_Output_Bit_File( &out, buffer );
    CFS_Open_Input_Bit_File( &in, inbuf, inlen );

    out.fd = req;
    out.mode = WS_NET_OUTPUT;
    out.length = 0;

    CFS_Expand_File(&cfs_state, &in, &out );
    CFS_Close_Output_Bit_File( &out );

    if(out.mode & WS_NET_OUTPUT)
    {
        if(WSN_Write_To_Net(out.fd, out.fstart, (UINT32)out.length, WS_FILETRNSFR) != NU_SUCCESS)
            NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
    }
    
    return(NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Compress_File                                                     
*                                                                       
* DESCRIPTION                                                           
*        
*                                                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *input                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*       *output                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID  CFS_Compress_File(CFS_STATE *cfsp, WS_BIT_FILE *input, WS_BIT_FILE *output)
{
    INT     c;
    WS_SYMBOL  s;

    CFS_Build_Model(cfsp, input, output);
    CFS_Init_Arithmetic_Encoder(cfsp);

    while ( ( c = CFS_Buf_Getc( input ) ) != EOF )
    {
        CFS_Convert_Int_To_Symbol(cfsp, c, &s );
        CFS_Encode_Symbol(cfsp, output, &s );
    }
    CFS_Convert_Int_To_Symbol(cfsp, CFS_END_OF_STREAM, &s );
    CFS_Encode_Symbol(cfsp, output, &s );
    CFS_Flush_Arithmetic_Encoder(cfsp, output );
    CFS_Output_Bits( output, 0L, 16 );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Expand_File                                                       
*                                                                       
* DESCRIPTION                                                           
*      
*                                                                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       *input                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*       *output                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID  CFS_Expand_File(CFS_STATE *cfsp, WS_BIT_FILE *input, WS_BIT_FILE *output)
{
    WS_SYMBOL  s;
    INT     c;
    INT     count;

    CFS_Input_Counts(cfsp, input);
    CFS_Init_Arithmetic_Decoder(cfsp, input);
    while(1)
    {
        CFS_Get_Symbol_Scale(cfsp, &s);
        count = CFS_Get_Current_Count(cfsp, &s);
        c = CFS_Convert_Symbol_To_Int(cfsp, count, &s);
        if ( c == CFS_END_OF_STREAM )
            break;
        CFS_Remove_Symbol_From_Stream(cfsp, input, &s);
        CFS_Buf_Putc((unsigned int)c, output);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Build_Model                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This is the routine that is called to scan the input file,       
*       scale the counts, build the totals array, then outputs the       
*       scaled counts to the output file.                                                                                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *input                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*       *output                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Build_Model(CFS_STATE *cfsp, WS_BIT_FILE *input, WS_BIT_FILE *output)
{
    UINT32  counts[256];
    UINT8   scaled_counts[256];

    CFS_Count_Bytes(input, counts);
    CFS_Scale_Counts(counts, scaled_counts);
    CFS_Output_Counts(output, scaled_counts);
    CFS_Build_Totals(cfsp, scaled_counts);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Count_Bytes                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine runs through the file and counts the appearances    
*       of each character.                                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *input                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be 
*                               compressed.                      
*       counts                  Number of bytes that are found   
*                               in this segement of the file.    
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Count_Bytes(WS_BIT_FILE *input, UINT32 counts[])
{
    INT i;
    INT c;

    for ( i = 0 ; i < 256 ; i++ )
        counts[ i ] = 0;

    while ( ( c = CFS_Buf_Getc( input )) != EOF )
        counts[ c ]++;

    input->cur_pos = input->fstart;          /* "REWIND" input stream */
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Scale_Counts                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine is called to scale the counts down. There are       
*       two types of scaling that must be done.  First, the counts       
*       need to be scaled down so that the individual counts fit         
*       into a single UINT8. Then, the counts need to be         
*       rescaled so that the total of all counts is less than 16384.     
*                                                                       
* INPUTS                                                                
*                                                                       
*       counts                  Number of bytes in file.         
*       scaled_counts           Scaled Number of bytes in file.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Scale_Counts(UINT32 counts[], UINT8 scaled_counts[])
{
    INT             i;
    UINT32          max_count;
    unsigned int    total;
    UINT32          scale;

    /*
     * The first section of code makes sure each count fits into a single byte.
     */
    max_count = 0;
    for ( i = 0 ; i < 256 ; i++ )
        if ( counts[ i ] > max_count )
            max_count = counts[ i ];
    scale = max_count / 256;
    scale = scale + 1;
    for ( i = 0 ; i < 256 ; i++ )
    {
        scaled_counts[ i ] = (UINT8 ) ( counts[ i ] / scale );
        if ( scaled_counts[ i ] == 0 && counts[ i ] != 0 )
            scaled_counts[ i ] = 1;
    }
    /*
     * This next section makes sure the total is less than 16384.  I initialize
     * the total to 1 instead of 0 because there will be an additional 1 added
     * in for the CFS_END_OF_STREAM symbol;
     */
    total = 1;
    for ( i = 0 ; i < 256 ; i++ )
        total += scaled_counts[ i ];
    if ( total > ( 32767 - 256 ) )
        scale = 4;
    else if ( total > 16383 )
        scale = 2;
    else
        return;
    for ( i = 0 ; i < 256 ; i++ )
        scaled_counts[ i ] = (UINT8)(scaled_counts[ i ] / (UINT8)scale);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Build_Totals                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine is used by both the encoder and decoder to          
*       build the table of cumulative totals. The counts for the         
*       characters in the file are in the counts array, and we know      
*       that there will be a single instance of the EOF symbol.          
*                                                                       
* INPUTS                                                                
*                                                                       
*       scaled_counts           Scaled Number of bytes in file.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Build_Totals(CFS_STATE *cfsp, UINT8 scaled_counts[])
{
    INT i;
    
    cfsp->CFS_Totals[0] = 0;
    for ( i = 0 ; i < CFS_END_OF_STREAM ; i++ )
        cfsp->CFS_Totals[i + 1] = (INT16)(cfsp->CFS_Totals[i] + (INT16)scaled_counts[i]);
    cfsp->CFS_Totals[CFS_END_OF_STREAM + 1] = (INT16)(cfsp->CFS_Totals[CFS_END_OF_STREAM] + 1);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Output_Counts                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       In order for the compressor to build the same model, I have      
*       to store the symbol counts in the compressed file so the         
*       expander can read them in.  In order to save space, I don't      
*       save all 256 symbols unconditionally. The format used to store   
*       counts looks like this:                                          
*                                                                       
*       start, stop, counts, start, stop, counts, ... 0                  
*                                                                       
*       This means that I store runs of counts, until all the            
*       non-zero counts have been stored.  At this time the list is      
*       terminated by storing a start value of 0.  Note that at least    
*       1 run of counts has to be stored, so even if the first start     
*       value is 0, I read it in. It also means that even in an empty    
*       file that has no counts, I have to pass at least one count.      
*                                                                       
*       In order to efficiently use this format, I have to identify      
*       runs of non-zero counts.  Because of the format used, I don't    
*       want to stop a run because of just one or two zeros in the count 
*       stream. So I have to sit in a loop looking for strings of three  
*       or more zero values in a row.                                    
*                                                                       
*       This is simple in concept, but it ends up being one of the most  
*       complicated routines in the whole program.  A routine that just  
*       writes out 256 values without attempting to optimize would be    
*       much simpler, but would hurt compression quite a bit on small    
*       files.                                                           
*                                                                       
* INPUTS                                                                
*                                                                       
*       output                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                      
*       scaled_counts           Scaled Number of bytes in file.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Output_Counts(WS_BIT_FILE *output, UINT8 scaled_counts[])
{
    INT first;
    INT last;
    INT next;
    INT i;

    first = 0;
    while ( first < 255 && scaled_counts[ first ] == 0 )
        first++;
    /*
     * Each time I hit the start of the loop, I assume that first is the
     * number for a run of non-zero values.  The rest of the loop is
     * concerned with finding the value for last, which is the end of the
     * run, and the value of next, which is the start of the next run.
     * At the end of the loop, I assign next to first, so it starts in on
     * the next run.
     */
    for ( ; first < 256 ; first = next )
    {
        last = first + 1;
        while(1)
        {
            for ( ; last < 256 ; last++ )
                if ( scaled_counts[ last ] == 0 )
                    break;
            last--;
            for ( next = last + 1; next < 256 ; next++ )
                if ( scaled_counts[ next ] != 0 )
                    break;
            if ( next > 255 )
                break;
            if ( ( next - last ) > 3 )
                break;
            last = next;
        }
        /*
         * Here is where I output first, last, and all the counts in between.
         */
        if ( CFS_Buf_Putc((unsigned int)first, output) != first )
            CFS_Fatal_Error( "Error writing byte counts\n" );
        if ( CFS_Buf_Putc( (unsigned int)last, output ) != last )
            CFS_Fatal_Error( "Error writing byte counts\n" );
        for ( i = first ; i <= last ; i++ ) {
            if ( CFS_Buf_Putc( (unsigned int)scaled_counts[ i ], output ) !=
                (INT) scaled_counts[ i ] )
                CFS_Fatal_Error( "Error writing byte counts\n" );
        }
    }
    if ( CFS_Buf_Putc( 0, output ) != 0 )
        CFS_Fatal_Error( "Error writing byte counts\n" );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Input_Counts                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When expanding, I have to read in the same set of counts.        
*       This is quite a bit easier that the process of writing           
*       them out, since no decision making needs to be done. All         
*       I do is read in first, check to see if I am all done, and        
*       if not, read in last and a string of counts.                     
*                                                                       
* INPUTS                                                                
*                                                                       
*       *input                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
************************************************************************/
STATIC VOID CFS_Input_Counts(CFS_STATE *cfsp, WS_BIT_FILE *input)
{
    INT     first;
    INT     last;
    INT     i;
    INT     c;
    UINT8   scaled_counts[256];
    
    for ( i = 0 ; i < 256 ; i++ )
        scaled_counts[ i ] = 0;
    if ( ( first = CFS_Buf_Getc( input ) ) == EOF )
        CFS_Fatal_Error( "Error reading byte counts\n" );
    if ( ( last = CFS_Buf_Getc( input ) ) == EOF )
        CFS_Fatal_Error( "Error reading byte counts\n" );
    while(1)
    {
        for ( i = first ; i <= last && i >= 0; i++ )
            if ( ( c = CFS_Buf_Getc( input ) ) == EOF )
                CFS_Fatal_Error( "Error reading byte counts\n" );
            else
                scaled_counts[ i ] = ( UINT8 )c;
            
        if ( ( first = CFS_Buf_Getc( input ) ) == EOF )
            CFS_Fatal_Error( "Error reading byte counts\n" );
        if ( first == 0 )
            break;
        if ( ( last = CFS_Buf_Getc( input ) ) == EOF )
            CFS_Fatal_Error( "Error reading byte counts\n" );
    }
    CFS_Build_Totals( cfsp, scaled_counts );
}

/*
 * Everything from here down define the arithmetic coder section
 * of the program.
 */

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Init_Arithmetic_Encoder                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine must be called to initialize the encoding           
*       process. The high register is initialized to all 1s, and         
*       it is assumed that it has an infinite string of 1s to be         
*       shifted into the lower bit positions when needed.                
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
STATIC VOID CFS_Init_Arithmetic_Encoder(CFS_STATE *cfsp)
{
    cfsp->CFS_Low = 0;
    cfsp->CFS_High = 0xffff;
    cfsp->CFS_Underflow_Bits = 0;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Flush_Arithmetic_Encoder                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       At the end of the encoding process, there are still              
*       significant bits left in the high and low registers.             
*       We output two bits, plus as many underflow bits as               
*       are necessary.                                                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *stream                  Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                                                             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Flush_Arithmetic_Encoder(CFS_STATE *cfsp, WS_BIT_FILE *stream)
{
    CFS_Output_Bit( stream, (INT)(cfsp->CFS_Low & 0x4000) );
    cfsp->CFS_Underflow_Bits++;
    while ( cfsp->CFS_Underflow_Bits-- > 0 )
        CFS_Output_Bit( stream, (INT)(~cfsp->CFS_Low & 0x4000) );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Convert_Int_To_Symbol                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Finding the low count, high count, and scale for                 
*       a symbol is really easy, because of the way the totals           
*       are stored. This is the one redeeming feature of the             
*       data structure used in this implementation.                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       c                       Integer to be converted.         
*       *s                      Pointer to the SYMBOL structure  
*                               that contains the converted      
*                               symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Convert_Int_To_Symbol(CFS_STATE *cfsp, INT c, WS_SYMBOL *s)
{
    s->scale = cfsp->CFS_Totals[ CFS_END_OF_STREAM + 1 ];
    s->low_count = cfsp->CFS_Totals[ c ];
    s->high_count = cfsp->CFS_Totals[ c + 1 ];
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Get_Symbol_Scale                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Getting the scale for the current context is easy.               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                          Pointer to the SYMBOL structure  
*                                   that contains the converted      
*                                   symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Get_Symbol_Scale(CFS_STATE *cfsp, WS_SYMBOL *s)
{
    s->scale = cfsp->CFS_Totals[ CFS_END_OF_STREAM + 1 ];
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Convert_Symbol_To_Int                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       During decompression, we have to search through the              
*       table until we find the symbol that straddles the                
*       "count" parameter. When it is found, it is returned.             
*       The reason for also setting the high count and low count         
*       is so that symbol can be properly removed from the               
*       arithmetic coded input.                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       count                   The current count.               
*       *s                      Pointer to the SYMBOL structure  
*                               that contains the converted      
*                               symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       c                       Integer form of converted symbol.
*                                                                       
*************************************************************************/
STATIC INT CFS_Convert_Symbol_To_Int(CFS_STATE *cfsp, INT count, WS_SYMBOL *s)
{
    INT c;

    for ( c = CFS_END_OF_STREAM ; count < cfsp->CFS_Totals[ c ] ; c-- );
    s->high_count = cfsp->CFS_Totals[ c + 1 ];
    s->low_count = cfsp->CFS_Totals[ c ];
    return( c );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Encode_Symbol                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine is called to encode a symbol. The symbol            
*       is passed in the SYMBOL structure as a low count, a              
*       high count, and a range, instead of the more conventional        
*       probability ranges. The encoding process takes two steps.        
*       First, the values of high and low are updated to take            
*       into account the range restriction created by the  new           
*       symbol. Then, as many bits as possible are shifted out to        
*       the output stream.  Finally, high and low are stable again       
*       and the routine returns.                                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       *stream                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the output file to be 
*                               compressed.                      
*       *s                      Pointer to the SYMBOL structure  
*                               that contains the converted      
*                               symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Encode_Symbol(CFS_STATE *cfsp, WS_BIT_FILE *stream, WS_SYMBOL *s)
{
    INT32 range;
    /*
     * These three lines rescale high and low for the new symbol.
     */
    range = (INT32) ( cfsp->CFS_High - cfsp->CFS_Low ) + 1;
    cfsp->CFS_High = (UINT16)(cfsp->CFS_Low + (UINT16)((range * s->high_count) / s->scale - 1));
    cfsp->CFS_Low = (UINT16)(cfsp->CFS_Low + (UINT16)((range * s->low_count) / s->scale));
    /*
     * This loop turns out new bits until high and low are far enough
     * apart to have stabilized.
     */
    while(1)
    {
    /*
     * If this test passes, it means that the MSDigits match, and can
     * be sent to the output stream.
     */
        if ( ( cfsp->CFS_High & 0x8000 ) == ( cfsp->CFS_Low & 0x8000 ) )
        {
            CFS_Output_Bit( stream, (INT)(cfsp->CFS_High & 0x8000) );
            while ( cfsp->CFS_Underflow_Bits > 0 )
            {
                CFS_Output_Bit( stream, (INT)(~cfsp->CFS_High & 0x8000) );
                cfsp->CFS_Underflow_Bits--;
            }
        }
        /*
         * If this test passes, the numbers are in danger of underflow, because
         * the MSDigits don't match, and the 2nd digits are just one apart.
         */
        else if ( ( cfsp->CFS_Low & 0x4000 ) && !( cfsp->CFS_High & 0x4000 ))
        {
            cfsp->CFS_Underflow_Bits += 1;
            cfsp->CFS_Low &= 0x3fff;
            cfsp->CFS_High |= 0x4000;
        } else
            return ;
        cfsp->CFS_Low <<= 1;
        cfsp->CFS_High <<= 1;
        cfsp->CFS_High |= 1;
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Get_Current_Count                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When decoding, this routine is called to figure out              
*       which symbol is presently waiting to be decoded. This            
*       routine expects to get the current model scale in the            
*       s->scale parameter, and it returns  a count that                 
*       corresponds to the present floating point code:                  
*                                                                       
*       code = count / s->scale                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Pointer to the SYMBOL structure  
*                               that contains the converted      
*                               symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       count                   Current Count.                   
*                                                                       
*************************************************************************/
STATIC INT16 CFS_Get_Current_Count(CFS_STATE *cfsp, WS_SYMBOL *s)
{
    INT32   range;
    INT16   count;

    range = (INT32) ( cfsp->CFS_High - cfsp->CFS_Low ) + 1;
    count = (INT16)
        ((((INT32) ( cfsp->CFS_Code - cfsp->CFS_Low ) + 1 ) * s->scale - 1 ) / range );
    return( count );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Init_Arithmetic_Decoder                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine is called to initialize the state                  
*       of the arithmetic decoder.  This involves initializing          
*       the high and low registers to their conventional                
*       starting values, plus reading the first 16 bits from            
*       the input stream into the code value.                           
*                                                                       
* INPUTS                                                                
*                                                                       
*       *stream                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Init_Arithmetic_Decoder(CFS_STATE *cfsp, WS_BIT_FILE *stream)
{
    INT i;

    cfsp->CFS_Code = 0;
    for ( i = 0 ; i < 16 ; i++ )
    {
        cfsp->CFS_Code <<= 1;
        cfsp->CFS_Code = (UINT16)(cfsp->CFS_Code + (CFS_Input_Bit(stream)));
    }
    cfsp->CFS_Low = 0;
    cfsp->CFS_High = 0xffff;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Remove_Symbol_From_Stream                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Just figuring out what the present symbol is                     
*       doesn't remove it from the input bit stream. After               
*       the character has been decoded, this routine has to              
*       be called to remove it from the input stream.                    
*                                                                       
* INPUTS                                                                
*                                                                       
*       *stream                 Pointer to BIT_FILE structure.   
*                               This structure holds all the     
*                               related to the input file to be  
*                               compressed.                      
*       *s                      Pointer to the SYMBOL structure  
*                               that contains the converted      
*                               symbol information.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Remove_Symbol_From_Stream(CFS_STATE *cfsp, WS_BIT_FILE *stream, WS_SYMBOL *s)
{
    INT32 range;
    
    /*
     * First, the range is expanded to account for the symbol removal.
     */
    range = (INT32)( cfsp->CFS_High - cfsp->CFS_Low ) + 1;
    cfsp->CFS_High = (UINT16)(cfsp->CFS_Low + (UINT16)((range * s->high_count) / s->scale - 1));
    cfsp->CFS_Low = (UINT16)(cfsp->CFS_Low + (UINT16)((range * s->low_count) / s->scale));
    /*
     * Next, any possible bits are shipped out.
     */
    while(1)
    {
    /*
    * If the MSDigits match, the bits will be shifted out.
        */
        if ( ( cfsp->CFS_High & 0x8000 ) == ( cfsp->CFS_Low & 0x8000 ) )
        {
        }
        /*
        * Else, if underflow is threatening, shift out the 2nd MSDigit.
        */
        else if ( (cfsp->CFS_Low & 0x4000) == 0x4000  && (cfsp->CFS_High & 0x4000) == 0 )
        {
            cfsp->CFS_Code ^= 0x4000;
            cfsp->CFS_Low   &= 0x3fff;
            cfsp->CFS_High  |= 0x4000;
        } else
        /*
        * Otherwise, nothing can be shifted out, so I return.
        */
            return;
        
        cfsp->CFS_Low <<= 1;
        cfsp->CFS_High <<= 1;
        cfsp->CFS_High |= 1;
        cfsp->CFS_Code <<= 1;
        cfsp->CFS_Code = (UINT16)(cfsp->CFS_Code + CFS_Input_Bit(stream));
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Open_Output_Bit_File                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function initializes the Output Bit File that is to be      
*       compressed or decompressed.                                                                                                     
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that holds all 
*                               information about the file.                        
*       *buf                    Output Bit Buffer to be opened.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Open_Output_Bit_File( WS_BIT_FILE * bit_file, CHAR *buf )
{
    bit_file->fstart = buf;
    bit_file->cur_pos = buf;
    bit_file->length = 0;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Open_Input_Bit_File                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function initializes the Input Bit File that is to be       
*       compressed or decompressed.                                                                                                     
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that        
*                               holds all information about      
*                               the file.                        
*       *buf                    Input Bit Buffer to be opened.   
*       length                  Input Bit Buffer length.         
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS
*                                                                       
*************************************************************************/
STATIC VOID CFS_Open_Input_Bit_File( WS_BIT_FILE *bit_file, CHAR *buf, INT32 length)
{
    bit_file->fstart = buf;
    bit_file->cur_pos = buf;
    bit_file->length = length;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Close_Output_Bit_File                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function that closes the bit file and then flushes the buffer.                                                                
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that holds all 
*                               information about the file.                        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Close_Output_Bit_File(WS_BIT_FILE *bit_file)
{
    if ( bit_file->mask != 0x80 )
        if ( CFS_Buf_Putc( (unsigned int)bit_file->rack, bit_file ) != bit_file->rack )
            CFS_Fatal_Error( "Fatal error in CFS_Close_Output_Bit_File!\n" );
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Output_Bit                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to output a specific bit into the file stream.          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bit_file               Pointer to structure that holds all 
*                               information about the file.                        
*       bit                     Bit to be outputted              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Output_Bit(WS_BIT_FILE *bit_file, INT bit)
{
    if ( bit )
        bit_file->rack |= bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
    {
        if ( CFS_Buf_Putc( (unsigned int)bit_file->rack, bit_file ) != bit_file->rack )
            CFS_Fatal_Error( "Fatal error in CFS_Output_Bit!\n" );
        else
            bit_file->pacifier_counter++;

        bit_file->rack = 0;
        bit_file->mask = 0x80;
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Output_Bits                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to output a code a certain number of counts. It also    
*       checks to make sure the value that is outputted is correct.  If  
*       it is not then write to file an error message.                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that holds all 
*                               information about the file.                        
*       code                    A flag used in the selection of  
*                               how the data is output.              
*       count                   Represents the number of         
*                               left shifts when building the    
*                               mask.                            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Output_Bits(WS_BIT_FILE *bit_file, UINT32 code, INT count)
{
    UINT32 mask;

    mask = 1L << ( count - 1 );
    while ( mask != 0)
    {
    if ( mask & code )
        bit_file->rack |= bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
    {
        if ( CFS_Buf_Putc( (unsigned int)bit_file->rack, bit_file ) != bit_file->rack )
            CFS_Fatal_Error( "Fatal error in CFS_Output_Bits!\n" );
        else 
            bit_file->pacifier_counter++;

        bit_file->rack = 0;
        bit_file->mask = 0x80;
    }
    mask >>= 1;
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Input_Bit                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function gets a bit from an input file stream. It returns   
*       one or zero depending on the bit value.                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that        
*                               holds all information about      
*                               the file.                        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       value                   Holds the bit value.             
*                                                                       
*************************************************************************/
STATIC INT CFS_Input_Bit(WS_BIT_FILE *bit_file)
{
    INT value;
    
    if ( bit_file->mask == 0x80 )
    {
        bit_file->rack = CFS_Buf_Getc( bit_file );
        if ( bit_file->rack == EOF )
            CFS_Fatal_Error( "Fatal error in CFS_Input_Bit!\n" );
        bit_file->pacifier_counter++;
    }
    value = bit_file->rack & bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
        bit_file->mask = 0x80;
    return( value ? 1 : 0 );
}

#if 0
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Input_Bits                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to input 32 bits from an input file.  It returns a 32   
*       bit value.  It is currently not used. It is here for future      
*       updates.                                                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that        
*                               holds all information about      
*                               the file.                        
*       bit_count               A specific count used for        
*                               creating the mask.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       return_value            32 bit value.                    
*                                                                       
*************************************************************************/
STATIC UINT32 CFS_Input_Bits(WS_BIT_FILE *bit_file, INT bit_count)
{
    UINT32 mask;
    UINT32 return_value;

    mask = 1L << ( bit_count - 1 );
    return_value = 0;
    while ( mask != 0)
    {
        if ( bit_file->mask == 0x80 )
        {
            bit_file->rack = CFS_Buf_Getc( bit_file );
            if ( bit_file->rack == EOF )
                CFS_Fatal_Error( "Fatal error in InputBit!" );
            if ( ( bit_file->pacifier_counter++ & CFS_PACIFIER_COUNT ) == 0 );
            /* null statement */
        }
        if ( bit_file->rack & bit_file->mask )
            return_value |= mask;
        mask >>= 1;
        bit_file->mask >>= 1;
        if ( bit_file->mask == 0 )
            bit_file->mask = 0x80;
    }
    return( return_value );
}
#endif /* CFS_Input_Bits not used */

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Buf_Putc                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function is used to take a character in memory and put the  
*       the data in a buffer. Then based on the mode it either puts the  
*       value in a string or it transmits the character over the network.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that        
*                               holds all information about      
*                               the file.                                                               
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       out                     String that the character was    
*                               placed in.                       
*
*************************************************************************/
STATIC INT CFS_Buf_Putc( unsigned int out, WS_BIT_FILE *bitfile )
{
    if(bitfile->mode  & WS_NET_OUTPUT)
    {
        if(bitfile->length >= WS_CFS_DECOMP_SZ)
        {
            if(WSN_Write_To_Net(bitfile->fd, bitfile->fstart, 
                                (UINT32)bitfile->length, WS_FILETRNSFR) != NU_SUCCESS)
                NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
            
            bitfile->cur_pos = bitfile->fstart;
            bitfile->length = 0;
        }
        
        *(bitfile->cur_pos) = (CHAR)out;
    }
    else if((bitfile->mode & WS_DONT_OUTPUT) == 0)
    {
        *( bitfile->cur_pos ) = (( CHAR )out);
    }
    
    bitfile->cur_pos++;
    bitfile->length++;
    
    return(out);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Buf_Getc                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function that gets a character from an input bitfile. It checks  
*       if the character is equal to EOF.  If it is not then it returns  
*       the character position to the calling routine.                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *bitfile                Pointer to structure that        
*                               holds all information about      
*                               the file.                        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       EOF                     EOF character                    
*       i                       Indicates that a character was   
*                               found and gives position.        
*                                                                       
*************************************************************************/
STATIC INT CFS_Buf_Getc( WS_BIT_FILE *bitfile )
{
    INT i;
    
    if(( bitfile->cur_pos - bitfile->fstart ) > (( bitfile->length )) - 1 )
    {
        return( EOF);
    }
    
    i = *(bitfile->cur_pos) & 0xff;
    bitfile->cur_pos++;
    
    return(i);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       CFS_Fatal_Error                                                      
*                                                                       
* DESCRIPTION 
*                                                         
*       Function to printf error message to file during creation of      
*       of in-memory file system(pkmfs).                                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s                      Holds the message                
*                               that is to be printed to the     
*                               file.                                                                           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID CFS_Fatal_Error( CHAR *s)
{   
    /*  remove warnings */
    UNUSED_PARAMETER(s);

#if NU_WEBSERV_DEBUG
    printf("Bit error %s\n", s);
#endif

}

#endif /* INCLUDE_COMPRESSION */
