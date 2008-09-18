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
 * Large file support.
 */
#define _FILE_OFFSET_BITS   64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

/*
 * Hasefroch requirements.  No need to inclose within "#ifdef" as no other
 * system cares about these definitions.
 */
#define WINVER              0x0501
#define __MSVCRT_VERSION__  0x0601
#define _NO_OLDNAMES

/*
 * Early multiplaform support (more details on hase.h).
 */
#include <sys/types.h>
#ifdef __MINGW32__
#  include <winsock2.h>
   typedef __int64 off_t;
   typedef int socklen_t;
#  define CCP_CAST  (const char *)
#else
#  include <netinet/in.h>
#  define closesocket(s)  close(s)
#  define CCP_CAST
#endif

/*
 * Attributes help to catch silly mistakes, but they are not always available.
 */
#if !defined(__GNUC__) && !defined(__MINGW32__)
#  define __attribute__(x)
#endif

/*
 * Constants.
 */
#define DEFAULT_PORT 2211
#define LINE_SIZE    4096  /* Same value as PATH_MAX */

/*
 * Recognized commands.
 */
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

/*
 * Data channel modes.
 */
enum data_mode
{
        DEFAULT_MODE,  /* RFC default, unimplemented/unsupported */
        ACTIVE_MODE,   /* Active mode enabled with PORT requests */
        PASSIVE_MODE   /* Passive mode enabled with PASV requests */
};

/*
 * Session (client) state.
 */
struct _SessionScope
{
        /* Sockets */
        int  control_sk;    /* Control channel */
        int  data_sk;       /* Data channel */
        int  passive_sk;    /* Server socket for passive mode */

        /* Buffer offsets and fill counters */
        int  input_offset;  /* Input buffer data offset */
        int  input_len;     /* Bytes in input buffer */
        int  cwd_len;       /* Length of current working directory */

        /* Misc state information */
        int             pid;          /* Cached Process ID */
        enum data_mode  mode;         /* Current data channel mode */
        off_t           rest_offset;  /* Last REST offset accepted */
        char           *arg;          /* Pointer to comand line argument */

        /* Session addresses */
        struct sockaddr_in  port_destination;  /* Parsed PORT argument */
        struct sockaddr_in  local_address;     /* Control local IP */
        struct sockaddr_in  client_address;    /* Control peer IP */

        /* Buffers */
        char  input[LINE_SIZE];  /* Incoming command buffer */
        char  aux[LINE_SIZE];    /* Auxiliary buffer */
        char  cwd[LINE_SIZE];    /* Current Working Directory */
};

extern struct _SessionScope  SS;  /* SS --> Session State */


/*
 * Logging functions.  These functions are implemented in log.c
 */
#ifdef DEBUG
#  define assert(cond)  if (!(cond)) warning("Assertion '" #cond "' failed")
   void debug (const char *, ...) __attribute__((format(printf,1,2)));
#else
#  define assert(cond)
   static inline void debug (const char *msg, ...) {}
#endif

/*
 * These functions will be replaced by variadic macros when compilers other than
 * GCC can be tested.
 */
void notice  (const char *, ...) __attribute__((format(printf,1,2)));
void warning (const char *, ...) __attribute__((format(printf,1,2)));
void error   (const char *, ...) __attribute__((format(printf,1,2)));
void fatal   (const char *, ...) __attribute__((format(printf,1,2), noreturn));


/*
 * Reply functions, used to send data over the control and data channels
 * respectively.  Implemented in reply.c.
 */
void reply      (const char *, int);
int  data_reply (const char *, int);


/*
 * Other functions.  Each function declared here is implemented in a separate
 * file, with the same name as the function.  Functions sorted alphabetically.
 */
void         change_dir          (void);
void         command_loop        (void)  __attribute__((noreturn));
void         enable_passive      (void);
int          expand_arg          (void);
void         file_stats          (int);
void         init_session        (int);
void         list_dir            (int);
enum command next_command        (void);
int          open_data_channel   (void);
int          open_file           (off_t *);
void         parse_port_argument (void);
void         send_file           (void);


/*
 * Utility macro to call reply() with a constant string.  At compile time, the
 * length of these strings is known.
 */
#define reply_c(str)  reply(str, sizeof(str) - 1)

