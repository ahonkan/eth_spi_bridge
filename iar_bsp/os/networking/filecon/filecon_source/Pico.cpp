// Pico.cpp : implementation file
//

#include "stdafx.h"
#include "fileconvert.h"
#include "Pico.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPico

CPico::CPico()
{

    underflow_bits = 0;
    file_map       = 0;
    con_fil        = 0;
    con_file_ptr   = 0;
    con_file_size  = 0;
    out            = 0;
    m_ProgressCtrl = 0;
    embedded_files = 0;
    m_Compress     = FALSE;
    m_NumFiles     = 0;

}

CPico::~CPico()
{
}


//This code was taken from Pico Server Code and modified
BOOL CPico::Generate_C_File(char *input, char *output, CProgressCtrl *ProgressCtrl) 
{
    CString message;

    FILE * F;
    struct ps_line *pl;
    int cs;


    m_ProgressCtrl = ProgressCtrl;

    status.SpoolMessage("Generating C File...");
    
    out = fopen(output,"wb");
    if( out == NULL ){

        status.SpoolMessage("Unable to open output file");
        status.SpoolMessage("C file not generated!");
        return FALSE;
    }

    F = fopen(input ,"rb");
    if( F == NULL ){
        status.SpoolMessage("Unable to open temporary transaction file!");
        status.SpoolMessage("C file not generated!");
        return FALSE;
    }

    cs = (int)file_size(input);
    con_file_size = cs;
    con_fil = (char *)malloc((unsigned long)(cs+1));
    con_file_ptr = con_fil;

    fread(con_fil,1,cs,F);      /* read in configuration file */
    parse_con(con_fil,cs);

    if (m_ProgressCtrl)
    {
        m_ProgressCtrl->ShowWindow(TRUE);
        m_ProgressCtrl->SetRange(0, m_NumFiles);
        m_ProgressCtrl->SetStep(1);
        m_ProgressCtrl->SetPos(0);
    }
    
    pl = file_map;

    check_file_names();
    output_header(output);      /* start generationg file */

    make_file_data();   /* data define each file */
    make_fs_table();    /* file table */
    output_footer(output);      /* output the footer */
    fclose(out);


    status.SpoolMessage("-------------------------------------------------------------");
    message.Format("%s generated.", output);
    status.SpoolMessage(message);

    if (m_ProgressCtrl)
        status.DoModal();

    if (m_ProgressCtrl)
        m_ProgressCtrl->ShowWindow(FALSE);

    return TRUE;
}

///////////////////////////////////Pico Server Code//////////////////////////////////////////////
/* ps_mkfs.c
 *
 *  THIS PROGRAM IS NOT A PART OF THE 
 *  IMBEDDED PICO-WEBSERVER
 *
 * This program is used to create an
 * imbedded file system for the PicoServer
 * imbedded web server.
 *
 * We read a data file (see below)
 * and generate a C file that is
 * compiled with the Picoserve
 * 
 * If compression is enabled all input files are compressed
 * before being converted to C structures
 * in a format that the Picoserver can understand.
 *
 * File Compression is enabled/disabled  in this porgram
 * with the FILE_COMPRESSION define in ../ps_conf.h
 * 
 * NOTE: that booth this program and the Picoserver
 * need to compiled with the same ps_conf.h and ps_pico.h
 * file to guarentee interoperation.
 *
 * Usage: ps_mkfs data_file [out_file]
 */

/*
 *  # We read a data file of the form:
 *  
 *  /native/path/name-0     /embeded/path/name-0
 *  .    .       .            .   .    .
 *  /native/path/name-n     /embeded/path/name-n
 *
 *  # A line begining with '#' is ignored 
 */


/* parse the data file and make a linked
 * list of native-embedded file mappings 
 */

static char basebuf[256];
static unsigned short int code;  /* The present input code value       */
static unsigned short int low;   /* Start of the current code range    */
static unsigned short int high;  /* End of the current code range      */

