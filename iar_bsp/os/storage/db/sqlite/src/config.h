#include "nucleus.h"

/* Enable Nucleus Mutex and VFS subsystem. */
#define  SQLITE_OS_OTHER 1
#define  SQLITE_OS_NUCLEUS 1

/* Enable Nucleus Memory subsystem. */
#define  SQLITE_NUCLEUS_MALLOC 1

/* Disable floating point support. */
#define  SQLITE_OMIT_FLOATING_POINT 1

/* Disable auto inititialization of SQLite by itself. */
#define  SQLITE_OMIT_AUTOINIT 1

/* Enable SQLite to use soundex algorithm. */
#if CFG_NU_OS_STOR_DB_SQLITE_SQL_SOUNDEX
#define SQLITE_SOUNDEX
#endif

/* The maximum length of a TEXT or BLOB in bytes. */
#define SQLITE_MAX_LENGTH CFG_NU_OS_STOR_DB_SQLITE_SQL_MAX_LENGTH

/* The maximum number of bytes in the text of an SQL statement. */
#define SQLITE_MAX_SQL_LENGTH CFG_NU_OS_STOR_DB_SQLITE_SQL_MAX_SQL_LENGTH

/* Disable support for File Size larger then 2GB*/
#define SQLITE_DISABLE_LFS

/* Define to the full name of this package. */
#define PACKAGE_NAME "sqlite"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "sqlite 3.7.14.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "sqlite"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.7.14.1"

