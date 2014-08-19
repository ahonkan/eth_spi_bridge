
#ifndef _PICO_H
#define _PICO_H


/* 
 * 
 * PicoServer  Embedded Web Server 
 *
 * Copywrite (c) 1995 1996 1997 CNiT 
 *
 * Communication  and Information Technology 
 *
 *
 */
#define FILE_UPLOAD
#define LITTLE_ENDIAN

#ifndef ERROR  
#define ERROR	-1 
#endif

#include 	"ps_conf.h"
#include	"stdio.h"

#define		XOS_FS_PREFIX	"/tuner/html/"	/* osdependent server root */ 
#define		OS_FS_PREFIX	"/tmp/"			/* osdependent server root */ 

typedef void* Critical;

#define DEBUG

#ifndef _MNT
#ifdef STAND
#include "target.h"
#endif
#endif  


#ifdef _MNT
typedef short NET_FD;
typedef void* Critical;
#else
typedef short NET_FD;
#endif

#ifdef PS_NUCLEUS
#include "ps_nuc.h"
#endif

#ifdef PS_LYNX
// typedef unsigned int int16;
typedef unsigned int NET_FD;
#endif

/* LOCAL CONFIGURATION SECTION  
 *
 */

/* define URL's for success and failure (Dont forget the '\n' at the end !!! */

#define AUTH_SUCCESS_URI	"/success.htm\n"		/* go here after sucessfull authentication */
#define AUTH_FAILURE_URI	"/failure.htm\n"		/* go here after sucessfully logged in */


#define AUTH_SCREEN_URI		"Login.htm"			/* Screen to show an unauthenticated user */
#define ODEFAULT_URI			"default.htm"			/* default uri when "/" is referenced */
#define DEFAULT_URI			"dir"			/* default uri when "/" is referenced */
#define PUBLIC_DIR			"publicdir"			/* anything is this directory is ALWAYS public */


/* HASH tokens to identify type of HTTP transfer */

#define GET					224
#define HEAD				206
#define POST				242
#define AUTH				234
#define SATH				232


/* HTTP output headers */

#define CONTENT_LENGTH			"Content-length: "
#define CONTENT_TYPE			"Content-type: "
#define CONTENT_LOCATION		"Location: "

#define TYPE_TXT_HTML	"text/html"



#define SSI_TOKEN		"<!-#"		/* ServerSideInclude magic string */
#define SSI_MARKER		".ssi"		/* extrnsion for a server side include HTML file */
#define SSI_NAMELEN		32			/* maximum */
#define TOKEN_HEAP		256			/* maximum number of tokens per request */
#define SSI_LINE		256			/* maximum size of a SSI tag */

#define URL_LEN			48			/* length of a URL */
#define URI_LEN			(URL_LEN/2)
#define NUM_LEN			16			/* maximum size of a number */
#define MAX_PLUGIN		32			/* maximum plugins */
#define CRYPT_LEN		8			/* encryption system works in units of this size */
#define	CRYPT_BLKSZ		CRYPT_LEN	/* we encrypt/decrypt in thease units */
#define	CRYPT_KEYSZ		8			/* bytes needed by an encryption key */
#define	IPADDR			4			/* bytes in an IP address */
#define SALT_BUF		64	
#define NUM_SEED		2	
#define	SRG_SIZE		128	
#define BPB				8			/* bits per byte */
#define RECEIVE_SIZE    2048		/* size of the recieve buffer */

#ifndef NULL
#define NULL 0
#endif


#define SUCCESS			1
#define FAILURE			0
#define ENOFILE			2			/* file not found */

#define DATA_FILE		0			/* the file contains data to serve */
#define CGI_FORM		1			/* the file is a form handler */
#define CGI_CALL		2			/* the file is a CGI-generator */


#define NET_WAIT		1

#define MAX_CGI_ARGS		32

#define REQ_PROCEED			1	/* function preformed, continue */
#define REQ_ABORTED			2	/* request should be aborted */
#define REQ_NOACTION		3	/* plugin did nto preform intended action */
#define REQ_ERROR			4	/* plugin i/o error abort request */


/*	HTTP return values */

#define PROTO_OK			200		/*  OK (success)--- */
#define PROTO_NO_RESPONSE	204		/*  OK but no data to be returned to client */
#define PROTO_REDIRECT		302		/* Redirect client to new URL in Location: header */
#define PROTO_NOT_MODIFIED	304		/* URL not modified (in resp to client conditional GET */
#define PROTO_BAD_REQUEST	400		/* Tells clinet she sent a bad request */
#define PROTO_UNAUTHORIZED	401		/* The request requires user authentication */
#define PROTO_FORBIDDEN		403		/* Server forbids this client request */
#define PROTO_NOT_FOUND		404		/* the famous old "404" URL not found error */
#define PROTO_SERVER_ERROR	500		/* Internal Server ERROR */
#define PROTO_NOT_IMPLEMENTED 501	/* Server feature not implemented */