void CPico::parse_con(char * conbuf, int size)
{
    char * cs;
    struct ps_line * psl;
    struct ps_line * psm;
    struct ps_line * last;
    char  *buf;

    cs =conbuf;

    /// calloc used wrong here : calloc(number, size)
    file_map = (struct ps_line *)calloc(sizeof( struct ps_line),1);
    psl=file_map;

    /// Loop fills in linked list containing info about each file
    while( buf =(char*)get_line()  ){

        m_NumFiles++;

        if( buf[0] == '#' )continue;
      if( buf[0] == '*' )
      {
          if(strncmp(buf, "*secure=", 8) == 0)
          {
              if(buf[8] !='/')
              {
                  theApp.m_PrivateDirectory = "/";
                  theApp.m_PrivateDirectory += (&buf[8]);
              }
              else
                  theApp.m_PrivateDirectory = (&buf[8]);
          }
          
          continue;
      }

        psl->native =buf;

        /*CHANGED FOR LONG FILENAME SUPPORT*/
        /*while( (*buf != '\t') && (*buf != ' ') )*/
        while( (*buf != '\t') && (*buf != '@') )
            buf++;

        *buf++ = '\0';

        /*while( (*buf != '\t') && (*buf != ' ') )
            buf++;*/

        psl->picopat = buf;

        
        while ((*buf != '\t') && (*buf != '@'))
            buf++;
        *buf++ = '\0';
  
        psl->compress = buf;
        
        while ((*buf != '\t') && (*buf != '@'))
            buf++;
        *buf++ = '\0';
        
        psl->secure = buf;   
        
        while( *buf && (*buf != '\r') && (*buf != '\n') )
            buf++;

        *buf++ ='\0';


        psm=(struct ps_line*)calloc(sizeof( struct ps_line), 1);
        psl->next = psm;
        psm->next = NULL;
        last = psl;
        psl = psm;
        

    }

    last->next = NULL;
}

/* get the next line in the input buffer */

char *CPico::get_line()
{
    char * cf;
    char * r;

    /// Are we pointing past the file?
    if( con_file_ptr >= (con_fil + con_file_size ) )
        return(NULL);

    /// Set up some pointers to the begining of the line
    r = cf = con_file_ptr;

    /// Look for line feed or end line delimeters
    while( (*cf != '\r') && (*cf !='\n') )
            *cf++;

    /// NULL terminate character string
    *cf = '\0';
    cf++;
    
    /// Look for line feed or end line delimeters
    while( (*cf == '\r') || (*cf =='\n') )
            *cf++;
    
    /// Set global pointer to the next line
    con_file_ptr = cf;
    
    /// Return the pointer to the begining of the line
    return(r);
}

#ifndef SEEK_END
#define SEEK_END 2
#endif

/* return the basename of a string */

char *CPico::ps_basename(char * s)
{
    char *t;
    char *r;

    t = s;
    r = NULL;
    
    /// Move past the path of the file name
    while( *t )
    { 
        if( *t == '/') 
            r = t;
        t++;
    }

    /// If a path was not attached, set pointer to original name
    if( r == NULL ) 
        r = s;

    /// Remove any begining slashes associated with a path string
    while( (*r =='/') ) 
        r++;

    /// Point to begining of string
    t = basebuf;
    
    /// Replace full path file name with just the file name, no extension
    while( *r && (*r != '.') )
        *t++ = *r++;
    
    /// NULL terminate the string
    *t = '\0';

    /// Return only the name of the file, no extension
    return(basebuf);
}

/* partable way to find the length of a file */

int CPico::file_size( char *name )
{
    long eof_ftell;
    FILE *file;

    file = fopen( name, "r");

    if ( file == NULL )
        return( 0L );
    
    fseek( file, 0L, SEEK_END );
    eof_ftell = ftell( file );
    fclose( file );
    
    return( eof_ftell );
}


void CPico::usage()
{
    printf("Usage: ps_mkfs infile [outfile]\n");
}

/* see if the files in list are openable */

void CPico::check_file_names()
{
    FILE * F;
    int fs;
    int i;
    char * filemem;
    CString message;

    struct ps_line *psl;
    psl = file_map;

    i = 0;

    /// Loop through linked list of files and check for validity and compression
    while( psl )
    {
        
        if( (fs = file_size(psl->native)) == (int)NULL){
            
            message.Format("Can't open file:%s\n", psl->native);
            status.SpoolMessage(message);
            i++;
        }
        else
        {
            /// Set the file size
            psl->length = fs;
            
            /*#ifdef FILE_COMPRESSION*/
            /// Check if file should be compressed
            if (strncmp(psl->compress, "YES", 3) == 0)
            {
                filemem = (char*)malloc((unsigned long)fs);
                F = fopen(psl->native, "rb");
                fread(filemem, 1, fs, F);
                /*
                int ps_compress(int mode,char * inbuf, char *outbuf,int length);    
                int ps_decompress(Request * req,char * inbuf, char *outbuf,int ilen, int olen);
                */
                fs = ps_compress(DONT_OUTPUT, filemem, NULL, fs);
                free(filemem);

                /// Is compressed length less than the actual length?
                if( (fs + CHDSZ + CHDFUD ) < psl->length)
                    psl->clength = fs;
                else
                    psl->clength =0;
            }           
            /*#endif*/
            
        }
        
        /// Increment the global variable
        embedded_files++;

        /// Point to the next file in the list
        psl = psl->next;
    }
    
    /// Any errors?
    if( i )
    {
        message.Format("Open failed for %d file(s) Exiting.\n",i);
        status.SpoolMessage(message);
    }
}

