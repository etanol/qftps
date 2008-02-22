/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * uftps.h - Common definitions and some platform independence
 */
#ifndef __uftps
#define __uftps

/* Large file support */
#define _FILE_OFFSET_BITS   64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

/* Standard include files */
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Debug support */
#ifdef DEBUG
extern void debug_msg (const char *format, ...);
#else
#define debug_msg(format, ...)
#endif /* DEBUG */

/* Constants */
#define DEFAULT_PORT 2211
#define LINE_SIZE    4096  /* Same value as PATH_MAX */

/* Recognized commands; please keep it sorted with the same criteria as in
 * gendfa.pl */
typedef enum _Command {
        FTP_NONE = 0,
        FTP_ABOR, FTP_ACCT, FTP_ALLO, FTP_APPE, FTP_CDUP, FTP_CWD,  FTP_DELE,
        FTP_FEAT, FTP_HELP, FTP_LIST, FTP_MDTM, FTP_MKD,  FTP_MODE, FTP_NLST,
        FTP_NOOP, FTP_OPTS, FTP_PASS, FTP_PASV, FTP_PORT, FTP_PWD,  FTP_QUIT,
        FTP_REIN, FTP_REST, FTP_RETR, FTP_RMD,  FTP_RNFR, FTP_RNTO, FTP_SITE,
        FTP_SIZE, FTP_SMNT, FTP_STAT, FTP_STOR, FTP_STOU, FTP_STRU, FTP_SYST,
        FTP_TYPE, FTP_USER
} command_t;

/* Buffers */
extern char LineBuf[LINE_SIZE]; /* Incoming command line buffer */
extern char AuxBuf[LINE_SIZE];  /* Auxiliary buffer */

/* chroot() emulation */
extern char *Basedir;
extern int   Basedir_len;

/* Session state information (prefixed by "S_") */
extern int       S_cmd_sk;          /* Control channel */
extern int       S_data_sk;         /* Data channel */
extern int       S_passive_bind_sk; /* Cached data socket for passive mode */
extern int       S_passive_mode;    /* Passive mode flag */
extern long long S_offset;          /* Last REST offset accepted */
extern char      S_passive_str[64]; /* Cached reply for PASV */
extern char     *S_arg;             /* Pointer to comand line argument */


/***************************  FUNCTION PROTOTYPES  ***************************/

void      init_session   (int cmd_sk);        /* init_session.c */
command_t next_command   (void);              /* next_command.c */
void      command_loop   (void);              /* command_loop.c */
void      change_dir     (void);              /* change_dir.c */
void      client_port    (void);              /* client_port.c */
void      send_file      (void);              /* send_file.c */
void      file_stats     (int type);          /* file_stats.c */
void      list_dir       (int full_list);     /* list_dir.c */
void      fatal          (char *msg);         /* misc.c */
void      send_reply     (int sk, char *msg); /*   |    */
int       path_is_secure (char *path);        /*  _/    */


/* Inline utility functions */
static inline char *
expanded_arg (void)
{
        if (S_arg != NULL && S_arg[0] == '/') {
                strncpy(AuxBuf, Basedir, Basedir_len);
                AuxBuf[Basedir_len] = '\0';
                strncat(AuxBuf, S_arg, LINE_SIZE - Basedir_len);
                S_arg = AuxBuf;
        }

        return S_arg;
}

#endif /* __uftps */

