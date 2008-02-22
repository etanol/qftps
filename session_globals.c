/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * session_globals.c - Session state variables.
 *
 * As each child process only attends to one client code doesn't need to be
 * thread safe.
 */

#include "uftps.h"

/*
 * Buffers
 */
char LineBuf[LINE_SIZE]; /* Incoming commands buffer */
char AuxBuf[LINE_SIZE];  /* Auxiliary buffer */

/*
 * chroot() emulation
 */
char *Basedir;
int   Basedir_len;        /* Size of the hidden working directory path */

/*
 * Session information (prefixed by "S_")
 */
int       S_cmd_sk;           /* Control channel */
int       S_data_sk;          /* Data channel */
int       S_passive_bind_sk;  /* Cached data socket for passive mode */
int       S_passive_mode;     /* Passive mode flag */
long long S_offset;           /* Last REST offset accepted */
char      S_passive_str[64];  /* Cached reply for PASV */
char     *S_arg;              /* Current command's argument */