/// Create the header to the file holding all converted files

void CPico::output_header(char *name)
{
    CString buf(name);
    CString pad = "*        ";
    int     index;

    fprintf(out, "/***********************************************************************\n");
    fprintf(out, "*\n");
    fprintf(out, "* FILE NAME\n");
    fprintf(out, "*\n");
    index = buf.GetLength() - (buf.ReverseFind('\\') +1);
    buf = pad.Left(8) + buf.Right(index);
    buf = buf + "\n";
    fprintf(out, buf);
    fprintf(out, "*\n");
    fprintf(out, "* DESCRIPTION\n");
    fprintf(out, "*\n");
    fprintf(out, "*       This is an auto-generated file containing contents of\n");
    fprintf(out, "*       files which are to be written to a disk.\n");
    fprintf(out, "*\n");
    fprintf(out, "************************************************************************\n");
    fprintf(out, "* WARNING: This file is generated by a program.\n");
    fprintf(out, "* WARNING: Do not edit this file.\n");
    fprintf(out, "***********************************************************************/\n");
    fprintf(out, "#include \"nucleus.h\"\n");
    fprintf(out, "#include \"storage/pcdisk.h\"\n");
    fprintf(out, "\n");

}

/// Create the footer to the file holding all converted files

void CPico::output_footer(char *name)
{
    CString buf(name);

    fprintf(out, "\n");
    fprintf(out, "/******************************************************************************\n");
    fprintf(out, "*\n");
    fprintf(out, "* FUNCTION\n");
    fprintf(out, "*\n");
    fprintf(out, "*      Filecon_Write_Files_To_Disk\n");
    fprintf(out, "*\n");
    fprintf(out, "* DESCRIPTION\n");
    fprintf(out, "*\n");
    fprintf(out, "*      This function writes all file contents to the file system.\n");
    fprintf(out, "*\n");
    fprintf(out, "******************************************************************************/\n");
    fprintf(out, "STATUS Filecon_Write_Files_To_Disk(VOID)\n");
    fprintf(out, "{\n");
    fprintf(out, "    INT32 ret;\n");
    fprintf(out, "    INT fd;\n");
    fprintf(out, "    INT i;\n");
    fprintf(out, "\n");
    fprintf(out, "    /* Loop for all file entries in the table. */\n");
    fprintf(out, "    for (i = 0; Filecon_Fs[i].file_name != NU_NULL; i++)\n");
    fprintf(out, "    {\n");
    fprintf(out, "        fd = NU_Open((CHAR*)Filecon_Fs[i].file_name,\n");
    fprintf(out, "                     PO_WRONLY | PO_CREAT | PO_BINARY, PS_IWRITE);\n");
    fprintf(out, "        if (fd < 0)\n");
    fprintf(out, "        {\n");
    fprintf(out, "            return (-1);\n");
    fprintf(out, "        }\n");
    fprintf(out, "\n");
    fprintf(out, "        ret = NU_Write(fd, (CHAR*)Filecon_Fs[i].file_data,\n");
    fprintf(out, "                          Filecon_Fs[i].file_size);\n");
    fprintf(out, "        if (ret != Filecon_Fs[i].file_size)\n");
    fprintf(out, "        {\n");
    fprintf(out, "            NU_Close(fd);\n");
    fprintf(out, "            return (-1);\n");
    fprintf(out, "        }\n");
    fprintf(out, "\n");
    fprintf(out, "        NU_Close(fd);\n");
    fprintf(out, "    }\n");
    fprintf(out, "\n");
    fprintf(out, "    return (NU_SUCCESS);\n");
    fprintf(out, "\n");
    fprintf(out, "} /* Filecon_Write_Files_To_Disk */\n");

}

/// This function is never used

void CPico::make_extern_table()
{
    struct ps_line *psl;
    psl = file_map;

    /// Loop through list of files
    while( psl )
    {
        fprintf(out,"extern char FAR cfs_%s_%s[];\n",ps_basename(psl->picopat), ps_ext(psl->picopat));
        psl = psl->next;
    }

    fprintf(out,"\n");

//    fprintf(out,"int	Embedded_Files=%d;\n",embedded_files);
}

