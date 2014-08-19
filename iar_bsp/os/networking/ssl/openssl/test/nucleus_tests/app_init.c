/************************************************************************
*
*   FILENAME
*                                                                                 
*       app_init.c
*
*
*************************************************************************/

#include "nucleus.h"
#include "storage/pcdisk.h"
#include "networking/nu_net.h"
#include "e_os.h"
#include "os/kernel/plus/supplement/inc/error_management.h"


#define BYTES_1_K 1024
#define TASK_STACK_SIZE (BYTES_1_K * 40)

/* Define Application data structures.  */

extern NU_MEMORY_POOL      System_Memory;

#include "evptests_txt.h"
#include "root_pem.h" 
#include "server_pem.h"
#include "client_pem.h"

#include "openssl/des.h"
#include "openssl/blowfish.h"
#include "openssl/cast.h"
#include "openssl/aes.h"


static NU_TASK gMainTask;

static VOID Main_Task(UNSIGNED argc, VOID *argv);

extern int main_bftest(int, char **);
extern int main_bntest(int, char **);
extern int main_casttest(int, char **);
extern int main_destest(int, char **);
extern int main_dhtest(int, char **);
extern int main_dsatest(int, char **);
extern int main_ecdhtest(int, char **);
extern int main_ecdsatest(int, char **);
extern int main_ectest(int, char **);
extern int main_enginetest(int, char **);
extern int main_evp_test(int, char **);
extern int main_hmactest(int, char **);
extern int main_md4test(int, char **);
extern int main_md5test(int, char **);
extern int main_mdc2test(int, char **);
extern int main_randtest(int, char **);
extern int main_rc2test(int, char **);
extern int main_arc4test(int, char **);
extern int main_rmdtest(int, char **);
extern int main_rsa_test(int, char **);
extern int main_sha1test(int, char **);
extern int main_sha256t(int, char **);
extern int main_sha512t(int, char **);
extern int main_shatest(int, char **);
extern int main_wp_test(int, char **);
extern int main_ssltest(int, char **);

/******************************************************************************
*
*   FUNCTION
*
*      Application_Initialize
*
*   DESCRIPTION
*
*      Define the Application_Initialize routine that determines the
*      initial Nucleus PLUS application environment.
*
******************************************************************************/

VOID Application_Initialize(NU_MEMORY_POOL *system_pool, 
                            NU_MEMORY_POOL *uncached_system_pool)
{
    STATUS status;
    VOID*  ptr;

    UNUSED_PARAMETER(system_pool);
    UNUSED_PARAMETER(uncached_system_pool);

    /* Create Main task. */
    
    status = NU_Allocate_Memory(&System_Memory, &ptr, TASK_STACK_SIZE, NU_NO_SUSPEND);
    if(status != NU_SUCCESS)
    {
        printf("Can not create memory for Main task.\r\n");
        ERC_System_Error(status);
    }

    status = NU_Create_Task(&gMainTask, "main", Main_Task, 0, NU_NULL,
                            ptr, TASK_STACK_SIZE, 5, 0, NU_PREEMPT, NU_START);
    if(status != NU_SUCCESS)
    {
        printf("Cannot create Main_Task.\r\n");
        ERC_System_Error(status);
    }

} /* Application_Initialize */

/******************************************************************************
*
*   FUNCTION
*
*      Prepare_Test_Files
*
*   DESCRIPTION
*
*      This function prepares the test files on the file system.
*
******************************************************************************/

