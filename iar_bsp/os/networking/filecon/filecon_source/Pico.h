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

// Pico.h : header file
#ifndef _CPICO_H
#define _CPICO_H
//
#include "ps_pico.h"
#include "status.h"

#define PACIFIER_COUNT 2047
#define END_OF_STREAM 256

/* native to mapped file_system */
struct ps_line{
    int         length;
    int         clength;
    char        * native;   /* native file path */
    char        * picopat;  /* pico_fs path */
    struct ps_line  * next;     /* next line structure*/
    char *compress; /* Added to track compression on per file basis*/
    char *secure; /* Added to track per file secure option */
};
 
typedef struct bit_file {
    char * fstart;  /* start of "file" */
    char * cur_pos; /* current position in "file" */
    int    length;  /* length of "file" */
    unsigned char mask;
    int rack;
    int pacifier_counter;
    int mode;
    Request *  fd;
} BIT_FILE;

typedef struct {
    unsigned short int low_count;
    unsigned short int high_count;
    unsigned short int scale;
} SYMBOL;


/////////////////////////////////////////////////////////////////////////////
// CPico window

class CPico : public CObject
{

// Construction
public:
    CPico();
    BOOL Generate_C_File(char *input, char *output, CProgressCtrl *ProgressCtrl);
    char *ps_ext(char * s);

    void make_extern_table();
    void parse_con(char * conbuf, int size);
    int file_size( char *name );
    void usage();
    void check_file_names();
    void output_header(char *name);
    void output_footer(char *name);
    void make_fs_table();
    void make_file_data();
    void ps_net_write(Request * req, char * buf, int sz);
    
    void build_model( BIT_FILE *input, BIT_FILE *output );
    void scale_counts( unsigned long counts[], unsigned char scaled_counts[] );
    void build_totals( unsigned char scaled_counts[] );
    void count_bytes( BIT_FILE *input, unsigned long counts[] );
    void output_counts( BIT_FILE *output, unsigned char scaled_counts[] );
    void input_counts( BIT_FILE *stream );
    void convert_int_to_symbol( int symbol, SYMBOL *s );
    void get_symbol_scale( SYMBOL *s );
    int convert_symbol_to_int( int count, SYMBOL *s );
    void initialize_arithmetic_encoder( void );
    void encode_symbol( BIT_FILE *stream, SYMBOL *s );
    void flush_arithmetic_encoder( BIT_FILE *stream );
    short int get_current_count( SYMBOL *s );
    void initialize_arithmetic_decoder( BIT_FILE *stream );
    void remove_symbol_from_stream( BIT_FILE *stream, SYMBOL *s );
    void fatal_error( char *s);
    int CompressFile( BIT_FILE * buf_input, BIT_FILE * buf_output );
    void ExpandFile( BIT_FILE *buf_input, BIT_FILE *buf_output );
    BIT_FILE *OpenInputBitFile( BIT_FILE *name,char * buf, int length );
    BIT_FILE *OpenOutputBitFile( BIT_FILE *name, char * buf, int length );
    void OutputBit( BIT_FILE *bit_file, int bit );
    void OutputBits( BIT_FILE *bit_file, unsigned long code, int count );
    int InputBit( BIT_FILE *bit_file );
    unsigned long InputBits( BIT_FILE *bit_file, int bit_count );
    void CloseInputBitFile( BIT_FILE *bit_file );
    void CloseOutputBitFile( BIT_FILE *bit_file );
    int buf_getc(BIT_FILE *bitfile);
    int buf_putc(unsigned int out, BIT_FILE *bitfile);
    void flush_buf_putc(BIT_FILE *b);
    int ps_compress(int mode,char * inbuf, char *outbuf,int length);    
    int ps_decompress(Request * req,char * inbuf, char *outbuf,int ilen, int olen);


// Attributes
public:
    CStatus status;
    short int totals[ 258 ];            /* The cumulative totals                */
    /*
    * These four variables define the current state of the arithmetic
    * coder/decoder.  They are assumed to be 16 bits long.  Note that
    * by declaring them as short ints, they will actually be 16 bits
    * on most 80X86 and 680X0 machines, as well as VAXen.
    */
    long underflow_bits;             /* Number of underflow bits pending   */
    struct ps_line *file_map;
    char * con_fil;
    char * con_file_ptr;
    int    con_file_size;
    char * get_line();
    char * ps_basename(char * s);
    FILE * out;

    CProgressCtrl *m_ProgressCtrl;

    int embedded_files;
    BOOL m_Compress;

    int m_NumFiles;

// Operations
public:
// Implementation
public:
    virtual ~CPico();
};

#endif 
/////////////////////////////////////////////////////////////////////////////