/// Create the table of structures holding data of each file

void CPico::make_fs_table()
{
    struct ps_line *psl;
    
    psl = file_map;
//    fprintf(out,"int	Embedded_Files=%d;\n\n",embedded_files);

    /// Initial structure definition
    fprintf(out, "\n");
	fprintf(out, "struct FILECON_FS_ITEM\n");
    fprintf(out, "{\n");
    fprintf(out, "    CHAR            *file_name;             /* Name of the file. */\n");
    fprintf(out, "    CHAR            *file_data;             /* Address of contents. */\n");
    fprintf(out, "    INT32           file_size;              /* Length in bytes. */\n");
    fprintf(out, "};\n\n");

    /// Begining of structure
    fprintf(out, "struct FILECON_FS_ITEM Filecon_Fs[] = {\n");

    /// Loop through all files, printing all info on a line
    while( psl ){

        if(strncmp(psl->secure, "YES", 3) == 0)
        {
            fprintf(out,"{{\"%s%s\"},",theApp.m_PrivateDirectory, psl->picopat);
        }
        else
        {
            fprintf(out,"    { \"%s\",",(psl->picopat) + 1);
        }
        fprintf(out," (CHAR*)cfs_%s_%s,",ps_basename(psl->picopat), ps_ext(psl->picopat));
        
        
/*        fprintf(out," WS_COMPILED");

        if (strncmp(psl->compress, "YES", 3) == 0)
        {
            if( psl->clength)
                fprintf(out," | WS_COMPRESSED");
        }

        fprintf(out,",");*/
        
        fprintf(out," %d",psl->length);    /* length == clength */
        /*fprintf(out," %d,",psl->clength);*/   /* length == clength */
//        if( psl->next )


//        fprintf(out," NU_NULL},");
/*        else
            fprintf(out,"NU_NULL}");
*/        
        fprintf(out," },\n");
        
        psl= psl->next;
    }
    fprintf(out,"    { NU_NULL, NU_NULL, 0 }\n");
    fprintf(out,"};\n");
}

///

void CPico::make_file_data()
{
    struct ps_line *psl;
    char * filemem;
    char * cfilemem;
    FILE * F;
    int i;
    char * outmem;
    int  length;
    CString message;
    int compress=0;
    
    compress=0;
    

    psl = file_map;
    while( psl ){
    
        if (m_ProgressCtrl)
            m_ProgressCtrl->StepIt();
        
        F=fopen(psl->native,"rb");

        //If bad file, skip it
        //Mark B. 8/11/98
        if (F == NULL)
        {
            psl = psl->next;
            continue;
        }
        
        filemem=(char*)malloc((unsigned long)psl->length);
        if (filemem == NULL)
        {
            ::MessageBox(NULL, "Out of memory!", "Error", MB_OK);
            return;
        }
        
        i = fread(filemem,1,psl->length,F);
        if( i != psl->length)
        {
            message.Format("make_file_data read error\n");
            status.SpoolMessage(message);
        }
        outmem=filemem;

        length=psl->length;

/*#ifdef FILE_COMPRESSION*/
        if (strncmp(psl->compress, "YES", 3) == 0)
        {
            
            if( psl->clength)
            {
                compress = 1;
                cfilemem = (char*)malloc((unsigned long)psl->clength + CHDSZ);
                length = ps_compress(DO_OUTPUT, filemem, cfilemem + 4,
                                    psl->length);
                memcpy(cfilemem, CHD, 4);
                outmem = cfilemem;
                length += 4;
                /*free(cfilemem);*/
            }
        }
/*#endif*/

        message.Format("%s ",psl->native);
        status.SpoolMessage(message);
        message.Format("%d compress length=%d",psl->length,length);
        status.SpoolMessage(message);
        fprintf(out,"static const CHAR cfs_%s_%s[]={\n",ps_basename(psl->picopat), ps_ext(psl->picopat));
        
        for(i=0; i<=length-2;i++){

            fprintf(out,"\'\\%o\',",*(outmem+i)&0xff);
            if( ! (i%12 ) )fprintf(out,"\n");
        }
    
        fprintf(out,"\'\\%o\'};\n\n",*(outmem+i)&0xff );
        

        free(filemem);
        // If compressed Free compressed buffer and set the compress flag back to 0 for next flag.
        if (compress == 1)
        {
            free(cfilemem);
            compress = 0;
        }
        psl = psl->next;
        fclose(F);

    }
}


