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
*       enc_des.c                                                 
*                                                                       
* COMPONENT                                                             
*            
*       Nucleus WebServ                                                          
*                                                                       
* DESCRIPTION          
*                                                 
*       The functions contained within this file make up the 40 bit key  
*       DES encryption/decryption library.                               
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       ENC_Ip                  Initial Permutation array.       
*       ENC_Fp                  Final permutation array.         
*       ENC_Pc1                 Permutated choice table key.     
*       ENC_Total_Rot           Number of left rotation of pc1.  
*       ENC_Pc2                 Permuted choice key table.       
*       ENC_Si                  The S-box arrays.                
*       ENC_P32i                32 bit permutation function p    
*                               used on the output of the s      
*                               boxes.                           
*       ENC_Sp                  Combined S and P boxes.          
*       ENC_Iperm               Initial Permutation.             
*       ENC_Fperm               Final Permutation.               
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       ENC_Decrypt             decrypts an encrypted message    
*                               using the 40 bit DES encyption   
*                               algorithm.                       
*       ENC_Encrypt             encrypts a message using the 40  
*                               bit DES encyption algorithm.     
*       ENC_DES_Init            Initializes encryption and       
*                               decryption.                      
*       ENC_Set_Key             Initializes the key schedule     
*                               array.                           
*       ENC_Decrypt_Block       In-place decryption of 64-bit    
*                               block.                           
*       ENC_Encrypt_Block       In-place encryption of 64-bit    
*                               block.                           
*       ENC_SP_Init             Initialize the lookup table for  
*                               the combined S and P boxes.      
*       ENC_Perm_Init           Initializes a perm array.        
*       ENC_Byte_Swap           Swaps long word bytes if L.E.    
*       ENC_Coder               The nonlinear function f(r,k).   
*       ENC_Permute             Permutes the inblock with perm.  
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h       
*                                                                       
*************************************************************************/

/* 
 * Nucleus WebServ DES library
 */

#include "networking/nu_websr.h"

#if INCLUDE_DES_AUTH

STATIC VOID     ENC_DES_Init(INT mode);
STATIC INT      ENC_Set_Key(CHAR (*kn)[8], CHAR *key);
STATIC INT      ENC_Decrypt_Block(CHAR (*kn)[8],CHAR *block);
STATIC INT      ENC_Encrypt_Block(CHAR (*kn)[8],CHAR *block);
STATIC VOID     ENC_Permute(CHAR *inblock, CHAR perm[16][16][8], CHAR *outblock);
STATIC VOID     ENC_Perm_Init(CHAR perm[16][16][8], CHAR p[64]);
STATIC VOID     ENC_SP_Init(VOID);
STATIC INT32    ENC_Coder(register long r, register char *subkey);
STATIC INT32    ENC_Byte_Swap(INT32 x);

CHAR    ENC_Ks[16][8];
extern  INT DES_Little_Endian;
extern  WS_SERVER WS_Master_Server;

/* Tables defined in the Data Encryption Standard documents */

/* initial permutation IP */
static CHAR ENC_Ip[] = {
    58, 50, 42, 34, 26, 18, 10,  2,
    60, 52, 44, 36, 28, 20, 12,  4,
    62, 54, 46, 38, 30, 22, 14,  6,
    64, 56, 48, 40, 32, 24, 16,  8,
    57, 49, 41, 33, 25, 17,  9,  1,
    59, 51, 43, 35, 27, 19, 11,  3,
    61, 53, 45, 37, 29, 21, 13,  5,
    63, 55, 47, 39, 31, 23, 15,  7
};

/* final permutation IP^-1 */
static CHAR ENC_Fp[] = {
    40,  8, 48, 16, 56, 24, 64, 32,
    39,  7, 47, 15, 55, 23, 63, 31,
    38,  6, 46, 14, 54, 22, 62, 30,
    37,  5, 45, 13, 53, 21, 61, 29,
    36,  4, 44, 12, 52, 20, 60, 28,
    35,  3, 43, 11, 51, 19, 59, 27,
    34,  2, 42, 10, 50, 18, 58, 26,
    33,  1, 41,  9, 49, 17, 57, 25
};