/* the incore filesystem table */

struct fs_file{

	char name[URI_LEN];			/* name */
	char * addr;				/* starting address */
	short type;				/* mode */
	int	  length;			/* real length */
	int	  clength;			/* compressed length */
	struct fs_file * next;			/* next file in chain */
};

/* incore file system TYPE flags */

#define COMPILED	1		/* file was compiled */
#define DELEATED	2		/* file is deleated */
#define COMPRESSED	4		/* file compressed */

/* compression control */

#define DO_OUTPUT	1
#define DONT_OUTPUT	4
#define NET_OUTPUT	2		/* de-compr output to net */
#define CHDFUD		16		/* fudge factor */
#define CHDSZ		4		/* size of preamble */
#define CHD		"pS$C"		/* compression marker*/

/* upload file mode control */

#define UPL_MODE	0666

/* Server control defines */

#define		S_SHUTDOWN		1		/* Shutdown the server (after the next command finishes ) */
#define		S_ONLINE        2			/* Make the server go online */
#define		S_PORT			4		/* set the port network port the server operates on */


/* The authentication structure */

#define AUTH_MAX_USERS		4			/* maximum authenticated users allowed at a time*/
#define AUTH_TIMEOUT		20			/* a session idle for AUTH_TIMEOUT seconds is de-authenticated */
#define AUTH_GIVEN			2		/* state value for an authenticated user */	
#define AUTH_FREE			1		/* auth structure is free */
#define AUTH_NEED_SATH		4			/* salt given need encrypted packet */
#define	SALT_TIMEOUT		5			/* timeout divisor for salt timeouts 
							 * if authentication timeout is N seconds
							 * the SALT_TIMEOUT (or login timeout ) is
							 * N /SALT_TIMEOUT seconds
							 * This is to keep from running out of user_auth
							 * structures if user dosent login sucessfully
							 */

typedef struct ps_auth ps_auth;
typedef struct Ps_server Ps_server;	


/* Stores authentication state variables
 * for each connection 
 */
 struct ps_auth{

	int				state;				/* authentication state of this connection */
	int				countdown;			/* countdown untill this structure is re-claimed */
	unsigned char	ip[IPADDR];					/* ip address of the client */
	char	salt[NUM_SEED][CRYPT_LEN];				/* sent to user to combine with user plaintext */
	struct ps_auth	*next;						/* linked list of structures */

};



#define		AUTH_ENABLED	0x80	/* authentication is enabled */


 /* auth API */

#define		A_ENABLE		1		/* enable authentication */
#define		A_DISABLE		2		/* disable authentication */
#define		A_CREDS			3		/* change credentials */
#define		A_AUTH_URI		4		/* send user who needs authentication here */
#define		A_AUTH_ADD		5		/* register the passed ip address as authenticated */
#define		A_AUTH_GLOBL	6		/* make this directory available w/o authentication */
#define		A_TIMEOUT		7		/* set the authentication timeout value */
#define		A_LOGOUT		8		/* logout the selected ip address */

/* Keys to the server */

typedef struct {
	char		key[CRYPT_LEN+1];			/* encryption key for this server */
	char		auth0[CRYPT_LEN+1];		/* cleartext  first key to server */
	char		auth1[CRYPT_LEN+1];		/* cleartext second key to server */
	char		auth_uri[URI_LEN];
	ps_auth		user_auth[AUTH_MAX_USERS];
	char		auth_public[URI_LEN];	/* directory tree excempt from authentication */
	int			timeoutval;				/* timeout value for user_auth */
	int			last_time;				/* last time value */

	int			flags;					/* mask to declare conditions of match */

}master_auth;


typedef struct {
	char			*name;
	char			*value;
}ps_tok_elem;

typedef struct token Token;

struct token{

	ps_tok_elem		arg;
	Token			*next;
};
 /*	misc per server global data
  * 
  *	this structure will be dynamically
  * allocated per request 
  */

#define OBUFSZ 1024	

struct req_data {