/* extract extension of file */
char *CPico::ps_ext(char * s)
{
    char *pos;

    pos = s + strlen(s);
    while ((*pos != '.') && (pos != s))
        pos --;
    return (++pos);
}


/* dummy routine to satisfy build */

//void
//ps_net_write(Request * req, char * buf, int sz)
//{
//}


////////////////////COMPRESSION ROUTINES//////////////////////////////////////////////

int CPico::ps_compress(int mode,char * inbuf, char *outbuf,int length)
{
    BIT_FILE out;
    BIT_FILE in;

    OpenOutputBitFile( &out, outbuf, length );
    out.mode = mode;

    OpenInputBitFile( &in, inbuf, length );

    CompressFile(&in,&out);
    CloseOutputBitFile( &out );

    return(out.length+1);
}

int CPico::ps_decompress(Request * req,char * inbuf, char *outbuf,int ilen, int olen)
{
    BIT_FILE in;
    BIT_FILE out;


#ifdef DEBUG 
printf("ps_decompress(%x %x %d %d) \n",inbuf,outbuf,ilen,olen);
#endif

    OpenOutputBitFile(&out, outbuf, olen );
    OpenInputBitFile( &in, inbuf, ilen );

    out.fd=req;
    out.mode = NET_OUTPUT | DONT_OUTPUT;
    out.length = 0;

    ExpandFile(&in,&out);
    CloseOutputBitFile( &out );

    return(0);

}

int CPico::CompressFile(BIT_FILE *input, BIT_FILE *output)
{
    int c;
    SYMBOL s;

    build_model( input, output);
    initialize_arithmetic_encoder();

    while ( ( c = buf_getc( input ) ) != EOF ) {
    convert_int_to_symbol( c, &s );
    encode_symbol( output, &s );
    }
    convert_int_to_symbol( END_OF_STREAM, &s );
    encode_symbol( output, &s );
    flush_arithmetic_encoder( output );
    OutputBits( output, 0L, 16 );
    return 0;
}

void CPico::ExpandFile(BIT_FILE *input, BIT_FILE *output)
{
    SYMBOL s;
    int c;
    int count;

    input_counts( input );
    initialize_arithmetic_decoder( input );
    for ( ; ; ) {
    get_symbol_scale( &s );
    count = get_current_count( &s );
    c = convert_symbol_to_int( count, &s );
    if ( c == END_OF_STREAM )
            break;
    remove_symbol_from_stream( input, &s );
        buf_putc( (char) c, output );
    }
}

/*
 * This is the routine that is called to scan the input file, scale
 * the counts, build the totals array, the output the scaled counts
 * to the output file.
 */

void CPico::build_model(BIT_FILE *input, BIT_FILE *output)
{
    unsigned long counts[ 256 ];
    unsigned char scaled_counts[ 256 ];

    count_bytes( input, counts );
    scale_counts( counts, scaled_counts );
    output_counts( output, scaled_counts );
    build_totals( scaled_counts );
}

/*
 * This routine runs through the file and counts the appearances of each
 * character.
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

void CPico::count_bytes(BIT_FILE *input, unsigned long counts[])
{
    int i;
    int c;

    for ( i = 0 ; i < 256 ; i++ )
        counts[ i ] = 0;

    while ( ( c = buf_getc( input )) != EOF )
    counts[ c ]++;

    input->cur_pos= input->fstart;  /* "REWIND" input stream */
}

/*
 * This routine is called to scale the counts down.  There are two types
 * of scaling that must be done.  First, the counts need to be scaled
 * down so that the individual counts fit into a single unsigned char.
 * Then, the counts need to be rescaled so that the total of all counts
 * is less than 16384.
 */
void CPico::scale_counts(unsigned long counts[], unsigned char scaled_counts[])
{
    int i;
    unsigned long max_count;
    unsigned int total;
    unsigned long scale;

/*
 * The first section of code makes sure each count fits into a single byte.
 */
    max_count = 0;
    for ( i = 0 ; i < 256 ; i++ )
       if ( counts[ i ] > max_count )
       max_count = counts[ i ];
    scale = max_count / 256;
    scale = scale + 1;
    for ( i = 0 ; i < 256 ; i++ ) {
        scaled_counts[ i ] = (unsigned char ) ( counts[ i ] / scale );
        if ( scaled_counts[ i ] == 0 && counts[ i ] != 0 )
            scaled_counts[ i ] = 1;
    }
/*
 * This next section makes sure the total is less than 16384.  I initialize
 * the total to 1 instead of 0 because there will be an additional 1 added
 * in for the END_OF_STREAM symbol;
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
        scaled_counts[ i ] /= scale; /* this code gives warning but didn't want to modify
                                        since it is part of the original Pico Server code */
}