/* permuted choice table (key) */
static CHAR ENC_Pc1[] = {
    57, 49, 41, 33, 25, 17,  9,
     1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,

    63, 55, 47, 39, 31, 23, 15,
     7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of pc1 */
static CHAR ENC_Total_Rot[] = {
    1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

/* permuted choice key (table) */
static CHAR ENC_Pc2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

/* The (in)famous S-boxes */
static CHAR ENC_Si[8][64] = {
    /* S1 */
    {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
     0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
     4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
     15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13},

    /* S2 */
    {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
     3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
     0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
     13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9},

    /* S3 */
    {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
    13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
    13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
    1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12},

    /* S4 */
    { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
    13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
    3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14},

    /* S5 */
    { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
    14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
     4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
     11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3},

    /* S6 */
    {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
    10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
     9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
     4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13},

    /* S7 */
    { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
    13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
     1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
     6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12},

    /* S8 */
    {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
     1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
     7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
     2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11}
};

/* 32-bit permutation function P used on the output of the S-boxes */
static CHAR ENC_P32i[] = {  
    16,  7, 20, 21,
    29, 12, 28, 17,
     1, 15, 23, 26,
     5, 18, 31, 10,
     2,  8, 24, 14,
    32, 27,  3,  9,
    19, 13, 30,  6,
    22, 11,  4, 25
};
/* End of DES-defined tables */

/* Lookup tables initialized once only at startup by ENC_DES_Init() */
static INT32 (*ENC_Sp)[64];                             /* Combined S and P boxes */

static CHAR (*ENC_Iperm)[16][8];                        /* Initial and final permutations */
static CHAR (*ENC_Fperm)[16][8];


/* bit 0 is left-most in byte */
static INT ENC_Byte_Bit[] = {
    0200,0100,040,020,010,04,02,01
};

static INT ENC_Nibble_Bit[] = {
     010,04,02,01
};

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Decrypt                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function decrypts an encrypted message using the 40 bit DES 
*       encyption algorithm.                                             
*                                                                       
* INPUTS                                                                
*                                                                       
*       *des_key                Key used for  decryption.        
*       *cypher_text            Encrypted text to be decrypted.  
*       length                  Length in Bytes.                 
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID ENC_Decrypt(CHAR *des_key, CHAR *cypher_text, INT length)
{
    INT     c, cnt;
    INT     i;
    CHAR    key[8];
                     
    for(i=0; i<8; i++)
        key[i] = des_key[i];

    /* Set up key, determine parity bit */
    for(cnt = 0; cnt < 8; cnt++)
    {
        c = 0;
        for(i=0;i<7;i++)
            if(key[cnt] & (1 << i))
                c++;
        if((c & 1) == 0)
            key[cnt] |= 0x80;
        else
            key[cnt] &= ~0x80;
    }

    for(i=0; i<8;i+=2)
        key[i] &= 0xf0;

    ENC_DES_Init(0);
    ENC_Set_Key(ENC_Ks,key);

    /* decrypt blocks of eight bytes 
     * result to be written back into
     * input array
     */
    for(i = 0; i < length; i++ )
    {
        ENC_Decrypt_Block(ENC_Ks, cypher_text + i * 8);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Encrypt                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function encrypts a message using the 40 bit DES            
*       encyption algorithm.                                             
*                                                                       
* INPUTS                                                                
*                                                                       
*       *des_key                Key used for  encryption.        
*       *cypher_text            Random Number to be encrypted.   
*       length                  Length in Bytes.                 
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID ENC_Encrypt(CHAR *des_key, CHAR *cypher_text, INT length)
{
    INT     c, cnt;
    INT     i;
    CHAR    key[8];
                         
    for(i=0; i<8; i++)
        key[i] = des_key[i];

    /* Set up key, determine parity bit */
    for(cnt = 0; cnt < 8; cnt++)
    {
        c = 0;
        for(i=0;i<7;i++)
            if(key[cnt] & (1 << i))
                c++;
        if((c & 1) == 0)
            key[cnt] |= 0x80;
        else
            key[cnt] &= ~0x80;
    }

    for(i=0; i<8;i+=2)
        key[i] &= 0xf0;

    ENC_DES_Init(0);
    ENC_Set_Key(ENC_Ks,key);

    /* encrypt blocks of eight bytes 
     * result to be written back into
     * input array
     */

    for(i = 0; i < length ; i ++ )
    {
        ENC_Encrypt_Block(ENC_Ks, cypher_text + i*8);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_DES_Init                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Allocate space and initialize DES lookup arrays.                 
*       mode == 0: standard Data Encryption Algorithm.                   
*       mode == 1: DEA without initial and final permutations for speed  
*                                                                       
* INPUTS                                                                
*                                                                       
*       mode                    Defines encyption algorithm.     
*                               0= standard Data Encryption      
*                               Algorithm.                    
*                               1= DEA without initial and final 
*                               permutations for speed        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       Returns zero if already initialized, ENC_Sp equals NULL, mode is 1,  
*       or completes the function normally.                                                                            
*       Returns minus one if either the ENC_Iperm or the ENC_Fperm arrays        
*       are NULL.                                                        
*                                                                       
*************************************************************************/
STATIC VOID ENC_DES_Init(INT mode)
{
    STATUS  status;

    if(ENC_Sp != NU_NULL)
    {
        /* Already initialized */
        return;
    }

    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, (VOID**)&ENC_Sp,
                                sizeof(INT32) * 8 * 64, NU_NO_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return;
    }
    ENC_SP_Init();
    if(mode == 1)   /* No permutations */
        return;

    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, (VOID**)&ENC_Iperm,
                                sizeof(CHAR) * 16 * 16 * 8, NU_NO_SUSPEND);
    if(status != NU_SUCCESS)
    {
        if(NU_Deallocate_Memory(ENC_Sp) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        return;
    }
    ENC_Perm_Init(ENC_Iperm, ENC_Ip);

    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, (VOID**)&ENC_Fperm,
                                sizeof(CHAR) * 16 * 16 * 8, NU_NO_SUSPEND);
    if(status != NU_SUCCESS) 
    {
        if(NU_Deallocate_Memory(ENC_Sp) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        if(NU_Deallocate_Memory(ENC_Iperm) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        return;
    }
    ENC_Perm_Init(ENC_Fperm, ENC_Fp);
    
    return;
}

#ifdef TEST_OPTION
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Des_Done                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function frees up storage used by DES.                      
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
VOID ENC_Des_Done()
{
    if(ENC_Sp == NU_NULL)
        return; /* Already done */

    NU_Deallocate_Memory(ENC_Sp);
    if(ENC_Iperm != NU_NULL)
        NU_Deallocate_Memory(ENC_Iperm);
    if(ENC_Fperm != NU_NULL)
        NU_Deallocate_Memory(ENC_Fperm);

    ENC_Sp = NU_NULL;
    ENC_Iperm = NU_NULL;
    ENC_Fperm = NU_NULL;
}
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Set_Key                                                           
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This Function Initializes the key schedule array.                
*                                                                       
* INPUTS                                                                
*                                                                       
*       *kn                     Key Schedule.                                   
*       *key                    Key used for encryption/         
*                               decryption.  Uses only 56 bits.  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Success
*       -1                      The key schedule is NULL                
*                                                                       
*************************************************************************/
STATIC INT ENC_Set_Key(CHAR (*kn)[8], CHAR *key)
{
    CHAR            pc1m[56];                                   /* place to modify pc1 into */
    CHAR            pcr[56];                                    /* place to rotate pc1 into */
    register int    i, j, l;
    INT             m;

    if(kn == NU_NULL)
    {
        return WS_FAILURE;
    }

    /* Clear key schedule */
    UTL_Zero((CHAR *)kn,16*8);

    for (j=0; j<56; j++) 
    {       /* convert pc1 to bits of key */
         
        l = ENC_Pc1[j]-1;                                 /* integer bit location */
        m = l & 07;                                 /* find bit */
        pc1m[j] = (key[l>>3] &                        /* find which key byte l is in */
                 ENC_Byte_Bit[m])                        /* and which bit of that byte */
                 ? 1 : 0;                           /* and store 1-bit result */
    }
    for (i=0; i<16; i++)
    {       /* key chunk for each iteration */

        for (j=0; j<56; j++)                        /* rotate pc1 the right amount */

            pcr[j] = pc1m[(l = j + ENC_Total_Rot[i]) < (j < 28? 28 : 56) ? l: l - 28];
            /* rotate left and right halves independently */

        for (j=0; j<48; j++)
        {   /* select bits individually */
            /* check bit that goes to kn[j] */
            if (pcr[ENC_Pc2[j] - 1])
            {
                /* mask it in if it's there */
                l=j % 6;
                kn[i][j/6] |= ENC_Byte_Bit[l] >> 2;
            }
        }
    }

    return (NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Encrypt_Block                                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function calculates In-place encryption of 64-bit Block.    
*       The function accomplishes this by by encrypting blocks of        
*       eight bytes.                                                     
*                                                                       
* INPUTS                                                                
*                                                                       
*       *block                  Block of eight bytes.            
*       *kn                     Key Schedule.                    
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Success
*       -1                      The key schedule is NULL
*                                                                       
************************************************************************/
STATIC INT ENC_Encrypt_Block(CHAR (*kn)[8], CHAR *block)
{
    register long   left, right;
    register char   *knp;
    INT32           work[2];                        /* Working data storage */

    if(kn == NU_NULL || block == NU_NULL)
        return WS_FAILURE;
    ENC_Permute(block, ENC_Iperm, (CHAR *)work);    /* Initial Permutation */

    if(DES_Little_Endian)
    {
        left = ENC_Byte_Swap(work[0]);
        right = ENC_Byte_Swap(work[1]);
    }else{
        left = work[0];
        right = work[1];
    }

    /* Do the 16 rounds.
     * The rounds are numbered from 0 to 15. On even rounds
     * the right half is fed to f() and the result exclusive-ORs
     * the left half; on odd rounds the reverse is done.
     */
    knp = &kn[0][0];
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);
    knp += 8;
    left ^= ENC_Coder(right,knp);
    knp += 8;
    right ^= ENC_Coder(left,knp);

    /* Left/right half swap, plus byte swap if little-endian */
    if(DES_Little_Endian) 
    {
        work[1] = ENC_Byte_Swap(left);
        work[0] = ENC_Byte_Swap(right);
    }else{
        work[0] = right;
        work[1] = left;
    }

    ENC_Permute((CHAR *)work, ENC_Fperm, (CHAR *) block);  /* Inverse initial permutation */
    return (NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Decrypt_Block                                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       In-place decryption of 64-bit block. This function               
*       is the mirror image of encryption; exactly the same              
*       steps are taken, but in reverse order.                           
*                                                                       
* INPUTS                                                                
*                                                                       
*       *block                  Block of eight bytes.            
*       *kn                     Key Schedule.                    
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Success
*       -1                      The key schedule is NULL
*                                                                       
*************************************************************************/
STATIC INT ENC_Decrypt_Block(CHAR (*kn)[8], CHAR *block)
{
    register long   left, right;
    register char   *knp;
    INT32           work[2];            /* Working data storage */

    if(kn == NU_NULL || block == NU_NULL)
        return -1;
    ENC_Permute(block, ENC_Iperm, (CHAR *)work);              /* Initial permutation */

    /* Left/right half swap, plus byte swap if little-endian */

    if(DES_Little_Endian)
    {
        left = ENC_Byte_Swap(work[1]);
        right = ENC_Byte_Swap(work[0]);
    }else{
        left = work[1];
        right = work[0];
    }
    /* Do the 16 rounds in reverse order.
     * The rounds are numbered from 15 to 0. On even rounds
     * the right half is fed to f() and the result exclusive-ORs
     * the left half; on odd rounds the reverse is done.
     */
    knp = &kn[15][0];
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);
    knp -= 8;
    right ^= ENC_Coder(left,knp);
    knp -= 8;
    left ^= ENC_Coder(right,knp);

    /* Left/right half swap, plus byte swap if little-endian */
    if(DES_Little_Endian) 
    {
        work[0] = ENC_Byte_Swap(left);
        work[1] = ENC_Byte_Swap(right);
    }else{
        work[1] = right;
        work[0] = left;
    }

    ENC_Permute((CHAR *)work, ENC_Fperm, block);              /* Inverse initial permutation */
    return (NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Permute                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function permutes inblock with perm.                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       *inblock                Input of 64 bits.                
*       perm                    2K bytes of defining permutation.
*       *outblock
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       outblock                Result of 64bits.                
*                                                                       
*************************************************************************/
STATIC VOID ENC_Permute(CHAR *inblock, CHAR perm[16][16][8], CHAR *outblock)
{
    register char   *ib, *ob;   /* ptr to input or output block */
    register char   *p, *q;
    register int    j;

    if(perm == NU_NULL)
    {
        /* No permutation, just copy */
        WS_Mem_Cpy(outblock, inblock, 8);
        return;
    }
    
    /* Clear output block */
    UTL_Zero(outblock, 8);

    ib = inblock;
    for (j = 0; j < 16; j += 2, ib++) 
    { 
        /* for each input nibble */
        ob = outblock;
        p = perm[j][(*ib >> 4) & 0xf];
        q = perm[j + 1][*ib & 0xf];
        
        /* and each output byte, OR the masks together */
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob   |= *p   | *q;
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Coder                                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The nonlinear function f(r,k), the heart of DES.                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       r                       32 bit value.                    
*       *subkey                 48 bit key.                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       rval                    The result of E(R)^K.            
*                                                                       
*************************************************************************/
STATIC INT32 ENC_Coder(register long r, register char *subkey)
{
    register long   *spp;
    register long   rval, rt;
    register int    er;

#ifdef  TRACE
    printf("f(%08lx, %02x %02x %02x %02x %02x %02x %02x %02x) = ",
        r,
        subkey[0], subkey[1], subkey[2],
        subkey[3], subkey[4], subkey[5],
        subkey[6], subkey[7]);
#endif
    /* Run E(R) ^ K through the combined S & P boxes.
     * This code takes advantage of a convenient regularity in
     * E, namely that each group of 6 bits in E(R) feeding
     * a single S-box is a contiguous segment of R.
     */
    subkey += 7;

    /* Compute E(R) for each block of 6 bits, and run thru boxes */
    er = ((INT)r << 1) | ((r & 0x80000000ul) ? 1 : 0);
    spp = &ENC_Sp[7][0];
    rval = spp[(er ^ *subkey--) & 0x3f];
    spp -= 64;
    rt = (UINT32)r >> 3;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((INT)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rt |= (r & 1) << 5;
    rval |= spp[((INT)rt ^ *subkey) & 0x3f];
#ifdef  TRACE
    printf(" %08lx\n",rval);
#endif
    return rval;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Perm_Init                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function initializes a perm array.                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       perm                    64-bit, either initial  or final 
*                               permutation.                     
*       p                       Initial or final permutation     
*                               array pointer.                   
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID ENC_Perm_Init(CHAR perm[16][16][8], CHAR p[64])
{
    register int    l, j, k;
    INT             i,m;

    /* Clear the permutation array */
    UTL_Zero((CHAR *)perm,16*16*8);

    for (i=0; i<16; i++)                            /* each input nibble position */
        for (j = 0; j < 16; j++)                    /* each possible input nibble */
            for (k = 0; k < 64; k++)                /* each output bit position */
            {   
                l = p[k] - 1;                       /* where does this bit come from*/
                if ((l >> 2) != i)                  /* does it come from input posn?*/
                    continue;                       /* if not, bit k is 0 */
                if (!(j & ENC_Nibble_Bit[l & 3]))
                    continue;                       /* any such bit in input? */
                m = k & 07;                         /* which bit is this in the byte*/
                perm[i][j][k>>3] |= ENC_Byte_Bit[m];
            }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_SP_Init                                                           
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function initializes the lookup table for combined S and P  
*       boxes.                                                           
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
STATIC VOID ENC_SP_Init(VOID)
{
    CHAR    pbox[32];
    INT     p, i, s, j, rowcol;
    INT32   val;

    /* 
     * Compute pbox, the inverse of ENC_P32i.
     * This is easier to work with
     */
    for(p = 0;p < 32;p++)
    {
        for(i = 0;i < 32;i++)
        {
            if(ENC_P32i[i] - 1 == p)
            {
                pbox[p] = (CHAR)i;
                break;
            }
        }
    }

    for(s = 0; s < 8; s++)
    {           
        /* For each S-box */
        for(i=0; i<64; i++)
        {       
            /* For each possible input */
            val = 0;

            /* The row number is formed from the first and last
             * bits; the column number is from the middle 4
             */
            rowcol = (i & 32) | ((i & 1) ? 16 : 0) | ((i >> 1) & 0xf);
            for(j=0;j<4;j++)
            {   
                /* For each output bit */
                if(ENC_Si[s][rowcol] & (8 >> j))
                {
                    val |= 1L << (31 - pbox[4*s + j]);
                }
            }

            ENC_Sp[s][i] = val;
        }
    }
}

/* Byte swap a long */

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       ENC_Byte_Swap                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function is only used if it the target platform is Little   
*       Endian.  This function swaps the bytes to conform to the Little  
*       Endian standard.                                                 
*                                                                       
* INPUTS                                                                
*                                                                       
*       x                       The value to be byte swapped.    
*                                                                       
* OUTPUTS                                                               
*                                                                        
*       x                       The byte swapped value.          
*                                                                       
*************************************************************************/
STATIC INT32 ENC_Byte_Swap(INT32 x)
{
    register char *cp, tmp;

    cp = (CHAR *)&x;
    tmp = cp[3];
    cp[3] = cp[0];
    cp[0] = tmp;

    tmp = cp[2];
    cp[2] = cp[1];
    cp[1] = tmp;

    return x;
}

#endif /* INCLUDE_DES_AUTH */