	char		lbuf[RECEIVE_SIZE+2];			/* raw HTTP request from client is stored here */
	Token		heap[TOKEN_HEAP];			/* pool of free token structures */
	char		out_head[RECEIVE_SIZE/2];		/* space to store the response header for this request */
	char		ssi_buf[SSI_LINE];
	char 		obuf[OBUFSZ];				/* output buffer  for ps_net_write() */
};

typedef struct {

	NET_FD			sd;				/* socket descripter for this connection */
	Token			*pg_args;			/* plugin arguments (if request is a POST ) */
	Ps_server		*server;			/* pointer to infor for this server */
	unsigned char	ip[IPADDR];				/* ip address of client */
	short			method;				/* HTTP method of this request GET, POST, HEAD etc */
	short			obufcnt;			/* HTTP method of this request GET, POST, HEAD etc */
	char			* headers;			/* Any header info the client may have sent */
	char			* fname;			/* URI (filename) of the requested entity GET,POST */
	char			* response;			/* the HTTP response to the current request */
	struct ps_stat	* stat;					/* internal stat structure */
	struct req_data * rdata;				/* per request allocated data */
	int	nbytes;						/* number of bytes in input buffer */

}Request;


/* used to map a plugin URI to
 * a plugin entry address
 */
//struct plugin{				
//
//	char		*name;				/* a name in our file-space which when accessed */
//	int			(*plugin)();		/* triggers a call to this function */
//
//};

#define		NOT_FOUND	0			/* no file found */
#define		INCORE		1			/* an incore file...no I/O needed */
#define		NORMAL		2			/* a regular file (can do normal I/O ) */
#define		FOUND		4			/* file was found */
#define		DIRECTORY	8			/* file was found */
#define		PLUGIN		5			/* file was a plugin */
		
struct ps_stat {						/* used to describe file infomation */

	unsigned int	size;
	int				flags;
	char			*address;		/* address if incore file */
	int			(*plugin)();		/* triggers a call to this function */

};


/* Per server infotmation */
 struct Ps_server{

	int					flags;			/* server state */
	struct Ps_server	*next;			/* next server (multiple server support ) */
	int					bytes_in;		/* total bytes recieved */
	int					bytes_out;		/* total bytes sent */
	unsigned char		ip[IPADDR];		/* ip address of the server */
	int					port;			/* port the server listens to (default 80 ) */
	master_auth			master;			/* authentication info for this server */

};

#define DEFAULT_MIME	0

/* used to create the mime_table */
struct mime_tab{
	char * ext;
	char * mime_type;
};


/*
Ps_server * ps_get_server()  ;
void		ps_INIT(void);
int			os_send_file(Request * req);
int			os_read_file(Request * req, char * f);
int			os_stat_file(Request * req);
int			strmatch(char * s, char* d );
char*		substr(char*s, char* d);	
int			ps_net_read(Request * req, char * buf, int sz, int time_out);
void		ps_net_write(Request *req, char * buf, int sz);
void		ps_send_compressed(NET_FD sd,char *start,int length); 
int			parse_http_request(Request * req,char * line);
char		*ps_token_findval(char * name, Token * tok);
void		*ps_malloc( unsigned int size);
void		ps_mfree(void * mem);
void		ps_reg_plugin(int(* plug_in)(), char* uri);

void		server_error( char * reason);
void		ps_proto_status(Request * req, int code);
void		ps_proto_start_response(Request * req);
void		ps_proto_redirect(Request * req, char * url);
char		*ps_proto_uri2url(Request * req, char * uri);
void		ps_outhdr_nninsert( char * name, int value, char * outhdr);
void		ps_outhdr_insert(char * name, char * value, char * outhdr);
void		ps_proto_finish_request(Request * req);
void *      ps_crit_init (void * critvar);
void        ps_crit_exit (void * critvar);
void        ps_crit_enter(void * critvar);
int			ps_a2hex(char * s);
void		ps_decrypt(char * key, char * data, int blocks );
void		ps_encrypt(char * key, char * data, int blocks );
int			ps_gettime(void); 
char		*in_string(char * s, char * l );
void 		 send_client_html_msg(Request * req, char * mes);
int			md_file_stat(Request * req);
int	os_write_fs(Request * req, char * fname, char * filemem, int length);

int	 ps_compress(int mode,char * inbuf, char *outbuf,int length);

int	ps_decompress(Request * req,char * inbuf, char *outbuf,int ilen, int olen);

Ps_server	*ps_get_server(void);

int			ps_auth_ctl(Ps_server* server, int auth_cmd, void* auth);
void		ps_server_ctl(Ps_server* server , int  command,void * arg);

void	ps_net_flush(Request * req);
*/

/* PS_API */
#endif /* _PICO_B */