/*
 * This routine is used by both the encoder and decoder to build the
 * table of cumulative totals.  The counts for the characters in the
 * file are in the counts array, and we know that there will be a single
 * instance of the EOF symbol.
 */
void CPico::build_totals(unsigned char scaled_counts[])
{
    int i;

    totals[ 0 ] = 0;
    for ( i = 0 ; i < END_OF_STREAM ; i++ )
        totals[ i + 1 ] = totals[ i ] + scaled_counts[ i ];
    totals[ END_OF_STREAM + 1 ] = totals[ END_OF_STREAM ] + 1;
}

/*
 * In order for the compressor to build the same model, I have to store
 * the symbol counts in the compressed file so the expander can read
 * them in.  In order to save space, I don't save all 256 symbols
 * unconditionally.  The format used to store counts looks like this:
 *
 *  start, stop, counts, start, stop, counts, ... 0
 *
 * This means that I store runs of counts, until all the non-zero
 * counts have been stored.  At this time the list is terminated by
 * storing a start value of 0.  Note that at least 1 run of counts has
 * to be stored, so even if the first start value is 0, I read it in.
 * It also means that even in an empty file that has no counts, I have
 * to pass at least one count.
 *
 * In order to efficiently use this format, I have to identify runs of
 * non-zero counts.  Because of the format used, I don't want to stop a
 * run because of just one or two zeros in the count stream.  So I have
 * to sit in a loop looking for strings of three or more zero values in
 * a row.
 *
 * This is simple in concept, but it ends up being one of the most
 * complicated routines in the whole program.  A routine that just
 * writes out 256 values without attempting to optimize would be much
 * simpler, but would hurt compression quite a bit on small files.
 *
 */
void CPico::output_counts(BIT_FILE *output, unsigned char scaled_counts[])
{
    int first;
    int last;
    int next;
    int i;

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
    for ( ; first < 256 ; first = next ) {
    last = first + 1;
    for ( ; ; ) {
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
    };
/*
 * Here is where I output first, last, and all the counts in between.
 */
        if ( buf_putc( first, output ) != first )
        fatal_error( "Error writing byte counts\n" );
    if ( buf_putc( last, output ) != last )
        fatal_error( "Error writing byte counts\n" );
    for ( i = first ; i <= last ; i++ ) {
            if ( buf_putc( scaled_counts[ i ], output ) !=
                 (int) scaled_counts[ i ] )
        fatal_error( "Error writing byte counts\n" );
    }
    }
    if ( buf_putc( 0, output ) != 0 )
        fatal_error( "Error writing byte counts\n" );
}

/*
 * When expanding, I have to read in the same set of counts.  This is
 * quite a bit easier that the process of writing them out, since no
 * decision making needs to be done.  All I do is read in first, check
 * to see if I am all done, and if not, read in last and a string of
 * counts.
 */

void CPico::input_counts(BIT_FILE *input)
{
    int first;
    int last;
    int i;
    int c;
    unsigned char scaled_counts[ 256 ];

    for ( i = 0 ; i < 256 ; i++ )
        scaled_counts[ i ] = 0;
    if ( ( first = buf_getc( input ) ) == EOF )
    fatal_error( "Error reading byte counts\n" );
    if ( ( last = buf_getc( input ) ) == EOF )
    fatal_error( "Error reading byte counts\n" );
    for ( ; ; ) {
    for ( i = first ; i <= last ; i++ )
            if ( ( c = buf_getc( input ) ) == EOF )
        fatal_error( "Error reading byte counts\n" );
        else
                scaled_counts[ i ] = (unsigned char) c;
        if ( ( first = buf_getc( input ) ) == EOF )
        fatal_error( "Error reading byte counts\n" );
    if ( first == 0 )
        break;
        if ( ( last = buf_getc( input ) ) == EOF )
        fatal_error( "Error reading byte counts\n" );
    }
    build_totals( scaled_counts );
}

/*
 * Everything from here down define the arithmetic coder section
 * of the program.
 */

/*
 * This routine must be called to initialize the encoding process.
 * The high register is initialized to all 1s, and it is assumed that
 * it has an infinite string of 1s to be shifted into the lower bit
 * positions when needed.
 */
void CPico::initialize_arithmetic_encoder()
{
    low = 0;
    high = 0xffff;
    underflow_bits = 0;
}

/*
 * At the end of the encoding process, there are still significant
 * bits left in the high and low registers.  We output two bits,
 * plus as many underflow bits as are necessary.
 */