STATUS Prepare_Test_Files(VOID)
{
    typedef struct _table
    {
        const CHAR* file_name;
        const CHAR* file_data;
        INT         file_size;
    } FILE_TABLE; 

    const FILE_TABLE test_files[] = {
                                     {"evptests.txt", evptests_txt, sizeof(evptests_txt)},
                                     {"root.pem", root_pem, sizeof(root_pem)},
                                     {"server.pem", server_pem, sizeof(server_pem)},
                                     {"client.pem", client_pem, sizeof(client_pem)}
                                    };
    #define NUM_ENTRIES 4

    STATUS stat;
    CHAR* disk = "A:\\";
    const INT16 drive = 0;
    INT fd;
    INT i;
    INT errors = 0;

    stat = NU_Become_File_User();
    if (stat != NU_SUCCESS)
        return stat;

    stat = NU_Open_Disk(disk);
    if (stat != NU_SUCCESS)
        return stat;
    
    stat = NU_Set_Default_Drive(drive);
    if (stat != NU_SUCCESS)
        return stat;

    stat = NU_Set_Current_Dir (disk);
    if (stat != NU_SUCCESS)
        return stat;

    for (i = 0; i < NUM_ENTRIES; i++)
    {
        fd = NU_Open((CHAR*) test_files[i].file_name, PO_WRONLY|PO_CREAT|PO_BINARY, PS_IWRITE);
        if (fd < 0)
        {
            --errors;
            continue;
        }
    
        stat = NU_Write(fd, (CHAR*) test_files[i].file_data, test_files[i].file_size);

        NU_Close(fd);
    }

    /* Don't close the disk, as it will be used by the tests. */
    /*NU_Close_Disk(disk);
    NU_Release_File_User();*/

    return errors;
 
} /* Prepare_Test_Files */

/******************************************************************************
*
*   FUNCTION
*
*      Functional_Tests
*
*   DESCRIPTION
*
*      It calls the main test functions.
*
******************************************************************************/

