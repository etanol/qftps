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
 * Common definitions.
 */

/* Large file support */
#define _FILE_OFFSET_BITS   64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <sys/types.h>

/* Attributes help to catch silly mistakes, but they are not always available */
#ifndef __GNUC__
#  define __attribute__(x)
#endif

/* Constants */
#define DEFAULT_PORT 2211
#define LINE_SIZE    4096  /* Same value as PATH_MAX */

/* Recognized commands */
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

/* Session (client) state */
struct _SessionScope
{
        /* Sockets */
        int    control_sk;        /* Control channel */
        int    data_sk;           /* Data channel */
        int    passive_sk;        /* Server socket for passive mode */

        /* Buffer offsets and fill counters */
        int    input_offset;      /* Input buffer data offset */
        int    input_len;         /* Bytes in input buffer */
        int    passive_len;       /* Length of passive reply */
        int    cwd_len;           /* Length of current working directory */

        /* Other state information */
        int    passive_mode;      /* Passive mode flag */
        off_t  file_offset;       /* Last REST offset accepted */
        char  *arg;               /* Pointer to comand line argument */

        /* Buffers */
        char   input[LINE_SIZE];  /* Incoming command buffer */
        char   aux[LINE_SIZE];    /* Auxiliary buffer */
        char   cwd[LINE_SIZE];    /* Current Working Directory */
        char   passive_str[64];   /* Cached reply for PASV */

};

extern struct _SessionScope  SS;  /* SS --> Session State */


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

void         init_session   (int cmd_sk);        /* session.c */
enum command next_command   (void);              /* next_command.c */
void         command_loop   (void);              /* command_loop.c */
void         client_port    (void);              /* client_port.c */
void         send_file      (void);              /* send_file.c */
void         file_stats     (int type);          /* file_stats.c */
void         list_dir       (int full_list);     /* list_dir.c */
void         change_dir     (void);              /* path.c */
int          expand_arg     (void);              /*  _/    */
void         read_request   (void);              /* control.c */
void         reply          (const char *, int); /*   _/      */

#define reply_c(str)  reply(str, sizeof(str) - 1)