void CPico::flush_arithmetic_encoder(BIT_FILE *stream)
{
    OutputBit( stream, low & 0x4000 );
    underflow_bits++;
    while ( underflow_bits-- > 0 )
        OutputBit( stream, ~low & 0x4000 );
}

/*
 * Finding the low count, high count, and scale for a symbol
 * is really easy, because of the way the totals are stored.
 * This is the one redeeming feature of the data structure used
 * in this implementation.
 */
void CPico::convert_int_to_symbol(int c, SYMBOL *s)
{
    s->scale = totals[ END_OF_STREAM + 1 ];
    s->low_count = totals[ c ];
    s->high_count = totals[ c + 1 ];
}

/*
 * Getting the scale for the current context is easy.
 */
void CPico::get_symbol_scale(SYMBOL *s)
{
    s->scale = totals[ END_OF_STREAM + 1 ];
}

/*
 * During decompression, we have to search through the table until
 * we find the symbol that straddles the "count" parameter.  When
 * it is found, it is returned. The reason for also setting the
 * high count and low count is so that symbol can be properly removed
 * from the arithmetic coded input.
 */
int CPico::convert_symbol_to_int(int count, SYMBOL *s)
{
    int c;

    for ( c = END_OF_STREAM ; count < totals[ c ] ; c-- )
    ;
    s->high_count = totals[ c + 1 ];
    s->low_count = totals[ c ];
    return( c );
}

/*
 * This routine is called to encode a symbol.  The symbol is passed
 * in the SYMBOL structure as a low count, a high count, and a range,
 * instead of the more conventional probability ranges.  The encoding
 * process takes two steps.  First, the values of high and low are
 * updated to take into account the range restriction created by the
 * new symbol.  Then, as many bits as possible are shifted out to
 * the output stream.  Finally, high and low are stable again and
 * the routine returns.
 */
void CPico::encode_symbol(BIT_FILE *stream, SYMBOL *s)
{
    long range;
/*
 * These three lines rescale high and low for the new symbol.
 */
    range = (long) ( high-low ) + 1;
    high = low + (unsigned short int)
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int)
                 (( range * s->low_count ) / s->scale );
/*
 * This loop turns out new bits until high and low are far enough
 * apart to have stabilized.
 */
    for ( ; ; ) {
/*
 * If this test passes, it means that the MSDigits match, and can
 * be sent to the output stream.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
            OutputBit( stream, high & 0x8000 );
            while ( underflow_bits > 0 ) {
                OutputBit( stream, ~high & 0x8000 );
                underflow_bits--;
            }
        }
/*
 * If this test passes, the numbers are in danger of underflow, because
 * the MSDigits don't match, and the 2nd digits are just one apart.
 */
        else if ( ( low & 0x4000 ) && !( high & 0x4000 )) {
            underflow_bits += 1;
            low &= 0x3fff;
            high |= 0x4000;
        } else
            return ;
        low <<= 1;
        high <<= 1;
        high |= 1;
    }
}

/*
 * When decoding, this routine is called to figure out which symbol
 * is presently waiting to be decoded.  This routine expects to get
 * the current model scale in the s->scale parameter, and it returns
 * a count that corresponds to the present floating point code:
 *
 *  code = count / s->scale
 */
short int CPico::get_current_count(SYMBOL *s)
{
    long range;
    short int count;

    range = (long) ( high - low ) + 1;
    count = (short int)
            ((((long) ( code - low ) + 1 ) * s->scale-1 ) / range );
    return( count );
}

/*
 * This routine is called to initialize the state of the arithmetic
 * decoder.  This involves initializing the high and low registers
 * to their conventional starting values, plus reading the first
 * 16 bits from the input stream into the code value.
 */
void CPico::initialize_arithmetic_decoder(BIT_FILE *stream)
{
    int i;

    code = 0;
    for ( i = 0 ; i < 16 ; i++ ) {
        code <<= 1;
        code += InputBit( stream );
    }
    low = 0;
    high = 0xffff;
}

/*
 * Just figuring out what the present symbol is doesn't remove
 * it from the input bit stream.  After the character has been
 * decoded, this routine has to be called to remove it from the
 * input stream.
 */
void CPico::remove_symbol_from_stream(BIT_FILE *stream, SYMBOL *s)
{
    long range;

/*
 * First, the range is expanded to account for the symbol removal.
 */
    range = (long)( high - low ) + 1;
    high = low + (unsigned short int)
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int)
                 (( range * s->low_count ) / s->scale );
