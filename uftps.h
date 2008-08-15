/*
 * User FTP Server,  Share folders over FTP without being root.
 * Copyright (C) 2008  Isaac Jurado
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
 * Common definitions and some platform independence
 */

/* Large file support */
#define _FILE_OFFSET_BITS   64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stddef.h>
#include <string.h>

#ifndef __GNUC__
/* Attributes help to catch silly mistakes, but they are not always available */
#  define __attribute__(x)
#endif

/* Constants */
#define DEFAULT_PORT 2211
#define LINE_SIZE    4096  /* Same value as PATH_MAX */

/* Recognized commands; please keep it sorted with the same criteria as in
 * gendfa.pl */
enum command
{
        FTP_NONE = 0,
        FTP_ABOR, FTP_ACCT, FTP_ALLO, FTP_APPE, FTP_CDUP, FTP_CWD,  FTP_DELE,
        FTP_FEAT, FTP_HELP, FTP_LIST, FTP_MDTM, FTP_MKD,  FTP_MODE, FTP_NLST,
        FTP_NOOP, FTP_OPTS, FTP_PASS, FTP_PASV, FTP_PORT, FTP_PWD,  FTP_QUIT,
        FTP_REIN, FTP_REST, FTP_RETR, FTP_RMD,  FTP_RNFR, FTP_RNTO, FTP_SITE,
        FTP_SIZE, FTP_SMNT, FTP_STAT, FTP_STOR, FTP_STOU, FTP_STRU, FTP_SYST,
        FTP_TYPE, FTP_USER
};

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


/*
 * Logging functions.  Defined at log.c
 */
#ifdef DEBUG
#  define assert(cond)  if (!(cond)) warning("Assertion '" #cond "' failed")
   void debug (const char *, ...) __attribute__((format(printf,1,2)));
#else
#  define assert(cond)
   static inline void debug (const char *msg, ...) {}
#endif

void notice  (const char *, ...) __attribute__((format(printf,1,2)));
void warning (const char *, ...) __attribute__((format(printf,1,2)));
void error   (const char *, ...) __attribute__((format(printf,1,2)));
void fatal   (const char *, ...) __attribute__((format(printf,1,2), noreturn));


/*****************************  OTHER FUNCTIONS  *****************************/

void         init_session   (int cmd_sk);        /* init_session.c */
enum command next_command   (void);              /* next_command.c */
void         command_loop   (void);              /* command_loop.c */
void         change_dir     (void);              /* change_dir.c */
void         client_port    (void);              /* client_port.c */
void         send_file      (void);              /* send_file.c */
void         file_stats     (int type);          /* file_stats.c */
void         list_dir       (int full_list);     /* list_dir.c */
void         send_reply     (int sk, char *msg); /* misc.c */
int          path_is_secure (char *path);        /*  _/    */


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