VOID Functional_Tests(VOID)
{
    INT ret, num_tests, errs;
    CHAR* evp_test_args[] = {"evp_test",  "evptests.txt", NULL};
    CHAR* ssltest_args[]  = {"ssltest",
                             "-key", "server.pem",
                             "-cert", "server.pem",
                             "-c_key", "client.pem",
                             "-c_cert", "client.pem",
                             NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL};

    ret = Prepare_Test_Files();
    if (ret != NU_SUCCESS)
    {
        printf("Error preparing test files. Some tests may fail!\n");
    }

    printf("\n----- OpenSSL Unit Tests -----\n");
    printf("\nNOTE: Some of these tests really take some time, so please be patient.\n");

    num_tests = 0;
    errs = 0;

    ret = main_bftest(0, NU_NULL);
    printf("BF_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_bntest(0, NU_NULL);
    printf("BN_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_casttest(0, NU_NULL);
    printf("Cast_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_destest(0, NU_NULL);
    printf("DES_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_dhtest(0, NU_NULL);
    printf("DH_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_dsatest(0, NU_NULL);
    /* This test uses "1" as success. */
    printf("DSA_test %s...\n", (ret == 1) ? "OK" : "failed");
    num_tests++;
    errs += !ret;

    ret = main_ecdhtest(0, NU_NULL);
    printf("ECDH_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_ecdsatest(0, NU_NULL);
    printf("ECDSA_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_ectest(0, NU_NULL);
    printf("EC_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_enginetest(0, NU_NULL);
    printf("Engine_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_evp_test(2, evp_test_args);
    printf("EVP_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_hmactest(0, NU_NULL);
    printf("HMAC_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_md4test(0, NU_NULL);
    printf("MD4_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_md5test(0, NU_NULL);
    printf("MD5_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_mdc2test(0, NU_NULL);
    printf("MDC2_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_randtest(0, NU_NULL);
    printf("Rand_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_rc2test(0, NU_NULL);
    printf("RC2_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_arc4test(0, NU_NULL);
    printf("ARC4_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_rmdtest(0, NU_NULL);
    printf("RMD_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_rsa_test(0, NU_NULL);
    printf("RSA_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_sha1test(0, NU_NULL);
    printf("SHA1_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_sha256t(0, NU_NULL);
    printf("SHA256_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_sha512t(0, NU_NULL);
    printf("SHA512_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_shatest(0, NU_NULL);
    printf("SHA_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_wp_test(0, NU_NULL);
    printf("WP_test %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9] = "-test_cipherlist";
    ret = main_ssltest(10, ssltest_args);
    printf("SSL_test (args: -test_cipherlist) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9] = "-ssl2";
    ret = main_ssltest(10, ssltest_args);
    printf("SSL_test (args: -ssl2) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl2";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (args: -ssl2 -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl2";
    ssltest_args[10] = "-client_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (args: -ssl2 -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl2";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -ssl2 -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9] = "-ssl3";
    ret = main_ssltest(10, ssltest_args);
    printf("SSL_test (args: -ssl3) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl3";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (args: -ssl3 -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl3";
    ssltest_args[10] = "-client_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (args: -ssl3 -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-ssl3";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -ssl3 -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ret = main_ssltest(9, ssltest_args);
    printf("SSL_test (ssl2/3) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-server_auth";
    ssltest_args[10] = "-CAfile";
    ssltest_args[11] = "root.pem";
    ret = main_ssltest(12, ssltest_args);
    printf("SSL_test (ssl2/3, args: -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-client_auth";
    ssltest_args[10] = "-CAfile";
    ssltest_args[11] = "root.pem";
    ret = main_ssltest(12, ssltest_args);
    printf("SSL_test (ssl2/3, args: -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-server_auth";
    ssltest_args[10] = "-client_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (ssl2/3, args: -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl2";
    ret = main_ssltest(11, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl2) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl2";
    ssltest_args[11] = "-server_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl2 -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl2";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl2 -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl2";
    ssltest_args[11] = "-server_auth";
    ssltest_args[12] = "-client_auth";
    ssltest_args[13] = "-CAfile";
    ssltest_args[14] = "root.pem";
    ret = main_ssltest(15, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl2 -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl3";
    ret = main_ssltest(11, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl3) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl3";
    ssltest_args[11] = "-server_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl3 -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl3";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl3 -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-ssl3";
    ssltest_args[11] = "-server_auth";
    ssltest_args[12] = "-client_auth";
    ssltest_args[13] = "-CAfile";
    ssltest_args[14] = "root.pem";
    ret = main_ssltest(15, ssltest_args);
    printf("SSL_test (args: -bio_pair -ssl3 -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-no_dhe";
    ret = main_ssltest(11, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -no_dhe) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-dhe1024dsa";
    ssltest_args[11] = "-v";
    ret = main_ssltest(12, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -dhe1024dsa -v) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -server_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-client_auth";
    ssltest_args[11] = "-CAfile";
    ssltest_args[12] = "root.pem";
    ret = main_ssltest(13, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -ssl3 -server_auth -client_auth) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-server_auth";
    ssltest_args[11] = "-client_auth";
    ssltest_args[12] = "-CAfile";
    ssltest_args[13] = "root.pem";
    ssltest_args[14] = "-app_verify";
    ret = main_ssltest(15, ssltest_args);
    printf("SSL_test (ssl2/3, args: -bio_pair -ssl3 -server_auth -client_auth -app_verify) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-tls1";
    ssltest_args[10] = "-cipher";
    ssltest_args[11] = "PSK";
    ssltest_args[12] = "-psk";
    ssltest_args[13] = "abc123";
    ret = main_ssltest(14, ssltest_args);
    printf("SSL_test (args: -tls1 -cipher PSK -psk abc123) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-tls1";
    ssltest_args[11] = "-cipher";
    ssltest_args[12] = "PSK";
    ssltest_args[13] = "-psk";
    ssltest_args[14] =  "abc123";
    ret = main_ssltest(15, ssltest_args);
    printf("SSL_test (args: -bio_pair -tls1 -cipher PSK -psk abc123) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    ssltest_args[9]  = "-bio_pair";
    ssltest_args[10] = "-tls1";
    ssltest_args[11] = "-cipher";
    ssltest_args[12] = "ADH";
    ssltest_args[13] = "-dhe1024dsa";
    ssltest_args[14] = "-num";
    ssltest_args[15] = "10";
    ssltest_args[16] = "-f";
    ssltest_args[17] = "-time";
    ssltest_args[18] = "-v";
    ret = main_ssltest(19, ssltest_args);
    printf("SSL_test (args: -bio_pair -tls1 -cipher ADH -dhe1024dsa -num 10 -f -time -v) %s...\n", (ret == 0) ? "OK" : "failed");
    num_tests++;
    errs += ret;

    printf("\nTotal %d tests passed, %d failed...\n", num_tests, errs);

} /* Functional_Tests */

/******************************************************************************
*
*   FUNCTION
*
*      Clock_Diff
*
*   DESCRIPTION
*
*      It calculates difference between two time values, keeping in mind
*      the roll-over case.
*
******************************************************************************/

static inline UNSIGNED Clock_Diff(UNSIGNED a, UNSIGNED b)
{
    if (a > b) return (a - b);
    
    return ( ~(b - a) + 1 );  
}

/******************************************************************************
*
*   FUNCTION
*
*      Timing_Tests
*
*   DESCRIPTION
*
*      It calculates execution times for some algorithms.
*
******************************************************************************/

static VOID Timing_Tests(VOID)
{
    UINT8 in[4096];
    UINT8 out[4096];
    UINT8 iv[16];
    UINT8 key[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
    INT lp_indx;
    UNSIGNED ti,tf; 
    DES_key_schedule des_key;
    BF_KEY bf_key;
    CAST_KEY cast_key;
    AES_KEY aes_key;
    const INT NUM_ITER = 1000;

    /* DES timing */

    DES_set_key_checked((const_DES_cblock*)key, &des_key);

    ti = NU_Retrieve_Clock();

    for (lp_indx = 0; lp_indx < NUM_ITER; lp_indx++)
    {
        DES_cbc_encrypt(in, out, sizeof(in), &des_key, (DES_cblock*)iv, DES_ENCRYPT);
    }

    tf = NU_Retrieve_Clock();

    printf("\nDES CBC encryption timing : %ld\n", Clock_Diff(tf, ti));

    /* 3DES timing */

    ti = NU_Retrieve_Clock();

    for (lp_indx = 0; lp_indx < NUM_ITER; lp_indx++)
    {
        DES_ede3_cbc_encrypt(in, out, sizeof(in), &des_key, &des_key, &des_key, (DES_cblock*)iv, DES_ENCRYPT);
    }

    tf = NU_Retrieve_Clock();

    printf("\n3DES CBC encryption timing : %ld\n", Clock_Diff(tf, ti));

    /* BF timing */

    BF_set_key(&bf_key, sizeof(key), key);

    ti = NU_Retrieve_Clock();

    for (lp_indx = 0; lp_indx < NUM_ITER; lp_indx++)
    {
        BF_cbc_encrypt(in, out, sizeof(in), &bf_key, iv, BF_ENCRYPT);
    }

    tf = NU_Retrieve_Clock();

    printf("\nBF CBC encryption timing : %ld\n", Clock_Diff(tf, ti));

    /* Cast-128 timing */

    CAST_set_key(&cast_key, sizeof(key), key);

    ti = NU_Retrieve_Clock();

    for (lp_indx = 0; lp_indx < NUM_ITER; lp_indx++)
    {
        CAST_cbc_encrypt(in, out, sizeof(in), &cast_key, iv, CAST_ENCRYPT);
    }

    tf = NU_Retrieve_Clock();

    printf("\nCast-128 CBC encryption timing : %ld\n", Clock_Diff(tf, ti));

    /* AES timing */

    AES_set_encrypt_key(key, sizeof(key)*8, &aes_key);

    ti = NU_Retrieve_Clock();

    for (lp_indx = 0; lp_indx < NUM_ITER; lp_indx++)
    {
        AES_cbc_encrypt(in, out, sizeof(in), &aes_key, iv, AES_ENCRYPT);
    }

    tf = NU_Retrieve_Clock();

    printf("\nAES CBC encryption timing : %ld\n", Clock_Diff(tf, ti));

} /* Timing_Tests */

/******************************************************************************
*
*   FUNCTION
*
*      Main_Task
*
*   DESCRIPTION
*
*      It calls test functions.
*
******************************************************************************/

static VOID Main_Task(UNSIGNED argc, VOID *argv)
{
    Functional_Tests();

    Timing_Tests();

    while(1)
    {
        NU_Sleep(NU_PLUS_TICKS_PER_SEC);
    }

} /* Main_Task */