/*
 * Next, any possible bits are shipped out.
 */
    for ( ; ; ) {
/*
 * If the MSDigits match, the bits will be shifted out.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) ) {
        }
/*
 * Else, if underflow is threatening, shift out the 2nd MSDigit.
 */
        else if ((low & 0x4000) == 0x4000  && (high & 0x4000) == 0 ) {
            code ^= 0x4000;
            low   &= 0x3fff;
            high  |= 0x4000;
        } else
 /*
 * Otherwise, nothing can be shifted out, so I return.
 */
            return;
        low <<= 1;
        high <<= 1;
        high |= 1;
        code <<= 1;
        code += InputBit( stream );
    }
}

BIT_FILE *CPico::OpenOutputBitFile( BIT_FILE * bit_file, char *buf,int length )
{
    bit_file->fstart =  buf;
    bit_file->cur_pos = buf;
    bit_file->length = 0;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;

    return(0);
}

BIT_FILE *CPico::OpenInputBitFile( BIT_FILE * bit_file, char * buf, int length)
{
    bit_file->fstart = buf;
    bit_file->cur_pos = buf;
    bit_file->length = length;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
    return(0);
}

void CPico::CloseOutputBitFile(BIT_FILE *bit_file)
{
    if ( bit_file->mask != 0x80 )
        if ( buf_putc( bit_file->rack, bit_file ) != bit_file->rack )
            fatal_error( "Fatal error in CloseBitFile!\n" );

    flush_buf_putc(bit_file);
}

void CPico::CloseInputBitFile(BIT_FILE *bit_file)
{
}

void CPico::OutputBit(BIT_FILE *bit_file, int bit)
{
    if ( bit )
        bit_file->rack |= bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 ) {
        if ( buf_putc( bit_file->rack, bit_file ) != bit_file->rack )
            fatal_error( "Fatal error in OutputBit!\n" );
        else
        if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
        ;/*null statement*/
        bit_file->rack = 0;
        bit_file->mask = 0x80;
    }
}

void CPico::OutputBits(BIT_FILE *bit_file, unsigned long code, int count)
{
    unsigned long mask;

    mask = 1L << ( count - 1 );
    while ( mask != 0) {
        if ( mask & code )
            bit_file->rack |= bit_file->mask;
        bit_file->mask >>= 1;
        if ( bit_file->mask == 0 ) {
        if ( buf_putc( bit_file->rack, bit_file ) != bit_file->rack )
        fatal_error( "Fatal error in OutputBit!\n" );
        else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
            ;/* null statement*/
        bit_file->rack = 0;
            bit_file->mask = 0x80;
        }
        mask >>= 1;
    }
}

int CPico::InputBit(BIT_FILE *bit_file)
{
    int value;

    if ( bit_file->mask == 0x80 ) {
        bit_file->rack = buf_getc( bit_file );
        if ( bit_file->rack == EOF )
            fatal_error( "Fatal error in InputBit!\n" );
    if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
        ;/*null statement */
    }
    value = bit_file->rack & bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
        bit_file->mask = 0x80;
    return( value ? 1 : 0 );
}


unsigned long CPico::InputBits(BIT_FILE *bit_file, int bit_count)
{
    unsigned long mask;
    unsigned long return_value;

    mask = 1L << ( bit_count - 1 );
    return_value = 0;
    while ( mask != 0) {
    if ( bit_file->mask == 0x80 ) {
        bit_file->rack = buf_getc( bit_file );
        if ( bit_file->rack == EOF )
        fatal_error( "Fatal error in InputBit!" );
        if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
        ; /* null statement */
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


int CPico::buf_putc(unsigned int out, BIT_FILE *bitfile)
{
    char c;

    if(( bitfile->mode & DONT_OUTPUT) == 0  ) {
        *(bitfile->cur_pos) = (char)out;

    }

    if( bitfile->mode  & NET_OUTPUT ) {
        c=(char)out;
        //ps_net_write(bitfile->fd,&c,1);
    }

    bitfile->cur_pos++;
    bitfile->length++;

    return(out);
}

void CPico::flush_buf_putc(BIT_FILE *b)
{
}


int CPico::buf_getc(BIT_FILE *bitfile)
{
    int i;

    if( (bitfile->cur_pos - bitfile->fstart) > ((bitfile->length))-1 ){


            return( EOF);
    };
        
    i = *(bitfile->cur_pos) & 0xff;
    bitfile->cur_pos++;

    return(i);
}

void CPico::fatal_error( char *s)
{
    CString message;
    message.Format("Bit error %s\n",s);
    status.SpoolMessage(message);

}
